#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include "centrallix.h"
#include "cxlib/bdqs.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/exception.h"
#include "obj.h"
#include "stparse_ne.h"
#include "stparse.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	net_bdqs.c              				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 23, 2002    					*/
/* Description:	Network handler providing the BDQS protocol spec.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: net_bdqs.c,v 1.2 2005/02/26 06:42:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_bdqs.c,v $

    $Log: net_bdqs.c,v $
    Revision 1.2  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.1  2002/03/23 06:26:49  gbeeley
    Added BDQS network listener.  Be sure to cvs update the centrallix-os
    module to get a fresh copy of the centrallix.conf with the net_bdqs
    section in it, and be sure to cvs update the centrallix-lib module, as
    this module depends on it.

 **END-CVSDATA***********************************************************/


/*** bnet_internal_CloseConn - callback used for closing up a network
 *** connection, in this case bdqs over tcp/ip.
 ***/
int
bnet_internal_CloseConn(void* rconn, void* wconn)
    {
    pFile conn = (pFile)rconn;
    return netCloseTCP(conn, 1000, 0);
    }


/*** bnet_internal_Handler - manages incoming BDQS connections.
 ***/
void
bnet_internal_Handler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;

	/** Set the thread's name **/
	thSetName(NULL,"BDQS Network Listener");

	/** Get our configuration **/
	strcpy(listen_port,"808");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_bdqs");
	if (my_config)
	    {
	    /** Got the config.  Check to see if BDQS is enabled **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "enable_bdqs"), NULL, &strval, 0) < 0 || !strval || !strcmp(strval,"no"))
		{
		/** Nope. **/
		thExit();
		}

	    /** Now lookup what the TCP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "listen_port"), &intval, &strval, 0) >= 0)
		{
		if (strval)
		    {
		    memccpy(listen_port, strval, 0, 31);
		    listen_port[31] = '\0';
		    }
		else
		    {
		    snprintf(listen_port,32,"%d",intval);
		    }
		}
	    }

    	/** Open the server listener socket. **/
	listen_socket = netListenTCP(listen_port, 32, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"BNET","Could not open network listener");
	    thExit();
	    }
	
	/** Loop, accepting requests **/
	while((connection_socket = netAcceptTCP(listen_socket,0)))
	    {
	    if (bdqsStartServer(fdRead,fdWrite,connection_socket,connection_socket,
		    NULL,NULL,NULL,NULL,bnet_internal_CloseConn, 1400, 
		    BDQS_PROTO_F_LSB | BDQS_PROTO_F_MULTIQUERY | BDQS_PROTO_F_SELECT |
		    BDQS_PROTO_F_COLLATE | BDQS_PROTO_F_EVAL | BDQS_PROTO_F_EVENTS) < 0)
	        {
		mssError(1,"BNET","Could not start BDQS server on the connection!");
		netCloseTCP(connection_socket,0,0);
		}
	    }

	/** Exit. **/
	mssError(1,"BNET","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }


/*** bnetInitialize() - init the BDQS Network handler and start the listener.
 ***/
int
bnetInitialize()
    {

	/** Initialize globals **/

	/** Start the network listener. **/
	thCreate(bnet_internal_Handler, 0, NULL);

    return 0;
    }

