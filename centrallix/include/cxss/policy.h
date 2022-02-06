#ifndef _CXSS_POLICY_H
#define _CXSS_POLICY_H

#include "cxss/cxss.h"
#include <stdbool.h>

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

/*** Actions for rules ***/
#define CXSS_ACT_T_ALLOW	1
#define CXSS_ACT_T_DENY		2
#define CXSS_ACT_T_DEFALLOW	4
#define CXSS_ACT_T_DEFDENY	8
#define CXSS_ACT_T_LOG		16

/*** Result of rule match check **/
#define CXSS_MATCH_T_FALSE  0
#define CXSS_MATCH_T_TRUE   1
#define CXSS_MATCH_T_ERR   -1

/*** Structure for one rule (or rule group) ***/
typedef struct
    {
    char		MatchObject[OBJSYS_MAX_PATH + 1];
    char		MatchSubject[CXSS_IDENTIFIER_LENGTH];
    char		MatchEndorsement[CXSS_IDENTIFIER_LENGTH];
    int			MatchAccess;			/* mask of CXSS_ACC_T_xxx */
    int			Action;				/* mask of CXSS_ACT_T_xxx */
    }
    CxssPolRule, *pCxssPolRule;


/*** Subject Lists i.e. authentication methods ***/
typedef struct
    {
    char		IdentMethod[CXSS_IDENTIFIER_LENGTH];
    char		AuthMethod[CXSS_IDENTIFIER_LENGTH];
    char		Identity[CXSS_IDENTIFIER_LENGTH];
    bool		IsDefault;
    void*		Criteria;			/* pExpression */
    char		AddEndorsement[CXSS_IDENTIFIER_LENGTH];
    }
    CxssPolAuth, *pCxssPolAuth;


/*** Policy inclusion rules ***/
typedef struct
    {
    char		ProbeSQL[1024];
    char		Path[OBJSYS_MAX_PATH + 1];
    char		Domain[CXSS_IDENTIFIER_LENGTH];
    bool		AllowInclusion;
    bool		AllowSubjectlist;
    bool		AllowRule;
    bool		AllowMode;			/* disable/warn/enforce */
    }
    CxssPolInclude, *pCxssPolInclude;


/*** Structure for overall security policy data ***/
typedef struct _CXSSPOL
    {
    // i.e. if this policy is in a subpolicy, it points back to parent policy
    // becomes a tree - each node in tree has list of rules to iterate through,
    // have to iterate through each node in the tree path for that object
    struct _CXSSPOL *	Parent;
    char		PolicyPath[OBJSYS_MAX_PATH + 1];
    char		DomainPath[OBJSYS_MAX_PATH + 1];
    char		Domain[CXSS_IDENTIFIER_LENGTH];
    int			PolicyMode;			/* CXSS_MODE_T_xxx */
    DateTime	ModifyDate;
    // go into SubPolicies (the result of Inclusions) to evaluate requests
    // it's an array of this current data structure (recursive)
    XArray		SubPolicies;			/* pCxssPolicy */
    XArray		Authentications;		/* pCxssPolAuth */
    XArray		Inclusions;			/* pCxssPolInclude */
    XArray		Rules;				/* pCxssPolRule */
    }
    CxssPolicy, *pCxssPolicy;

    //This class is to implement the queue
    typedef struct node {
        pCxssPolicy Policy;
        struct node *next;
    } node_t;

/*** Structure to store a policy into the xQueue for breadthfirst traversal ***/


#endif /* defined _CXSS_POLICY_H */

