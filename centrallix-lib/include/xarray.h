#ifndef _XARRAY_H
#define _XARRAY_H


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

    $Id: xarray.h,v 1.1 2001/08/13 18:04:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xarray.h,v $

    $Log: xarray.h,v $
    Revision 1.1  2001/08/13 18:04:20  gbeeley
    Initial revision

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
int xaInit(pXArray this, int init_size);
int xaDeInit(pXArray this);
int xaAddItem(pXArray this, void* item);
int xaAddItemSorted(pXArray this, void* item, int keyoffset, int keylen);
void* xaGetItem(pXArray this, int index);
int xaFindItem(pXArray this, void* item);
int xaRemoveItem(pXArray this, int index);
int xaClear(pXArray this);
int xaCount(pXArray this);

#define CLD(x,y,z) ((x)((y)->Children.Items[(z)]))

#endif /* _XARRAY_H */
