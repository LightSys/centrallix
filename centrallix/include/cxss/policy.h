#ifndef _CXSS_POLICY_H
#define _CXSS_POLICY_H

#include "cxss/cxss.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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
/* Module:	cxss (security subsystem)                               */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	February 22, 2007                                       */
/*									*/
/* Description:	CXSS provides the core security services for the	*/
/*		Centrallix application platform.			*/
/************************************************************************/

/*** Security policy access types - mask ***/
#define CXSS_ACC_T_OBSERVE	1
#define CXSS_ACC_T_READ		2
#define CXSS_ACC_T_WRITE	4
#define CXSS_ACC_T_CREATE	8
#define CXSS_ACC_T_DELETE	16
#define CXSS_ACC_T_EXEC		32
#define CXSS_ACC_T_NOEXEC	64
#define CXSS_ACC_T_DELEGATE	128
#define CXSS_ACC_T_ENDORSE	256

/*** Authorization subsystem logging indications - mask ***/
#define CXSS_LOG_T_SUCCESS	1
#define CXSS_LOG_T_FAILURE	2
#define CXSS_LOG_T_ALL		(CXSS_LOG_T_SUCCESS | CXSS_LOG_T_FAILURE)

/*** Operation modes ***/
#define CXSS_MODE_T_DISABLE	1
#define CXSS_MODE_T_WARN	2
#define CXSS_MODE_T_ENFORCE	3

/*** Actions for rules - mask ***/
#define CXSS_ACT_T_ALLOW	1
#define CXSS_ACT_T_DENY		2
#define CXSS_ACT_T_ENDORSE	4


/*** Structure for one rule (or rule group) ***/
typedef struct
    {
    XArray		MatchPath;			/* strings */
    XArray		MatchEndorsement;		/* strings */
    int			MatchAction;			/* CXSS_ACC_T_xxx */
    }
    CxssPolRule, *pCxssPolRule;


/*** Structure for overall security policy data ***/
typedef struct _CXSSPOL
    {
    char		PolicyPath[OBJSYS_MAX_PATH + 1];
    char		DomainPath[OBJSYS_MAX_PATH + 1];
    char		Domain[CXSS_IDENTIFIER_LENGTH];
    int			PolicyMode;			/* CXSS_MODE_T_xxx */
    int			DefaultAction;			/* CXSS_ACT_T_xxx */
    DateTime		ModifyDate;
    XArray		SubPolicies;			/* pCxssPolicy */
    XArray		Authentications;		/* pCxssPolAuth */
    XArray		Inclusions;			/* pCxssPolInclude */
    XArray		Rules;				/* pCxssPolRule */
    }
    CxssPolicy, *pCxssPolicy;

#endif /* defined _CXSS_POLICY_H */

