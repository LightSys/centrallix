#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <stdarg.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

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

    $Id: ht_render.c,v 1.23 2002/07/18 20:12:40 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/ht_render.c,v $

    $Log: ht_render.c,v $
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
    XHashTable	WidgetDrivers;		/* widget name -> driver struct map */
    XArray	Drivers;		/* simple driver listing. */
    }
    HTR;

/** Several XArrays that are used to hold compiled regular expressions for quick lookup **/
XArray htregNtsp47;
XArray htregMoz;
XArray htregMSIE;

XHashTable htWidgetSets;
XHashTable htNtsp47_default;
XHashTable htMSIE_default;
XHashTable htMoz_default;

/*** htrRegisterUserAgent - creates a bunch of regular expressions that
 *** will be used to do lookups for detecting the user agent.
 ***/
int
htrRegisterUserAgent()
    {
    regex_t *reg;

	xaInit(&(htregNtsp47),4);
	xaInit(&(htregMoz),4);
	xaInit(&(htregMSIE),4);

	/** We will need to regfree the compiled regular expressions at deinitialization
	    of the HTML generator.  Currently this (regfree'ing) is not implemented **/

	/** Netscape 4.7x regular expressions **/
	reg = (regex_t *)nmMalloc(sizeof(regex_t));
	if (!regcomp(reg, "Mozilla\\/4\\.7[5-9]", REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    xaAddItem(&(htregNtsp47), (void *)reg);

	/** Mozilla regular expressions **/
	reg = (regex_t *)nmMalloc(sizeof(regex_t));
	if (!regcomp(reg, "Mozilla\\/5\\.0 .*rv:0\\.9\\.[7-9]", REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    xaAddItem(&(htregMoz), (void *)reg);
	reg = (regex_t *)nmMalloc(sizeof(regex_t));
	if (!regcomp(reg, "Mozilla\\/5\\.0 .*rv:1\\.0\\.[0-9]", REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    xaAddItem(&(htregMoz), (void *)reg);
	reg = (regex_t *)nmMalloc(sizeof(regex_t));
	if (!regcomp(reg, "Mozilla\\/5\\.0 .*rv:1\\.1a", REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    xaAddItem(&(htregMoz), (void *)reg);

	/** Internet Explorer regular expressions **/
	reg = (regex_t *)nmMalloc(sizeof(regex_t));
	if (!regcomp(reg, "MSIE 5.0[0-9]{0,5}", REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    xaAddItem(&(htregMSIE), (void *)reg);
	reg = (regex_t *)nmMalloc(sizeof(regex_t));
	if (!regcomp(reg, "MSIE 5.5[0-9]{0,5}", REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    xaAddItem(&(htregMSIE), (void *)reg);
	reg = (regex_t *)nmMalloc(sizeof(regex_t));
	if (!regcomp(reg, "MSIE 6.0[0-9]{0,5}", REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    xaAddItem(&(htregMSIE), (void *)reg);

    return 0;
    }

int
htr_internal_GetBrowser(char *browser)
    {
    int count;
    int i;
    regex_t *reg;

    /** first lets check the Netscape 4.7 regexes **/
    count = xaCount(&(htregNtsp47));
    for(i=0;i<count;i++) 
	{
	if ((reg = (regex_t*)xaGetItem(&(htregNtsp47),i))) 
	    {
	    /** 0 signifies a match, REG_NOMATCH signifies the opposite **/
	    if (regexec(reg, browser, (size_t)0, NULL, 0) == 0)
		return HTR_NETSCAPE_47;
	    }
	}
    count = xaCount(&(htregMoz));
    for(i=0;i<count;i++) 
	{
	if ((reg = (regex_t*)xaGetItem(&(htregMoz),i)))
	    {
	    /** 0 signifies a match, REG_NOMATCH signifies the opposite **/
	    if (regexec(reg, browser, (size_t)0, NULL, 0) == 0)
		return HTR_MOZILLA;
	    }
	}
    count = xaCount(&(htregMSIE));
    for(i=0;i<count;i++) 
	{
	if ((reg = (regex_t*)xaGetItem(&(htregMSIE),i)))
	    {
	    /** 0 signifies a match, REG_NOMATCH signifies the opposite **/
	    if (regexec(reg, browser, (size_t)0, NULL, 0) == 0)
		return HTR_MSIE;
	    }
	}

    return 0;
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
    char drv_key[64];
    pXHashTable widget_drivers = NULL;
    char* agent = NULL;

	agent = (char*)mssGetParam("User-Agent");
	if (!agent)
	    {
	    mssError(1, "HTR", "User-Agent undefined in the session parameters");
	    return -1;
	    }

	/** Figure out which browser is being used **/
	switch (htr_internal_GetBrowser(agent))
	    {
		case HTR_NETSCAPE_47:
		    strcpy(drv_key, "Netscape47x");
		    break;
		case HTR_MOZILLA:
		    strcpy(drv_key, "Mozilla");
		    break;
		case HTR_MSIE:
		    strcpy(drv_key, "MSIE");
		    break;
		default:
		    htrAddBodyItem_va(session, "No widgets have been defined for your browser type.<br>Your browser was identified as: %s", agent);
		    mssError(1, "HTR", "Unknown browser type");
		    return -1;
	    }

	/** Find the desired style/theme **/
	/** Right now we are just using one theme **/
	strcat(drv_key, ":default");

	/** Find the hashtable keyed with widget names for this combination of 
	 ** user-agent:style that contains pointers to the drivers to use.
	 **/
	widget_drivers = (pXHashTable)xhLookup(&(htWidgetSets), drv_key);
	if (!widget_drivers)
	    {
	    htrAddBodyItem_va(session, "No widgets have been defined for your browser type and requested style combination.<br>Your browser and style have been identified as: %s", drv_key);
	    mssError(1, "HTR", "Invalid UserAgent:style combination %s.", drv_key);
	    return -1;
	    }

	/** Get the name of the widget.. **/
	objGetAttrValue(widget_obj, "outer_type", POD(&w_name));
	if (strncmp(w_name,"widget/",7)) 
	    {
	    mssError(1,"HTR","Invalid content type for widget - must be widget/xxx");
	    return -1;
	    }

	/** Lookup the driver **/
	drv = (pHtDriver)xhLookup(widget_drivers,w_name+7);
	if (!drv) 
	    {
	    mssError(1,"HTR","Unknown widget content type");
	    return -1;
	    }

	/** Call the driver. **/
	drv->Render(session, widget_obj, z, parentname, parentobj);

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
	    if (rval == -1 || rval > (s->TmpbufSize - 1))
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
	    xhInit(&(obj->HashTable),31,0);
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
	    xhInit(&(evt->HashTable),31,0);
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

    	/** Initialize the session **/
	s = (pHtSession)nmMalloc(sizeof(HtSession));
	if (!s) return -1;

	/** Setup the page structures **/
	s->Tmpbuf = nmSysMalloc(512);
	s->TmpbufSize = 512;
	if (!s->Tmpbuf) 
	    {
	    nmFree(s, sizeof(HtSession));
	    return -1;
	    }
	xhInit(&(s->Page.NameFunctions),63,0);
	xaInit(&(s->Page.Functions),32);
	xhInit(&(s->Page.NameIncludes),31,0);
	xaInit(&(s->Page.Includes),32);
	xhInit(&(s->Page.NameGlobals),127,0);
	xaInit(&(s->Page.Globals),64);
	xaInit(&(s->Page.Inits),64);
	xaInit(&(s->Page.Cleanups),64);
	xaInit(&(s->Page.HtmlBody),64);
	xaInit(&(s->Page.HtmlHeader),64);
	xaInit(&(s->Page.HtmlStylesheet),64);
	xaInit(&(s->Page.HtmlBodyParams),16);
	xaInit(&(s->Page.EventScripts.Array),16);
	xhInit(&(s->Page.EventScripts.HashTable),31,0);
	s->Page.HtmlBodyFile = NULL;
	s->Page.HtmlHeaderFile = NULL;
	s->Page.HtmlStylesheetFile = NULL;
	s->DisableBody = 0;

	/** Render the top-level widget. **/
	htrRenderWidget(s, appstruct, 10, "document", "document");

	/** Output the DOCTYPE for Mozilla -- this will make Mozilla use HTML 4.0 Strict **/
	if(htr_internal_GetBrowser((char*)mssGetParam("User-Agent"))==HTR_MOZILLA)
	    fdWrite(output, "<!DOCTYPE HTML>\n",16,0,FD_U_PACKET);
	
	/** Write the HTML out... **/
	snprintf(sbuf, HT_SBUF_SIZE, "<!--\nGenerated by Centrallix v%s (http://www.centrallix.org)\n"
				     "(c) 1998-2002 by LightSys Technology Services, Inc.\n\n", VERSION);
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
	snprintf(sbuf, HT_SBUF_SIZE, "    <META NAME=\"Generator\" CONTENT=\"Centrallix v%s\">\n", VERSION);
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

	/** Write the initialization lines **/
	fdWrite(output,"\nfunction startup()\n    {\n",26,0,FD_U_PACKET);
	fdWrite(output,"    if(typeof(pg_status_init)=='function')pg_status_init();\n",60,0,FD_U_PACKET);
	for(i=0;i<s->Page.Inits.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Inits.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output,"    events();\n", 14,0,FD_U_PACKET);
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


/*** htrRegisterDriver - register a new driver with the rendering system
 *** and map the widget name to the driver's structure for later access.
 ***/
int 
htrRegisterDriver(pHtDriver drv)
    {
	pXHashTable lookupHash;

    	/** Add to the drivers listing and the widget name map. **/
	xaAddItem(&(HTR.Drivers),(void*)drv);
	xhAdd(&(HTR.WidgetDrivers),drv->WidgetName,(char*)drv);

	
	lookupHash = (pXHashTable)xhLookup(&(htWidgetSets),drv->Target);
	if (!lookupHash) 
	    {
	    mssError(1,"HTR","Widget set not found for User-Agent:style combination of %s on driver %s",drv->Target,drv->Name);
	    return -1;
	    }
	xhAdd(lookupHash,drv->WidgetName,(char*)drv);
    return 0;
    }


/*** htrInitialize - initialize the system and the global variables and
 *** structures.
 ***/
int
htrInitialize()
    {
	/** Register the user agents and the regular expressions to match them **/
	htrRegisterUserAgent();

    	/** Initialize the global hash tables and arrays **/
	xaInit(&(HTR.Drivers),64);
	xhInit(&(HTR.WidgetDrivers),255,0);

	/** This is a hashtable that will be keyed with user-agent:style combinations
	    that point to hashtables keyed with widget names that point to the widget 
	    driver. **/
	xhInit(&(htWidgetSets),32,0);

	/** Each user-agent:style combination will have a hashtable of widget names
	    that contain pointers to the widget driver to use.  Each widget set will
	    need a hashtable. **/
	xhInit(&(htNtsp47_default),32,0);
	xhInit(&(htMSIE_default),32,0);
	xhInit(&(htMoz_default),32,0);

	/** Add the target User-Agent/styles to the hash of widget sets.  Each widget
	    set will need to be added here. **/
	xhAdd(&(htWidgetSets),"Netscape47x:default",(char*)&(htNtsp47_default));
	xhAdd(&(htWidgetSets),"MSIE:default",(char*)&(htMSIE_default));
	xhAdd(&(htWidgetSets),"Mozilla:default",(char*)&(htMoz_default));
    return 0;
    }
