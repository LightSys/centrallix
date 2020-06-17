#ifndef _ENDORSEMENT_UTILS_H
#define _ENDORSEMENT_UTILS_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2013 LightSys Technology Services, Inc.		*/
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
/* Module: 	endorsement_utils					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	15-Jul-2013     					*/
/* Description:	This module provides some standard functionality for	*/
/*		checking to see if security constraints pass on a given	*/
/*		object.							*/
/************************************************************************/

#include "obj.h"

int endVerifyEndorsements(void* node_v, int (*attr_func)(), char** failed_endorsement_name);

#endif /* not defined _ENDORSEMENT_UTILS_H */
