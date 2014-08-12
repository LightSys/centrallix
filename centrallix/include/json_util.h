#ifndef _JSON_UTIL_H
#define _JSON_UTIL_H

#include "json.h"
#include "cxlib/datatypes.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2014 LightSys Technology Services, Inc.		*/
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
/* Module:	json_util                                               */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	July 22, 2014                                           */
/*									*/
/* Description:	This file provides some utility functions to more easily*/
/*		handle json data (via json-c)				*/
/************************************************************************/


/*** json utility functions ***/
int jutilIsDateTimeObject(struct json_object* jobj);
int jutilGetDateTimeObject(struct json_object* jobj, pDateTime dt);
int jutilIsMoneyObject(struct json_object* jobj);
int jutilGetMoneyObject(struct json_object* jobj, pMoneyType m);

#endif /* not defined _JSON_UTIL_H */

