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
/* Creation:	April 26th, 2000					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/*									*/
/*		File: prtmgmt_main.c - provides the "administrative"	*/
/*		and other internal functionality for prtmgmt.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_main.c,v 1.1 2001/08/13 18:01:15 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_main.c,v $

    $Log: prtmgmt_main.c,v $
    Revision 1.1  2001/08/13 18:01:15  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:17  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** GLOBALS ***/
PRT_INF_t PRT_INF;


/*** These first set of functions deal with the management of the data
 *** structures within the printing system, and the registration of print
 *** output content type drivers.
 ***/


/*** prt_internal_AllocOS - allocate an objstream element of a given type
 ***/
pPrtObjStream
prt_internal_AllocOS(int type)
    {
    pPrtObjStream os;

    	/** Allocate the thing and fill in some defaults **/
	os = (pPrtObjStream)nmMalloc(sizeof(PrtObjStream));
	if (!os) return NULL;
	os->Type = type;
	os->Next = NULL;
	os->Prev = NULL;
	os->YNext = NULL;
	os->YPrev = NULL;
	os->Head = NULL;
	os->Parent = NULL;
	os->Attributes = 0;
	os->FGColor = 0x000000;
	os->BGColor = 0xFFFFFF;
	os->Font = PRT_FONT_COURIER;
	os->FontSize = 12;
	os->Justification = PRT_JST_LEFT;
	os->Flags = 0;
	os->LinesPerInch = 6.0;
	os->RelativeX = 0.0;
	os->RelativeY = 0.0;
	os->Height = 1.0;
	os->Width = 0.0;
	os->Magic = MGK_PRTOBJSTRM;
	os->Driver = PRT_INF.LayoutDrivers[type];
	if (os->Driver) os->Driver->Init(os);
#if 0
	switch(type)
	    {
	    case PRT_OS_T_STRING:	os->String.Length = 0;
	    				os->String.Text = NULL;
					break;
	    case PRT_OS_T_TABLE:	os->Table.Rows = NULL;
	    				os->Table.HdrRow = NULL;
					break;
	    case PRT_OS_T_TABLEROW:	os->TableRow.Columns = NULL;
	    				break;
	    case PRT_OS_T_TABLECELL:	os->TableCell.Content = NULL;
	    				break;
	    case PRT_OS_T_SECTION:	os->Section.InitialStream = NULL;
	    				os->Section.nColumns = 1;
					os->Section.Streams[0] = NULL;
					os->Section.Header = NULL;
					break;
	    case PRT_OS_T_AREA:		os->Area.Content = NULL;
	    				break;
	    case PRT_OS_T_PICTURE:	os->Picture.Data = NULL;
	    				break;
	    }
#endif 
    return os;
    }


/*** prt_internal_FreeOS - free an objstream element.
 ***/
int
prt_internal_FreeOS(pPrtObjStream element)
    {

#if 00
	ASSERTMAGIC(element, MGK_PRTOBJSTRM)
	if (element->Type == PRT_OS_T_STRING && element->String.Text) nmSysFree(element->String.Text);
	if (element->Type == PRT_OS_T_PICTURE && element->Picture.Data) nmSysFree(element->Picture.Data);
	nmFree(element, sizeof(PrtObjStream));
#endif

    return 0;
    }


/*** prt_internal_FreeStream - free a stream of objstream elements
 ***/
int
prt_internal_FreeStream(pPrtObjStream element_stream)
    {
    pPrtObjStream del;

	while(element_stream)
	    {
	    ASSERTMAGIC(element_stream, MGK_PRTOBJSTRM)

    	    /** Free sub-streams? **/
#if 00
	    switch(element_stream->Type)
	        {
	        case PRT_OS_T_TABLE:	prt_internal_FreeStream(element_stream->Table.Rows);
	    				prt_internal_FreeStream(element_stream->Table.HdrRow);
					break;
	        case PRT_OS_T_TABLEROW:	prt_internal_FreeStream(element_stream->TableRow.Columns);
	    				break;
	        case PRT_OS_T_TABLECELL:prt_internal_FreeStream(element_stream->TableCell.Content);
	    				break;
	        case PRT_OS_T_SECTION:	prt_internal_FreeStream(element_stream->Section.InitialStream);
	    				prt_internal_FreeStream(element_stream->Section.Header);
					break;
	        case PRT_OS_T_AREA:	prt_internal_FreeStream(element_stream->Area.Content);
	    				break;
	        default:		break;
	        }
#endif
	    /** Free this element, and finish up. **/
	    del = element_stream;
	    element_stream = element_stream->Next;
	    prt_internal_FreeOS(del);
	    }

    return 0;
    }


/*** prt_internal_CopyOS - create a copy of an objstream element.  This
 *** routine also makes duplicates of allocated regions like String.Text
 *** but does NOT duplicate the rest of the stream or any substreams.
 *** Thus, the misuse of such a duplicated element could cause some
 *** inconsistencies in the tree structure.
 ***/
pPrtObjStream
prt_internal_CopyOS(pPrtObjStream element)
    {
    pPrtObjStream new_element;

	ASSERTMAGIC(element, MGK_PRTOBJSTRM)

    	/** Allocate the new element **/
	new_element = (pPrtObjStream)nmMalloc(sizeof(PrtObjStream));
	if (!new_element) return NULL;

	/** Do the basic copy operation **/
	memcpy(new_element, element, sizeof(PrtObjStream));

	/** Undo any existing linkages **/
	new_element->Next = NULL;
	new_element->Prev = NULL;
	new_element->YNext = NULL;
	new_element->YPrev = NULL;
	new_element->Parent = NULL;
	new_element->Head = NULL;

	/** Duplicate memory, such as string.text and picture.data **/
	if (new_element->Type == PRT_OS_T_STRING)
	    {
	    new_element->String.Text = nmSysStrdup(element->String.Text);
	    }
	else if (new_element->Type == PRT_OS_T_PICTURE)
	    {
	    new_element->Picture.Data = nmSysMalloc(element->Picture.DataLength);
	    memcpy(new_element->Picture.Data, element->Picture.Data, element->Picture.DataLength);
	    }

    return new_element;
    }


/*** prt_internal_CopyStream - create a copy of an entire objstream,
 *** including any substreams.  The resulting tree structure is guaranteed
 *** to be consistent and useable in all regards.
 ***/
pPrtObjStream
prt_internal_CopyStream(pPrtObjStream stream, pPrtObjStream new_parent, pPrtObjStream* new_head)
    {
    pPrtObjStream new_stream=NULL,tmp,prev=NULL;
    pPrtObjStream s1, s2;
    int i;

    	/** Copy entire stream... **/
	while(stream)
	    {
	    stream = xqGetStruct(qptr,Seq,pPrtObjStream);
	    ASSERTMAGIC(stream, MGK_PRTOBJSTRM)

	    /** Copy the element **/
	    tmp = prt_internal_CopyOS(stream);
	    if (!new_stream) new_stream = tmp;

	    /** Unlink it from Y-ordering **/
	    tmp->YPrev = NULL;
	    tmp->YNext = NULL;

	    /** Link in with new stream **/
	    tmp->Prev = prev;
	    if (prev) prev->Next = tmp;
	    tmp->Next = NULL;
	    tmp->Parent = new_parent;
	    tmp->Head = new_head;

	    /** Copy substreams **/
	    switch(stream->Type)
	        {
		case PRT_OS_T_TABLE:	tmp->Table.Rows = prt_internal_CopyStream(stream->Table.Rows, tmp, &(tmp->Table.Rows));
					tmp->Table.HdrRow = prt_internal_CopyStream(stream->Table.HdrRow, tmp, &(tmp->Table.HdrRow));
					break;
		case PRT_OS_T_TABLEROW:	tmp->TableRow.Columns = prt_internal_CopyStream(stream->TableRow.Columns, tmp, &(tmp->TableRow.Columns));
					break;
		case PRT_OS_T_TABLECELL:tmp->TableCell.Content = prt_internal_CopyStream(stream->TableCell.Content, tmp, &(tmp->TableCell.Content));
					break;
		case PRT_OS_T_SECTION:	tmp->Section.InitialStream = prt_internal_CopyStream(stream->Section.InitialStream,tmp, &(tmp->Section.InitialStream));
					tmp->Section.Header = prt_internal_CopyStream(stream->Section.Header, tmp, &(tmp->Section.Header));
					break;
		case PRT_OS_T_AREA:	tmp->Area.Content = prt_internal_CopyStream(stream->Area.Content, tmp, &(tmp->Area.Content));
					break;
		case PRT_OS_T_PAGE:	tmp->Page.Content = prt_internal_CopyStream(stream->Page.Content, tmp, &(tmp->Page.Content));
					break;
		}

	    /** If a section, update section Streams pointers **/
	    if (stream->Type == PRT_OS_T_SECTION)
	        {
		s1 = stream->Section.InitialStream;
		s2 = tmp->Section.InitialStream;
		while(s1)
		    {
		    for (i=0;i<stream->Section.nColumns;i++)
		        {
			if (stream->Section.Streams[i] == s1) tmp->Section.Streams[i] = s2;
			}
		    s1 = s1->Next;
		    s2 = s2->Next;
		    }
		}

	    prev=tmp;
	    stream = stream->Next;
	    }

    return new_stream;
    }


/*** prt_internal_CopyStyle - copies style information from one element to
 *** another (usually becaue the latter is going to be appended onto the
 *** objstream), including setting the RelativeX and RelativeY.
 ***/
int
prt_internal_CopyStyle(pPrtObjStream src, pPrtObjStream dst)
    {

    	/** Copy style data **/
	dst->Attributes = src->Attributes;
	dst->FGColor = src->FGColor;
	dst->BGColor = src->BGColor;
	dst->Font = src->Font;
	dst->FontSize = src->FontSize;
	dst->Justification = src->Justification;
	dst->LinesPerInch = src->LinesPerInch;
	dst->RelativeY = src->RelativeY;
	dst->RelativeX = src->RelativeX + src->Width;
	if (dst->Type == PRT_OS_T_STRING) dst->Height = 6.0/dst->LinesPerInch;

    return 0;
    }


/*** prt_internal_AddOS - add an objstream element to an existing stream
 *** after the given element.  The streamhead is specified in the event
 *** that 'stream' is NULL, indicating that the stream is empty, or if 
 *** inserting at the stream head.
 ***/
int
prt_internal_AddOS(pPrtObjStream parent, pPrtObjStream* streamhead, pPrtObjStream stream, pPrtObjStream new_element)
    {
    pXQueue q;

	ASSERTMAGIC(new_element, MGK_PRTOBJSTRM)

    	/** Link in to the next/prev **/
	if (!streamhead) streamhead = stream->Head;

	if (!stream) q=streamhead; else q = &(stream->Seq);
	xqAddAfter(q,&new_element->Seq);
	new_element->Parent = parent;

    return 0;
    }


/*** prt_internal_RemoveOS - removes an objstream element from an existing
 *** stream.  The streamhead is supplied so that if the element is first
 *** in the stream, the streamhead can be updated.
 ***/
int
prt_internal_RemoveOS(pPrtObjStream rm_element)
    {

	ASSERTMAGIC(rm_element, MGK_PRTOBJSTRM)

    	/** Remove from Ynext/Yprev? **/
	xqRemove(&rm_element->YSeq);

	/** Remove from normal objstream? **/
	xqRemove(&rm_element->Seq);
	rm_element->Parent = NULL;

    return 0;
    }


/*** prt_internal_AllocSession - allocate a new print session structure and
 *** initialize it to defaults.
 ***/
pPrtSession
prt_internal_AllocSession()
    {
    pPrtSession this;
    pPrtObjStream page, string;

    	/** Alloc the thing and fill in some default values **/
	this = (pPrtSession)nmMalloc(sizeof(PrtSession));
	if (!this) return NULL;
	memset(this,0,sizeof(PrtSession));
	this->Magic = MGK_PRTOBJSSN;
	page = prt_internal_AllocOS(PRT_OS_T_PAGE);
	string = prt_internal_AllocOS(PRT_OS_T_STRING);
	this->ObjStreamHead = page;
	this->ObjStreamPtr = string;
	this->PageNum = 1;
	this->PageWidth = 80.0;
	this->PageHeight = 60.0;
	page->Width = this->PageWidth;
	page->Height = this->PageHeight;
	page->ColWidth = this->PageWidth;
	page->LMargin = 0.0;
	page->RMargin = 0.0;
	page->ColSep = 0.0;
	string->Width = 0.0;
	string->RelativeX = 0.0;
	string->Height = 1.0;
	string->RelativeY = 0.0;
	prt_internal_AddOS(page, &(page->Content), NULL, string);

    return this;
    }


/*** prt_internal_FreeSession - release a print session.
 ***/
int
prt_internal_FreeSession(pPrtSession this)
    {

    	ASSERTMAGIC(this, MGK_PRTOBJSSN)

    	/** Free the objstream for the session **/
	if (this->ObjStreamHead) obj_internal_FreeStream(this->ObjStreamHead);

	/** Release the session's structure **/
	nmFree(this, sizeof(PrtSession));

    return 0;
    }


/*** prt_internal_DumpStream_r - the recursive component of the
 *** dump-objstream operation.
 ***/
int
prt_internal_DumpStream_r(pPrtObjStream stream, int indent)
    {
    static char* typenames[] = { "", "String","Table","TableRow","TableCell","Section","Area","Picture","Page"};
    static char* fontnames[] = { "courier","times","helvetica" };
    static char* justnames[] = { "left","center","right","full" };
    static char* colormodenames[] = { "binary/1bpp","grey/8bpp","color/8bpp","color/24bpp" };
    pPrtObjStream substream;
    int i;

    	/** Print the basic information... **/
	while(stream->Seq.Next != stream->Seq.Head)
	    {
	    stream = xqGetStruct(stream->Seq.Next,Seq,pPrtObjStream);
	    if (stream->Type < 1 || stream->Type > 7 || stream->Font < 0 || stream->Font > 2 ||
	        stream->Justification < 0 || stream->Justification > 3 || stream->Magic != MGK_PRTOBJSTRM)
		{
		printf("%*sERR - INVALID %8.8X\n",indent,"",stream);
		return 0;
		}
	    printf("%*s%9.9s  (%5.1lf,%5.1lf) %5.1lfx%5.1lf A=%2.2X C=%6.6X F=%s S=%d J=%s F=%2.2X H=%5.1lf",
	        indent,"",
		typenames[stream->Type],
		stream->RelativeX, stream->RelativeY,
		stream->Width, stream->Height,
		stream->Attributes, stream->FGColor, fontnames[stream->Font], stream->FontSize,
		justnames[stream->Justification], stream->Flags, stream->LinesPerInch);

	    switch(stream->Type)
	        {
		case PRT_OS_T_STRING: 	printf("\n%*s    Text='%s', Length=%d\n", 
						indent,"", 
						stream->String.Text, 
						stream->String.Length);
				     	break;

		case PRT_OS_T_TABLE:	printf(" nCols=%d, CS=%5.1lf\n%*s    Table Header Rows....\n",
						indent,"",
						stream->Table.nColumns,
						stream->Table.ColSep);
					substream = stream->Table.HdrRow;
					while(substream)
					    {
					    prt_internal_DumpStream_r(substream, indent+8);
					    substream = substream->Next;
					    }
					printf("%*s    Table Body Rows....\n", indent,"");
					substream = stream->Table.Rows;
					while(substream)
					    {
					    prt_internal_DumpStream_r(substream, indent+8);
					    substream = substream->Next;
					    }
					break;

		case PRT_OS_T_TABLEROW:	printf("\n");
					substream = stream->TableRow.Columns;
					while(substream)
					    {
					    prt_internal_DumpStream_r(substream, indent+4);
					    substream = substream->Next;
					    }
					break;

		case PRT_OS_T_TABLECELL:printf(" span=%d id=%d\n", stream->TableCell.ColSpan, stream->TableCell.ColID);
					substream = stream->TableCell.Content;
					while(substream)
					    {
					    prt_internal_DumpStream_r(substream, indent+4);
					    substream = substream->Next;
					    }
					break;
					
		case PRT_OS_T_SECTION:	printf(" LM=%5.1lf RM=%5.1lf nCols=%d CS=%5.1lf CW=%5.1lf\n",
						stream->Section.LMargin, stream->Section.RMargin,
						stream->Section.nColumns, stream->Section.ColSep,
						stream->Section.ColWidth);
					substream = stream->Section.InitialStream;
					while(substream)
					    {
					    for(i=0;i<stream->nColumns;i++)
					        {
						if (substream == stream->Section.Streams[i])
						    {
						    printf("%*s    Beginning of column #%d\n",indent,"",i+1);
						    break;
						    }
						}
					    if (substream == stream->Section.Header)
					        printf("%*s    Section Header:\n",indent,"");
					    prt_internal_DumpStream_r(substream, indent+8);
					    substream = substream->Next;
					    }
					break;
					
		case PRT_OS_T_AREA:	printf(" OFS=%5.1lf\n",stream->Area.TextOffset);
					substream = stream->Area.Content;
					while(substream)
					    {
					    prt_internal_DumpStream_r(substream, indent+4);
					    substream = substream->Next;
					    }
					break;
					
		case PRT_OS_T_PICTURE:	printf("\n%*s    VW=%5.1lf VH=%5.1lf PW=%d PH=%d DPW=%d DPH=%d\n",
						indent,"",
						stream->Picture.VisualWidth, stream->Picture.VisualHeight,
						stream->Picture.PixelWidth, stream->Picture.PixelHeight,
						stream->Picture.DataPixelWidth, stream->Picture.DataPixelHeight);
					printf("%*s    CM=%s DL=%d F=%2.2X\n",
						indent,"",
						colormodenames[stream->Picture.ColorMode],
						stream->Picture.DataLength, stream->Picture.Flags);
					break;
		}
	    }

    return 0;
    }


/*** prtDumpStream - prints the contents of an objstream on the
 *** standard output for debugging purposes.
 ***/
int
prtDumpStream(pPrtObjStream stream)
    {
    return prt_internal_DumpStream_r(stream,0);
    }


/*** prtRegisterPrintDriver - register an output content type driver for the
 *** print formatting system. 
 ***/
int
prtRegisterPrintDriver(pPrintDriver drv)
    {
    xaAddItem(&PRT_INF.Drivers, (void*)drv);
    return 0;
    }


/*** prtInitialize - init the printing subsystem
 ***/
int
prtInitialize()
    {

	/** Setup the globals **/
	memset(&PRT_INF, 0, sizeof(PRT_INF));
    	xaInit(&PRT_INF.Drivers, 16);

	/** Call the layout driver initialization routines **/

    return 0;
    }
