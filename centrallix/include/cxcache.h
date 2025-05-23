#ifndef _CXCACHE_H
#define _CXCACHE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2017 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version approved by LightSys Technology	*/
/* Services, Inc.							*/
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
/* Module:	cxcache.h, cxcache.c					*/
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	Oct 23, 2017                                            */
/*									*/
/* Description:	This utility module provides caching services and logic.*/
/************************************************************************/


/*** Cache control structure ***/
typedef struct
    {
    char*		Directory;
    XHashQueue		Cache;
    }
    CxCache, *pCxCache;


/*** Cache item structure ***/
typedef struct
    {
    char*		ContentFile;
    char*		AttrsFile;
    }
    CxCacheItem, *pCxCacheItem;


/*** Functions ***/
pCxCache cxcOpenCache(char* directory, int max_size_mb);


#endif /* not defined _CXCACHE_H */
