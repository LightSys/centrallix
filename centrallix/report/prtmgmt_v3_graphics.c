#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <errno.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_PNG_H
#include <png.h>
#endif
#include <setjmp.h>
#include "barcode.h"
#include "report.h"
#include "mtask.h"
#include "magic.h"
#include "xarray.h"
#include "xstring.h"
#include "prtmgmt_v3.h"
#include "htmlparse.h"
#include "mtsession.h"

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
/* Module:	prtmgmt_v3_graphics.c                                   */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	March 17, 2003                                          */
/*									*/
/* Description:	This module implements the graphics routines in the	*/
/*		print formatting subsystem, including a couple of API	*/
/*		level calls.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_graphics.c,v 1.2 2003/03/19 18:24:40 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_graphics.c,v $

    $Log: prtmgmt_v3_graphics.c,v $
    Revision 1.2  2003/03/19 18:24:40  gbeeley
    Added simple greyscale support via matrix dithering.

    Revision 1.1  2003/03/18 04:06:25  gbeeley
    Added basic image (picture/bitmap) support; only PNG images supported
    at present.  Moved image and border (rectangles) functionality into a
    new file prtmgmt_v3_graphics.c.  Graphics are only monochrome at the
    present and work only under PCL (not plain text!!!).  PNG support is
    via libpng, so libpng was added to configure/autoconf.

 **END-CVSDATA***********************************************************/


typedef struct
    {
    int		(*read_fn)();
    void*	read_arg;
    }
    PrtPngReadInfo, *pPrtPngReadInfo;

/*** prtAllocBorder - this is a convenience function to allocate a new
 *** border descriptor.
 ***/
pPrtBorder
prtAllocBorder(int n_lines, double sep, double pad, ...)
    {
    va_list va;
    pPrtBorder b;
    int i;

	/** Make sure caller didn't ask for too many border lines **/
	if (n_lines > PRT_MAXBDR)
	    {
	    mssError(1,"PRT","Too many lines (%d) requested in border.  Max is %d.",
		    n_lines, PRT_MAXBDR);
	    return NULL;
	    }

	/** Allocate the thing **/
	b = (pPrtBorder)nmMalloc(sizeof(PrtBorder));
	if (!b) return NULL;
	b->nLines = n_lines;
	b->Sep = sep;
	b->Pad = pad;
	b->TotalWidth = pad;

	/** Get the params for each line **/
	va_start(va, pad);
	for(i=0; i<n_lines; i++)
	    {
	    b->Width[i] = va_arg(va, double);
	    b->Color[i] = va_arg(va, int);
	    if (i>0) b->TotalWidth += b->Sep;
	    b->TotalWidth += b->Width[i];
	    }
	va_end(va);

    return b;
    }


/*** prtFreeBorder() - reverse of the above.
 ***/
int
prtFreeBorder(pPrtBorder b)
    {
    nmFree(b, sizeof(PrtBorder));
    return 0;
    }


/*** prt_internal_MakeBorder() - creates rectangles to represent the
 *** given border data, and adds them to the parent container at the
 *** requested location with the given size.
 ***
 *** Params:
 ***	parent	- the object to add the borders within.
 ***	x	- the X coordinate of the start of the border
 ***	y	- the Y coordinate of the start of the border
 ***	len	- the length of the border line
 ***	flags	- PRT_MKBDR_F_xxx flags bitmask
 ***	b	- border data for *this* border, must be set.
 ***	sb	- border that this one connects to at starting point, NULL if none.
 ***	eb	- border that this one connects to at ending point, NULL if none.
 ***/
int
prt_internal_MakeBorder(pPrtObjStream parent, double x, double y, double len, int flags, pPrtBorder b, pPrtBorder sb, pPrtBorder eb)
    {
    pPrtObjStream rect_obj;
    int i, selected_line;
    int offset_dir;
    int is_horiz;
    double total_s_thickness, total_e_thickness, total_thickness;
    double s_thickness, e_thickness, thickness;

	/** Which 'direction' (as in positive or negative) **/
	if ((flags & PRT_MKBDR_DIRFLAGS) == PRT_MKBDR_F_TOP ||
	    (flags & PRT_MKBDR_DIRFLAGS) == PRT_MKBDR_F_LEFT)
	    offset_dir = +1;
	else if ((flags & PRT_MKBDR_DIRFLAGS) == PRT_MKBDR_F_BOTTOM ||
	    (flags & PRT_MKBDR_DIRFLAGS) == PRT_MKBDR_F_RIGHT)
	    offset_dir = -1;
	else
	    offset_dir = 0;

	/** Which 'orientation' (vertical vs. horizontal) **/
	if (flags & (PRT_MKBDR_F_TOP | PRT_MKBDR_F_BOTTOM))
	    is_horiz = 1;
	else
	    is_horiz = 0;

	/** Figure total thickness of border **/
	total_thickness = b->Pad;
	total_e_thickness = eb?(eb->Pad):0.0;
	total_s_thickness = sb?(sb->Pad):0.0;
	for(i=0; i < b->nLines; i++)
	    {
	    if (offset_dir != 0) selected_line = i;
	    else selected_line = ((b->nLines-1)-i/2);
	    if (i > 0) total_thickness += b->Sep;
	    total_thickness += b->Width[selected_line];
	    if (sb && sb->nLines > i+1) total_s_thickness += (sb->Sep + sb->Width[i]);
	    if (eb && eb->nLines > i+1) total_e_thickness += (eb->Sep + eb->Width[i]);
	    }

	/** Add number of requested line objects **/
	thickness = b->Pad;
	e_thickness = eb?(eb->Pad):0.0;
	s_thickness = sb?(sb->Pad):0.0;
	for(i=0; i < b->nLines; i++)
	    {
	    /** Get a new rectangle **/
	    rect_obj = prt_internal_AllocObjByID(PRT_OBJ_T_RECT);
	    if (!rect_obj) return -ENOMEM;
	    rect_obj->TextStyle.Color = rect_obj->FGColor = b->Color[i];
	    if (flags & PRT_MKBDR_F_MARGINRELEASE) rect_obj->Flags |= PRT_OBJ_F_MARGINRELEASE;

	    /** Set up its geometry **/
	    if (offset_dir != 0) selected_line = i;
	    else selected_line = ((b->nLines-1)-i/2);
	    if (is_horiz)
		{
		rect_obj->Height = b->Width[selected_line]*PRT_XY_CORRECTION_FACTOR;
		if (offset_dir != 0) 
		    {
		    rect_obj->Width = len - s_thickness - e_thickness;
		    rect_obj->X = x + s_thickness;
		    }
		else 
		    {
		    rect_obj->Width = len - total_s_thickness - total_e_thickness;
		    rect_obj->X = x + total_s_thickness;
		    }
		rect_obj->Y = y - ((offset_dir==0)*(total_thickness/2.0) + (offset_dir==-1)*(b->Width[selected_line]) - (offset_dir!=-1)*thickness + (offset_dir==-1)*thickness)*PRT_XY_CORRECTION_FACTOR;
		}
	    else
		{
		rect_obj->Width = b->Width[selected_line];
		if (offset_dir != 0) 
		    {
		    rect_obj->Height = len - (s_thickness + e_thickness)*PRT_XY_CORRECTION_FACTOR;
		    rect_obj->Y = y + (s_thickness*PRT_XY_CORRECTION_FACTOR);
		    }
		else
		    {
		    rect_obj->Height = len - (total_s_thickness + total_e_thickness)*PRT_XY_CORRECTION_FACTOR;
		    rect_obj->Y = y + (total_s_thickness*PRT_XY_CORRECTION_FACTOR);
		    }
		rect_obj->X = x - (offset_dir==0)*(total_thickness/2.0) - (offset_dir==-1)*(b->Width[selected_line]) + (offset_dir!=-1)*thickness - (offset_dir==-1)*thickness;
		}

	    /** Add the rectangle **/
	    prt_internal_Add(parent, rect_obj);

	    /** Account for our own line thickness **/
	    thickness += (b->Sep + b->Width[selected_line]);

	    /** Account for thickness of connecting border lines **/
	    if (sb && sb->nLines > i+1) s_thickness += (sb->Sep + sb->Width[i]);
	    if (eb && eb->nLines > i+1) e_thickness += (eb->Sep + eb->Width[i]);
	    }

    return 0;
    }



#if defined(HAVE_PNG_H) && defined(HAVE_LIBPNG)
/*** prt_png_Malloc() - replacement malloc function for libpng which
 *** use the newmalloc library
 ***/
void*
prt_png_Malloc(png_structp png_ptr, png_uint_32 size)
    {
    return nmSysMalloc((int)size);
    }

void
prt_png_Free(png_structp png_ptr, voidp ptr)
    {
    nmSysFree((void*)ptr);
    return;
    }

void
prt_png_Read(png_structp png_ptr, png_bytep data, png_size_t length)
    {
    pPrtPngReadInfo read_info = png_get_io_ptr(png_ptr);

        if (read_info->read_fn(read_info->read_arg, (char*)data, (int)length, 0, FD_U_PACKET) < (int)length)
	    {
	    png_error(png_ptr, (png_const_charp)"Failed to PNG image data from Object/File");
	    }

    return;
    }

#endif


/*** prtCreateImageFromPNG() - creates a new image from PNG file data,
 *** read from an arbitrary location (pObject, pFile, XString, etc).
 ***/
pPrtImage
prtCreateImageFromPNG(int (*read_fn)(), void* read_arg)
    {
#if defined(HAVE_PNG_H) && defined(HAVE_LIBPNG)
    pPrtImage img;
    char ckbuf[8];
    png_structp libpng_png_ptr;
    png_infop libpng_info_ptr;
    png_infop libpng_end_ptr;
    PrtPngReadInfo read_info;
    png_uint_32 width, height;
    int bit_depth, color_type;
    png_bytep *row_pointers;
    int bytes_per_row;
    int i;
    int our_color_type;

	/** See if the file is a PNG file at all **/
	if (read_fn(read_arg, ckbuf, sizeof(ckbuf), 0, FD_U_PACKET) < 8)
	    {
	    mssError(0,"PRT","Could not read PNG image data");
	    return NULL;
	    }
	if (png_sig_cmp(ckbuf, 0, sizeof(ckbuf)) != 0)
	    {
	    mssError(1,"PRT","Object/File does not appear to contain PNG image data");
	    return NULL;
	    }

	/** Setup to do some business with libpng **/
	/*libpng_png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, 
		NULL, NULL, NULL, NULL, prt_png_Malloc, prt_png_Free);*/
	libpng_png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (!libpng_png_ptr)
	    {
	    mssError(1,"PRT","Bark!  libpng error: could not create_read_struct()");
	    /*mssError(1,"PRT","Bark!  libpng error: could not create_read_struct_2()");*/
	    return NULL;
	    }
	/*png_set_mem_fn(libpng_png_ptr, NULL, prt_png_Malloc, prt_png_Free);*/
	read_info.read_fn = read_fn;
	read_info.read_arg = read_arg;
	png_set_read_fn(libpng_png_ptr, (void*)&read_info, prt_png_Read);
	libpng_info_ptr = png_create_info_struct(libpng_png_ptr);
	if (!libpng_info_ptr)
	    {
	    mssError(1,"PRT","Bark!  libpng error: could not create_info_struct()");
	    png_destroy_read_struct(&libpng_png_ptr, NULL, NULL);
	    return NULL;
	    }
	libpng_end_ptr = png_create_info_struct(libpng_png_ptr);
	if (!libpng_end_ptr)
	    {
	    mssError(1,"PRT","Bark!  libpng error: could not create_info_struct()");
	    png_destroy_read_struct(&libpng_png_ptr, &libpng_info_ptr, NULL);
	    return NULL;
	    }
	png_set_sig_bytes(libpng_png_ptr, sizeof(ckbuf));

	/** Handle error return... (e.g., like a Catch() routine in Catch/Throw) **/
	if (setjmp(png_jmpbuf(libpng_png_ptr)))
	    {
	    mssError(1,"PRT","Error reading PNG image data - inaccessible or corrupted data?");
	    png_destroy_read_struct(&libpng_png_ptr, &libpng_info_ptr, &libpng_end_ptr);
	    return NULL;
	    }

	/** Read the PNG header **/
	png_read_info(libpng_png_ptr, libpng_info_ptr);
	png_get_IHDR(libpng_png_ptr, libpng_info_ptr, &width, &height, &bit_depth, &color_type, 
		NULL, NULL, NULL);
	if (bit_depth > 1 && bit_depth < 8 && color_type == PNG_COLOR_TYPE_GRAY)
	    {
	    png_set_gray_1_2_4_to_8(libpng_png_ptr);
	    bit_depth=8;
	    }
	png_set_strip_16(libpng_png_ptr);
	if (bit_depth == 16) bit_depth = 8;
	png_set_strip_alpha(libpng_png_ptr);
	color_type &= ~PNG_COLOR_MASK_ALPHA;
	png_set_swap(libpng_png_ptr);
	png_set_bgr(libpng_png_ptr);
	png_set_packswap(libpng_png_ptr);
	png_set_palette_to_rgb(libpng_png_ptr);
	if (bit_depth == 8 && color_type == PNG_COLOR_TYPE_RGB) 
	    png_set_filler(libpng_png_ptr, 0, PNG_FILLER_AFTER);

	/** Allocate row pointers for image data **/
	if (bit_depth == 1 && color_type == PNG_COLOR_TYPE_GRAY)
	    {
	    bytes_per_row = (width+7)/8;
	    our_color_type = PRT_COLOR_T_MONO;
	    }
	else if (bit_depth == 8 && (color_type == PNG_COLOR_TYPE_RGB))
	    {
	    bytes_per_row = width*4;
	    our_color_type = PRT_COLOR_T_FULL;
	    }
	else if (bit_depth == 8 && color_type == PNG_COLOR_TYPE_GRAY)
	    {
	    bytes_per_row = width;
	    our_color_type = PRT_COLOR_T_GREY;
	    }
	else
	    {
	    mssError(1,"PRT","Invalid color type / bit depth for image");
	    png_destroy_read_struct(&libpng_png_ptr, &libpng_info_ptr, &libpng_end_ptr);
	    return NULL;
	    }
	row_pointers = (png_bytep*)nmSysMalloc(height*sizeof(png_bytep));

	/** Allocate our image header **/
	img = (pPrtImage)nmSysMalloc(sizeof(PrtImageHdr) + bytes_per_row*height);
	for(i=0;i<height;i++)
	    {
	    row_pointers[i] = img->Data.Byte + i*bytes_per_row;
	    }
	img->Hdr.Width = width;
	img->Hdr.Height = height;
	img->Hdr.ColorMode = our_color_type;
	img->Hdr.DataLength = bytes_per_row*height;
	img->Hdr.YOffset = 0.0;

	/** Read the image data **/
	png_read_image(libpng_png_ptr, row_pointers);

	/** All done... **/
	png_read_end(libpng_png_ptr, libpng_end_ptr);
	png_destroy_read_struct(&libpng_png_ptr, &libpng_info_ptr, &libpng_end_ptr);

    return img;
#else
    mssError(1,"PRT","PNG image support not available; cannot use prtCreateImageFromPNG()");
    return NULL;
#endif
    }


/*** prtFreeImage() - frees memory used by an image.
 ***/
int
prtFreeImage(pPrtImage i)
    {
    nmSysFree(i);
    return 0;
    }


/*** prtImageSize() - returns the total memory used by an image.
 ***/
int
prtImageSize(pPrtImage i)
    {
    return sizeof(PrtImageHdr) + i->Hdr.DataLength;
    }


/*** prtWriteImage() - output an image into a container
 ***/
int
prtWriteImage(int handle_id, pPrtImage imgdata, double x, double y, double width, double height, int flags)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream image_obj;
    int rval;

	/** check **/
	if (!obj || !imgdata) return -1;
	ASSERTMAGIC(obj,MGK_PRTOBJSTRM);

	/** build a new image object **/
	image_obj = prt_internal_AllocObjByID(PRT_OBJ_T_IMAGE);
	if (!image_obj) return -ENOMEM;
	prt_internal_CopyAttrs(obj, image_obj);
	image_obj->Flags = flags & PRT_OBJ_UFLAGMASK;
	image_obj->X = x;
	image_obj->Y = y;
	image_obj->Width = width;
	image_obj->Height = height;
	image_obj->ConfigWidth = width;
	image_obj->ConfigHeight = height;
	image_obj->Content = (void*)imgdata;
	image_obj->ContentSize = prtImageSize(imgdata);

	/** Add it **/
	rval = obj->LayoutMgr->AddObject(obj, image_obj);

    return rval;
    }


/*** prt_internal_GetPixel() - returns the color value of a given point
 *** in the image.  It takes relative coordinates in the range of 
 *** 0.0<=xoffset<1.0 and 0.0<=yoffset<1.0, where (0.0,0.0) is the upper
 *** left corner of the image.  Returns the RGB color value as 0x00RRGGBB.
 ***/
int
prt_internal_GetPixel(pPrtImage img, double xoffset, double yoffset)
    {
    int color,x,y,bit,datawidth;

	/** Find the real X and Y in the image **/
	x = xoffset*img->Hdr.Width;
	y = yoffset*img->Hdr.Height;
	if (x < 0) x = 0;
	else if (x >= img->Hdr.Width) x = img->Hdr.Width-1;
	if (y < 0) y = 0;
	else if (y >= img->Hdr.Height) y = img->Hdr.Height-1;

	/** Desired pixel location depends on color mode **/
	if (img->Hdr.ColorMode == PRT_COLOR_T_MONO)
	    {
	    bit = x&7;
	    x = (x>>3);
	    datawidth = (img->Hdr.Width+7)>>3;
	    color = (img->Data.Byte[y*datawidth + x]>>bit) & 0x01;
	    if (color) color=0x00FFFFFF;
	    }
	else if (img->Hdr.ColorMode == PRT_COLOR_T_GREY)
	    {
	    color = (img->Data.Byte[y*img->Hdr.Width + x]);
	    color = color | (color<<8) | (color<<16);
	    }
	else if (img->Hdr.ColorMode == PRT_COLOR_T_FULL)
	    {
	    color = (img->Data.Word[y*img->Hdr.Width + x] & 0x00FFFFFF);
	    }
	else
	    {
	    return -1;
	    }

    return color;
    }


