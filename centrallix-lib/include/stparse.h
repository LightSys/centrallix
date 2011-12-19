#ifndef _STPARSE_H
#define _STPARSE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	stparse.c, stparse.h 					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 29, 1998					*/
/* Description:	Parser to handle the request data stream from the end-	*/
/*		user.  Uses the MTLEXER module.				*/
/************************************************************************/



#ifdef CXLIB_INTERNAL
#include "mtask.h"
#include "mtlexer.h"
#else
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#endif

#define	ST_MAX_SUBINF	(128)
#define ST_MAX_VALUES	(64)
#define ST_MAX_NAMELEN	(64)

/** Structure for storing protocol data. **/
typedef struct _PI
    {
    char	Name[ST_MAX_NAMELEN];	/* name of component */
    char	UsrType[ST_MAX_NAMELEN]; /* type of component. */
    char*	StrVal[ST_MAX_VALUES];	/* string value, null if integer */
    int		IntVal[ST_MAX_VALUES];	/* integer value, as appropriate */
    int		StrAlloc[ST_MAX_VALUES]; /* string was malloc'd? */
    int		nVal;			/* number of value items */
    int		Type;			/* type of component: ST_T_xxx */
    struct _PI*	SubInf[ST_MAX_SUBINF];	/* Additional info attached here */
    int		nSubInf;		/* number of subparts */
    struct _PI*	Parent;			/* Parent inf */
    void*	UserData;		/* Misc linkage value */
    }
    StructInf, *pStructInf;

#define ST_T_STRUCT 	0
#define ST_T_ATTRIB	1
#define ST_T_SUBGROUP	2
#define ST_T_SCRIPT	3

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

#endif /* _STPARSE_H */

