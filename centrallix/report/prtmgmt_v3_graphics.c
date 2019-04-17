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
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"
#ifdef HAVE_RSVG_H
#include "librsvg/rsvg.h"
#include "cairo-svg.h"
#include "cairo-ps.h"
#endif

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



typedef struct
    {
    int		(*io_fn)();
    void*	io_arg;
    }
    PrtIOInfo, *pPrtIOInfo;


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
	    prt_internal_CopyAttrs(parent,rect_obj);
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
    pPrtIOInfo read_info = png_get_io_ptr(png_ptr);

        if (read_info->io_fn(read_info->io_arg, (char*)data, (int)length, 0, FD_U_PACKET) < (int)length)
	    {
	    png_error(png_ptr, (png_const_charp)"Failed to read PNG image data from Object/File");
	    }

    return;
    }

void
prt_png_Write(png_structp png_ptr, png_bytep data, png_size_t length)
    {
    pPrtIOInfo write_info = png_get_io_ptr(png_ptr);

        if (write_info->io_fn(write_info->io_arg, (char*)data, (int)length, 0, FD_U_PACKET) < (int)length)
	    {
	    png_error(png_ptr, (png_const_charp)"Failed to write PNG image data to Object/File");
	    }

    return;
    }

void
prt_png_Flush(png_structp png_ptr)
    {
    /*pPrtIOInfo write_info = png_get_io_ptr(png_ptr);*/
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
    unsigned char ckbuf[8];
    png_structp libpng_png_ptr;
    png_infop libpng_info_ptr;
    png_infop libpng_end_ptr;
    PrtIOInfo read_info;
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
	read_info.io_fn = read_fn;
	read_info.io_arg = read_arg;
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
	    png_set_expand_gray_1_2_4_to_8(libpng_png_ptr);
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
	prt_internal_CopyAttrs((obj->ContentTail)?(obj->ContentTail):obj,image_obj);
	image_obj->Flags = flags & PRT_OBJ_UFLAGMASK;
	image_obj->X = x;
	image_obj->Y = y;
	image_obj->Width = width;
	image_obj->Height = height;
	image_obj->ConfigWidth = width;
	image_obj->ConfigHeight = height;
	image_obj->Content = (void*)imgdata;
	image_obj->ContentSize = prtImageSize(imgdata);
	image_obj->YBase = height;

	/** Add it **/
	rval = obj->LayoutMgr->AddObject(obj, image_obj);

    return rval;
    }


/*** prt_internal_GetPixelAntialias() - returns the color value of a given
 *** point with antialiasing enabled to improve the quality of scaling using
 *** this routine.
 ***/
int
prt_internal_GetPixelAntialias(pPrtImage img, double xoffset, double yoffset)
    {
    int color,x,y;
    double x1,x2,y1,y2,pw,ph;
    int color11, color12, color21, color22;
    int r11, r12, r21, r22;
    int g11, g12, g21, g22;
    int b11, b12, b21, b22;
    int r,g,b;

	/** Find the real X and Y in the image **/
	x = xoffset*img->Hdr.Width;
	y = yoffset*img->Hdr.Height;

	/** Find the actual locations of the affected pixels **/
	x1 = ((double)x)/img->Hdr.Width + 0.000001;
	y1 = ((double)y)/img->Hdr.Height + 0.000001;
	pw = 1.0/img->Hdr.Width;
	ph = 1.0/img->Hdr.Height;
	x2 = x1 + pw;
	y2 = y1 + ph;

	/** Get the four color values **/
	color11 = prt_internal_GetPixel(img,x1,y1); 
	r11=(color11>>16)&0xFF; g11=(color11>>8)&0xFF; b11=color11&0xFF;
	color12 = prt_internal_GetPixel(img,x1,y2);
	r12=(color12>>16)&0xFF; g12=(color12>>8)&0xFF; b12=color12&0xFF;
	color21 = prt_internal_GetPixel(img,x2,y1);
	r21=(color21>>16)&0xFF; g21=(color21>>8)&0xFF; b21=color21&0xFF;
	color22 = prt_internal_GetPixel(img,x2,y2);
	r22=(color22>>16)&0xFF; g22=(color22>>8)&0xFF; b22=color22&0xFF;

	/** Build the final color values **/
	r =  r11*(1.0 - ((xoffset-x1)/pw + (yoffset-y1)/ph)/2.0);
	r += r12*(1.0 - ((xoffset-x1)/pw + (y2-yoffset)/ph)/2.0);
	r += r21*(1.0 - ((x2-xoffset)/pw + (yoffset-y1)/ph)/2.0);
	r += r22*(1.0 - ((x2-xoffset)/pw + (y2-yoffset)/ph)/2.0);
	r /= 2;
	g =  g11*(1.0 - ((xoffset-x1)/pw + (yoffset-y1)/ph)/2.0);
	g += g12*(1.0 - ((xoffset-x1)/pw + (y2-yoffset)/ph)/2.0);
	g += g21*(1.0 - ((x2-xoffset)/pw + (yoffset-y1)/ph)/2.0);
	g += g22*(1.0 - ((x2-xoffset)/pw + (y2-yoffset)/ph)/2.0);
	g /= 2;
	b =  b11*(1.0 - ((xoffset-x1)/pw + (yoffset-y1)/ph)/2.0);
	b += b12*(1.0 - ((xoffset-x1)/pw + (y2-yoffset)/ph)/2.0);
	b += b21*(1.0 - ((x2-xoffset)/pw + (yoffset-y1)/ph)/2.0);
	b += b22*(1.0 - ((x2-xoffset)/pw + (y2-yoffset)/ph)/2.0);
	b /= 2;
	color = ((r&0xFF)<<16) + ((g&0xFF)<<8) + (b&0xFF);

    return color;
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
	x = (xoffset*(img->Hdr.Width) + 0.5);
	y = (yoffset*(img->Hdr.Height) + 0.5);
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


/*** prt_internal_GetPixelDirect() - gets a specific (x,y) pixel from the image
 ***/
int
prt_internal_GetPixelDirect(pPrtImage img, int x, int y)
    {
    return (img->Data.Word[y*img->Hdr.Width + x] & 0x00FFFFFF);
    }


/*** prt_internal_WriteImageToPNG() - outputs a PrtImage image structure to a
 *** given location as PNG image data with a given width and height in pixels.
 ***/
int
prt_internal_WriteImageToPNG(int (*write_fn)(), void* write_arg, pPrtImage img, int w, int h)
    {
#if defined(HAVE_PNG_H) && defined(HAVE_LIBPNG)
    png_structp libpng_png_ptr;
    png_infop libpng_info_ptr;
    PrtIOInfo write_info;
    int bitdepth, colortype;
    png_byte* row_pointer = NULL;
    int i,j;
    int bytes_per_row;
    int pixel;
    int color;

	/** width/height not specified? **/
	if (w == -1) w = img->Hdr.Width;
	if (h == -1) h = img->Hdr.Height;

	/** Setup libpng **/
	libpng_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (!libpng_png_ptr)
	    {
	    mssError(1,"PRT","Bark!  libpng error: could not create_write_struct()");
	    return -1;
	    }
	write_info.io_fn = write_fn;
	write_info.io_arg = write_arg;
	png_set_write_fn(libpng_png_ptr, (void*)&write_info, prt_png_Write, prt_png_Flush);
	libpng_info_ptr = png_create_info_struct(libpng_png_ptr);
	if (!libpng_info_ptr)
	    {
	    mssError(1,"PRT","Bark!  libpng error: could not create_info_struct()");
	    png_destroy_write_struct(&libpng_png_ptr, NULL);
	    return -1;
	    }

	/** Handle error return... (e.g., like a Catch() routine in Catch/Throw) **/
	if (setjmp(png_jmpbuf(libpng_png_ptr)))
	    {
	    mssError(1,"PRT","Error writing PNG image data");
	    if (row_pointer) nmSysFree(row_pointer);
	    png_destroy_write_struct(&libpng_png_ptr, &libpng_info_ptr);
	    return -1;
	    }

	/** Setup the PNG image header info **/
	if (img->Hdr.ColorMode == PRT_COLOR_T_MONO)
	    {
	    bytes_per_row = (w+7)/8;
	    bitdepth = 1;
	    }
	else
	    {
	    bitdepth = 8;
	    bytes_per_row = w;
	    }
	if (img->Hdr.ColorMode == PRT_COLOR_T_FULL)
	    {
	    colortype = PNG_COLOR_TYPE_RGB;
	    bytes_per_row *= 4;
	    }
	else
	    {
	    colortype = PNG_COLOR_TYPE_GRAY;
	    }
	png_set_IHDR(libpng_png_ptr, libpng_info_ptr, w, h, bitdepth, colortype,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(libpng_png_ptr, libpng_info_ptr);
	png_set_bgr(libpng_png_ptr);
	if (img->Hdr.ColorMode == PRT_COLOR_T_FULL) 
	    png_set_filler(libpng_png_ptr, 0, PNG_FILLER_AFTER);

	/** Allocate and setup row pointer **/
	row_pointer = (png_byte*)nmSysMalloc(bytes_per_row);
	
	/** Write each row **/
	for(i=0;i<h;i++)
	    {
	    memset(row_pointer, 0, bytes_per_row);
	    /** Setup the row's content **/
	    for(j=0;j<w;j++)
		{
		if (w == img->Hdr.Width && h == img->Hdr.Height)
		    pixel = prt_internal_GetPixel(img, ((double)j)/w, ((double)i)/h);
		else
		    pixel = prt_internal_GetPixelAntialias(img, ((double)j)/w, ((double)i)/h);
		switch(img->Hdr.ColorMode)
		    {
		    case PRT_COLOR_T_MONO:
			color = ((pixel&0xFF) + ((pixel>>8)&0xFF) + ((pixel>>16)&0xFF))/3;
			if (color >= 0x80) color=1; else color=0;
			if (color) row_pointer[j/8] |= 1<<(j&0x7);
			break;

		    case PRT_COLOR_T_GREY:
			row_pointer[j] = pixel&0xFF;
			break;

		    case PRT_COLOR_T_FULL:
			row_pointer[j*4] = pixel&0xFF;
			row_pointer[j*4+1] = (pixel>>8)&0xFF;
			row_pointer[j*4+2] = (pixel>>16)&0xFF;
			row_pointer[j*4+3] = (pixel>>24)&0xFF;
			break;
		    }
		}

	    /** Write the row **/
	    png_write_row(libpng_png_ptr, row_pointer);
	    }

	/** End the write **/
	png_write_end(libpng_png_ptr, libpng_info_ptr);
	png_destroy_write_struct(&libpng_png_ptr, &libpng_info_ptr);
	nmSysFree(row_pointer);

    return 0;
#else
    mssError(1,"PRT","PNG image support not available; cannot use prtWriteImageToPNG()");
    return -1;
#endif
    }


/*
 *  VECTOR IMAGE FUNCTIONS
 *  ______________________
 *
 */


#if defined(HAVE_RSVG_H) && defined(HAVE_LIBRSVG)
/*** svgSanityCheck() - checks whether a buffer contains valid svg data.
 ***/
int
svgSanityCheck(char* svg_data, int length)
    {
    RsvgHandle *rsvg;
    GError *rsvg_err;

    /* Attempt to load SVG data into rsvg handle */
    rsvg = rsvg_handle_new_from_data(svg_data, length, &rsvg_err);

    if (!rsvg) {
	mssError(0, "PRT", "SVG sanity check failed: %s", rsvg_err->message);
        g_object_unref(rsvg_err);
        return -1;    
    }

    g_object_unref(rsvg);
    return 0;
    }


/*** prtSvgSize() - returns the total memory used by an svg image.
 ***/
int
prtSvgSize(pPrtSvg svg)
    {
    return svg->SvgData->Length + sizeof(PrtSvg);
    }


/*** prtFreeSvg() - free memory used by an svg image.
 ***/
int
prtFreeSvg(pPrtSvg svg)
    {
    xsFree(svg->SvgData);
    nmFree(svg, sizeof(PrtSvg));
    
    return 0;
    }


/*** prt_svg_Write() - this function gets passed to CAIRO when
 *** outputting to a file.
 ***/
static cairo_status_t
prt_svg_Write(void* write_info, char* data, int length)
    {
    if (((PrtIOInfo *)write_info)->io_fn(((PrtIOInfo *)write_info)->io_arg, 
                                          data, length, 0, FD_U_PACKET) < 0)
        return CAIRO_STATUS_WRITE_ERROR;

    return CAIRO_STATUS_SUCCESS;
    }


/*** prt_svg_WriteXS() - this function gets passed to CAIRO when
 *** outputting to an XString.
 ***/
static cairo_status_t
prt_svg_WriteXS(void* xs, char* data, int length)
    {
    if (xsConcatenate((pXString)xs, data, length) < 0)
        return CAIRO_STATUS_WRITE_ERROR;
    
    return CAIRO_STATUS_SUCCESS;
    }
#endif


/*** prtWriteSvgToContainer() - add an svg image to a Centrallix container
 ***/
int
prtWriteSvgToContainer(int handle_id, pPrtSvg svg, double x, double y, 
		       double width, double height, int flags)
    {
#if defined(HAVE_RSVG_H) && defined(HAVE_LIBRSVG)
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream svg_obj;

    /* Allocate new (sub)object of type "svg" */
    svg_obj = prt_internal_AllocObjByID(PRT_OBJ_T_SVG);
    if (!svg_obj) return -ENOMEM;
    
    /* Copy attributes inherited from parent */
    if (obj->ContentTail) 
        prt_internal_CopyAttrs(obj->ContentTail, svg_obj);
    else 
        prt_internal_CopyAttrs(obj, svg_obj);

    /* Set other relevant attributes */
    svg_obj->Flags = flags & PRT_OBJ_UFLAGMASK;
    svg_obj->X = x;
    svg_obj->Y = y;
    svg_obj->Width = width;
    svg_obj->Height = height;
    svg_obj->ConfigWidth = width;
    svg_obj->Content = (void*)svg;
    svg_obj->ContentSize = prtSvgSize(svg);
    svg_obj->YBase = height;
    svg_obj->Finalize = prtFreeSvg;

    /* Add obj to layout manager */
    return obj->LayoutMgr->AddObject(obj, svg_obj);

#else
    mssError(1,"PRT","SVG image support not available");
    return -1;
#endif 
    }


/*** prtConvertSvgToEps() - convert an svg image (pPrtSvg) to encapsulated
 *** postscript. Returns an xstring with the eps data.     
 ***/
pXString prtConvertSvgToEps(pPrtSvg svg, double w, double h)
    {
#if defined(HAVE_RSVG_H) && defined(HAVE_LIBRSVG)
    RsvgHandle *rsvg;
    RsvgDimensionData dimensions;    
    pXString epsXString;
    cairo_surface_t *surface;
    cairo_t *cr;

    /* Init EPS string */
    epsXString = xsNew();
    if (!epsXString)
    {
        mssError(0, "PRT", "EPS data allocation error");
        return NULL;
    }

    /* Load SVG data into rsvg handle */
    rsvg = rsvg_handle_new_from_data(svg->SvgData->String, svg->SvgData->Length, NULL); 
    if (!rsvg)
    {
        mssError(0, "PRT", "Error reloading SVG data");
        xsFree(epsXString);
        return NULL;
    }

    /* Retrieve current dimensions */
    rsvg_handle_get_dimensions(rsvg, &dimensions);

    /* Set up cairo context */
    surface = cairo_ps_surface_create_for_stream((cairo_write_func_t)prt_svg_WriteXS, (void*)epsXString, w, h);
    cairo_ps_surface_set_eps(surface, TRUE);

    /* Resize image */
    cr = cairo_create(surface);
    cairo_scale(cr, (double)w / dimensions.width, (double)h / dimensions.height); 

    /* Render */
    if (!rsvg_handle_render_cairo(rsvg, cr))
    {
        mssError(0, "PRT", "Error rendering EPS image");
        goto error;
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(rsvg);
    return epsXString;

error:
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(rsvg);
    xsFree(epsXString);
    return NULL;

#else
    mssError(1,"PRT","SVG image support not available");
    return NULL;
#endif 
    }


/*** prtReadSvg() - reads an svg image from an arbitrary
 *** location (pObject, pFile, XString, etc) into Centrallix.
 ***/
pPrtSvg
prtReadSvg(int (*read_fn)(), void* read_arg)
    {
#if defined(HAVE_RSVG_H) && defined(HAVE_LIBRSVG)  
    pPrtSvg svg;
    pXString svgXString;

    int count;
    char buf[256];

    /* Init SVG string */
    svgXString = xsNew();
    if (!svgXString)
    {
	mssError(0, "PRT", "SVG data allocation error");
	return NULL;
    }

    /* Read SVG data */
    while ((count = read_fn(read_arg, buf, sizeof(buf), 0, 0))) 
    {
	if (count < 0) {
	    mssError(0, "PRT", "Error while reading SVG file");
	    goto error;
	}
	if (xsConcatenate(svgXString, buf, count) < 0) {
	    mssError(0, "PRT", "Error while reading SVG file");
	    goto error;
	}
    }

    /* SVG sanity check */
    if (svgSanityCheck(svgXString->String, svgXString->Length) < 0)
    {
	goto error;
    }

    /* Allocate SVG struct */
    svg = (pPrtSvg)nmMalloc(sizeof(PrtSvg));
    if (!svg)
    {
	mssError(0, "PRT", "SVG struct allocation error");
	goto error;
    }
    
    svg->SvgData = svgXString;
    return svg;

error:
    xsFree(svgXString);
    return NULL;

#else
    mssError(1,"PRT","SVG image support not available");
    return NULL;
#endif 
}


/*** prt_internal_WriteSvgToFile() - write svg image to a file, with
 *** given dimensions (scale to a given width and height).
 ***/
int
prt_internal_WriteSvgToFile(int (*write_fn)(), void* write_arg, pPrtSvg svg,
                            int w, int h)
    {
#if defined(HAVE_RSVG_H) && defined(HAVE_LIBRSVG)
    RsvgHandle *rsvg;
    RsvgDimensionData dimensions;
    PrtIOInfo write_info;
    cairo_surface_t *surface;
    cairo_t *cr;

    /* Load SVG data into rsvg handle */
    rsvg = rsvg_handle_new_from_data(svg->SvgData->String, svg->SvgData->Length, NULL);
    if (!rsvg)
    {
        mssError(0, "PRT", "Error reloading SVG data");
        return -1;
    }

    /* Retrieve current dimensions */
    rsvg_handle_get_dimensions(rsvg, &dimensions);

    /* Set up cairo context */
    write_info.io_fn = write_fn;
    write_info.io_arg = write_arg;
    surface = cairo_svg_surface_create_for_stream((cairo_write_func_t)prt_svg_Write, (void*)&write_info, w, h);
    cairo_svg_surface_set_document_unit(surface, CAIRO_SVG_UNIT_PX);

    /* Resize image */
    cr = cairo_create(surface);
    cairo_scale(cr, ((double)w) / dimensions.width, 
                    ((double)h) / dimensions.height); 

    /* Render */
    if (!rsvg_handle_render_cairo(rsvg, cr))
    {
        mssError(0, "PRT", "Error rendering SVG image");
        goto error;
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(rsvg);
    return 0;

error:
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(rsvg);
    return -1;

#else
    mssError(1,"PRT","SVG image support not available");
    return -1;
#endif
    }

