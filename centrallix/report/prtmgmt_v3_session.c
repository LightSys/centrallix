#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2001 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt.c,prtmgmt.h                                     */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	December 12, 2001                                       */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_session.c,v 1.11 2005/02/26 06:42:41 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_session.c,v $

    $Log: prtmgmt_v3_session.c,v $
    Revision 1.11  2005/02/26 06:42:41  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.10  2005/02/24 05:44:32  gbeeley
    - Adding PostScript and PDF report output formats.  (pdf is via ps2pdf).
    - Special Thanks to Tim Irwin who participated in the Apex NC CODN
      Code-a-Thon on Feb 5, 2005, for much of the initial research on the
      PostScript support!!  See http://www.codn.net/
    - More formats (maybe PNG?) should be easy to add.
    - TODO: read the *real* font metric files to get font geometries!
    - TODO: compress the images written into the .ps file!

    Revision 1.9  2003/09/02 15:37:13  gbeeley
    - Added enhanced command line interface to test_obj.
    - Enhancements to v3 report writer.
    - Fix for v3 print formatter in prtSetTextStyle().
    - Allow spec pathname to be provided in the openctl (command line) for
      CSV files.
    - Report writer checks for params in the openctl.
    - Local filesystem driver fix for read-only files/directories.
    - Race condition fix in UX printer osdriver
    - Banding problem workaround installed for image output in PCL.
    - OSML objOpen() read vs. read+write fix.

    Revision 1.8  2003/04/21 21:00:48  gbeeley
    HTML formatter additions including image, table, rectangle, multi-col,
    fonts and sizes, now supported.  Rearranged header files for the
    subsystem so that LMData (layout manager specific info) can be
    shared with HTML formatter subcomponents.

    Revision 1.7  2003/03/01 07:24:02  gbeeley
    Ok.  Balanced columns now working pretty well.  Algorithm is currently
    somewhat O(N^2) however, and is thus a bit expensive, but still not
    bad.  Some algorithmic improvements still possible with both word-
    wrapping and column balancing, but this is 'good enough' for the time
    being, I think ;)

    Revision 1.6  2003/02/27 22:02:25  gbeeley
    Some improvements in the balanced multi-column output.  A lot of fixes
    in the multi-column output and in the text layout manager.  Added a
    facility to "schedule" reflows rather than having them take place
    immediately.

    Revision 1.5  2003/02/27 05:21:19  gbeeley
    Added multi-column layout manager functionality to support multi-column
    sections (this is newspaper-style multicolumn formatting).  Tested in
    test_prt "columns" command with various numbers of columns.  Balanced
    mode not yet working.

    Revision 1.4  2003/02/19 22:53:54  gbeeley
    Page break now somewhat operational, both with hard breaks (form feeds)
    and with soft breaks (page wrapping).  Some bugs in how my printer (870c)
    places the text on pages after a soft break (but the PCL seems to look
    correct), and in how word wrapping is done just after a page break has
    occurred.  Use "printfile" command in test_prt to test this.

    Revision 1.3  2002/10/18 22:01:39  gbeeley
    Printing of text into an area embedded within a page now works.  Two
    testing options added to test_prt: text and printfile.  Use the "output"
    option to redirect output to a file or device instead of to the screen.
    Word wrapping has also been tested/debugged and is functional.  Added
    font baseline logic to the design.

    Revision 1.2  2002/04/25 04:30:14  gbeeley
    More work on the v3 print formatting subsystem.  Subsystem compiles,
    but report and uxprint have not been converted yet, thus problems.

    Revision 1.1  2002/01/27 22:50:06  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/


/*** prtOpenSession - open a new printing session, given a target output
 *** content (inner) type, and an output channel (write function and write
 *** parameter).
 ***/
pPrtSession 
prtOpenSession(char* output_type, int (*write_fn)(), void* write_arg, int page_flags)
    {
    pPrtSession this;
    pPrtFormatter f;
    pPrtObjStream page_os;
    int i;

	/** Allocate and init the session structure **/
	this = (pPrtSession)nmMalloc(sizeof(PrtSession));
	if (!this) return NULL;
	SETMAGIC(this, MGK_PRTOBJSSN);
	this->WriteFn = write_fn;
	this->WriteArg = write_arg;
	this->Units = prtLookupUnits("default");
	this->PageWidth = 85.0;		    /* 8-1/2 inches */
	this->PageHeight = 66.0;	    /* 11 inches */
	this->ResolutionX = 72;		    /* 72 dpi */
	this->ResolutionY = 72;		    /* 72 dpi */
	this->PendingEvents = NULL;
	this->ImageContext = NULL;
	this->ImageOpenFn = NULL;
	this->ImageWriteFn = NULL;
	this->ImageCloseFn = NULL;

	/** Search for a formatter module that will do this content type **/
	this->Formatter = NULL;
	for(i=0;i<PRTMGMT.FormatterList.nItems;i++)
	    {
	    f = (pPrtFormatter)(PRTMGMT.FormatterList.Items[i]);
	    if ((this->FormatterData = f->Probe(this,output_type)) != NULL)
	        {
		this->Formatter = f;
		break;
		}
	    }

	/** Error if no formatter liked the requested content type **/
	if (!this->Formatter)
	    {
	    nmFree(this, sizeof(PrtSession));
	    mssError(1,"PRT","Could not find an appropriate formatter for type '%s'", output_type);
	    return NULL;
	    }

	/** Create the initial document object stream **/
	this->StreamHead = prt_internal_AllocObj("document");
	this->StreamHead->Session = this;

	/** Add a page to the document. **/
	page_os = prt_internal_AllocObj("page");
	page_os->Flags &= ~PRT_OBJ_UFLAGMASK;
	page_os->Flags |= (page_flags & PRT_OBJ_UFLAGMASK);
	page_os->Flags &= ~PRT_OBJ_F_REQCOMPLETE;   /* disallow reqcomplete on a page */
	page_os->Width = this->PageWidth;
	page_os->Height = this->PageHeight;
	page_os->ConfigWidth = this->PageWidth;
	page_os->ConfigHeight = this->PageHeight;
	page_os->MarginLeft = 2.5;
	page_os->MarginRight = 2.5;
	page_os->MarginTop = 3.0;
	page_os->MarginBottom = 3.0;
	prt_internal_Add(this->StreamHead, page_os);

	/** Create a handle for the initial page. **/
	this->FocusHandle = prtAllocHandle(page_os);

	/** Set initial attributes. **/
	prtSetAttr(this->FocusHandle, 0);
	prtSetFont(this->FocusHandle, "monospace");
	prtSetFontSize(this->FocusHandle, 12);

    return this;
    }


/*** prtCloseSession - end a printing session.  This results in any
 *** open printstream objects to be closed, and the resulting pages
 *** formatted and written to the output.
 ***/
int 
prtCloseSession(pPrtSession s)
    {
    pPrtObjStream obj;

	ASSERTMAGIC(s, MGK_PRTOBJSSN);

	/** Send any incomplete pages out to the output formatter **/
	if (s->StreamHead && s->StreamHead->ContentHead)
	    {
	    ASSERTMAGIC(s->StreamHead, MGK_PRTOBJSTRM);
	    for(obj=s->StreamHead->ContentHead;obj;obj=obj->Next)
		{
		ASSERTMAGIC(obj, MGK_PRTOBJSTRM);
		if (obj->ContentHead) prt_internal_GeneratePage(s, obj);
		}
	    }

	s->Formatter->Close(s->FormatterData);

	/** Release the memory used by the pages **/
	if (s->StreamHead) prt_internal_FreeTree(s->StreamHead);

	/** Free the session structure and exit. **/
	nmFree(s,sizeof(PrtSession));

    return 0;
    }



/*** prtSetPageGeometry() - sets the width and height of the page, relative
 *** to the units of measure currently in effect.
 ***/
int 
prtSetPageGeometry(pPrtSession s, double width, double height)
    {
    pPrtObjStream obj;
	
	/** Set the session geometry values **/
	s->PageWidth = prtUnitX(s,width);
	s->PageHeight = prtUnitY(s,height);

	/** Resize any current pages. **/
	if (s->StreamHead && s->StreamHead->ContentHead)
	    {
	    for(obj=s->StreamHead->ContentHead;obj;obj=obj->Next)
		{
		obj->LayoutMgr->Resize(obj,s->PageWidth,s->PageHeight);
		obj->ConfigWidth = obj->Width;
		obj->ConfigHeight = obj->Height;
		}
	    }

    return 0;
    }



/*** prtGetPageGeometry() - returns the current page size.
 ***/
int 
prtGetPageGeometry(pPrtSession s, double *width, double *height)
    {

	/** Return width and height **/
	*width = prtUsrUnitX(s,s->PageWidth);
	*height = prtUsrUnitY(s,s->PageHeight);

    return 0;
    }


/*** prtSetUnits() - sets the conversion units which will be in use for this
 *** session.
 ***/
int 
prtSetUnits(pPrtSession s, char* units_name)
    {
    pPrtUnits u;

	/** Lookup the units structure, and set it if found. **/
	u = prtLookupUnits(units_name);
	if (!u) return -1;
	s->Units = u;

    return 0;
    }


/*** prtGetUnits() - return the name of the conversion units name
 *** that is currently in effect for the session.
 ***/
char* 
prtGetUnits(pPrtSession s)
    {
    return s->Units->Name;
    }


/*** prtSetResolution() - set the resolution that is to be used for raster
 *** data rendering on this page.  Note that the resolution does not 
 *** necessarily determine the quality of text output - rather it determines
 *** data resolution for images and such.
 ***/
int
prtSetResolution(pPrtSession s, int dpi)
    {
    s->ResolutionX = dpi;
    s->ResolutionY = dpi;
    return 0;
    }


/*** prtSetImageStore() - set the image store location, which is used by
 *** output formats which do not allow embedding of images physically in
 *** the byte stream of the format itself, but rather must link to those
 *** images (e.g., HTML).
 ***
 *** extdir:	directory where images will appear to external user
 *** sysdir:	directory to be used by the Open function, see below...
 *** open_ctx:	context for open function, below...
 *** open_fn:	call to open a new image, open_fn(open_ctx, path, mode, mask, type)
 *** write_fn:	call to write to image, write_fn(arg, buf, cnt, offs, flg)
 *** close_fn:  call to close image, close_fn(arg)
 ***
 *** The functions above are designed to match the OSML API functions of
 *** the same purposes.
 ***
 *** The prtmgmt subsystem is NOT responsible for managing the lifetime 
 *** of these (perhaps somewhat temporary) images.
 ***/
int
prtSetImageStore(pPrtSession s, char* extdir, char* sysdir, void* open_ctx, void* (*open_fn)(), void* (*write_fn)(), void* (*close_fn)())
    {

	ASSERTMAGIC(s, MGK_PRTOBJSSN);

	/** Set up the session structure with the new information. **/
	memccpy(s->ImageExtDir, extdir, 0, 255);
	s->ImageExtDir[255] = '\0';
	memccpy(s->ImageSysDir, extdir, 0, 255);
	s->ImageSysDir[255] = '\0';
	s->ImageContext = open_ctx;
	s->ImageOpenFn = open_fn;
	s->ImageWriteFn = write_fn;
	s->ImageCloseFn = close_fn;

    return 0;
    }

