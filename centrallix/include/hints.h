#ifndef _HINTS_H
#define _HINTS_H

#include "obj.h"
#include "stparse.h"
#include "xstring.h"
#include "ptod.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	hints.c, hints.h					*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	March 10, 2003						*/
/* Description:	Provides declarations for the presentation hints	*/
/*              utility functions.					*/
/************************************************************************/

pObjPresentationHints objInfToHints(pStructInf inf, int data_type);
pObjPresentationHints hntObjToHints(pObject obj);
int hntVerifyHints(pObjPresentationHints hints, pTObjData ptod, char** msg, pParamObjects objlist);
int hntEncodeHints(pObjPresentationHints hints, pXString xs);

#endif /** _HINTS_H **/
