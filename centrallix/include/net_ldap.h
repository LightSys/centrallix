#ifndef _NET_LDAP_H
#define _NET_LDAP_H

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
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/exception.h"
#include "cxlib/memstr.h"
#include "cxlib/xstring.h"
#include "obj.h"
#include "stparse_ne.h"
#include "stparse.h"
#include "cxlib/xhandle.h"
#include "cxlib/magic.h"
#include "cxlib/util.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
#include "cxss/cxss.h"
#include <lber.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2017 LightSys Technology Services, Inc.		*/
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
/* Module: 	net_ldap.h, net_ldap.c,					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 28, 2017    					*/
/* Description:	Network handler providing an LDAP interface to the 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/


/*** Connection data ***/
typedef struct
    {
    pFile	ConnFD;
    int		Port;
    char	Username[32];
    char	Password[32];
    char	IPAddr[20];
    pFile	ReportingFD;
    int		SSLpid;
    int		UsingTLS:1;
    Sockbuf *	LberSockbuf;
    Sockbuf_IO	LberIo;
    ber_int_t	RequestID;
    int		AppID;
    ber_int_t	Version;
    char*	RequestorDN;
    char	ResponseTime[48];
    }
    NldapConn, *pNldapConn;


/*** GLOBALS ***/
struct 
    {
    int		InactivityTime;
    int		RestrictToLocalhost;
    pFile	AccessLogFD;
    char	AccessLogFile[256];
    int		ClkTck;
    SSL_CTX*	SSL_ctx;
    }
    NLDAP;

void nht_i_TLSHandler(void* v);
void nht_i_Handler(void* v);
int nht_i_CheckAccessLog();

#endif
