#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "centrallix.h"
#include "mtask.h"
#include "obj.h"
#include "xstring.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "mtlexer.h"

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
/* Module: 	lsmain.c               					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 4, 1998 					*/
/* Description:	Main startup code to start the Centrallix as a server	*/
/*		process waiting for network connections.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: lsmain.c,v 1.14 2002/03/23 06:26:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/lsmain.c,v $

    $Log: lsmain.c,v $
    Revision 1.14  2002/03/23 06:26:49  gbeeley
    Added BDQS network listener.  Be sure to cvs update the centrallix-os
    module to get a fresh copy of the centrallix.conf with the net_bdqs
    section in it, and be sure to cvs update the centrallix-lib module, as
    this module depends on it.

    Revision 1.13  2002/03/16 02:54:59  bones120
    This might help...

    Revision 1.12  2002/03/13 19:48:46  gbeeley
    Fixed a window-dragging issue with nested html windows.  Added the
    dropdown widget to lsmain.c.  Updated changelog.

    Revision 1.11  2002/03/09 02:42:01  bones120
    Initial commit of the spinner box.

    Revision 1.10  2002/03/09 02:38:48  jheth
    Make OSRC work with Form - Query at least

    Revision 1.9  2002/03/08 02:07:13  jorupp
    * initial commit of alerter widget
    * build callback listing object for form
    * form has many more of it's callbacks working

    Revision 1.8  2002/03/06 23:30:35  lkehresman
    Added form status widget.

    Revision 1.7  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.6  2002/02/14 00:55:20  gbeeley
    Added configuration file centrallix.conf capability.  You now MUST have
    this file installed, default is /usr/local/etc/centrallix.conf, in order
    to use Centrallix.  A sample centrallix.conf is found in the centrallix-os
    package in the "doc/install" directory.  Conf file allows specification of
    file locations, TCP port, server string, auth realm, auth method, and log
    method.  rootnode.type is now an attribute in the conf file instead of
    being a separate file, and thus is no longer used.

    Revision 1.5  2001/11/12 20:43:43  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

    Revision 1.4  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.3  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.2  2001/10/02 20:45:03  gbeeley
    New build/subbuild system; fixed Makefile to work better with it...

    Revision 1.1.1.1  2001/08/13 18:00:46  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:50  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** The following variables are used to access build/version/stability
 *** information so that other modules don't have to be rebuilt with
 *** every compile - just this file in particular.
 ***/
char* cx__version = VERSION;
int cx__build = BUILD;
int cx__subbuild = SUBBUILD;
char* cx__stability = STABILITY;
char* cx__years = YEARS;


/*** Instantiate the globals from centrallix.h 
 ***/
CxGlobals_t CxGlobals;

/*** start - this function is called by the MTASK module once the 
 *** mtInitialize() routine is finished intializing the first thread.  This
 *** is where processing really starts.  The purpose of this function is
 *** primarily to initialize the various modules and thus get things
 *** started.
 ***/
void
start(void* v)
    {
    pFile cxconf;
    pStructInf mss_conf;
    char* authmethod;
    char* authmethodfile;
    char* logmethod;
    char* logprog;
    int log_all_errors;
    char* ptr;

#ifndef LS_QUIET_INIT

	/** Startup message **/
	if (!CxGlobals.QuietInit)
	    {
	    printf("\n");
	    printf("Centrallix/%s build #%4.4d-%d [%s]\n\n", cx__version, cx__build, cx__subbuild, cx__stability);
	    printf("Copyright (C) %s LightSys Technology Services, Inc.\n", cx__years);
	    printf("An open source community developed project.  Provided with\n");
	    printf("ABSOLUTELY NO WARRANTY.  See the file 'COPYING' for details.\n");
	    printf("\n");
	    }

#endif


	/** Load the configuration file **/
	cxconf = fdOpen(CxGlobals.ConfigFileName, O_RDONLY, 0600);
	if (!cxconf)
	    {
	    printf("centrallix: could not open config file '%s'\n", CxGlobals.ConfigFileName);
	    thExit();
	    }
	CxGlobals.ParsedConfig = stParseMsg(cxconf, 0);
	if (!CxGlobals.ParsedConfig)
	    {
	    printf("centrallix: error parsing config file '%s'\n", CxGlobals.ConfigFileName);
	    thExit();
	    }
	fdClose(cxconf, 0);

	/** Init the session handler.  We have to extract the config data for this 
	 ** module ourselves, because mtsession is in the centrallix-lib, and thus can't
	 ** use the new stparse module's routines.
	 **/
	mss_conf = stLookup(CxGlobals.ParsedConfig, "mtsession");
	if (stAttrValue(stLookup(mss_conf,"auth_method"),NULL,&authmethod,0) < 0) authmethod = "system";
	if (stAttrValue(stLookup(mss_conf,"altpasswd_file"),NULL,&authmethodfile,0) < 0) authmethodfile = "/usr/local/etc/cxpasswd";
	if (stAttrValue(stLookup(mss_conf,"log_method"),NULL,&logmethod,0) < 0) logmethod = "stdout";
	if (stAttrValue(stLookup(mss_conf,"log_progname"),NULL,&logprog,0) < 0) logprog = "centrallix";
	log_all_errors = 0;
	if (stAttrValue(stLookup(mss_conf,"log_all_errors"),NULL,&ptr,0) < 0 || !strcmp(ptr,"yes")) log_all_errors = 1;
	mssInitialize(authmethod, authmethodfile, logmethod, log_all_errors, logprog);

	/** Initialize the various parts **/
	nmInitialize();				/* memory manager */
	expInitialize();			/* expression processor/compiler */
	if (objInitialize() < 0) thExit();	/* OSML */
	snInitialize();				/* Node structure file handler */

	/** Init the multiquery system and drivers **/
	mqInitialize();				/* MultiQuery system */
	mqtInitialize();			/* tablegen query module */
	mqpInitialize();			/* projection query module */
	mqjInitialize();			/* join query module */

	/** Init the objectsystem drivers **/
	uxdInitialize();			/* UNIX filesystem driver */
	sybdInitialize();			/* Sybase CT-lib driver */
	stxInitialize();			/* Structure file driver */
	qytInitialize();			/* Query Tree driver */
	rptInitialize();			/* report writer driver */
	uxpInitialize();			/* UNIX printer access driver */
	datInitialize();			/* flat ascii datafile (CSV, etc) */
	uxuInitialize();			/* UNIX users list driver */
	audInitialize();			/* Audio file player driver */

	/** Init the html-generation subsystem **/
	htrInitialize();			/* HTML generator */
	htpageInitialize();			/* DHTML page generator */
	htspaneInitialize();			/* scrollable pane module */
	httreeInitialize();			/* treeview module */
	hthtmlInitialize();			/* html pane module */
	htconnInitialize();			/* connector nonvisual module */
	htibtnInitialize();			/* image button module */
	httbtnInitialize();			/* text button module */
	htmenuInitialize();			/* dropdown/popup menu module */
	htsetInitialize();			/* frameset module */
	htvblInitialize();			/* variable nonvisual module */
	httabInitialize();			/* tab control / tab page module */
	htpnInitialize();			/* pane module */
	httblInitialize();			/* tabular data module */
	htwinInitialize();			/* draggable window module */
	htcbInitialize();			/* checkbox module */
	htrbInitialize();			/* radiobutton module */
	htebInitialize();			/* editbox module */
	httmInitialize();			/* timer nonvisual module */
	htexInitialize();			/* method exec module */
	htspnrInitialize();			/* spinner box module*/
	htfsInitialize();			/* form status module */
	htddInitialize();			/* dropdown htdrv module */

	htformInitialize();			/* forms module */
	htosrcInitialize();			/* osrc module */
	htalrtInitialize();			/* alert module */

	/** Init the reporting content drivers **/
	pclInitialize();			/* PCL report generator */
	htpInitialize();			/* HTML report generator */
	fxpInitialize();			/* Epson FX report generator */

	nmRegister(sizeof(XString),"XString");
	nmRegister(sizeof(XArray),"XArray");
	nmRegister(sizeof(XHashTable),"XHashTable");
	nmRegister(sizeof(XHashEntry),"XHashEntry");
	nmRegister(sizeof(StructInf),"StructInf");
	nmRegister(sizeof(LxSession),"LxSession");
	nmRegister(sizeof(MTObject),"MTObject");
	nmRegister(sizeof(File),"File");
	nmRegister(sizeof(EventReq),"EventReq");
	nmRegister(sizeof(Thread),"Thread");

	/** Init the network listeners. **/
#ifdef NHT_ENABLE
	nhtInitialize();			/* HTTP network listener */
#endif
#ifdef CGI_ENABLE
	cgiInitialize();			/* CGI "listener" */
#endif

	bnetInitialize();			/* BDQS network listener */

    thExit();
    }


/*** main - called from the C runtime to start the program.
 ***/
int 
main(int argc, char* argv[])
    {
    int ch;

	/** Default global values **/
	strcpy(CxGlobals.ConfigFileName, "/usr/local/etc/centrallix.conf");
	CxGlobals.QuietInit = 0;
	CxGlobals.ParsedConfig = NULL;
    
	/** Check for config file options on the command line **/
	while ((ch=getopt(argc,argv,"hc:q")) > 0)
	    {
	    switch (ch)
	        {
		case 'c':	memccpy(CxGlobals.ConfigFileName, optarg, 0, 255);
				CxGlobals.ConfigFileName[255] = '\0';
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'h':	printf("Usage:  centrallix [-c <config-file>]\n");
				exit(0);

		case '?':
		default:	printf("Usage:  centrallix [-c <config-file>]\n");
				exit(1);
		}
	    }

	/** Init the multithreading module to start the first thread **/
	/** 'start' is the name of the function to be the first thread **/
	mtInitialize(0, start);

    return 0; /* never reached */
    }

