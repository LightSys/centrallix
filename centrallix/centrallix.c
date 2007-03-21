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
#include "cxlib/mtask.h"
#include "obj.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "cxlib/mtlexer.h"
#include <signal.h>
#include "wgtr.h"
#include "iface.h"
#include "cxss/cxss.h"

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

    $Id: centrallix.c,v 1.40 2007/03/21 04:48:08 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/centrallix.c,v $

    $Log: centrallix.c,v $
    Revision 1.40  2007/03/21 04:48:08  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.

    Revision 1.39  2007/02/22 23:25:13  gbeeley
    - (feature) adding initial framework for CXSS, the security subsystem.
    - (feature) CXSS entropy pool and key generation, basic framework.
    - (feature) adding xmlhttprequest capability
    - (change) CXSS requires OpenSSL, adding that check to the build
    - (security) Adding application key to thwart request spoofing attacks.
      Once the AML is active, application keying will be more important and
      will be handled there instead of in net_http.

    Revision 1.38  2006/10/19 21:53:23  gbeeley
    - (feature) First cut at the component-based client side development
      system.  Only rendering of the components works right now; interaction
      with the components and their containers is not yet functional.  For
      an example, see "debugwin.cmp" and "window_test.app" in the samples
      directory of centrallix-os.

    Revision 1.37  2005/09/17 01:23:50  gbeeley
    - Adding sysinfo objectsystem driver, which is roughly analogous to
      the /proc filesystem in Linux.

    Revision 1.36  2005/06/23 22:07:58  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.35  2005/03/01 07:14:59  gbeeley
    - RPM build fixes.  The deployment root may not be the same as the build
      root, so we need to adjust the paths in the generated configuration
      files accordingly, if --with-builddir is specified.

    Revision 1.34  2005/02/26 06:42:35  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.33  2005/02/24 05:44:32  gbeeley
    - Adding PostScript and PDF report output formats.  (pdf is via ps2pdf).
    - Special Thanks to Tim Irwin who participated in the Apex NC CODN
      Code-a-Thon on Feb 5, 2005, for much of the initial research on the
      PostScript support!!  See http://www.codn.net/
    - More formats (maybe PNG?) should be easy to add.
    - TODO: read the *real* font metric files to get font geometries!
    - TODO: compress the images written into the .ps file!

    Revision 1.32  2004/12/31 04:35:14  gbeeley
    - Adding the Object Canvas widget, an objectsource client which allows
      data to be displayed visually on a canvas (useful with maps and diagrams
      and such).  Link a form to the osrc as well and have some fun :)

    Revision 1.31  2004/08/28 06:27:13  jorupp
     * the widget drivers are all initialized in wgtrInitialize()

    Revision 1.30  2004/08/02 14:09:33  mmcgill
    Restructured the rendering process, in anticipation of new deployment methods
    being added in the future. The wgtr module is now the main widget-related
    module, responsible for all non-deployment-specific widget functionality.
    For example, Verifying a widget tree is non-deployment-specific, so the verify
    functions have been moved out of htmlgen and into the wgtr module.
    Changes include:
    *   Creating a new folder, wgtr/, to contain the wgtr module, including all
        wgtr drivers.
    *   Adding wgtr drivers to the widget tree module.
    *   Moving the xxxVerify() functions to the wgtr drivers in the wgtr module.
    *   Requiring all deployment methods (currently only DHTML) to register a
        Render() function with the wgtr module.
    *   Adding wgtrRender(), to abstract the details of the rendering process
        from the caller. Given a widget tree, a string representing the deployment
        method to use ("DHTML" for now), and the additional args for the rendering
        function, wgtrRender() looks up the appropriate function for the specified
        deployment method and calls it.
    *   Added xxxNew() functions to each wgtr driver, to be called when a new node
        is being created. This is primarily to allow widget drivers to declare
        the interfaces their widgets support when they are instantiated, but other
        initialization tasks can go there as well.

    Also in this commit:
    *   Fixed a typo in the inclusion guard for iface.h (most embarrasing)
    *   Fixed an overflow in objCopyData() in obj_datatypes.c that stomped on
        other stack variables.
    *   Updated net_http.c to call wgtrRender instead of htrRender(). Net drivers
        can now be completely insulated from the deployment method by the wgtr
        module.

    Revision 1.29  2004/07/30 17:59:55  mmcgill
    Added the Interface module on the server-side. The module provides support
    for widget interfaces, and the capability to easily add support for new
    interface types.

    As yet this module is unused. Future commits will add interface support to
    the widget tree module, and eventually to the client-side DHTML code.

    This module anticipates the addition of dynamically loadable component
    widgets, which necessitate some sort of interface support.

    Revision 1.28  2004/07/20 21:28:51  mmcgill
    *   ht_render
        -   Added code to perform verification of widget-tree prior to
            rendering.
        -   Added concept of 'pseudo-types' for widget-drivers, e.g. the
            table driver getting called for 'table-column' widgets. This is
            necessary now since the 'table-column' entry in an app file will
            actually get put into its own widget node. Pseudo-type names
            are stored in an XArray in the driver struct during the
            xxxInitialize() function of the driver, and BEFORE ANY CALLS TO
            htrAddSupport().
        -   Added htrLookupDriver() to encapsulate the process of looking up
            a driver given an HtSession and widget type
        -   Added 'pWgtrVerifySession VerifySession' to HtSession.
            WgtrVerifySession represents a 'verification context' to be used
            by the xxxVerify functions in the widget drivers to schedule new
            widgets for verification, and otherwise interact with the
            verification system.
    *   xxxVerify() functions now take a pHtSession parameter.
    *   Updated the dropdown, tab, and table widgets to register their
        pseudo-types
    *   Moved the ObjProperty out of obj.h and into wgtr.c to internalize it,
        in anticipation of converting the Wgtr module to use PTODs instead.
    *   Fixed some Wgtr module memory-leak issues
    *   Added functions wgtrScheduleVerify() and wgtrCancelVerify(). They are
        to be used in the xxxVerify() functions when a node has been
        dynamically added to the widget tree during tree verification.
    *   Added the formbar widget driver, as a demonstration of how to modify
        the widget-tree during the verification process. The formbar widget
        doesn't actually do anything during the rendering process excpet
        call htrRenderWidget on its subwidgets, but during Verify it adds
        all the widgets necessary to reproduce the 'form control pane' from
        ors.app. This will eventually be done even more efficiently with
        component widgets - this serves as a tech test.

    Revision 1.27  2004/07/19 15:30:39  mmcgill
    The DHTML generation system has been updated from the 2-step process to
    a three-step process:
        1)	Upon request for an application, a widget-tree is built from the
    	app file requested.
        2)	The tree is Verified (not actually implemented yet, since none of
    	the widget drivers have proper Verify() functions - but it's only
    	a matter of a function call in net_http.c)
        3)	The widget drivers are called on their respective parts of the
    	tree structure to generate the DHTML code, which is then sent to
    	the user.

    To support widget tree generation the WGTR module has been added. This
    module allows OSML objects to be parsed into widget-trees. The module
    also provides an API for building widget-trees from scratch, and for
    manipulating existing widget-trees.

    The Render functions of all widget drivers have been updated to make their
    calls to the WGTR module, rather than the OSML, and to take a pWgtrNode
    instead of a pObject as a parameter.

    net_internal_GET() in net_http.c has been updated to call
    wgtrParseOpenObject() to make a tree, pass that tree to htrRender(), and
    then free it.

    htrRender() in ht_render.c has been updated to take a pWgtrNode instead of
    a pObject parameter, and to make calls through the WGTR module instead of
    the OSML where appropriate. htrRenderWidget(), htrRenderSubwidgets(),
    htrGetBoolean(), etc. have also been modified appropriately.

    I have assumed in each widget driver that w_obj->Session is equivelent to
    s->ObjSession; in other words, that the object being passed in to the
    Render() function was opened via the session being passed in with the
    HtSession parameter. To my understanding this is a valid assumption.

    While I did run through the test apps and all appears to be well, it is
    possible that some bugs were introduced as a result of the modifications to
    all 30 widget drivers. If you find at any point that things are acting
    funny, that would be a good place to check.

    Revision 1.26  2004/02/24 19:59:31  gbeeley
    - adding component-declaration widget driver
    - adding image widget driver
    - adding app-level presentation hints pseudo-widget driver

    Revision 1.25  2004/01/15 14:34:06  jorupp
     * switch to using MTask's signal handlers instead of 'real' signal handlers

    Revision 1.24  2003/11/12 22:21:39  gbeeley
    - addition of delete support to osml, mq, datafile, and ux modules
    - added objDeleteObj() API call which will replace objDelete()
    - stparse now allows strings as well as keywords for object names
    - sanity check - old rpt driver to make sure it isn't in the build

    Revision 1.23  2003/08/05 16:45:12  affert
    Initial Berkeley DB support.

    Revision 1.22  2003/07/15 01:57:51  gbeeley
    Adding an independent DHTML scrollbar widget that will be used to
    control scrolling/etc on other widgets.

    Revision 1.21  2003/07/12 04:14:34  gbeeley
    Initial rough beginnings of a calendar widget.

    Revision 1.20  2003/07/09 18:13:20  gbeeley
    Further polishing/work on the table output in the report writer.  Re-
    enabled uxprint OSD once its dependence on prtmgmt was removed.

    Revision 1.19  2003/06/27 21:19:47  gbeeley
    Okay, breaking the reporting system for the time being while I am porting
    it to the new prtmgmt subsystem.  Some things will not work for a while...

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
    if(CxGlobals.ShuttingDown)
	return;
    CxGlobals.ShuttingDown = 1;
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
	mtAddSignalHandler(SIGINT,cxShutdownThread);

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

	/** Init the security subsystem.
	 **/
	cxssInitialize();

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
	nmRegister(sizeof(WgtrNode), "WgtrNode");

	/** Init the multiquery system and drivers **/
	mqInitialize();				/* MultiQuery system */
	mqtInitialize();			/* tablegen query module */
	mqpInitialize();			/* projection query module */
	mqjInitialize();			/* join query module */

	/** Init the objectsystem drivers **/
	sysInitialize();			/* Sys info driver */
	uxdInitialize();			/* UNIX filesystem driver */
#if 0
	sybdInitialize();			/* Sybase CT-lib driver */
#endif
	stxInitialize();			/* Structure file driver */
	qytInitialize();			/* Query Tree driver */
	rptInitialize();			/* report writer driver */
	uxpInitialize();			/* UNIX printer access driver */
	datInitialize();			/* flat ascii datafile (CSV, etc) */
	/*berkInitialize();*/			/* Berkeley Database support */
	uxuInitialize();			/* UNIX users list driver */
	audInitialize();			/* Audio file player driver */

	/** Init the reporting content drivers **/
#if 0
	pclInitialize();			/* PCL report generator */
	htpInitialize();			/* HTML report generator */
	fxpInitialize();			/* Epson FX report generator */
	txtInitialize();			/* text only report gen */
#endif

	prtInitialize();
	prt_htmlfm_Initialize();
	prt_strictfm_Initialize();
	prt_pclod_Initialize();
	prt_textod_Initialize();
	prt_psod_Initialize();

	/** Initialize the wgtr module **/
	wgtrInitialize();

	/** Initialize the Interface module **/
	ifcInitialize();

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
	htcaInitialize();			/* calendar module */
	htsbInitialize();			/* scrollbar module */
	htimgInitialize();			/* image widget */
	htfbInitialize();			/* form bar composite widget test */
	htocInitialize();			/* object canvas widget */

	htformInitialize();			/* forms module */
	htosrcInitialize();			/* osrc module */
	htalrtInitialize();			/* alert module */
	htlblInitialize();			/* label module */
	httermInitialize();			/* terminal module */
	hthintInitialize();			/* pres. hints module */
	htparamInitialize();			/* parameter module */

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

	bnetInitialize();			/* BDQS network listener */

	/** Load any network driver modules **/
	cx_internal_LoadModules("netdriver");

    return 0;
    }
#endif
