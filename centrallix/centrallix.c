#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <locale.h>
#include <langinfo.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif
#include "centrallix.h"
#include "cxlib/mtask.h"
#include "obj.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "cxlib/mtlexer.h"
#include "cxlib/qprintf.h"
#include <signal.h>
#include "wgtr.h"
#include "iface.h"
#include "cxss/cxss.h"
#include "charsets.h"
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

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
int CxSupportedInterfaces[] = {2, 0};

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
		if (!CxGlobals.QuietInit) printf("mod: driver %s failed\n", one_module->Name);
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
		if (!CxGlobals.QuietInit) printf("mod: %s %s failed\n", modtype, one_module->Name);
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
		if (!CxGlobals.QuietInit) printf("mod: %s %s failed\n", modtype, one_module->Name);
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
		if (!CxGlobals.QuietInit) printf("mod: %s %s failed\n", modtype, one_module->Name);
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
		if (!CxGlobals.QuietInit) printf("mod: %s %s failed\n", modtype, one_module->Name);
		mssError(1,"CX","Module '%s' has unsupported interface version %d (do you need to recompile/reinstall modules?)",
		    modpath, *(moduledata->InterfaceVer));
		continue;
		}

	    /** Attempt to initialize the module **/
	    if (moduledata->InitFn() < 0)
		{
		dlclose(moduledata->dlhandle);
		nmFree(moduledata, sizeof(CxModule));
		mssError(0,"CX","Module '%s' load failed",modpath);
		if (!CxGlobals.QuietInit) printf("mod: %s %s failed\n", modtype, one_module->Name);
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
    if(CxGlobals.Flags & CX_F_SHUTTINGDOWN)
	return;
    CxGlobals.Flags |= CX_F_SHUTTINGDOWN;
    mssError(0,"CX","Centrallix is shutting down");
    for(i=0;i<xaCount(&CxGlobals.ShutdownHandlers);i++)
	{
	ShutdownHandlerFunc handler = (ShutdownHandlerFunc) xaGetItem(&CxGlobals.ShutdownHandlers,i);
	if(handler)
	    handler();
	}
    exit(0);
    thExit();
    }

int cxAddShutdownHandler(ShutdownHandlerFunc handler)
    {
    return xaAddItem(&CxGlobals.ShutdownHandlers,handler);
    }


void
cxRemovePidFile()
    {
    char pidbuf[16];
    int pidbuf_id;
    int fd;
    int cnt;
	

	if (CxGlobals.PidFile[0] && (CxGlobals.Flags & CX_F_SERVICE))
	    {
	    /** Double-check to see if we own it.  Another centrallix
	     ** process might have overwritten it.  Leave the file alone
	     ** in that case.
	     **
	     ** There is technically a race condition here -- but the
	     ** consequences are minor and we're doing a courtesy. :)
	     **/
	    fd = open(CxGlobals.PidFile, O_RDONLY, 0600);
	    if (fd >= 0)
		{
		cnt = read(fd, pidbuf, sizeof(pidbuf));
		if (cnt > 1 && cnt < sizeof(pidbuf))
		    {
		    pidbuf[cnt] = '\0';
		    pidbuf_id = strtol(pidbuf, NULL, 10);

		    /** Unlink it if the pids match **/
		    if (pidbuf_id == getpid())
			unlink(CxGlobals.PidFile);
		    }
		close(fd);
		}
	    
	    CxGlobals.PidFile[0] = '\0';
	    }

    return;
    }


int
cxInitialize(void* v)
    {
    pFile cxconf;
    pFile cxcharsetmap;
    pStructInf mss_conf;
    pStructInf thisCharsetPtr;
    char* charsetmapFileName;
    char* authmethod;
    char* authmethodfile;
    char* logmethod;
    char* logprog;
    int log_all_errors;
    char* ptr;
    int n;
    int fd;
    time_t tm;
    int pid;
    char rbuf[16];
    char* debugfile;

	xaInit(&CxGlobals.ShutdownHandlers,4);

        /** Set the current locale to user's locale for chars **/
	setlocale(LC_CTYPE, "");

	/** set up the interrupt handler so we can shutdown properly **/
	mtAddSignalHandler(SIGINT,cxShutdownThread);
	mtAddSignalHandler(SIGTERM,cxShutdownThread);

	/** Add a shutdown handler to delete the pid file **/
	if (CxGlobals.PidFile[0])
	    cxAddShutdownHandler(cxRemovePidFile);

#ifdef _SC_CLK_TCK
        CxGlobals.ClkTck = sysconf(_SC_CLK_TCK);
#else
        CxGlobals.ClkTck = CLK_TCK;
#endif

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

	/** Force the server locale? **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "locale"), NULL, &ptr, 0) == 0)
	    {
	    if (setlocale(LC_CTYPE, ptr) == NULL)
		setlocale(LC_CTYPE, "");
	    }

	/** Check if UTF-8 or single byte encoding **/
	if(MB_CUR_MAX == 1)
            CxGlobals.CharacterMode = CharModeSingleByte;
        else if(strcmp(nl_langinfo(CODESET), "UTF-8") == 0)
            CxGlobals.CharacterMode = CharModeUTF8;
        else
            {
            fprintf(stderr, "Current locale is not single byte encoding or UTF-8! Using C locale.\n");
            setlocale(LC_CTYPE, "C");
            }

        /** Load the charsetmap file **/
        if(stAttrValue(stLookup(CxGlobals.ParsedConfig, CHR_CHARSETMAP_FILE_KEY), NULL, &charsetmapFileName, 0) != 0)
	    {
            printf("centrallix: did not find required key '%s' in config file '%s'\n", CHR_CHARSETMAP_FILE_KEY, CxGlobals.ConfigFileName);
            thExit();
	    }
        cxcharsetmap = fdOpen(charsetmapFileName, O_RDONLY, 0600);
        if(!cxcharsetmap)
	    {
            printf("centrallix: could not open charsetmap file '%s'\n", charsetmapFileName);
            thExit();
	    }
        CxGlobals.CharsetMap = stParseMsg(cxcharsetmap, 0);
	if (!CxGlobals.CharsetMap)
	    {
	    printf("centrallix: error parsing charsetmap file '%s'\n", charsetmapFileName);
	    thExit();
	    }
        fdClose(cxcharsetmap, 0);
        
        /** Now pull out the current charset and free the
         rest **/
        thisCharsetPtr = stLookup(CxGlobals.CharsetMap, nl_langinfo(CODESET));
        if(thisCharsetPtr)
	    {
            stLinkInf(thisCharsetPtr);
            stFreeInf(CxGlobals.CharsetMap);
            CxGlobals.CharsetMap = thisCharsetPtr;
	    }
        else
	    {
            printf("centrallix: could not find current charset '%s' in '%s'\n", nl_langinfo(CODESET), charsetmapFileName);
            thExit();
	    }
        
        /** Debug log **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "debug_log_file"), NULL, &debugfile, 0) < 0)
	    debugfile = "/var/log/cx_debug_log";
	CxGlobals.DebugFile = fdOpen(debugfile, O_WRONLY | O_APPEND | O_CREAT, 0600);
	if (!CxGlobals.DebugFile)
	    perror("centrallix: warning: could not open debug log file");
	cxDebugLog("centrallix initializing...", getpid());

	/** This setting can be dangerous apart from the RBAC security subsystem.
	 ** We default to Enabled here, but this is turned off in the default config.
	 **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "enable_send_credentials"), &n, NULL, 0) != 0 || n != 0)
	    CxGlobals.Flags |= CX_F_ENABLEREMOTEPW;

	/** Init the security subsystem.
	 **/
	cxssInitialize();

	/** init qprintf **/
	qpfInitialize(); 

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
	nmRegister(sizeof(WgtrNode), "WgtrNode");

	/** Init the openssl library **/
	SSL_library_init();
	SSL_load_error_strings();
	fd = open("/dev/urandom", O_RDONLY | O_NOCTTY);
	if (fd >= 0)
	    {
	    read(fd, rbuf, 16);
	    RAND_add(rbuf, 16, (double)16.0);
	    close(fd);
	    }
	pid = getpid();
	RAND_add(&pid, 4, (double)0.25);
	tm = time(NULL);
	RAND_add(&tm, 4, (double)0.125);

	/** Init the multiquery system and drivers **/
	mqInitialize();				/* MultiQuery system */
	mqtInitialize();			/* tablegen query module */
	mqpInitialize();			/* projection query module */
	mqjInitialize();			/* join query module */
	mqisInitialize();			/* insert-select query mod */
	mquInitialize();			/* update statement query mod */
	mqusInitialize();			/* upsert statement query mod */
	mqdInitialize();			/* delete statement query mod */
	mqobInitialize();			/* orderby module */

	/** Init the objectsystem drivers **/
	sysInitialize();			/* Sys info driver */
	uxdInitialize();			/* UNIX filesystem driver */
#if 0
	sybdInitialize();			/* Sybase CT-lib driver */
	mysdInitialize();			/* MySQL driver */
#endif

	stxInitialize();			/* Structure file driver */
	qytInitialize();			/* Query Tree driver */
	qypInitialize();			/* Query Pivot driver */
	qyInitialize();				/* stored query (aka view) driver */
	rptInitialize();			/* report writer driver */
	uxpInitialize();			/* UNIX printer access driver */
	datInitialize();			/* flat ascii datafile (CSV, etc) */
	/*berkInitialize();*/			/* Berkeley Database support */
	uxuInitialize();			/* UNIX users list driver */
	audInitialize();			/* Audio file player driver */
	lnkInitialize();			/* Symlink driver */
	jsonInitialize();			/* JSON data driver */

	/** Init the reporting content drivers **/
#if 0
	pclInitialize();			/* PCL report generator */
	htpInitialize();			/* HTML report generator */
	fxpInitialize();			/* Epson FX report generator */
	txtInitialize();			/* text only report gen */
#endif

	prtInitialize();
	prt_htmlfm_Initialize();
	prt_csvfm_Initialize();
	prt_strictfm_Initialize();
	prt_pclod_Initialize();
	prt_textod_Initialize();
	prt_psod_Initialize();
	prt_fxod_Initialize();

	/** Initialize the wgtr module **/
	wgtrInitialize();

	/** Initialize the Interface module **/
	ifcInitialize();

	/** Application management layer **/
	appInitialize();

	/** Init the modules being used if dynamic loading is disabled **/
	
#if (USE_DBL == CX_STATIC)
	dblInitialize();
#endif

#ifndef WITH_DYNAMIC_LOAD

#ifdef USE_HTTP
	httpInitialize();
#endif

#ifdef USE_MIME
	mimeInitialize();
#endif

#ifdef USE_SYBASE
	sybdInitialize();
#endif

#ifdef USE_MYSQL
	mysdInitialize();			/* MySQL driver */
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
	htruleInitialize();			/* rule module */
	htpageInitialize();			/* DHTML page generator */
	htspaneInitialize();		/* scrollable pane module */
	httreeInitialize();			/* treeview module */
	hthtmlInitialize();			/* html pane module */
	htconnInitialize();			/* connector nonvisual module */
	htbtnInitialize();			/* generic button module */
	htibtnInitialize();			/* image button module */
	httbtnInitialize();			/* text button module */
	htmenuInitialize();			/* dropdown/popup menu module */
	htsetInitialize();			/* frameset module */
	htvblInitialize();			/* variable nonvisual module */
	httabInitialize();			/* tab control / tab page module */
	htpnInitialize();			/* pane module */
	httblInitialize();			/* tabular data module */
	htchtInitialize();			/* chart module */
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
	htcaInitialize();			/* calendar module */
	htsbInitialize();			/* scrollbar module */
	htimgInitialize();			/* image widget */
	htfbInitialize();			/* form bar composite widget test */
	htocInitialize();			/* object canvas widget */
	htmapInitialize();			/* object canvas widget */
	htfuInitialize();			/* file upload widget */

	htformInitialize();			/* forms module */
	htosrcInitialize();			/* osrc module */
	htalrtInitialize();			/* alert module */
	htlblInitialize();			/* label module */
	httermInitialize();			/* terminal module */
	hthintInitialize();			/* pres. hints module */
	htparamInitialize();			/* parameter module */
	htalInitialize();			/* autolayout module */
	htrptInitialize();			/* repeat module */
	htmsInitialize();			/* multiscroll module */

	htcmpdInitialize();			/* component declaration */
	htcmpInitialize();			/* component instance */

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

#ifndef WITH_DYNAMIC_LOAD

#ifdef USE_NETLDAP
	nldapInitialize();			/* LDAP network interface */
#endif

#endif

	bnetInitialize();			/* BDQS network listener */

	/** Load any network driver modules **/
	cx_internal_LoadModules("netdriver");

    return 0;
    }
#endif

int
cxDebugLog(char* fmt, ...)
    {
    va_list va;
    int rval;
    long long msec;
    char* our_fmt;

	if (!CxGlobals.DebugFile)
	    return -1;

	msec = mtRealTicks() * 1000LL / CxGlobals.ClkTck;

	our_fmt = nmSysMalloc(strlen(fmt) + 256);
	if (!our_fmt)
	    return -ENOMEM;
	sprintf(our_fmt, "T%lld.%3.3lld P%5.5d %s\n", msec/1000, msec%1000, getpid(), fmt);

	/** Alloc a printf buf? **/
	if (!CxGlobals.DebugFile->PrintfBuf)
	    {
	    CxGlobals.DebugFile->PrintfBufSize = FD_PRINTF_BUFSIZ;
	    CxGlobals.DebugFile->PrintfBuf = (char*)nmSysMalloc(CxGlobals.DebugFile->PrintfBufSize);
	    if (!CxGlobals.DebugFile->PrintfBuf)
		return -ENOMEM;
	    }

	/** Print it. **/
	va_start(va,fmt);
	rval=xsGenPrintf_va(fdWrite, CxGlobals.DebugFile, &(CxGlobals.DebugFile->PrintfBuf), &(CxGlobals.DebugFile->PrintfBufSize), our_fmt, va);
	va_end(va);

	nmSysFree(our_fmt);

    return rval;
    }

