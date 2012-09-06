#ifndef _PARAM_H
#define _PARAM_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2012 LightSys Technology Services, Inc.		*/
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
/* Module:	param.c, param.h                                        */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	26-May-2012                                             */
/*									*/
/* Description:	Based on WgtrAppParam from the wgtr module.  This	*/
/*		provides a simple ability to parse and free parameters	*/
/*		that are in the form of a presentation hints record but	*/
/*		with a data type and name supplied, too.		*/
/*									*/
/************************************************************************/

#include "obj.h"
#include "hints.h"
#include "ptod.h"

#define PARAM_NAME_LEN		64

typedef struct
    {
    int				Type;		/* data type DATA_T_xxx */
    char			Name[PARAM_NAME_LEN];	/* name of parameter */
    pObjPresentationHints	Hints;		/* controls to apply to the param, incl defaults */
    pTObjData			Value;		/* Value */
    }
    Param, *pParam;

/*** Functions for handling parameters ***/
int paramFree(pParam param);
pParam paramCreateFromObject(pObject obj);
pParam paramCreateFromInf(pStructInf inf);
int paramSetValue(pParam param, pTObjData value);
int paramSetValueFromInfNe(pParam param, pStruct inf);
int paramEvalHints(pParam param, pParamObjects objlist);

#endif /* not defined _PARAM_H */
