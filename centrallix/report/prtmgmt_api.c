#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_new.h"
#include "prtmgmt_private.h"
#include "htmlparse.h"
#include "cxlib/magic.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_*.c, prtmgmt_new.h				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 28th, 2000					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/*									*/
/*		File: prtmgmt_api.c - implements the prtmgmt API that	*/
/*		is visible to the top-layer, with the exception of 	*/
/*		session-level functionality.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_api.c,v 1.2 2005/02/26 06:42:40 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_api.c,v $

    $Log: prtmgmt_api.c,v $
    Revision 1.2  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.1.1.1  2001/08/13 18:01:14  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:17  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** prtWriteString - print a string of text, containing optional embedded
 *** control characters like '\n', '\t'.  '\r' is ignored.
 ***/
int
prtWriteString(pPrtSession this, char* text, int len)
    {	
    char* ptr;
    int n;
    double x;
    pPrtObjStream os;
    
	/** Caller wants us to compute length? **/
	if (len == -1) len = strlen(text);
    
    	/** Empty write? **/
	if (len == 0) return 0;

    	/** Check pagination if just beginning a new line? **/
	if (this->ObjStreamPtr->Width == 0.0 && this->ObjStreamPtr->RelativeX == 0.0)
	    {
	    prt_internal_CheckPageBreak(this);
	    }

	/** Break write operation up into separate chunks? **/
	while ((ptr=memchr(text,'\n',len)) || (ptr=memchr(text,'\t',len)))
	    {
	    /** Print the segment. **/
	    n = ptr - text;
	    if (n) prtWriteString(this,text,n);
	    text = ptr+1;
	    len -= (n+1);

	    /** Handle the special character. **/
	    if (*ptr == '\n') 
	        {
		/** \n is a newline -- use WriteNL for that. **/
		prtWriteNL(this);
		}
	    else if (*ptr == '\t')
	        {
		/** \t is a tab -- round up to the nearest 8.0-align **/
		x = this->ObjStreamPtr->RelativeX + this->ObjStreamPtr->Width;
		n = (int)x;
		x = (n+7)&(~7);
		if (x >= this->ObjStreamPtr->Parent->AvailableWidth)
		    prtWriteNL(this);
		else
		    prtSetPosition(this, x, -1);
		}
	    }

	/** Ok, now we have a normal text string.  See if we can append to an
	 ** existing string element in the objstream.
	 **/
	if (this->ObjStreamPtr->Type == PRT_OS_T_STRING)
	    {
	    /** Appending to an existing string element **/
	    ptr = nmSysMalloc(this->ObjStreamPtr->String.Length + len + 1);
	    if (this->ObjStreamPtr->String.Text)
	        {
		strcpy(ptr, this->ObjStreamPtr->String.Text);
		nmSysFree(this->ObjStreamPtr->String.Text);
		}
	    strncpy(ptr + this->ObjStreamPtr->String.Length, text);
	    ptr[this->ObjStreamPtr->String.Length + len] = '\0';
	    this->ObjStreamPtr->String.Length = this->ObjStreamPtr->String.Length + len;
	    this->ObjStreamPtr->String.Text = ptr;
	    this->ObjStreamPtr->Width = 
	        prt_internal_DetermineTextWidth(this, this->ObjStreamPtr->String.Text, this->ObjStreamPtr->String.Length);
	    }
	else
	    {
	    /** Creating a new string element **/
	    os = prt_internal_AllocOS(PRT_OS_T_STRING);
	    os->String.Text = nmSysMalloc(len+1);
	    memcpy(os->String.Text, text, len);
	    os->String.Text[len] = '\0';
	    os->Width = prt_internal_DetermineTextWidth(this,text,len);
	    prt_internal_CopyStyle(this->ObjStreamPtr, os);
	    os->Height = 6.0/os->LinesPerInch;
	    prt_internal_AddOS(this->ObjStreamPtr->Parent, NULL, this->ObjStreamPtr, os);
	    this->ObjStreamPtr = os;
	    }

	/** Check word-wrap, justification, page/col break, etc. **/
	prt_internal_CheckTextFlow(this, this->ObjStreamPtr);

    return 0;
    }


/*** prtWriteNL - print a newline; that is, cause a paragraph break and move
 *** down to the next logical line of text.
 ***/
int
prtWriteNL(pPrtSession this)
    {
    double y_inc;
    pPrtObjStream os;

    	/** Check pagination if just beginning a new line? **/
	if (this->ObjStreamPtr->Width == 0.0 && this->ObjStreamPtr->RelativeX == 0.0)
	    {
	    prt_internal_CheckPageBreak(this);
	    }

	/** Determine y-increment for new line, and add the new empty-string element **/
	os = prt_internal_AllocOS(PRT_OS_T_STRING);
	prt_internal_CopyStyle(this->ObjStreamPtr, os);
	this->ObjStreamPtr->Flags |= PRT_OS_F_BREAK;
	prt_internal_AddOS(this->ObjStreamPtr->Parent, NULL, this->ObjStreamPtr, os);
	prt_internal_CheckTextFlow(this, this->ObjStreamPtr);
	y_inc = prt_internal_DetermineYIncrement(this, this->ObjStreamPtr);
	os->RelativeX = 0.0;
	os->RelativeY = this->ObjStreamPtr->RelativeY + y_inc;
	this->ObjStreamPtr = os;
	/*prt_internal_CheckTextFlow(this, this->ObjStreamPtr);*/
    
    return 0;
    }


/*** prtWriteFF - print a form feed; that is cause a page break and output
 *** the current page to the printer, possibly after running the footer-
 *** generator callback functions.
 ***/
int
prtWriteFF(pPrtSession this)
    {

    	/** Cause a page break **/
	prt_internal_PageBreak(this);

    return 0;
    }


/*** prtWriteLine - prints a horizontal line across the page.
 ***/
int
prtWriteLine(pPrtSession this)
    {
    pPrtObjStream os;

    	/** Need to issue a newline first? **/
	if (this->ObjStreamPtr->Width != 0.0 || this->ObjStreamPtr->RelativeX != 0.0)
	    {
	    prtWriteNL(this);
	    }
	prt_internal_CheckPageBreak(this);

	/** Allocate the picture element **/
	os = prt_internal_AllocOS(PRT_OS_T_PICTURE);
	prt_internal_CopyStyle(this->ObjStreamPtr, os);
	os->Picture.VisualWidth = this->ObjStreamPtr->Parent->AvailableWidth;
	os->Picture.VisualHeight = this->ObjStreamPtr->Height;
	os->Width = os->Picture.VisualWidth;
	os->Height = os->Picture.VisualHeight;
	os->Picture.DataLength = 3*12;
	os->Picture.ColorMode = PRT_IMG_HIGHCOLOR;
	os->Picture.Flags = 0;
	os->Picture.PixelWidth = 1;
	os->Picture.PixelHeight = 12;
	os->Picture.DataPixelWidth = 1;
	os->Picture.DataPixelHeight = 12;
	os->Picture.TextOffset = 0.0;
	os->Picture.Flags = PRT_OS_F_INFLOW | PRT_OS_F_BREAK;
	os->Picture.Data = nmSysMalloc(os->Picture.DataLength);
	memset(os->Picture.Data, 0, os->Picture.DataLength);
	os->Picture.Data[3*5 + 0] = os->FGColor >> 16;
	os->Picture.Data[3*5 + 1] = (os->FGColor >> 8) & 0xFF;
	os->Picture.Data[3*5 + 2] = os->FGColor & 0xFF;
	os->Picture.Data[3*6 + 0] = os->FGColor >> 16;
	os->Picture.Data[3*6 + 1] = (os->FGColor >> 8) & 0xFF;
	os->Picture.Data[3*6 + 2] = os->FGColor & 0xFF;

	/** Add the picture to the objstream, and begin a new line. **/
	prt_internal_AddOS(this->ObjStreamPtr->Parent, NULL, this->ObjStreamPtr, os);
	prtWriteNL(this);

    return 0;
    }

