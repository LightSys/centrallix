#ifndef _STPARSE_H
#define _STPARSE_H
#ifdef __cplusplus
extern "C" {
#endif
    
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

/**CVSDATA***************************************************************

    $Id: stparse.h,v 1.5 2009/07/14 22:08:08 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/stparse.h,v $

    $Log: stparse.h,v $
    Revision 1.5  2009/07/14 22:08:08  gbeeley
    - (feature) adding cx__download_as object attribute which is used by the
      HTTP interface to set the content disposition filename.
    - (feature) adding "filename" property to the report writer to use the
      cx__download_as feature to specify a filename to the browser to "Save
      As...", so reports have a more intelligent name than just "report.rpt"
      (or whatnot) when downloaded.

    Revision 1.4  2008/03/29 02:26:15  gbeeley
    - (change) Correcting various compile time warnings such as signed vs.
      unsigned char.

    Revision 1.3  2005/02/26 06:42:38  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.2  2001/10/22 17:36:05  gbeeley
    Beginning to add support for JS scripting facilities.

    Revision 1.1  2001/10/16 23:53:01  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:02  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "obj.h"
#include "expression.h"


/** New structure for storing structure file data. **/
typedef struct _SI
    {
    int		    Magic;
    char*	    Name;	/* name of attrib or group */
    char*	    UsrType;	/* type of group, null if attrib */
    pExpression	    Value;	/* value; EXPR_N_LIST if several listed */
    struct _SI*	    Parent;	/* Parent inf, null if toplevel */
    struct _SI**    SubInf;	/* List of attrs/groups included */
    unsigned short  nSubInf;	/* Number of attrs/groups - up to 65535 */
    unsigned char   nSubAlloc;	/* Amount of space allocated for subinf ptrs */
    unsigned char   Flags;	/* ST_F_xxx - either top, attrib, or group */
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

#define ST_ALLOCSIZ(x)	(((x)->nSubAlloc)<<(ST_SUBALLOC_BLKSIZ))

#define ST_SUBINF_LIMIT	    65535


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
pStructInf stLookup(pStructInf, char* name);
int stAttrValue(pStructInf, int* intval, char** strval, int nval);
pStructInf stAllocInf();
int stFreeInf(pStructInf inf);
int stAddInf(pStructInf main_inf, pStructInf sub_inf);

/*** new functions ***/
int stGetAttrValue(pStructInf, int type, pObjData value, int nval);
int stGetAttrValueOSML(pStructInf, int type, pObjData value, int nval, pObjSession sess);
int stGetAttrType(pStructInf, int nval);
int stStructType(pStructInf);
int stSetAttrValue(pStructInf, int type, pObjData value, int nval);
pExpression stGetExpression(pStructInf, int nval);
void* stGetValueList(pStructInf, int type, unsigned int* nval);
int stAttrIsList(pStructInf);

#ifdef __cplusplus
}
#endif
#endif /* _STPARSE_H */

