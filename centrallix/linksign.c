#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include "cxlib/mtask.h"
#include "cxlib/strtcpy.h"
#include "cxlib/mtsession.h"
#include "cxlib/xstring.h"
#include "cxlib/strtcpy.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
#include "cxss/cxss.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2024 LightSys Technology Services, Inc.		*/
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
/* Module:	linksign.c                                              */
/* Author:	Greg Beeley                                             */
/* Date:	05-Apr-2024                                             */
/*									*/
/* Description:	This utility signs a link so that it will be recognized	*/
/*		by the Centrallix server as a trusted link, thus able   */
/*		to bypass the same-origin checks in the browser.        */
/************************************************************************/

struct
    {
    char    Link[256];
    }
    LINKSIGN;


void
start(void* v)
    {
    pStructInf my_config;
    pXString url_xs;

	if (cxInitialize() < 0) thExit();

	/** Link signing config is in the http server section **/
	my_config = stLookup(CxGlobals.ParsedConfig, "net_http");
	if (!my_config)
	    {
	    puts("no net_http section in configuration, exiting.");
	    exit(1);
	    }
	cxLinkSigningSetup(my_config);

	/** No file specified? **/
	if (!LINKSIGN.Link[0]) 
	    {
	    puts("no link specified to sign (use option -l).");
	    exit(1);
	    }

	/** Sign the link **/
	url_xs = cxssLinkSign(LINKSIGN.Link);
	if (!url_xs)
	    {
	    puts("could not sign link.");
	    exit(1);
	    }

	puts(xsString(url_xs));
	xsFree(url_xs);
	
    exit(0);
    }


int 
main(int argc, char* argv[])
    {
    int ch;

	/** Default global values **/
	strcpy(CxGlobals.ConfigFileName, CENTRALLIX_CONFIG);
	CxGlobals.QuietInit = 0;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
	CxGlobals.ArgV = argv;
	CxGlobals.Flags = 0;
	memset(&LINKSIGN, 0, sizeof(LINKSIGN));

	/** Check for config file options on the command line **/
	while ((ch=getopt(argc,argv,"c:l:q?h")) > 0)
	    {
	    switch (ch)
	        {
		case 'c':	strtcpy(CxGlobals.ConfigFileName, optarg, sizeof(CxGlobals.ConfigFileName));
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'l':	strtcpy(LINKSIGN.Link, optarg, sizeof(LINKSIGN.Link));
				break;

		default:
		case '?':
		case 'h':	printf("Usage:  linksign [-c configfile] [-l link-to-sign] [-q]\n");
				exit(0);
		}
	    }

	mtInitialize((CxGlobals.QuietInit)?MT_F_QUIET:0, start);

    return 0;
    }

