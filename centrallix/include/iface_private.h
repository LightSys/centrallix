#ifndef _IFACE_PRIVATE_H
#define _IFACE_PRIVATE_H

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "iface.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "obj.h"
#include "centrallix.h"
#include "ht_render.h"

/************** Debug Enable/Disable *****************/
#define IFC_DEBUG 0


/************** Internal Typedefs ********************/

/* This struct represents a major version of an interface.
 * There is an XArray of members for each category. Within each XArray,
 * members are stored in descending order according to minor version.
 * There is an Offsets XArray for each category as well, which contains
 * The offset for each minor version into the Members XArray.
 *
 * This ordering also means the correct member properties for the
 * requested minor version will be returned without any special code.
 */
typedef struct
    {
    XArray*	Offsets;	    /* Offsets for each minor version into the members arrays */
    XArray*	Members;	    /* XArrays of members, one for each category */
    XArray*	Properties;	    /* XArrays of properties, one for each category */
    int		NumMinorVersions;    /* How many minor versions there are */
    } IfcMajorVersion, *pIfcMajorVersion;

/* This struct represents an interface definition - all major and minor versions */
typedef struct
    {
    char*	Path;		    /* The path that identifies the interface definition */
    pObjSession ObjSession;	    /* The object session the definition is a part of */
    pObject	Obj;		    /* OSML object handle to the definition */
    XArray	MajorVersions;	    /* Array of pIfcMajorVersion structs, one for each major version */
    int		Type;		    /* Interface type (IFC_T_XXX) */
    }
    IfcDefinition, *pIfcDefinition;


struct IfcHandle_t
    {
    pObjSession		ObjSession;
    char*		DefPath;
    char*		FullPath;
    int			MinorVersion, MajorVersion;
    int			LinkCount;
    int			Type;
    int*		Offsets;
    pXArray		Members, Properties;
    };


/************ Globals *********************/

struct _IFC
    {
    XHashTable	    Definitions;	/* Already-parsed definitions */
    XHashTable	    HandleCache;	/* Cache of interface handles */
    char*	    IfaceDir;		/* Base directory for interface definitions */
    int		    NumCategories[IFC_NUM_TYPES];		/* Number of categories for each interface type */
    char*	    TypeNames[IFC_NUM_TYPES];			/* Names of the interface types */
    char**	    CategoryNames[IFC_NUM_TYPES];		/* Names of categories for each interface type */
    };

extern struct _IFC IFC;

#endif /* not defined _IFACE_PRIVATE_H */
