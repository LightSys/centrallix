#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <stdarg.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
#include "centrallix.h"
#include "expression.h"

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
/* Module: 	ht_render.h,ht_render.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Page rendering engine that interacts with the 	*/
/*		various widget drivers to produce a dynamic HTML page.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: ht_render.c,v 1.37 2003/06/21 23:54:41 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/ht_render.c,v $

    $Log: ht_render.c,v $
    Revision 1.37  2003/06/21 23:54:41  jorupp
     * fixex up a few problems I found with the version I committed (like compilation...)
     * removed some code that was commented out

    Revision 1.36  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.35  2003/06/03 23:31:04  gbeeley
    Adding pro forma netscape 4.8 support.

    Revision 1.34  2003/05/30 17:39:49  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.33  2003/03/30 22:49:23  jorupp
     * get rid of some compile warnings -- compiles with zero warnings under gcc 3.2.2

    Revision 1.32  2003/01/05 04:18:08  lkehresman
    Added detection for Mozilla 1.2.x

    Revision 1.31  2002/12/24 09:41:07  jorupp
     * move output of cn_browser to ht_render, also moving up above the first place where it is needed

    Revision 1.30  2002/12/04 00:19:09  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.29  2002/11/22 19:29:36  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.28  2002/09/27 22:26:04  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.27  2002/09/11 00:57:08  jorupp
     * added check for Mozilla 1.1

    Revision 1.26  2002/08/12 09:14:28  mattphillips
    Use the built-in PACKAGE_VERSION instead of VERSION to get the current version
    number to be more standard.  PACKAGE_VERSION is set by autoconf, but read from
    .version when configure is generated.

    Revision 1.25  2002/08/03 02:36:34  gbeeley
    Made all hash tables the same size at 257 (a prime) entries.

    Revision 1.24  2002/08/02 19:44:20  gbeeley
    Have ht_render report the widget type when it complains about not knowing
    a widget type when generating a page.

    Revision 1.23  2002/07/18 20:12:40  lkehresman
    Added support for a loadstatus icon to be displayed, hiding the drawing
    of the visible windows.  This looks MUCH nicer when loading Kardia or
    any other large apps.  It is completely optional part of the page widget.
    To take advantage of it, put the parameter "loadstatus" equal to "true"
    in the page widget.

    Revision 1.22  2002/07/18 15:17:44  lkehresman
    Ok, I got caught being lazy.  I used snprintf and the string sbuf to
    help me count the number of characters in the string I modified.  But
    sbuf was being used elsewhere and I messed it up.  Fixed it so it isn't
    using sbuf any more.  I broke down and counted the characters.
    (how many times can we modify this line in one hour?? SHEESH!

    Revision 1.20  2002/07/18 14:31:05  lkehresman
    Whoops!  I was sending the wrong string size to fdWrite.  Fixed it.

    Revision 1.19  2002/07/18 14:26:13  lkehresman
    Added a work-around for the Netscape resizing bug.  Instead of leaving
    the page totally messed up on a resize, it will now completely reload the
    page whenever the window is resized.

    Revision 1.18  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.17  2002/07/15 21:27:02  lkehresman
    Added copyright statement at top of generated DHTML documents.  (if the
    wording needs to change, please do it or let me know)

    Revision 1.16  2002/07/07 00:17:01  jorupp
     * add support for Mozilla 1.1alpha (1.1a)

    Revision 1.15  2002/06/24 20:07:41  lkehresman
    Committing a fix for Jonathan (he doesn't have CVS access right now).
    This now detects Mozilla pre-1.0 versions.

    Revision 1.14  2002/06/20 16:22:08  gbeeley
    Wrapped the nonconstant format string warning in an ifdef WITH_SECWARN
    so it doesn't bug people other than developers.

    Revision 1.13  2002/06/19 19:57:13  gbeeley
    Added warning code if htr..._va() function is passed a format string
    from the heap or other modifiable data segments.  Half a kludge...

    Revision 1.12  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.11  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.10  2002/05/03 03:43:25  gbeeley
    Added FD_U_PACKET to the fdWrite() calls in ht_render.  It is possible
    that some data was getting dropped - fdWrite() makes no guarantee of
    writing *all* the data unless you include the FD_U_PACKET flag :)

    Revision 1.9  2002/05/02 01:12:43  gbeeley
    Fixed some buggy initialization code where an XArray was not being
    setup prior to being used.  Was causing potential bad pointers to
    realloc() and other various problems, especially once the dynamic
    loader was messing with things.

    Revision 1.8  2002/04/28 06:00:38  jorupp
     * added htrAddScriptCleanup* stuff
     * added cleanup stuff to osrc

    Revision 1.7  2002/04/28 03:19:53  gbeeley
    Fixed a bit of a bug in ht_render where it did not properly set the
    length on the StrValue structures when adding script functions.  This
    was basically causing some substantial heap corruption.

    Revision 1.6  2002/04/25 22:54:48  gbeeley
    Set the starting tmpbuf back to 512 from the 8 bytes I was using to
    test the auto-realloc logic... ;)

    Revision 1.5  2002/04/25 22:51:29  gbeeley
    Added vararg versions of some key htrAddThingyItem() type of routines
    so that all of this sbuf stuff doesn't have to be done, as we have
    been bumping up against the limits on the local sbuf's due to very
    long object names.  Modified label, editbox, and treeview to test
    out (and make kardia.app work).

    Revision 1.4  2002/04/25 04:27:21  gbeeley
    Added new AddInclude() functionality to the html generator, so include
    javascript files can be added.  Untested.

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/10/22 17:19:42  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.1.1.1  2001/08/13 18:00:48  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:53  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** GLOBALS ***/
struct
    {
    XArray	Drivers;		/* simple driver listing. */
    XHashTable	Classes;		/* classes of widget sets */
    }
    HTR;

/** a structure to store the detection code for a specific browser/set of browsers **/
typedef struct
    {
    regex_t  UAreg;
    XArray  Children;
    HtCapabilities  Capabilities;
    }
    AgentCapabilities, *pAgentCapabilities;

/**
 * Processes a single node (and it's children) of the tree of useragents
 **/
pAgentCapabilities
htr_internal_ProcessUserAgent(const pStructInf node, const pHtCapabilities parentCap)
    {
    int i;
    pStructInf entry;
    pAgentCapabilities agentCap;
    char* data;

    /** build the structure **/
    agentCap = nmMalloc(sizeof(AgentCapabilities));
    if(!agentCap)
	{
	mssError(0,"HTR","nmMalloc() failed");
	return NULL;
	}
    memset(agentCap, 0, sizeof(AgentCapabilities));

    /** if we're a top-level definition (under a class), there's no parent agentCapabilities to inherit **/
    if(parentCap)
	{
	memcpy(&(agentCap->Capabilities), parentCap, sizeof(HtCapabilities));
	}

    /** find and build the regex for the useragent detection **/
    entry = stLookup(node,"useragent");
    if(!entry)
	{
	mssError(1,"HTR","Missing useragent for %s", node->Name);
	nmFree(agentCap,sizeof(AgentCapabilities));
	return NULL;
	}
    if(stGetAttrValue(entry, DATA_T_STRING, POD(&data), 0)<0)
	{
	mssError(1,"HTR","Can't read useragent for %s", node->Name);
	nmFree(agentCap,sizeof(AgentCapabilities));
	return NULL;
	}
    if(regcomp(&(agentCap->UAreg), data, REG_EXTENDED | REG_NOSUB | REG_ICASE))
	{
	mssError(1,"HTR","Could not compile regular expression: '%s' for %s", data, node->Name);
	nmFree(agentCap,sizeof(AgentCapabilities));
	return NULL;
	}

    /** 
     * process all the listed agentCapabilities
     * seriously... who wants to write this code for each attribute.... 
     **/
#define PROCESS_CAP_INIT(attr) \
    if((entry = stLookup(node, # attr ))) \
	{ \
	if(stGetAttrValue(entry, DATA_T_STRING, POD(&data), 0)>=0) \
	    { \
	    if(!strcmp(data, "yes")) \
		agentCap->Capabilities.attr = 1; \
	    else if(!strcmp(data, "no")) \
		agentCap->Capabilities.attr = 1; \
	    else \
		mssError(1,"HTR","%s must be yes, no, 0, or 1 in %s", # attr ,node->Name); \
	    } \
	else if(stGetAttrValue(entry, DATA_T_INTEGER, POD(&i), 0)>=0) \
	    { \
	    if(i==0) \
		agentCap->Capabilities.attr = 0; \
	    else \
		agentCap->Capabilities.attr = 1; \
	    } \
	else \
	    mssError(1,"HTR","%s must be yes, no, 0, or 1 in %s", # attr ,node->Name); \
	}

    PROCESS_CAP_INIT(Dom0NS);
    PROCESS_CAP_INIT(Dom0IE);
    PROCESS_CAP_INIT(Dom1HTML);
    PROCESS_CAP_INIT(Dom1XML);
    PROCESS_CAP_INIT(Dom2Core);
    PROCESS_CAP_INIT(Dom2HTML);
    PROCESS_CAP_INIT(Dom2XML);
    PROCESS_CAP_INIT(Dom2Views);
    PROCESS_CAP_INIT(Dom2StyleSheets);
    PROCESS_CAP_INIT(Dom2CSS);
    PROCESS_CAP_INIT(Dom2CSS2);
    PROCESS_CAP_INIT(Dom2Events);
    PROCESS_CAP_INIT(Dom2MouseEvents);
    PROCESS_CAP_INIT(Dom2HTMLEvents);
    PROCESS_CAP_INIT(Dom2MutationEvents);
    PROCESS_CAP_INIT(Dom2Range);
    PROCESS_CAP_INIT(Dom2Traversal);
    PROCESS_CAP_INIT(CSS1);
    PROCESS_CAP_INIT(CSS2);
    PROCESS_CAP_INIT(HTML40);

    /** now process children, passing a reference to our capabilities along **/
    xaInit(&(agentCap->Children), 4);
    for(i=0;i<node->nSubInf;i++)
	{
	pStructInf childNode;
	childNode = node->SubInf[i];
	/** UsrType is non-null if this is a sub-structure, ie. not an attribute **/
	if(childNode && childNode->UsrType)
	    {
	    pAgentCapabilities childCap = htr_internal_ProcessUserAgent(childNode, &(agentCap->Capabilities));
	    if(childCap)
		{
		xaAddItem(&(agentCap->Children), childCap);
		}
	    }
	}

    return agentCap;
    }

/***
 ***  Writes the capabilities of the browser used in the passed session to the passed pFile
 ***     as the cx__capabilities object (for javascript)
 ***/
void
htr_internal_writeCxCapabilities(pHtSession s, pFile out)
    {
    fdWrite(out,"    cx__capabilities = new Object();\n",37,0,FD_U_PACKET);
#define PROCESS_CAP_OUT(attr) \
    fdWrite(out,"    cx__capabilities.",21,0,FD_U_PACKET); \
    fdWrite(out, # attr ,strlen( # attr ),0,FD_U_PACKET); \
    fdWrite(out," = ",3,0,FD_U_PACKET); \
    fdWrite(out,(s->Capabilities.attr?"1;\n":"0;\n"),3,0,FD_U_PACKET);

    PROCESS_CAP_OUT(Dom0NS);
    PROCESS_CAP_OUT(Dom0IE);
    PROCESS_CAP_OUT(Dom1HTML);
    PROCESS_CAP_OUT(Dom1XML);
    PROCESS_CAP_OUT(Dom2Core);
    PROCESS_CAP_OUT(Dom2HTML);
    PROCESS_CAP_OUT(Dom2XML);
    PROCESS_CAP_OUT(Dom2Views);
    PROCESS_CAP_OUT(Dom2StyleSheets);
    PROCESS_CAP_OUT(Dom2CSS);
    PROCESS_CAP_OUT(Dom2CSS2);
    PROCESS_CAP_OUT(Dom2Events);
    PROCESS_CAP_OUT(Dom2MouseEvents);
    PROCESS_CAP_OUT(Dom2HTMLEvents);
    PROCESS_CAP_OUT(Dom2MutationEvents);
    PROCESS_CAP_OUT(Dom2Range);
    PROCESS_CAP_OUT(Dom2Traversal);
    PROCESS_CAP_OUT(CSS1);
    PROCESS_CAP_OUT(CSS2);
    PROCESS_CAP_OUT(HTML40);
    }
    
/**
 * Registers the tree of classes and user agents by reading the file specified in the config file
**/
int
htrRegisterUserAgents()
    {
    pStructInf uaConfigEntry;
    char *uaConfigFilename;
    pFile uaConfigFile;
    pStructInf uaConfigRoot;
    int i,j;

    /** find the name of the config file **/
    uaConfigEntry = stLookup(CxGlobals.ParsedConfig,"useragent_config");
    if(!uaConfigEntry)
	{
	mssError(1,"HTR","No configuration directive useragent_config found.  Unable to register useragents.");
	return -1;
	}
    if(stGetAttrValue(uaConfigEntry, DATA_T_STRING, POD(&uaConfigFilename), 0) <0 || !uaConfigFilename )
	{
	mssError(0,"HTR","Unable to read useragent_config's value.  Unable to register useragents.");
	return -1;
	}
    
    /** open and parse it **/
    uaConfigFile = fdOpen(uaConfigFilename, O_RDONLY, 0600);
    if(!uaConfigFile)
	{
	mssError(0,"HTR","Unable to open useragent_config %s", uaConfigFilename);
	return -1;
	}
    uaConfigRoot = stParseMsg(uaConfigFile, 0);
    if(!uaConfigRoot)
	{
	mssError(0,"HTR","Unable to parse useragent_config %s", uaConfigFilename);
	fdClose(uaConfigFile, 0);
	}

    /** iterate through the classes and create them **/
    for(i=0;i<uaConfigRoot->nSubInf;i++)
	{
	pStructInf stClass;
	stClass = uaConfigRoot->SubInf[i];
	if(stClass)
	    {
	    pHtClass class = (pHtClass)nmMalloc(sizeof(HtClass));
	    if(!class)
		{
		mssError(0,"HTR","nmMalloc() failed");
		return -1;
		}
	    memset(class, 0, sizeof(HtClass));
	    strncpy(class->ClassName, stClass->Name, 32);
	    class->ClassName[31] = '\0';

	    xaInit(&(class->Agents),4);
	    xhInit(&(class->WidgetDrivers), 257, 0);
	    
	    for(j=0;j<stClass->nSubInf;j++)
		{
		pStructInf entry = stClass->SubInf[j];
		if(entry && entry->UsrType)
		    {
		    pAgentCapabilities cap;
		    if((cap = htr_internal_ProcessUserAgent(entry, NULL)))
			{
			xaAddItem(&(class->Agents), cap);
			}
		    }
		}
	    xhAdd(&(HTR.Classes), class->ClassName, (void*) class);
	    }
	}

    fdClose(uaConfigFile, 0);
    return 0;
    }

/**
 * This function finds the capabilities of the specified browser for the specified class
 * Both browser and class are required to not be null
 * A null return value indicates that no match was found for the browser
 * A non-null return value should not be freeed or modified in any way
 **/
pHtCapabilities
htr_internal_GetBrowserCapabilities(char *browser, pHtClass class)
    {
    pXArray list;
    pAgentCapabilities agentCap = NULL;
    pHtCapabilities cap = NULL;
    int i;
    if(!browser || !class)
	return NULL;

    list = &(class->Agents);
    for(i=0;i<xaCount(list);i++)
	{
	if ((agentCap = (pAgentCapabilities)xaGetItem(list,i)))
	    {
	    /** 0 signifies a match, REG_NOMATCH signifies the opposite **/
	    if (regexec(&(agentCap->UAreg), browser, (size_t)0, NULL, 0) == 0)
		{
		list = &(agentCap->Children);
		if(xaCount(list)>0)
		    {
		    /** remember this point in case there are no more matches **/
		    cap = &(agentCap->Capabilities);
		    /** reset to the beginning of the list **/
		    i=0;
		    }
		else
		    {
		    /** no children -- this is a terminal node **/
		    return &(agentCap->Capabilities);
		    }
		}
	    }
	}
    
    /** if we found a match while walking the tree (at a non-terminal node), but
	nothing under it matched, cap will not be null, otherwise it will be 
	(we don't get here if we matched a terminal node) **/
    return cap;
    }


/*** htr_internal_AddTextToArray - adds a string of text to an array of 
 *** buffer blocks, allocating new blocks in the XArray if necessary.
 ***/
int
htr_internal_AddTextToArray(pXArray arr, char* txt)
    {
    int l,n,cnt;
    char* ptr;

    	/** Need new block? **/
	if (arr->nItems == 0)
	    {
	    ptr = (char*)nmMalloc(2048);
	    if (!ptr) return -1;
	    *(int*)ptr = 0;
	    l = 0;
	    xaAddItem(arr,ptr);
	    }
	else
	    {
	    ptr = (char*)(arr->Items[arr->nItems-1]);
	    l = *(int*)ptr;
	    }

	/** Copy into the blocks, allocating more as needed. **/
	n = strlen(txt);
	while(n)
	    {
	    cnt = n;
	    if (cnt > (2040-l)) cnt = 2040-l;
	    memcpy(ptr+l+8,txt,cnt);
	    n -= cnt;
	    txt += cnt;
	    l += cnt;
	    *(int*)ptr = l;
	    if (n)
	        {
		ptr = (char*)nmMalloc(2048);
		if (!ptr) return -1;
		*(int*)ptr = 0;
		l = 0;
		xaAddItem(arr,ptr);
		}
	    }

    return 0;
    }


/*** htrRenderWidget - generate a widget into the HtPage structure, given the
 *** widget's objectsystem descriptor...
 ***/
int
htrRenderWidget(pHtSession session, pObject widget_obj, int z, char* parentname, char* parentobj)
    {
    char* w_name;
    pHtDriver drv;
    pXHashTable widget_drivers = NULL;

	/** Find the hashtable keyed with widget names for this combination of 
	 ** user-agent:style that contains pointers to the drivers to use.
	 **/
	if(!session->Class)
	    {
	    printf("Class not defined %s:%i\n",__FILE__,__LINE__);
	    }
	widget_drivers = &( session->Class->WidgetDrivers);
	if (!widget_drivers)
	    {
	    htrAddBodyItem_va(session, "No widgets have been defined for your browser type and requested class combination.");
	    mssError(1, "HTR", "Invalid UserAgent:class combination");
	    return -1;
	    }

	/** Get the name of the widget.. **/
	objGetAttrValue(widget_obj, "outer_type", DATA_T_STRING, POD(&w_name));
	if (strncmp(w_name,"widget/",7)) 
	    {
	    mssError(1,"HTR","Invalid content type for widget - must be widget/xxx");
	    return -1;
	    }

	/** Lookup the driver **/
	drv = (pHtDriver)xhLookup(widget_drivers,w_name+7);
	if (!drv) 
	    {
	    mssError(1,"HTR","Unknown widget object type '%s'", w_name);
	    return -1;
	    }

    return drv->Render(session, widget_obj, z, parentname, parentobj);
    }


/*** htrAddStylesheetItem -- copies stylesheet definitions into the
 *** buffers that will eventually be output as HTML.
 ***/
int 
htrAddStylesheetItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlStylesheet), html_text);
    }


/*** htrAddHeaderItem -- copies html text into the buffers that will 
 *** eventually be output as the HTML header.
 ***/
int 
htrAddHeaderItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlHeader), html_text);
    }


/*** htrAddBodyItem -- copies html text into the buffers that will 
 *** eventually be output as the HTML body.
 ***/
int 
htrAddBodyItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlBody), html_text);
    }


/*** htrAddExpressionItem -- copies html text into the buffers that will 
 *** eventually be output as the HTML body.
 ***/
int 
htrAddExpressionItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlExpressionInit), html_text);
    }


/*** htrAddBodyParam -- copies html text into the buffers that will
 *** eventually be output as the HTML body.  These are simple html tag
 *** parameters (i.e., "BGCOLOR=white")
 ***/
int 
htrAddBodyParam(pHtSession s, char* html_param)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlBodyParams), html_param);
    }


extern int __data_start;

/*** htr_internal_AddText() - use vararg mechanism to add text using one of
 *** the standard add routines.
 ***/
int
htr_internal_AddText(pHtSession s, int (*fn)(), char* fmt, va_list va)
    {
    va_list orig_va;
    int rval;
    char* new_buf;
    int new_buf_size;

	/** Print a warning if we think the format string isn't a constant.
	 ** We'll need to upgrade this once htdrivers start being loaded as
	 ** modules, since their text segments will have different addresses
	 ** and we'll then have to read /proc/self/maps manually.
	 **/
#ifdef WITH_SECWARN
	if ((unsigned int)fmt > (unsigned int)(&__data_start))
	    {
	    printf("***WARNING*** htrXxxYyy_va() format string '%s' at address 0x%X > 0x%X may not be a constant.\n",fmt,(unsigned int)fmt,(unsigned int)(&__data_start));
	    }
#endif

	/** Save the current va_list state so we can retry it. **/
	orig_va = va;

	/** Attempt to print the thing to the tmpbuf. **/
	while(1)
	    {
	    rval = vsnprintf(s->Tmpbuf, s->TmpbufSize, fmt, va);

	    /** Sigh.  Some libc's return -1 and some return # bytes that would be written. **/
	    if (rval < 0 || rval > (s->TmpbufSize - 1))
		{
		/** I think I need a bigger box.  Fix it and try again. **/
		new_buf_size = s->TmpbufSize * 2;
		while(new_buf_size < rval) new_buf_size *= 2;
		new_buf = nmSysMalloc(new_buf_size);
		if (!new_buf)
		    {
		    return -1;
		    }
		nmSysFree(s->Tmpbuf);
		s->Tmpbuf = new_buf;
		s->TmpbufSize = new_buf_size;
		va = orig_va;
		}
	    else
		{
		break;
		}
	    }

	/** Ok, now add the tmpbuf normally. **/
	fn(s, s->Tmpbuf);

    return 0;
    }


/*** htrAddBodyItem_va() - use a vararg list (like sprintf, etc) to add a 
 *** formatted string to the body of the document.
 ***/
int
htrAddBodyItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_AddText(s, htrAddBodyItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddExpressionItem_va() - use a vararg list (like sprintf, etc) to add a 
 *** formatted string to the body of the document.
 ***/
int
htrAddExpressionItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_AddText(s, htrAddExpressionItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddStylesheetItem_va() - use a vararg list (like sprintf, etc) to add a 
 *** formatted string to the stylesheet definition of the document.
 ***/
int
htrAddStylesheetItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_AddText(s, htrAddStylesheetItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddHeaderItem_va() - use a vararg list (like sprintf, etc) to add a 
 *** formatted string to the header of the document.
 ***/
int
htrAddHeaderItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_AddText(s, htrAddHeaderItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddBodyParam_va() - use a vararg list (like sprintf, etc) to add a 
 *** formatted string to the body tag of the document.
 ***/
int
htrAddBodyParam_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_AddText(s, htrAddBodyParam, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddScriptInit_va() - use a vararg list (like sprintf, etc) to add a 
 *** formatted string to startup function of the document.
 ***/
int
htrAddScriptInit_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_AddText(s, htrAddScriptInit, fmt, va);
	va_end(va);

    return 0;
    }

/*** htrAddScriptCleanup_va() - use a vararg list (like sprintf, etc) to add a 
 *** formatted string to cleanup function of the document.
 ***/
int
htrAddScriptCleanup_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_AddText(s, htrAddScriptCleanup, fmt, va);
	va_end(va);

    return 0;
    }



/*** htrAddScriptInclude -- adds a script src= type entry between the html
 *** header and html body.
 ***/
int
htrAddScriptInclude(pHtSession s, char* filename, int flags)
    {
    pStrValue sv;

    	/** Alloc the string val. **/
	if (xhLookup(&(s->Page.NameIncludes), filename)) return 0;
	sv = (pStrValue)nmMalloc(sizeof(StrValue));
	if (!sv) return -1;
	sv->Name = filename;
	if (flags & HTR_F_NAMEALLOC) sv->NameSize = strlen(filename)+1;
	sv->Value = "";
	sv->Alloc = (flags & HTR_F_NAMEALLOC);

	/** Add to the hash table and array **/
	xhAdd(&(s->Page.NameIncludes), filename, (char*)sv);
	xaAddItem(&(s->Page.Includes), (char*)sv);

    return 0;
    }


/*** htrAddScriptFunction -- adds a script function to the list of functions
 *** that will be output.  Note that duplicate functions won't be added, so
 *** the widget drivers need not keep track of this.
 ***/
int 
htrAddScriptFunction(pHtSession s, char* fn_name, char* fn_text, int flags)
    {
    pStrValue sv;

    	/** Alloc the string val. **/
	if (xhLookup(&(s->Page.NameFunctions), fn_name)) return 0;
	sv = (pStrValue)nmMalloc(sizeof(StrValue));
	if (!sv) return -1;
	sv->Name = fn_name;
	if (flags & HTR_F_NAMEALLOC) sv->NameSize = strlen(fn_name)+1;
	sv->Value = fn_text;
	if (flags & HTR_F_VALUEALLOC) sv->ValueSize = strlen(fn_text)+1;
	sv->Alloc = flags;

	/** Add to the hash table and array **/
	xhAdd(&(s->Page.NameFunctions), fn_name, (char*)sv);
	xaAddItem(&(s->Page.Functions), (char*)sv);

    return 0;
    }


/*** htrAddScriptGlobal -- adds a global variable to the list of variables
 *** to be output in the HTML JavaScript section.  Duplicates are suppressed.
 ***/
int 
htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags)
    {
    pStrValue sv;

    	/** Alloc the string val. **/
	if (xhLookup(&(s->Page.NameGlobals), var_name)) return 0;
	sv = (pStrValue)nmMalloc(sizeof(StrValue));
	if (!sv) return -1;
	sv->Name = var_name;
	sv->NameSize = strlen(var_name)+1;
	sv->Value = initialization;
	sv->ValueSize = strlen(initialization)+1;
	sv->Alloc = flags;

	/** Add to the hash table and array **/
	xhAdd(&(s->Page.NameGlobals), var_name, (char*)sv);
	xaAddItem(&(s->Page.Globals), (char*)sv);

    return 0;
    }


/*** htrAddScriptInit -- adds some initialization text that runs outside of a
 *** function context in the HTML JavaScript.
 ***/
int 
htrAddScriptInit(pHtSession s, char* init_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.Inits), init_text);
    }

/*** htrAddScriptCleanup -- adds some initialization text that runs outside of a
 *** function context in the HTML JavaScript.
 ***/
int 
htrAddScriptCleanup(pHtSession s, char* init_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.Cleanups), init_text);
    }

/*** htrAddEventHandler - adds an event handler script code segment for a
 *** given event on a given object (usually the 'document').
 ***/
int
htrAddEventHandler(pHtSession s, char* event_src, char* event, char* drvname, char* handler_code)
    {
    pHtNameArray obj, evt, drv;

    	/** Is this object already listed? **/
	obj = (pHtNameArray)xhLookup(&(s->Page.EventScripts.HashTable), event_src);

	/** If not, create new object for this event source **/
	if (!obj)
	    {
	    obj = (pHtNameArray)nmMalloc(sizeof(HtNameArray));
	    if (!obj) return -1;
	    memccpy(obj->Name, event_src, 0, 127);
	    obj->Name[127] = '\0';
	    xhInit(&(obj->HashTable),257,0);
	    xaInit(&(obj->Array),16);
	    xhAdd(&(s->Page.EventScripts.HashTable), obj->Name, (void*)(obj));
	    xaAddItem(&(s->Page.EventScripts.Array), (void*)obj);
	    }

	/** Is this event name already listed? **/
	evt = (pHtNameArray)xhLookup(&(obj->HashTable), event);
	
	/** If not already, create new. **/
	if (!evt)
	    {
	    evt = (pHtNameArray)nmMalloc(sizeof(HtNameArray));
	    if (!evt) return -1;
	    memccpy(evt->Name, event,0,127);
	    evt->Name[127] = '\0';
	    xhInit(&(evt->HashTable),257,0);
	    xaInit(&(evt->Array),16);
	    xhAdd(&(obj->HashTable), evt->Name, (void*)evt);
	    xaAddItem(&(obj->Array), (void*)evt);
	    }

	/** Is the driver name already listed? **/
	drv = (pHtNameArray)xhLookup(&(evt->HashTable),drvname);

	/** If not already, add new. **/
	if (!drv)
	    {
	    drv = (pHtNameArray)nmMalloc(sizeof(HtNameArray));
	    if (!drv) return -1;
	    memccpy(drv->Name, drvname, 0, 127);
	    drv->Name[127] = '\0';
	    xaInit(&(drv->Array),16);
	    xhAdd(&(evt->HashTable), drv->Name, (void*)drv);
	    xaAddItem(&(evt->Array), (void*)drv);
	   
	    /** Ok, got event and object.  Now, add script text. **/
            htr_internal_AddTextToArray(&(drv->Array), handler_code);
	    }

    return 0;
    }


/*** htrDisableBody - disables the <BODY> </BODY> tags so that, for instance,
 *** a frameset item can be used.
 ***/
int
htrDisableBody(pHtSession s)
    {
    s->DisableBody = 1;
    return 0;
    }


/*** htrAddEvent - adds an event to a driver.
 ***/
int
htrAddEvent(pHtDriver drv, char* event_name)
    {
    pHtEventAction event;

	/** Create the action **/
	event = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	if (!event) return -1;
	memccpy(event->Name, event_name, 0, 31);
	event->Name[31] = '\0';
	xaInit(&(event->Parameters),16);
	xaAddItem(&drv->Events, (void*)event);

    return 0;
    }


/*** htrAddAction - adds an action to a widget.
 ***/
int
htrAddAction(pHtDriver drv, char* action_name)
    {
    pHtEventAction action;

	/** Create the action **/
	action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	if (!action) return -1;
	memccpy(action->Name, action_name, 0, 31);
	action->Name[31] = '\0';
	xaInit(&(action->Parameters),16);
	xaAddItem(&drv->Actions, (void*)action);

    return 0;
    }


/*** htrAddParam - adds a parameter to a widget's action or event
 ***/
int
htrAddParam(pHtDriver drv, char* eventaction, char* param_name, int datatype)
    {
    pHtEventAction ea = NULL;
    int i;
    pHtParam p;

	/** Look for a matching event/action **/
	for(i=0;i<drv->Actions.nItems;i++)
	    {
	    if (!strcmp(((pHtEventAction)(drv->Actions.Items[i]))->Name, eventaction))
	        {
		ea = (pHtEventAction)(drv->Actions.Items[i]);
		break;
		}
	    }
	if (!ea) for(i=0;i<drv->Events.nItems;i++)
	    {
	    if (!strcmp(((pHtEventAction)(drv->Events.Items[i]))->Name, eventaction))
	        {
		ea = (pHtEventAction)(drv->Events.Items[i]);
		break;
		}
	    }
	if (!ea) return -1;

	/** Add the parameter **/
	p = nmSysMalloc(sizeof(HtParam));
	if (!p) return -1;
	memccpy(p->ParamName, param_name, 0, 31);
	p->ParamName[31] = '\0';
	p->DataType = datatype;
	xaAddItem(&(ea->Parameters), (void*)p);

    return 0;
    }


/*** htrAddBodyItemLayer_va - adds an entire "layer" to the document.  A layer
 *** in the traditional netscape-4 sense of a layer, which might be implemented
 *** in a number of different ways in the actual user agent itself.
 ***/
int
htrAddBodyItemLayer_va(pHtSession s, int flags, char* id, int cnt, const char* fmt, ...)
    {
    va_list va;

	/** Add the opening tag **/
	htrAddBodyItemLayerStart(s, flags, id, cnt);

	/** Add the content **/
	va_start(va, fmt);
	htr_internal_AddText(s, htrAddBodyItem, (char*)fmt, va);
	va_end(va);

	/** Add the closing tag **/
	htrAddBodyItemLayerEnd(s, flags);

    return 0;
    }


/*** htrAddBodyItemLayerStart - adds just the opening tag sequence 
 *** but not some content for a layer.  Does not add the closing tag sequence
 *** for the layer.
 ***
 *** WARNING!!! DO NOT ALLOW THE END-USER TO INFLUENCE THE VALUE OF THE 'id'
 *** PARAMETER WHICH IS A FORMAT STRING FOR THE LAYER'S ID!!!
 ***/
int
htrAddBodyItemLayerStart(pHtSession s, int flags, char* id, int cnt)
    {
    char* starttag;
    char id_sbuf[64];

	if(s->Capabilities.HTML40)
	    {
	    if (flags & HTR_LAYER_F_DYNAMIC)
		starttag = "IFRAME frameBorder=\"0\"";
	    else
		starttag = "DIV";
	    }
	else
	    {
	    starttag = "DIV";
	    }

	/** Add it. **/
	snprintf(id_sbuf,sizeof(id_sbuf),id,cnt);
	htrAddBodyItem_va(s, "<%s id=\"%s\">", starttag, id_sbuf);

    return 0;
    }


/*** htrAddBodyItemLayerEnd - adds the ending tag
 *** for a layer.  Does not emit the starting tag.
 ***/
int
htrAddBodyItemLayerEnd(pHtSession s, int flags)
    {
    char* endtag;

	if(s->Capabilities.HTML40)
	    {
	    if (flags & HTR_LAYER_F_DYNAMIC)
		endtag = "IFRAME";
	    else
		endtag = "DIV";
	    }
	else
	    {
	    endtag = "DIV";
	    }

	/** Add it. **/
	htrAddBodyItem_va(s, "</%s>", endtag);

    return 0;
    }


/*** htrAddExpression - adds an expression to control a given property of
 *** an object.  When any object reference in the expression changes, the
 *** expression will be re-run to modify the object's property.  The
 *** object in question can then put a watchpoint on the property, causing
 *** actual actions to occur based on changes in the value of the expression
 *** during application operation.
 ***/
int
htrAddExpression(pHtSession s, char* objname, char* property, pExpression exp)
    {
    int i,first;
    XArray objs, props;
    XString xs,exptxt;
    char* obj;
    char* prop;

	xaInit(&objs, 16);
	xaInit(&props, 16);
	xsInit(&xs);
	xsInit(&exptxt);
	expGetPropList(exp, &objs, &props);

	xsCopy(&xs,"new Array(",-1);
	first=1;
	for(i=0;i<objs.nItems;i++)
	    {
	    obj = (char*)(objs.Items[i]);
	    prop = (char*)(props.Items[i]);
	    if (obj && prop)
		{
		xsConcatPrintf(&xs,"%snew Array('%s','%s')",first?"":",",obj,prop);
		first = 0;
		}
	    }
	xsConcatenate(&xs,")",1);
	expGenerateText(exp, NULL, xsWrite, &exptxt, '\\', "javascript");
	htrAddExpressionItem_va(s, "    pg_expression('%s','%s','%s',%s);\n", objname, property, exptxt.String, xs.String);

	for(i=0;i<objs.nItems;i++)
	    {
	    if (objs.Items[i]) nmSysFree(objs.Items[i]);
	    if (props.Items[i]) nmSysFree(props.Items[i]);
	    }
	xaDeInit(&objs);
	xaDeInit(&props);
	xsDeInit(&xs);
	xsDeInit(&exptxt);

    return 0;
    }


/*** htrRenderSubwidgets - generates the code for all subwidgets within
 *** the current widget.  This is  a generic function that does not 
 *** necessarily apply to all widgets that contain other widgets, but 
 *** is useful for your basic ordinary "container" type widget, such
 *** as panes and tab pages.
 ***/
int
htrRenderSubwidgets(pHtSession s, pObject widget_obj, char* docname, char* layername, int zlevel)
    {
    pObjQuery qy;
    pObject sub_widget_obj;

	/** Open the query for subwidgets **/
	qy = objOpenQuery(widget_obj, "", NULL, NULL, NULL);
	if (qy)
	    {
	    while((sub_widget_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_widget_obj, zlevel, docname, layername);
		objClose(sub_widget_obj);
		}
	    objQueryClose(qy);
	    }

    return 0;
    }


/*** htrRender - generate an HTML document given the app structure subtree
 *** as an open ObjectSystem object.
 ***/
int
htrRender(pFile output, pObject appstruct)
    {
    pHtSession s;
    int i,n,j,k,l;
    pStrValue tmp;
    char* ptr;
    pStrValue sv;
    char sbuf[HT_SBUF_SIZE];
    char ename[40];
    pHtNameArray tmp_a, tmp_a2, tmp_a3;
    char* agent = NULL;
    char* classname = NULL;
    int rval;

	/** What UA is on the other end of the connection? **/
	agent = (char*)mssGetParam("User-Agent");
	if (!agent)
	    {
	    mssError(1, "HTR", "User-Agent undefined in the session parameters");
	    return -1;
	    }

    	/** Initialize the session **/
	s = (pHtSession)nmMalloc(sizeof(HtSession));
	if (!s) return -1;
	memset(s,0,sizeof(HtSession));

	/** Did user request a class of widgets? **/
	classname = (char*)mssGetParam("Class");
	if (classname)
	    {
	    s->Class = (pHtClass)xhLookup(&(HTR.Classes), classname);
	    if(!s->Class)
		mssError(1,"HTR","Warning: class %s is not defined... acting like it wasn't specified",classname);
	    }
	else	
	    s->Class = NULL;

	/** find the right capabilities for the class we're using **/
	if(s->Class)
	    {
	    pHtCapabilities pCap = htr_internal_GetBrowserCapabilities(agent, s->Class);
	    if(pCap)
		{
		s->Capabilities = *pCap;
		}
	    else
		{
		mssError(1,"HTR","no capabilities found for %s in class %s",agent, s->Class->ClassName);
		memset(&(s->Capabilities),0,sizeof(HtCapabilities));
		}
	    }
	else
	    {
	    /** somehow decide on a widget priority, and go down the list
		till you find a workable one **/
	    /** for now, I'm going to get them in the order they are in the hash, 
		which is no order at all :) **/
	    /** also, this sets the class when it finds capabilities.... 
		is that a good thing? -- not sure **/
	    int i;
	    pHtCapabilities pCap = NULL;
	    for(i=0;i<HTR.Classes.nRows && !pCap;i++)
		{
		pXHashEntry ptr = (pXHashEntry)xaGetItem(&(HTR.Classes.Rows),i);
		while(ptr && !pCap)
		    {
		    pCap = htr_internal_GetBrowserCapabilities(agent, (pHtClass)ptr->Data);
		    if(pCap)
			{
			s->Class = (pHtClass)ptr->Data;
			}
		    ptr = ptr->Next;
		    }
		}
	    if(pCap)
		{
		s->Capabilities = *pCap;
		}
	    else
		{
		mssError(1,"HTR","no capabilities found for %s in any class",agent);
		memset(&(s->Capabilities),0,sizeof(HtCapabilities));
		}
	    }

	/** Setup the page structures **/
	s->Tmpbuf = nmSysMalloc(512);
	s->TmpbufSize = 512;
	if (!s->Tmpbuf) 
	    {
	    nmFree(s, sizeof(HtSession));
	    return -1;
	    }
	xhInit(&(s->Page.NameFunctions),257,0);
	xaInit(&(s->Page.Functions),32);
	xhInit(&(s->Page.NameIncludes),257,0);
	xaInit(&(s->Page.Includes),32);
	xhInit(&(s->Page.NameGlobals),257,0);
	xaInit(&(s->Page.Globals),64);
	xaInit(&(s->Page.Inits),64);
	xaInit(&(s->Page.Cleanups),64);
	xaInit(&(s->Page.HtmlBody),64);
	xaInit(&(s->Page.HtmlHeader),64);
	xaInit(&(s->Page.HtmlStylesheet),64);
	xaInit(&(s->Page.HtmlBodyParams),16);
	xaInit(&(s->Page.HtmlExpressionInit),16);
	xaInit(&(s->Page.EventScripts.Array),16);
	xhInit(&(s->Page.EventScripts.HashTable),257,0);
	s->Page.HtmlBodyFile = NULL;
	s->Page.HtmlHeaderFile = NULL;
	s->Page.HtmlStylesheetFile = NULL;
	s->DisableBody = 0;

	/** Render the top-level widget. **/
	rval = htrRenderWidget(s, appstruct, 10, "document", "document");

	if (rval < 0)
	    {
	    fdPrintf(output, "<HTML><HEAD><TITLE>Error</TITLE></HEAD><BODY bgcolor=\"white\"><h1>An Error occured while attempting to render this document</h1><br><pre>");
	    mssPrintError(output);
	    }

	/** Output the DOCTYPE for browsers supporting HTML 4.0 -- this will make them use HTML 4.0 Strict **/
	/** FIXME: should probably specify the DTD.... **/
	if(s->Capabilities.HTML40)
	    fdWrite(output, "<!DOCTYPE HTML>\n",16,0,FD_U_PACKET);
	
	/** Write the HTML out... **/
	snprintf(sbuf, HT_SBUF_SIZE, "<!--\nGenerated by Centrallix v%s (http://www.centrallix.org)\n"
				     "(c) 1998-2002 by LightSys Technology Services, Inc.\n\n", PACKAGE_VERSION);
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	snprintf(sbuf, HT_SBUF_SIZE, "This DHTML document contains Javascript and other DHTML\n"
				     "generated from Centrallix which is licensed under the\n"
				     "GNU GPL (http://www.gnu.org/licenses/gpl.txt).  Any copying\n");
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	snprintf(sbuf, HT_SBUF_SIZE, "modifying, or redistributing of this generated code falls\n"
				     "under the restrictuions of the GPL.\n"
				     "-->\n");
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	fdWrite(output, "<HTML>\n<HEAD>\n",14,0,FD_U_PACKET);
	snprintf(sbuf, HT_SBUF_SIZE, "    <META NAME=\"Generator\" CONTENT=\"Centrallix v%s\">\n", cx__version);
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);

	fdWrite(output, "    <STYLE TYPE=\"text/css\">\n", 28, 0, FD_U_PACKET);
	/** Write the HTML header items. **/
	for(i=0;i<s->Page.HtmlStylesheet.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlStylesheet.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output, "    </STYLE>\n", 13, 0, FD_U_PACKET);
	/** Write the HTML header items. **/
	for(i=0;i<s->Page.HtmlHeader.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlHeader.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output, "\n</HEAD>\n",9,0,FD_U_PACKET);

	/** Write the script globals **/
	fdWrite(output, "<SCRIPT language=javascript>\n\n\n", 31,0,FD_U_PACKET);
	for(i=0;i<s->Page.Globals.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Globals.Items[i]);
	    if (sv->Value[0])
		snprintf(sbuf,HT_SBUF_SIZE,"var %s = %s;\n", sv->Name, sv->Value);
	    else
		snprintf(sbuf,HT_SBUF_SIZE,"var %s;\n", sv->Name);
	    fdWrite(output, sbuf, strlen(sbuf),0,FD_U_PACKET);
	    }

	/** Write the includes **/
	fdWrite(output, "\n</SCRIPT>\n\n", 12,0,FD_U_PACKET);

	/** include ht_render.js **/
	snprintf(sbuf,HT_SBUF_SIZE,"<SCRIPT language=javascript src=\"/sys/js/ht_render.js\"></SCRIPT>\n\n");
	fdWrite(output, sbuf, strlen(sbuf), 0,FD_U_PACKET);
	for(i=0;i<s->Page.Includes.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Includes.Items[i]);
	    snprintf(sbuf,HT_SBUF_SIZE,"<SCRIPT language=javascript src=\"%s\"></SCRIPT>\n\n",sv->Name);
	    fdWrite(output, sbuf, strlen(sbuf), 0,FD_U_PACKET);
	    }
	fdWrite(output, "<SCRIPT language=javascript>\n\n", 30,0,FD_U_PACKET);

	/** Write the script functions **/
	for(i=0;i<s->Page.Functions.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Functions.Items[i]);
	    fdWrite(output, sv->Value, strlen(sv->Value),0,FD_U_PACKET);
	    }

	/** Write the event scripts themselves. **/
	for(i=0;i<s->Page.EventScripts.Array.nItems;i++)
	    {
	    tmp_a = (pHtNameArray)(s->Page.EventScripts.Array.Items[i]);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
	        snprintf(sbuf,HT_SBUF_SIZE,"\nfunction e%d_%d(e)\n    {\n",i,j);
		fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);
	        snprintf(sbuf,HT_SBUF_SIZE,"    var e = htr_event(e);\n");
		fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);
		for(k=0;k<tmp_a2->Array.nItems;k++)
		    {
		    tmp_a3 = (pHtNameArray)(tmp_a2->Array.Items[k]);
		    for(l=0;l<tmp_a3->Array.nItems;l++)
		        {
		        ptr = (char*)(tmp_a3->Array.Items[l]);
		        fdWrite(output,ptr+8,*(int*)ptr,0,FD_U_PACKET);
			}
		    }
		fdWrite(output,"    return true;\n    }\n",23,0,FD_U_PACKET);
		}
	    }

	/** Write the event capture lines **/
	fdWrite(output,"\nfunction events()\n    {\n", 25,0,FD_U_PACKET);
	for(i=0;i<s->Page.EventScripts.Array.nItems;i++)
	    {
	    tmp_a = (pHtNameArray)(s->Page.EventScripts.Array.Items[i]);
	    snprintf(sbuf,HT_SBUF_SIZE,"    %.64s.captureEvents(",tmp_a->Name);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
		if (j!=0) strcat(sbuf," | ");
		strcat(sbuf,"Event.");
		strcat(sbuf,tmp_a2->Name);
		}
	    strcat(sbuf,");\n");
	    fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
		n = strlen(tmp_a2->Name);
		for(k=0;k<=n;k++) ename[k] = tolower(tmp_a2->Name[k]);
		snprintf(sbuf,HT_SBUF_SIZE,"    %.64s.on%s=e%d_%d;\n",tmp_a->Name,ename,i,j);
		fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);
		}
	    }
	fdWrite(output,"    }\n",6,0,FD_U_PACKET);

	/** Write the expression initializations **/
	fdWrite(output,"\nfunction expinit()\n    {\n",26,0,FD_U_PACKET);
	for(i=0;i<s->Page.HtmlExpressionInit.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlExpressionInit.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output,"    }\n",6,0,FD_U_PACKET);

	/** Write the initialization lines **/
	fdWrite(output,"\nfunction startup()\n    {\n",26,0,FD_U_PACKET);
	htr_internal_writeCxCapabilities(s,output);

	fdWrite(output,"    if(typeof(pg_status_init)=='function')pg_status_init();\n",60,0,FD_U_PACKET);
	for(i=0;i<s->Page.Inits.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Inits.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output,"    events();\n", 14,0,FD_U_PACKET);
	fdWrite(output,"    expinit();\n", 15,0,FD_U_PACKET);
	fdWrite(output,"    if(typeof(pg_status_close)=='function')pg_status_close();\n",62,0,FD_U_PACKET);
	fdWrite(output,"    }\n",6,0,FD_U_PACKET);

	/** Write the cleanup lines **/
	fdWrite(output,"\nfunction cleanup()\n    {\n",26,0,FD_U_PACKET);
	for(i=0;i<s->Page.Cleanups.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Cleanups.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output,"    }\n",6,0,FD_U_PACKET);


	/** If the body part is disabled, skip over body section generation **/
	if (s->DisableBody == 0)
	    {
	    /** Write the HTML body params **/
	    fdWrite(output, "\n</SCRIPT>\n<BODY", 16,0,FD_U_PACKET);
	    for(i=0;i<s->Page.HtmlBodyParams.nItems;i++)
	        {
	        ptr = (char*)(s->Page.HtmlBodyParams.Items[i]);
	        n = *(int*)ptr;
	        fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	        }
	    fdWrite(output, " onResize=\"location.reload()\" onLoad=\"startup();\" onUnload=\"cleanup();\">\n",73,0,FD_U_PACKET);
	    }
	else
	    {
	    fdWrite(output, "\n</SCRIPT>\n",11,0,FD_U_PACKET);
	    }

	/** Write the HTML body. **/
	for(i=0;i<s->Page.HtmlBody.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlBody.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }

	if (s->DisableBody == 0)
	    {
	    fdWrite(output, "</BODY>\n</HTML>\n",16,0,FD_U_PACKET);
	    }
	else
	    {
	    fdWrite(output, "\n</HTML>\n",9,0,FD_U_PACKET);
	    }

	/** Deinitialize the session and page structures **/
	for(i=0;i<s->Page.Functions.nItems;i++)
	    {
	    tmp = (pStrValue)(s->Page.Functions.Items[i]);
	    xhRemove(&(s->Page.NameFunctions),tmp->Name);
	    if (tmp->Alloc & HTR_F_NAMEALLOC) nmFree(tmp->Name,tmp->NameSize);
	    if (tmp->Alloc & HTR_F_VALUEALLOC) nmFree(tmp->Value,tmp->ValueSize);
	    nmFree(tmp,sizeof(StrValue));
	    }
	xaDeInit(&(s->Page.Functions));
	xhDeInit(&(s->Page.NameFunctions));
	for(i=0;i<s->Page.Includes.nItems;i++)
	    {
	    tmp = (pStrValue)(s->Page.Includes.Items[i]);
	    xhRemove(&(s->Page.NameIncludes),tmp->Name);
	    if (tmp->Alloc & HTR_F_NAMEALLOC) nmFree(tmp->Name,tmp->NameSize);
	    nmFree(tmp,sizeof(StrValue));
	    }
	xaDeInit(&(s->Page.Includes));
	xhDeInit(&(s->Page.NameIncludes));
	for(i=0;i<s->Page.Globals.nItems;i++)
	    {
	    tmp = (pStrValue)(s->Page.Globals.Items[i]);
	    xhRemove(&(s->Page.NameGlobals),tmp->Name);
	    if (tmp->Alloc & HTR_F_NAMEALLOC) nmFree(tmp->Name,tmp->NameSize);
	    if (tmp->Alloc & HTR_F_VALUEALLOC) nmFree(tmp->Value,tmp->ValueSize);
	    nmFree(tmp,sizeof(StrValue));
	    }
	xaDeInit(&(s->Page.Globals));
	xhDeInit(&(s->Page.NameGlobals));
	for(i=0;i<s->Page.Inits.nItems;i++) nmFree(s->Page.Inits.Items[i],2048);
	xaDeInit(&(s->Page.Inits));
	for(i=0;i<s->Page.Cleanups.nItems;i++) nmFree(s->Page.Cleanups.Items[i],2048);
	xaDeInit(&(s->Page.Cleanups));
	for(i=0;i<s->Page.HtmlBody.nItems;i++) nmFree(s->Page.HtmlBody.Items[i],2048);
	xaDeInit(&(s->Page.HtmlBody));
	for(i=0;i<s->Page.HtmlHeader.nItems;i++) nmFree(s->Page.HtmlHeader.Items[i],2048);
	xaDeInit(&(s->Page.HtmlHeader));
	for(i=0;i<s->Page.HtmlStylesheet.nItems;i++) nmFree(s->Page.HtmlStylesheet.Items[i],2048);
	xaDeInit(&(s->Page.HtmlStylesheet));
	for(i=0;i<s->Page.HtmlBodyParams.nItems;i++) nmFree(s->Page.HtmlBodyParams.Items[i],2048);
	xaDeInit(&(s->Page.HtmlBodyParams));
	for(i=0;i<s->Page.HtmlExpressionInit.nItems;i++) nmFree(s->Page.HtmlExpressionInit.Items[i],2048);
	xaDeInit(&(s->Page.HtmlExpressionInit));

	/** Clean up the event script structure, which is multi-level. **/
	for(i=0;i<s->Page.EventScripts.Array.nItems;i++)
	    {
	    tmp_a = (pHtNameArray)(s->Page.EventScripts.Array.Items[i]);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
		for(k=0;k<tmp_a2->Array.nItems;k++) 
		    {
		    tmp_a3 = (pHtNameArray)(tmp_a2->Array.Items[k]);
		    for(l=0;l<tmp_a3->Array.nItems;l++)
		        {
			nmFree(tmp_a3->Array.Items[l],2048);
			}
	            xaDeInit(&(tmp_a3->Array));
	            xhRemove(&(tmp_a2->HashTable),tmp_a3->Name);
		    nmFree(tmp_a3,sizeof(HtNameArray));
		    }
	        xaDeInit(&(tmp_a2->Array));
		xhDeInit(&(tmp_a2->HashTable));
	        xhRemove(&(tmp_a->HashTable),tmp_a2->Name);
	        nmFree(tmp_a2,sizeof(HtNameArray));
		}
	    xaDeInit(&(tmp_a->Array));
	    xhDeInit(&(tmp_a->HashTable));
	    xhRemove(&(s->Page.EventScripts.HashTable),tmp_a->Name);
	    nmFree(tmp_a,sizeof(HtNameArray));
	    }
	xhDeInit(&(s->Page.EventScripts.HashTable));
	xaDeInit(&(s->Page.EventScripts.Array));

	nmSysFree(s->Tmpbuf);

	nmFree(s,sizeof(HtSession));

    return 0;
    }


/*** htrAllocDriver - allocates a driver structure that can later be
 *** registered by using the next function below, htrRegisterDriver().
 ***/
pHtDriver
htrAllocDriver()
    {
    pHtDriver drv;

	/** Allocate the driver structure **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return NULL;

	/** Init some of the basic array structures **/
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
    return drv;
    }


/*** htrAddSupport - adds support for a class to a driver (by telling the class)
 ***   note: _must_ be called after the driver registers
 ***/
int
htrAddSupport(pHtDriver drv, char* className)
    {
	pHtClass class = (pHtClass)xhLookup(&(HTR.Classes),className);
	if(!class)
	    {
	    mssError(1,"HTR","unable to find class '%s' for widget driver '%s'",className,drv->WidgetName);
	    return -1;
	    }
	return xhAdd(&(class->WidgetDrivers),drv->WidgetName, (void*)drv);

    return 0;
    }

/*** htrRegisterDriver - register a new driver with the rendering system
 *** and map the widget name to the driver's structure for later access.
 ***/
int 
htrRegisterDriver(pHtDriver drv)
    {
    	/** Add to the drivers listing and the widget name map. **/
	xaAddItem(&(HTR.Drivers),(void*)drv);

    return 0;
    }


/*** htrInitialize - initialize the system and the global variables and
 *** structures.
 ***/
int
htrInitialize()
    {
    	/** Initialize the global hash tables and arrays **/
	xaInit(&(HTR.Drivers),64);
	xhInit(&(HTR.Classes), 63, 0);

	/** Register the classes, user agents and the regular expressions to match them.  **/
	htrRegisterUserAgents();

    return 0;
    }

