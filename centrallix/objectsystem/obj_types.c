#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "mtask.h"
#include "mtlexer.h"
#include "obj.h"
#include "xarray.h"
#include "xhash.h"


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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 16, 1999					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		obj_types.c --> implements the objectsystem type	*/
/*		heirarchy ("is-a", etc.)				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_types.c,v 1.1 2001/08/13 18:01:00 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_types.c,v $

    $Log: obj_types.c,v $
    Revision 1.1  2001/08/13 18:01:00  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:01  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/** Structure for managing types **/
struct
    {
    XHashTable		ContentTypes;		/* List of content types */
    }
    OTYP;
