#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "xstring.h"
#include "stparse.h"
#include "stparse_ne.h"
#include "st_node.h"
#include "expression.h"
#include "report.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_report.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 12, 1999   					*/
/* Description:	Report generator objectsystem driver.  This driver is	*/
/*		used to generate HTML reports from a variety of data	*/
/*		sources, similarly to the way the original report 	*/
/*		server could generate them.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_report_v3.c,v 1.9 2004/06/12 00:10:15 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_report_v3.c,v $

    $Log: objdrv_report_v3.c,v $
    Revision 1.9  2004/06/12 00:10:15  mmcgill
    Chalk one up under 'didn't understand the build process'. The remaining
    os drivers have been updated, and the prototype for objExecuteMethod
    in obj.h has been changed to match the changes made everywhere it's
    called - param is now of type pObjData, not void*.

    Revision 1.8  2004/02/24 20:25:41  gbeeley
    - misc changes: runclient check in evaltree in stparse, eval() function
      rejected in sybase driver, update version in centrallix.conf, .cmp
      extension added for component-decl in types.cfg

    Revision 1.7  2003/09/02 15:37:13  gbeeley
    - Added enhanced command line interface to test_obj.
    - Enhancements to v3 report writer.
    - Fix for v3 print formatter in prtSetTextStyle().
    - Allow spec pathname to be provided in the openctl (command line) for
      CSV files.
    - Report writer checks for params in the openctl.
    - Local filesystem driver fix for read-only files/directories.
    - Race condition fix in UX printer osdriver
    - Banding problem workaround installed for image output in PCL.
    - OSML objOpen() read vs. read+write fix.

    Revision 1.6  2003/07/09 18:13:20  gbeeley
    Further polishing/work on the table output in the report writer.  Re-
    enabled uxprint OSD once its dependence on prtmgmt was removed.

    Revision 1.5  2003/06/27 21:19:47  gbeeley
    Okay, breaking the reporting system for the time being while I am porting
    it to the new prtmgmt subsystem.  Some things will not work for a while...

    Revision 1.4  2003/06/20 03:22:35  gbeeley
    Updating changelog and release notes for 0.7.3

    Revision 1.6  2002/08/10 02:09:45  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

    Revision 1.5  2002/06/19 23:20:42  gbeeley
    Fixed some compiler warnings, repaired some potential security issues.

    Revision 1.4  2002/05/03 03:48:16  gbeeley
    Update due to modification in the xhClear() api.

    Revision 1.3  2001/10/16 23:53:02  gbeeley
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

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:09  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:08  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Structure used for managing a reporting session ***/
typedef struct
    {
    pPrtSession		PSession;
    int			PageHandle;
    pFile		FD;
    pXHashTable		Queries;
    pObjSession		ObjSess;
    pStructInf		HeaderInf;
    pStructInf		FooterInf;
    }
    RptSession, *pRptSession;


/*** Structure for precompiled expressions, etc. ***/
typedef struct
    {
    pExpression		Exp;
    pExpression		LastValue;
    }
    RptUserData, *pRptUserData;


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    char	ContentType[64];
    int		Flags;
    pObject	Obj;
    int		Mask;
    pSnNode	Node;
    pFile	MasterFD;
    pFile	SlaveFD;
    int		AttrID;
    pStructInf	AttrOverride;
    StringVec	SVvalue;
    IntVec	IVvalue;
    void*	VecData;
    pParamObjects ObjList;
    pRptSession	RSess;
    int		Version;
    pSemaphore	StartSem;
    pSemaphore	IOSem;
    XArray	UserDataSlots;
    int		NextUserDataSlot;
    }
    RptData, *pRptData;

#define RPT_F_ISDIR		1
#define RPT_F_ISOPEN		2
#define RPT_F_ERROR		4
#define RPT_F_ALREADY_FF	8

#define RPT(x) ((pRptData)(x))

#define RPT_MAX_CACHE	32


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pRptData	ObjInf;
    pObjQuery	Query;
    char	NameBuf[256];
    char*	QyText;
    pObject	LLQueryObj;
    pObjQuery	LLQuery;
    int		NextSubInfID;
    char*	ItemText;
    char*	ItemSrc;
    char*	ItemWhere;
    XHashTable	StructTable;
    }
    RptQuery, *pRptQuery;


/*** Structure for report query connection information ***/
typedef struct
    {
    pStructInf	UserInf;
    char*	Name;
    pObjQuery	Query;
    pObject	QueryItem;
    pParamObjects ObjList;
    XArray	AggregateExpList;
    XArray	AggregateWhereList;
    XArray	AggregateDoReset;
    int		RecordCnt;
    int		InnerExecCnt;
    int		IsInnerOuter;
    char*	DataBuf;
    }
    QueryConn, *pQueryConn;


/*** Structure used for source activation within table/form ***/
typedef struct
    {
    int		Count;
    int		StackPtr;
    int		InnerMode[16];
    int		OuterMode[16];
    pQueryConn	Queries[16];
    int		Flags[16];
    char*	Names[16];
    int		MultiMode;
    }
    RptActiveQueries, *pRptActiveQueries;

#define RPT_A_F_JUSTSTARTED	1
#define RPT_A_F_NEEDUPDATE	2

#define RPT_MM_NESTED		1
#define RPT_MM_PARALLEL		2
#define RPT_MM_SERIAL		3
#define RPT_MM_MULTINESTED	4


/*** Globals. ***/
struct
    {
    XArray	Printers;
    }
    RPT_INF;

#define RPT_NUM_ATTR	7
static char* attrnames[RPT_NUM_ATTR] = {"bold","expanded","compressed","center","underline","italic","barcode"};


int rpt_internal_DoTable(pRptData, pStructInf, pRptSession, int container_handle);
int rpt_internal_DoSection(pRptData, pStructInf, pRptSession, pQueryConn);
int rpt_internal_DoField(pRptData, pStructInf, pRptSession, pQueryConn);
int rpt_internal_DoComment(pRptData, pStructInf, pRptSession);
int rpt_internal_DoData(pRptData, pStructInf, pRptSession, int container_handle);
int rpt_internal_DoForm(pRptData, pStructInf, pRptSession, int container_handle);
int rpt_internal_WriteExpResult(pRptSession rs, pExpression exp, int container_handle);


/*** rpt_internal_GetNULLstr - get the string to be substituted in place of
 *** a null value in tables and such.
 ***/
char*
rpt_internal_GetNULLstr()
    {
    char* ptr = mssGetParam("nfmt");
    return ptr?ptr:"NULL";
    }


/*** rpt_internal_GetParam - retrieves a parameter value from either the
 *** structure file or from the parameter override structure.
 ***/
pStructInf
rpt_internal_GetParam(pRptData inf, char* paramname)
    {
    pStructInf find_inf;

    	/** Look for it in the param override structure first **/
	find_inf = stLookup(inf->AttrOverride, paramname);
	
	/** If not found, look in the structure file **/
	if (!find_inf)
	    {
	    if (inf->Version == 1)
	        {
		/** Version 1: top-level attr **/
	        find_inf = stLookup(inf->Node->Data, paramname);
	        }
	    else
	        {
		/** Version 2: system/parameter, default attr **/
		find_inf = stLookup(inf->Node->Data, paramname);
		if (!find_inf) return NULL;
		find_inf = stLookup(find_inf,"default");
		}
	    }

    return find_inf;
    }


/*** rpt_internal_SubstParam - performs parameterized substitution on the
 *** given string, returning an allocated string containing the substitution.
 ***/
pXString
rpt_internal_SubstParam(pRptData inf, char* src)
    {
    pXString dest;
    char* ptr;
    char* ptr2;
    char paramname[64];
    int n;
    pStructInf param_inf;
    char* sptr;
    int t;

    	/** Allocate our string. **/
	dest = (pXString)nmMalloc(sizeof(XString));
	if (!dest) return NULL;
	xsInit(dest);

	/** Look for the param subst & sign... **/
	while((ptr = strchr(src,'&')))
	    {
	    /** Copy all of string up to the & but not including it **/
	    xsConcatenate(dest, src, ptr-src);

	    /** Now look for the name of the parameter. **/
	    ptr2 = ptr+1;
	    while((*ptr2 >= 'A' && *ptr2 <= 'Z') || (*ptr2 >= 'a' && *ptr2 <= 'z') || *ptr2 == '_')
	        {
		ptr2++;
		}
	    n = (ptr2 - ptr) - 1;
	    if (n <= 63 && n > 0)
	        {
		memcpy(paramname, ptr+1, n);
		paramname[n] = 0;
		}
	    else
	        {
		xsConcatenate(dest, "&", 1);
		src = ptr+1;
		continue;
		}

	    /** Ok, got the paramname.  Now look it up. **/
	    param_inf = rpt_internal_GetParam(inf, paramname);
	    if (!param_inf)
	        {
		xsConcatenate(dest, "&", 1);
		src = ptr+1;
		continue;
		}

	    /** If string, copy it.  If number, sprintf it then copy it. **/
	    t = stGetAttrType(param_inf, 0);
	    if (t == DATA_T_STRING)
		{
		stGetAttrValue(param_inf, DATA_T_STRING, POD(&sptr), 0);
		xsConcatenate(dest, sptr, -1);
		}
	    else if (t == DATA_T_INTEGER)
		{
		stGetAttrValue(param_inf, DATA_T_INTEGER, POD(&n), 0);
		xsConcatPrintf(dest, "%d", n);
		}
	
	    /** Ok, skip over the paramname so we don't copy it... **/
	    src = ptr2;
	    }

	/** Copy the tail of the string **/
	xsConcatenate(dest, src, -1);

    return dest;
    }


/*** rpt_internal_GetStyle - gets the style bitmask based on the listing in
 *** a given inf structure.
 ***/
int
rpt_internal_GetStyle(pStructInf element)
    {
    int stylemask=0;
    char* ptr;
    int i,j;
    pStructInf st;

	/** Check to see if style specified.  If not, return err **/
	st = stLookup(element,"style");
	if (!st) return -1;

	/** Plain style? **/
	ptr = NULL;
	stGetAttrValue(st, DATA_T_STRING, POD(&ptr), 0);
	if (ptr && !strcmp(ptr,"plain"))
	    {
	    return 0;
	    }

	/** Lookup attributes, if necessary **/
	for(i=0;i<RPT_NUM_ATTR;i++)
	    {
	    ptr=NULL;
            stAttrValue(st,NULL,&ptr,i);
	    if (ptr)
		{
		for(j=0;j<RPT_NUM_ATTR;j++)
		    {
		    if (!strcmp(attrnames[j],ptr))
			{
			stylemask |= (1<<j);
			break;
			}
		    }
		}
	    else
		{
		break;
		}
	    }

    return stylemask;
    }


/*** rpt_internal_CheckFormats - check for new datetime/money/null formats or
 *** restore original formats.  Limit on formatting strings is 31 chars, plus
 *** one null terminator.
 ***/
int
rpt_internal_CheckFormats(pStructInf inf, char* savedmfmt, char* saveddfmt, char* savednfmt, int restore)
    {
    char* newmfmt=NULL;
    char* newdfmt=NULL;
    char* newnfmt=NULL;
    char* oldfmt;

    	/** Restoring or saving? **/
	if (restore)
	    {
	    if (*savedmfmt) mssSetParam("mfmt",savedmfmt);
	    if (*saveddfmt) mssSetParam("dfmt",saveddfmt);
	    if (*savednfmt) mssSetParam("nfmt",savednfmt);
	    }
	else
	    { 
	    /** Default - no new, so no saved **/
	    *savedmfmt = 0;
	    *saveddfmt = 0;
	    *savednfmt = 0;

	    /** Lookup possible 'dateformat','moneyformat' **/
	    stAttrValue(stLookup(inf,"dateformat"),NULL,&newdfmt,0);
	    stAttrValue(stLookup(inf,"moneyformat"),NULL,&newmfmt,0);
	    stAttrValue(stLookup(inf,"nullformat"),NULL,&newnfmt,0);
	    if (newdfmt) 
	        {
		oldfmt = (char*)mssGetParam("dfmt");
		if (!oldfmt) oldfmt = obj_default_date_fmt;
		memccpy(saveddfmt, oldfmt, 0, 31);
		saveddfmt[31] = 0;
		mssSetParam("dfmt",newdfmt);
		}
	    if (newmfmt) 
	        {
		oldfmt = (char*)mssGetParam("mfmt");
		if (!oldfmt) oldfmt = obj_default_money_fmt;
		memccpy(savedmfmt, oldfmt, 0, 31);
		savedmfmt[31] = 0;
		mssSetParam("mfmt",newmfmt);
		}
	    if (newnfmt)
	        {
		oldfmt = (char*)mssGetParam("nfmt");
		if (!oldfmt) oldfmt = obj_default_null_fmt;
		memccpy(savednfmt, oldfmt, 0, 31);
		savednfmt[31] = 0;
		mssSetParam("nfmt",newnfmt);
		}
	    }

    return 0;
    }


/*** rpt_internal_CheckGoto - check for xpos/ypos settings for a field or a
 *** comment element.
 ***/
int
rpt_internal_CheckGoto(pRptSession rs, pStructInf object, int container_handle)
    {
    int xpos = -1;
    int ypos = -1;
    int relypos = -1;

    	/** Check for ypos and xpos **/
	stAttrValue(stLookup(object, "ypos"),&ypos,NULL,0);
	stAttrValue(stLookup(object, "xpos"),&xpos,NULL,0);
	stAttrValue(stLookup(object, "relypos"),&relypos,NULL,0);

	/** If ypos is set, do it first **/
	if (ypos != -1) prtSetVPos(container_handle, (double)ypos);

	/** Next check xpos **/
	if (xpos != -1) prtSetHPos(container_handle, (double)xpos);

	/** Relative y position? **/
	/*if (relypos != -1) prtSetRelVPos(rs->PSession, (double)relypos);*/

    return 0;
    }


#if 00
/*** rpt_internal_CheckFont - check for font setting change or restore an
 *** old font setting.
 ***/
int
rpt_internal_CheckFont(pRptSession rs, pStructInf object, char** saved_font)
    {
    char* new_font = NULL;

    	/** If saved font is valid, restore. **/
	if (saved_font && *saved_font && **saved_font)
	    {
	    prtSetFont(rs->PSession, *saved_font);
	    }
	else
	    {
	    /** Otherwise, find a possible new font setting. **/
	    stAttrValue(stLookup(object,"font"),NULL,&new_font,0);
	    if (new_font && *new_font)
	        {
		if (saved_font) *saved_font = prtGetFont(rs->PSession);
		prtSetFont(rs->PSession, new_font);
		}
	    }

    return 0;
    }
#endif

/*** rpt_internal_ProcessAggregates - checks for aggregate computations in 
 *** the query, and calculates the current row into the aggregate functions.
 ***/
int
rpt_internal_ProcessAggregates(pQueryConn qy)
    {
    int i;
    pExpression a_exp, wh_exp;

    	/** Search through sub-objects of the report/query element **/
	for(i=0;i<qy->AggregateExpList.nItems;i++)
	    {
	    wh_exp = (pExpression)(qy->AggregateWhereList.Items[i]);
	    a_exp = (pExpression)(qy->AggregateExpList.Items[i]);
	    if (wh_exp)
	        {
	        expModifyParam(qy->ObjList, "this", (void*)qy);
		if (expEvalTree(wh_exp, qy->ObjList) < 0)
		    {
		    mssError(0,"RPT","Error evaluating where= in aggregate");
		    return -1;
		    }
		if ((wh_exp->Flags & EXPR_F_NULL) || wh_exp->DataType != DATA_T_INTEGER || wh_exp->Integer == 0)
		    {
		    continue;
		    }
		}
	    expModifyParam(qy->ObjList, "this", (void*)qy);
	    expUnlockAggregates(a_exp);
	    if (expEvalTree(a_exp, qy->ObjList) < 0)
	        {
		mssError(0,"RPT","Error evaluating compute= in aggregate");
		return -1;
		}
	    }

    return 0;
    }


/*** rpt_internal_QyGetAttrType - get the attribute type of a result column
 *** in a query.  This is used as a passthrough to objGetAttrType when the 
 *** column is not an aggregate computation expression.
 ***/
int
rpt_internal_QyGetAttrType(void* qyobj, char* attrname)
    {
    pObject obj = ((pQueryConn)qyobj)->QueryItem;
    pQueryConn qy = (pQueryConn)qyobj;
    int n,i;
    pExpression exp;
    pStructInf subitem;

    	/** Search for aggregates first. **/
	n = 0;
	for(i=0;i<qy->UserInf->nSubInf;i++)
	    {
	    subitem = qy->UserInf->SubInf[i];
	    if (stStructType(subitem) == ST_T_SUBGROUP && !strcmp(subitem->UsrType,"report/aggregate"))
	        {
	    	if (!strcmp(subitem->Name, attrname))
	            {
		    exp = (pExpression)(qy->AggregateExpList.Items[n]);
		    return exp->DataType;
		    }
	   	n++;
	    	}
	    }

	/** Determine whether query is active? **/
	if (!strcmp(attrname,"ls__isactive")) return DATA_T_INTEGER;

    	/** Return unavailable if object is NULL. **/
	if (obj == NULL) return DATA_T_UNAVAILABLE;

    return objGetAttrType(obj,attrname);
    }


/*** rpt_internal_QyGetAttrValue - return the attribute value of a result
 *** column, and then reset the aggregate counter if it was an aggregate
 *** value.
 ***/
int
rpt_internal_QyGetAttrValue(void* qyobj, char* attrname, int datatype, pObjData data_ptr)
    {
    pObject obj = ((pQueryConn)qyobj)->QueryItem;
    pQueryConn qy = (pQueryConn)qyobj;
    int n,i;
    pExpression exp;
    pStructInf subitem;

    	/** Free existing query conn data buf? **/
	if (qy->DataBuf)
	    {
	    nmSysFree(qy->DataBuf);
	    qy->DataBuf = NULL;
	    }

    	/** Search for aggregates first. **/
	n = 0;
	for(i=0;i<qy->UserInf->nSubInf;i++)
	    {
	    subitem = qy->UserInf->SubInf[i];
	    if (stStructType(subitem) == ST_T_SUBGROUP && !strcmp(subitem->UsrType,"report/aggregate"))
	        {
	    	if (!strcmp(subitem->Name, attrname))
	            {
		    exp = (pExpression)(qy->AggregateExpList.Items[n]);
		    if (datatype != exp->DataType && !(exp->Flags & EXPR_F_NULL))
			{
			mssError(1,"RPT","Type mismatch accessing query property '%s' [requested=%s, actual=%s]",
				attrname, datatype, exp->DataType);
			return -1;
			}
		    if (!(exp->Flags & EXPR_F_NULL)) switch(exp->DataType)
		        {
			case DATA_T_INTEGER:	data_ptr->Integer = exp->Integer; break;
			case DATA_T_DOUBLE:	data_ptr->Double = exp->Types.Double; break;
			case DATA_T_STRING: 
			    qy->DataBuf = (char*)nmSysMalloc(strlen(exp->String)+1);
			    data_ptr->String = qy->DataBuf;
			    strcpy(qy->DataBuf, exp->String);
			    break;
			case DATA_T_MONEY: 
			    qy->DataBuf = (char*)nmSysMalloc(sizeof(MoneyType));
			    memcpy(qy->DataBuf, &(exp->Types.Money), sizeof(MoneyType));
			    data_ptr->Money = (pMoneyType)(qy->DataBuf);
			    break;
			case DATA_T_DATETIME: 
			    qy->DataBuf = (char*)nmSysMalloc(sizeof(DateTime));
			    memcpy(qy->DataBuf, &(exp->Types.Date), sizeof(DateTime));
			    data_ptr->DateTime = (pDateTime)(qy->DataBuf);
			    break;
			default: return -1;
			}
		    if (qy->AggregateDoReset.Items[n])
		        {
		        expResetAggregates(exp, -1);
		        expEvalTree(exp,qy->ObjList);
			}
		    return (exp->Flags & EXPR_F_NULL)?1:0;
		    }
	   	n++;
	    	}
	    }

	/** Determine whether query is active? **/
	if (!strcmp(attrname,"ls__isactive"))
	    {
	    if (qy->Query) data_ptr->Integer = 1;
	    else data_ptr->Integer = 0;
	    return 0;
	    }

	/** Otherwise, if inactive, say so. **/
	if (qy->Query == NULL)
	    {
	    mssError(1,"RPT","Query '%s' is not active.",qy->Name);
	    return -1;
	    }

    	/** Return 1 if object is NULL. **/
	if (obj == NULL) return 1;

    return objGetAttrValue(obj, attrname, datatype, data_ptr);
    }

/*** rpt_internal_PrepareQuery - perform all operations necessary to prepare
 *** for the retrieval of a query result set.  This is done in both the 
 *** tabular and free-form result set types.  The function returns the query
 *** connection structure extracted from the hash table 'queryinf', with that
 *** structure set up with an open ObjectSystem query in ->Query.
 ***/
pQueryConn
rpt_internal_PrepareQuery(pRptData inf, pStructInf object, pRptSession rs, int index)
    {
    pQueryConn qy;
    /*char* newsql;*/
    int i,cnt,v;
    char* ptr;
    char* src=NULL;
    char* sql;
    XArray links;
    pQueryConn lqy;
    pStructInf ui;
    char* lvalue;
    char* cname;
    char nbuf[32][32];
    int t,n;
    pXString sql_str;
    pXString newsql;
    pDateTime dt;
    pMoneyType m;
    double dbl;
    int inner_mode = 0;
    int outer_mode = 0;
    char* endptr;

	/** Lookup the database query information **/
	stAttrValue(stLookup(object,"source"),NULL,&src,index);
	if (!src)
	    {
            mssError(1,"RPT","Source required for table/form '%s'", object->Name);
	    return NULL;
	    }
	qy = (pQueryConn)xhLookup(rs->Queries, src);
	if (!qy)
	    {
            mssError(1,"RPT","Source '%s' given for table/form '%s' is undefined", src, object->Name);
	    return NULL;
	    }

	/** Check for a mode entry **/
	if (stAttrValue(stLookup(object,"mode"),NULL,&ptr,index) >= 0)
	    {
	    if (!strcmp(ptr,"outer")) outer_mode = 1;
	    if (!strcmp(ptr,"inner")) inner_mode = 1;
	    }

	/** Requested an object for inner mode that isn't open that way? **/
	if (inner_mode && !qy->IsInnerOuter)
	    {
	    mssError(1,"RPT","Inner-mode '%s' must have a corresponding outer-mode form", object->Name);
	    return NULL;
	    }

	/** Query object busy? **/
	if (qy->Query != NULL && (!qy->IsInnerOuter || !inner_mode))
	    {
	    mssError(1,"RPT","Source query '%s' is busy, form/table '%s' can't use it", src, object->Name);
	    return NULL;
	    }

	if (!inner_mode) qy->IsInnerOuter = outer_mode;

	/** Ok, if inner mode, we don't have to start the query.  It's already in progress **/
	if (inner_mode) return qy;

	/** Construct the SQL statement, doing any necessary link substitution **/
	/** First, we find the sql itself for the query **/
	sql=NULL;
	stAttrValue(stLookup(qy->UserInf,"sql"),NULL,&sql,0);
	if (!sql)
	    {
            mssError(1,"RPT","Table/Form query source '%s' does not specify sql=", src);
	    return NULL;
	    }

	/** Do subst on the SQL? **/
	sql_str = rpt_internal_SubstParam(inf, sql);
	sql = sql_str->String;
	cnt = strlen(sql);

	/** Next, we need to piece together the linkages to other queries **/
	xaInit(&links,16);
	for(i=0;i<qy->UserInf->nSubInf;i++)
	    {
	    ui = qy->UserInf->SubInf[i];
	    if (!strcmp(ui->Name,"link"))
		{
		if (i >= 32)
		    {
		    mssError(1,"RPT","Too many links in query '%s' (max 32)",src);
		    xsDeInit(sql_str);
		    nmFree(sql_str, sizeof(XString));
	    	    return NULL;
		    }
		ptr=NULL;
		stAttrValue(ui,NULL,&ptr,0);
		if (!ptr)
		    {
            	    mssError(1,"RPT","Table/Form source query linkage type mismatch in query '%s'", src);
		    xsDeInit(sql_str);
		    nmFree(sql_str, sizeof(XString));
	    	    return NULL;
		    }
		lqy=(pQueryConn)xhLookup(rs->Queries,ptr);
		if (!lqy)
		    {
		    mssError(1,"RPT","Undefined linkage in table/form source query '%s' (%s)",src, ptr);
		    xsDeInit(sql_str);
		    nmFree(sql_str, sizeof(XString));
		    return NULL;
		    }
		stAttrValue(ui,NULL,&cname,1);
		if (!cname)
		    {
		    mssError(1,"RPT","Column name in table/form source query '%s' linkage '%s' is undefined", src, ptr);
		    xsDeInit(sql_str);
		    nmFree(sql_str, sizeof(XString));
		    return NULL;
		    }
		lvalue=NULL;
		if (!lqy->QueryItem)
		    {
		    mssError(1,"RPT","Table/Form source query '%s' linkage '%s' not active", src,ptr);
		    xsDeInit(sql_str);
		    nmFree(sql_str, sizeof(XString));
		    return NULL;
		    }
		t = objGetAttrType(lqy->QueryItem, cname);
		if (t < 0)
		    {
		    mssError(1,"RPT","Unknown column '%s' in table/form source query '%s' linkage '%s'", cname, src, ptr);
		    xsDeInit(sql_str);
		    nmFree(sql_str, sizeof(XString));
		    return NULL;
		    }
		switch(t)
		    {
		    case DATA_T_INTEGER:
		        if (objGetAttrValue(lqy->QueryItem, cname, DATA_T_INTEGER, POD(&n)) == 1)
		            snprintf(nbuf[i],32,"%s",rpt_internal_GetNULLstr());
		        else
		            snprintf(nbuf[i],32,"%d",n);
		        lvalue = nbuf[i];
			break;

		    case DATA_T_STRING:
		        if (objGetAttrValue(lqy->QueryItem, cname, DATA_T_STRING, POD(&lvalue)) == 1)
		            {
		            snprintf(nbuf[i],32,"%s",rpt_internal_GetNULLstr());
			    lvalue = nbuf[i];
			    }
			break;

		    case DATA_T_DATETIME:
		        if (objGetAttrValue(lqy->QueryItem, cname, DATA_T_DATETIME, POD(&dt)) == 1 || dt == NULL)
		            snprintf(nbuf[i],32,"%s",rpt_internal_GetNULLstr());
			else
			    snprintf(nbuf[i],32,"%s",objDataToStringTmp(DATA_T_DATETIME, dt, 0));
			lvalue = nbuf[i];
			break;

		    case DATA_T_MONEY:
		        if (objGetAttrValue(lqy->QueryItem, cname, DATA_T_MONEY, POD(&m)) == 1 || dt == NULL)
			    snprintf(nbuf[i],32,"%s",rpt_internal_GetNULLstr());
			else
			    snprintf(nbuf[i],32,"%s",objDataToStringTmp(DATA_T_MONEY, m, 0));
			lvalue = nbuf[i];
			break;

		    case DATA_T_DOUBLE:
		        if (objGetAttrValue(lqy->QueryItem, cname, DATA_T_DOUBLE, POD(&dbl)) == 1)
			    snprintf(nbuf[i],32,"%s",rpt_internal_GetNULLstr());
			else
			    snprintf(nbuf[i],32,"%f",dbl);
			lvalue = nbuf[i];
			break;

		    default:
		        lvalue = "";
			break;
		    }
		cnt+=(strlen(lvalue)+4);
		xaAddItem(&links,lvalue);
		}
	    }

	/** Now that we've got the linkages & values, put the sql together. **/
	newsql = (pXString)nmMalloc(sizeof(XString));
	xsInit(newsql);
	cnt=0;
	while(*sql)
	    {
	    if (sql[0] == '&' && (sql[1] >= '0' && sql[1] <= '9'))
		{
		/*newsql[cnt++] = '"';*/
		v = strtol(sql+1,&endptr,10);
		if (v == 0 || v > links.nItems)
		    {
		    mssError(1,"RPT","Parameter error in table/form '%s' source '%s' sql", object->Name, src);
		    nmSysFree(newsql);
		    xsDeInit(sql_str);
		    nmFree(sql_str, sizeof(XString));
		    return NULL;
		    }
		xsConcatenate(newsql, links.Items[v-1], -1);
		/*strcpy(newsql+cnt,links.Items[v-1]);*/
		cnt+=strlen(links.Items[v-1]);
		/*newsql[cnt++] = '"';*/
		sql = endptr;
		}
	    else
		{
		xsConcatenate(newsql, sql, 1);
		/*newsql[cnt++] = *sql;*/
	        sql++;
		}
	    }
	/*newsql[cnt] = 0;*/
	xsDeInit(sql_str);
	nmFree(sql_str, sizeof(XString));

	/** Ok, now issue the query. **/
	qy->Query = objMultiQuery(rs->ObjSess, newsql->String);
        /*nmSysFree(newsql);*/
	xsDeInit(newsql);
	nmFree(newsql,sizeof(XString));
	if (!qy->Query) 
	    {
	    mssError(0,"RPT","Could not open report/query '%s' SQL for form/table '%s'", src, object->Name);
	    return NULL;
	    }
	xaDeInit(&links);
    
    return qy;
    }


#if 00
/*** rpt_internal_DoSection - process a report section, which is an 
 *** abstract division in the report normally used simply to change the
 *** formatting style, such as margins, columns, etc.
 ***/
int
rpt_internal_DoSection(pRptData inf, pStructInf section, pRptSession rs, pQueryConn this_qy)
    {
    int style,oldstyle=0;
    char* title = NULL;
    int cols=1,colsep=4,lmargin=0,rmargin=0;
    int oldcols=-1, oldcolsep, oldlmargin=-1, oldrmargin;
    int oldspacing=-1,spacing;
    int err=0,i;
    char oldmfmt[32],olddfmt[32],oldnfmt[32];
    char* saved_font = NULL;
    int n;

	/** Get style information **/
	style = rpt_internal_GetStyle(section);
	rpt_internal_CheckFormats(section, oldmfmt, olddfmt, oldnfmt, 0);
	rpt_internal_CheckFont(rs,section,&saved_font);

	/** Check for set-page-number? **/
	if (stAttrValue(stLookup(section,"page"),&n,NULL,0) >= 0) prtSetPageNumber(rs->PSession, n);

	/** Title? **/
	stAttrValue(stLookup(section, "title"),NULL,&title,0);
	if (title)
	    {
	    oldstyle = prtGetAttr(rs->PSession);
	    prtSetAttr(rs->PSession, oldstyle | RS_TX_CENTER | RS_TX_BOLD);
	    prtWriteString(rs->PSession, title, -1);
	    prtSetAttr(rs->PSession, oldstyle);
	    prtWriteNL(rs->PSession);
	    }

	/** Set the style for the printer? **/
	if (style >= 0)
	    {
	    oldstyle = prtGetAttr(rs->PSession);
	    prtSetAttr(rs->PSession, style);
	    }

	/** Set line spacing? **/
	if (stAttrValue(stLookup(section,"linespacing"),&spacing,NULL,0) >= 0)
	    {
	    oldspacing = prtGetLineSpacing(rs->PSession);
	    prtSetLineSpacing(rs->PSession, spacing);
	    }

	/** Set margins? **/
	if (stAttrValue(stLookup(section,"margins"),&lmargin,NULL,0) >= 0)
	    {
	    prtGetLRMargins(rs->PSession, &oldlmargin, &oldrmargin);
	    stAttrValue(stLookup(section,"margins"),&rmargin,NULL,1);
	    prtSetLRMargins(rs->PSession, lmargin, rmargin);
	    }

	/** Set columns? **/
	if (stAttrValue(stLookup(section,"columns"),&cols,NULL,0) >= 0)
	    {
	    prtGetColumns(rs->PSession, &oldcols, &oldcolsep);
	    stAttrValue(stLookup(section,"colsep"),&colsep,NULL,0);
	    prtSetColumns(rs->PSession, cols, colsep, 0);
	    }

	/** Now do sub-components. **/
        for(i=0;i<section->nSubInf;i++) if (stStructType(section->SubInf[i]) == ST_T_SUBGROUP)
            {
            if (!strcmp("report/column",section->SubInf[i]->UsrType))
                {
                if (rpt_internal_DoField(inf, section->SubInf[i],rs,this_qy) <0) 
                    {
                    err = 1;
                    break;
                    }
                }
            else if (!strcmp("report/form",section->SubInf[i]->UsrType))
                {
                if (rpt_internal_DoForm(inf, section->SubInf[i],rs) <0)
                    {
                    err = 1;
                    break;
                    }
                }
            else if (!strcmp("report/comment",section->SubInf[i]->UsrType))
                {
                if (rpt_internal_DoComment(inf, section->SubInf[i],rs) <0)
                    {
                    err = 1;
                    break;
                    }
                }
            else if (!strcmp("report/data",section->SubInf[i]->UsrType))
                {
                if (rpt_internal_DoData(inf, section->SubInf[i],rs) <0)
                    {
                    err = 1;
                    break;
                    }
                }
            else if (!strcmp("report/table",section->SubInf[i]->UsrType))
                {
                if (rpt_internal_DoTable(inf, section->SubInf[i],rs) <0)
                    {
                    err = 1;
                    break;
                    }
                }
            else if (!strcmp("report/section",section->SubInf[i]->UsrType))
                {
                if (rpt_internal_DoSection(inf, section->SubInf[i],rs,this_qy) <0)
                    {
                    err = 1;
                    break;
                    }
                }
            }

	/** Reset columns? **/
	if (oldcols != -1)
	    {
	    prtSetColumns(rs->PSession, oldcols, oldcolsep, 0);
	    }

	/** Reset columns? **/
	if (oldspacing != -1)
	    {
	    prtSetLineSpacing(rs->PSession, oldspacing);
	    }

	/** Reset margins? **/
	if (oldlmargin != -1)
	    {
	    prtSetLRMargins(rs->PSession, oldlmargin, oldrmargin);
	    }

	/** If style changed, change it back. **/
	if (style >= 0)
	    {
	    prtSetAttr(rs->PSession, oldstyle);
	    }
	rpt_internal_CheckFormats(section, oldmfmt, olddfmt, oldnfmt, 1);
	rpt_internal_CheckFont(rs,section,&saved_font);

    return err?-1:0;
    }
#endif

/*** rpt_internal_NextRecord_Parallel - processes the next row, but operates
 *** the queries in parallel instead of nested.  It keeps on returning more
 *** records until all sources have been exhausted.
 ***/
int
rpt_internal_NextRecord_Parallel(pRptActiveQueries this, pRptData inf, pStructInf object, pRptSession rs, int is_initial)
    {
    int more_records = 0;
    int i;
    int err = 0;

    	/** Scan through each query. **/
	for(i=0;i<this->Count;i++)
	    {
	    /** Need to activate the query? **/
	    if (this->Queries[i] == NULL)
	        {
	        this->Queries[i] = rpt_internal_PrepareQuery(inf, object, rs, i);
	        if (!this->Queries[i])
		    {
		    err = 1;
		    break;
		    }
		if (this->OuterMode[i]) this->Queries[i]->InnerExecCnt = 0;
	        if (this->InnerMode[i]) this->Queries[i]->InnerExecCnt++;
		if (!this->InnerMode[i]) this->Queries[i]->QueryItem = NULL;
		if (this->InnerMode[i] && !this->Queries[i]->Query)
		    {
		    err = 1;
		    mssError(1,"RPT","No outer-mode form corresponds to inner-mode '%s'",this->Names[i]);
		    break;
		    }
		}

	    /** Retrieve a record?  (or just check for one if outer mode unless 1st time) **/
	    if ((!this->OuterMode[i] || (this->Queries[i]->InnerExecCnt == 0 || !this->Queries[i]->QueryItem)))
	        {
		if (this->Queries[i]->QueryItem) objClose(this->Queries[i]->QueryItem);
		this->Queries[i]->QueryItem = objQueryFetch(this->Queries[i]->Query, 0400);
		expModifyParam(inf->ObjList, this->Queries[i]->Name, (void*)(this->Queries[i]));
		}
	    if (this->Queries[i] && this->Queries[i]->Query && this->Queries[i]->QueryItem) more_records = 1;
	    }

	/** If error, return -1 **/
	if (err) return -1;

	/** If no more records, return value is 1 **/
	if (!more_records) return 1;

    return 0;
    }


/*** rpt_internal_NextRecord - processes the next row, given a list of query
 *** connections, list of inner/outer flags, query count, and "stack pointer".
 *** Returns 0 if record valid, 1 if no more records, and -1 on error.
 ***/
int
rpt_internal_NextRecord(pRptActiveQueries this, pRptData inf, pStructInf object, pRptSession rs, int is_initial)
    {
    int id;
    int stack_ptr = 0;
    int err = 0;
    int new_item = 0;

    	/** Parallel queries?  Use parallel-mode nextrec if so. **/
	if (this->MultiMode == RPT_MM_PARALLEL)
	    {
	    return rpt_internal_NextRecord_Parallel(this,inf,object,rs,is_initial);
	    }

    	/** Loop while trying to find more rows **/
	/*printf("NEXTREC: Retrieving record for element '%s'\n",object->Name);*/
	while(!err)
	    {
	    /** Need to activate the query? **/
	    if (this->Queries[stack_ptr] == NULL)
	        {
		/*printf("  NEXTREC: Activating datasource '%s' O=%d I=%d\n", this->Names[stack_ptr], this->OuterMode[stack_ptr], this->InnerMode[stack_ptr]);*/
	        this->Queries[stack_ptr] = rpt_internal_PrepareQuery(inf, object, rs, stack_ptr);
	        if (!this->Queries[stack_ptr])
		    {
		    err = 1;
		    break;
		    }
		if (this->OuterMode[stack_ptr]) this->Queries[stack_ptr]->InnerExecCnt = 0;
	        if (this->InnerMode[stack_ptr]) this->Queries[stack_ptr]->InnerExecCnt++;
		if (!this->InnerMode[stack_ptr]) this->Queries[stack_ptr]->QueryItem = NULL;
		}

	    /** Need to fetch a query item? **/
	    /** GRB - note.  I put parens around first || in first clause, not sure about it **/
	    if ((this->Queries[stack_ptr]->Query && (this->Queries[stack_ptr]->QueryItem == NULL || (new_item && !this->OuterMode[stack_ptr]))) ||
	        (stack_ptr == this->Count-1 && !this->OuterMode[stack_ptr] && (!is_initial || !this->InnerMode[stack_ptr])))
	        {
		/*printf("  NEXTREC: Fetching item from source '%s' NI=%d O=%d QI=%d\n", this->Names[stack_ptr], new_item, this->OuterMode[stack_ptr], this->Queries[stack_ptr]->QueryItem);*/
		/*printf("  NEXTREC: Fetching item from source '%s' O=%d I=%d INIT=%d\n", this->Names[stack_ptr], this->OuterMode[stack_ptr], this->InnerMode[stack_ptr], is_initial);*/
		new_item = 0;
		if (this->Queries[stack_ptr]->QueryItem) objClose(this->Queries[stack_ptr]->QueryItem);
		this->Queries[stack_ptr]->QueryItem = objQueryFetch(this->Queries[stack_ptr]->Query, 0400);
		id = expModifyParam(inf->ObjList, this->Queries[stack_ptr]->Name, (void*)(this->Queries[stack_ptr]));
		inf->ObjList->CurrentID = id;
		if (this->Queries[stack_ptr]->QueryItem == NULL)
		    {
		    /*printf("  NEXTREC: No more items!\n");*/
		    if (!this->InnerMode[stack_ptr])
		        {
			objQueryClose(this->Queries[stack_ptr]->Query);
		        this->Queries[stack_ptr]->Query = NULL;
			this->Queries[stack_ptr] = NULL;
			}
		    stack_ptr--;
		    new_item = 1;
		    if (stack_ptr < 0) return 1;
		    continue;
		    }
		this->Queries[stack_ptr]->RecordCnt++;
		this->Flags[stack_ptr] |= RPT_A_F_NEEDUPDATE;
		stack_ptr++;
		if (stack_ptr == this->Count || this->MultiMode == RPT_MM_MULTINESTED) return 0;
		continue;
		}

	    /** New item not retrieved because of outer mode?  Return OK if so. **/
	    if ((new_item || stack_ptr == this->Count-1) && this->OuterMode[stack_ptr]) 
		return 0;
	    else if (new_item)
		return 1;

	    /** Ok, valid item still, or outer mode so we don't bother **/
	    new_item = 0;
	    stack_ptr++;
	    if (stack_ptr == this->Count) return 0;
	    }

    return err?-1:0;
    }


/*** rpt_internal_UseRecord - performs processing on the query objects that
 *** occurs when the row is actually printed on the reporting output.  One example
 *** of this is updating the row aggregates as needed.
 ***/
int
rpt_internal_UseRecord(pRptActiveQueries this)
    {
    int i;
    int err = 0;

    	/** Run an update aggregates on those that are flagged as need update **/
	for(i=0;i<this->Count;i++)
	    {
	    if (this->Flags[i] & RPT_A_F_NEEDUPDATE)
	        {
		this->Flags[i] &= ~RPT_A_F_NEEDUPDATE;
	        if (rpt_internal_ProcessAggregates(this->Queries[i]) < 0)
		    {
		    err = 1;
		    break;
		    }
		}
	    }

    return err?-1:0;
    }


/*** rpt_internal_Activate - activates queries (ones that are non-inner) and
 *** retrieves the first record(s) from the source(s).
 ***/
pRptActiveQueries
rpt_internal_Activate(pRptData inf, pStructInf object, pRptSession rs)
    {
    pRptActiveQueries ac;
    char* ptr;
    int err = 0;
    int i;

    	/** Allocate the structure **/
	ac = (pRptActiveQueries)nmMalloc(sizeof(RptActiveQueries));
	if (!ac) return NULL;

	/** Is this a parallel query? **/
	ac->MultiMode = RPT_MM_NESTED;
	if (stAttrValue(stLookup(object, "multimode"), NULL, &ptr, 0) >= 0)
	    {
	    if (!strcmp(ptr,"parallel")) ac->MultiMode = RPT_MM_PARALLEL;
	    else if (!strcmp(ptr,"nested")) ac->MultiMode = RPT_MM_NESTED;
	    else if (!strcmp(ptr,"multinested")) ac->MultiMode = RPT_MM_MULTINESTED;
	    else
	        {
		mssError(1,"RPT","Invalid multimode <%s>.  Valid types = nested,parallel,multinested", ptr);
		return NULL;
		}
	    }

	/** Scan through the object's inf looking for source, etc **/
	ac->Count = 0;
	while(1)
	    {
	    /** Find a source to activate... **/
	    ptr = NULL;
	    stAttrValue(stLookup(object, "source"), NULL, &ptr, ac->Count);
	    if (!ptr) break;
	    ac->Names[ac->Count] = ptr;

	    /** Get inner/outer mode information **/
	    ac->InnerMode[ac->Count] = 0;
	    ac->OuterMode[ac->Count] = 0;
	    if (stAttrValue(stLookup(object, "mode"), NULL, &ptr, ac->Count) >= 0)
	        {
		if (!strcmp(ptr,"inner")) ac->InnerMode[ac->Count] = 1;
		if (!strcmp(ptr,"outer")) 
		    {
		    if (!strcmp(object->UsrType,"report/table"))
		        {
			err = 1;
			mssError(1,"RPT","report/table '%s' cannot be mode=outer",object->Name);
			break;
			}
		    ac->OuterMode[ac->Count] = 1;
		    }
		}

	    /** If not inner mode, try to activate and fetch first row **/
	    if (!ac->InnerMode[ac->Count]) 
	        {
		/*expAddParamToList(inf->ObjList, ac->Names[ac->Count], NULL, EXPR_O_CURRENT);
		expSetParamFunctions(inf->ObjList, ac->Names[ac->Count], rpt_internal_QyGetAttrType, rpt_internal_QyGetAttrValue, NULL);*/
		}
	    ac->Queries[ac->Count] = NULL;
#if 00
	    ac->Queries[ac->Count] = rpt_internal_PrepareQuery(inf, object, rs, ac->Count);
	    if (!ac->Queries[ac->Count])
		{
		err = 1;
		break;
		}
	
	    /** Now fetch the first item... **/
	    if (!ac->InnerMode[ac->Count]) 
	        {
		/** Add an object-list item for this datasource/query **/
		expAddParamToList(inf->ObjList, ac->Names[ac->Count], NULL, EXPR_O_CURRENT);
		expSetParamFunctions(inf->ObjList, ac->Names[ac->Count], rpt_internal_QyGetAttrType, rpt_internal_QyGetAttrValue, NULL);

		/** Fetch the first row. **/
		ac->Queries[ac->Count]->QueryItem = NULL;
	        /*ac->Queries[ac->Count]->QueryItem = objQueryFetch(ac->Queries[ac->Count]->Query, 0400);
		expModifyParam(inf->ObjList, ac->Queries[ac->Count]->Name, ac->Queries[ac->Count]->QueryItem);*/
		if (ac->OuterMode[ac->Count]) ac->Queries[ac->Count]->InnerExecCnt = 0;
		}
	    else
	        {
		/** Let any outer query know the inner form/table got run **/
	        ac->Queries[ac->Count]->InnerExecCnt++;
		}
#endif
	    ac->Count++;
	    }

	/** If err, close up the queries and return NULL **/
	if (err)
	    {
	    for(i=ac->Count-1;i>=0;i--)
	        {
		objQueryClose(ac->Queries[i]->Query);
		ac->Queries[i]->Query = NULL;
		}
	    nmFree(ac, sizeof(RptActiveQueries));
	    return NULL;
	    }

    return ac;
    }


/*** rpt_internal_Deactivate - shut down queries being processed via the
 *** rptactivequeries structure and activated via rpt_internal_Activate.
 *** Of course, don't shut down queries accessed via an inner form
 ***/
int
rpt_internal_Deactivate(pRptData inf, pRptActiveQueries ac)
    {
    int i;

    	/** Close the queries first. **/
	for(i=ac->Count-1;i>=0;i--) if (!ac->InnerMode[i])
	    {
	    if (ac->Queries[i] && ac->Queries[i]->QueryItem) 
	        {
		objClose(ac->Queries[i]->QueryItem);
	        ac->Queries[i]->QueryItem = NULL;
		}
	    /*expRemoveParamFromList(inf->ObjList, ac->Names[i]);*/
	    if (ac->Queries[i] && ac->Queries[i]->Query)
	        {
		objQueryClose(ac->Queries[i]->Query);
	        ac->Queries[i]->Query = NULL;
		}
	    }
	
	/** Free the data structure. **/
	nmFree(ac, sizeof(RptActiveQueries));

    return 0;
    }


/*** rpt_internal_DoTableRow - process a single row object in a table.
 ***/
int
rpt_internal_DoTableRow(pRptData inf, pStructInf tablerow, pRptSession rs, int numcols, int table_handle)
    {
    int tablerow_handle, tablecell_handle, area_handle;
    int i;
    pStructInf subinf;
    int is_header, is_footer, flags;
    int is_cellrow, is_generalrow;
    int rval;
    pPrtBorder tb,bb,lb,rb,ib,ob;
    double dbl;
    char oldmfmt[32],olddfmt[32],oldnfmt[32];

	rval = rpt_internal_CheckCondition(inf,tablerow);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get table row object flags **/
	is_header = rpt_internal_GetBool(tablerow,"header",0,0);
	is_footer = rpt_internal_GetBool(tablerow,"footer",0,0);
	flags = 0;
	if (rpt_internal_GetBool(tablerow, "allowbreak", 1, 0)) flags |= PRT_OBJ_U_ALLOWBREAK;
	if (rpt_internal_GetBool(tablerow, "fixedsize", 0, 0)) flags |= PRT_OBJ_U_FIXEDSIZE;

	/** Border settings? **/
	tb=bb=lb=rb=ib=ob=NULL;
	if (rpt_internal_GetDouble(tablerow, "outerborder", &dbl, 0) == 0)
	    ob = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(tablerow, "innerborder", &dbl, 0) == 0)
	    ib = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(tablerow, "topborder", &dbl, 0) == 0)
	    tb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(tablerow, "bottomborder", &dbl, 0) == 0)
	    bb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(tablerow, "leftborder", &dbl, 0) == 0)
	    lb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(tablerow, "rightborder", &dbl, 0) == 0)
	    rb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);

	/** Create the table row **/
	tablerow_handle = prtAddObject(table_handle, PRT_OBJ_T_TABLEROW, -1,-1,-1,-1, flags, "header", is_header, "footer", is_footer, 
		"outerborder", ob, "innerborder", ib, "topborder", tb, "bottomborder", bb, "leftborder", lb, "rightborder", rb, NULL);
	if (tablerow_handle < 0)
	    {
	    mssError(0,"RPT","could not build table row object '%s'", tablerow->Name);
	    return -1;
	    }

	/** Set the style for the table **/
	rpt_internal_SetStyle(inf, tablerow, rs, tablerow_handle);
	rpt_internal_SetMargins(tablerow, tablerow_handle);

	/** Output data formats **/
	rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 0);

	/** Loop through subobjects **/
	is_cellrow = is_generalrow = 0;
	for(i=0;i<tablerow->nSubInf;i++) if (stStructType(tablerow->SubInf[i]) == ST_T_SUBGROUP)
	    {
	    subinf = tablerow->SubInf[i];

	    /** Is this a cell object, or do we have a general-purpose row here? **/
	    if (!strcmp(subinf->UsrType,"report/table-cell"))
		{
		if (is_generalrow)
		    {
		    mssError(1,"RPT","cannot mix a general purpose row (%s) contents with a cell (%s)", tablerow->Name, subinf->Name);
		    prtEndObject(tablerow_handle);
		    rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);
		    return -1;
		    }
		is_cellrow = 1;

		/** Init the cell **/
		tablecell_handle = prtAddObject(tablerow_handle, PRT_OBJ_T_TABLECELL, -1,-1,-1,-1, flags, NULL);
		if (tablecell_handle < 0)
		    {
		    mssError(0,"RPT","could not build table cell object '%s'", subinf->Name);
		    prtEndObject(tablerow_handle);
		    rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);
		    return -1;
		    }

		/** Does cell have a builtin value?  If so, don't treat the cell as a general
		 ** purpose container, but instead put an Area in the cell and treat the cell's
		 ** config as a report/data element.
		 **/
		if (stLookup(subinf, "value"))
		    {
		    area_handle = prtAddObject(tablecell_handle, PRT_OBJ_T_AREA, 0,0,-1,-1, flags, NULL);
		    if (area_handle < 0)
			{
			mssError(0,"RPT","problem constructing cell object '%s' (error adding area object)", subinf->Name);
			prtEndObject(tablecell_handle);
			prtEndObject(tablerow_handle);
			rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);
			return -1;
			}
		    /*rpt_internal_SetStyle(inf, subinf, rs, area_handle);*/
		    rpt_internal_SetMargins(subinf, area_handle);
		    if (rpt_internal_DoData(inf, subinf, rs, area_handle) < 0)
			{
			mssError(0,"RPT","problem constructing cell object '%s' (error doing area content)", subinf->Name);
			prtEndObject(area_handle);
			prtEndObject(tablecell_handle);
			prtEndObject(tablerow_handle);
			rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);
			return -1;
			}
		    prtEndObject(area_handle);
		    }
		else
		    {
		    /** general purpose container here. **/
		    if (rpt_internal_DoContainer(inf, subinf, rs, tablecell_handle) < 0)
			{
			mssError(0,"RPT","problem constructing cell object '%s' (error doing content)", subinf->Name);
			prtEndObject(tablecell_handle);
			prtEndObject(tablerow_handle);
			rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);
			return -1;
			}
		    }

		/** End the cell **/
		prtEndObject(tablecell_handle);
		}
	    else
		{
		if (is_cellrow)
		    {
		    mssError(1,"RPT","cannot mix a cell row (%s) contents with a general purpose object (%s)", tablerow->Name, subinf->Name);
		    prtEndObject(tablerow_handle);
		    rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);
		    return -1;
		    }
		is_generalrow = 1;
		if (rpt_internal_DoContainer(inf, tablerow, rs, tablerow_handle) < 0)
		    {
		    mssError(0,"RPT","problem constructing row object '%s' (error doing content)", tablerow->Name);
		    prtEndObject(tablerow_handle);
		    rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);
		    return -1;
		    }
		break; /* did all objects in DoContainer */
		}
	    }

	/** Finish the row **/
	prtEndObject(tablerow_handle);
	rpt_internal_CheckFormats(tablerow, oldmfmt, olddfmt, oldnfmt, 1);

    return 0;
    }


/*** rpt_internal_DoTable - process tabular data from the database and
 *** output using the print driver's table feature.
 ***/
int
rpt_internal_DoTable(pRptData inf, pStructInf table, pRptSession rs, int container_handle)
    {
    char* ptr;
    int titlebar=1;
    int style,oldstyle=0;
    int cnt,v,colmask,len,i;
    pQueryConn qy = NULL;
    char* cname;
    char* cname2;
    pStructInf ui;
    int t,n;
    char nbuf[32];
    void* p;
    double dbl;
    char oldmfmt[32],olddfmt[32],oldnfmt[32];
    char* saved_font = NULL;
    int err = 0;
    int reclimit = -1;
    double colsep = 1.0;
    pXArray xa = (pXArray)(table->UserData);
    pExpression exp;
    pRptActiveQueries ac;
    int reccnt;
    int rval;
    int is_rel_cols = 1;
    int no_data_msg = 1;
    int lower_sep = 0;
    int has_source = 0;
    double cwidths[64];
    unsigned char cflagslist[64];
    unsigned char hflagslist[64];
    int table_handle, tablerow_handle, tablecell_handle, area_handle;
    int flags;
    double x,y,w,h;
    int numcols;
    pPrtBorder outerborder=NULL, innerborder=NULL, shadow=NULL;
    pStructInf rowinf, summarize_for_inf;
    pRptUserData ud;
    int changed;

	rval = rpt_internal_CheckCondition(inf,table);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get table outer geometry **/
	if (rpt_internal_GetDouble(table, "x", &x, 0) < 0) x = -1;
	if (rpt_internal_GetDouble(table, "y", &y, 0) < 0) y = -1;
	if (rpt_internal_GetDouble(table, "width", &w, 0) < 0) w = -1;
	if (rpt_internal_GetDouble(table, "height", &h, 0) < 0) h = -1;

	/** Check for flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;
	if (rpt_internal_GetBool(table, "allowbreak", 1, 0)) flags |= PRT_OBJ_U_ALLOWBREAK;
	if (rpt_internal_GetBool(table, "fixedsize", 0, 0)) flags |= PRT_OBJ_U_FIXEDSIZE;

	/** Get column count and widths. **/
	numcols = 0;
	while(numcols < 64 && rpt_internal_GetDouble(table, "widths", &(cwidths[numcols]), numcols) == 0) 
	    numcols++;
	if (numcols == 0)
	    {
	    mssError(1,"RPT","Table '%s' must have valid width info specifying at least one column.", table->Name);
	    return -1;
	    }
	n = 0;
	stAttrValue(stLookup(table,"columns"),&n, NULL, 0);
	if (n != numcols)
	    {
	    mssError(1,"RPT","Table '%s' column count %d not specified or does not match number of column widths (%d).", table->Name, n, numcols);
	    return -1;
	    }

	/** Check for column separation amount **/
	if (rpt_internal_GetDouble(table, "colsep", &colsep, 0) != 0) colsep = 1.0;

	/** Check for borders / shadow **/
	if (rpt_internal_GetDouble(table, "outerborder", &dbl, 0) == 0)
	    outerborder = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(table, "innerborder", &dbl, 0) == 0)
	    innerborder = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(table, "shadow", &dbl, 0) == 0)
	    shadow = prtAllocBorder(1,0.0,0.0, dbl,0x000000);

	/** Create the table **/
	table_handle = prtAddObject(container_handle, PRT_OBJ_T_TABLE, x,y,w,h, flags, "numcols", numcols, "colwidths", cwidths, "colsep", colsep, "outerborder", outerborder, "innerborder", innerborder, "shadow", shadow, NULL);
	if (table_handle < 0)
	    {
	    mssError(0, "RPT", "Could not create table '%s'", table->Name);
	    return -1;
	    }

	/** Output data formats **/
	rpt_internal_CheckFormats(table, oldmfmt, olddfmt, oldnfmt, 0);

	/** Record-count limiter? **/
	stAttrValue(stLookup(table,"reclimit"),&reclimit,NULL,0);

	/** Set the style for the table **/
	rpt_internal_SetStyle(inf, table, rs, table_handle);
	rpt_internal_SetMargins(table, table_handle);

	/** Suppress "no data returned" message on 0 rows? **/
	no_data_msg = rpt_internal_GetBool(table, "nodatamsg", 1, 0);

	/** Check to see if table has sources specified. **/
	if (stLookup(table,"source")) has_source = 1;

	/** Start query if table has source(s), otherwise skip that. **/
	if (has_source)
	    {
	    /** Start the query. **/
	    if ((ac = rpt_internal_Activate(inf, table, rs)) == NULL)
	        {
		rpt_internal_CheckFormats(table, oldmfmt, olddfmt, oldnfmt, 1);
		prtEndObject(table_handle);
	        return -1;
	        }
    
	    /** Fetch the first row. **/
	    if ((rval = rpt_internal_NextRecord(ac, inf, table, rs, 1)) < 0)
	        {
		rpt_internal_CheckFormats(table, oldmfmt, olddfmt, oldnfmt, 1);
		prtEndObject(table_handle);
	        return -1;
	        }
    
	    /** No data? **/
	    if (!ac->Queries[0] || !ac->Queries[0]->QueryItem)
	        {
	        if (no_data_msg)
	            {
		    tablerow_handle = prtAddObject(table_handle, PRT_OBJ_T_TABLEROW, 0,0,-1,-1, 0, NULL);
		    area_handle = prtAddObject(tablerow_handle, PRT_OBJ_T_AREA, 0,0,-1,-1, 0, NULL);
		    prtWriteString(area_handle, "(no data returned)");
		    prtEndObject(area_handle);
		    prtEndObject(tablerow_handle);
		    }
	        rpt_internal_Deactivate(inf, ac);
		prtEndObject(table_handle);
		rpt_internal_CheckFormats(table, oldmfmt, olddfmt, oldnfmt, 1);
	        return 0;
	        }
	    }
	else
	    {
	    /*if (!table->UserData)
	        {
		mssError(1,"Table '%s' has no source, and no 'expressions=yes'", table->Name);
		prtEndObject(table_handle);
	        return -1;
		}*/
	    ac = NULL;
	    }
#if 00
	/** Decide which columns of the result set to use **/
	colmask = 0x7FFFFFFF;
	if (ac)
	    {
	    /** Determine which source to use. **/
	    if (ac->MultiMode == RPT_MM_MULTINESTED)
		qy = ac->Queries[0];
	    else
		qy = ac->Queries[ac->Count-1];

	    if ((ui=stLookup(table,"columns")))
	        {
	        colmask=0;
	        for(v=0,cname=objGetFirstAttr(qy->QueryItem);cname;v++,cname=objGetNextAttr(qy->QueryItem))
	            {
	            for(cnt=0;stAttrValue(ui,NULL,&cname2,cnt) >=0 && cname2;cnt++)
		        {
		        if (!strcmp(cname,cname2)) 
		            {
			    colmask |= (1<<v);
			    break;
			    }
			}
		    }
		}
	    }
#endif

	/** First, scan for any header rows in the table **/
	for(i=0;i<table->nSubInf;i++) if (stStructType(table->SubInf[i]) == ST_T_SUBGROUP)
	    {
	    rowinf = table->SubInf[i];
	    if (!strcmp(rowinf->UsrType,"report/table-row"))
		{
		if (rpt_internal_GetBool(rowinf,"header",0,0))
		    {
		    if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle) < 0)
			{
			err = 1;
			break;
			}
		    }
		}
	    }

	/** Read the result set and build the table **/
	reccnt = 0;
	if (!err) do  
	    {
	    if (ac) for(i=0;i<ac->Count;i++) if (ac->Queries[i]) qy = ac->Queries[i];
	    if (reclimit != -1 && reccnt >= reclimit) break;
	    if (ac && rpt_internal_UseRecord(ac) < 0)
	        {
		err = 1;
		break;
		}

	    /** Search for 'normal' rows **/
	    for(i=0;i<table->nSubInf;i++) if (stStructType(table->SubInf[i]) == ST_T_SUBGROUP)
		{
		rowinf = table->SubInf[i];
		if (!strcmp(rowinf->UsrType,"report/table-row"))
		    {
		    if (!rpt_internal_GetBool(rowinf,"header",0,0) && !rpt_internal_GetBool(rowinf,"summary",0,0))
			{
			if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle) < 0)
			    {
			    err = 1;
			    break;
			    }
			}
		    }
		}

	    if (err) break;
	    if (ac) rval = rpt_internal_NextRecord(ac, inf, table, rs, 0);
	    else rval = 1;

	    /** Emit summary rows? **/
	    for(i=0;i<table->nSubInf;i++) if (stStructType(table->SubInf[i]) == ST_T_SUBGROUP)
		{
		rowinf = table->SubInf[i];
		if (!strcmp(rowinf->UsrType,"report/table-row"))
		    {
		    if (rpt_internal_GetBool(rowinf,"summary",0,0))
			{
			if (rval != 0) 
			    {
			    /** End of table.  do the final summary. **/
			    if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle) < 0)
				{
				err = 1;
				break;
				}
			    }
			else
			    {
			    /** Not end of table; check to see if we need the summary record **/
			    summarize_for_inf = stLookup(rowinf, "summarize_for");
			    if (summarize_for_inf)
				{
				ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (int)(summarize_for_inf->UserData));
				if (expEvalTree(ud->Exp, inf->ObjList) < 0)
				    {
				    mssError(0,"RPT","Could not evaluate report/table-row '%s' summarize_for expression", rowinf->Name);
				    err = 1;
				    break;
				    }
				if (!ud->LastValue)
				    {
				    /** First record; not initialized.  don't issue summary **/
				    ud->LastValue = expAllocExpression();
				    ud->LastValue->NodeType = expDataTypeToNodeType(ud->Exp->DataType);
				    expCopyValue(ud->Exp, ud->LastValue, 1);
				    }
				else if (ud->LastValue->DataType != ud->Exp->DataType)
				    {
				    /** Data type changed.  Issue summary. **/
				    if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle) < 0)
					{
					err = 1;
					break;
					}
				    ud->LastValue->NodeType = expDataTypeToNodeType(ud->Exp->DataType);
				    expCopyValue(ud->Exp, ud->LastValue, 1);
				    }
				else
				    {
				    /** See if value changed. **/
				    changed = 0;
				    switch(ud->LastValue->DataType)
					{
					case DATA_T_INTEGER:
					    changed = ud->LastValue->Integer != ud->Exp->Integer;
					    break;
					case DATA_T_STRING:
					    changed = strcmp(ud->LastValue->String, ud->Exp->String);
					    break;
					case DATA_T_MONEY:
					    changed = memcmp(&(ud->LastValue->Types.Money), &(ud->Exp->Types.Money), sizeof(MoneyType));
					    break;
					case DATA_T_DATETIME:
					    changed = memcmp(&(ud->LastValue->Types.Date), &(ud->Exp->Types.Date), sizeof(DateTime));
					    break;
					case DATA_T_DOUBLE:
					    changed = ud->LastValue->Types.Double != ud->Exp->Types.Double;
					    break;
					}
				    if (changed)
					{
					/** Value changed.  Issue summary. **/
					if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle) < 0)
					    {
					    err = 1;
					    break;
					    }
					expCopyValue(ud->Exp, ud->LastValue, 1);
					}
				    }
				}
			    }
			}
		    }
		}

	    reccnt++;
	    } while(rval == 0);

	/** Finish off the table object **/
	prtEndObject(table_handle);
	rpt_internal_CheckFormats(table, oldmfmt, olddfmt, oldnfmt, 1);

	/** Remove the paramobject item for this query's result set **/
	if (ac) rpt_internal_Deactivate(inf, ac);

	/** If error, indicate such **/
	if (err) return -1;

    return 0;
    }


/*** rpt_internal_WriteExpResult - writes the result of an expression to the
 *** output.  Used by DoData, but also by DoField when printing an aggregate
 *** value.
 ***/
int
rpt_internal_WriteExpResult(pRptSession rs, pExpression exp, int container_handle)
    {
    pXString str_data;

	/** Check the evaluated result value and output it in the report. **/
	str_data = (pXString)nmMalloc(sizeof(XString));
	xsInit(str_data);
	xsCopy(str_data,"",-1);
	if (!(exp->Flags & EXPR_F_NULL))
	    {
	    switch(exp->DataType)
	        {
                case DATA_T_INTEGER:
                    objDataToString(str_data, exp->DataType, &(exp->Integer), 0);
                    break;
                case DATA_T_DOUBLE:
                    objDataToString(str_data, exp->DataType, &(exp->Types.Double), 0);
                    break;
                case DATA_T_STRING:
                    objDataToString(str_data, exp->DataType, exp->String, 0);
                    break;
                case DATA_T_MONEY:
                    objDataToString(str_data, exp->DataType, &(exp->Types.Money), 0);
                    break;
                case DATA_T_DATETIME:
                    objDataToString(str_data, exp->DataType, &(exp->Types.Date), 0);
                    break;
                case DATA_T_INTVEC:
                    objDataToString(str_data, exp->DataType, &(exp->Types.IntVec), 0);
                    break;
                case DATA_T_STRINGVEC:
                    objDataToString(str_data, exp->DataType, &(exp->Types.StrVec), 0);
                    break;
		}
	    }
	else
	    {
	    xsCopy(str_data,rpt_internal_GetNULLstr(),-1);
	    }
	prtWriteString(container_handle, str_data->String);
	xsDeInit(str_data);
	nmFree(str_data, sizeof(XString));

    return 0;
    }


/*** rpt_internal_WritePOD - take a data type and POD value and write its text
 *** on output.
 ***/
int
rpt_internal_WritePOD(pRptSession rs, int t, pObjData pod, int container_handle)
    {
    pXString str_data;

	/** Init our output data xstring **/
	str_data = (pXString)nmMalloc(sizeof(XString));
	xsInit(str_data);
	xsCopy(str_data,"",-1);
	if (pod)
	    {
	    /** Not null value; output according to data type **/
	    switch(t)
	        {
                case DATA_T_INTEGER:
                case DATA_T_DOUBLE:
                    objDataToString(str_data, t, pod, 0);
                    break;
                case DATA_T_STRING:
                case DATA_T_MONEY:
                case DATA_T_DATETIME:
                case DATA_T_INTVEC:
                case DATA_T_STRINGVEC:
                    objDataToString(str_data, t, pod->Generic, 0);
                    break;
		}
	    }
	else
	    {
	    /** null value - just get the NULL string **/
	    xsCopy(str_data,rpt_internal_GetNULLstr(),-1);
	    }
	prtWriteString(container_handle, str_data->String);
	xsDeInit(str_data);
	nmFree(str_data, sizeof(XString));

    return 0;
    }

/*** rpt_internal_DoData - process an expression-based data element within
 *** the report system.  This element should eventually replace the comment
 *** and column entities, but not necessarily.
 ***/
int
rpt_internal_DoData(pRptData inf, pStructInf data, pRptSession rs, int container_handle)
    {
    int attr=0,oldattr=0;
    char* ptr = NULL;
    char oldmfmt[32],olddfmt[32],oldnfmt[32];
    int nl = 0;
    PrtTextStyle oldstyle;
    pPrtTextStyle oldstyleptr = &oldstyle;
    int t,rval;
    ObjData od;
    pStructInf value_inf;
    pRptUserData ud;

	rval = rpt_internal_CheckCondition(inf,data);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get style information **/
	prtGetTextStyle(container_handle, &oldstyleptr);
	rpt_internal_CheckFormats(data, oldmfmt, olddfmt, oldnfmt, 0);
	rpt_internal_CheckGoto(rs,data,container_handle);
	rpt_internal_SetStyle(inf, data, rs, container_handle);

	/** Need to enable auto-newline? **/
	ptr=NULL;
	if (stAttrValue(stLookup(data,"autonewline"),NULL,&ptr,0) >=0)
	    {
	    if (ptr && !strcmp(ptr,"yes")) nl=1;
	    }

	/** Get the expression itself **/
	value_inf = stLookup(data,"value");
	if (!value_inf)
	    {
	    mssError(0,"RPT","%s '%s' must have a value expression", data->UsrType, data->Name);
	    prtSetTextStyle(container_handle, &oldstyle);
	    rpt_internal_CheckFormats(data, oldmfmt, olddfmt, oldnfmt, 1);
	    return -1;
	    }
	t = stGetAttrType(value_inf, 0);
	if (t == DATA_T_CODE)
	    {
	    ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (int)(value_inf->UserData));
	    if (ud)
		{
		/** Evaluate the expression **/
		rval = expEvalTree(ud->Exp, inf->ObjList);
		if (rval < 0)
		    {
		    mssError(0,"RPT","Could not evaluate %s '%s' value expression", data->UsrType, data->Name);
		    prtSetTextStyle(container_handle, &oldstyle);
		    rpt_internal_CheckFormats(data, oldmfmt, olddfmt, oldnfmt, 1);
		    return -1;
		    }

		/** Output the result **/
		rpt_internal_WriteExpResult(rs, ud->Exp, container_handle);
		}
	    else
		{
		rpt_internal_WritePOD(rs, t, NULL, container_handle);
		}
	    }
	else if (t == DATA_T_STRING || t == DATA_T_DOUBLE || t == DATA_T_INTEGER || t == DATA_T_MONEY)
	    {
	    rval = stGetAttrValue(stLookup(data,"value"), t, POD(&od), 0);
	    if (rval == 0) rpt_internal_WritePOD(rs, t, &od, container_handle);
	    else if (rval == 1) rpt_internal_WritePOD(rs, t, NULL, container_handle);
	    }
	else
	    {
	    mssError(1,"RPT","Unknown data value for %s '%s' element", data->UsrType, data->Name);
	    prtSetTextStyle(container_handle, &oldstyle);
	    rpt_internal_CheckFormats(data, oldmfmt, olddfmt, oldnfmt, 1);
	    return -1;
	    }

	/** Emit a newline? **/
	if (nl) prtWriteNL(container_handle);

	/** Put the fonts etc back **/
	prtSetTextStyle(container_handle, &oldstyle);
	rpt_internal_CheckFormats(data, oldmfmt, olddfmt, oldnfmt, 1);

    return 0;
    }

#if 00

/*** rpt_internal_DoComment - outputs a comment to the printer driver
 *** specified in the req structure.
 ***/
int
rpt_internal_DoComment(pRptData inf, pStructInf comment, pRptSession rs)
    {
    int attr=0,oldattr=0;
    char* ptr=NULL;
    int nl=1;
    pXString text_str;
    char oldmfmt[32],olddfmt[32],oldnfmt[32];
    char* saved_font = NULL;

	/** Get style information **/
	attr = rpt_internal_GetStyle(comment);
	rpt_internal_CheckFormats(comment, oldmfmt, olddfmt, oldnfmt, 0);
	rpt_internal_CheckGoto(rs,comment);
	rpt_internal_CheckFont(rs,comment,&saved_font);

	/** Need to disable auto-newline? **/
	ptr=NULL;
	if (stAttrValue(stLookup(comment,"autonewline"),NULL,&ptr,0) >=0)
	    {
	    if (ptr && !strcmp(ptr,"no")) nl=0;
	    }

	/** Find the comment text **/
	ptr=NULL;
	stAttrValue(stLookup(comment,"text"),NULL,&ptr,0);
	if (ptr)
	    {
	    text_str = rpt_internal_SubstParam(inf, ptr);
	    ptr = text_str->String;
	    if (attr >= 0)
		{
	        oldattr = prtGetAttr(rs->PSession);
	        prtSetAttr(rs->PSession,attr);
		}
	    prtWriteString(rs->PSession,ptr,-1);
	    if (nl) prtWriteNL(rs->PSession);
	    if (attr >= 0) prtSetAttr(rs->PSession,oldattr);
	    xsDeInit(text_str);
	    nmFree(text_str,sizeof(XString));
	    }
	rpt_internal_CheckFont(rs,comment,&saved_font);
	rpt_internal_CheckFormats(comment, oldmfmt, olddfmt, oldnfmt, 1);

    return 0;
    }



/*** rpt_internal_DoField - insert a data-driven piece of text into the
 *** form, with a given style.
 ***/
int
rpt_internal_DoField(pRptData inf, pStructInf field, pRptSession rs, pQueryConn this_qy)
    {
    int style,oldstyle=0;
    char* src;
    char* tsrc;
    int nl=1;
    pQueryConn qy;
    char* txt;
    char* ptr;
    int t,n,i;
    char nbuf[16];
    pStructInf subitem;
    void* p;
    double dbl;
    char oldmfmt[32],olddfmt[32],oldnfmt[32];
    char* saved_font = NULL;
    pExpression exp;

	/** Get style information **/
	style = rpt_internal_GetStyle(field);
	rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 0);
	rpt_internal_CheckGoto(rs,field);
	rpt_internal_CheckFont(rs,field,&saved_font);

	/** Need to disable auto-newline? **/
	ptr=NULL;
	if (stAttrValue(stLookup(field,"autonewline"),NULL,&ptr,0) >=0)
	    {
	    if (ptr && !strcmp(ptr,"no")) nl=0;
	    }

	/** Set the style for the printer, if specified **/
	if (style >= 0)
	    {
	    oldstyle = prtGetAttr(rs->PSession);
	    prtSetAttr(rs->PSession, style);
	    }

	/** Get the source field.  This may also include source query. **/
	tsrc=NULL;
	stAttrValue(stLookup(field,"source"),NULL,&tsrc,0);
	if (!tsrc)
	    {
	    mssError(1,"RPT","Invalid source for form field '%s' element", field->Name);
	    rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 1);
	    rpt_internal_CheckFont(rs,field,&saved_font);
	    return -1;
	    }
	src=NULL;
	stAttrValue(stLookup(field,"source"),NULL,&src,1);
	if (!src)
	    {
	    src=tsrc;
	    tsrc=NULL;
	    }

	/** If user specified source qy, look it up.  Otherwise use current one **/
	if (tsrc)
	    {
	    /** Query source exists? **/
	    qy = (pQueryConn)xhLookup(rs->Queries, tsrc);
	    if (!qy)
	        {
                mssError(1,"RPT","Query source '%s' given for form field '%s' is undefined", tsrc, field->Name);
	        rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 1);
	        rpt_internal_CheckFont(rs,field,&saved_font);
	        return -1;
	        }

	    /** If user is asking for an aggregate value, look for that **/
	    n = 0;
	    for(i=0;i<qy->UserInf->nSubInf;i++)
	        {
		subitem = qy->UserInf->SubInf[i];
		if (stStructType(subitem) == ST_T_SUBGROUP && !strcmp(subitem->UsrType,"report/aggregate"))
		    {
		    if (!strcmp(subitem->Name, src))
		        {
		        exp = (pExpression)(qy->AggregateExpList.Items[n]);
	    	        rpt_internal_WriteExpResult(rs, exp);
		        expResetAggregates(exp, -1);
			expEvalTree(exp,qy->ObjList);
	                rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 1);
	                rpt_internal_CheckFont(rs,field,&saved_font);
		        if (style >= 0) prtSetAttr(rs->PSession, oldstyle);
		        if (nl) prtWriteNL(rs->PSession);
		        return 0;
		        }
		    n++;
		    }
		}
	
	    /** Otherwise, query must be active. **/
	    if (qy->QueryItem == NULL)
		{
                mssError(1,"RPT","Query source '%s' given for form field '%s' is inactive", tsrc, field->Name);
	        rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 1);
	        rpt_internal_CheckFont(rs,field,&saved_font);
	        return -1;
		}
	    }
	else
	    {
	    if (this_qy == NULL)
	        {
		mssError(1,"RPT","Must specify query source for data field '%s'", field->Name);
	        rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 1);
	        rpt_internal_CheckFont(rs,field,&saved_font);
		return -1;
		}
	    qy = this_qy;
	    }

	/** Lookup the field and get the value **/
	txt=NULL;
	t = objGetAttrType(qy->QueryItem, src);
	if (t < 0)
	    {
            mssError(1,"RPT","Field source '%s' given for form field '%s' does not exist", src, field->Name);
	    rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 1);
	    rpt_internal_CheckFont(rs,field,&saved_font);
	    return -1;
	    }
	switch(t)
	    {
	    case DATA_T_INTEGER:
	        if (objGetAttrValue(qy->QueryItem, src, DATA_T_INTEGER, POD(&n)) == 1)
	            {
	            txt = rpt_internal_GetNULLstr();
		    }
	        else
	            {
		    sprintf(nbuf,"%d",n);
	            txt = nbuf;
		    }
		break;

	    case DATA_T_STRING:
	        if (objGetAttrValue(qy->QueryItem, src, DATA_T_STRING, POD(&txt)) == 1) txt=rpt_internal_GetNULLstr();
		break;

	    case DATA_T_MONEY:
	    case DATA_T_DATETIME:
	        if (objGetAttrValue(qy->QueryItem, src, t, POD(&p)) == 1 || p == NULL) 
		    txt = rpt_internal_GetNULLstr();
		else 
		    txt = objDataToStringTmp(t, p, 0);
		break;

	    case DATA_T_DOUBLE:
	        if (objGetAttrValue(qy->QueryItem, src, DATA_T_DOUBLE, POD(&dbl)) == 1)
		    {
		    txt = rpt_internal_GetNULLstr();
		    }
		else
		    {
		    sprintf(nbuf,"%f",dbl);
		    txt = nbuf;
		    }
		break;

	    default:
	        txt = "";
		break;
	    }

	/** Output the value to the printer. **/
	prtWriteString(rs->PSession,txt,-1);

	/** Restore the printer style, if we changed it **/
	if (style >= 0) prtSetAttr(rs->PSession, oldstyle);
	if (nl) prtWriteNL(rs->PSession);
	rpt_internal_CheckFormats(field, oldmfmt, olddfmt, oldnfmt, 1);
	rpt_internal_CheckFont(rs,field,&saved_font);

    return 0;
    }
#endif

/*** rpt_internal_DoForm - creates a free-form style report element, 
 *** which can contain fields, comments, tables, and other forms.
 ***/
int
rpt_internal_DoForm(pRptData inf, pStructInf form, pRptSession rs, int container_handle)
    {
    int i;
    pQueryConn qy;
    int rulesep=0,ffsep=0;
    char* ptr;
    int err=0;
    char oldmfmt[32],olddfmt[32], oldnfmt[32];
    char* saved_font = NULL;
    int relylimit = -1;
    int reclimit = -1;
    int outer_mode = 0;
    int inner_mode = 0;
    pRptActiveQueries ac;
    PrtTextStyle oldstyle;
    pPrtTextStyle oldstyleptr = &oldstyle;
    int reccnt;
    int rval;
    int n;

	rval = rpt_internal_CheckCondition(inf,form);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Issue horizontal rule between records? **/
	if (stAttrValue(stLookup(form,"rulesep"),NULL,&ptr,0) >= 0 && ptr && !strcmp(ptr,"yes"))
	    {
	    rulesep=1;
	    }

	/** Issue form feed between records? **/
	if (stAttrValue(stLookup(form,"ffsep"),NULL,&ptr,0) >= 0 && ptr && !strcmp(ptr,"yes"))
	    {
	    ffsep=1;
	    }

	/** Check for set-page-number? **/
	if (stAttrValue(stLookup(form,"page"),&n,NULL,0) >= 0) prtSetPageNumber(rs->PageHandle, n);

	/** Check for a mode entry **/
	if (stAttrValue(stLookup(form,"mode"),NULL,&ptr,0) >= 0)
	    {
	    if (!strcmp(ptr,"outer")) outer_mode = 1;
	    if (!strcmp(ptr,"inner")) inner_mode = 1;
	    }

	/** Relative-Y record limiter? **/
	if (stAttrValue(stLookup(form,"relylimit"),&relylimit,NULL,0) >= 0)
	    {
	    if (outer_mode)
	        {
		mssError(1,"RPT","relylimit can only be used on an non-outer form ('%s' is outer).", form->Name);
		return -1;
		}
	    if (rulesep || ffsep)
	        {
		mssError(1,"RPT","relylimit is incompatible with rulesep/ffsep in form '%s'", form->Name);
		return -1;
		}
	    }

	/** Record-count limiter? **/
	if (stAttrValue(stLookup(form,"reclimit"),&reclimit,NULL,0) >= 0)
	    {
	    if (outer_mode)
	        {
		mssError(1,"RPT","reclimit can only be used on an non-outer form ('%s' is outer).", form->Name);
		return -1;
		}
	    }

	/** Get style information **/
	prtGetTextStyle(container_handle, &oldstyleptr);
	rpt_internal_CheckFormats(form, oldmfmt, olddfmt, oldnfmt, 0);
	rpt_internal_CheckGoto(rs,form,container_handle);
	rpt_internal_SetStyle(inf, form, rs, container_handle);

	/** Start the query. **/
	if ((ac = rpt_internal_Activate(inf, form, rs)) == NULL) return -1;

	/** Fetch the first row. **/
	if ((rval = rpt_internal_NextRecord(ac, inf, form, rs, 1)) < 0)
	    {
	    return -1;
	    }

	/** Enter the row retrieval loop.  For each row, do all sub-parts **/
	reccnt = 0;
	/*if (rulesep) prtWriteLine(rs->PSession);*/
	qy = ac->Queries[ac->Count-1];
	while(rval == 0)
	    {
	    if (/*(relylimit != -1 && prtGetRelVPos(rs->PSession) >= relylimit) || */
	        (reclimit != -1 && reccnt >= reclimit))
	        {
		break;
		}
	    if (rpt_internal_UseRecord(ac) < 0)
	        {
		err = 1;
		break;
		}
	    rval = rpt_internal_DoContainer(inf, form, rs, container_handle);
	    if (rval < 0) err = 1;
	    if (outer_mode && qy->InnerExecCnt == 0)
	        {
		mssError(err?0:1,"RPT","No inner-mode form/table was run for outer-mode '%s'",form->Name);
		err = 1;
		}
	    if (ffsep) prtWriteFF(container_handle);
	    expModifyParam(inf->ObjList, "this", inf->Obj); /* page no changed, force re-eval */
	    /*if (rulesep) prtWriteLine(rs->PSession);*/
	    rval = rpt_internal_NextRecord(ac, inf, form, rs, 0);
	    reccnt++;
	    if (err) break;
	    }

	rpt_internal_Deactivate(inf, ac);

	rpt_internal_CheckFormats(form, oldmfmt, olddfmt, oldnfmt, 1);
	prtSetTextStyle(container_handle, &oldstyle);

	/** Error? **/
	if (err) return -1;

    return 0;
    }

#if 00
/*** rpt_internal_DoFooter - generate a report footer on demand.  This is
 *** a callback function from the print management layer when a page reaches
 *** the point where the footer is needed.
 ***/
int
rpt_internal_DoFooter(void* v, int cur_line)
    {
    pRptData inf = (pRptData)v;
    int rval;

	/** Treat this like a report/section **/
	rval = rpt_internal_DoSection(inf, inf->RSess->FooterInf, inf->RSess, NULL);
	if (rval < 0) mssError(0,"RPT","Could not do report/footer section");

    return rval;
    }


/*** rpt_internal_DoHeader - generate a report header on demand.  This is also
 *** a callback function from the print management layer when a new page is
 *** begun.
 ***/
int
rpt_internal_DoHeader(void* v, int cur_line)
    {
    pRptData inf = (pRptData)v;
    int rval;

	/** Treat this like a report/section **/
	rval = rpt_internal_DoSection(inf, inf->RSess->HeaderInf, inf->RSess, NULL);
	if (rval < 0) mssError(0,"RPT","Could not do report/header section");

    return rval;
    }
#endif



/*** rpt_internal_CheckCondition - check for a condition statement in the current
 *** object, and if it exists, evaluate it to see if we can do this object.  If
 *** the condition is false, return 0, otherwise 1 for success.  On error, return
 *** -1.
 ***/
int
rpt_internal_CheckCondition(pRptData inf, pStructInf config)
    {
    pStructInf condition_inf;
    int t;
    int rval;
    pRptUserData ud;

	/** Does this object have a condition statement? **/
	condition_inf = stLookup(config,"condition");
	if (!condition_inf) return 1; /* succeed */

	/** Get the expression itself **/
	t = stGetAttrType(condition_inf, 0);
	if (t == DATA_T_CODE)
	    {
	    ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (int)(condition_inf->UserData));
	    if (ud)
		{
		/** Evaluate the expression **/
		rval = expEvalTree(ud->Exp, inf->ObjList);
		if (rval < 0)
		    {
		    mssError(0,"RPT","Could not evaluate '%s' condition expression", config->Name);
		    return -1;
		    }
		if (ud->Exp->DataType == DATA_T_INTEGER)
		    {
		    if (ud->Exp->Integer == 0) return 0;
		    else return 1;
		    }
		return 0;
		}
	    else
		{
		mssError(0,"RPT","Could not get '%s' condition expression", config->Name);
		return -1;
		}
	    }
	else if (t == DATA_T_STRING || t == DATA_T_DOUBLE || t == DATA_T_INTEGER || t == DATA_T_MONEY)
	    {
	    if (rpt_internal_GetBool(config,"condition",1,0))
		return 1;
	    else
		return 0;
	    }
	else
	    {
	    mssError(1,"RPT","Unknown data value for condition expression for '%s'", config->Name);
	    return -1;
	    }

    return 0;
    }



/*** rpt_internal_SetStyle - check for any style/appearance commands in the current
 *** object, and if found, configure those for the current print object.
 ***/
int
rpt_internal_SetStyle(pRptData inf, pStructInf config, pRptSession rs, int prt_obj)
    {
    char* ptr;
    int n;
    int attr = 0;
    int i;

	/** Check for font, size, color **/
	if (stAttrValue(stLookup(config,"font"), NULL, &ptr, 0) == 0)
	    {
	    prtSetFont(prt_obj, ptr);
	    }
	if (stAttrValue(stLookup(config,"fontsize"), &n, NULL, 0) == 0)
	    {
	    prtSetFontSize(prt_obj, n);
	    }
	if (stAttrValue(stLookup(config,"fontcolor"), NULL, &ptr, 0) == 0)
	    {
	    if (ptr[0] == '#')
		{
		n = strtol(ptr+1, NULL, 16);
		prtSetColor(prt_obj, n);
		}
	    }

	/** Check justification **/
	if (stAttrValue(stLookup(config,"align"), NULL, &ptr, 0) == 0)
	    {
	    if (!strcmp(ptr,"left")) prtSetJustification(prt_obj, PRT_JUST_T_LEFT);
	    else if (!strcmp(ptr,"right")) prtSetJustification(prt_obj, PRT_JUST_T_RIGHT);
	    else if (!strcmp(ptr,"center")) prtSetJustification(prt_obj, PRT_JUST_T_CENTER);
	    }

	/** Check attrs (bold/italic/underline) **/
	attr = prtGetAttr(prt_obj);
	for(i=0;i<16;i++)
	    {
	    if (stAttrValue(stLookup(config,"style"), NULL, &ptr, i) != 0) break;
	    if (!strcmp(ptr,"bold")) attr |= PRT_OBJ_A_BOLD;
	    else if (!strcmp(ptr,"italic")) attr |= PRT_OBJ_A_ITALIC;
	    else if (!strcmp(ptr,"underline")) attr |= PRT_OBJ_A_UNDERLINE;
	    else if (!strcmp(ptr,"plain")) attr = 0;
	    }

    return 0;
    }


/*** rpt_internal_SetMargins - sets the margins for a print object
 ***/
int
rpt_internal_SetMargins(pStructInf config, int prt_obj)
    {
    double ml, mr, mt, mb;

	/** Set the margins **/
	if (rpt_internal_GetDouble(config, "margintop", &mt, 0) < 0) mt = 0.0;
	if (rpt_internal_GetDouble(config, "marginbottom", &mb, 0) < 0) mb = 0.0;
	if (rpt_internal_GetDouble(config, "marginleft", &ml, 0) < 0) ml = 0.0;
	if (rpt_internal_GetDouble(config, "marginright", &mr, 0) < 0) mr = 0.0;
	if (mt != 0 || mb != 0 || ml != 0 || mr != 0) 
	    prtSetMargins(prt_obj, mt, mb, ml, mr);

    return 0;
    }


/*** rpt_internal_GetDouble - gets a double precision floating point value from
 *** the rpt config, automatically translating any integers into doubles
 ***/
int
rpt_internal_GetDouble(pStructInf config, char* attr, double* val, int nval)
    {
    int t,n;
    pStructInf attr_inf;

	attr_inf = stLookup(config, attr);
	if (!attr_inf) return -1;
	t = stGetAttrType(attr_inf, nval);
	if (t == DATA_T_INTEGER)
	    {
	    if (stGetAttrValue(attr_inf, t, POD(&n), nval) != 0) return -1;
	    *val = (double)n;
	    }
	else if (t == DATA_T_DOUBLE)
	    {
	    if (stGetAttrValue(attr_inf, t, POD(val), nval) != 0) return -1;
	    }
	else
	    {
	    return -1;
	    }

    return 0;
    }


/*** rpt_internal_GetBool - gets a boolean value, in the form of 0/1, true/false,
 *** on/off, or yes/no.
 ***/
int
rpt_internal_GetBool(pStructInf config, char* attr, int def, int nval)
    {
    int t;
    pStructInf attr_inf;
    int n;
    char* ptr;

	attr_inf = stLookup(config, attr);
	if (!attr_inf) return def;
	t = stGetAttrType(attr_inf, nval);
	if (t == DATA_T_INTEGER)
	    {
	    if (stGetAttrValue(attr_inf, t, POD(&n), nval) != 0) return def;
	    }
	else if (t == DATA_T_STRING)
	    {
	    if (stGetAttrValue(attr_inf, t, POD(&ptr), nval) != 0) return def;
	    if (!strcmp(ptr,"yes") || !strcmp(ptr,"true") || !strcmp(ptr,"on")) n = 1;
	    else if (!strcmp(ptr,"no") || !strcmp(ptr,"false") || !strcmp(ptr,"off")) n = 0;
	    else return def;
	    }

    return n;
    }


/*** rpt_internal_DoContainer - loop through subobjects of a container (area, table
 *** cell, etc.)...
 ***/
int
rpt_internal_DoContainer(pRptData inf, pStructInf container, pRptSession rs, int container_handle)
    {
    int rval = 0;
    int i;
    pStructInf subobj;

	/** Loop through container subobjects **/
	for (i=0;i<container->nSubInf;i++)
	    {
	    subobj = container->SubInf[i];
	    if (stStructType(subobj) == ST_T_SUBGROUP)
		{
		if (!strcmp(subobj->UsrType, "report/area"))
		    {
		    rval = rpt_internal_DoArea(inf, subobj, rs, NULL, container_handle);
		    if (rval < 0) break;
		    }
		else if (!strcmp(subobj->UsrType, "report/image"))
		    {
		    rval = rpt_internal_DoImage(inf, subobj, rs, NULL, container_handle);
		    if (rval < 0) break;
		    }
		else if (!strcmp(subobj->UsrType, "report/data"))
		    {
		    rval = rpt_internal_DoData(inf, subobj, rs, container_handle);
		    if (rval < 0) break;
		    }
		else if (!strcmp(subobj->UsrType, "report/form"))
		    {
		    rval = rpt_internal_DoForm(inf, subobj, rs, container_handle);
		    if (rval < 0) break;
		    }
		else if (!strcmp(subobj->UsrType, "report/table"))
		    {
		    rval = rpt_internal_DoTable(inf, subobj, rs, container_handle);
		    if (rval < 0) break;
		    }
		}
	    }

    return rval;
    }


/*** rpt_internal_DoImage - put a picture on the report.
 ***/
int
rpt_internal_DoImage(pRptData inf, pStructInf image, pRptSession rs, pQueryConn this_qy, int container_handle)
    {
    char* imgsrc;
    pPrtImage img;
    pObject imgobj;
    int flags;
    double x,y,w,h;
    int rval;

	rval = rpt_internal_CheckCondition(inf,image);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get area geometry **/
	if (rpt_internal_GetDouble(image, "x", &x, 0) < 0) x = -1;
	if (rpt_internal_GetDouble(image, "y", &y, 0) < 0) y = -1;
	if (rpt_internal_GetDouble(image, "width", &w, 0) < 0) return -1;
	if (rpt_internal_GetDouble(image, "height", &h, 0) < 0) return -1;

	/** Load the image **/
	if (stGetAttrValue(stLookup(image,"source"), DATA_T_STRING, POD(&imgsrc), 0) < 0) return -1;
	if ((imgobj = objOpen(inf->Obj->Session, imgsrc, O_RDONLY, 0400, "image/png")) == NULL) return -1;
	img = prtCreateImageFromPNG(objRead, imgobj);
	objClose(imgobj);
	if (!img) return -1;

	/** Check flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;

	/** Add it to its container **/
	if (prtWriteImage(container_handle, img, x,y,w,h, flags) < 0) return -1;

    return 0;
    }


/*** rpt_internal_DoArea - build an 'area' on the report in which textflow type
 *** content can be placed.  V3 prtmgmt.
 ***/
int
rpt_internal_DoArea(pRptData inf, pStructInf area, pRptSession rs, pQueryConn this_qy, int container_handle)
    {
    int area_handle;
    double x, y, w, h, bw;
    int flags;
    pPrtBorder bdr = NULL;
    int rval = 0;

	rval = rpt_internal_CheckCondition(inf,area);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get area geometry **/
	if (rpt_internal_GetDouble(area, "x", &x, 0) < 0) x = -1;
	if (rpt_internal_GetDouble(area, "y", &y, 0) < 0) y = -1;
	if (rpt_internal_GetDouble(area, "width", &w, 0) < 0) w = -1;
	if (rpt_internal_GetDouble(area, "height", &h, 0) < 0) h = -1;

	/** Check for flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;
	if (rpt_internal_GetBool(area, "allowbreak", 1, 0)) flags |= PRT_OBJ_U_ALLOWBREAK;
	if (rpt_internal_GetBool(area, "fixedsize", 0, 0)) flags |= PRT_OBJ_U_FIXEDSIZE;

	/** Check for border **/
	if (stGetAttrValue(stLookup(area, "border"), DATA_T_DOUBLE, POD(&bw), 0) == 0)
	    {
	    bdr = prtAllocBorder(1,0.0,0.0, bw,0x000000);
	    }

	/** Create the area **/
	if (bdr)
	    area_handle = prtAddObject(container_handle, PRT_OBJ_T_AREA, x, y, w, h, flags, "border", bdr, NULL);
	else
	    area_handle = prtAddObject(container_handle, PRT_OBJ_T_AREA, x, y, w, h, flags, NULL);
	if (bdr) prtFreeBorder(bdr);
	if (area_handle < 0) return -1;

	/** Set the style for the area **/
	rpt_internal_SetStyle(inf, area, rs, area_handle);
	rpt_internal_SetMargins(area, area_handle);

	/** Do stuff that is contained in the area **/
	rval = rpt_internal_DoContainer(inf, area, rs, area_handle);

	/** End the area **/
	prtEndObject(area_handle);

    return rval;
    }



/*** rpt_internal_PreBuildExp - gets on individual expression during the pre-
 *** processing part of running a report.  Returns 0 on success and -1 on
 *** failure.  'obj' is the report attribute containing the expression, and
 *** is the object whose UserData part will be set up.  If the value of the
 *** 'obj' attribute is not an expression, returns 0 but does not set up
 *** the UserData and its expression.
 ***/
int
rpt_internal_PreBuildExp(pRptData inf, pStructInf obj, pParamObjects objlist)
    {
    int t, rval;
    pRptUserData ud;
    pExpression exp;

	/** See if we have a real expression here **/
	t = stGetAttrType(obj, 0);
	if (t == DATA_T_CODE)
	    {
	    rval = stGetAttrValue(obj, t, POD(&exp), 0);
	    if (rval == 0)
		{
		/** Okay, got an expression.  Bind it to the current objlist **/
		ud = (pRptUserData)nmMalloc(sizeof(RptUserData));
		if (!ud) return -1;
		ud->LastValue = NULL;
		ud->Exp = expDuplicateExpression(exp);
		expBindExpression(ud->Exp, objlist, EXPR_F_RUNSERVER);

		/** ... and set it up in our user data slots structure **/
		if (!(obj->UserData)) 
		    obj->UserData = (void*)(inf->NextUserDataSlot++);
		else if ((int)(obj->UserData) >= inf->NextUserDataSlot)
		    inf->NextUserDataSlot = ((int)(obj->UserData))+1;
		xaSetItem(&(inf->UserDataSlots), (int)(obj->UserData), (void*)ud);
		}
	    else
		{
		/** Fail if couldn't get the expression **/
		return -1;
		}
	    }
	else if (t < 0)
	    {
	    /** Fail if couldn't determine type, or obj is invalid **/
	    return -1;
	    }

    return 0;
    }



/*** rpt_internal_PreProcess - compiles all expressions in the report prior
 *** to the report's execution, including report/table expressions as well 
 *** as report/data expressions.
 ***/
int
rpt_internal_PreProcess(pRptData inf, pStructInf object, pRptSession rs, pParamObjects objlist)
    {
    pParamObjects use_objlist;
    int i,t,rval;
    pStructInf ck_inf;
    char* ptr;
    pXArray xa = NULL;
    pExpression exp;
    int err = 0;
    pXString subst_value;
    pStructInf expr_inf;

    	/** Need to allocate an object list? **/
	if (!objlist)
	    {
	    use_objlist = expCreateParamList();
	    expAddParamToList(use_objlist, "this", NULL, EXPR_O_PARENT | EXPR_O_CURRENT);
	    }
	else
	    {
	    use_objlist = objlist;
	    }

	/** Default userdata is NULL **/
	object->UserData = NULL;

	/** IF this is a report/data or report/table-cell, compile its one expression **/
	if (!strcmp(object->UsrType,"report/data") || !strcmp(object->UsrType,"report/table-cell"))
	    {
	    /** Get the expression itself **/
	    expr_inf = stLookup(object,"value");
	    if (expr_inf && rpt_internal_PreBuildExp(inf, expr_inf, use_objlist) < 0)
		{
		if (!strcmp(object->UsrType,"report/data"))
		    {
		    err = 1;
		    mssError(1,"RPT","report/data '%s' must have a value expression.", object->Name);
		    }
		else
		    {
		    expr_inf->UserData = NULL;
		    }
		}
	    }

	/** If no errors, proceed... **/
	if (!err)
	    {
	    for(i=0;i<object->nSubInf;i++) 
	        {
		if (stStructType(object->SubInf[i]) == ST_T_SUBGROUP)
		    {
		    if (rpt_internal_PreProcess(inf, object->SubInf[i], rs, use_objlist) < 0)
			{
			err = 1;
			break;
			}
		    }
		else
		    {
		    /** Attribute, condition exp.? **/
		    if (!strcmp(object->SubInf[i]->Name, "condition") || (!strcmp(object->UsrType,"report/table-row") && !strcmp(object->SubInf[i]->Name, "summarize_for")))
			{
			expr_inf = object->SubInf[i];
			if (rpt_internal_PreBuildExp(inf, expr_inf, use_objlist) < 0)
			    {
			    err = 1;
			    mssError(1,"RPT","%s on '%s' must have an expression.", object->SubInf[i]->Name, object->Name);
			    }
			}
		    }
		}
	    }
	
	/** Error? **/
	if (err && object->UserData)
	    {
	    /*if (!strcmp(object->UsrType,"report/data"))
	        {
		expFreeExpression((pExpression)(object->UserData));
		object->UserData = NULL;
		}
	    else*/ if (!strcmp(object->UsrType,"report/table"))
	        {
	        object->UserData = NULL;
		if (xa)
		    {
		    for(i=0;i<xa->nItems;i++) if (xa->Items[i]) expFreeExpression((pExpression)(xa->Items[i]));
		    xaDeInit(xa);
		    nmFree(xa,sizeof(XArray));
		    xa = NULL;
		    }
		}
	    }

    return err?-1:0;
    }


/*** rpt_internal_UnPreProcess - undo the preprocessing step to free up
 *** the "UserData" stuff.
 ***/
int
rpt_internal_UnPreProcess(pRptData inf, pStructInf object, pRptSession rs)
    {
    int i;
    pXArray xa;
    pRptUserData ud;

	/** Visit all sub-inf structures that are groups **/
	for(i=0;i<object->nSubInf;i++) if (stStructType(object->SubInf[i]) == ST_T_SUBGROUP)
	    {
	    rpt_internal_UnPreProcess(inf, object->SubInf[i], rs);
	    }

	/** If userdata alloc'd, free it **/
	if (object->UserData)
	    {
	    /** IF report/table, free up the xarray of expressions **/
	    if (!strcmp(object->UsrType,"report/table"))
	        {
		xa = (pXArray)(object->UserData);
		for(i=0;i<xa->nItems;i++) expFreeExpression((pExpression)(xa->Items[i]));
		xaDeInit(xa);
		nmFree(xa,sizeof(XArray));
		}
	    else if (!strcmp(object->UsrType,"report/data"))
	        {
		/*expFreeExpression((pExpression)(object->UserData));*/
		}
	    object->UserData = NULL;
	    }

	for(i=1;i<inf->UserDataSlots.nItems;i++) if (inf->UserDataSlots.Items[i])
	    {
	    ud = (pRptUserData)(inf->UserDataSlots.Items[i]);
	    expFreeExpression(ud->Exp);
	    inf->UserDataSlots.Items[i] = NULL;
	    nmFree(ud, sizeof(RptUserData));
	    }

    return 0;
    }


/*** rpt_internal_FreeQC - callback routine for xhClear to free a query
 *** conn structure.
 ***/
int
rpt_internal_FreeQC(pQueryConn qc)
    {
    int i;
   	
	/** Release qc expressions (like aggregates) **/
	for(i=0;i<qc->AggregateExpList.nItems;i++) 
	    {
	    if (qc->AggregateExpList.Items[i]) expFreeExpression((pExpression)(qc->AggregateExpList.Items[i]));
	    }
	for(i=0;i<qc->AggregateWhereList.nItems;i++) 
	    {
	    if (qc->AggregateWhereList.Items[i]) expFreeExpression((pExpression)(qc->AggregateWhereList.Items[i]));
	    }
	xaDeInit(&(qc->AggregateExpList));
	xaDeInit(&(qc->AggregateWhereList));
	xaDeInit(&(qc->AggregateDoReset));

	/** Release qc data, and qc structure. **/
    	if (qc->DataBuf) nmSysFree(qc->DataBuf);
    	if (qc->ObjList) expFreeParamList(qc->ObjList);
    	nmFree(qc,sizeof(QueryConn));

    return 0;
    }


/*** rpt_internal_Run - execute an immediate adhoc report with complete
 *** information listed in the req structure.
 ***/
int
rpt_internal_Run(pRptData inf, pFile out_fd, pPrtSession ps)
    {
    char* title = NULL;
    char* ptr=NULL;
    char sbuf[128];
    XHashTable queries;
    pStructInf subreq,req = inf->Node->Data;
    pObjSession s = inf->Obj->Session;
    int i,j,n_lines;
    pQueryConn qc;
    pRptSession rs;
    int err = 0;
    pXString title_str = NULL;
    pXString subst_str;
    time_t cur_time;
    int no_title_bar = 0;
    pExpression exp;
    char oldmfmt[32],olddfmt[32], oldnfmt[32];
    int resolution;
    double pagewidth, pageheight;

    	/** Report has no titlebar header? **/
	stAttrValue(rpt_internal_GetParam(inf,"titlebar"),NULL,&ptr,0);
	if (ptr && !strcasecmp(ptr,"no"))
	    {
	    no_title_bar = 1;
	    }

	/** Report has a title? **/
        stAttrValue(rpt_internal_GetParam(inf,"title"),NULL,&title,0);

	/** If had a title, do param subst on it. **/
	if (title) 
	    {
	    title_str = rpt_internal_SubstParam(inf, title);
	    title = title_str->String;
	    }

	/** Resolution specified? **/
	if (stAttrValue(rpt_internal_GetParam(inf,"resolution"), &resolution, NULL, 0) == 0)
	    {
	    prtSetResolution(ps, resolution);
	    }

	/** Units of measure? **/
	ptr = NULL;
	stAttrValue(stLookup(req,"units"), NULL, &ptr, 0);
	if (ptr) 
	    {
	    if (prtSetUnits(ps, ptr) < 0)
		{
		mssError(1,"RPT","Invalid units-of-measure '%s' specified", ptr);
		err = 1;
		}
	    }

	/** Page geometry? **/
	prtGetPageGeometry(ps, &pagewidth, &pageheight);
	rpt_internal_GetDouble(req,"pagewidth",&pagewidth,0);
	rpt_internal_GetDouble(req,"pageheight",&pageheight,0);
	prtSetPageGeometry(ps, pagewidth, pageheight);

	/** Output the title, unless instructed otherwise. **/
	/*if (!no_title_bar)
	    {
	    prtWriteLine(ps);
	    prtSetAttr(ps, RS_TX_BOLD | RS_TX_CENTER);
	    if (title)
	        {
	        prtWriteString(ps,title,-1);
	        }
	    else
	        {
	        sprintf(sbuf,"UNTITLED REPORT");
	        prtWriteString(ps,sbuf,-1);
	        }
	    prtWriteNL(ps);
	    cur_time = time(NULL);
	    snprintf(sbuf,128,"REQUESTED BY USER %s AT %s",mssUserName(),ctime(&cur_time));
	    prtWriteString(ps,sbuf,-1);
	    prtSetAttr(ps,0);
	    prtWriteLine(ps);
	    }*/

	/** Create the parameter object list, adding the report object itself as 'this' **/
	inf->ObjList = expCreateParamList();
	expAddParamToList(inf->ObjList, "this", inf->Obj, EXPR_O_PARENT | EXPR_O_CURRENT);
	inf->ObjList->Session = inf->Obj->Session;

	/** Ok, now look through the request for queries, comments, tables, forms **/
	xhInit(&queries, 31, 0);
	rs = (pRptSession)nmMalloc(sizeof(RptSession));
	rs->PSession = ps;
	rs->FD = out_fd;
	rs->Queries = &queries;
	rs->ObjSess = s;
	rs->HeaderInf = NULL;
	rs->FooterInf = NULL;
	inf->RSess = rs;
	rs->PageHandle = prtGetPageRef(ps);
	prtSetPageNumber(rs->PageHandle, 1);

	/** Build the object list. **/
	for(i=0;i<req->nSubInf;i++) if (stStructType(req->SubInf[i]) == ST_T_SUBGROUP && !strcmp(req->SubInf[i]->UsrType,"report/query"))
	    {
	    expAddParamToList(inf->ObjList, req->SubInf[i]->Name, NULL, 0);
	    expSetParamFunctions(inf->ObjList, req->SubInf[i]->Name, rpt_internal_QyGetAttrType, 
	    	rpt_internal_QyGetAttrValue, NULL);
	    }

	/** Preprocess the report structure, compile its expressions **/
	if (rpt_internal_PreProcess(inf, req, rs, inf->ObjList) < 0) err = 1;

	/** First, check for report/header and report/footer stuff **/
#if 00
	for(i=0;i<req->nSubInf;i++)
	    {
	    if (stStructType(req->SubInf[i]) == ST_T_SUBGROUP)
		{
		subreq = req->SubInf[i];
		if (!strcmp(subreq->UsrType,"report/header"))
		    {
		    if (rs->HeaderInf)
		        {
			mssError(1,"RPT","Cannot have two report/header elements");
			err = 1;
			break;
			}
		    if (stAttrValue(stLookup(subreq,"lines"),&n_lines,NULL,0) < 0)
		        {
			mssError(1,"RPT","Report/header element must have a lines= attribute");
			err = 1;
			break;
			}
		    rs->HeaderInf = subreq;
		    prtSetHeader(ps, rpt_internal_DoHeader, inf, n_lines);
		    }
		else if (!strcmp(subreq->UsrType,"report/footer"))
		    {
		    if (rs->FooterInf)
		        {
			mssError(1,"RPT","Cannot have two report/footer elements");
			err = 1;
			break;
			}
		    if (stAttrValue(stLookup(subreq,"lines"),&n_lines,NULL,0) < 0)
		        {
			mssError(1,"RPT","Report/footer element must have a lines= attribute");
			err = 1;
			break;
			}
		    rs->FooterInf = subreq;
		    prtSetFooter(ps, rpt_internal_DoFooter, inf, n_lines);
		    }
		}
	    if (err) break;
	    }
#endif

	/** Set top-level formatting **/
	rpt_internal_CheckFormats(req,oldmfmt,olddfmt,oldnfmt,0);

	/** Now do the 'normal' report stuff **/
	for(i=0;i<req->nSubInf;i++)
	    {
	    if (stStructType(req->SubInf[i]) == ST_T_SUBGROUP)
		{
		subreq = req->SubInf[i];
		if (!strcmp(subreq->UsrType,"report/query"))
		    {
		    /** First, build the query information structure **/
		    qc=(pQueryConn)nmMalloc(sizeof(QueryConn));
		    if (!qc) break;
		    qc->UserInf = subreq;
		    ptr=NULL;
		    /*if (stAttrValue(stLookup(subreq,"name"),NULL,&ptr,0) < 0) continue;*/
		    qc->Name = subreq->Name;
		    qc->Query = NULL;
		    qc->QueryItem = NULL;
		    qc->DataBuf = NULL;
		    qc->ObjList = expCreateParamList();
		    xaInit(&(qc->AggregateExpList),16);
		    xaInit(&(qc->AggregateWhereList),16);
		    xaInit(&(qc->AggregateDoReset),16);
		    expAddParamToList(qc->ObjList, "this", NULL, EXPR_O_CURRENT);
		    expSetParamFunctions(qc->ObjList, "this", rpt_internal_QyGetAttrType, rpt_internal_QyGetAttrValue, NULL);
		    xhAdd(&queries, qc->Name, (char*)qc);

		    /** Now, check for any aggregate/summary elements **/
		    for(j=0;j<subreq->nSubInf;j++)
		        {
			if (stStructType(subreq->SubInf[j]) == ST_T_SUBGROUP && !strcmp(subreq->SubInf[j]->UsrType,"report/aggregate"))
			    {
			    stAttrValue(stLookup(subreq->SubInf[j], "compute"), NULL, &ptr, 0);
			    if (!ptr)
			        {
				mssError(1,"RPT","report/aggregate element '%s' must have a compute= attribute", subreq->SubInf[j]->Name);
				err = 1;
				break;
				}
			    subst_str = rpt_internal_SubstParam(inf, ptr);
	    		    exp = expCompileExpression(subst_str->String, qc->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
			    xsDeInit(subst_str);
			    nmFree(subst_str,sizeof(XString));
			    if (!exp)
			        {
				mssError(0,"RPT","invalid compute expression in report/aggregate '%s'", subreq->SubInf[j]->Name);
				err = 1;
				break;
				}
			    exp->DataType = DATA_T_UNAVAILABLE;
		            expResetAggregates(exp, -1);
			    expEvalTree(exp,qc->ObjList);
			    xaAddItem(&(qc->AggregateExpList), (void*)exp);
			    xaAddItem(&(qc->AggregateDoReset), (void*)1);
			    ptr = NULL;
			    stAttrValue(stLookup(subreq->SubInf[j], "where"), NULL, &ptr, 0);
			    if (ptr)
			        {
			        subst_str = rpt_internal_SubstParam(inf, ptr);
				exp = expCompileExpression(subst_str->String, qc->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
			        xsDeInit(subst_str);
			        nmFree(subst_str,sizeof(XString));
				if (!exp)
				    {
				    mssError(0,"RPT","invalid where expression in report/aggregate '%s'", subreq->SubInf[j]->Name);
				    err = 1;
				    break;
				    }
			        xaAddItem(&(qc->AggregateWhereList), (void*)exp);
				}
			    else
			        {
			        xaAddItem(&(qc->AggregateWhereList), (void*)NULL);
				}
			    }
			}
		    }
#if 00
		else if (!strcmp(subreq->UsrType,"report/comment"))
		    {
		    if (rpt_internal_DoComment(inf, subreq, rs) <0)
		        {
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/column"))
		    {
		    if (rpt_internal_DoField(inf, subreq, rs, NULL) <0)
		        {
			err = 1;
			break;
			}
		    }
                else if (!strcmp(subreq->UsrType,"report/section"))
                    {
                    if (rpt_internal_DoSection(inf, subreq, rs, NULL) <0)
                        {
                        err = 1;
                        break;
                        }
                    }
#endif
		else if (!strcmp(subreq->UsrType,"report/table"))
		    {
		    if (rpt_internal_DoTable(inf, subreq,rs,rs->PageHandle) <0)
		        {
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/area"))
		    {
		    if (rpt_internal_DoArea(inf, subreq, rs, NULL, rs->PageHandle) < 0)
			{
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/image"))
		    {
		    if (rpt_internal_DoImage(inf, subreq, rs, NULL, rs->PageHandle) < 0)
			{
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/data"))
		    {
		    if (rpt_internal_DoData(inf, subreq, rs, rs->PageHandle) <0)
		        {
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/form"))
		    {
		    if (rpt_internal_DoForm(inf, subreq,rs, rs->PageHandle) <0)
		        {
			err = 1;
			break;
			}
		    }
		}
	    if (err) break;
	    }
	xhClear(&queries,rpt_internal_FreeQC, NULL);
	xhDeInit(&queries);
	inf->RSess = NULL;
	nmFree(rs,sizeof(RptSession));

	/** Undo formatting changes **/
	rpt_internal_CheckFormats(req,oldmfmt,olddfmt,oldnfmt,1);

	/** Undo the preprocessing - release the expression trees **/
	rpt_internal_UnPreProcess(inf, req, rs);

	/** Free the objlist **/
	expFreeParamList(inf->ObjList);

	/** End with a form feed, close up and get out. **/
	/*prtWriteFF(ps);*/

	/** Need to free up title string? **/
	if (title_str)
	    {
	    xsDeInit(title_str);
	    nmFree(title_str,sizeof(XString));
	    }

	/** Error? **/
	if (err) 
	    {
	    mssError(0,"RPT","Could not run report '%s'", inf->Obj->Pathname->Pathbuf);
	    return -1;
	    }

    return 0;
    }


/*** rpt_internal_Generator - this function actually generates the report content
 *** and writes it to the slave side of the socket pair, which the calling 
 *** user will read via the objRead call.
 ***/
void
rpt_internal_Generator(void* v)
    {
    pRptData inf = (pRptData)v;
    pPrtSession ps;

    	/** Set this thread's name **/
	thSetName(NULL,"Report Generator");

	/** Open a print session **/
	ps = prtOpenSession(inf->ContentType, fdWrite, inf->SlaveFD, PRT_OBJ_U_ALLOWBREAK);
	if (!ps) 
	    {
	    ps = prtOpenSession("text/html", fdWrite, inf->SlaveFD, PRT_OBJ_U_ALLOWBREAK);
	    if (ps) strcpy(inf->ContentType, "text/html");
	    }
	if (!ps)
	    {
	    inf->Flags |= RPT_F_ERROR;
	    mssError(1,"RPT","Could not locate an appropriate content generator for type '%s'", inf->ContentType);
	    fdWrite(inf->SlaveFD, "<PRE>\r\n",7,0,0);
	    mssPrintError(inf->SlaveFD);
	    fdWrite(inf->SlaveFD, "</PRE>\r\n",8,0,0);
	    fdClose(inf->SlaveFD,0);
	    thExit();
	    }

	/** Set image store location **/
	prtSetImageStore(ps, "/tmp/", "/tmp/", inf->Obj->Session, (void*(*)())objOpen, (void* (*)())objWrite, (void* (*)())objClose);

	/** Indicate that we've started up here... **/
	syPostSem(inf->StartSem, 1, 0);
	if (syGetSem(inf->IOSem, 1, 0) < 0)
	    {
	    prtCloseSession(ps);
	    fdClose(inf->SlaveFD,0);
	    thExit();
	    }

	/** Try to run the report... **/
	if (rpt_internal_Run(inf, inf->SlaveFD, ps) < 0)
	    {
	    inf->Flags |= RPT_F_ERROR;
	    fdWrite(inf->SlaveFD, "<PRE><FONT COLOR=black>\r\n",25,0,0);
	    mssPrintError(inf->SlaveFD);
	    fdWrite(inf->SlaveFD, "</FONT></PRE>\r\n",15,0,0);
	    prtCloseSession(ps);
	    fdClose(inf->SlaveFD,0);
	    thExit();
	    }

	/** Close the slave side and exit. **/
	prtCloseSession(ps);
	fdClose(inf->SlaveFD,0);
	thExit();

    return;
    }


/*** rpt_internal_SpawnGenerator - this function opens up a socketpair, and
 *** connects them to MTASK-enabled pFile's.  It then starts the report generator
 *** writing the report to the slave side of the socket pair, and whenever a
 *** read operation is done using objRead, the master side is read to obtain
 *** report data.
 ***/
int
rpt_internal_StartGenerator(pRptData inf)
    {
    int lowlevel_fd[2];

    	/** Create the socket pair and connect 'em with mtask **/
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, lowlevel_fd) < 0)
	    {
	    mssErrorErrno(1,"RPT","Could not open socketpair pipe for report generator");
	    return -1;
	    }
	inf->MasterFD = fdOpenFD(lowlevel_fd[0], O_RDONLY);
	inf->SlaveFD = fdOpenFD(lowlevel_fd[1], O_WRONLY);
	/*fdSetOptions(inf->SlaveFD, FD_UF_WRBUF);*/

	/** Create a new thread to start the report generation. **/
	if (!thCreate(rpt_internal_Generator, 0, (void*)inf))
	    {
	    mssError(1,"RPT","Failed to create thread for report generator");
	    fdClose(inf->MasterFD,0);
	    fdClose(inf->SlaveFD,0);
	    return -1;
	    }

    return 0;
    }


/*** rptOpen - open a new report for report generation.
 ***/
void*
rptOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pRptData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    int i;
    pStruct paramdata;
    pStructInf newparam;

    	/** This driver doesn't support sub-nodes.  Yet.  Check for that. **/
	if (obj->SubPtr != obj->Pathname->nElements)
	    {
	    return NULL;
	    }

	/** Determine node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** try to open it **/
	node = snReadNode(obj->Prev);

	/** If node access failed, quit out. **/
	if (!node)
	    {
	    mssError(0,"RPT","Could not open report node object");
	    return NULL;
	    }

	/** Allocate the structure **/
	inf = (pRptData)nmMalloc(sizeof(RptData));
	memset(inf, 0, sizeof(RptData));
	obj_internal_PathPart(obj->Pathname,0,0);
	if (!inf) return NULL;
	inf->Obj = obj;
	inf->Mask = mask;
	inf->AttrOverride = stCreateStruct(NULL,NULL);
	if (usrtype)
	    memccpy(inf->ContentType, usrtype, 0, 63);
	else
	    strcpy(inf->ContentType, "text/plain");
	inf->ContentType[63] = 0;
	xaInit(&(inf->UserDataSlots), 16);
	inf->NextUserDataSlot = 1;

	/** Lookup attribute/param override data in the openctl **/
	if (obj->Pathname->OpenCtl[obj->SubPtr-1])
	    {
	    paramdata = obj->Pathname->OpenCtl[obj->SubPtr-1];
	    for(i=0;i<paramdata->nSubInf;i++)
		{
		newparam = stAddAttr(inf->AttrOverride, paramdata->SubInf[i]->Name);
		stSetAttrValue(newparam, DATA_T_STRING, POD(&(paramdata->SubInf[i]->StrVal)), 0);
		}
	    }

	/** Content type must be application/octet-stream or more specific. **/
	rval = obj_internal_IsA(inf->ContentType, "application/octet-stream");
	if (rval == OBJSYS_NOT_ISA)
	    {
	    mssError(1,"RPT","Requested content type must be at least application/octet-stream");
	    xaDeInit(&(inf->UserDataSlots));
	    nmFree(inf,sizeof(RptData));
	    return NULL;
	    }
	if (rval < 0)
	    {
	    strcpy(inf->ContentType,"application/octet-stream");
	    }

	/** Set object params. **/
	inf->Node = node;
	inf->Node->OpenCnt++;
	obj->SubCnt = 1;
	inf->StartSem = syCreateSem(0,0);
	inf->IOSem = syCreateSem(0,0);

	/** Check format version **/
	if (stAttrValue(stLookup(inf->Node->Data,"version"),&(inf->Version),NULL,0) < 0)
	    inf->Version = 1;

    return (void*)inf;
    }


/*** rptClose - close an open file or directory.
 ***/
int
rptClose(void* inf_v, pObjTrxTree* oxt)
    {
    pRptData inf = RPT(inf_v);

    	/** Is the worker thread running?? **/
	if (inf->MasterFD != NULL)
	    {
	    fdClose(inf->MasterFD, 0);
	    inf->MasterFD = NULL;
	    }

	/** Release the memory **/
	inf->Node->OpenCnt --;
	stFreeInf(inf->AttrOverride);
	syDestroySem(inf->StartSem, SEM_U_HARDCLOSE);
	syDestroySem(inf->IOSem, SEM_U_HARDCLOSE);
	xaDeInit(&(inf->UserDataSlots));
	nmFree(inf,sizeof(RptData));

    return 0;
    }


/*** rptCreate - create a new file without actually opening that 
 *** file.
 ***/
int
rptCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = rptOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	rptClose(inf, oxt);

    return 0;
    }


/*** rptDelete - delete an existing file or directory.
 ***/
int
rptDelete(pObject obj, pObjTrxTree* oxt)
    {
    char* node_path;
    pSnNode node;

    	/** This driver doesn't support sub-nodes.  Yet.  Check for that. **/
	if (obj->SubPtr != obj->Pathname->nElements)
	    {
	    return -1;
	    }

	/** Determine node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);
	node = snReadNode(obj->Prev);
	if (!node) 
	    {
	    mssError(0,"RPT","Could not open report file node");
	    return -1;
	    }

	/** Delete the thing. **/
	if (snDelete(node) < 0) 
	    {
	    mssError(0,"RPT","Could not delete report file node");
	    return -1;
	    }

    return 0;
    }


/*** rptRead - Attempt to read from the report generator thread, and start
 *** that thread if it hasn't been started yet...
 ***/
int
rptRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pRptData inf = RPT(inf_v);
    int rcnt;

    	/** Is the worker thread running yet?  Start it if not. **/
	if (inf->MasterFD == NULL)
	    {
	    if (rpt_internal_StartGenerator(inf) < 0) 
	        return -1;
	    syGetSem(inf->StartSem, 1, 0);
	    }

	/** Attempt the read operation. **/
	syPostSem(inf->IOSem, 1, 0);
	rcnt = fdRead(inf->MasterFD, buffer, maxcnt, offset, flags);
	if (rcnt <= 0 && (inf->Flags & RPT_F_ERROR))
	    {
	    return -1;
	    }

    return rcnt;
    }


/*** rptWrite - This fails for reports.
 ***/
int
rptWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pRptData inf = RPT(inf_v);*/
    mssError(1,"RPT","Cannot write to a report generator object in system/report mode");
    return -1;
    }


/*** rptOpenQuery - open a directory query.  We don't support directory queries yet.
 ***/
void*
rptOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    /*pRptData inf = RPT(inf_v);*/
    pRptQuery qy;

    	qy = NULL;

    return (void*)qy;
    }


/*** rptQueryFetch - get the next directory entry as an open object.
 ***/
void*
rptQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pRptData inf;
    /*pRptQuery qy = ((pRptQuery)(qy_v));
    pObject llobj = NULL;
    char* objname = NULL;
    int cur_id = -1;*/

    	inf = NULL;

    return (void*)inf;
    }


/*** rptQueryClose - close the query.
 ***/
int
rptQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    /*pRptQuery qy = ((pRptQuery)(qy_v));*/

    return -1;
    }


/*** rptGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
rptGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pRptData inf = RPT(inf_v);
    char* ptr;
    pStructInf find_inf,value_inf,tmp_inf;
    int t;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname, "inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** Current page number is a predefined one for reports **/
	if (!strcmp(attrname,"page")) return DATA_T_INTEGER;

	/** Look for a sub inf structure for attribute information **/
	find_inf = stLookup(inf->Node->Data, attrname);
	if (inf->Version == 1)
	    {
	    if (!find_inf || stStructType(find_inf) != ST_T_ATTRIB) 
	        {
	        mssError(1,"RPT","Could not find requested report attribute '%s'", attrname);
	        return -1;
		}

	    /** Check override structure... **/
	    tmp_inf = stLookup(inf->AttrOverride,attrname);
	    if (tmp_inf) find_inf = tmp_inf;

	    t = stGetAttrType(find_inf, 0);
	    if (stAttrIsList(find_inf))
		{
		if (t == DATA_T_INTEGER) return DATA_T_INTVEC;
		else return DATA_T_STRINGVEC;
		}
	    else
		{
		return t;
		}
	    }
	else
	    {
	    if (!find_inf || stStructType(find_inf) != ST_T_SUBGROUP)
	        {
	        mssError(1,"RPT","Could not find requested report attribute '%s'", attrname);
	        return -1;
		}
	    value_inf = stLookup(find_inf,"type");
	    ptr="";
	    stAttrValue(value_inf,NULL,&ptr,0);
	    if (!strcmp(ptr,"integer")) return DATA_T_INTEGER;
	    else if (!strcmp(ptr,"string")) return DATA_T_STRING;
	    else if (!strcmp(ptr,"datetime")) return DATA_T_DATETIME;
	    else if (!strcmp(ptr,"double")) return DATA_T_DOUBLE;
	    else if (!strcmp(ptr,"money")) return DATA_T_MONEY;
	    else
	        {
		value_inf = stLookup(find_inf,"default");
		if (!value_inf) return DATA_T_UNAVAILABLE;

		t = stGetAttrType(value_inf, 0);
		if (stAttrIsList(value_inf))
		    {
		    if (t == DATA_T_INTEGER) return DATA_T_INTVEC;
		    else return DATA_T_STRINGVEC;
		    }
		else
		    {
		    return t;
		    }
		}
	    }

    return -1;
    }


/*** rptGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
rptGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pRptData inf = RPT(inf_v);
    pStructInf find_inf, value_inf, tmp_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /* val->String = inf->Node->Data->Name;*/
	    val->String = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements - 1, 0);
	    obj_internal_PathPart(inf->Obj->Pathname,0,0);
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
    	    /** Is the worker thread running yet?  Start it if not. **/
	    if (inf->MasterFD == NULL)
	        {
	        if (rpt_internal_StartGenerator(inf) < 0) 
	            return -1;
	        syGetSem(inf->StartSem, 1, 0);
	        }
	    val->String = inf->ContentType;
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->Node->Data->UsrType;
	    return 0;
	    }

	/** Caller is asking for current page #? **/
	if (!strcmp(attrname,"page"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be integer)", attrname);
		return -1;
		}
	    if (!inf->RSess || !inf->RSess->PSession)
	        {
		val->Integer = 1;
		}
	    else
	        {
		val->Integer = prtGetPageNumber(inf->RSess->PageHandle);
		}
	    return 0;
	    }

	/** Look through attributes below this inf structure. **/
	for(i=0;i<inf->Node->Data->nSubInf;i++)
	    {
	    find_inf = inf->Node->Data->SubInf[i];
	    if (!strcmp(attrname,find_inf->Name) && stStructType(find_inf) == ST_T_ATTRIB && inf->Version == 1)
                {
		tmp_inf = stLookup(inf->AttrOverride,attrname);
		if (tmp_inf) find_inf = tmp_inf;
		if (stGetAttrType(find_inf,0) == DATA_T_STRING && stAttrIsList(find_inf))
		    {
		    if (datatype != DATA_T_STRINGVEC)
			{
			mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be stringvec)", attrname);
			return -1;
			}
		    inf->VecData = stGetValueList(find_inf, DATA_T_STRING, &(inf->SVvalue.nStrings));
		    inf->SVvalue.Strings = (char**)(inf->VecData);
                    val->StringVec = &(inf->SVvalue);
		    }
                else if (stGetAttrType(find_inf,0) == DATA_T_INTEGER && stAttrIsList(find_inf))
                    {
		    if (datatype != DATA_T_INTVEC)
			{
			mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be intvec)", attrname);
			return -1;
			}
		    inf->VecData = stGetValueList(find_inf, DATA_T_INTEGER, &(inf->IVvalue.nIntegers));
		    inf->IVvalue.Integers = (int*)(inf->VecData);
                    val->IntVec = &(inf->IVvalue);
                    }
		else
		    {
		    stGetAttrValue(find_inf, datatype, val, 0);
		    }
                return 0;
                }
	    else if (!strcmp(attrname,find_inf->Name) && stStructType(find_inf) == ST_T_SUBGROUP && inf->Version >= 2)
	        {
		/** Check default= attr as well as override structure. **/
		value_inf = stLookup(find_inf,"default");
		tmp_inf = stLookup(inf->AttrOverride,attrname);
		if (tmp_inf) value_inf = tmp_inf;

		/** No default, and nothing set in override?  Null if so. **/
		if (!value_inf) return 1;

		/** Return result based on type. **/
	        tmp_inf = stLookup(find_inf,"type");
	        ptr="";
	        stAttrValue(tmp_inf,NULL,&ptr,0);
	        if (!strcmp(ptr,"integer"))
		    {
		    }
	        else if (!strcmp(ptr,"string"))
		    {
		    }
	        else if (!strcmp(ptr,"datetime"))
		    {
		    }
	        else if (!strcmp(ptr,"double"))
		    {
		    }
	        else if (!strcmp(ptr,"money"))
		    {
		    }
	        else
	            {
		    /** Return the data of the appropriate type. **/
		    if (stGetAttrType(value_inf,0) == DATA_T_STRING && stAttrIsList(value_inf))
			{
			if (datatype != DATA_T_STRINGVEC)
			    {
			    mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be stringvec)", attrname);
			    return -1;
			    }
			inf->VecData = stGetValueList(value_inf, DATA_T_STRING, &(inf->SVvalue.nStrings));
			inf->SVvalue.Strings = (char**)(inf->VecData);
			*(pStringVec*)val = &(inf->SVvalue);
			}
		    else if (stGetAttrType(value_inf,0) == DATA_T_INTEGER && stAttrIsList(value_inf))
			{
			if (datatype != DATA_T_INTVEC)
			    {
			    mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be intvec)", attrname);
			    return -1;
			    }
			inf->VecData = stGetValueList(value_inf, DATA_T_INTEGER, &(inf->IVvalue.nIntegers));
			inf->IVvalue.Integers = (int*)(inf->VecData);
			*(pIntVec*)val = &(inf->IVvalue);
			}
		    else
			{
			stGetAttrValue(value_inf, datatype, POD(val), 0);
			}
		    }
		}
            }

	/** If annotation, and not found, return "" **/
        if (!strcmp(attrname,"annotation"))
            {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
            val->String = "";
            return 0;
            }

	mssError(1,"RPT","Could not find requested report attribute '%s'", attrname);

    return -1;
    }


/*** rptGetNextAttr - get the next attribute name for this object.
 ***/
char*
rptGetNextAttr(void* inf_v, pObjTrxTree *oxt)
    {
    pRptData inf = RPT(inf_v);
    int i;
    pStructInf subinf;

    	/** Loop through top-level attributes in the inf structure. **/
	for(i=inf->AttrID;i<inf->Node->Data->nSubInf;i++)
	    {
	    subinf = (pStructInf)(inf->Node->Data->SubInf[i]);

	    /** Version 1: top-level attribute inf **/
	    if (inf->Version == 1)
	        {
	        if (stStructType(subinf) == ST_T_ATTRIB)
	            {
		    inf->AttrID = i+1;
		    return subinf->Name;
		    }
		}
	    else
	        {
		/** Version 2: top-level subgroup with default attr **/
		if (stStructType(subinf) == ST_T_SUBGROUP && !strcmp(subinf->UsrType,"system/attribute"))
		    {
		    inf->AttrID = i+1;
		    return subinf->Name;
		    }
		}
	    }
	
	/** No find? **/
	inf->AttrID = inf->Node->Data->nSubInf;

    return NULL;
    }


/*** rptGetFirstAttr - get the first attribute name for this object.
 ***/
char*
rptGetFirstAttr(void* inf_v, pObjTrxTree *oxt)
    {
    pRptData inf = RPT(inf_v);

    	/** Reset the attribute cnt. **/
	inf->AttrID = 0;

    return rptGetNextAttr(inf_v, oxt);
    }


/*** rptSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
rptSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree *oxt)
    {
    pRptData inf = RPT(inf_v);
    pStructInf find_inf;
    int type;
    int n;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"RPT","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /** GRB - error out on this for now.  The stuff that is needed to rename
	     ** a node like this isn't really in place.
	     **/
	    mssErrorErrno(1,"RPT","SetAttr 'name': could not rename report node object");
	    return -1;
	    if (inf->Node->Data)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(val->String) + 1 > 255)
		    {
		    mssError(1,"RPT","SetAttr 'name': name too long for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);

		/** GRB - rpt may not be in a fs file.  It is not this driver's
		 ** duty to call things like rename().
		 **/
	        /*if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0)
		    {
		    mssErrorErrno(1,"RPT","SetAttr 'name': could not rename report node object");
		    return -1;
		    }*/
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    strcpy(inf->Node->Data->Name,val->String);
	    return 0;
	    }
	
	/** Content-type?  can't set that **/
	if (!strcmp(attrname,"content_type")) 
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"RPT","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    mssError(1,"RPT","Illegal attempt to modify content type");
	    return -1;
	    }

	/** Otherwise, try to set top-level attrib inf value in the override struct **/
	/** First, see if the thing exists in the original inf struct. **/
	type = rptGetAttrType(inf_v, attrname, oxt);
	if (type < 0) return -1;
	if (datatype != type)
	    {
	    mssError(1,"RPT","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
		    attrname, obj_type_names[datatype], obj_type_names[type]);
	    return -1;
	    }

	/** Now, look for it in the override struct. **/
	find_inf = stLookup(inf->AttrOverride, attrname);

	/** If not found, add a new one **/
	if (!find_inf)
	    {
	    find_inf = stAddAttr(inf->AttrOverride, attrname);
	    n = 0;
	    }

	/** Set the value. **/
	if (find_inf)
	    {
	    stSetAttrValue(find_inf, type, val, 0);
	    return 0;
	    }

    return -1;
    }


/*** rptAddAttr - add an attribute to an object. Fails for this.
 ***/
int
rptAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree *oxt)
    {
    /*pRptData inf = RPT(inf_v);*/

    return -1;
    }


/*** rptOpenAttr - open an attribute as an object.  Fails.
 ***/
void*
rptOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree *oxt)
    {
    /*pRptData inf = RPT(inf_v);*/

    return NULL;
    }


/*** rptGetFirstMethod -- no methods.  Fails.
 ***/
char*
rptGetFirstMethod(void* inf_v, pObjTrxTree *oxt)
    {
    /*pRptData inf = RPT(inf_v);*/

    return NULL;
    }


/*** rptGetNextMethod -- no methods here.  Fails.
 ***/
char*
rptGetNextMethod(void* inf_v, pObjTrxTree *oxt)
    {
    /*pRptData inf = RPT(inf_v);*/

    return NULL;
    }


/*** rptExecuteMethod - no methods here.  Fails.
 ***/
int
rptExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree *oxt)
    {
    /*pRptData inf = RPT(inf_v);*/

    return -1;
    }


/*** rptInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem management layer.
 ***/
int
rptInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&RPT_INF,0,sizeof(RPT_INF));
	xaInit(&RPT_INF.Printers,16);

	/** Setup the structure **/
	strcpy(drv->Name,"RPT - Reporting Translation Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/report");

	/** Setup the function references. **/
	drv->Open = rptOpen;
	drv->Close = rptClose;
	drv->Create = rptCreate;
	drv->Delete = rptDelete;
	drv->OpenQuery = rptOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = rptQueryFetch;
	drv->QueryClose = rptQueryClose;
	drv->Read = rptRead;
	drv->Write = rptWrite;
	drv->GetAttrType = rptGetAttrType;
	drv->GetAttrValue = rptGetAttrValue;
	drv->GetFirstAttr = rptGetFirstAttr;
	drv->GetNextAttr = rptGetNextAttr;
	drv->SetAttrValue = rptSetAttrValue;
	drv->AddAttr = rptAddAttr;
	drv->OpenAttr = rptOpenAttr;
	drv->GetFirstMethod = rptGetFirstMethod;
	drv->GetNextMethod = rptGetNextMethod;
	drv->ExecuteMethod = rptExecuteMethod;
	/*drv->PresentationHints = rptPresentationHints*/;

	nmRegister(sizeof(RptData),"RptData");
	nmRegister(sizeof(RptQuery),"RptQuery");
	nmRegister(sizeof(QueryConn),"QueryConn");
	nmRegister(sizeof(RptSession),"RptSession");
	nmRegister(sizeof(PrintDriver),"PrintDriver");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

