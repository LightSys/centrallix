#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "mtask.h"
#include "obj.h"
#include "xstring.h"
#include "xarray.h"
#include "xhash.h"
#include "expression.h"
#include "jsvm.h"

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
/* Module:	jsvm_main.c, jsvm.h                                     */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	October 19, 2001                                        */
/*									*/
/* Description:	This module implements the JS virtual machine,		*/
/*		compiler, and generator.  This file, jsvm_main,		*/
/*		implements the main functions for the subsystem.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: jsvm_main.c,v 1.1 2001/10/22 17:36:05 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/jsvm/jsvm_main.c,v $

    $Log: jsvm_main.c,v $
    Revision 1.1  2001/10/22 17:36:05  gbeeley
    Beginning to add support for JS scripting facilities.


 **END-CVSDATA***********************************************************/


/*** jsvm globals ***/
struct
    {
    XHashTable	Functions;	/* name to function pointer table */
    XHashTable	Operators;	/* op name to function pointer table */
    XArray	Nodes;		/* node type to function ptr table */
    XHashTable	OpPrec;		/* operator precedence level */
    XArray	NodePrec;	/* nodetype precedence level */
    XHashTable	Languages;	/* script language compilers/generators */
    }
    JSVM_INF;


/*** jsvmInitialize - init the js virtual machine (and compiler, generator,
 *** etc).
 ***/
int
jsvmInitialize()
    {

	/** Init the globals **/
	memset(&JSVM_INF, 0, sizeof(JSVM_INF));
	xhInit(&(JSVM_INF.Functions), 255, 0);
	xhInit(&(JSVM_INF.Operators), 255, 0);
	xhInit(&(JSVM_INF.OpPrec), 255, 0);
	xaInit(&(JSVM_INF.Nodes), 64);
	xaInit(&(JSVM_INF.NodePrec), 64);
	xhInit(&(JSVM_INF.Languages), 63, 0);

    return 0;
    }

