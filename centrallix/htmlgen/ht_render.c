#include <stdbool.h>
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
#include "cxlib/expect.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/util.h"
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
    htrWrite(s,"\tcx__capabilities = {};\n",24);
#define PROCESS_CAP_OUT(attr) \
    htrWrite(s,"\tcx__capabilities.",18); \
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
	    ptr = (char*)check_ptr(nmMalloc(2048));
	    if (ptr == NULL) goto err;
	    *(int*)ptr = 0;
	    l = 0;
	    if (check_neg(xaAddItem(arr, ptr)) < 0) goto err;
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
		ptr = (char*)check_ptr(nmMalloc(2048));
		if (ptr == NULL) goto err;
		*(int*)ptr = 0;
		l = 0;
		if (check_neg(xaAddItem(arr, ptr)) < 0) goto err;
		}
	    }

	return 0;

    err:
	mssError(1, "HTR",
	    "Failed to add text to array with %d items: \"%s\"",
	    xaCount(arr), txt
	);
	return -1;
    }


/*** htrRenderWidget - generate a widget into the HtPage structure, given the
 *** widget's objectsystem descriptor...
 ***/
int
htrRenderWidget(pHtSession session, pWgtrNode widget, int z)
    {
    pHtDriver drv;
    pXHashTable widget_drivers = NULL;
    int rval = -1;
    
	/** Guard segfault. **/
	if (UNLIKELY(widget == NULL))
	    {
	    mssError(1, "HTR", "Failed to render NULL widget.");
	    return -1;
	    }

	/** Check recursion **/
	if (UNLIKELY(thExcessiveRecursion()))
	    {
	    mssError(1,"HTR","Could not render application: resource exhaustion occurred");
	    goto end;
	    }

	/** Find the hashtable keyed with widget names for this combination of
	 ** user-agent:style that contains pointers to the drivers to use.
	 **/
	if (UNLIKELY(session->Class == NULL))
	    {
	    printf("Class not defined %s:%i\n",__FILE__,__LINE__);
	    goto end;
	    }
	widget_drivers = &session->Class->WidgetDrivers;

	/** Get the name of the widget.. **/
	if (UNLIKELY(strncmp(widget->Type, "widget/", 7) != 0))
	    {
	    mssError(1,"HTR","Invalid content type for widget - must be widget/xxx");
	    goto end;
	    }

	/** Lookup the driver. These are what are defined in the various htdrv_* functions **/
	drv = (pHtDriver)xhLookup(widget_drivers,widget->Type+7);
	if (UNLIKELY(drv == NULL))
	    {
	    mssError(1,"HTR","Unknown widget object type '%s'", widget->Type);
	    goto end;
	    }

	/** Has this driver been used this session yet? **/
	if (xhLookup(&session->UsedDrivers, drv->WidgetName) == NULL)
	    {
	    if (check_neg(xhAdd(&session->UsedDrivers, drv->WidgetName, (void*)drv)) != 0) goto end;
	    if (drv->Setup != NULL && check_neg(drv->Setup(session)) < 0) goto end;
	    }

	/** Crossing a namespace boundary? **/
	if (UNLIKELY(htrCheckNSTransition(session, widget->Parent, widget) != 0)) goto end;

	/** will be a *Render function found in a htmlgen/htdrv_*.c file (eg htpageRender) **/
	rval = drv->Render(session, widget, z);
	if (UNLIKELY(rval != 0))
	    {
	    char error_code[32] = {'\0'};
	    if (rval != -1) snprintf(error_code, sizeof(error_code), " (error code: %d)", rval);
	    mssError(0, "HTR", "\"%s\" render failed%s.", drv->Name, error_code);
	    }

	/** Going back to previous namespace? (Even if rendering failed.) **/
	if (UNLIKELY(htrCheckNSTransitionReturn(session, widget->Parent, widget) != 0))
	    {
	    rval = -1;
	    goto end;
	    }

    end:
	if (UNLIKELY(rval != 0))
	    {
	    mssError(0, "HTR",
		"Failed to render \"%s\":\"%s\".",
		widget->Name, widget->Type
	    );
	    }
	
	return rval;
    }


/*** htrCheckNSTransition -- check to see if we are transitioning between
 *** namespaces, and if so, emit the code to switch the context.
 ***/
int
htrCheckNSTransition(pHtSession s, pWgtrNode parent, pWgtrNode child)
    {
	/** Crossing a namespace boundary? **/
	if (child != NULL && parent != NULL &&
	    strcmp(child->Namespace, parent->Namespace) != 0)
	    {
	    if (UNLIKELY(htrAddNamespace(s, NULL, child->Namespace, 1) != 0))
		{
		mssError(0, "HTR",
		    "Transition failed: \"%s\":\"%s\" (parent) --> \"%s\":\"%s\" (child).",
		    parent->Name, parent->Type, child->Name, child->Type
		);
		return -1;
		}
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
	if (child != NULL && parent != NULL &&
	    strcmp(child->Namespace, parent->Namespace) != 0)
	    {
	    if (UNLIKELY(htrLeaveNamespace(s) != 0))
		{
		mssError(0, "HTR",
		    "Transition return failed: \"%s\":\"%s\" (parent) <-- \"%s\":\"%s\" (child).",
		    parent->Name, parent->Type, child->Name, child->Type
		);
		return -1;
		}
	    }

    return 0;
    }


/*** htrAddStylesheetItem -- copies stylesheet definitions into the
 *** buffers that will eventually be output as HTML.
 ***/
int
htrAddStylesheetItem(pHtSession s, char* html_text)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.HtmlStylesheet), html_text) != 0))
	    {
	    mssError(0, "HTR", "Failed to add style sheet item.");
	    return -1;
	    }

    return 0;
    }


/*** htrAddHeaderItem -- copies html text into the buffers that will
 *** eventually be output as the HTML header.
 ***/
int
htrAddHeaderItem(pHtSession s, char* html_text)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.HtmlHeader), html_text) != 0))
	    {
	    mssError(0, "HTR", "Failed to add header item.");
	    return -1;
	    }

    return 0;
    }


/*** htrAddBodyItem -- copies html text into the buffers that will
 *** eventually be output as the HTML body.
 ***/
int
htrAddBodyItem(pHtSession s, char* html_text)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.HtmlBody), html_text) != 0))
	    {
	    mssError(0, "HTR", "Failed to add body item.");
	    return -1;
	    }

    return 0;
    }


/*** htrAddExpressionItem -- copies html text into the buffers that will
 *** eventually be output as the HTML body.
 ***/
int
htrAddExpressionItem(pHtSession s, char* html_text)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.HtmlExpressionInit), html_text) != 0))
	    {
	    mssError(0, "HTR", "Failed to add expression item.");
	    return -1;
	    }

    return 0;
    }


/*** htrAddBodyParam -- copies html text into the buffers that will
 *** eventually be output as the HTML body.  These are simple html tag
 *** parameters (i.e., "BGCOLOR=white")
 ***/
int
htrAddBodyParam(pHtSession s, char* html_param)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.HtmlBodyParams), html_param) != 0))
	    {
	    mssError(0, "HTR", "Failed to add body parameter.");
	    return -1;
	    }

    return 0;
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

	if (UNLIKELY(*size >= req_size)) return 1;

	assert(*str == s->Tmpbuf);
	assert(*size == s->TmpbufSize);
	new_buf_size = s->TmpbufSize * 2;
	while(new_buf_size < req_size) new_buf_size *= 2;
	new_buf = check_ptr(nmSysRealloc(s->Tmpbuf, new_buf_size));
	if (new_buf == NULL) return false; /* Grow failed. */
	*str = s->Tmpbuf = new_buf;
	*size = s->TmpbufSize = new_buf_size;

    return true; /* Success. */
    }


/*** htr_internal_QPAddText() - same as below function, but uses qprintf
 *** instead of snprintf.
 ***/
int
htr_internal_QPAddText(pHtSession s, int (*fn)(), char* fmt, va_list va)
    {
    int rval = -1, result;
    pQPSession error_session = NULL;

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

	/** Create a pQPSession to track errors from qpfPrintf. **/
	error_session = check_ptr(qpfOpenSession());
	if (error_session == NULL) goto end;

	/** Print text using qpfPrintf(). **/
	result = qpfPrintf_va_internal(error_session, &(s->Tmpbuf), &(s->TmpbufSize), htr_internal_GrowFn, (void*)s, fmt, va);
	if (UNLIKELY(result < 0))
	    {
	    mssError(1, "HTR", "QPAddText() failed to format: \"%s\"", fmt);
	    qpfLogErrors(error_session);
	    goto end;
	    }
	if (UNLIKELY(s->TmpbufSize - 1 < result))
	    {
	    mssError(1, "HTR", "qpfPrintf() failed to write all the required text.");
	    qpfLogErrors(error_session);
	    goto end;
	    }

	/** Add the text using the callback function. **/
	result = fn(s, s->Tmpbuf);
	if (UNLIKELY(result != 0))
	    {
	    char error_code[32] = {'\0'};
	    if (UNLIKELY(result != -1)) snprintf(error_code, sizeof(error_code), " (error code: %d)", result);
	    mssError(0, "HTR", "Provided callback function failed%s.", error_code);
	    goto end;
	    }
	
	/** Success. **/
	rval = 0;
	
    end:
	/** Clean up. **/
	if (LIKELY(error_session != NULL)) qpfCloseSession(error_session);

	return rval;
    }


/*** htr_internal_AddText() - use vararg mechanism to add text using one of
 *** the standard add routines.
 ***/
int
htr_internal_AddText(pHtSession s, int (*fn)(), char* fmt, va_list va)
    {
    int result;

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

	/** Save a copy of the va_list so we can retry. **/
	va_list orig_va;
	va_copy(orig_va, va);

    retry:
	/** Attempt to print the thing to the tmpbuf. **/
	result = vsnprintf(s->Tmpbuf, s->TmpbufSize, fmt, va);
	if (UNLIKELY(result < 0 || result > s->TmpbufSize - 1))
	    {
	    /** Increase buffer size. **/
	    size_t new_buf_size = s->TmpbufSize * 2lu;
	    while (new_buf_size < result) new_buf_size *= 2lu;
	    
	    /** Allocate the new buffer. **/
	    void* new_buf = check_ptr(nmSysMalloc(new_buf_size));
	    if (s->Tmpbuf == NULL) return -1;
	    nmSysFree(s->Tmpbuf); /* Clean up. */
	    
	    /** Set new buffer. **/
	    s->TmpbufSize = new_buf_size;
	    s->Tmpbuf = new_buf;
	    
	    /** Try the write again. **/
	    va = orig_va;
	    goto retry;
	    }

	/** Add the text using the callback function. **/
	result = fn(s, s->Tmpbuf);
	if (UNLIKELY(result != 0))
	    {
	    char error_code[32] = {'\0'};
	    if (UNLIKELY(result != -1)) snprintf(error_code, sizeof(error_code), " (error code: %d)", result);
	    mssError(0, "HTR", "Provided callback function failed%s.", error_code);
	    return -1;
	    }
	
	/** Success. **/
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
	const int ret = htr_internal_QPAddText(s, htrAddBodyItem, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add HTML body item.");

    return ret;
    }


/*** htrAddExpressionItem_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the body of the document.
 ***/
int
htrAddExpressionItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	const int ret = htr_internal_QPAddText(s, htrAddExpressionItem, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add expression item.");

    return ret;
    }


/*** htrAddStylesheetItem_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the stylesheet definition of the document.
 ***/
int
htrAddStylesheetItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	const int ret = htr_internal_QPAddText(s, htrAddStylesheetItem, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add CSS stylesheet item.");

    return ret;
    }


/*** htrAddHeaderItem_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the header of the document.
 ***/
int
htrAddHeaderItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	const int ret = htr_internal_QPAddText(s, htrAddHeaderItem, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add HTML document header item.");

    return ret;
    }


/*** htrAddBodyParam_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the body tag of the document.
 ***/
int
htrAddBodyParam_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	const int ret = htr_internal_QPAddText(s, htrAddBodyParam, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add body parameter.");

    return ret;
    }


/*** htrAddScriptWgtr_va() - use a vararg list to add a formatted string
 *** to build_wgtr_* function of the document
 ***/
int
htrAddScriptWgtr_va(pHtSession s, char* fmt, ...)
    {
    va_list va;

	va_start(va, fmt);
	const int ret = htr_internal_QPAddText(s, htrAddScriptWgtr, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add JS code.");

    return ret;
    }



/*** htrAddScriptInit_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to startup function of the document.
 ***/
int
htrAddScriptInit_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	/*** TODO: Greg - Can we remove this code? It seems like this is the
	 *** concern of htrRender(), and if it's not, should we remove the
	 *** line from htrRender() that does this?
	 ***/
	if (!s->Namespace->HasScriptInits)
	    {
	    /** No script inits for this namespace yet?  Issue the context
	     ** switch if no inits yet.
	     **/
	    s->Namespace->HasScriptInits = 1;
	    /*htrAddScriptInit_va(s, "\n    nodes = wgtrNodeList(%STR&SYM);\n", s->Namespace->DName);*/
	    htrAddScriptInit_va(s, "\tns = \"%STR&SYM\";\n", s->Namespace->DName);
	    }

	va_start(va, fmt);
	const int ret = htr_internal_QPAddText(s, htrAddScriptInit, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add JS init call.");

    return ret;
    }


/*** htrAddScriptCleanup_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to cleanup function of the document.
 ***/
int
htrAddScriptCleanup_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	const int ret = htr_internal_QPAddText(s, htrAddScriptCleanup, fmt, va);
	va_end(va);
	if (UNLIKELY(ret != 0)) mssError(0, "HTR", "Failed to add JS clean up code.");

    return ret;
    }



/*** htrAddScriptInclude -- adds a script src= type entry between the html
 *** header and html body.
 ***/
int
htrAddScriptInclude(pHtSession s, char* filename, int flags)
    {
    pStrValue sv;

	/** Ignore this call if the script is already included. **/
	char* scripts_loaded = htrParamValue(s, "cx__scripts");
	if (scripts_loaded != NULL && strstr(scripts_loaded, filename))
	    return 0;
	
	/** Cache check. **/
	if (xhLookup(&(s->Page.NameIncludes), filename) != NULL) return 0;

    	/** Alloc the string val. **/
	sv = (pStrValue)check_ptr(nmMalloc(sizeof(StrValue)));
	if (sv == NULL) goto err;
	sv->Name = filename;
	if (flags & HTR_F_NAMEALLOC) sv->NameSize = strlen(filename)+1;
	sv->Value = "";
	sv->Alloc = (flags & HTR_F_NAMEALLOC);

	/** Add to the hash table and array **/
	if (check(xhAdd(&(s->Page.NameIncludes), filename, (char*)sv)) != 0) goto err;
	if (check_neg(xaAddItem(&(s->Page.Includes), (char*)sv)) < 0) goto err;

	return 0;
	
    err:
	mssError(1, "HTR", "Failed to include script file: %s", filename);
	return -1;
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
	sv = (pStrValue)check_ptr(nmMalloc(sizeof(StrValue)));
	if (sv == NULL) goto err;
	sv->Name = fn_name;
	if (flags & HTR_F_NAMEALLOC) sv->NameSize = strlen(fn_name)+1;
	sv->Value = fn_text;
	if (flags & HTR_F_VALUEALLOC) sv->ValueSize = strlen(fn_text)+1;
	sv->Alloc = flags;

	/** Add to the hash table and array **/
	if (check(xhAdd(&(s->Page.NameFunctions), fn_name, (char*)sv)) != 0) goto err;
	if (check_neg(xaAddItem(&(s->Page.Functions), (char*)sv)) < 0) goto err;

	return 0;

    err:
	mssError(1, "HTR", "Failed to add JS function: %s()", fn_name);
	return -1;
    }


/*** htrAddScriptGlobal -- adds a global variable to the list of variables
 *** to be output in the HTML JavaScript section.  Duplicates are suppressed.
 ***/
int
htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags)
    {
    pStrValue sv;

	/** Cache check. **/
	if (xhLookup(&(s->Page.NameGlobals), var_name) != NULL) return 0;
	
	/** Alloc a new string val. **/
	sv = (pStrValue)check_ptr(nmMalloc(sizeof(StrValue)));
	if (sv == NULL) goto err;
	sv->Name = var_name;
	sv->NameSize = strlen(var_name)+1;
	sv->Value = initialization;
	sv->ValueSize = strlen(initialization)+1;
	sv->Alloc = flags;

	/** Add to the hash table and array **/
	if (check(xhAdd(&(s->Page.NameGlobals), var_name, (char*)sv)) != 0) goto err;
	if (check_neg(xaAddItem(&(s->Page.Globals), (char*)sv)) < 0) goto err;

	/** Success. **/
	return 0;
	
    err:;
	char flags_buf[20] = {'\0'};
	if (flags != 0) snprintf(flags_buf, sizeof(flags_buf), " (flags %d)", flags);
	mssError(1, "HTR",
	    "Failed to add global variable %s = %s%s.",
	    var_name, initialization, flags_buf
	);
	return -1;
    }


/*** htrAddScriptInit -- adds some initialization text that runs outside of a
 *** function context in the HTML JavaScript.
 ***/
int
htrAddScriptInit(pHtSession s, char* init_text)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.Inits), init_text) != 0))
	    {
	    mssError(0, "HTR", "Failed to add script init.");
	    return -1;
	    }

    return 0;
    }


/*** htrAddScriptWgtr - adds some text to the wgtr function
 ***/
int
htrAddScriptWgtr(pHtSession s, char* wgtr_text)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.Wgtr), wgtr_text) != 0))
	    {
	    mssError(0, "HTR", "Failed to add to script in widget tree.");
	    return -1;
	    }

    return 0;
    }


/*** htrAddScriptCleanup -- adds some initialization text that runs outside of a
 *** function context in the HTML JavaScript.
 ***/
int
htrAddScriptCleanup(pHtSession s, char* init_text)
    {
	if (UNLIKELY(htr_internal_AddTextToArray(&(s->Page.Cleanups), init_text) != 0))
	    {
	    mssError(0, "HTR", "Failed to add script clean up.");
	    return -1;
	    }

    return 0;
    }


/*** htrAddEventHandlerFunction - adds an event handler script code segment for
 *** a given event on a given object (usually the 'document').
 ***
 *** GRB note -- event_src is no longer used, should always be 'document'.
 ***          -- drvname is no longer used, just ignored.
 ***/
int
htrAddEventHandlerFunction(pHtSession s, char* event_src, char* event, char* drv_name, char* function)
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
	if (e == NULL)
	    {
	    e = (pHtDomEvent)check_ptr(nmMalloc(sizeof(HtDomEvent)));
	    if (e == NULL) return -1;
	    strtcpy(e->DomEvent, event, sizeof(e->DomEvent));
	    if (check(xaInit(&e->Handlers,64)) != 0) goto err;
	    if (check_neg(xaAddItem(&s->Page.EventHandlers, e)) < 0) goto err;
	    }

	/** Add our handler **/
	cnt = xaCount(&e->Handlers);
	for(i=0;i<cnt;i++)
	    {
	    if (!strcmp(function, (char*)xaGetItem(&e->Handlers,i)))
		return 0;
	    }
	if (check_neg(xaAddItem(&e->Handlers, function) < 0)) goto err;

	return 0;
	
	err:
	mssError(1, "HTR",
	    "Failed to add %s event handler function \'%s()\' to %d for driver %s.",
	    event, function, event_src, drv_name
	);
	return -1;
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
    pHtEventAction event = NULL;

	/** Allocate the event struct. **/
	event = (pHtEventAction)check_ptr(nmSysMalloc(sizeof(HtEventAction)));
	if (event == NULL) goto err;
	
	/** Write the event name with a null terminator. **/
	memccpy(event->Name, event_name, 0, sizeof(event->Name) - 1lu);
	event->Name[sizeof(event->Name) - 1lu] = '\0';
	
	/** Register the event. **/
	if (check(xaInit(&(event->Parameters), 16)) != 0) goto err;
	if (check_neg(xaAddItem(&drv->Events, (void*)event)) < 0) goto err;

	/** Success. **/
	return 0;

	err:
	mssError(1, "HTR", "Failed to add event: \"%s\".", event_name);
	
	/** Clean up. **/
	if (LIKELY(event != NULL)) nmSysFree(event);
	
	return -1;
    }


/*** htrAddAction - adds an action to a widget.
 ***/
int
htrAddAction(pHtDriver drv, char* action_name)
    {
    pHtEventAction action = NULL;

	/** Allocate the action struct. **/
	action = (pHtEventAction)check_ptr(nmSysMalloc(sizeof(HtEventAction)));
	if (action == NULL) goto err;
	
	/** Write the action name with a null terminator. **/
	memccpy(action->Name, action_name, 0, sizeof(action->Name) - 1lu);
	action->Name[sizeof(action->Name) - 1lu] = '\0';
	
	/** Register the action. **/
	if (check(xaInit(&(action->Parameters), 16)) != 0) goto err;
	if (check_neg(xaAddItem(&drv->Actions, (void*)action)) < 0) goto err;

	/** Success. **/
	return 0;

	err:
	mssError(1, "HTR", "Failed to add action: \"%s\".", action_name);
	
	/** Clean up. **/
	if (LIKELY(action != NULL)) nmSysFree(action);
	
	return -1;
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
	if (UNLIKELY(ea == NULL)) goto err;

	/** Allocate the parameter struct. **/
	p = check_ptr(nmSysMalloc(sizeof(HtParam)));
	if (p == NULL) goto err;
	
	/** Initialize the parameter struct. **/
	memccpy(p->ParamName, param_name, 0, sizeof(p->ParamName) - 1lu);
	p->ParamName[sizeof(p->ParamName) - 1lu] = '\0';
	p->DataType = datatype;
	
	/** Register the parameter. **/
	if (check_neg(xaAddItem(&(ea->Parameters), (void*)p)) < 0) goto err;

	/** Success. **/
	return 0;

	err:
	mssError(1, "HTR", "Failed to add parameter: \"%s\" of type %d.", param_name, datatype);
	
	/** Clean up. **/
	if (LIKELY(p != NULL)) nmSysFree(p);
	
	return -1;
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
	if (UNLIKELY(htrAddBodyItemLayerStart(s, flags, id, cnt, cls) != 0)) goto err;

	/** Add the content **/
	va_start(va, fmt);
	const int rval = htr_internal_QPAddText(s, htrAddBodyItem, (char*)fmt, va);
	va_end(va);
	if (UNLIKELY(rval != 0)) goto err;

	/** Add the closing tag **/
	if (UNLIKELY(htrAddBodyItemLayerEnd(s, flags) != 0)) goto err;

	return 0;

	err:
	mssError(0, "HTR", "Failed to add body item layer of format: \"%s\".", fmt);
	return -1;
    }


/*** htrAddBodyItemLayerStart - adds just the opening tag sequence
 *** but not some content for a layer.  Does not add the closing tag sequence
 *** for the layer.
 ***
 *** WARNING!!! DO NOT ALLOW THE END-USER TO INFLUENCE THE VALUE OF THE 'id'
 *** PARAMETER WHICH IS A FORMAT STRING FOR THE LAYER'S ID!!!
 ***/
int
htrAddBodyItemLayerStart(pHtSession s, int flags, char* id, int cnt, char* class)
    {
    char id_sbuf[64];
    int rval = -1;
    pQPSession error_session = NULL;
	
	/** Create a pQPSession to track errors from qpfPrintf. **/
	error_session = check_ptr(qpfOpenSession());
	if (error_session == NULL) goto end;

	/** Pick a starting tag. **/
	const int use_iframe = (s->Capabilities.HTML40 && (flags & HTR_LAYER_F_DYNAMIC)); 
	const char* start_tag = (use_iframe) ? "iframe frameBorder='0'" : "div";

	/** Add the starting tag. **/
	if (UNLIKELY(qpfPrintf(error_session, id_sbuf, sizeof(id_sbuf), id, cnt) < 0))
	    {
	    mssError(1, "HTR", "Failed to format \"%s\" with %d", id, cnt);
	    qpfLogErrors(error_session);
	    goto end;
	    }
	if (UNLIKELY(htrAddBodyItem_va(s,
	    "<%STR %[class='%STR&HTE' %]id='%STR&HTE'>",
	    start_tag, (class != NULL), class, id_sbuf
	) != 0))
	    {
	    mssError(0, "HTR", "Failed to write HTML tag to start body item.");
	    goto end;
	    }

	/** Success. **/
	rval = 0;

	end:
	if (UNLIKELY(rval != 0)) mssError(0, "HTR", "Failed to add body item layer start for id format: \"%s\".", id);
	
	/** Clean up. **/
	if (LIKELY(error_session != NULL)) qpfCloseSession(error_session);
	
	return rval;
    }


/*** htrAddBodyItemLayerEnd - adds the ending tag
 *** for a layer.  Does not emit the starting tag.
 ***/
int
htrAddBodyItemLayerEnd(pHtSession s, int flags)
    {
	/** Pick a starting tag. **/
	const int use_iframe = (s->Capabilities.HTML40 && (flags & HTR_LAYER_F_DYNAMIC)); 
	const char* end_tag = (use_iframe) ? "iframe frameBorder='0'" : "div";
	
	/** Add it. **/
	if (UNLIKELY(htrAddBodyItem_va(s, "</%STR>", end_tag) != 0))
	    {
	    mssError(0, "HTR", "Failed to write HTML tag to start body item.");
	    return -1;
	    }

    return 0;
    }


/*** htrGetExpParams - retrieve the list of values used in an expression
 *** and put them in a javascript array.
 ***/
int
htrGetExpParams(pExpression exp, pXString xs)
    {
    int rval = -1;
    XArray objs = { nAlloc: 0 }, props = { nAlloc: 0 };

	/** setup **/
	if (check(xaInit(&objs, 16)) != 0) goto end;
	if (check(xaInit(&props, 16)) != 0) goto end;

	/** Find the properties accessed by the expression **/
	if (check_neg(expGetPropList(exp, &objs, &props) < 0)) goto end;

	/** Build the list **/
	if (check(xsCopy(xs, "[", -1)) != 0) goto end;
	bool first = true;
	for (unsigned int i = 0u; i < objs.nItems; i++)
	    {
	    const char* obj = (char*)(objs.Items[i]);
	    const char* prop = (char*)(props.Items[i]);
	    if (obj && prop)
		{
		if (check_neg(xsConcatQPrintf(xs,
		    "%[,%]{obj:'%STR&JSSTR',attr:'%STR&JSSTR'}",
		    (!first), obj, prop
		)) < 0) goto end;
		first = false;
		}
	    }
	if (check_neg(xsConcatenate(xs, "]", 1)) < 0) goto end;
	
	/** Success. **/
	rval = 0;

	end:
	if (UNLIKELY(rval != 0))
	    mssError(1, "HTR", "Failed to get params for expresion: \"%s\".", exp->Name);
	
	/** Clean up. **/
	for (unsigned int i = 0u; i < objs.nItems; i++)
	    {
	    if (objs.Items[i] != NULL) nmSysFree(objs.Items[i]);
	    if (props.Items[i] != NULL) nmSysFree(props.Items[i]);
	    }
	if (LIKELY(objs.nAlloc != 0)) xaDeInit(&objs);
	if (LIKELY(props.nAlloc != 0)) xaDeInit(&props);
	
	return rval;
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
    int rval = -1, cls = 1;
    XArray objs = { nAlloc: 0 }, props = { nAlloc: 0 };
    XString xs = { AllocLen: 0 }, exptxt = { AllocLen: 0 };

	/** Allocate data structures. **/
	if (check(xaInit(&objs, 16)) != 0) goto end;
	if (check(xaInit(&props, 16)) != 0) goto end;
	if (check(xsInit(&xs)) != 0) goto end;
	if (check(xsInit(&exptxt)) != 0) goto end;
	if (check_neg(expGetPropList(exp, &objs, &props) < 0)) goto end;

	/** Copy expression data. 8*/
	if (check_neg(xsCopy(&xs, "[", -1)) < 0) goto end;
	bool first = true;
	for (unsigned int i = 0u; i < objs.nItems; i++)
	    {
	    const char* obj = (char*)(objs.Items[i]);
	    const char* prop = (char*)(props.Items[i]);
	    if (obj && prop)
		{
		if (check_neg(xsConcatQPrintf(&xs,
		    "%[,%]['%STR&JSSTR','%STR&JSSTR']",
		    (!first), obj, prop
		)) < 0) goto end;
		first = false;
		}
	    }
	if (check_neg(xsConcatenate(&xs, "]", 1)) < 0) goto end;
	if (UNLIKELY(expGenerateText(exp, NULL, xsWrite, &exptxt, '\0', "javascript", EXPR_F_RUNCLIENT) != 0))
	    {
	    mssError(0, "HTR", "Failed to generate expression text.");
	    cls = 0;
	    goto end;
	    }
	if (UNLIKELY(htrAddExpressionItem_va(s,
	    "    pg_expression('%STR&SYM','%STR&SYM',function (_context, _this) { return %STR; },%STR,'%STR&SYM');\n",
	    objname, property, exptxt.String, xs.String, s->Namespace->DName
	) != 0))
	    {
	    mssError(0, "HTR", "Failed to write the expresion item.");
	    cls = 0;
	    goto end;
	    }

	/** Success. **/
	rval = 0;

	end:
	if (UNLIKELY(rval != 0)) mssError(cls, "HTR", "Failed to add expression \"%s\".", objname);
	
	/** Clean up. **/
	for (unsigned int i = 0; i < objs.nItems; i++)
	    {
	    if (objs.Items[i] != NULL) nmSysFree(objs.Items[i]);
	    if (props.Items[i] != NULL) nmSysFree(props.Items[i]);
	    }
	if (LIKELY(objs.nAlloc != 0)) xaDeInit(&objs);
	if (LIKELY(props.nAlloc != 0)) xaDeInit(&props);
	if (LIKELY(xs.AllocLen != 0)) xsDeInit(&xs);
	if (LIKELY(exptxt.AllocLen != 0)) xsDeInit(&exptxt);
	
	return rval;
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
	    if (UNLIKELY(wgtrGetPropertyValue(tree, property, DATA_T_CODE, POD(&code)) < 0))
		{
		mssError(1, "HTR", "Failed to get property: '%s'", property);
		goto err;
		}
	    if (UNLIKELY(htrAddExpression(s, w_name, property, code) != 0)) goto err;
	    return 1;
            }

	return 0;

	err:
	mssError(0, "HTR",
	    "Failed to add expression \"%s\" to widget \"%s\":\"%s\".",
	    property, tree->Name, tree->Type
	);
	return -1;
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
	/** Iterate through each sub_widget and call its render function. **/
	const unsigned int n_sub_widgets = widget->Children.nItems;
	for (unsigned int i = 0u; i < n_sub_widgets; i++)
	    {
	    pWgtrNode sub_widget = widget->Children.Items[i];
	    if (UNLIKELY(htrRenderWidget(s, sub_widget, zlevel) != 0))
		{
		mssError(0, "HTR",
		    "Failed to render \"%s\":\"%s\", child #%d/%d of \"%s\":\"%s\"",
		    sub_widget->Name, sub_widget->Type, i + 1, n_sub_widgets, widget->Name, widget->Type
		);
		return -1;
		}
	    }

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
		htrQPrintf(s, "\t<script>\n// Included from: %STR&HTE\n\n", filename);
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
	if (slash != NULL)
	    {
	    *slash = '\0';
	    htrQPrintf(s, "\t<script src='%STR%[/CXDC:%STR%]/%STR'></script>\n", path, buf[0], buf, slash+1);
	    }

    return 0;
    }


/*** htr_internal_WriteWgtrProperty - write one widget property in javascript
 *** as a part of the widget tree
 ***/
int
htr_internal_WriteWgtrProperty(pHtSession s, pWgtrNode tree, char* propname)
    {    
	/** Get type. **/
	const int type = wgtrGetPropertyType(tree, propname);
	if (UNLIKELY(type < 0))
	    {
	    mssError(1, "HTR", "Failed to get property type for property '%s'.", propname);
	    goto err;
	    }
	
	/** Get value. **/
	ObjData od;
	const int result = wgtrGetPropertyValue(tree, propname, type, &od);
	if (UNLIKELY(result < 0))
	    {
	    mssError(1, "HTR",
		"Failed to get property value for property '%s' of type %d.",
		propname, type
	    );
	    goto err;
	    }
	
	/** Value is null. **/
	if (result == 1)
	    {
	    if (UNLIKELY(htrAddScriptWgtr_va(s, "%STR&SYM:null, ", propname) != 0)) goto err;
	    return 0; /* success */
	    }
	
	/** Write values by type. **/
	switch (type)
	    {
	    case DATA_T_INTEGER:
		if (UNLIKELY(htrAddScriptWgtr_va(s, "%STR&SYM:%INT, ", propname, od.Integer) != 0)) goto err;
		break;
	    
	    case DATA_T_STRING:
		if (UNLIKELY(htrAddScriptWgtr_va(s, "%STR&SYM:'%STR&JSSTR', ", propname, od.String) != 0)) goto err;
		break;
	    
	    case DATA_T_DOUBLE:
		if (UNLIKELY(htrAddScriptWgtr_va(s, "%STR&SYM:%DBL, ", propname, od.Double) != 0)) goto err;
		break;
	    
	    case DATA_T_DATETIME:
		if (UNLIKELY(htrAddScriptWgtr_va(s,
		    "%STR&SYM:new Date(%LL, %LL, %LL, %LL, %LL, %LL), ",
		    propname,
		    (long long)od.DateTime->Part.Year + 1900,
		    (long long)od.DateTime->Part.Month,
		    (long long)od.DateTime->Part.Day + 1,
		    (long long)od.DateTime->Part.Hour,
		    (long long)od.DateTime->Part.Minute,
		    (long long)od.DateTime->Part.Second
		) != 0)) goto err;
		break;
	    
	    case DATA_T_INTVEC:
		if (UNLIKELY(htrAddScriptWgtr_va(s, "%STR&SYM:[", propname) != 0)) goto err;
		for (unsigned int i = 0; i < od.IntVec->nIntegers; i++)
		    if (UNLIKELY(htrAddScriptWgtr_va(s, "%[, %]%INT", (i != 0), od.IntVec->Integers[i]) != 0)) goto err;
		if (UNLIKELY(htrAddScriptWgtr(s, "], ") != 0)) goto err;
		break;
	    
	    case DATA_T_STRINGVEC:
		if (UNLIKELY(htrAddScriptWgtr_va(s, "%STR&SYM:[", propname) != 0)) goto err;
		for (unsigned int i = 0; i < od.StringVec->nStrings; i++)
		    if (UNLIKELY(htrAddScriptWgtr_va(s, "%[, %]'%STR&JSSTR'", (i != 0), od.StringVec->Strings[i]) != 0)) goto err;
		if (UNLIKELY(htrAddScriptWgtr(s, "], ") != 0)) goto err;
		break;
	    
	    case DATA_T_CODE:
		{
		pExpression code;
		XString exptxt = { AllocLen: 0 };
		XString proptxt = { AllocLen: 0 };
		
		if (UNLIKELY(wgtrGetPropertyValue(tree, propname, DATA_T_CODE, POD(&code)) < 0))
		    {
		    mssError(1, "HTR", "Failed to get value for property '%s'", propname);
		    goto err_free;
		    }
		if (check(xsInit(&exptxt)) != 0) goto err_free;
		if (check(xsInit(&proptxt)) != 0) goto err_free;
		if (UNLIKELY(htrGetExpParams(code, &proptxt) != 0)) goto err_free;
		if (UNLIKELY(expGenerateText(code, NULL, xsWrite, &exptxt, '\0', "javascript", EXPR_F_RUNCLIENT) != 0))
		    {
		    mssError(0, "HTR", "Failed to generate expression text.");
		    goto err_free;
		    }
		if (UNLIKELY(htrAddScriptWgtr_va(s,
		    "%STR&SYM:{ val:null, exp:(_this, _context) => { return ( %STR ); }, props:%STR, revexp:null }, ",
		    propname, exptxt.String, proptxt.String
		) != 0)) goto err_free;
		break;
		
    err_free:   /** Clean up. **/
		if (proptxt.AllocLen != 0) check_neg(xsDeInit(&proptxt)); /* Failure ignored. */
		if (exptxt.AllocLen != 0) check_neg(xsDeInit(&exptxt)); /* Failure ignored. */
		goto err;
		}
	    
	    default:
		mssError(1, "HTR", "Unknown datatype %d.\n", type);
		break;
	    }
	
	/** Success. **/
	return 0;

	err:
	mssError(0, "HTR",
	    "Failed to write widget property \"%s\" to widget \"%s\":\"%s\".",
	    propname, tree->Name, tree->Type
	);
	
	return -1;
    }


/*** htr_internal_BuildClientWgtr - generate the DHTML to represent the widget
 *** tree.
 ***/
int
htr_internal_BuildClientWgtr_r(pHtSession s, pWgtrNode tree, int indent)
    {
    int childcnt = xaCount(&tree->Children);
    pHtDMPrivateData inf = wgtrGetDMPrivateData(tree);
    pWgtrNode child;
    char* scope = NULL;
    char* scopename = NULL;
    char* propname;

	/** Check recursion **/
	if (UNLIKELY(thExcessiveRecursion()))
	    {
	    mssError(1,"HTR","Could not render application: resource exhaustion occurred");
	    goto err;
	    }

	/** Widget name scope **/
	if (wgtrGetPropertyValue(tree,"scope",DATA_T_STRING,POD(&scope)) == 0)
	    {
	    if (strcmp(scope,"application") && strcmp(scope,"local") && strcmp(scope,"session"))
		scope = NULL;
	    }
	wgtrGetPropertyValue(tree,"scope_name",DATA_T_STRING,POD(&scopename));

	/** Deploy the widget **/
	const char* objinit = (inf != NULL) ? (inf->ObjectLinkage) : NULL;
	const char* ctrinit = (inf != NULL) ? (inf->ContainerLinkage) : NULL;
	const int result = htrAddScriptWgtr_va(s, 
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
	if (UNLIKELY(result != 0))
	    {
	    mssError(0, "HTR", "Failed to write JS data.");
	    goto err;
	    }

	/** Parameters **/
	if (UNLIKELY(htrAddScriptWgtr(s, ", param:{") != 0)) goto err;
	if (!(tree->Flags & WGTR_F_NONVISUAL))
	    {
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "x") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "y") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "width") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "height") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "r_x") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "r_y") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "r_width") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "r_height") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_x") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_y") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_width") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_height") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_scale_x") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_scale_y") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_scale_w") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_scale_h") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_parent_w") != 0)) goto err;
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, "fl_parent_h") != 0)) goto err;
	    }
	propname = wgtrFirstPropertyName(tree);
	while (propname != NULL)
	    {
	    if (UNLIKELY(htr_internal_WriteWgtrProperty(s, tree, propname) != 0)) goto err;
	    propname = wgtrNextPropertyName(tree);
	    }
	if (UNLIKELY(htrAddScriptWgtr(s, "}") != 0)) goto err;

	/** ... and any subwidgets **/ //TODO: there's a glitch in this section in which a comma is placed after the last element of an array.
	unsigned int rendercnt;
	for (unsigned int i = rendercnt = 0u; i < childcnt; i++)
	    {
	    child = (pWgtrNode)xaGetItem(&tree->Children, i);
	    if (child->RenderFlags & HT_WGTF_NORENDER)
		continue;
	    rendercnt++;
	    }
	if (LIKELY(rendercnt > 0))
	    {
	    if (htrAddScriptWgtr_va(s,
		", sub:\n        %STR&*LEN    [\n",
		indent*4, "                                        "
	    ) != 0) goto err;
	    rendercnt = 0;
	    for (unsigned int i = 0u; i < childcnt; i++)
		{
		child = (pWgtrNode)xaGetItem(&tree->Children, i);
		if (child->RenderFlags & HT_WGTF_NORENDER)
		    continue;
		rendercnt--;
		if (htr_internal_BuildClientWgtr_r(s, child, indent+1) < 0)
		    goto err;
		if (rendercnt == 0) /* last one - no comma */
		    htrAddScriptWgtr(s, "\n");
		else
		    htrAddScriptWgtr(s, ",\n");
		}
	    htrAddScriptWgtr_va(s, "        %STR&*LEN    ] }", indent*4, "                                        ");
	    }
	else
	    {
	    if (UNLIKELY(htrAddScriptWgtr(s, "}") != 0)) goto err;
	    }

	/** Success. **/
	return 0;

	err:
	mssError(0, "HTR",
	    "Failed to build client widget tree for widget \"%s\":\"%s\" (indent: %d).",
	    tree->Name, tree->Type, indent
	);
	
	return -1;
    }

int
htrBuildClientWgtr(pHtSession s, pWgtrNode tree)
    {

	if (htrAddScriptInclude(s, "/sys/js/ht_utils_wgtr.js", 0) != 0) goto err;
	if (htrAddScriptWgtr_va(s, "    pre_%STR&SYM =\n", tree->DName) != 0) goto err;
	if (htr_internal_BuildClientWgtr_r(s, tree, 0) != 0) goto err;
	if (htrAddScriptWgtr(s, ";\n") != 0) goto err;

	/** Success. **/
	return 0;

	err:
	mssError(0, "HTR",
	    "Failed to build client widget tree for widget \"%s\":\"%s\".",
	    tree->Name, tree->Type
	);
	
	return -1;
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
	rval = xsQPrintf_va(xs, fmt, va);
	va_end(va);
	if (rval < 0) return rval;
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

	/** What UA is on the other end of the connection? **/
	agent = (char*)mssGetParam("User-Agent");
	if (!agent)
	    {
	    mssError(1, "HTR", "User-Agent undefined in the session parameters");
	    return -1;
	    }

    	/** Initialize the session **/
	s = (pHtSession)check_ptr(nmMalloc(sizeof(HtSession)));
	if (s == NULL) return -1;
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
	htrAddScriptInit_va(s, "\tvar ns = '%STR&SYM';\n", s->Namespace->DName);
	htrAddScriptInit_va(s, "\tbuild_wgtr_%STR&SYM();\n", s->Namespace->DName);

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
	htrAddStylesheetItem(s, "\t\t#dbgwnd {position: absolute; top: 400; left: 50;}\n");
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
		htrQPrintf(s, "<html><head><title>Error</title></head><body style='background-color:white'><h1>An Error occurred while attempting to render this document</h1><br><pre>%STR&HTE</pre></body></html>\r\n", xsString(err_xs));
		xsFree(err_xs);
		}
	    }
	
	/** Output the DOCTYPE for browsers supporting HTML 4.0 -- this will make them use HTML 4.0 Strict **/
	/** FIXME: should probably specify the DTD.... **/
	if(s->Capabilities.HTML40 && !s->Capabilities.Dom0IE)
	    htrWrite(s, "<!doctype html>\n\n", 17);

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

	htrQPrintf(s,	"<html lang='en'>\n"
			"<head>\n"
			    "\t<meta charset='utf-8'>\n"
			    "\t<meta name='generator' content='Centrallix/%STR'>\n"
			    "\t<meta name='pragma' content='no-cache'>\n"
			    "\t<meta name='referrer' content='same-origin'>\n"
			, cx__version);

	htrWrite(s, "\t<style>\n", 9);
	/** Write the HTML stylesheet items. **/
	for(i=0;i<s->Page.HtmlStylesheet.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlStylesheet.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }
	htrWrite(s, "\t</style>\n", -1);
	/** Write the HTML header items. **/
	for(i=0;i<s->Page.HtmlHeader.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlHeader.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }

	/** Write the script globals **/
	htrWrite(s, "\t<script>\n", 10);
	for(i=0;i<s->Page.Globals.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Globals.Items[i]);
	    if (sv->Value[0] != '\0')
		{
		qpfPrintf(NULL, sbuf, HT_SBUF_SIZE,
		    "\t\tif (typeof %STR&SYM == 'undefined') var %STR&SYM = %STR;\n",
		    sv->Name, sv->Name, sv->Value
		);
		}
	    else
		{
		qpfPrintf(NULL, sbuf, HT_SBUF_SIZE, "\t\tvar %STR&SYM;\n", sv->Name);
		}
	    htrWrite(s, sbuf, -1);
	    }

	/** Write the includes **/
	htrWrite(s, "\t</script>\n\n", 12);

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
	htrWrite(s, "\n\t<script>", 10);

	/** Write the script functions **/
	for(i=0;i<s->Page.Functions.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Functions.Items[i]);
	    htrWrite(s, sv->Value, -1);
	    }


	/** Start writing the event registration function. **/
	htrQPrintf(s," \nfunction events_%STR()\n\t{\n", s->Namespace->DName);

	/** Write the event captures. **/
	cnt = xaCount(&s->Page.EventHandlers);
	strcpy(sbuf,
	    "\tif (window.Event)\n"
		"\t\t{\n"
		"\t\thtr_captureevents(");
	for(i=0;i<cnt;i++)
	    {
	    e = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    if (i) strcat(sbuf, " | ");
	    strcat(sbuf,"Event.");
	    strcat(sbuf,e->DomEvent);
	    }
	strcat(sbuf,");\n\t\t}\n");
	htrWrite(s, sbuf, -1);

	/** Write event handlers and listeners. **/
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
		htrQPrintf(s,
		    "\thtr_addeventhandler(%STR&DQUOT,%STR&DQUOT);\n",
		    ename, xaGetItem(&e->Handlers, j)
		);
		}
	    if (strcmp(ename, "mousemove") == 0)
		{
		htrQPrintf(s,
		    "\thtr_addeventlistener(%STR&DQUOT, document, htr_mousemovehandler);\n",
		    ename
		);
		}
	    else
		{
		htrQPrintf(s,
		    "\thtr_addeventlistener(%STR&DQUOT, document, htr_eventhandler);\n",
		    ename
		);
		}
	    }

	/** Finish writing the event registration function. **/
	htrWrite(s, "\t}\n", 3);

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
	htrQPrintf(s, "\nfunction startup_%STR()\n\t{\n", s->Namespace->DName);
	htr_internal_writeCxCapabilities(s); //TODO: (by Seth) this really only needs to happen during first-load.
	htrWrite(s, "\n", 1);

	for(i=0;i<s->Page.Inits.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Inits.Items[i]);
	    n = *(int*)ptr;
	    htrWrite(s, ptr+8, n);
	    }
	htrQPrintf(s, "\tevents_%STR();\n", s->Namespace->DName);
	htrQPrintf(s, "\texpinit_%STR();\n", s->Namespace->DName);
	htrWrite(s, "\t}\n", -1);

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
	drv = (pHtDriver)check_ptr(nmMalloc(sizeof(HtDriver)));
	if (drv == NULL) goto err;
	memset(drv, 0, sizeof(HtDriver));

	/** Init some of the basic array structures **/
	if (check(xaInit(&(drv->PosParams), 16)) != 0) goto err;
	if (check(xaInit(&(drv->Properties), 16)) != 0) goto err;
	if (check(xaInit(&(drv->Events), 16)) != 0) goto err;
	if (check(xaInit(&(drv->Actions), 16)) != 0) goto err;
	if (check(xaInit(&(drv->PseudoTypes), 4)) != 0) goto err;

	/** Success. **/
	return drv;

	err:
	mssError(0, "HTR", "Failed to allocate driver.");
	return NULL;
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
    pQPSession error_session = NULL;
    int rval = -1;

	/** Initialize the buffer. **/
	if (UNLIKELY(buflen < 1))
	    {
	    mssError(1, "HTR", "Failed! buflen (%d) < 1", buflen);
	    goto end;
	    }
	buf[0] = '\0';
	
	/** Create a pQPSession to track errors from qpfPrintf. **/
	error_session = check_ptr(qpfOpenSession());
	if (error_session == NULL) goto end;

	/** Prefix supplied? **/
	if (prefix != NULL && prefix[0] != '\0')
	    {
	    /** Initialize buffers with prefixed attribute names. **/
	    if (qpfPrintf(error_session, bgcolor_name, sizeof(bgcolor_name), "%STR&SYM_bgcolor", prefix) < 0)
		{
		mssError(1, "HTR", "Failed to write background color attribute name.");
		qpfLogErrors(error_session);
		goto end;
		}
	    else qpfClearErrors(error_session);
	    if (qpfPrintf(error_session, background_name, sizeof(background_name), "%STR&SYM_background", prefix) < 0)
		{
		mssError(1, "HTR", "Failed to write background color attribute name.");
		qpfLogErrors(error_session);
		goto end;
		}
	    else qpfClearErrors(error_session);
	    
	    /** Update attribute name pointers. **/
	    bgcolor = bgcolor_name;
	    background = background_name;
	    }

	/** Search for different background types. **/
	if (wgtrGetPropertyValue(tree, background, DATA_T_STRING, POD(&ptr)) == 0)
	    { /* Background image. */
	    /** Check for invalid characters in the string. **/
	    if (strpbrk(ptr, "\"'\n\r\t") != NULL)
		{
		mssError(1, "HTR",
		    "Value for attribute '%s' contains invalid characters: \"%s\"",
		    background, ptr
		);
		goto end;
		}
	    
	    /** Write background image. **/
	    const char* format = (as_style) ? "background-image:URL('%STR&CSSURL');" : "background='%STR&HTE'";
	    if (qpfPrintf(error_session, buf, buflen, format, ptr) < 0)
		{
		mssError(1, "HTR", "Failed to write background image using format: \"%s\"", format);
		qpfLogErrors(error_session);
		goto end;
		}
	    else qpfClearErrors(error_session);
	    }
	else if (wgtrGetPropertyValue(tree, bgcolor, DATA_T_STRING, POD(&ptr)) == 0)
	    { /* Background color. */
	    /** Check for invalid characters in the string. **/
	    if (strpbrk(ptr, "\"'\n\r\t;}<>&") != NULL)
		{
		mssError(1, "HTR",
		    "Value for attribute '%s' contains invalid characters: \"%s\"",
		    background, ptr
		);
		goto end;
		}
	    
	    /** Write background color. **/
	    const char* format = (as_style) ? "background-color:%STR&CSSVAL;" : "bgColor='%STR&HTE";
	    if (qpfPrintf(error_session, buf, buflen, format, ptr) < 0)
		{
		mssError(1, "HTR", "Failed to write background color using format: \"%s\"", format);
		qpfLogErrors(error_session);
		goto end;
		}
	    else qpfClearErrors(error_session);
	    }
	/** Fail quietly, as this may be intended behavior. **/
	else goto free;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    {
	    mssError(0, "HTR",
		"Failed write \"%s\"-background for \"%s\":\"%s\" into buffer %p of length %d.",
		prefix, tree->Name, tree->Type, buf, buflen
	    );
	    }

    free:
	/** Clean up. **/
	if (LIKELY(error_session != NULL)) qpfCloseSession(error_session);

	return -1;
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
	if (t < 0) goto end;

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
	    mssError(1,"HTR","Invalid datatype %d for attribute '%s'", t, attrname);
	    rval = -1;
	    goto end;
	    }

    end:
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
	    inf = (pHtDMPrivateData)check_ptr(nmMalloc(sizeof(HtDMPrivateData)));
	    if (inf == NULL) return NULL;
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
	/** Get private data. **/
	pHtDMPrivateData inf = check_ptr(htr_internal_CheckDMPrivateData(widget));
	if (inf == NULL) goto err;
	
	/** Get temporary string data. **/
	char* str_tmp = check_ptr(objDataToStringTmp(DATA_T_STRING, linkage, DATA_F_QUOTED));
	if (str_tmp == NULL) goto err;
	
	/** Dup string data. **/
	char* str = check_ptr(nmSysStrdup(str_tmp));
	if (str == NULL) goto err;
	
	/** Set string data. **/
	inf->ObjectLinkage = str;

	return 0;

    err:
	mssError(1, "HTR",
	    "Failed to add object linkage \"%s\" to widget \"%s\":\"%s\".",
	    linkage, widget->Name, widget->Type
	);
	return -1;
    }


/*** htrAddWgtrObjLinkage_va() - varargs version of the above.
 ***/
int
htrAddWgtrObjLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...)
    {
    va_list va;
    char buf[256];
    int rval = -1, tmp;
    pQPSession error_session = NULL;

	/** Create a pQPSession to track errors from qpfPrintf. **/
	error_session = check_ptr(qpfOpenSession());
	if (error_session == NULL) goto end;

	/** Process the provided format. **/
	va_start(va, fmt);
	tmp = qpfPrintf_va(error_session, buf, sizeof(buf), fmt, va);
	va_end(va);
	if (UNLIKELY(tmp < 0))
	    {
	    mssError(1, "HTR", "qpfPrintf_va() failed to format: \"%s\"", fmt);
	    qpfLogErrors(error_session);
	    rval = tmp;
	    goto end;
	    }
	
	/** Add the linkage. **/
	tmp = htrAddWgtrObjLinkage(s, widget, buf);
	if (UNLIKELY(tmp < 0)) goto end;
	
	/** Success. **/
	rval = 0;
    
    end:
	if (rval != 0)
	    {
	    mssError(0, "HTR",
		"Failed to add object linkage of format \"%s\" to widget \"%s\":\"%s\".",
		fmt, widget->Name, widget->Type
	    );
	    }
	
	/** Clean up. **/
	if (LIKELY(error_session != NULL)) qpfCloseSession(error_session);
	
	return rval;
    }


/*** htrAddWgtrCtrLinkage() - specify what function/object to call to find out
 *** what the actual client-side object is that represents an object inside
 *** the widget tree.
 ***/
int
htrAddWgtrCtrLinkage(pHtSession s, pWgtrNode widget, char* linkage)
    {
	/** Get private data. **/
	pHtDMPrivateData inf = check_ptr(htr_internal_CheckDMPrivateData(widget));
	if (inf == NULL) goto err;
	
	/** Get temporary string data. **/
	char* str_tmp = check_ptr(objDataToStringTmp(DATA_T_STRING, linkage, DATA_F_QUOTED));
	if (str_tmp == NULL) goto err;
	
	/** Dup string data. **/
	char* str = check_ptr(nmSysStrdup(str_tmp));
	if (str == NULL) goto err;
	
	/** Set string data. **/
	inf->ContainerLinkage = str;

	return 0;

    err:
	mssError(1, "HTR",
	    "Failed to add container linkage \"%s\" to widget \"%s\":\"%s\".",
	    linkage, widget->Name, widget->Type
	);
	return -1;
    }


/*** htrAddWgtrCtrLinkage_va() - varargs version of the above.
 ***/
int
htrAddWgtrCtrLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...)
    {
    va_list va;
    char buf[256];
    int rval = -1, tmp;
    pQPSession error_session = NULL;

	/** Create a pQPSession to track errors from qpfPrintf. **/
	error_session = check_ptr(qpfOpenSession());
	if (error_session == NULL) goto end;

	/** Process the provided format. **/
	va_start(va, fmt);
	tmp = qpfPrintf_va(error_session, buf, sizeof(buf), fmt, va);
	va_end(va);
	if (UNLIKELY(tmp < 0))
	    {
	    mssError(1, "HTR", "qpfPrintf_va() failed to format: \"%s\"", fmt);
	    qpfLogErrors(error_session);
	    rval = tmp;
	    goto end;
	    }

	/** Add the linkage. **/
	rval = htrAddWgtrCtrLinkage(s, widget, buf);
	if (tmp < 0) goto end;
	
	/** Success. **/
	rval = 0;
    
    end:
	if (rval != 0)
	    {
	    mssError(1, "HTR",
		"Failed to add object linkage of format \"%s\" to widget \"%s\":\"%s\".",
		fmt, widget->Name, widget->Type
	    );
	    }
	
	/** Clean up. **/
	if (LIKELY(error_session != NULL)) qpfCloseSession(error_session);
	
	return rval;
    }


/*** htrAddWgtrInit() - sets the initialization function for the widget
 ***/
int
htrAddWgtrInit(pHtSession s, pWgtrNode widget, char* func, char* paramfmt, ...)
    {
    va_list va;
    char buf[256];

	pHtDMPrivateData inf = check_ptr(htr_internal_CheckDMPrivateData(widget));
	if (UNLIKELY(inf == NULL)) goto err;

	/** Process format. **/
	inf->InitFunc = func;
	va_start(va, paramfmt);
	vsnprintf(buf, sizeof(buf), paramfmt, va);
	va_end(va);

	/** Process string data. **/
	char* str_tmp = check_ptr(objDataToStringTmp(DATA_T_STRING, buf, DATA_F_QUOTED));
	if (str_tmp == NULL) goto err;
	char* str = check_ptr(nmSysStrdup(str_tmp));
	if (str == NULL) goto err;
	inf->Param = str;

	return 0;

    err:
	mssError(1, "HTR",
	    "Failed to set widget initialization function to %s(%s) for widget \"%s\":\"%s\".",
	    func, paramfmt, widget->Name, widget->Type
	);
	return -1;
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
	new_ns = (pHtNamespace)check_ptr(nmMalloc(sizeof(HtNamespace)));
	if (UNLIKELY(new_ns == NULL)) goto err;
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

    err:;
	char context[152];
	if (container != NULL)
	    snprintf(context, sizeof(context),
	    "to container widget \"%s\":\"%s\"",
	    container->Name, container->Type
	);
	mssError(1, "HTR",
	    "Failed to add namespace \"%s\"%s.",
	    nspace, context
	);
	return -1;
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
	const int result = htrAddStylesheetItem_va(s,
	    "\t\t%STR { "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"%[width:"ht_flex_format"; %]"
		"%[height:"ht_flex_format"; %]"
		"%[z-index:%POS; %]"
		"%[color:%STR&CSSVAL; %]"
		"%[font-weight:bold; %]"
		"%[text-decoration:underline; %]"
		"%[font-style:italic; %]"
		"%[font:%STR&CSSVAL; %]"
		"%[font-size:%DBLpx; %]"
		"%[background-color:%STR&CSSVAL; %]"
		"%[background-image:url('%STR&CSSURL'); %]"
		"%[padding:%DBLpx; %]"
		"%[border:1px %STR&CSSVAL %STR&CSSVAL; %]"
		"%[border-radius:%DBLpx; %]"
		"%[text-align:%STR&CSSVAL; %]"
		"%[white-space:nowrap; %]"
		"%[box-shadow:"
		    "%DBLpx "
		    "%DBLpx "
		    "%DBLpx "
		    "%STR&CSSVAL%STR&CSSVAL; "
		"%]"
		"%[%STR %]"
	    "}\n",
	    id,
	             ht_flex_x(x, node),
	             ht_flex_y(y, node),
	    (w > 0), ht_flex_w(w, node),
	    (h > 0), ht_flex_h(h, node),
	    (z > 0), z,
	    (*textcolor), textcolor,
	    (!strcmp(style, "bold")),
	    (!strcmp(style, "underline")),
	    (!strcmp(style, "italic")),
	    (*font), font,
	    (font_size > 0), font_size,
	    (*bgcolor), bgcolor,
	    (*background), background,
	    (padding > 0), padding,
	    (*border_color), (*border_style) ? border_style : "solid", border_color,
	    (border_radius > 0), border_radius,
	    (*align), align,
	    (!wrap),
	    (*shadow_color && shadow_radius > 0),
	    sin(shadow_angle * M_PI/180) * shadow_offset,
	    cos(shadow_angle * M_PI/180) *(-shadow_offset),
	    shadow_radius,
	    shadow_color, (strcasecmp(shadow_location, "inside") == 0) ? " inset" : "",
	    (addl != NULL && addl[0] != '\0'), addl
	);
	if (UNLIKELY(result != 0))
	    {
	    mssError(0, "HTR", "Failed to write CSS while formatting element.\n");
	    return -1;
	    }

    return 0;
    }


int
ht_get_parent_w__INTERNAL(pWgtrNode widget)
    {
    /** Edge case checks. **/
    if (widget == NULL) 
	{
	mssError(1, "HTR", "Failed to get width on NULL widget.\n");
	return -1;
	}
	
    /** Recursion check. **/
    if (thExcessiveRecursion())
	{
	mssError(1, "HTR", "Resource exhaustion in ht_get_parent_w__INTERNAL()");
	return 0;
	}
    
    /** Check for a cached value. **/
    const int cached_value = widget->fl_parent_w;
    if (cached_value != -1) return cached_value;

    /** Check for a width value on the parent. **/
    const pWgtrNode parent = widget->Parent;
    if (parent == NULL) 
	{
	mssError(1, "HTR",
	    "Failed to get parent width on widget \"%s\":\"%s\" because parent is NULL.\n",
	    widget->Name, widget->Type
	);
	return -1;
	}
    const int parentWidth = parent->width;
    return widget->fl_parent_w = (parentWidth >= 0)
	? parentWidth - (parent->left + parent->right) /* Width found! */
	: ht_get_parent_w__INTERNAL(parent); /* Width not found: search recursively. */
    }

int
ht_get_parent_h__INTERNAL(pWgtrNode widget)
    {
    /** Edge case checks. **/
    if (widget == NULL) 
	{
	mssError(1, "HTR", "Failed to get height on NULL widget.\n");
	return -1;
	}
	
    /** Recursion check. **/
    if (thExcessiveRecursion())
	{
	mssError(1, "HTR", "Resource exhaustion in ht_get_parent_h__INTERNAL()");
	return 0;
	}
    
    /** Check for a cached value. **/
    const int cached_value = widget->fl_parent_h;
    if (cached_value != -1) return cached_value;

    /** Check for a height value on the parent. **/
    const pWgtrNode parent = widget->Parent;
    if (parent == NULL) 
	{
	mssError(1, "HTR",
	    "Failed to get parent height on widget \"%s\":\"%s\" because parent is NULL.\n",
	    widget->Name, widget->Type
	);
	return -1;
	}
    const int parentHeight = parent->height;
    return widget->fl_parent_h = (parentHeight >= 0)
	? parentHeight - (parent->top + parent->bottom) /* Height found! */
	: ht_get_parent_h__INTERNAL(parent); /* Height not found: search recursively. */
    }