#ifndef _INTERFACE_H
#define _INTERFACE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	Interface Module     					*/
/* Author:	Matt McGill (MJM)					*/
/* Creation:	July 26, 2004   					*/
/* Description:	Provies the functionality for handling interfaces in a  */
/*		general way, to be used by any Centrallix module that   */
/*		has need of support for interfaces.                    	*/
/************************************************************************/

/**CVSDATA***************************************************************

 **END-CVSDATA***********************************************************/

#include "cxlib/xarray.h"
#include "stparse.h"

/* interface types */
#define IFC_T_WIDGET	    0
#define IFC_NUM_TYPES	    1

#define IFC_CACHE_SIZE	    17

/* interface categories */
#define IFC_CAT_WIDGET_PARAM	    0
#define IFC_CAT_WIDGET_EVENT	    1
#define IFC_CAT_WIDGET_ACTION	    2
#define IFC_CAT_WIDGET_PROP	    3
#define IFC_CAT_WIDGET_CONTAINER    4


/* The user doesn't need to know the contents...just treat it like magic */
typedef struct IfcHandle_t* IfcHandle;

/** External Functions **/
int ifcContains(IfcHandle h, int category, char* member);
pObject ifcGetProperties(IfcHandle h, int category, char* member);
int ifcIsSubset(IfcHandle h1, IfcHandle h2);	/* is h2 a subset of h1? Does h1 provide all the functionality in h2? */
IfcHandle ifcGetHandle(pObjSession s, char* path);	/* Returns a handle to an interfae */
void ifcReleaseHandle(IfcHandle handle);    /* Releases a handle to an interface */
int ifcInitialize();

/** Functions for writing interfaces out in various formats **/
int ifcToHtml(pFile file, pObjSession s, char* def_str);

#endif
