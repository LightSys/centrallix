#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h> //for regex functions
#include <regex.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_LIBZ 1
#endif

#include "centrallix.h"

/** xdr/rpc stuff **/
#include <rpc/xdr.h>
#include <rpc/rpc_msg.h>

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
/* Module: 	net_nfs.c                 				*/
/* Author:	Jonathan Rupp, Nathan Ehresman, Luke Ehresman,		*/
/*		Michael Rivera, Corey Cooper				*/
/* Creation:	February 19, 2003  					*/
/* Description:	Network handler providing an NFS interface to	 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: net_nfs.c,v 1.1 2003/02/23 21:56:59 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_nfs.c,v $

    $Log: net_nfs.c,v $
    Revision 1.1  2003/02/23 21:56:59  jorupp
     * added configure and makefile support for net_nfs
     * added basic shell of net_nfs, including the XDR parsing of the RPC headers
       -- it will listen for requests (mount & nfs) and print out some of the decoded RPC headers


 **END-CVSDATA***********************************************************/

#define MAX_PACKET_SIZE 16384

/*** GLOBALS ***/
struct 
    {
    }
    NNFS;

    
/*** nnfs_internal_nfs_listener - listens for and processes nfs requests
 ***   RFC 1094
 ***/
void
nnfs_internal_nfs_listener(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    int i;
    char *buf;
    char *remotehost;
    int remoteport;
    struct sockaddr_in remoteaddr;

    	/*printf("Handler called, stack ptr = %8.8X\n",&listen_socket);*/

	/** Set the thread's name **/
	thSetName(NULL,"NFS Listener");

	/** Get our configuration **/
	strcpy(listen_port,"5001");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_nfs");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the UDP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "nfs_port"), &intval, &strval, 0) >= 0)
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
	listen_socket = netListenUDP(listen_port, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NNFS","Could not open nfs listener");
	    thExit();
	    }
	
	buf = (char*)nmMalloc(MAX_PACKET_SIZE);
	/** Loop, accepting requests **/
	while((i=netRecvUDP(listen_socket,buf,MAX_PACKET_SIZE,0,&remoteaddr,&remotehost,&remoteport)) != -1)
	    {
	    XDR xdr;
	    struct rpc_msg msg;
	    /** process packet **/
	    printf("%i bytes recieved from: %s:%i\n",i,remotehost,remoteport);
	    xdrmem_create(&xdr,buf,i,XDR_DECODE);
	    if(!xdr_callmsg(&xdr,&msg))
		{
		mssError(0,"NNFS","unable to retrieve message");
		xdr_destroy(&xdr);
		continue;
		}
	    printf("xid: %lu\n",msg.rm_xid);
	    printf("msg_type: %i\n",msg.rm_direction);
	    if(msg.rm_direction==0)
		{
		printf("rpc version: %lu\n",msg.rm_call.cb_rpcvers);
		printf("prog: %lu\n",msg.rm_call.cb_prog);
		printf("version: %lu\n",msg.rm_call.cb_vers);
		printf("procedure: %lu\n",msg.rm_call.cb_proc);
		printf("auth flavor: %i\n",msg.rm_call.cb_cred.oa_flavor);
		printf("bytes of auth data: %u\n",msg.rm_call.cb_cred.oa_length);
		}
	    printf("current position: %u\n",xdr_getpos(&xdr));
	    xdr_free((xdrproc_t)xdr_callmsg,(char*)&msg);
	    xdr_destroy(&xdr);
	    }
	nmFree(buf,MAX_PACKET_SIZE);

	/** Exit. **/
	mssError(1,"NNFS","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }

/*** nnfs_internal_mount_listener - listens for and processes mount requests
 ***   RFC 1094
 ***/
void
nnfs_internal_mount_listener(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    int i;
    char *buf;
    char *remotehost;
    int remoteport;
    struct sockaddr_in remoteaddr;

    	/*printf("Handler called, stack ptr = %8.8X\n",&listen_socket);*/

	/** Set the thread's name **/
	thSetName(NULL,"NFS Listener");

	/** Get our configuration **/
	strcpy(listen_port,"5000");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_nfs");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the UDP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "mount_port"), &intval, &strval, 0) >= 0)
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
	listen_socket = netListenUDP(listen_port, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NNFS","Could not open mount listener");
	    thExit();
	    }
	
	buf = (char*)nmMalloc(MAX_PACKET_SIZE);
	/** Loop, accepting requests **/
	while((i=netRecvUDP(listen_socket,buf,MAX_PACKET_SIZE,0,&remoteaddr,&remotehost,&remoteport)) != -1)
	    {
	    XDR xdr;
	    struct rpc_msg msg;
	    /** process packet **/
	    printf("%i bytes recieved from: %s:%i\n",i,remotehost,remoteport);
	    xdrmem_create(&xdr,buf,i,XDR_DECODE);
	    if(!xdr_callmsg(&xdr,&msg))
		{
		mssError(0,"NNFS","unable to retrieve message");
		xdr_destroy(&xdr);
		continue;
		}
	    printf("xid: %lu\n",msg.rm_xid);
	    printf("msg_type: %i\n",msg.rm_direction);
	    if(msg.rm_direction==0)
		{
		printf("rpc version: %lu\n",msg.rm_call.cb_rpcvers);
		printf("prog: %lu\n",msg.rm_call.cb_prog);
		printf("version: %lu\n",msg.rm_call.cb_vers);
		printf("procedure: %lu\n",msg.rm_call.cb_proc);
		printf("auth flavor: %i\n",msg.rm_call.cb_cred.oa_flavor);
		printf("bytes of auth data: %u\n",msg.rm_call.cb_cred.oa_length);
		}
	    printf("current position: %u\n",xdr_getpos(&xdr));
	    xdr_free((xdrproc_t)xdr_callmsg,(char*)&msg);
	    xdr_destroy(&xdr);
	    }
	nmFree(buf,MAX_PACKET_SIZE);

	/** Exit. **/
	mssError(1,"NNFS","Mount listener could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }


/*** nhtInitialize - initialize the HTTP network handler and start the 
 *** listener thread.
 ***/
int
nnfsInitialize()
    {
	/** Start the mountd listener **/
	thCreate(nnfs_internal_mount_listener, 0, NULL);

	/** Start the nfs listener. **/
	thCreate(nnfs_internal_nfs_listener, 0, NULL);

	/** Start the request handler **/
	//thCreate(nnfs_internal_request_handler, 0, NULL);

    return 0;
    }

MODULE_INIT(nnfsInitialize);
MODULE_PREFIX("nnfs");
MODULE_DESC("NFS Network Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

