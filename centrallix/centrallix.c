#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif
#include "centrallix.h"
#include "mtask.h"
#include "obj.h"
#include "xstring.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "mtlexer.h"
#include <signal.h>


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2002 LightSys Technology Services, Inc.		*/
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
/* Module: 	centrallix.c           					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	May 1, 2002      					*/
/* Description:	This module contains common startup code which is used	*/
/*		by both test_obj and lsmain.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: centrallix.c,v 1.18 2003/04/03 21:01:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/centrallix.c,v $

    $Log: centrallix.c,v $
    Revision 1.18  2003/04/03 21:01:23  gbeeley
    Fixed make depend to include header dependencies on centrallix-lib
    headers that sometimes change during development.  Fixed autoconf
    settings to correctly detect default centrallix-lib directories.
    Had centrallix.c exit() instead of _exit() since it is truly
    terminating the process, thus allowing gmon.out to be written.

    Revision 1.17  2003/03/30 22:49:24  jorupp
     * get rid of some compile warnings -- compiles with zero warnings under gcc 3.2.2

    Revision 1.16  2003/03/09 18:59:13  jorupp
     * add SIGINT handling, which calls shutdown handlers

    Revision 1.15  2003/01/08 18:01:33  gbeeley
    Oops!  Where'd that one go?  Evidently I left out an init call when I
    created this file.

    Revision 1.14  2002/12/24 09:51:56  jorupp
     * yep, this is what it looks like -- inital commit of the terminal widget :)
       -- the first Mozilla-only widget
     * it's not even close to a 'real' form yet -- mozilla takes so much CPU time
       rendering the table that it's pretty useless

    Revision 1.13  2002/10/18 01:48:45  gbeeley
    Added attribute-based enable/disable module, so that structure file
    editors can turn them on and off and not ignore them as a result of
    a module being commented out.  This will also come in handy if we
    want autoconf to automatically turn them on/off when make config is
    done.

    Revision 1.12  2002/09/24 09:53:58  jorupp
     * check to ensure that libdl is there before including the header for it

    Revision 1.11  2002/08/24 04:38:31  gbeeley
    We are still having trouble with conflicts between cxlibconfig.h and
    config.h - they both end up being included because mtask.h is including
    cxlibconfig.h.  The only really clean way to handle this seems to be
    to have both private and public .h files in centrallix-lib.  This however
    is a remedial step solving the version-printing problem.

    Revision 1.10  2002/08/16 03:48:29  jorupp
     * removed a debugging statement I added a little while ago

    Revision 1.9  2002/08/16 03:39:42  jorupp
     * fixed a bug in the configure script that was causing the 'autodetection' of the
        support of dynamic loading to always fail

    Revision 1.8  2002/08/16 03:09:44  jorupp
     * added the ability to disable the dynamic loading modules -- needed under cygwin
       -- centrallix now compiles and runs under cygwin (without dynamic loading modules)

    Revision 1.7  2002/08/12 09:14:28  mattphillips
    Use the built-in PACKAGE_VERSION instead of VERSION to get the current version
    number to be more standard.  PACKAGE_VERSION is set by autoconf, but read from
    .version when configure is generated.

    Revision 1.6  2002/08/08 21:58:36  pfinley
    changes to Makefile & centrallix.c for the clock widget.

    Revision 1.5  2002/07/29 01:39:18  jorupp
     * prints error message explaining itself if it fails loading a module

    Revision 1.4  2002/07/12 15:04:20  pfinley
    Multiline textarea widget initial commit (still a some bugs)

    Revision 1.3  2002/07/09 14:09:03  lkehresman
    Added first revision of the datetime widget.  No form interatction, and no
    time setting functionality, only date.  This has been on my laptop for a
    while and I wanted to get it into CVS for backup purposes.  More functionality
    to come soon.

    Revision 1.2  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.1  2002/05/02 01:14:56  gbeeley
    Added dynamic module loading support in Centrallix, starting with the
    Sybase driver, using libdl.


 **END-CVSDATA***********************************************************/


/*** The following variables are used to access build/version/stability
 *** information so that other modules don't have to be rebuilt with
 *** every compile - just this file in particular.
 ***/
char* cx__version = PACKAGE_VERSION;
int cx__build = BUILD;
int cx__subbuild = SUBBUILD;
char* cx__stability = STABILITY;
char* cx__years = YEARS;


/*** Instantiate the globals from centrallix.h 
 ***/
CxGlobals_t CxGlobals;
int CxSupportedInterfaces[] = {1, 0};

int
cx_internal_LoadModules(char* type)
    {
#ifdef WITH_DYNAMIC_LOAD
    pStructInf modules_inf;
    pStructInf one_module;
    char* modtype;
    char* modpath;
    char* modenable;
    pCxModule moduledata;
    int i,j,found;
    int n_loaded = 0;

	/** Find the modules section in the configuration file.  It is not
	 ** an error if the section is not there - just no module loading
	 ** will be done.  But, we'll warn about it anyhow.
	 **/
	modules_inf = stLookup(CxGlobals.ParsedConfig, "modules");
	if (!modules_inf) 
	    {
	    mssError(1,"CX","Warning - no modules section in config file");
	    return 0;
	    }

	/** Go through the modules list, looking for appropriate ones **/
	for(i=0;i<modules_inf->nSubInf;i++)
	    {
	    one_module = modules_inf->SubInf[i];
	    modtype=NULL;
	    modpath=NULL;
	    stAttrValue(stLookup(one_module,"path"),NULL,&modpath,0);
	    stAttrValue(stLookup(one_module,"type"),NULL,&modtype,0);
	    if (modtype && strcmp(modtype,type)) continue;
	    if (!modpath || !modtype)
		{
		mssError(1,"CX","Module '%s' must have path and type settings; load failed",one_module->Name);
		continue;
		}

	    /** Don't load it if it is disabled. **/
	    modenable = NULL;
	    stAttrValue(stLookup(one_module,"enable_module"), NULL, &modenable, 0);
	    if (modenable && !strcasecmp(modenable, "no"))
		{
		if (!CxGlobals.QuietInit)
		    {
		    printf("mod: %s %s disabled\n", modtype, one_module->Name);
		    }
		continue;
		}

	    /** Setup a structure for this fella **/
	    moduledata = (pCxModule)nmMalloc(sizeof(CxModule));
	    if (!moduledata) continue;
	    memset(moduledata, 0, sizeof(CxModule));

	    /** Try to load the bugger. **/
	    moduledata->dlhandle = dlopen(modpath, RTLD_NOW);
	    if (!moduledata->dlhandle)
		{
		nmFree(moduledata, sizeof(CxModule));
		mssError(1,"CX","Module '%s' could not be loaded",modpath);
		mssError(1,"CX","ERROR: %s",dlerror());
		continue;
		}

	    /** Lookup some key symbols. **/
	    moduledata->InitFn = dlsym(moduledata->dlhandle, "moduleInitialize");
	    if (!moduledata->InitFn)
		{
		dlclose(moduledata->dlhandle);
		nmFree(moduledata, sizeof(CxModule));
		mssError(1,"CX","Module '%s' has no Init function declared: %s",modpath, dlerror());
		continue;
		}
	    moduledata->DeinitFn = dlsym(moduledata->dlhandle, "moduleDeInitialize");
	    moduledata->Prefix = dlsym(moduledata->dlhandle, "modulePrefix");
	    moduledata->Description = dlsym(moduledata->dlhandle, "moduleDescription");
	    moduledata->Version = dlsym(moduledata->dlhandle, "moduleVersion");
	    moduledata->InterfaceVer = dlsym(moduledata->dlhandle, "moduleInterface");
	    if (!moduledata->InterfaceVer)
		{
		dlclose(moduledata->dlhandle);
		nmFree(moduledata, sizeof(CxModule));
		mssError(1,"CX","Module '%s' has no Interface Version declared: %s",modpath,dlerror());
		continue;
		}

	    /** Is the interface supported? **/
	    for(found=j=0;CxSupportedInterfaces[j];j++)
		{
		if (*(moduledata->InterfaceVer) == CxSupportedInterfaces[j])
		    {
		    found = 1;
		    break;
		    }
		}
	    if (!found)
		{
		dlclose(moduledata->dlhandle);
		nmFree(moduledata, sizeof(CxModule));
		mssError(1,"CX","Module '%s' has unsupported interface version %d",
		    modpath, *(moduledata->InterfaceVer));
		continue;
		}

	    /** Attempt to initialize the module **/
	    if (moduledata->InitFn() < 0)
		{
		dlclose(moduledata->dlhandle);
		nmFree(moduledata, sizeof(CxModule));
		mssError(0,"CX","Module '%s' load failed",modpath);
		continue;
		}
	    
	    /** Ok, add this one to the list of modules. **/
	    moduledata->Next = CxGlobals.ModuleList;
	    CxGlobals.ModuleList = moduledata;
	    if (!CxGlobals.QuietInit)
		{
		printf("mod: %s %s (%s, %d.%d.%d)\n",
		    modtype,
		    moduledata->Prefix?*(moduledata->Prefix):"unknown",
		    moduledata->Description?*(moduledata->Description):"-",
		    moduledata->Version?((*(moduledata->Version))>>24):0,
		    moduledata->Version?(((*(moduledata->Version))>>16)&0xFF):0,
		    moduledata->Version?((*(moduledata->Version))&0xFFFF):0);
		}
	    n_loaded++;
	    }

	if (!CxGlobals.QuietInit && n_loaded > 0) printf("\n");
#endif

    return 0;
    }


/** cxShutdownThread - thread created from the SIGINT handler, 
***   call all registered shutdown handlers then exits
**/
void cxShutdownThread(void *v)
    {
    int i;
    mssError(0,"CN","Centrallix is shutting down");
    for(i=0;i<xaCount(&CxGlobals.ShutdownHandlers);i++)
	{
	ShutdownHandlerFunc handler = (ShutdownHandlerFunc) xaGetItem(&CxGlobals.ShutdownHandlers,i);
	if(handler)
	    handler();
	}
    exit(0);
    thExit();
    }


/** cxShutdown - handler for SIGINT
**/
void cxShutdown(int signal)
    {
    if(CxGlobals.ShuttingDown)
	return;
    CxGlobals.ShuttingDown = 1;
    thCreate(cxShutdownThread,0,NULL);
    }

int cxAddShutdownHandler(ShutdownHandlerFunc handler)
    {
    return xaAddItem(&CxGlobals.ShutdownHandlers,handler);
    }

int
cxInitialize(void* v)
    {
    pFile cxconf;
    pStructInf mss_conf;
    char* authmethod;
    char* authmethodfile;
    char* logmethod;
    char* logprog;
    int log_all_errors;
    char* ptr;

	CxGlobals.ShuttingDown = 0;
	xaInit(&CxGlobals.ShutdownHandlers,4);

	/** set up the interrupt handler so we can shutdown properly **/
	signal(SIGINT,cxShutdown);

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
	if (objInitialize() < 0) return -1;	/* OSML */
	snInitialize();				/* Node structure file handler */

	/** Let newmalloc in on the names of some structures **/
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

	/** Init the multiquery system and drivers **/
	mqInitialize();				/* MultiQuery system */
	mqtInitialize();			/* tablegen query module */
	mqpInitialize();			/* projection query module */
	mqjInitialize();			/* join query module */

	/** Init the objectsystem drivers **/
	uxdInitialize();			/* UNIX filesystem driver */
#if 0
	sybdInitialize();			/* Sybase CT-lib driver */
#endif
	stxInitialize();			/* Structure file driver */
	qytInitialize();			/* Query Tree driver */
	rptInitialize();			/* report writer driver */
	uxpInitialize();			/* UNIX printer access driver */
	datInitialize();			/* flat ascii datafile (CSV, etc) */
	uxuInitialize();			/* UNIX users list driver */
	audInitialize();			/* Audio file player driver */

	/** Init the reporting content drivers **/
	pclInitialize();			/* PCL report generator */
	htpInitialize();			/* HTML report generator */
	fxpInitialize();			/* Epson FX report generator */
	txtInitialize();			/* text only report gen */

#ifndef WITH_DYNAMIC_LOAD
	/** Init the modules being used if dynamic loading is disabled **/
	
#ifdef USE_DBL
	dblInitialize();
#endif

#ifdef USE_HTTP
	httpInitialize();
#endif

#ifdef USE_MIME
	mimeInitialize();
#endif

#ifdef USE_SYBASE
	sybdInitialize();
#endif

#ifdef USE_XML
	xmlInitialize();
#endif

#endif

	/** Load any osdriver and querydriver modules. **/
	cx_internal_LoadModules("osdriver");
	cx_internal_LoadModules("qydriver");

    return 0;
    }


#ifndef CX_NO_HTMLGEN
int
cxHtInit()
    {

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
	htdtInitialize();			/* datetime htdrv module */
	httxInitialize();			/* textarea htdrv module */
	htclInitialize();			/* clock htdrv module */

	htformInitialize();			/* forms module */
	htosrcInitialize();			/* osrc module */
	htalrtInitialize();			/* alert module */
	htlblInitialize();			/* label module */
	httermInitialize();			/* terminal module */

	/** Load any htdriver modules **/
	cx_internal_LoadModules("htdriver");

    return 0;
    }
#endif

#ifndef CX_NO_NETWORK
int
cxNetworkInit()
    {

	/** Init the network listeners. **/
#ifdef NHT_ENABLE
	nhtInitialize();			/* HTTP network listener */
#endif
#ifdef CGI_ENABLE
	cgiInitialize();			/* CGI "listener" */
#endif

	bnetInitialize();			/* BDQS network listener */

	/** Load any network driver modules **/
	cx_internal_LoadModules("netdriver");

    return 0;
    }
#endif
