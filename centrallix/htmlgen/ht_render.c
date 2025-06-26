#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <stdarg.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "expression.h"
#include "cxlib/qprintf.h"
#include <assert.h>

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



/*** GLOBALS ***/
struct
    {
    XArray	Drivers;		/* simple driver listing. */
    XHashTable	Classes;		/* classes of widget sets (see centrallix/htmlgen/README) */
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
		agentCap->Capabilities.attr = 0; \
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
    PROCESS_CAP_INIT(CSSBox);
    PROCESS_CAP_INIT(CSSClip);
    PROCESS_CAP_INIT(HTML40);
    PROCESS_CAP_INIT(JS15);
    PROCESS_CAP_INIT(XMLHttpRequest);

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
htr_internal_writeCxCapabilities(pHtSession s)
    {
    htrWrite(s,"    cx__capabilities = {};\n",27);
#define PROCESS_CAP_OUT(attr) \
    htrWrite(s,"    cx__capabilities.",21); \
    htrWrite(s, # attr ,strlen( # attr )); \
    htrWrite(s," = ",3); \
    htrWrite(s,(s->Capabilities.attr?"1;\n":"0;\n"),3);

    PROCESS_CAP_OUT(Dom0NS); //true when navigator is netscape navigator
    PROCESS_CAP_OUT(Dom0IE); //true when navigator is IE
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
    PROCESS_CAP_OUT(CSSBox); //true if navigator implements the CSSBox model enough by the standards (aka, if it's not IE)
    PROCESS_CAP_OUT(CSSClip);
    PROCESS_CAP_OUT(HTML40);
    PROCESS_CAP_OUT(JS15);
    PROCESS_CAP_OUT(XMLHttpRequest);
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
	return -1;
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
		    i = -1;
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
htrRenderWidget(pHtSession session, pWgtrNode widget, int z)
    {
    pHtDriver drv;
    pXHashTable widget_drivers = NULL;
    int rval;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"HTR","Could not render application: resource exhaustion occurred");
	    return -1;
	    }

	/** Find the hashtable keyed with widget names for this combination of
	 ** user-agent:style that contains pointers to the drivers to use.
	 **/
	if(!session->Class)
	    {
	    printf("Class not defined %s:%i\n",__FILE__,__LINE__);
	    return -1;
	    }
	widget_drivers = &( session->Class->WidgetDrivers);
	if (!widget_drivers)
	    {
	    htrAddBodyItem_va(session, "No widgets have been defined for your browser type and requested class combination.");
	    mssError(1, "HTR", "Invalid UserAgent:class combination");
	    return -1;
	    }

	/** Get the name of the widget.. **/
	if (strncmp(widget->Type,"widget/",7))
	    {
	    mssError(1,"HTR","Invalid content type for widget - must be widget/xxx");
	    return -1;
	    }

	/** Lookup the driver. These are what are defined in the various htdrv_* functions **/
	drv = (pHtDriver)xhLookup(widget_drivers,widget->Type+7);
	if (!drv)
	    {
	    mssError(1,"HTR","Unknown widget object type '%s'", widget->Type);
	    return -1;
	    }

	/** Has this driver been used this session yet? **/
	if (xhLookup(&session->UsedDrivers, drv->WidgetName) == NULL)
	    {
	    xhAdd(&session->UsedDrivers, drv->WidgetName, (void*)drv);
	    if (drv->Setup)
		{
		if (drv->Setup(session) < 0)
		    return -1;
		}
	    }

	/** Crossing a namespace boundary? **/
	htrCheckNSTransition(session, widget->Parent, widget);

	/** will be a *Render function found in a htmlgen/htdrv_*.c file (eg htpageRender) **/
	rval = drv->Render(session, widget, z);

	/** Going back to previous namespace? **/
	htrCheckNSTransitionReturn(session, widget->Parent, widget);

    return rval;
    }


/*** htrCheckNSTransition -- check to see if we are transitioning between
 *** namespaces, and if so, emit the code to switch the context.
 ***/
int
htrCheckNSTransition(pHtSession s, pWgtrNode parent, pWgtrNode child)
    {

	/** Crossing a namespace boundary? **/
	if (child && parent && strcmp(child->Namespace, parent->Namespace) != 0)
	    {
	    htrAddNamespace(s, NULL, child->Namespace, 1);
	    }

    return 0;
    }


/*** htrCheckNSTransitionReturn -- check to see if we are transitioning between
 *** namespaces, and if so, emit the code to switch the context.
 ***/
int
htrCheckNSTransitionReturn(pHtSession s, pWgtrNode parent, pWgtrNode child)
    {

	/** Crossing a namespace boundary? **/
	if (child && parent && strcmp(child->Namespace, parent->Namespace) != 0)
	    {
	    htrLeaveNamespace(s);
	    }

    return 0;
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


/*** htr_internal_GrowFn() - qPrintf grow function to resize tmp buffer on
 *** the fly.
 ***/
int
htr_internal_GrowFn(char** str, size_t* size, size_t offs, void* arg, size_t req_size)
    {
    pHtSession s = (pHtSession)arg;
    char* new_buf;
    size_t new_buf_size;

	if (*size >= req_size) return 1;

	assert(*str == s->Tmpbuf);
	assert(*size == s->TmpbufSize);
	new_buf_size = s->TmpbufSize * 2;
	while(new_buf_size < req_size) new_buf_size *= 2;
	new_buf = nmSysRealloc(s->Tmpbuf, new_buf_size);
	if (!new_buf)
	    return 0;
	*str = s->Tmpbuf = new_buf;
	*size = s->TmpbufSize = new_buf_size;

    return 1; /* OK */
    }


/*** htr_internal_QPAddText() - same as below function, but uses qprintf
 *** instead of snprintf.
 ***/
int
htr_internal_QPAddText(pHtSession s, int (*fn)(), char* fmt, va_list va)
    {
    int rval;

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

	rval = qpfPrintf_va_internal(NULL, &(s->Tmpbuf), &(s->TmpbufSize), htr_internal_GrowFn, (void*)s, fmt, va);
	if (rval < 0)
	    {
	    printf("WARNING:  QPAddText() failed for format: %s\n", fmt);
	    }
	if (rval < 0 || rval > (s->TmpbufSize - 1))
	    return -1;

	/** Ok, now add the tmpbuf normally. **/
	fn(s, s->Tmpbuf);

    return 0;
    }


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
	va_copy(orig_va, va);

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
	htr_internal_QPAddText(s, htrAddBodyItem, fmt, va);
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
	htr_internal_QPAddText(s, htrAddExpressionItem, fmt, va);
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
	htr_internal_QPAddText(s, htrAddStylesheetItem, fmt, va);
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
	htr_internal_QPAddText(s, htrAddHeaderItem, fmt, va);
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
	htr_internal_QPAddText(s, htrAddBodyParam, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddScriptWgtr_va() - use a vararg list to add a formatted string
 *** to build_wgtr_* function of the document
 ***/
int
htrAddScriptWgtr_va(pHtSession s, char* fmt, ...)
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddScriptWgtr, fmt, va);
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

	if (!s->Namespace->HasScriptInits)
	    {
	    /** No script inits for this namespace yet?  Issue the context
	     ** switch if no inits yet.
	     **/
	    s->Namespace->HasScriptInits = 1;
	    /*htrAddScriptInit_va(s, "\n    nodes = wgtrNodeList(%STR&SYM);\n", s->Namespace->DName);*/
	    htrAddScriptInit_va(s, "\n    ns = \"%STR&SYM\";\n", s->Namespace->DName);
	    }

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddScriptInit, fmt, va);
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
	htr_internal_QPAddText(s, htrAddScriptCleanup, fmt, va);
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
    char* scripts_already;

	/** What scripts has the client already loaded?  If the
	 ** requested script is in the list, ignore it since the
	 ** client already has it.
	 **/
	scripts_already = htrParamValue(s, "cx__scripts");
	if (scripts_already && strstr(scripts_already, filename))
	    return 0;

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


/*** htrAddScriptWgtr - adds some text to the wgtr function
 ***/
int
htrAddScriptWgtr(pHtSession s, char* wgtr_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.Wgtr), wgtr_text);
    }


/*** htrAddScriptCleanup -- adds some initialization text that runs outside of a
 *** function context in the HTML JavaScript.
 ***/
int
htrAddScriptCleanup(pHtSession s, char* init_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.Cleanups), init_text);
    }


/*** htrAddEventHandlerFunction - adds an event handler script code segment for
 *** a given event on a given object (usually the 'document').
 ***
 *** GRB note -- event_src is no longer used, should always be 'document'.
 ***          -- drvname is no longer used, just ignored.
 ***/
int
htrAddEventHandlerFunction(pHtSession s, char* event_src, char* event, char* drvname, char* function)
    {
    pHtDomEvent e = NULL;
    pHtDomEvent e_srch;
    int i,cnt;

	/** Look it up? 
	 ** (GRB note -- is this slow enough to need an XHashTable?)
	 **/
	cnt = xaCount(&s->Page.EventHandlers);
	for(i=0;i<cnt;i++)
	    {
	    e_srch = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    if (!strcmp(event, e_srch->DomEvent))
		{
		e = e_srch;
		break;
		}
	    }
	
	/** Make a new one? **/
	if (!e)
	    {
	    e = (pHtDomEvent)nmMalloc(sizeof(HtDomEvent));
	    if (!e) return -1;
	    strtcpy(e->DomEvent, event, sizeof(e->DomEvent));
	    xaInit(&e->Handlers,64);
	    xaAddItem(&s->Page.EventHandlers, e);
	    }

	/** Add our handler **/
	cnt = xaCount(&e->Handlers);
	for(i=0;i<cnt;i++)
	    {
	    if (!strcmp(function, (char*)xaGetItem(&e->Handlers,i)))
		return 0;
	    }
	xaAddItem(&e->Handlers, function);

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
htrAddBodyItemLayer_va(pHtSession s, int flags, char* id, int cnt, char* cls, const char* fmt, ...)
    {
    va_list va;

	/** Add the opening tag **/
	htrAddBodyItemLayerStart(s, flags, id, cnt, cls);

	/** Add the content **/
	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddBodyItem, (char*)fmt, va);
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
htrAddBodyItemLayerStart(pHtSession s, int flags, char* id, int cnt, char* cls)
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
	qpfPrintf(NULL, id_sbuf,sizeof(id_sbuf),id,cnt);
	htrAddBodyItem_va(s, "<%STR %[class=\"%STR&HTE\" %]id=\"%STR&HTE\">", starttag, cls != NULL, cls, id_sbuf);

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
	htrAddBodyItem_va(s, "</%STR>", endtag);

    return 0;
    }


/*** htrGetExpParams - retrieve the list of values used in an expression
 *** and put them in a javascript array.
 ***/
int
htrGetExpParams(pExpression exp, pXString xs)
    {
    int i, first;
    XArray objs, props;
    char* obj;
    char* prop;

	/** setup **/
	xaInit(&objs, 16);
	xaInit(&props, 16);

	/** Find the properties accessed by the expression **/
	expGetPropList(exp, &objs, &props);

	/** Build the list **/
	xsCopy(xs,"[",-1);
	first=1;
	for(i=0;i<objs.nItems;i++)
	    {
	    obj = (char*)(objs.Items[i]);
	    prop = (char*)(props.Items[i]);
	    if (obj && prop)
		{
		xsConcatQPrintf(xs,"%[,%]{obj:'%STR&JSSTR',attr:'%STR&JSSTR'}", !first, obj, prop);
		first = 0;
		}
	    }
	xsConcatenate(xs,"]",1);

	/** cleanup **/
	for(i=0;i<objs.nItems;i++)
	    {
	    if (objs.Items[i]) nmSysFree(objs.Items[i]);
	    if (props.Items[i]) nmSysFree(props.Items[i]);
	    }
	xaDeInit(&objs);
	xaDeInit(&props);

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

	xsCopy(&xs,"[",-1);
	first=1;
	for(i=0;i<objs.nItems;i++)
	    {
	    obj = (char*)(objs.Items[i]);
	    prop = (char*)(props.Items[i]);
	    if (obj && prop)
		{
		xsConcatQPrintf(&xs,"%[,%]['%STR&JSSTR','%STR&JSSTR']", !first, obj, prop);
		first = 0;
		}
	    }
	xsConcatenate(&xs,"]",1);
	expGenerateText(exp, NULL, xsWrite, &exptxt, '\0', "javascript", EXPR_F_RUNCLIENT);
	htrAddExpressionItem_va(s, "    pg_expression('%STR&SYM','%STR&SYM',function (_context, _this) { return %STR; },%STR,'%STR&SYM');\n", objname, property, exptxt.String, xs.String, s->Namespace->DName);

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


/*** htrCheckAddExpression - checks if an expression should be added for a
 *** given widget property, and deploys it to the client if so.
 ***/
int
htrCheckAddExpression(pHtSession s, pWgtrNode tree, char* w_name, char* property)
    {
    pExpression code;

        if (wgtrGetPropertyType(tree,property) == DATA_T_CODE)
            {
            wgtrGetPropertyValue(tree,property,DATA_T_CODE,POD(&code));
            htrAddExpression(s, w_name, property, code);
	    return 1;
            }

    return 0;
    }


/*** htrRenderSubwidgets - generates the code for all subwidgets within
 *** the current widget.  This is  a generic function that does not
 *** necessarily apply to all widgets that contain other widgets, but
 *** is useful for your basic ordinary "container" type widget, such
 *** as panes and tab pages.
 ***/
int
htrRenderSubwidgets(pHtSession s, pWgtrNode widget, int zlevel)
    {
//    pObjQuery qy;
//    pObject sub_widget_obj;
    int i, count;

	/** Open the query for subwidgets **/
	/*
	qy = objOpenQuery(widget_obj, "", NULL, NULL, NULL, 0);
	if (qy)
	    {
	    while((sub_widget_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_widget_obj, zlevel, docname, layername);
		objClose(sub_widget_obj);
		}
	    objQueryClose(qy);
	    }
	**/
	count = xaCount(&(widget->Children));
	for (i=0;i<count;i++) 
	    htrRenderWidget(s, xaGetItem(&(widget->Children), i), zlevel);

    return 0;
    }


/*** htr_internal_GenInclude - generate an include statement for an
 *** external javascript file; or if collapse_includes is set, insert
 *** the thing directly in the output.  If file cannot be opened,
 *** then an include statement is generated in any event.
 ***/
int
htr_internal_GenInclude(pHtSession s, char* filename)
    {
    pStruct c_param;
    pObject include_file;
    char buf[256];
    char path[256];
    char* slash;
    int rcnt;
    ObjData pod;

	/** Insert file directly? **/
	c_param = stLookup_ne(s->Params, "ls__collapse_includes");
	if (c_param && !strcasecmp(c_param->StrVal,"yes"))
	    {
	    include_file = objOpen(s->ObjSession, filename, O_RDONLY, 0600, "application/x-javascript");
	    if (include_file)
		{
		htrQPrintf(s, "<script language=\"javascript\">\n// Included from: %STR&HTE\n\n", filename);
		while((rcnt = objRead(include_file, buf, sizeof(buf), 0, 0)) > 0)
		    {
		    htrWrite(s, buf, rcnt);
		    }
		objClose(include_file);
		htrQPrintf(s, "\n</script>\n");
		return 0;
		}
	    }

	/** Otherwise, just generate an include statement **/
	buf[0] = '\0';
	include_file = objOpen(s->ObjSession, filename, O_RDONLY, 0600, "application/x-javascript");
	if (include_file)
	    {
	    if (objGetAttrValue(include_file, "last_modification", DATA_T_DATETIME, &pod) == 0)
		{
		snprintf(buf, sizeof(buf), "%lld", pod.DateTime->Value);
		}
	    objClose(include_file);
	    }
	strtcpy(path, filename, sizeof(path));
	slash = strrchr(path, '/');
	if (slash)
	    {
	    *slash = '\0';
	    htrQPrintf(s, "\n<script language=\"javascript\" src=\"%STR%[/CXDC:%STR%]/%STR\"></script>\n", path, buf[0], buf, slash+1);
	    }

    return 0;
    }


/*** htr_internal_WriteWgtrProperty - write one widget property in javascript
 *** as a part of the widget tree
 ***/
int
htr_internal_WriteWgtrProperty(pHtSession s, pWgtrNode tree, char* propname)
    {
    int t;
    ObjData od;
    int rval;
    pExpression code;
    XString exptxt;
    XString proptxt;

	t = wgtrGetPropertyType(tree, propname);
	if (t > 0)
	    {
	    rval = wgtrGetPropertyValue(tree, propname, t, &od);
	    if (rval == 1)
		{
		/** null **/
		htrAddScriptWgtr_va(s, "%STR&SYM:null, ", propname);
		}
	    else if (rval == 0)
		{
		switch(t)
		    {
		    case DATA_T_INTEGER:
			htrAddScriptWgtr_va(s, "%STR&SYM:%INT, ", propname, od.Integer);
			break;

		    case DATA_T_DOUBLE:
			htrAddScriptWgtr_va(s, "%STR&SYM:%DBL, ", propname, od.Double);
			break;

		    case DATA_T_STRING:
			htrAddScriptWgtr_va(s, "%STR&SYM:'%STR&JSSTR', ", propname, od.String);
			break;

		    case DATA_T_CODE:
			wgtrGetPropertyValue(tree,propname,DATA_T_CODE,POD(&code));
			xsInit(&exptxt);
			xsInit(&proptxt);
			htrGetExpParams(code, &proptxt);
			expGenerateText(code, NULL, xsWrite, &exptxt, '\0', "javascript", EXPR_F_RUNCLIENT);
			htrAddScriptWgtr_va(s, "%STR&SYM:{val:null, exp:function(_this,_context){return ( %STR );}, props:%STR, revexp:null}, ", propname, exptxt.String, proptxt.String);
			xsDeInit(&proptxt);
			xsDeInit(&exptxt);
			break;

		    default:
			htrAddScriptWgtr_va(s, "%STR&SYM:'Unknown Datatype (%INT) - Add it in ht_render.c:htr_internal_WriteWgtrProperty()', ", propname, t);
			break;
		    }
		}
	    }

    return 0;
    }


/*** htr_internal_BuildClientWgtr - generate the DHTML to represent the widget
 *** tree.
 ***/
int
htr_internal_BuildClientWgtr_r(pHtSession s, pWgtrNode tree, int indent)
    {
    int i;
    int childcnt = xaCount(&tree->Children);
    char* objinit;
    char* ctrinit;
    pHtDMPrivateData inf = wgtrGetDMPrivateData(tree);
    pWgtrNode child;
    int rendercnt;
    char* scope = NULL;
    char* scopename = NULL;
    char* propname;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"HTR","Could not render application: resource exhaustion occurred");
	    return -1;
	    }

	/** Widget name scope **/
	if (wgtrGetPropertyValue(tree,"scope",DATA_T_STRING,POD(&scope)) == 0)
	    {
	    if (strcmp(scope,"application") && strcmp(scope,"local") && strcmp(scope,"session"))
		scope = NULL;
	    }
	wgtrGetPropertyValue(tree,"scope_name",DATA_T_STRING,POD(&scopename));

	/** Deploy the widget **/
	objinit = inf?(inf->ObjectLinkage):NULL;
	ctrinit = inf?(inf->ContainerLinkage):NULL;
	htrAddScriptWgtr_va(s, 
		"        %STR&*LEN{name:'%STR&SYM'%[, obj:%STR%]%[, cobj:%STR%]%[, scope:'%STR&JSSTR'%]%[, sn:'%STR&JSSTR'%], type:'%STR&JSSTR', vis:%STR, ctl:%STR%[, namespace:'%STR&SYM'%]", 
		indent*4, "                                        ",
		tree->Name,
		objinit, objinit,
		ctrinit, ctrinit,
		scope, scope,
		scopename, scopename,
		tree->Type, 
		(tree->Flags & WGTR_F_NONVISUAL)?"false":"true",
		(tree->Flags & WGTR_F_CONTROL)?"true":"false",
		(!tree->Parent || strcmp(tree->Parent->Namespace, tree->Namespace)),
		tree->Namespace);

	/** Parameters **/
	htrAddScriptWgtr_va(s, ", param:{");
	if (!(tree->Flags & WGTR_F_NONVISUAL))
	    {
	    htr_internal_WriteWgtrProperty(s, tree, "x");
	    htr_internal_WriteWgtrProperty(s, tree, "y");
	    htr_internal_WriteWgtrProperty(s, tree, "width");
	    htr_internal_WriteWgtrProperty(s, tree, "height");
	    htr_internal_WriteWgtrProperty(s, tree, "r_x");
	    htr_internal_WriteWgtrProperty(s, tree, "r_y");
	    htr_internal_WriteWgtrProperty(s, tree, "r_width");
	    htr_internal_WriteWgtrProperty(s, tree, "r_height");
	    }
	propname = wgtrFirstPropertyName(tree);
	while(propname)
	    {
	    htr_internal_WriteWgtrProperty(s, tree, propname);
	    propname = wgtrNextPropertyName(tree);
	    }
	htrAddScriptWgtr_va(s, "}");

	/** ... and any subwidgets **/ //TODO: there's a glitch in this section in which a comma is placed after the last element of an array.
	for(rendercnt=i=0;i<childcnt;i++)
	    {
	    child = (pWgtrNode)xaGetItem(&tree->Children, i);
	    if (child->RenderFlags & HT_WGTF_NORENDER)
		continue;
	    rendercnt++;
	    }
	if (rendercnt)
	    {
	    htrAddScriptWgtr_va(s, ", sub:\n        %STR&*LEN    [\n", indent*4, "                                        ");
	    rendercnt = 0;
	    for(i=0;i<childcnt;i++)
		{
		child = (pWgtrNode)xaGetItem(&tree->Children, i);
		if (child->RenderFlags & HT_WGTF_NORENDER)
		    continue;
		rendercnt--;
		if (htr_internal_BuildClientWgtr_r(s, child, indent+1) < 0)
		    return -1;
		if (rendercnt == 0) /* last one - no comma */
		    htrAddScriptWgtr(s, "\n");
		else
		    htrAddScriptWgtr(s, ",\n");
		}
	    htrAddScriptWgtr_va(s, "        %STR&*LEN    ] }", indent*4, "                                        ");
	    }
	else
	    {
	    htrAddScriptWgtr(s, "}");
	    }

    return 0;
    }

int
htrBuildClientWgtr(pHtSession s, pWgtrNode tree)
    {

	htrAddScriptInclude(s, "/sys/js/ht_utils_wgtr.js", 0);
	htrAddScriptWgtr_va(s, "    pre_%STR&SYM =\n", tree->DName);
	htr_internal_BuildClientWgtr_r(s, tree, 0);
	htrAddScriptWgtr(s, ";\n");

    return 0;
    }


/*** htr_internal_InitNamespace() - generate the code to build the namespace
 *** initializations on the client.
 ***/
int
htr_internal_InitNamespace(pHtSession s, pHtNamespace ns)
    {
    pHtNamespace child;

	/** If this namespace is within another, link to that in the tree
	 ** init by setting 'cobj', otherwise leave the parent linkage totally
	 ** empty.
	 **/
	if (!ns->IsSubnamespace)
	    {
	    if (ns->ParentCtr[0] && ns->Parent)
		htrAddScriptWgtr_va(s, "    %STR&SYM = wgtrSetupTree(pre_%STR&SYM, \"%STR&SYM\", {cobj:wgtrGetContainer(wgtrGetNode(\"%STR&SYM\",\"%STR&SYM\"))});\n",
			ns->DName, ns->DName, ns->DName, ns->Parent->DName, ns->ParentCtr);
	    else
		htrAddScriptWgtr_va(s, "    %STR&SYM = wgtrSetupTree(pre_%STR&SYM, \"%STR&SYM\", null);\n", 
			ns->DName, ns->DName, ns->DName);
	    /*htrAddScriptWgtr_va(s, "    pg_namespaces[\"%STR&SYM\"] = %STR&SYM;\n",
		    ns->DName, ns->DName);*/
	    }

	/** Init child namespaces too **/
	for(child = ns->FirstChild; child; child=child->NextSibling)
	    {
	    htr_internal_InitNamespace(s, child);
	    }

    return 0;
    }


int
htr_internal_FreeNamespace(pHtNamespace ns)
    {
    pHtNamespace child, next;

	/** if 'ns' does not have a parent, then it is builtin to the session
	 ** 'page' structure and need not be freed.
	 **/
	child = ns->FirstChild;
	if (ns->Parent) nmFree(ns, sizeof(HtNamespace));

	/** Free up sub namespaces too **/
	while(child)
	    {
	    next = child->NextSibling;
	    htr_internal_FreeNamespace(child);
	    child = next;
	    }

    return 0;
    }


/*** htrWrite - write data to the output stream
 ***/
int
htrWrite(pHtSession s, char* buf, int len)
    {
    if (len < 0)
	len = strlen(buf);
    return s->StreamWrite(s->Stream, buf, len, 0, 0);
    }


/*** htrQPrintf - write formatted data to output stream
 ***/
int
htrQPrintf(pHtSession s, char* fmt, ...)
    {
    pXString xs;
    va_list va;
    int rval;
   
	/** print to xstring first, then output via htrWrite() **/
	xs = xsNew();
	if (!xs)
	    return -1;
	va_start(va, fmt);
	xsQPrintf_va(xs, fmt, va);
	va_end(va);
	rval = htrWrite(s, xsString(xs), -1);
	xsFree(xs);

    return rval;
    }


/*** htrRender - generate an HTML document given the app structure subtree
 *** as an open ObjectSystem object.
 ***/
/** This is where all control for DHTML generation begins. The
    infrastructure of this function: This function traverses through the 

    understanding of returned infrastructure is needed first.
 **/
int
htrRender(void* stream, int (*stream_write)(void*, char*, int, int, int), pObjSession obj_s, pWgtrNode tree, pStruct params, pWgtrClientInfo c_info)
    {
    pHtSession s;
    int i,n,j,k,cnt,cnt2;
    pStrValue tmp;
    char* ptr;
    pStrValue sv;
    char sbuf[HT_SBUF_SIZE*2];
    char ename[40];
    pHtDomEvent e;
    char* agent = NULL;
    char* classname = NULL;
    int rval;
    pXString err_xs;

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
	s->Params = params;
	s->ObjSession = obj_s;
	s->ClientInfo = c_info;
	s->Namespace = &(s->Page.RootNamespace);
	s->Namespace->IsSubnamespace = 0;
	s->Namespace->HasScriptInits = 1;
	strtcpy(s->Namespace->DName, wgtrGetRootDName(tree), sizeof(s->Namespace->DName));
	s->IsDynamic = 1;
	s->Stream = stream;
	s->StreamWrite = stream_write;
	xhInit(&s->UsedDrivers, 257, 0);

	/** Parent container name specified? I (Seth) think that this
	    GET parameter is only used when a component is
	    requested. **/
	if ((ptr = htrParamValue(s, "cx__graft")))
	    s->GraftPoint = nmSysStrdup(ptr);
	else
	    s->GraftPoint = NULL;

	/** Note: 'classes' here refers to server-side "classes" (see
	    centrallix-sysdoc/README). Note: if this feature ever
	    does get implemented, then this code would actually have
	    to get moved somewhere else since by the time htrRender
	    gets called, it's already assumed that the client wants
	    the information in DHTML. **/
	// $$$ MARK: server-side classes $$$ 
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
	if(s->Class) //	find the right capabilities for the class we're using
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
	    if (s->GraftPoint) nmSysFree(s->GraftPoint);
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
	xaInit(&s->Page.EventHandlers,16);
	xaInit(&(s->Page.Wgtr), 64);
	s->Page.HtmlBodyFile = NULL;
	s->Page.HtmlHeaderFile = NULL;
	s->Page.HtmlStylesheetFile = NULL;
	s->DisableBody = 0;

	/** first thing in the startup() function should be calling build_wgtr **/
	htrAddScriptInit_va(s, "    build_wgtr_%STR&SYM();\n",
		s->Namespace->DName);
	/*htrAddScriptInit_va(s, "\n    var nodes = wgtrNodeList(%STR&SYM);\n",*/
	htrAddScriptInit_va(s, "\n    var ns = \"%STR&SYM\";\n",
			       /*"    var rootname = \"%STR&SYM\";\n", */
		s->Namespace->DName /*, s->Namespace->DName */);
	/*htrAddStylesheetItem(s, "\tdiv {position:absolute; visibility:inherit; overflow:hidden; }\n");*/

	/** Render the top-level widget -- the function that's run
	  * underneath will be dependent upon what the widget
	  * is. (eg. if the top widget is "widget/page", then the page
	  * driver (defined somewhere in htdrv_page.c) will run) **/
	rval = htrRenderWidget(s, tree, 10);

	/** Assemble the various objects into a javascript widget tree **/
	htrBuildClientWgtr(s, tree);

	/** Generate the namespace initialization. **/
	htr_internal_InitNamespace(s, s->Namespace);

	htr_internal_FreeNamespace(s->Namespace);

	/** Add wgtr debug window **/
#ifdef WGTR_DBG_WINDOW
	htrAddScriptWgtr_va(s, "    wgtrWalk(%STR&SYM);\n", tree->Name);
	htrAddScriptWgtr(s, "    ifcLoadDef(\"net/centrallix/button.ifc\");\n");
	htrAddStylesheetItem(s, "\t#dbgwnd {position: absolute; top: 400; left: 50;}\n");
	htrAddBodyItem(s,   "<div id=\"dbgwnd\"><form name=\"dbgform\">"
			    "<textarea name=\"dbgtxt\" cols=\"80\" rows=\"10\"></textarea>"
			    "</form></div>\n");
#endif


	/** Could not render? **/
	if (rval < 0)
	    {
	    err_xs = xsNew();
	    if (err_xs)
		{
		mssStringError(err_xs);
		htrQPrintf(s, "<html><head><title>Error</title></head><body bgcolor=\"white\"><h1>An Error occured while attempting to render this document</h1><br><pre>%STR&HTE</pre></body></html>\r\n", xsString(err_xs));
		xsFree(err_xs);
		}
	    }
	
	/** Output the DOCTYPE for browsers supporting HTML 4.0 -- this will make them use HTML 4.0 Strict **/
	/** FIXME: should probably specify the DTD.... **/
	if(s->Capabilities.HTML40 && !s->Capabilities.Dom0IE)
	    htrWrite(s, "<!doctype html>\n\n", -1);

	/** Write the HTML out... **/
	htrQPrintf(s,	"<!--\n"
			"Generated by Centrallix/%STR (http://www.centrallix.org)\n"
			"Centrallix is (c) 1998-2017 by LightSys Technology Services, Inc.\n"
			"and is free software, licensed under the GNU GPL version 2, or\n"
			"at your option any later version.  This software is provided\n"
			"with ABSOLUTELY NO WARRANTY, NOT EVEN THE IMPLIED WARRANTIES\n"
			"OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.\n"
			"-->\n\n"
			, cx__version);

	htrQPrintf(s,	"<html>\n"
			"<head>\n"
			"    <meta name=\"generator\" content=\"Centrallix/%STR\">\n"
			"    <meta name=\"pragma\" content=\"no-cache\">\n"
			"    <meta name=\"referrer\" content=\"same-origin\">\n"
			, cx__version);

	htrWrite(s, "    <style type=\"text/css\">\n", -1);
	/** Write the HTML stylesheet items. **/
	for(i=0;i<s->Page.HtmlStylesheet.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlStylesheet.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }
	htrWrite(s, "    </style>\n", -1);
	/** Write the HTML header items. **/
	for(i=0;i<s->Page.HtmlHeader.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlHeader.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }

	/** Write the script globals **/
	htrWrite(s, "<script language=\"javascript\">\n\n\n", -1);
	for(i=0;i<s->Page.Globals.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Globals.Items[i]);
	    if (sv->Value[0])
		qpfPrintf(NULL, sbuf,HT_SBUF_SIZE,"if (typeof %STR&SYM == 'undefined') var %STR&SYM = %STR;\n", sv->Name, sv->Name, sv->Value);
	    else
		qpfPrintf(NULL, sbuf,HT_SBUF_SIZE,"var %STR&SYM;\n", sv->Name);
	    htrWrite(s, sbuf, -1);
	    }

	/** Write the includes **/
	htrWrite(s, "\n</script>\n\n", -1);

	/** include ht_render.js **/
	htr_internal_GenInclude(s, "/sys/js/ht_render.js");

	/** include browser-specific geometry js **/
	if(s->Capabilities.Dom0IE)
	    {
	    htr_internal_GenInclude(s, "/sys/js/ht_geom_dom0ie.js");
	    }
	else if (s->Capabilities.Dom0NS)
	    {
	    htr_internal_GenInclude(s, "/sys/js/ht_geom_dom0ns.js");
	    }
	else if (s->Capabilities.Dom1HTML)
	    {
	    htr_internal_GenInclude(s, "/sys/js/ht_geom_dom1html.js");
	    }
	else
	    {
	    /** cannot render **/
	    }

	for(i=0;i<s->Page.Includes.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Includes.Items[i]);
	    htr_internal_GenInclude(s, sv->Name);
	    }
	htrWrite(s, "<script language=\"javascript\">\n\n", -1);

	/** Write the script functions **/
	for(i=0;i<s->Page.Functions.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Functions.Items[i]);
	    htrWrite(s, sv->Value, -1);
	    }


	/** Write the event capture lines **/
	htrQPrintf(s,"\nfunction events_%STR()\n    {\n",s->Namespace->DName);
	cnt = xaCount(&s->Page.EventHandlers);
	strcpy(sbuf,"    if(window.Event)\n        htr_captureevents(");
	for(i=0;i<cnt;i++)
	    {
	    e = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    if (i) strcat(sbuf, " | ");
	    strcat(sbuf,"Event.");
	    strcat(sbuf,e->DomEvent);
	    }
	strcat(sbuf,");\n");
	htrWrite(s, sbuf, -1);
	for(i=0;i<cnt;i++)
	    {
	    e = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    n = strlen(e->DomEvent);
	    if (n >= sizeof(ename)) n = sizeof(ename)-1;
	    for(k=0;k<=n;k++) ename[k] = tolower(e->DomEvent[k]);
	    ename[k] = '\0';
	    cnt2 = xaCount(&e->Handlers);
	    for(j=0;j<cnt2;j++)
		{
		htrQPrintf(s, "    htr_addeventhandler(%STR&DQUOT,%STR&DQUOT);\n",
			ename, xaGetItem(&e->Handlers, j));
		}
	    if (!strcmp(ename,"mousemove"))
		htrQPrintf(s, "    htr_addeventlistener(%STR&DQUOT, document, htr_mousemovehandler);\n",
			ename);
	    else
		htrQPrintf(s, "    htr_addeventlistener(%STR&DQUOT, document, htr_eventhandler);\n",
			ename);
	    }

	htrWrite(s,"    }\n",-1);

	/** Write the expression initializations **/
	htrQPrintf(s,"\nfunction expinit_%STR()\n    {\n",s->Namespace->DName);
	for(i=0;i<s->Page.HtmlExpressionInit.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlExpressionInit.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }
	htrWrite(s,"    }\n",-1);

	/** Write the wgtr declaration **/
	htrQPrintf(s, "\nfunction build_wgtr_%STR()\n    {\n",
		s->Namespace->DName);
	for (i=0;i<s->Page.Wgtr.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Wgtr.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }
	htrWrite(s, "    }\n", -1);

	/** Write the initialization lines **/
	htrQPrintf(s,"\nfunction startup_%STR()\n    {\n", s->Namespace->DName);
	htr_internal_writeCxCapabilities(s); //TODO: (by Seth) this really only needs to happen during first-load.

	for(i=0;i<s->Page.Inits.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Inits.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }
	htrQPrintf(s,"    events_%STR();\n", s->Namespace->DName);
	htrQPrintf(s,"    expinit_%STR();\n", s->Namespace->DName);
	htrWrite(s,"    }\n",-1);

	/** Write the cleanup lines **/
	htrWrite(s,"\nfunction cleanup()\n    {\n",-1);
	for(i=0;i<s->Page.Cleanups.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Cleanups.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }
	htrWrite(s,"    }\n",-1);


	/** If the body part is disabled, skip over body section generation **/
	if (s->DisableBody == 0)
	    {
	    /** Write the HTML body params **/
	    htrWrite(s, "\n</script>\n</head>",-1);
	    htrWrite(s, "\n<body", -1);
	    for(i=0;i<s->Page.HtmlBodyParams.nItems;i++)
	        {
	        ptr = (char*)(s->Page.HtmlBodyParams.Items[i]);
	        n = *(int*)ptr;
	        htrWrite(s, ptr+8, n);
	        }
	    /** work around the Netscape 4.x bug regarding page resizing **/
	    if(s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
		{
		htrWrite(s, " onResize=\"location.reload()\"",-1);
		}
	    htrWrite(s, ">\n", -1);
	    }
	else
	    {
	    htrWrite(s, "\n</script>\n",-1);
	    }

	/** Write the HTML body. **/
	for(i=0;i<s->Page.HtmlBody.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlBody.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }

	if (s->DisableBody == 0)
	    {
	    htrWrite(s, "</body>\n</html>\n",-1);
	    }
	else
	    {
	    htrWrite(s, "\n</HTML>\n",-1);
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
	for (i=0;i<s->Page.Wgtr.nItems;i++) nmFree(s->Page.Wgtr.Items[i], 2048);
	xaDeInit(&(s->Page.Wgtr));
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

	cnt = xaCount(&s->Page.EventHandlers);
	for(i=0;i<cnt;i++)
	    {
	    e = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    /** these must all be string constants; no need to free **/
	    xaDeInit(&e->Handlers);
	    nmFree(e, sizeof(HtDomEvent));
	    }
	xaDeInit(&s->Page.EventHandlers);

	nmSysFree(s->Tmpbuf);

	if (s->GraftPoint) nmSysFree(s->GraftPoint);

	htr_internal_FreeDMPrivateData(tree);

	xhDeInit(&s->UsedDrivers);
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
	memset(drv, 0, sizeof(HtDriver));

	/** Init some of the basic array structures **/
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
	xaInit(&(drv->PseudoTypes), 4);

    return drv;
    }


/*** htrAddSupport - adds support for a server-side class to a driver
 ***   (by telling the class) (see centrallix/htmlgen/README) note:
 ***   _must_ be called after the driver registers
 ***/
int
htrAddSupport(pHtDriver drv, char* className)
    {
    int i;

	pHtClass class = (pHtClass)xhLookup(&(HTR.Classes),className);
	if(!class)
	    {
	    mssError(1,"HTR","unable to find class '%s' for widget driver '%s'",className,drv->WidgetName);
	    return -1;
	    }
	/** Add main type name to hash **/
	if (xhAdd(&(class->WidgetDrivers),drv->WidgetName, (void*)drv) < 0) return -1;

	/** Add pseudo-type names to hash **/
	for (i=0;i<xaCount(&(drv->PseudoTypes));i++)
	    {
	    if (xhAdd(&(class->WidgetDrivers),xaGetItem(&(drv->PseudoTypes), i), (void*)drv) < 0) return -1;
	    }

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

	/** Add some entries to our hash, for fast driver look-up **/

    return 0;
    }

/*** htrLookupDriver - returns the proper driver for a given type.
 ***/
pHtDriver
htrLookupDriver(pHtSession s, char* type_name)
    {
    pXHashTable widget_drivers = NULL;

	if (!s->Class)
	    {
	    mssError(1, "HTR", "Class not defined in HtSession!");
	    return NULL;
	    }
	widget_drivers = &(s->Class->WidgetDrivers);
	if (!widget_drivers)
	    {
	    mssError(1, "HTR", "No widgets defined for useragent/class combo");
	    return NULL;
	    }
	return (pHtDriver)xhLookup(widget_drivers, type_name+7);
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
	wgtrAddDeploymentMethod("DHTML", htrRender);

    return 0;
    }


/*** htrGetBackground - gets the background image or color from the config
 *** and converts it into a string we can use in the DHTML
 ***
 *** obj - an open Object for the config data
 *** prefix - the prefix to add to 'bgcolor' and 'background' for the attrs
 *** as_style - set to 1 to build the string as a style rather than as HTML
 *** buf - the buffer to print into
 *** buflen - the buffer length we can use
 ***
 *** returns with buf set to "" if any error occurs.
 ***/
int
htrGetBackground(pWgtrNode tree, char* prefix, int as_style, char* buf, int buflen)
    {
    char bgcolor_name[64];
    char background_name[128];
    char* bgcolor = "bgcolor";
    char* background = "background";
    char* ptr;

	/** init buf **/
	if (buflen < 1) return -1;
	buf[0] = '\0';

	/** Prefix supplied? **/
	if (prefix && *prefix)
	    {
	    qpfPrintf(NULL, bgcolor_name,sizeof(bgcolor_name),"%STR&SYM_bgcolor",prefix);
	    qpfPrintf(NULL, background_name,sizeof(background_name),"%STR&SYM_background",prefix);
	    bgcolor = bgcolor_name;
	    background = background_name;
	    }

	/** Image? **/
	if (wgtrGetPropertyValue(tree, background, DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (strpbrk(ptr,"\"'\n\r\t")) return -1;
	    if (as_style)
		qpfPrintf(NULL, buf,buflen,"background-image: URL('%STR&CSSURL');",ptr);
	    else
		qpfPrintf(NULL, buf,buflen,"background='%STR&HTE'",ptr);
	    }
	else if (wgtrGetPropertyValue(tree, bgcolor, DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    /** Background color **/
	    if (strpbrk(ptr,"\"'\n\r\t;}<>&")) return -1;
	    if (as_style)
		qpfPrintf(NULL, buf,buflen,"background-color: %STR&CSSVAL;",ptr);
	    else
		qpfPrintf(NULL, buf,buflen,"bgColor='%STR&HTE'",ptr);
	    }
	else
	    {
	    return -1;
	    }

    return 0;
    }


/*** htrGetBoolean() - a convenience routine to retrieve a boolean value
 *** from the structure file.  Boolean values can be specified as yes/no,
 *** true/false, on/off, y/n, or 1/0.  This routine checks for those.
 *** Return value = 1 if true, 0 if false, -1 if error, and default_value
 *** if not found.  default_value can be set to -1 to indicate an error
 *** on a nonexistent attribute.
 ***/
int
htrGetBoolean(pWgtrNode wgt, char* attrname, int default_value)
    {
    int t;
    int rval = default_value;
    char* ptr;
    int n;

	/** type of attr (need to check number if 1/0) **/
	t = wgtrGetPropertyType(wgt,attrname);
	if (t < 0) return default_value;

	/** integer? **/
	if (t == DATA_T_INTEGER)
	    {
	    if (wgtrGetPropertyValue(wgt,attrname,t,POD(&n)) == 0)
		{
		rval = (n != 0);
		}
	    }
	else if (t == DATA_T_STRING)
	    {
	    /** string? **/
	    if (wgtrGetPropertyValue(wgt,attrname,t,POD(&ptr)) == 0)
		{
		if (!strcasecmp(ptr,"yes") || !strcasecmp(ptr,"true") || !strcasecmp(ptr,"on") || !strcasecmp(ptr,"y"))
		    {
		    rval = 1;
		    }
		else if (!strcasecmp(ptr,"no") || !strcasecmp(ptr,"false") || !strcasecmp(ptr,"off") || !strcasecmp(ptr,"n"))
		    {
		    rval = 0;
		    }
		}
	    }
	else
	    {
	    mssError(1,"HT","Invalid data type for attribute '%s'", attrname);
	    rval = -1;
	    }

    return rval;
    }


/*** htrParamValue() - for use by widget drivers; get the value of a param
 *** passed in to the application or component.
 ***/
char*
htrParamValue(pHtSession s, char* paramname)
    {
    pStruct attr;

	/** Make sure this isn't a reserved param **/
	if (!strncmp(paramname,"ls__",4)) return NULL;

	/** Look for it. **/
	attr = stLookup_ne(s->Params, paramname);
	if (!attr) return NULL;

    return attr->StrVal;
    }


/*** htr_internal_CheckDMPrivateData() - check to see if widget private info
 *** structure is allocated, and put it in there if it is not.
 ***/
pHtDMPrivateData
htr_internal_CheckDMPrivateData(pWgtrNode widget)
    {
    pHtDMPrivateData inf = wgtrGetDMPrivateData(widget);
    
	if (!inf)
	    {
	    inf = (pHtDMPrivateData)nmMalloc(sizeof(HtDMPrivateData));
	    memset(inf, 0, sizeof(HtDMPrivateData));
	    wgtrSetDMPrivateData(widget, inf);
	    }

    return inf;
    }


/*** htr_internal_FreeDMPrivateData() - walk through the widget tree, and
 *** free up our DMPrivateData structures.
 ***/
int
htr_internal_FreeDMPrivateData(pWgtrNode widget)
    {
    pHtDMPrivateData inf = wgtrGetDMPrivateData(widget);
    int i, cnt;

	cnt = xaCount(&widget->Children);
	for(i=0;i<cnt;i++)
	    htr_internal_FreeDMPrivateData((pWgtrNode)xaGetItem(&widget->Children, i));

	if (inf)
	    {
	    if (inf->ObjectLinkage) nmSysFree(inf->ObjectLinkage);
	    if (inf->ContainerLinkage) nmSysFree(inf->ContainerLinkage);
	    if (inf->Param) nmSysFree(inf->Param);
	    nmFree(inf, sizeof(HtDMPrivateData));
	    wgtrSetDMPrivateData(widget, NULL);
	    }

    return 0;
    }


/*** htrAddWgtrObjLinkage() - specify what function/object to call to find out
 *** what the actual client-side object is that represents an object inside
 *** the widget tree.
 ***/
int
htrAddWgtrObjLinkage(pHtSession s, pWgtrNode widget, char* linkage)
    {
    pHtDMPrivateData inf = htr_internal_CheckDMPrivateData(widget);
    
	inf->ObjectLinkage = nmSysStrdup(objDataToStringTmp(DATA_T_STRING, linkage, DATA_F_QUOTED));

    return 0;
    }


/*** htrAddWgtrObjLinkage_va() - varargs version of the above.
 ***/
int
htrAddWgtrObjLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...)
    {
    va_list va;
    char buf[256];

	va_start(va, fmt);
	qpfPrintf_va(NULL, buf, sizeof(buf), fmt, va);
	va_end(va);

    return htrAddWgtrObjLinkage(s, widget, buf);
    }


/*** htrAddWgtrCtrLinkage() - specify what function/object to call to find out
 *** what the actual client-side object is that represents an object inside
 *** the widget tree.
 ***/
int
htrAddWgtrCtrLinkage(pHtSession s, pWgtrNode widget, char* linkage)
    {
    pHtDMPrivateData inf = htr_internal_CheckDMPrivateData(widget);
    
	inf->ContainerLinkage = nmSysStrdup(objDataToStringTmp(DATA_T_STRING, linkage, DATA_F_QUOTED));

    return 0;
    }


/*** htrAddWgtrCtrLinkage_va() - varargs version of the above.
 ***/
int
htrAddWgtrCtrLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...)
    {
    va_list va;
    char buf[256];

	va_start(va, fmt);
	qpfPrintf_va(NULL, buf, sizeof(buf), fmt, va);
	va_end(va);

    return htrAddWgtrCtrLinkage(s, widget, buf);
    }


/*** htrAddWgtrInit() - sets the initialization function for the widget
 ***/
int
htrAddWgtrInit(pHtSession s, pWgtrNode widget, char* func, char* paramfmt, ...)
    {
    va_list va;
    char buf[256];
    pHtDMPrivateData inf = htr_internal_CheckDMPrivateData(widget);
    
	inf->InitFunc = func;
	va_start(va, paramfmt);
	vsnprintf(buf, sizeof(buf), paramfmt, va);
	va_end(va);
	inf->Param = nmSysStrdup(objDataToStringTmp(DATA_T_STRING, buf, DATA_F_QUOTED));

    return 0;
    }


/*** htrAddNamespace() - adds a namespace context for expression evaluation
 *** et al.
 ***/
int
htrAddNamespace(pHtSession s, pWgtrNode container, char* nspace, int is_subns)
    {
    pHtNamespace new_ns;
    char* ptr;

	/** Allocate a new namespace **/
	new_ns = (pHtNamespace)nmMalloc(sizeof(HtNamespace));
	if (!new_ns) return -1;
	new_ns->Parent = s->Namespace;
	strtcpy(new_ns->DName, nspace, sizeof(new_ns->DName));

	new_ns->IsSubnamespace = is_subns;
	if (is_subns)
	    ptr = s->Namespace->ParentCtr;
	else
	    wgtrGetPropertyValue(container, "name", DATA_T_STRING, POD(&ptr));
	strtcpy(new_ns->ParentCtr, ptr, sizeof(new_ns->ParentCtr));

	/** Link it in **/
	new_ns->FirstChild = NULL;
	new_ns->NextSibling = s->Namespace->FirstChild;
	s->Namespace->FirstChild = new_ns;
	s->Namespace = new_ns;
	s->Namespace->HasScriptInits = 0;

	/** Add script inits **/
#if 00
	htrAddScriptInit_va(s, "\n    nodes = wgtrNodeList(%STR&SYM);\n"
			       /*"    rootname = \"%STR&SYM\";\n" */ ,
		nspace /*, nspace */);
#endif

    return 0;
    }


/*** htrLeaveNamespace() - revert back to the parent namespace of the one
 *** currently in use.
 ***/
int
htrLeaveNamespace(pHtSession s)
    {

	s->Namespace = s->Namespace->Parent;
	s->Namespace->HasScriptInits = 0;

	/** Add script inits **/
#if 00
	htrAddScriptInit_va(s, "\n    nodes = wgtrNodeList(%STR&SYM);\n"
			       /*"    rootname = \"%STR&SYM\";\n" */ ,
		s->Namespace->DName /*, s->Namespace->DName */);
#endif

    return 0;
    }


/*** htrFormatElement() - add a CSS line to format a DIV based on some basic
 *** style information.
 ***
 *** Styled properties: textcolor, style, font_size, font, bgcolor, padding,
 ***     border_color, border_radius, border_style, align, wrap, shadow_color,
 ***     shadow_radius, shadow_offset, shadow_location, shadow_angle
 ***/
int
htrFormatElement(pHtSession s, pWgtrNode node, char* id, int flags, int x, int y, int w, int h, int z, char* style_prefix, char* defaults[], char* addl)
    {
    char textcolor[32] = "";
    char style[32] = "";
    char font[32] = "";
    double font_size = 0;
    char bgcolor[32] = "";
    char background[128] = "";
    double padding = 0;
    char border_color[32] = "";
    double border_radius = 0;
    char border_style[32] = "";
    char align[32] = "";
    int wrap = 0;
    char shadow_color[32] = "";
    double shadow_radius = 0;
    double shadow_offset = 0;
    char shadow_location[32] = "";
    double shadow_angle = 135;
    char propname[64];
    char* propptr;
    char* strval;
    int i;

	/** Copy in defaults **/
	if (defaults)
	    {
	    for(i=0;defaults[i] && defaults[i+1];i+=2)
		{
		if (!strcmp(defaults[i], "textcolor"))
		    strtcpy(textcolor, defaults[i+1], sizeof(textcolor));
		else if (!strcmp(defaults[i], "style"))
		    strtcpy(style, defaults[i+1], sizeof(style));
		else if (!strcmp(defaults[i], "font"))
		    strtcpy(font, defaults[i+1], sizeof(font));
		else if (!strcmp(defaults[i], "font_size"))
		    font_size = strtod(defaults[i+1], NULL);
		else if (!strcmp(defaults[i], "bgcolor"))
		    strtcpy(bgcolor, defaults[i+1], sizeof(bgcolor));
		else if (!strcmp(defaults[i], "background"))
		    strtcpy(background, defaults[i+1], sizeof(background));
		else if (!strcmp(defaults[i], "padding"))
		    padding = strtod(defaults[i+1], NULL);
		else if (!strcmp(defaults[i], "border_color"))
		    strtcpy(border_color, defaults[i+1], sizeof(border_color));
		else if (!strcmp(defaults[i], "border_radius"))
		    border_radius = strtod(defaults[i+1], NULL);
		else if (!strcmp(defaults[i], "border_style"))
		    strtcpy(border_style, defaults[i+1], sizeof(border_style));
		else if (!strcmp(defaults[i], "align"))
		    strtcpy(align, defaults[i+1], sizeof(align));
		else if (!strcmp(defaults[i], "wrap") && !strcasecmp(defaults[i+1], "yes"))
		    wrap = 1;
		else if (!strcmp(defaults[i], "shadow_color"))
		    strtcpy(shadow_color, defaults[i+1], sizeof(shadow_color));
		else if (!strcmp(defaults[i], "shadow_radius"))
		    shadow_radius = strtod(defaults[i+1], NULL);
		else if (!strcmp(defaults[i], "shadow_offset"))
		    shadow_offset = strtod(defaults[i+1], NULL);
		else if (!strcmp(defaults[i], "shadow_location"))
		    strtcpy(shadow_location, defaults[i+1], sizeof(shadow_location));
		else if (!strcmp(defaults[i], "shadow_angle"))
		    shadow_angle = strtod(defaults[i+1], NULL);
		}
	    }

	/** Set up property name prefixes **/
	strtcpy(propname, style_prefix?style_prefix:"", sizeof(propname));
	strtcat(propname, style_prefix?"_":"", sizeof(propname));
	if (strlen(propname) >= 48)
	    {
	    mssError(1,"HTR","htrFormatDiv() - style prefix too long");
	    return -1;
	    }
	propptr = propname + strlen(propname);

	/** Get text styling information **/
	strcpy(propptr, "textcolor");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(textcolor, strval, sizeof(textcolor));
	strcpy(propptr, "fgcolor"); /* for legacy support -- deprecated */
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(textcolor, strval, sizeof(textcolor));
	strcpy(propptr, "style");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(style, strval, sizeof(style));
	strcpy(propptr, "font");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(font, strval, sizeof(font));
	strcpy(propptr, "font_size");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    font_size = strtod(strval, NULL);
	strcpy(propptr, "bgcolor");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(bgcolor, strval, sizeof(bgcolor));
	strcpy(propptr, "background");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(background, strval, sizeof(background));
	strcpy(propptr, "padding");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    padding = strtod(strval, NULL);
	strcpy(propptr, "border_color");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(border_color, strval, sizeof(border_color));
	strcpy(propptr, "border_radius");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    border_radius = strtod(strval, NULL);
	strcpy(propptr, "border_style");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(border_style, strval, sizeof(border_style));
	strcpy(propptr, "align");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(align, strval, sizeof(align));
	strcpy(propptr, "wrap");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    {
	    if (!strcasecmp(strval, "yes")) wrap = 1;
	    }
	strcpy(propptr, "shadow_color");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(shadow_color, strval, sizeof(shadow_color));
	strcpy(propptr, "shadow_radius");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    shadow_radius = strtod(strval, NULL);
	strcpy(propptr, "shadow_offset");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    shadow_offset = strtod(strval, NULL);
	strcpy(propptr, "shadow_location");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    strtcpy(shadow_location, strval, sizeof(shadow_location));
	strcpy(propptr, "shadow_angle");
	if (wgtrGetPropertyValue(node, propname, DATA_T_STRING, POD(&strval)) == 0)
	    shadow_angle = strtod(strval, NULL);

	/** Generate the style CSS **/
	htrAddStylesheetItem_va(s, "\t%STR { left:%POSpx; top:%POSpx; %[width:%POSpx; %]%[height:%POSpx; %]%[z-index:%POS; %]%[color:%STR&CSSVAL; %]%[font-weight:bold; %]%[text-decoration:underline; %]%[font-style:italic; %]%[font:%STR&CSSVAL; %]%[font-size:%DBLpx; %]%[background-color:%STR&CSSVAL; %]%[background-image:url('%STR&CSSURL'); %]%[padding:%DBLpx; %]%[border:1px %STR&CSSVAL %STR&CSSVAL; %]%[border-radius:%DBLpx; %]%[text-align:%STR&CSSVAL; %]%[white-space:nowrap; %]%[box-shadow:%DBLpx %DBLpx %DBLpx %STR&CSSVAL%STR&CSSVAL; %]%[%STR %]}\n",
		id,
		x, y, w > 0, w, h > 0, h, z > 0, z,
		*textcolor, textcolor,
		!strcmp(style, "bold"), !strcmp(style, "underline"), !strcmp(style, "italic"),
		*font, font, font_size > 0, font_size,
		*bgcolor, bgcolor, *background, background,
		padding > 0, padding,
		*border_color, (*border_style)?border_style:"solid", border_color, border_radius > 0, border_radius,
		*align, align,
		!wrap,
		(*shadow_color && shadow_radius > 0), sin(shadow_angle*M_PI/180)*shadow_offset, cos(shadow_angle*M_PI/180)*(-shadow_offset), shadow_radius, shadow_color, (!strcasecmp(shadow_location,"inside"))?" inset":"",
		addl && *addl, addl
		);

    return 0;
    }


