#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "double.h"

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
/* Module: 	double.c, double.h   					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 22, 2023    					*/
/* Description:	Utilities for comparison of double precision floating	*/
/*		point numbers.						*/
/************************************************************************/


/*** Compare two double precision numbers at a given precision level and
 *** return a value < = > zero depending on whether a < = > b.  The
 *** precision number is absolute, i.e. to compare integers use prec of
 *** 0.5.
 ***/
int
realComparePrecision(double a, double b, double prec)
    {
    if (fabs(a - b) < prec)
	return 0;
    else if (a > b)
	return 1;
    else
	return -1;
    }


/*** Compare two double precision numbers at a given percentage level and
 *** return a value < = > zero depending on whether a < = > b.  The
 *** percentage (in decimal form) is relative of course to the magnitudes
 *** of the two numbers given.  If one or both numbers is zero, use
 *** traditional comparisons.
 ***/
int
realComparePercent(double a, double b, double pct)
    {
    if (a == 0.0 && b == 0.0)
	return 0;
    else if (a == 0.0 || b == 0.0)
	{
	if (a > b)
	    return 1;
	else
	    return -1;
	}
    else if (fabs(a) >= fabs(b))
	{
	if (fabs(a/b) < (1.0 + pct))
	    return 0;
	else if (a > b)
	    return 1;
	else
	    return -1;
	}
    else
	{
	if (fabs(b/a) < (1.0 + pct))
	    return 0;
	else if (a > b)
	    return 1;
	else
	    return -1;
	}
    }
