#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "centrallix.h"
#include "mtask.h"
#include "obj.h"
#include "xstring.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "mtlexer.h"
#include <signal.h>


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2004 LightSys Technology Services, Inc.		*/
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
/* Module:	clog                                                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	July 28th, 2004                                         */
/*									*/
/* Description:	CLog is a CVS Commit Log entry editor that is designed	*/
/*		to standardize our commit logs and provide a way to	*/
/*		declare the origins of inserted or modified code.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: clog.c,v 1.1 2004/08/15 01:56:03 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/clog_src/clog.c,v $

    $Log: clog.c,v $
    Revision 1.1  2004/08/15 01:56:03  gbeeley
    - adding Commit Logger - a utility to help us standardize our commit log
      messages.  Only stubbed out at this point.  Planned are both text and
      GTK versions.  Use with $CVSEDITOR.

 **END-CVSDATA***********************************************************/

typedef struct
    {
    int		ArgC;
    char**	ArgV;
    char	Filename[256];
    }
    CLOG_t;

CLOG_t CLOG;


void
start(void* v)
    {
    int ch;

	/** Check for file to edit **/
	if (CLOG.ArgC != 1)
	    {
	    puts("clog: must specify exactly one file to edit");
	    thExit();
	    }
	memccpy(CLOG.Filename, CLOG.ArgV[1], 0, sizeof(CLOG.Filename)-1);
	CLOG.Filename[sizeof(CLOG.Filename)-1] = '\0';

    thExit();
    }


int
main(int argc, char* argv[])
    {

	CLOG.ArgC = argc;
	CLOG.ArgV = argv;

    return mtInitialize(0, start);
    }

