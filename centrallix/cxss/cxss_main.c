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
/* Module:	cxss (Centrallix Security Subsystem)                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	February 22, 2007                                       */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: cxss_main.c,v 1.1 2007/02/22 23:25:14 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/cxss/cxss_main.c,v $

    $Log: cxss_main.c,v $
    Revision 1.1  2007/02/22 23:25:14  gbeeley
    - (feature) adding initial framework for CXSS, the security subsystem.
    - (feature) CXSS entropy pool and key generation, basic framework.
    - (feature) adding xmlhttprequest capability
    - (change) CXSS requires OpenSSL, adding that check to the build
    - (security) Adding application key to thwart request spoofing attacks.
      Once the AML is active, application keying will be more important and
      will be handled there instead of in net_http.


 **END-CVSDATA***********************************************************/

/*** Module-wide globals ***/
CXSS_t CXSS;

/*** cxssInitialize - Initialize the security subsystem.
 ***/
int
cxssInitialize()
    {
    int err;

	/** Initialize the entropy pool **/
	err = cxss_internal_InitEntropy(CXSS_ENTROPY_SIZE);

    return err;
    }

