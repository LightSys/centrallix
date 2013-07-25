#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "cxlib/datatypes.h"
#include "obj.h"
#include "endorsement_utils.h"
#include "cxss/cxss.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2013 LightSys Technology Services, Inc.		*/
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
/* Module:	endorsement_utils.c                                     */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	15-Jul-2013                                             */
/*									*/
/* Description:	This module provides some standard functionality for	*/
/*		checking to see if security constraints pass on a given	*/
/*		object.							*/
/************************************************************************/


/*** endVerifyEndorsements() - see if this widget is requiring certain
 *** security endorsements.  If so, check them.  Returns 0 if endorsements pass or
 *** no endorsements are required.  Returns -1 if the endorsement check
 *** fails.
 ***/
int
endVerifyEndorsements(void* node_v, int (*attr_func)(), char** failed_endorsement_name)
    {
    char* endorsement_name;
    char* context;
    int endorsement_ok = 1;
    ObjData val;
    int i;
    char* ptr;

	/** Endorsement required? **/
	if (attr_func(node_v, "endorsement_context", DATA_T_STRING, POD(&context)) != 0)
	    context = "";
	if (attr_func(node_v, "require_endorsements", DATA_T_STRING, POD(&endorsement_name)) == 0)
	    {
	    /** Only one needed - do we have it? **/
	    if (cxssHasEndorsement(endorsement_name, context) <= 0)
		endorsement_ok = 0;
	    }
	else if (attr_func(node_v, "require_endorsements", DATA_T_STRINGVEC, &val) == 0)
	    {
	    /** Multiple endorsements required **/
	    for(i=0;i<val.StringVec->nStrings; i++)
		{
		endorsement_name = val.StringVec->Strings[i];
		if (cxssHasEndorsement(endorsement_name, context) <= 0)
		    {
		    endorsement_ok = 0;
		    break;
		    }
		}
	    }
	else if (attr_func(node_v, "require_one_endorsement", DATA_T_STRING, POD(&endorsement_name)) == 0)
	    {
	    /** Only one needed - do we have it? **/
	    if (cxssHasEndorsement(endorsement_name, context) <= 0)
		endorsement_ok = 0;
	    }
	else if (attr_func(node_v, "require_one_endorsement", DATA_T_STRINGVEC, &val) == 0)
	    {
	    /** One of a set of endorsements required **/
	    endorsement_ok = 0;
	    for(i=0;i<val.StringVec->nStrings; i++)
		{
		endorsement_name = val.StringVec->Strings[i];
		if (cxssHasEndorsement(endorsement_name, context) > 0)
		    {
		    endorsement_ok = 1;
		    break;
		    }
		}
	    }

	/** Invert security decision?  (this is for including an "alternate widget" in
	 ** the app when another widget has been omitted due to permissions.)
	 **/
	if (attr_func(node_v, "invert_security_check", DATA_T_STRING, POD(&ptr)) == 0 && (!strcasecmp(ptr,"yes")))
	    {
	    endorsement_ok = !endorsement_ok;
	    }

	if (failed_endorsement_name)
	    {
	    if (endorsement_ok)
		*failed_endorsement_name = "";
	    else
		*failed_endorsement_name = endorsement_name;
	    }

    return endorsement_ok?0:(-1);
    }

