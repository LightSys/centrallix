#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "mtask.h"
#include "xarray.h"
#include "xstring.h"
#include "prtmgmt_new.h"
#include "prtmgmt_private.h"
#include "htmlparse.h"
#include "magic.h"

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
/* Creation:	April 13th, 2000					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/*									*/
/*		File: prtmgmt_session.c - open and close session 	*/
/*		functionality and other session-level routines.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_session.c,v 1.1 2001/08/13 18:01:16 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_session.c,v $

    $Log: prtmgmt_session.c,v $
    Revision 1.1  2001/08/13 18:01:16  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:17  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** prtOpenSession - create a new printing session, with an output of a
 *** specific content type, and to a specific output file or object, using
 *** typically either fdWrite or objWrite (though any function conforming
 *** to those standards can also be used).
 ***/
pPrtSession
prtOpenSession(char* content_type, int (*write_fn)(), void* write_arg, int flags)
    {
    pPrintDriver drv;
    pPrtSession this;
    int i;

    	/** First, lookup the driver. **/
	for(i=0;i<PRT_INF.Drivers.nItems;drv=NULL,i++) 
	    {
	    drv = (pPrintDriver)(PRT_INF.Drivers.Items[i]);
	    if (!strcasecmp(content_type, drv->ContentType)) break;
	    }
	if (!drv)
	    {
	    mssError(1,"PRT","Could not find an appropriate driver for '%s'",content_type);
	    return NULL;
	    }

	/** Ok, allocate a session. **/
	this = prt_internal_AllocSession();
	this->Driver = drv;
	this->WriteFn = write_fn;
	this->WriteArg = write_arg;

	/** Open the driver. **/
	this->Driver->Open(this);
	this->Driver->PageGeom((int)(this->PageWidth), (int)(this->PageHeight));

    return this;
    }


/*** prtCloseSession - close an existing session, and print the final page if
 *** necessary.  Does NOT close the open file descriptor or object.
 ***/
int
prtCloseSession(pPrtSession this)
    {

    	/** Need to issue a form-feed first? **/
	if (this->ObjStreamPtr && (this->ObjStreamPtr->RelativeX != 0.0 ||
	     this->ObjStreamPtr->RelativeY != 0.0 || (this->ObjStreamPtr->Type != PRT_OS_T_STRING &&
	      this->ObjStreamPtr->Parent != NULL) || (this->ObjStreamPtr->Parent && 
	      this->ObjStreamPtr->Parent->Parent) || (this->ObjStreamPtr->Type == PRT_OS_T_STRING &&
	      this->ObjStreamPtr->String.Length > 0)))
	    {
	    prtWriteFF(this);
	    }

	/** Close the driver **/
	this->Driver->Close(this);

	/** Deallocate the session **/
	prt_internal_FreeSession(this);

    return 0;
    }

