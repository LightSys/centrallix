#ifndef _BARCODE_H
#define _BARCODE_H


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
/* Module: 	barcode.c,barcode.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 14, 1998					*/
/* Description:	Generates the appropriate sequences for postal barcodes	*/
/*		given the zip+9+2+ckdigit.  The output sequences are	*/
/*		returned as a sequence of ascii '0' and ascii '1',	*/
/*		where 0 is short bar and 1 is tall bar.  No grizzly	*/
/*		bars.							*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: barcode.h,v 1.1 2001/08/13 18:00:52 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/barcode.h,v $

    $Log: barcode.h,v $
    Revision 1.1  2001/08/13 18:00:52  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:19  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/** Barcode types **/
#define	BAR_T_POSTAL	1

/** Functions **/
int barEncodeNumeric(int type, char* unencoded, int len, char* encoded, int maxlen);

#endif /* _BARCODE_H */
