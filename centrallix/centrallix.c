#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
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

    $Id: centrallix.c,v 1.1 2002/05/02 01:14:56 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/centrallix.c,v $

    $Log: centrallix.c,v $
    Revision 1.1  2002/05/02 01:14:56  gbeeley
    Added dynamic module loading support in Centrallix, starting with the
    Sybase driver, using libdl.


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
int CxSupportedInterfaces[] = {1, 0};

int
cx_internal_LoadModules(char* type)
    {
    pStructInf modules_inf;
    pStructInf one_module;
    char* modtype;
    char* modpath;
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

    return 0;
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

	/** Load any osdriver and querydriver modules. **/
	cx_internal_LoadModules("osdriver");
	cx_internal_LoadModules("qydriver");

    return 0;
    }



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

	htformInitialize();			/* forms module */
	htosrcInitialize();			/* osrc module */
	htalrtInitialize();			/* alert module */
	htlblInitialize();			/* label module */

	/** Load any htdriver modules **/
	cx_internal_LoadModules("htdriver");

    return 0;
    }


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

