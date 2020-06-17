#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxss/cxss.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2015 LightSys Technology Services, Inc.		*/
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
/* Module:	cxss (Centrallix Security Subsystem) Policy  	        */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	October 8, 2015                                         */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.			*/
/*									*/
/*		The Policy module provides security policy		*/
/*		implementation.						*/
/************************************************************************/


/*** cxssAuthorizeSpec - given an object spec (domain:type:obj:attr), possibly
 *** containing blanks, and a given access type mask, determine whether the
 *** access is allowed.  Returns -1 on error, 0 on not allowed, and 1 on
 *** allowed.  The log_mode parameter is used to suppress logging if we're
 *** testing for access permissions but not actually about to do the access,
 *** or if for whatever reason a failed attempt should not be logged.
 ***/
int
cxssAuthorizeSpec(char* objectspec, int access_type, int log_mode)
    {
    char tmpspec[OBJSYS_MAX_PATH + 256];
    char *domain = "", *type = "", *path = "", *attr = "";
    char *colonptr;

	/** Make a local copy of the spec so we can adjust it **/
	if (strlen(objectspec) >= sizeof(tmpspec))
	    {
	    mssError(1,"CXSS","cxssAuthorizeSpec(): object spec too long.");
	    return -1;
	    }
	strtcpy(tmpspec, objectspec, sizeof(tmpspec));

	/** Break it up into its components. **/
	domain = tmpspec;
	colonptr = strchr(tmpspec, ':');
	if (colonptr)
	    {
	    type = colonptr+1;
	    *colonptr = '\0';
	    colonptr = strchr(type, ':');
	    if (colonptr)
		{
		path = colonptr+1;
		*colonptr = '\0';
		colonptr = strchr(path, ':');
		if (colonptr)
		    {
		    attr = colonptr+1;
		    *colonptr = '\0';
		    }
		}
	    }

    return cxssAuthorize(domain, type, path, attr, access_type, log_mode);
    }


/*** cxssAuthorize - like cxssAuthorizeSpec, but the various parts of the
 *** object spec are provided independently.
 ***/
int
cxssAuthorize(char* domain, char* type, char* path, char* attr, int access_type, int log_mode)
    {
    /** Just a stub right now **/
    return 1;
    }
