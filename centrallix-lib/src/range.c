/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      range.c, range.h                                        */
/* Author:      Israel Fuller                                           */
/* Date:        October 13, 2025                                        */
/* Description: Adds some useful numerical range functions/macros that  */
/*              C does not provide by default, such as min(), max(),    */
/*              clamp(), etc.                                           */
/************************************************************************/

#include <math.h>

#include "range.h"

double roundTo(double value, int decimals)
    {
    const double mul = pow(10, decimals);
    return round(value * mul) / mul;
    }
