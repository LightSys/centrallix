#ifndef _XARRAY_H
#define _XARRAY_H

#ifdef __cplusplus
 extern "C" {
 #endif

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	xarray.c, xarray.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 12, 1998					*/
/* Description:	Extensible array system for Centrallix.  Provides	*/
/*		for variable-length 32-bit value arrays.  On Alpha 	*/
/*		systems, values are 64-bit, but can still store 32-bit	*/
/*		values.							*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xarray.h,v 1.4 2004/07/22 00:20:52 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xarray.h,v $

    $Log: xarray.h,v $
    Revision 1.4  2004/07/22 00:20:52  mmcgill
    Added a magic number define for WgtrNode, and added xaInsertBefore and
    xaInsertAfter functions to the XArray module.

    Revision 1.3  2003/06/27 21:18:34  gbeeley
    Added xarray xaSetItem() method

    Revision 1.2  2002/11/14 03:44:27  gbeeley
    Added a new function to the XArray module to do sorted array adds
    based on an integer field, which is portable between LSB and MSB
    platforms.  Fixed the normal sorted add routine which was not
    operating correctly anyhow.

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:02  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


/** Structure **/
typedef struct
    {
    int		nItems;
    int		nAlloc;
    void**	Items;
    }
    XArray, *pXArray;

/** Functions **/
int xaInit(pXArray, int init_size);
int xaDeInit(pXArray);
int xaAddItem(pXArray, void* item);
int xaAddItemSorted(pXArray, void* item, int keyoffset, int keylen);
int xaAddItemSortedInt32(pXArray, void* item, int keyoffset);
void* xaGetItem(pXArray, int index);
int xaFindItem(pXArray, void* item);
int xaRemoveItem(pXArray, int index);
int xaClear(pXArray);
int xaCount(pXArray);
int xaSetItem(pXArray, int index, void* item);
int xaInsertBefore(pXArray, int index, void* item);
int xaInsertAfter(pXArray, int index, void* item);

#define CLD(x,y,z) ((x)((y)->Children.Items[(z)]))

#ifdef __cplusplus
 }
 #endif

#endif /* _XARRAY_H */
