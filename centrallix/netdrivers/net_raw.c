#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/exception.h"
#include "obj.h"
#include "stparse.h"
#include "cxlib/net_raw_api.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	net_raw.c               				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	May 1st, 2001     					*/
/* Description:	Network handler providing a raw socket interface to	*/
/*		Centrallix.  This interface is primarily used to 	*/
/*		interact with other Centrallix servers, but could be	*/
/*		used, for instance, by an ODBC driver.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: net_raw.c,v 1.2 2005/02/26 06:42:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_raw.c,v $

    $Log: net_raw.c,v $
    Revision 1.2  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.1.1.1  2001/08/13 18:00:57  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:22  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Structure associated with one session for a user ***/
typedef struct
    {
    int			ChannelID;
    pObjSession		Session;
    XArray		Objects;
    XArray		Queries;
    }

/*** Structure for one authenticated user within the connection ***/
typedef struct
    {
    int			ChannelID;
    char		Username[32];
    char		Password[32];
    XArray		Sessions;
    }
    ConnUser;

/*** Structure handling data for an open connection ***/
typedef struct
    {
    pFile		FD;
    int			LocalReqVersionID;
    int			RemoteReqVersionID;
    int			UsedVersionID;
    XArray		Users;
    }
    Connection;



/*** net_internal_ConnHandler - manages a single connection.
 ***/
void
net_internal_ConnHandler(void* conn_v)
    {
    pFile conn = (pFile)conn_v;

    	/** Set the thread's name **/
	thSetName(NULL,"RAW Network Handler");

    thExit();
    }


/*** net_internal_Handler - manages incoming HTTP connections and sends
 *** the appropriate documents/etc to the requesting client.
 ***/
void
net_internal_Handler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;

	/** Set the thread's name **/
	thSetName(NULL,"RAW Network Listener");

    	/** Open the server listener socket. **/
	listen_socket = netListenTCP("801", 32, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NET","Could not open network listener");
	    thExit();
	    }
	
	/** Loop, accepting requests **/
	while(connection_socket = netAcceptTCP(listen_socket,0))
	    {
	    if (!thCreate(net_internal_ConnHandler, 0, connection_socket))
	        {
		mssError(1,"NET","Could not create thread to handle connection!");
		netCloseTCP(connection_socket,0,0);
		}
	    }

	/** Exit. **/
	mssError(1,"NET","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }


/*** netInitialize - initialize the RAW network handler by starting
 *** the connection-listener thread.
 ***/
int
netInitialize()
    {
    int i;

	/** Initialize globals **/
	memset(&NET, 0, sizeof(NET));

	/** Start the network listener. **/
	thCreate(net_internal_Handler, 0, NULL);

    return 0;
    }

