#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "json/json.h"
#include "cxlib/datatypes.h"
#include "obj.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2014 LightSys Technology Services, Inc.		*/
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


/*** jutilIsDateTimeObject() - determine if a json_type_object node is
 *** actually a DATA_T_DATETIME object.
 ***
 *** Example (only year/month/day fields are required):
 *** { "year":2013, "month":11, "day":15, "hour":23, "minute":46, "second":30, "tzoffset":-420 }
 ***
 *** Returns 0 on false, 1 on true.
 ***/
int
jutilIsDateTimeObject(struct json_object* jobj)
    {
    struct json_object_iter iter;
    int has_year = 0;
    int has_month = 0;
    int has_day = 0;
    int has_hour = 0;
    int has_minute = 0;
    int has_second = 0;
    int has_tz = 0;
    int has_other = 0;

	/** Must be an 'object' **/
	if (!json_object_is_type(jobj, json_type_object))
	    return 0;

	/** Search the object's properties **/
	json_object_object_foreachC(jobj, iter)
	    {
	    if (json_object_is_type(iter.val, json_type_int))
		{
		if (!strcmp(iter.key, "year")) has_year = 1;
		else if (!strcmp(iter.key, "month")) has_month = 1;
		else if (!strcmp(iter.key, "day")) has_day = 1;
		else if (!strcmp(iter.key, "hour")) has_hour = 1;
		else if (!strcmp(iter.key, "minute")) has_minute = 1;
		else if (!strcmp(iter.key, "second")) has_second = 1;
		else if (!strcmp(iter.key, "tzoffset")) has_tz = 1;
		else has_other = 1;
		}
	    else    
		has_other = 1;
	    }

	/** What we require **/
	if (has_year && has_month && has_day && !has_other)
	    return 1;

    return 0;
    }


/*** jutilGetDateTimeObject() - get a DateTime value.
 ***
 *** Example (only year/month/day fields are required):
 *** { "year":2013, "month":11, "day":15, "hour":23, "minute":46, "second":30, "tzoffset":-420 }
 ***/
int
jutilGetDateTimeObject(struct json_object* jobj, pDateTime dt)
    {
    struct json_object_iter iter;
    int has_year = 0;
    int has_month = 0;
    int has_day = 0;
    int has_hour = 0;
    int has_minute = 0;
    int has_second = 0;
    int has_tz = 0;
    int has_other = 0;

	/** Must be an 'object' **/
	if (!json_object_is_type(jobj, json_type_object))
	    return 0;

	/** Search the object's properties **/
	memset(dt, 0, sizeof(DateTime));
	json_object_object_foreachC(jobj, iter)
	    {
	    if (json_object_is_type(iter.val, json_type_int))
		{
		if (!strcmp(iter.key, "year"))
		    {
		    has_year = 1;
		    dt->Part.Year = json_object_get_int(iter.val) - 1900;
		    }
		else if (!strcmp(iter.key, "month"))
		    {
		    has_month = 1;
		    dt->Part.Month = json_object_get_int(iter.val) - 1;
		    }
		else if (!strcmp(iter.key, "day"))
		    {
		    has_day = 1;
		    dt->Part.Day = json_object_get_int(iter.val) - 1;
		    }
		else if (!strcmp(iter.key, "hour"))
		    {
		    has_hour = 1;
		    dt->Part.Hour = json_object_get_int(iter.val);
		    }
		else if (!strcmp(iter.key, "minute"))
		    {
		    has_minute = 1;
		    dt->Part.Minute = json_object_get_int(iter.val);
		    }
		else if (!strcmp(iter.key, "second"))
		    {
		    has_second = 1;
		    dt->Part.Second = json_object_get_int(iter.val);
		    }
		else if (!strcmp(iter.key, "tzoffset"))
		    {
		    has_tz = 1;
		    }
		else
		    {
		    has_other = 1;
		    }
		}
	    else    
		has_other = 1;
	    }

	/** What we require **/
	if (has_year && has_month && has_day && !has_other)
	    return 0;

    return -1;
    }


/*** jutilIsMoneyObject() - determine if a json_type_object node is
 *** actually a DATA_T_MONEY object.
 ***
 *** Example:
 *** { "wholepart":123, "fractionpart":45 }
 ***
 *** Returns 0 on false, 1 on true.
 ***/
int
jutilIsMoneyObject(struct json_object* jobj)
    {
    struct json_object_iter iter;
    int has_whole = 0;
    int has_fraction = 0;
    int has_other = 0;

	/** Must be an 'object' **/
	if (!json_object_is_type(jobj, json_type_object))
	    return 0;

	/** Search the object's properties **/
	json_object_object_foreachC(jobj, iter)
	    {
	    if (json_object_is_type(iter.val, json_type_int))
		{
		if (!strcmp(iter.key, "wholepart")) has_whole = 1;
		else if (!strcmp(iter.key, "fractionpart")) has_fraction = 1;
		else has_other = 1;
		}
	    else    
		has_other = 1;
	    }

	/** What we require **/
	if (has_whole && has_fraction && !has_other)
	    return 1;

    return 0;
    }


/*** jutilGetMoneyObject() - get MoneyType data.
 ***
 *** Example:
 *** { "wholepart":123, "fractionpart":45 }
 ***/
int
jutilGetMoneyObject(struct json_object* jobj, pMoneyType m)
    {
    struct json_object_iter iter;
    int has_whole = 0;
    int has_fraction = 0;
    int has_other = 0;

	/** Must be an 'object' **/
	if (!json_object_is_type(jobj, json_type_object))
	    return 0;

	/** Search the object's properties **/
	memset(m, 0, sizeof(MoneyType));
	json_object_object_foreachC(jobj, iter)
	    {
	    if (json_object_is_type(iter.val, json_type_int))
		{
		if (!strcmp(iter.key, "wholepart"))
		    {
		    has_whole = 1;
		    //m->WholePart = json_object_get_int(iter.val);
		    m->Value += json_object_get_int(iter.val) * 10000;
		    }
		else if (!strcmp(iter.key, "fractionpart"))
		    {
		    has_fraction = 1;
		    //m->FractionPart = json_object_get_int(iter.val);
		    m->Value += json_object_get_int(iter.val);
		    }
		else
		    {
		    has_other = 1;
		    }
		}
	    else    
		has_other = 1;
	    }

	/** What we require **/
	if (has_whole && has_fraction && !has_other)
	    return 0;

    return -1;
    }


