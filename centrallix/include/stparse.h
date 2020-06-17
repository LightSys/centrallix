#ifndef _STPARSE_H
#define _STPARSE_H

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
/* Module: 	stparse_new.c, stparse_new.h 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 29, 1998					*/
/* Description:	Parser to handle the request data stream from the end-	*/
/*		user.  Uses the MTLEXER module.				*/
/************************************************************************/



#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "obj.h"
#include "expression.h"


/** New structure for storing structure file data. **/
typedef struct _SI
    {
    int		    Magic;
    int		    LinkCnt;
    char*	    Name;	/* name of attrib or group */
    char*	    UsrType;	/* type of group, null if attrib */
    pExpression	    Value;	/* value; EXPR_N_LIST if several listed */
    struct _SI*	    Parent;	/* Parent inf, null if toplevel */
    struct _SI**    SubInf;	/* List of attrs/groups included */
    unsigned int    nSubInf:21;	/* Number of attrs/groups - up to ST_SUBINF_LIMIT */
    unsigned char   nSubAlloc:6; /* Amount of space allocated for subinf ptrs */
    unsigned char   Flags:5;	/* ST_F_xxx - either top, attrib, or group */
    unsigned char*  ScriptText;	/* If a ST_F_SCRIPT node, here is the script text */
    void*	    ScriptCode;	/* "compiled" script code */
    void*	    UserData;	/* Misc linkage for use by app */
    }
    StructInf, *pStructInf;

#define ST_F_ATTRIB	1
#define ST_F_GROUP	2	/* set for top level group and other group */
#define ST_F_TOPLEVEL	4
#define ST_F_VERSION2	8	/* set if a version 2 structure file */
#define ST_F_SCRIPT   	16	/* set if a procedure/script. */

#define ST_SUBALLOC_BLKSIZ  (16/2)	/* starting alloc cnt for subinfs */
#define	ST_USRTYPE_STRLEN   64	/* if group, alloc bytes for type name */
#define ST_NAME_STRLEN	    64	/* alloc size for group/attrib name */

/*#define ST_ALLOCSIZ(x)	(((x)->nSubAlloc)<<(ST_SUBALLOC_BLKSIZ))*/
#define ST_ALLOCSIZ(x)	(ST_SUBALLOC_BLKSIZ<<((x)->nSubAlloc))

#define ST_SUBINF_LIMIT	    (2 * 1024 * 1024 - 1)


#if 00 /* GRB - old version */

/** Structure for storing protocol data. **/
typedef struct _PI
    {
    char	Name[64];	/* name of component */
    char	UsrType[64];	/* type of component. */
    char*	StrVal[64];	/* string value, null if integer */
    int		IntVal[64];	/* integer value, as appropriate */
    int		StrAlloc[64];	/* string was malloc'd? */
    int		nVal;		/* number of value items */
    int		Type;		/* type of component: ST_T_xxx */
    struct _PI*	SubInf[64];	/* Additional info attached here */
    int		nSubInf;	/* number of subparts */
    struct _PI*	Parent;		/* Parent inf */
    void*	UserData;	/* Misc linkage value */
    }
    StructInf, *pStructInf;

#endif /* if 0 GRB old version */

#ifndef ST_T_STRUCT
#define ST_T_STRUCT 	0
#define ST_T_ATTRIB	1
#define ST_T_SUBGROUP	2
#define ST_T_SCRIPT	3
#endif

/*** original structure file api ***/
pStructInf stParseMsg(pFile inp_fd, int flags);
pStructInf stParseMsgGeneric(void* src, int (*read_fn)(), int flags);
int stGenerateMsg(pFile out_fd, pStructInf info, int flags);
int stGenerateMsgGeneric(void* dst, int (*write_fn)(), pStructInf info, int flags);
pStructInf stCreateStruct(char* name, char* type);
pStructInf stAddAttr(pStructInf inf, char* name);
pStructInf stAddGroup(pStructInf inf, char* name, char* type);
int stAddValue(pStructInf inf, char* strval, int intval);
pStructInf stLookup(pStructInf this, char* name);
int stAttrValue(pStructInf this, int* intval, char** strval, int nval);
pStructInf stAllocInf();
int stFreeInf(pStructInf inf);
int stAddInf(pStructInf main_inf, pStructInf sub_inf);

/*** new functions ***/
pStructInf stLinkInf(pStructInf inf);
int stPrintInf(pStructInf this);
int stRemoveInf(pStructInf inf);
int stGetAttrValue(pStructInf this, int type, pObjData value, int nval);
int stGetObjAttrValue(pStructInf this, char* attrname, int type, pObjData value);
int stGetAttrValueOSML(pStructInf this, int type, pObjData value, int nval, pObjSession sess);
int stGetAttrType(pStructInf this, int nval);
int stStructType(pStructInf this);
int stSetAttrValue(pStructInf this, int type, pObjData value, int nval);
pExpression stGetExpression(pStructInf this, int nval);
void* stGetValueList(pStructInf this, int type, unsigned int* nval);
int stAttrIsList(pStructInf this);
/** \brief Separate a subtree from its parent.
 
 After calling this, you are responsible to free both this subtree
 and its parent if there is one.
 \param toSeparate The tree to separate from its parent, i f it has one.
 \return This returns 0 in all cases.
 */
int stSeparate(pStructInf toSeparate);

#endif /* _STPARSE_H */

