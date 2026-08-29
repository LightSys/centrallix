#ifndef _PRTMGMT_V3_LM_TEXT_H
#define _PRTMGMT_V3_LM_TEXT_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2001-2003 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_lm_text.h                                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	April 9th, 2003                                         */
/*									*/
/* Description:	The lm_text module provides layout functionality for	*/
/*		textflow type areas.					*/
/************************************************************************/



#include "prtmgmt_v3/prtmgmt_v3.h"
#define PRT_TEXTLM_F_RMSPACE    PRT_OBJ_F_LMFLAG1       /* space was removed at end */


typedef struct _PTL
    {
    PrtBorder   AreaBorder;     /* optional border around the text area */
    }
    PrtTextLMData, *pPrtTextLMData;


#endif /* not defined _PRTMGMT_V3_LM_TEXT_H */


