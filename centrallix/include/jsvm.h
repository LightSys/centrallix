#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtlexer.h"
#include "expression.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	jsvm.h                       				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 28, 2000					*/
/* Description:	Header information for the interaction between the 	*/
/*		Centrallix process and its javascript vm subprocesses	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: jsvm.h,v 1.1 2001/08/13 18:00:53 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/jsvm.h,v $

    $Log: jsvm.h,v $
    Revision 1.1  2001/08/13 18:00:53  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** BlueF --> JSVM semaphore request ID's **/
#define JSVM_REQ_SHUTDOWN	0		/* jsvm should shut down */
#define JSVM_REQ_COMMAND	1		/* jsvm should exec script */
