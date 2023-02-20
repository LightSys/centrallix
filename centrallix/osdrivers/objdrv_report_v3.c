#define _GNU_SOURCE
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
#include <ctype.h>
#include <math.h>
#include "config.h"
#ifdef HAVE_MGL1
#include <mgl/mgl_c.h>
#endif
#ifdef HAVE_MGL2
#include <stdbool.h>
#include <complex.h>
#include <mgl2/mgl_cf.h>
#endif
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "stparse_ne.h"
#include "st_node.h"
#include "expression.h"
#include "report.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "cxlib/util.h"
#include "cxss/cxss.h"
#include "obfuscate.h"
#include "param.h"

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
    void*		Inf;	/* pRptData */
    }
    RptSession, *pRptSession;


/*** Structure for precompiled expressions, etc. ***/
typedef struct
    {
    pExpression		Exp;
    pExpression		LastValue;
    }
    RptUserData, *pRptUserData;


/*** Structure for representing a report parameter. ***/
typedef struct
    {
    pParam	Param;			/* parameter config and data, from .rpt file */
    int		Flags;			/* RPT_PARAM_F_xxx */
    XArray	ValueObjs;		/* List of objects referenced in the Default value */
    XArray	ValueAttrs;		/* List of attributes referenced in the Default value */
    void*	Inf;			/* pRptData */
    }
    RptParam, *pRptParam;

#define	RPT_PARAM_F_IN		1	/* input param */
#define RPT_PARAM_F_OUT		2	/* output param - can be both in/out, flags=3 */
#define RPT_PARAM_F_DEFAULT	4	/* value is from the parameter's default expression */
#define RPT_PARAM_F_EXPR	8	/* value is an expression that needs to be evaluated */


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[OBJSYS_MAX_PATH];
    char	DownloadAs[OBJSYS_MAX_PATH];
    char	DocumentFormat[64];
    char	ContentType[64];
    char	ObKey[64];
    char	ObRulefile[OBJSYS_MAX_PATH];
    int		Flags;
    int		LinkCnt;
    pObject	Obj;
    int		Mask;
    pSnNode	Node;
    pFile	MasterFD;
    pFile	SlaveFD;
    int		AttrID;
    StringVec	SVvalue;
    IntVec	IVvalue;
    void*	VecData;
    pParamObjects ObjList;
    pParamObjects SavedObjList;
    pRptSession	RSess;
    int		Version;
    pSemaphore	StartSem;
    pSemaphore	IOSem;
    XArray	UserDataSlots;
    int		NextUserDataSlot;
    int		NameIndex;
    pObfSession	ObfuscationSess;
    XArray	Params;
    }
    RptData, *pRptData;

#define RPT_F_ISDIR		1
#define RPT_F_ISOPEN		2
#define RPT_F_ERROR		4
#define RPT_F_ALREADY_FF	8
#define RPT_F_FORCELEAF		16

#define RPT(x) ((pRptData)(x))

#define RPT_MAX_CACHE	32


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pRptData	ObjInf;
    pObjQuery	Query;
    char	NameBuf[OBJSYS_MAX_PATH];
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
    XArray	Aggregates; /* of pRptSourceAgg */
    int		RecordCnt;
    int		InnerExecCnt;
    int		IsInnerOuter;
    char*	DataBuf;
    pRptData	Inf;
    }
    RptSource, *pRptSource;


/*** Structure for query aggregate ***/
typedef struct
    {
    char*	Name;
    pRptSource	Query;
    pExpression	Value;
    pExpression	Where;
    int		DoReset;
    }
    RptSourceAgg, *pRptSourceAgg;


/*** Structure used for source activation within table/form ***/
typedef struct
    {
    int		Count;
    int		StackPtr;
    int		InnerMode[EXPR_MAX_PARAMS];
    int		OuterMode[EXPR_MAX_PARAMS];
    pRptSource	Queries[EXPR_MAX_PARAMS];
    int		Flags[EXPR_MAX_PARAMS];
    char*	Names[EXPR_MAX_PARAMS];
    int		MultiMode;
    }
    RptActiveQueries, *pRptActiveQueries;

#define RPT_A_F_JUSTSTARTED	1
#define RPT_A_F_NEEDUPDATE	2

#define RPT_MM_NESTED		1
#define RPT_MM_PARALLEL		2
#define RPT_MM_SERIAL		3
#define RPT_MM_MULTINESTED	4


/*** Structure used for chart values ***/
typedef struct
    {
    char*	Label;		/* label for x axis */
    double*	Values;		/* array of n doubles, where n = number of series */
    char*	Types;		/* array of n 8-bit ints for data types */
    }
    RptChartValues, *pRptChartValues;


/*** Chart context ***/
typedef struct
    {
    pXArray	series;
    pXArray	values;
    pStructInf	x_axis;
    pStructInf	y_axis;
    HMDT	chart_data;
    HMGL	gr;
    int		xres;
    int		yres;
    double	stand_w;
    double	stand_h;
    int		x_pixels;
    int		y_pixels;
    double	font_scale_factor;
    int		fontsize;
    double	min;
    double	max;
    double	minaxis;
    double	maxaxis;
    int		tickdist;
    int		scale;
    char*	color;
    char**	x_labels;
    pRptData	inf;
    struct
	{
	double	left;
	double	top;
	double	right;
	double	bottom;
	}
	trim;
    int		rend_x_pixels;
    int		rend_y_pixels;
    int		rotation;
    double	zoom;
    }
    RptChartContext, *pRptChartContext;


/*** Chart type driver ***/
typedef struct
    {
    char	Type[32];		/* chart type: bar, line, graph */
    int		(*PreProcessData)();	/* preprocess chart data */
    int		(*SetupFormat)();	/* setup output formatting */
    int		(*Generate)();		/* generate the chart */
    }
    RptChartDriver, *pRptChartDriver;


/*** Globals. ***/
struct
    {
    XArray	ChartDrivers;
    }
    RPT_INF;

#define RPT_NUM_ATTR	7
static char* attrnames[RPT_NUM_ATTR] = {"bold","expanded","compressed","center","underline","italic","barcode"};

#define RPT_FP_FUDGE	(0.00001)


/*** MathGL color definitions ***/
char rpt_mgl_colors[30] = "krRgGbBwWcCmMyYhHlLeEnNuUqQpP";
int rpt_mgl_color_hex[29] =
    {
    0x000000,
    0xff0000, 0x800000,
    0x00ff00, 0x008000,
    0x0000ff, 0x000080,
    0xffffff, 0xb2b2b2,
    0x00ffff, 0x008080,
    0xff00ff, 0x800080,
    0xffff00, 0x808000,
    0x808080, 0x4c4c4c,
    0x00ff80, 0x008040,
    0x80ff00, 0x408000,
    0x0080ff, 0x004080,
    0x8000ff, 0x400080,
    0xff8000, 0x804000,
    0xff0080, 0x800040
    };


/*** A list of properties that use the UserData mechanism for value tracking
 *** and performance reasons.  Other values support expressions, too, but these
 *** are the main ones.
 ***/
typedef struct
    {
    char*	Object;
    char*	Attr;
    int		Required;
    }
    RptUDItem;

RptUDItem rpt_ud_list[] =
    {
	{ "report/area",	"condition",	0 },
	{ "report/area",	"value",	0 },

	{ "report/chart",	"condition",	0 },
	{ "report/chart-series","x_value",	0 },
	{ "report/chart-series","y_value",	0 },

	{ "report/data",	"condition",	0 },
	{ "report/data",	"value",	1 },

	{ "report/form",	"condition",	0 },

	{ "report/image",	"condition",	0 },

	{ "report/section",	"condition",	0 },

	{ "report/svg",		"condition",	0 },

	{ "report/table",	"condition",	0 },
	{ "report/table-row",	"condition",	0 },
	{ "report/table-row",	"summarize_for",0 },
	{ "report/table-row",	"value",	0 },
	{ "report/table-cell",	"condition",	0 },
	{ "report/table-cell",	"value",	0 },

	{ NULL,			NULL,		0 }
    };


/*** "unspecified" integer value ***/
#define RPT_INT_UNSPEC		(0x7FFFFFFF)


/*** Some needed forward function declarations **/
int rpt_internal_DoTable(pRptData, pStructInf, pRptSession, int container_handle);
int rpt_internal_DoTableRow(pRptData inf, pStructInf tablerow, pRptSession rs, int numcols, int table_handle, double colsep);
int rpt_internal_DoSection(pRptData, pStructInf, pRptSession, int container_handle);
int rpt_internal_DoData(pRptData, pStructInf, pRptSession, int container_handle);
int rpt_internal_DoForm(pRptData, pStructInf, pRptSession, int container_handle);
int rpt_internal_WriteExpResult(pRptSession rs, pExpression exp, int container_handle, char* attrname, char* typename);
int rpt_internal_SetMargins(pRptData inf, pStructInf config, int prt_obj, double dt, double db, double dl, double dr);
int rpt_internal_QyGetAttrType(void* qyobj, char* attrname);


/*** rpt_internal_RegisterChartDriver() - register a new chart driver
 ***/
int
rpt_internal_RegisterChartDriver(char* name, int (*preprocess_fn)(), int (*setup_fn)(), int (*generate_fn)())
    {
    pRptChartDriver drv;

	/** Allocate and register **/
	drv = (pRptChartDriver)nmMalloc(sizeof(RptChartDriver));
	if (!drv)
	    return -1;
	strtcpy(drv->Type, name, sizeof(drv->Type));
	drv->PreProcessData = preprocess_fn;
	drv->SetupFormat = setup_fn;
	drv->Generate = generate_fn;
	xaAddItem(&RPT_INF.ChartDrivers, (void*)drv);

    return 0;
    }


/*** rpt_internal_GetNULLstr - get the string to be substituted in place of
 *** a null value in tables and such.
 ***/
char*
rpt_internal_GetNULLstr()
    {
    char* ptr;
    cxssGetVariable("nfmt", &ptr, "NULL");
    return ptr;
    }


/*** rpt_internal_FreeParam - release a report parameter.
 ***/
int
rpt_internal_FreeParam(pRptParam rptparam)
    {

	if (rptparam->Param)
	    paramFree(rptparam->Param);
	xaClear(&rptparam->ValueObjs, (void*)nmSysFree, NULL);
	xaDeInit(&rptparam->ValueObjs);
	xaClear(&rptparam->ValueAttrs, (void*)nmSysFree, NULL);
	xaDeInit(&rptparam->ValueAttrs);
	nmFree(rptparam, sizeof(RptParam));

    return 0;
    }


/*** rpt_internal_GetValue - gets a value from the rpt configuration.
 ***/
int
rpt_internal_GetValue(pRptData inf, pStructInf config, char* attrname, int datatype, pObjData value, pObjData def, int nval)
    {
    pStructInf attr;
    pRptUserData ud;
    pExpression exp = NULL;
    static pExpression our_exp = NULL;
    char* str;

	if (our_exp)
	    expFreeExpression(our_exp);
	our_exp = NULL;

	/** Find the attr **/
	attr = stLookup(config, attrname);

	/** No such attr?  Use default if it is available. **/
	if (!attr)
	    {
	    if (def)
		{
		objCopyData(def, value, datatype);
		return 0;
		}
	    else
		{
		return -1;
		}
	    }

	/** Look in UserData first of all **/
	if (attr->UserData)
	    {
	    ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (intptr_t)(attr->UserData));
	    exp = ud->Exp;
	    if (expEvalTree(exp, inf->ObjList) < 0)
		{
		mssError(0,"RPT","Could not evaluate property '%s' on object '%s'", attr->Name, config->Name);
		goto error;
		}
	    }

	/** No user data; evaluate it directly. **/
	if (!exp)
	    {
	    exp = stGetExpression(attr, nval);
	    if (!exp)
		goto error;
	    if (!expIsConstant(exp))
		{
		exp = our_exp = expDuplicateExpression(exp);
		if (!our_exp)
		    goto error;
		expBindExpression(our_exp, inf->ObjList, EXPR_F_RUNSERVER);
		if (expEvalTree(our_exp, inf->ObjList) < 0)
		    {
		    mssError(0,"RPT","Could not evaluate property '%s' on object '%s'", attr->Name, config->Name);
		    goto error;
		    }
		}
	    }

	/** Null? **/
	if (exp->Flags & EXPR_F_NULL)
	    {
	    if (def)
		{
		objCopyData(def, value, datatype);
		return 0;
		}
	    else
		{
		goto error;
		}
	    }

	/** Return the expression's value, optionally converting. **/
	if (exp->DataType == datatype || datatype == DATA_T_CODE)
	    expExpressionToPod(exp, exp->DataType, value);
	else if (exp->DataType == DATA_T_INTEGER && datatype == DATA_T_DOUBLE)
	    value->Double = exp->Integer;
	else if (exp->DataType == DATA_T_INTEGER && datatype == DATA_T_MONEY)
	    objDataToMoney(DATA_T_INTEGER, &exp->Integer, value->Money);
	else if (exp->DataType == DATA_T_MONEY && datatype == DATA_T_DOUBLE)
	    value->Double = objDataToDouble(DATA_T_MONEY, &(exp->Types.Money));
	else if (exp->DataType == DATA_T_STRING && datatype == DATA_T_INTEGER)
	    {
	    /** Support both string-based integers and boolean names **/
	    str = exp->String;
	    if (str[0] == '-' || str[0] == '+' || isdigit(str[0]))
		value->Integer = objDataToInteger(DATA_T_STRING, str, NULL);
	    else if (!strcmp(str,"yes") || !strcmp(str,"true") || !strcmp(str,"on"))
		value->Integer = 1;
	    else if (!strcmp(str,"no") || !strcmp(str,"false") || !strcmp(str,"off"))
		value->Integer = 0;
	    else if (def)
		value->Integer = def->Integer;
	    else
		{
		mssError(1, "RPT", "Type mismatch accessing attribute '%s' of object '%s'", attr->Name, config->Name);
		goto error;
		}
	    }
	else if (exp->DataType == DATA_T_STRING && datatype == DATA_T_DOUBLE)
	    {
	    str = exp->String;
	    if (str[0] == ' ' || str[0] == '$')
		str++;
	    value->Double = objDataToDouble(DATA_T_STRING, str);
	    }
	else
	    {
	    mssError(1, "RPT", "Type mismatch accessing attribute '%s' of object '%s'", attr->Name, config->Name);
	    goto error;
	    }

	return exp->DataType;

    error:
	return -1;
    }


/*** rpt_internal_GetDouble - gets a double precision floating point value from
 *** the rpt config, automatically translating any integers into doubles
 ***/
int
rpt_internal_GetDouble(pRptData inf, pStructInf config, char* attr, double* val, double def, int nval)
    {
    int rval;

	/** Get the value, and set default on error. **/
	rval = rpt_internal_GetValue(inf, config, attr, DATA_T_DOUBLE, POD(val), isnan(def)?NULL:(POD(&def)), nval);
	if (rval < 0 && !isnan(def))
	    {
	    mssError(0,"RPT","Warning: invalid data for number '%s' on object '%s'; using default value.", attr, config->Name);
	    *val = def;
	    }

    return rval;
    }


/*** rpt_internal_GetBool - gets a boolean value, in the form of 0/1, true/false,
 *** on/off, or yes/no.
 ***/
int
rpt_internal_GetBool(pRptData inf, pStructInf config, char* attr, int def, int nval)
    {
    int val;
    if (rpt_internal_GetValue(inf, config, attr, DATA_T_INTEGER, POD(&val), POD(&def), nval) < 0)
	{
	mssError(0,"RPT","Warning: invalid data for boolean '%s' on object '%s'; using default value.", attr, config->Name);
	val = def;
	}
    return val;
    }


/*** rpt_internal_GetString - get a string value.
 ***/
int
rpt_internal_GetString(pRptData inf, pStructInf config, char* attr, char** val, char* def, int nval)
    {
    int rval;
	
	rval = rpt_internal_GetValue(inf, config, attr, DATA_T_STRING, POD(val), def?(POD(&def)):NULL, nval);
	if (rval < 0 && def)
	    {
	    mssError(0,"RPT","Warning: invalid data for string '%s' on object '%s'; using default value.", attr, config->Name);
	    *val = def;
	    }

    return rval;
    }


/*** rpt_internal_GetInteger - get a string value.
 ***/
int
rpt_internal_GetInteger(pRptData inf, pStructInf config, char* attr, int* val, int def, int nval)
    {
    int rval;
	
	rval = rpt_internal_GetValue(inf, config, attr, DATA_T_INTEGER, POD(val), (def != RPT_INT_UNSPEC)?(POD(&def)):NULL, nval);
	if (rval < 0 && def != RPT_INT_UNSPEC)
	    {
	    mssError(0,"RPT","Warning: invalid data for string '%s' on object '%s'; using default value.", attr, config->Name);
	    *val = def;
	    }

    return rval;
    }


/*** rpt_internal_GetParam - retrieves a parameter.
 ***/
pRptParam
rpt_internal_GetParam(pRptData inf, char* paramname)
    {
    int i;
    pRptParam rptparam;

	/** Scan for this param **/
	for(i=0; i<inf->Params.nItems; i++)
	    {
	    rptparam = (pRptParam)inf->Params.Items[i];
	    if (!strcmp(paramname, rptparam->Param->Name))
		return rptparam;
	    }

    return NULL;
    }


/*** rpt_internal_EvalParam - evaluate a parameter's value by applying
 *** presentation hints (default).
 ***/
int
rpt_internal_EvalParam(pRptParam rptparam, pParamObjects objlist, pObjSession sess)
    {
    int use_default;
    int rval = 0;
    pRptSource src;
    int i;
    unsigned int active_mask = 0;

	/** Was this param null or otherwise a default value **/
	use_default = ((rptparam->Flags & RPT_PARAM_F_DEFAULT) || (rptparam->Param->Value->Flags & DATA_TF_NULL)) && rptparam->Param->Hints && rptparam->Param->Hints->DefaultExpr;

	/** Re-evaluate hints **/
	if (use_default)
	    {
	    /** Determine active sources mask **/
	    for(i=0; i<objlist->nObjects; i++)
		{
		if (objlist->GetTypeFn[i] == rpt_internal_QyGetAttrType)
		    {
		    src = (pRptSource)objlist->Objects[i];

		    /** Source exists and is active? **/
		    if (src && src->Query != NULL)
			active_mask |= (1<<i);
		    }
		else if (!(rptparam->Flags & RPT_PARAM_F_DEFAULT))
		    {
		    /** First time being evaluated? If so, non-rpt sources are "active" **/
		    active_mask |= (1<<i);
		    }
		}
	    if (!(rptparam->Flags & RPT_PARAM_F_DEFAULT))
		{
		/** Add ext references to eval if first time **/
		active_mask |= (EXPR_MASK_EXTREF | EXPR_MASK_INDETERMINATE);
		}

	    /** Only re-evaluate if there are active sources in the expression,
	     ** or if it's the first evaluation and no sources are referenced.
	     **/
	    if ((EXPR(rptparam->Param->Hints->DefaultExpr)->ObjCoverageMask & active_mask) || ((EXPR(rptparam->Param->Hints->DefaultExpr)->ObjCoverageMask & EXPR_MASK_ALLOBJECTS) == 0 && !(rptparam->Flags & RPT_PARAM_F_DEFAULT)))
		{
		/** Re-evaluate **/
		rptparam->Param->Value->Flags |= DATA_TF_NULL;
		rval = paramEvalHints(rptparam->Param, objlist, sess);
		}
	    }

	/** Re-set was-default flag if needed. **/
	rptparam->Flags &= ~RPT_PARAM_F_DEFAULT;
	if (use_default)
	    rptparam->Flags |= RPT_PARAM_F_DEFAULT;

    return rval;
    }


/*** rpt_internal_GetParamType - gets the value of a parameter
 ***/
int
rpt_internal_GetParamType(pRptData inf, char* paramname, pParamObjects objlist, pObjSession sess)
    {
    pRptParam rptparam;

	rptparam = rpt_internal_GetParam(inf, paramname);
	if (!rptparam)
	    return -1;

	/** Evalute **/
	rpt_internal_EvalParam(rptparam, objlist, sess);

    return paramGetType(rptparam->Param);
    }


/*** rpt_internal_GetParamValue - gets the value of a parameter
 ***/
int
rpt_internal_GetParamValue(pRptData inf, char* paramname, int datatype, pObjData value, pParamObjects objlist, pObjSession sess)
    {
    pRptParam rptparam;

	rptparam = rpt_internal_GetParam(inf, paramname);
	if (!rptparam)
	    return -1;

	/** Evalute **/
	rpt_internal_EvalParam(rptparam, objlist, sess);

    return paramGetValue(rptparam->Param, datatype, value);
    }


/*** rpt_internal_SetParamValue - set the value of a parameter
 ***/
int
rpt_internal_SetParamValue(pRptParam rptparam, int datatype, pObjData value, pParamObjects objlist, pObjSession sess)
    {
    int rval;

	rval = paramSetValueDirect(rptparam->Param, datatype, value, 0, objlist, sess);

	/** If successful, we no longer invoke the default **/
	if (rval >= 0)
	    {
	    rptparam->Flags &= ~RPT_PARAM_F_DEFAULT;
	    }

    return rval;
    }


/*** rpt_internal_CheckParamUpdate - scan the list of parameters for
 *** params that are still in default mode, and which reference the
 *** indicated object.  Update the default values on all of those
 *** parameters.  Attribute name will also be matched unless null.
 ***/
int
rpt_internal_CheckParamUpdate(pRptData inf, char* objname, char* attrname)
    {
    int i, j;
    pRptParam rptparam;
    char* param_objname;
    char* param_attrname;

	/** Scan through parameters **/
	for(i=0; i<inf->Params.nItems; i++)
	    {
	    rptparam = (pRptParam)inf->Params.Items[i];

	    /** See if this is a match based on object/attr names **/
	    for(j=0; j<rptparam->ValueObjs.nItems; j++)
		{
		param_objname = (char*)rptparam->ValueObjs.Items[j];
		param_attrname = (char*)rptparam->ValueAttrs.Items[j];

		/** Re-evaluate default if names match **/
		if (!strcmp(objname, param_objname) && (!attrname || !strcmp(attrname, param_attrname)))
		    {
		    rpt_internal_EvalParam(rptparam, inf->ObjList, inf->ObjList->Session);
		    break;
		    }
		}
	    }

    return 0;
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
    pRptParam rptparam;
    char* sptr;

    	/** Allocate our string. **/
	dest = xsNew();
	if (!dest) return NULL;

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
	    rptparam = rpt_internal_GetParam(inf, paramname);
	    if (!rptparam)
	        {
		xsConcatenate(dest, "&", 1);
		src = ptr+1;
		continue;
		}

	    /** Convert value to a string. **/
	    sptr = ptodToStringTmp(rptparam->Param->Value);
	    if (sptr)
		{
		xsConcatenate(dest, sptr, -1);
		}
	    else
		{
		xsConcatenate(dest, "&", 1);
		src = ptr+1;
		continue;
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
rpt_internal_CheckFormats(pRptData data, pStructInf inf)
    {
    char* newmfmt=NULL;
    char* newdfmt=NULL;
    char* newnfmt=NULL;
    char* neg_fc=NULL;

	    /** Lookup possible 'dateformat','moneyformat' **/
	    rpt_internal_GetString(data, inf, "dateformat", &newdfmt, NULL, 0);
	    rpt_internal_GetString(data, inf, "moneyformat", &newmfmt, NULL, 0);
	    rpt_internal_GetString(data, inf, "nullformat", &newnfmt, NULL, 0);
	    if (newdfmt) 
	        {
		cxssSetVariable("dfmt", newdfmt, 0);
		}
	    if (newmfmt) 
	        {
		cxssSetVariable("mfmt", newmfmt, 0);
		}
	    if (newnfmt)
	        {
		cxssSetVariable("nfmt", newnfmt, 0);
		}

	    /** Negative font color **/
	    rpt_internal_GetString(data, inf, "negative_fontcolor", &neg_fc, NULL, 0);
	    if (neg_fc)
	        {
		cxssSetVariable("rpt:neg_fc", neg_fc, 0);
		}

    return 0;
    }


/*** rpt_internal_CheckGoto - check for xpos/ypos settings for a field or a
 *** comment element.
 ***/
int
rpt_internal_CheckGoto(pRptSession rs, pStructInf object, int container_handle)
    {
    pRptData inf = (pRptData)rs->Inf;
    double xpos = -1.1;
    double ypos = -1.1;

    	/** Check for ypos and xpos **/
	rpt_internal_GetDouble(inf, object, "xpos", &xpos, -1.1, 0);
	rpt_internal_GetDouble(inf, object, "ypos", &ypos, -1.1, 0);

	/** If ypos is set, do it first **/
	if (ypos > -1) prtSetVPos(container_handle, (double)ypos);

	/** Next check xpos **/
	if (xpos > -1) prtSetHPos(container_handle, (double)xpos);

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
rpt_internal_ProcessAggregates(pRptSource qy)
    {
    int i;
    pRptSourceAgg aggregate;

    	/** Search through sub-objects of the report/query element **/
	for(i=0;i<qy->Aggregates.nItems;i++)
	    {
	    aggregate = (pRptSourceAgg)qy->Aggregates.Items[i];
	    if (aggregate->Where)
	        {
	        expModifyParam(qy->ObjList, "this", (void*)qy);
		if (expEvalTree(aggregate->Where, qy->ObjList) < 0)
		    {
		    mssError(0,"RPT","Error evaluating where= in aggregate");
		    return -1;
		    }
		if ((aggregate->Where->Flags & EXPR_F_NULL) || aggregate->Where->DataType != DATA_T_INTEGER || aggregate->Where->Integer == 0)
		    {
		    continue;
		    }
		}
	    expModifyParam(qy->ObjList, "this", (void*)qy);
	    expUnlockAggregates(aggregate->Value, 1);
	    if (expEvalTree(aggregate->Value, qy->ObjList) < 0)
	        {
		mssError(0,"RPT","Error evaluating compute= in aggregate");
		return -1;
		}
	    rpt_internal_CheckParamUpdate(qy->Inf, qy->Name, aggregate->Name);
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
    pObject obj;
    pRptSource qy;
    int i;
    pRptSourceAgg aggregate;

	/** Query freed? **/
	if (!qyobj)
	    return -1;

	qy = (pRptSource)qyobj;
	obj = qy->QueryItem;

    	/** Search for aggregates first. **/
	for(i=0;i<qy->Aggregates.nItems;i++)
	    {
	    aggregate = (pRptSourceAgg)qy->Aggregates.Items[i];
	    if (!strcmp(aggregate->Name, attrname))
		{
		return aggregate->Value->DataType;
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
    pObject obj;
    pRptSource qy;
    int i, was_null;
    pExpression exp;
    int rval;
    void* data_buf = NULL;
    pRptSourceAgg aggregate;

	/** Query freed? **/
	if (!qyobj)
	    return -1;

	qy = (pRptSource)qyobj;
	obj = qy->QueryItem;

    	/** Search for aggregates first. **/
	for(i=0;i<qy->Aggregates.nItems;i++)
	    {
	    aggregate = (pRptSourceAgg)qy->Aggregates.Items[i];

	    if (!strcmp(aggregate->Name, attrname))
		{
		exp = aggregate->Value;
		was_null = ((exp->Flags & EXPR_F_NULL) != 0);

		if (datatype != exp->DataType && !was_null)
		    {
		    mssError(1,"RPT","Type mismatch accessing query property '%s' [requested=%s, actual=%s]",
			    attrname, datatype, exp->DataType);
		    return -1;
		    }

		if (!was_null)
		    {
		    switch(exp->DataType)
			{
			case DATA_T_INTEGER:
			    data_ptr->Integer = exp->Integer;
			    break;
			case DATA_T_DOUBLE:
			    data_ptr->Double = exp->Types.Double;
			    break;
			case DATA_T_STRING: 
			    data_buf = (char*)nmSysMalloc(strlen(exp->String)+1);
			    data_ptr->String = data_buf;
			    strcpy(data_buf, exp->String);
			    break;
			case DATA_T_MONEY: 
			    data_buf = (char*)nmSysMalloc(sizeof(MoneyType));
			    memcpy(data_buf, &(exp->Types.Money), sizeof(MoneyType));
			    data_ptr->Money = (pMoneyType)(data_buf);
			    break;
			case DATA_T_DATETIME: 
			    data_buf = (char*)nmSysMalloc(sizeof(DateTime));
			    memcpy(data_buf, &(exp->Types.Date), sizeof(DateTime));
			    data_ptr->DateTime = (pDateTime)(data_buf);
			    break;
			default: return -1;
			}
		    }

		/** Reset it after being read **/
		if (aggregate->DoReset)
		    {
		    expResetAggregates(exp, -1, 1);
		    expEvalTree(exp, qy->ObjList);
		    }

		/** Free existing query conn data buf? **/
		if (qy->DataBuf)
		    {
		    nmSysFree(qy->DataBuf);
		    qy->DataBuf = NULL;
		    }

		/** Return our new data buffer for string/money/datetime **/
		qy->DataBuf = data_buf;

		return was_null;
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

	rval = objGetAttrValue(obj, attrname, datatype, data_ptr);
    /*if (!strcmp(qy->Name, "summary2_qy"))
	{
	if (rval == 0)
	    printf("%s: $ %d %2.2d\n", attrname, (*(pMoneyType*)data_ptr)->WholePart, (*(pMoneyType*)data_ptr)->FractionPart);
	else
	    printf("%s: null\n", attrname);
	}*/

	/** Obfuscate data? **/
	/*if (qy->Inf->ObfuscationSess && rval == 0)
	    {
	    switch(datatype)
		{
		case DATA_T_INTEGER:
		    obfObfuscateDataSess(qy->Inf->ObfuscationSess, data_ptr, &od, DATA_T_INTEGER, attrname, NULL, qy->Name);
		    data_ptr->Integer = od.Integer;
		    break;
		case DATA_T_DOUBLE:
		    obfObfuscateDataSess(qy->Inf->ObfuscationSess, data_ptr, &od, DATA_T_DOUBLE, attrname, NULL, qy->Name);
		    data_ptr->Double = od.Double;
		    break;
		case DATA_T_STRING:
		    obfObfuscateDataSess(qy->Inf->ObfuscationSess, data_ptr, &od, DATA_T_STRING, attrname, NULL, qy->Name);
		    if (qy->DataBuf) nmSysFree(qy->DataBuf);
		    data_ptr->String = qy->DataBuf = nmSysStrdup(od.String);
		    break;
		case DATA_T_MONEY:
		    obfObfuscateDataSess(qy->Inf->ObfuscationSess, data_ptr, &od, DATA_T_MONEY, attrname, NULL, qy->Name);
		    if (qy->DataBuf) nmSysFree(qy->DataBuf);
		    qy->DataBuf = nmSysMalloc(sizeof(MoneyType));
		    data_ptr->Money = (pMoneyType)qy->DataBuf;
		    memcpy(data_ptr->Money, od.Money, sizeof(MoneyType));
		    break;
		case DATA_T_DATETIME:
		    obfObfuscateDataSess(qy->Inf->ObfuscationSess, data_ptr, &od, DATA_T_DATETIME, attrname, NULL, qy->Name);
		    if (qy->DataBuf) nmSysFree(qy->DataBuf);
		    qy->DataBuf = nmSysMalloc(sizeof(DateTime));
		    data_ptr->DateTime = (pDateTime)qy->DataBuf;
		    memcpy(data_ptr->DateTime, od.DateTime, sizeof(DateTime));
		    break;
		}
	    }*/

    return rval;
    }

/*** rpt_internal_PrepareQuery - perform all operations necessary to prepare
 *** for the retrieval of a query result set.  This is done in both the 
 *** tabular and free-form result set types.  The function returns the query
 *** connection structure extracted from the hash table 'queryinf', with that
 *** structure set up with an open ObjectSystem query in ->Query.
 ***/
pRptSource
rpt_internal_PrepareQuery(pRptData inf, pStructInf object, pRptSession rs, int index)
    {
    pRptSource qy;
    /*char* newsql;*/
    int i,cnt,v;
    char* ptr;
    char* src=NULL;
    char* sql;
    XArray links;
    pRptSource lqy;
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
    pStructInf sql_inf;
    pExpression exp = NULL;

	/** Lookup the database query information **/
	if (rpt_internal_GetString(inf, object, "source", &src, NULL, index) < 0)
	    {
            mssError(1,"RPT","Source required for table/form '%s'", object->Name);
	    return NULL;
	    }
	qy = (pRptSource)xhLookup(rs->Queries, src);
	if (!qy)
	    {
            mssError(1,"RPT","Source '%s' given for table/form '%s' is undefined", src, object->Name);
	    return NULL;
	    }

	/** Check for a mode entry **/
	if (rpt_internal_GetString(inf, object, "mode", &ptr, NULL, index) >= 0)
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
	sql_inf = stLookup(qy->UserInf, "sql");
	if (sql_inf)
	    {
	    t = stGetAttrType(sql_inf, 0);
	    if (t == DATA_T_STRING)
		{
		stGetAttrValue(sql_inf, t, POD(&sql), 0);
		}
	    else if (t == DATA_T_CODE)
		{
		exp = NULL;
		stGetAttrValue(sql_inf, t, POD(&exp), 0);
		if (exp)
		    {
		    exp = expDuplicateExpression(exp);
		    expBindExpression(exp, inf->ObjList, EXPR_F_RUNSERVER);
		    if (expEvalTree(exp, inf->ObjList) == 0)
			{
			expExpressionToPod(exp, DATA_T_STRING, POD(&sql));
			}
		    }
		}
	    }
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
		    xsFree(sql_str);
	    	    return NULL;
		    }
		ptr=NULL;
		stAttrValue(ui,NULL,&ptr,0);
		if (!ptr)
		    {
            	    mssError(1,"RPT","Table/Form source query linkage type mismatch in query '%s'", src);
		    xsFree(sql_str);
	    	    return NULL;
		    }
		lqy=(pRptSource)xhLookup(rs->Queries,ptr);
		if (!lqy)
		    {
		    mssError(1,"RPT","Undefined linkage in table/form source query '%s' (%s)",src, ptr);
		    xsFree(sql_str);
		    return NULL;
		    }
		stAttrValue(ui,NULL,&cname,1);
		if (!cname)
		    {
		    mssError(1,"RPT","Column name in table/form source query '%s' linkage '%s' is undefined", src, ptr);
		    xsFree(sql_str);
		    return NULL;
		    }
		lvalue=NULL;
		if (!lqy->QueryItem)
		    {
		    mssError(1,"RPT","Table/Form source query '%s' linkage '%s' not active", src,ptr);
		    xsFree(sql_str);
		    return NULL;
		    }
		t = objGetAttrType(lqy->QueryItem, cname);
		if (t < 0)
		    {
		    mssError(1,"RPT","Unknown column '%s' in table/form source query '%s' linkage '%s'", cname, src, ptr);
		    xsFree(sql_str);
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
	newsql = xsNew();
	cnt=0;
	while(*sql)
	    {
	    if (sql[0] == '&' && (sql[1] >= '0' && sql[1] <= '9'))
		{
		/*newsql[cnt++] = '"';*/
		v = strtoi(sql+1,&endptr,10);
		if (v == 0 || v > links.nItems)
		    {
		    mssError(1,"RPT","Parameter error in table/form '%s' source '%s' sql", object->Name, src);
		    nmSysFree(newsql);
		    xsFree(sql_str);
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
	xsFree(sql_str);

	/** Ok, now issue the query. **/
	qy->Query = objMultiQuery(rs->ObjSess, newsql->String, inf->ObjList, 0);
	xsFree(newsql);
	if (!qy->Query) 
	    {
	    mssError(0,"RPT","Could not open report/query '%s' SQL for form/table '%s'", src, object->Name);
	    return NULL;
	    }
	xaDeInit(&links);

	if (exp) expFreeExpression(exp);
    
    return qy;
    }


/*** rpt_internal_DoSection - process a report section, which is an 
 *** abstract division in the report normally used simply to change the
 *** formatting style, such as margins, columns, etc.
 ***/
int
rpt_internal_DoSection(pRptData inf, pStructInf section, pRptSession rs, int container_handle)
    {
    int section_handle = -1;
    int area_handle = -1;
    double x, y, w, h, bw;
    double sepw;
    int flags;
    pPrtBorder bdr = NULL;
    int rval = 0;
    int is_balanced;
    int n_cols;

	rval = rpt_internal_CheckCondition(inf,section);
	if (rval < 0)
	    goto error;
	if (rval == 0)
	    return 0;

	/** Get section geometry **/
	rpt_internal_GetDouble(inf, section, "x", &x, -1.0, 0);
	rpt_internal_GetDouble(inf, section, "y", &y, -1.0, 0);
	rpt_internal_GetDouble(inf, section, "width", &w, -1.0, 0);
	rpt_internal_GetDouble(inf, section, "height", &h, -1.0, 0);
	rpt_internal_GetDouble(inf, section, "colsep", &sepw, 0.0, 0);
	n_cols = -1;
	rpt_internal_GetInteger(inf, section, "columns", &n_cols, 0, 0);
	if (n_cols <= 0)
	    {
	    mssError(1,"RPT","The 'columns' attribute must specify a valid number of columns");
	    goto error;
	    }

	/** Check for flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;
	if (rpt_internal_GetBool(inf, section, "allowbreak", 1, 0)) flags |= PRT_OBJ_U_ALLOWBREAK;
	if (rpt_internal_GetBool(inf, section, "fixedsize", 0, 0)) flags |= PRT_OBJ_U_FIXEDSIZE;
	is_balanced = rpt_internal_GetBool(inf, section, "balanced", 0, 0);

	/** Check for border **/
	if (rpt_internal_GetDouble(inf, section, "border", &bw, NAN, 0) >= 0)
	    {
	    bdr = prtAllocBorder(1, 0.0, 0.0, bw, 0x000000);
	    }

	/** Create the section **/
	if (bdr)
	    section_handle = prtAddObject(container_handle, PRT_OBJ_T_SECTION, x, y, w, h, flags, "numcols", n_cols, "colsep", sepw, "balanced", is_balanced, "separator", bdr, NULL);
	else
	    section_handle = prtAddObject(container_handle, PRT_OBJ_T_SECTION, x, y, w, h, flags, "numcols", n_cols, "colsep", sepw, "balanced", is_balanced, NULL);
	if (bdr)
	    {
	    prtFreeBorder(bdr);
	    bdr = NULL;
	    }
	if (section_handle < 0)
	    goto error;

	area_handle = prtAddObject(section_handle, PRT_OBJ_T_AREA, 0, 0, -1, 0, PRT_OBJ_U_ALLOWBREAK, NULL);
	if (area_handle < 0)
	    goto error;

	/** Set the style for the section **/
	if (rpt_internal_SetStyle(inf, section, rs, area_handle) < 0)
	    goto error;
	if (rpt_internal_SetMargins(inf, section, area_handle, 0, 0, 0, 0) < 0)
	    goto error;

	/** Now do sub-components. **/
	if (rpt_internal_DoContainer(inf, section, rs, area_handle) < 0)
	    goto error;

	/** End the section **/
	prtEndObject(area_handle);
	prtEndObject(section_handle);

	return 0;

    error:
	if (bdr) prtFreeBorder(bdr);
	if (area_handle >= 0) prtEndObject(area_handle);
	if (section_handle >= 0) prtEndObject(section_handle);
	return -1;
    }


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
		if (this->OuterMode[stack_ptr])
		    this->Queries[stack_ptr]->InnerExecCnt = 0;
	        if (this->InnerMode[stack_ptr])
		    this->Queries[stack_ptr]->InnerExecCnt++;
		if (!this->InnerMode[stack_ptr])
		    this->Queries[stack_ptr]->QueryItem = NULL;
		}

	    /** Need to fetch a query item?
	     **
	     ** Here's how this works.  This is an iterative implementation of
	     ** an algorithm that could otherwise be recursive, and it has to
	     ** take into account having multiple queries, two nesting options
	     ** (nested or multinested), and three modes (inner, outer, and
	     ** normal).  It's a bear.
	     **
	     ** new_item tracks if we're recursing back to retrieve a new
	     ** parent query item after a child query has returned its last
	     ** row.
	     **
	     ** is_initial is set to 1 if we're "bootstrapping" this process
	     ** as a result of fetching the first row (otherwise this would
	     ** never return any rows at all, because it would think it has
	     ** finished the querying already).
	     **
	     ** stack_ptr == this->Count-1 means we're working with the leaf-
	     ** level query (last one in the sources list).
	     **
	     ** OuterMode means we don't actually iterate any query results,
	     ** but just make sure there is one available for future iteration.
	     **
	     ** InnerMode means we iterate query results in the context of an
	     ** OuterMode form at a higher level.
	     **/
	    if ((this->Queries[stack_ptr]->Query &&
			(this->Queries[stack_ptr]->QueryItem == NULL || (new_item && !this->OuterMode[stack_ptr]))) ||
	        (stack_ptr == this->Count-1 &&
			!this->OuterMode[stack_ptr] &&
			(!is_initial || !this->InnerMode[stack_ptr])))
	        {
		/** Fetch an item from this source **/
		/*printf("  NEXTREC: Fetching item from source '%s' NI=%d O=%d QI=%d\n", this->Names[stack_ptr], new_item, this->OuterMode[stack_ptr], this->Queries[stack_ptr]->QueryItem);*/
		/*printf("  NEXTREC: Fetching item from source '%s' O=%d I=%d INIT=%d\n", this->Names[stack_ptr], this->OuterMode[stack_ptr], this->InnerMode[stack_ptr], is_initial);*/
		new_item = 0;
		if (this->Queries[stack_ptr]->QueryItem)
		    objClose(this->Queries[stack_ptr]->QueryItem);
		this->Queries[stack_ptr]->QueryItem = objQueryFetch(this->Queries[stack_ptr]->Query, O_RDONLY);
		id = expModifyParam(inf->ObjList, this->Queries[stack_ptr]->Name, (void*)(this->Queries[stack_ptr]));
		inf->ObjList->CurrentID = id;

		/** End of items from this particular query source? **/
		if (this->Queries[stack_ptr]->QueryItem == NULL)
		    {
		    /** If the form/table is in "inner" mode, we do not close
		     ** the query -- that will happen when the "outer" mode
		     ** form loops around again.
		     **/
		    /*printf("  NEXTREC: No more items!\n");*/
		    if (!this->InnerMode[stack_ptr])
		        {
			objQueryClose(this->Queries[stack_ptr]->Query);
		        this->Queries[stack_ptr]->Query = NULL;
			this->Queries[stack_ptr] = NULL;
			}

		    /** Recurse up and try to fetch the next item from the next
		     ** query level up.
		     **
		     ** If stack_ptr < 0, that means we have no more rows,
		     ** because we have recursed back up past the top level
		     ** query.
		     **/
		    new_item = 1;
		    stack_ptr--;
		    if (stack_ptr < 0)
			return 1;
		    continue;
		    }

		/** Got one result from this particular query source. **/
		this->Queries[stack_ptr]->RecordCnt++;
		this->Flags[stack_ptr] |= RPT_A_F_NEEDUPDATE;

		/** We exit here if we've filled out a query result object from
		 ** each query, or if we're doing "multinested" nesting, in
		 ** which case we return a "header" row for each query level.
		 **/
		stack_ptr++;
		if (stack_ptr == this->Count || this->MultiMode == RPT_MM_MULTINESTED)
		    return 0;

		/** Go back around and try to fill out the next object in the
		 ** sources list.
		 **/
		continue;
		}

	    /** New item not retrieved because of outer mode?  Return OK if so.
	     ** Otherwise, we're at the end of our results.
	     **/
	    if ((new_item || stack_ptr == this->Count-1) && this->OuterMode[stack_ptr]) 
		return 0;
	    else if (new_item)
		return 1;

	    /** Ok, valid item still, or outer mode so we don't bother **/
	    new_item = 0;
	    stack_ptr++;
	    if (stack_ptr == this->Count)
		return 0;
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

	/** Find queries that have updated data **/
	for(i=0;i<this->Count;i++)
	    {
	    if (this->Flags[i] & RPT_A_F_NEEDUPDATE)
	        {
		this->Flags[i] &= ~RPT_A_F_NEEDUPDATE;

		/** Update aggregates **/
	        if (rpt_internal_ProcessAggregates(this->Queries[i]) < 0)
		    {
		    err = 1;
		    break;
		    }

		/** Update any parameters depending on this data **/
		rpt_internal_CheckParamUpdate(this->Queries[i]->Inf, this->Queries[i]->Name, NULL);
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
	rpt_internal_GetString(inf, object, "multimode", &ptr, "nested", 0);
	if (!strcmp(ptr,"parallel"))
	    ac->MultiMode = RPT_MM_PARALLEL;
	else if (!strcmp(ptr,"nested"))
	    ac->MultiMode = RPT_MM_NESTED;
	else if (!strcmp(ptr,"multinested"))
	    ac->MultiMode = RPT_MM_MULTINESTED;
	else
	    {
	    mssError(1,"RPT","Invalid multimode <%s>.  Valid types = nested,parallel,multinested", ptr);
	    return NULL;
	    }

	/** Scan through the object's inf looking for source, etc **/
	ac->Count = 0;
	while(1)
	    {
	    /** Find a source to activate... **/
	    ptr = NULL;
	    if (rpt_internal_GetString(inf, object, "source", &ptr, NULL, ac->Count) < 0)
		break;
	    if (ac->Count >= EXPR_MAX_PARAMS)
		{
		mssError(1,"RPT","Too many queries for object '%s'", object->Name);
		err = 1;
		break;
		}
	    ac->Names[ac->Count] = ptr;

	    /** Get inner/outer mode information **/
	    ac->InnerMode[ac->Count] = 0;
	    ac->OuterMode[ac->Count] = 0;
	    if (rpt_internal_GetString(inf, object, "mode", &ptr, NULL, ac->Count) >= 0)
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
		}
	    ac->Queries[ac->Count] = NULL;
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
rpt_internal_DoTableRow(pRptData inf, pStructInf tablerow, pRptSession rs, int numcols, int table_handle, double colsep)
    {
    int tablerow_handle, tablecell_handle, area_handle;
    int i;
    pStructInf subinf;
    int is_header, is_footer, flags;
    int is_cellrow, is_generalrow;
    int rval;
    pPrtBorder tb,bb,lb,rb,ib,ob;
    double dbl;
    int colspan;
    int context_pushed = 0;

	/** conditional rendering **/
	rval = rpt_internal_CheckCondition(inf,tablerow);
	if (rval < 0) 
	    return rval;
	if (rval == 0)
	    return 0;

	/** Get table row object flags **/
	is_header = rpt_internal_GetBool(inf, tablerow,"header",0,0);
	is_footer = rpt_internal_GetBool(inf, tablerow,"footer",0,0);
	flags = 0;
	if (rpt_internal_GetBool(inf, tablerow, "allowbreak", 1, 0)) flags |= PRT_OBJ_U_ALLOWBREAK;
	if (rpt_internal_GetBool(inf, tablerow, "fixedsize", 0, 0)) flags |= PRT_OBJ_U_FIXEDSIZE;

	/** Border settings? **/
	tb=bb=lb=rb=ib=ob=NULL;
	if (rpt_internal_GetDouble(inf, tablerow, "outerborder", &dbl, NAN, 0) >= 0)
	    ob = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, tablerow, "innerborder", &dbl, NAN, 0) >= 0)
	    ib = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, tablerow, "topborder", &dbl, NAN, 0) >= 0)
	    tb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, tablerow, "bottomborder", &dbl, NAN, 0) >= 0)
	    bb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, tablerow, "leftborder", &dbl, NAN, 0) >= 0)
	    lb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, tablerow, "rightborder", &dbl, NAN, 0) >= 0)
	    rb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);

	/** Create the table row **/
	tablerow_handle = prtAddObject(table_handle, PRT_OBJ_T_TABLEROW, -1,-1,-1,-1, flags, "header", is_header, "footer", is_footer, 
		"outerborder", ob, "innerborder", ib, "topborder", tb, "bottomborder", bb, "leftborder", lb, "rightborder", rb, NULL);
	if (ob) prtFreeBorder(ob);
	if (ib) prtFreeBorder(ib);
	if (tb) prtFreeBorder(tb);
	if (bb) prtFreeBorder(bb);
	if (rb) prtFreeBorder(rb);
	if (lb) prtFreeBorder(lb);
	if (tablerow_handle < 0)
	    {
	    mssError(0,"RPT","could not build table row object '%s'", tablerow->Name);
	    goto error;
	    }

	/** Set the style for the table **/
	if (rpt_internal_SetStyle(inf, tablerow, rs, tablerow_handle) < 0)
	    goto error;
	if (rpt_internal_SetMargins(inf, tablerow, tablerow_handle, colsep/2/prtGetUnitsRatio(rs->PSession), colsep/2/prtGetUnitsRatio(rs->PSession), 0, 0) < 0)
	    goto error;

	/** Output data formats **/
	cxssPushContext();
	context_pushed = 1;
	rpt_internal_CheckFormats(inf, tablerow);

	/** Loop through subobjects **/
	is_cellrow = is_generalrow = 0;
	for(i=0;i<tablerow->nSubInf;i++) /*if (stStructType(tablerow->SubInf[i]) == ST_T_SUBGROUP)*/
	    {
	    subinf = tablerow->SubInf[i];

	    /** Is this a cell object, or do we have a general-purpose row here? **/
	    if (stStructType(subinf) == ST_T_SUBGROUP && !strcmp(subinf->UsrType,"report/table-cell"))
		{
		/** Check conditional cell **/
		if (rpt_internal_CheckCondition(inf,subinf) != 1)
		    continue;

		if (is_generalrow)
		    {
		    mssError(1,"RPT","cannot mix a general purpose row (%s) contents with a cell (%s)", tablerow->Name, subinf->Name);
		    prtEndObject(tablerow_handle);
		    goto error;
		    }
		is_cellrow = 1;

		/** Init the cell **/
		colspan = 1;
		rpt_internal_GetInteger(inf, subinf, "colspan", &colspan, 1, 0);
		if (colspan <= 0) colspan=1;
		tablecell_handle = prtAddObject(tablerow_handle, PRT_OBJ_T_TABLECELL, -1,-1,-1,-1, flags, "colspan", colspan, NULL);
		if (tablecell_handle < 0)
		    {
		    mssError(0,"RPT","could not build table cell object '%s'", subinf->Name);
		    prtEndObject(tablerow_handle);
		    goto error;
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
			goto error;
			}
		    /*rpt_internal_SetStyle(inf, subinf, rs, area_handle);*/
		    rpt_internal_SetMargins(inf, subinf, area_handle, 0, 0, 0, 0);
		    if (rpt_internal_DoData(inf, subinf, rs, area_handle) < 0)
			{
			mssError(0,"RPT","problem constructing cell object '%s' (error doing area content)", subinf->Name);
			prtEndObject(area_handle);
			prtEndObject(tablecell_handle);
			prtEndObject(tablerow_handle);
			goto error;
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
			goto error;
			}
		    }

		/** End the cell **/
		prtEndObject(tablecell_handle);
		}
	    else if (stStructType(subinf) == ST_T_SUBGROUP || !strcmp(subinf->Name,"value"))
		{
		if (is_cellrow)
		    {
		    mssError(1,"RPT","cannot mix a cell row (%s) contents with a general purpose object (%s)", tablerow->Name, subinf->Name);
		    prtEndObject(tablerow_handle);
		    goto error;
		    }
		is_generalrow = 1;

		if (stLookup(tablerow, "value"))
		    {
		    /** Handle Table row as a monolithic container with a value= in it **/
		    area_handle = prtAddObject(tablerow_handle, PRT_OBJ_T_AREA, 0,0,-1,-1, flags, NULL);
		    if (area_handle < 0)
			{
			mssError(0,"RPT","problem constructing row object '%s' (error adding area object)", tablerow->Name);
			prtEndObject(tablerow_handle);
			goto error;
			}
		    /*rpt_internal_SetStyle(inf, subinf, rs, area_handle);*/
		    rpt_internal_SetMargins(inf, tablerow, area_handle, 0, 0, 0, 0);
		    if (rpt_internal_DoData(inf, tablerow, rs, area_handle) < 0)
			{
			mssError(0,"RPT","problem constructing row object '%s' (error doing area content)", tablerow->Name);
			prtEndObject(area_handle);
			prtEndObject(tablerow_handle);
			goto error;
			}
		    prtEndObject(area_handle);
		    }
		else
		    {
		    /** Handle table row as a monolithic container with abstract content in it **/
		    if (rpt_internal_DoContainer(inf, tablerow, rs, tablerow_handle) < 0)
			{
			mssError(0,"RPT","problem constructing row object '%s' (error doing content)", tablerow->Name);
			prtEndObject(tablerow_handle);
			goto error;
			}
		    }
		break; /* end the subinf for() loop, since we did all objects in DoContainer or DoArea */
		}
	    }

	/** Finish the row **/
	prtEndObject(tablerow_handle);
	cxssPopContext();
	context_pushed = 0;

	return 0;

    error:
	if (context_pushed)
	    cxssPopContext();
	return -1;
    }


/*** rpt_internal_DoTable - process tabular data from the database and
 *** output using the print driver's table feature.
 ***/
int
rpt_internal_DoTable(pRptData inf, pStructInf table, pRptSession rs, int container_handle)
    {
    int i,j;
    int n;
    double dbl;
    int err = 0;
    int reclimit = -1;
    double colsep = 1.0;
    pRptActiveQueries ac = NULL;
    int reccnt;
    int rval;
    int no_data_msg = 1;
    int has_source = 0;
    double cwidths[64];
    int table_handle, tablerow_handle, area_handle;
    int flags;
    double x,y,w,h;
    int numcols;
    pPrtBorder outerborder=NULL, innerborder=NULL, shadow=NULL;
    pPrtBorder tb=NULL, bb=NULL, lb=NULL, rb=NULL;
    pStructInf rowinf, summarize_for_inf, cellinf;
    pRptUserData ud;
    int changed;
    int colspan;
    double cellwidth;
    int context_pushed = 0;
    int is_autowidth = 0;
	
	/** conditional rendering **/
	rval = rpt_internal_CheckCondition(inf,table);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get table outer geometry **/
	rpt_internal_GetDouble(inf, table, "x", &x, -1.0, 0);
	rpt_internal_GetDouble(inf, table, "y", &y, -1.0, 0);
	rpt_internal_GetDouble(inf, table, "width", &w, -1.0, 0);
	rpt_internal_GetDouble(inf, table, "height", &h, -1.0, 0);

	/** Check for flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;
	if (rpt_internal_GetBool(inf, table, "allowbreak", 1, 0)) flags |= PRT_OBJ_U_ALLOWBREAK;
	if (rpt_internal_GetBool(inf, table, "fixedsize", 0, 0)) flags |= PRT_OBJ_U_FIXEDSIZE;
	is_autowidth = rpt_internal_GetBool(inf, table, "autowidth", 0, 0);

	/** Get column count and widths. **/
	numcols = 0;
	while(numcols < 64 && rpt_internal_GetDouble(inf, table, "widths", &(cwidths[numcols]), NAN, numcols) >= 0) 
	    numcols++;
	rpt_internal_GetInteger(inf, table, "columns", &n, 0, 0);
	if (n != numcols)
	    {
	    mssError(1,"RPT","Table '%s' column count %d not specified or does not match number of column widths (%d).", table->Name, n, numcols);
	    goto error;
	    }

	/** If not explicitly supplied, look inside a header row **/
	if (numcols == 0)
	    {
	    /** scan for any header rows in the table **/
	    for(i=0;i<table->nSubInf;i++) if (stStructType(table->SubInf[i]) == ST_T_SUBGROUP)
		{
		rowinf = table->SubInf[i];
		if (!strcmp(rowinf->UsrType,"report/table-row"))
		    {
		    if (rpt_internal_GetBool(inf, rowinf,"header",0,0))
			{
			if (rpt_internal_CheckCondition(inf,rowinf) == 1)
			    {
			    /** valid header -- look for width data in the cells **/
			    for(j=0;j<rowinf->nSubInf;j++) if (stStructType(rowinf->SubInf[j]) == ST_T_SUBGROUP)
				{
				cellinf = rowinf->SubInf[j];
				if (rpt_internal_CheckCondition(inf,cellinf) == 1)
				    {
				    rpt_internal_GetInteger(inf, cellinf, "colspan", &colspan, 1, 0);
				    if (colspan != 1)
					{
					/** unsuitable header row to get widths -- cell spans more than one column **/
					numcols = 0;
					break;
					}
				    if (rpt_internal_GetDouble(inf, cellinf, "width", &cellwidth, NAN, 0) < 0)
					{
					/** unsuitable header row for widths - width not specified on a valid header cell **/
					numcols = 0;
					break;
					}
				    cwidths[numcols] = cellwidth;
				    numcols++;
				    if (numcols == 64) break;
				    }
				}
			    if (numcols > 0) break;
			    }
			}
		    }
		}
	    }

	/** No column/width information? **/
	if (numcols == 0)
	    {
	    mssError(1,"RPT","Table '%s' must have valid width info specifying at least one column.", table->Name);
	    goto error;
	    }

	/** Check for column separation amount **/
	rpt_internal_GetDouble(inf, table, "colsep", &colsep, 1.0, 0);

	/** Check for borders / shadow **/
	outerborder = innerborder = shadow = tb = bb = lb = rb = NULL;
	if (rpt_internal_GetDouble(inf, table, "outerborder", &dbl, NAN, 0) >= 0 && dbl > RPT_FP_FUDGE)
	    outerborder = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, table, "innerborder", &dbl, NAN, 0) >= 0 && dbl > RPT_FP_FUDGE)
	    innerborder = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, table, "shadow", &dbl, NAN, 0) >= 0 && dbl > RPT_FP_FUDGE)
	    shadow = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, table, "topborder", &dbl, NAN, 0) >= 0 && dbl > RPT_FP_FUDGE)
	    tb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, table, "bottomborder", &dbl, NAN, 0) >= 0 && dbl > RPT_FP_FUDGE)
	    bb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, table, "leftborder", &dbl, NAN, 0) >= 0 && dbl > RPT_FP_FUDGE)
	    lb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);
	if (rpt_internal_GetDouble(inf, table, "rightborder", &dbl, NAN, 0) >= 0 && dbl > RPT_FP_FUDGE)
	    rb = prtAllocBorder(1,0.0,0.0, dbl,0x000000);

	/** Create the table **/
	table_handle = prtAddObject(container_handle, PRT_OBJ_T_TABLE, x,y,w,h, flags,
		"numcols", numcols, "colwidths", cwidths, "colsep", colsep, "outerborder", outerborder,
		"innerborder", innerborder, "shadow", shadow, "topborder", tb, "bottomborder", bb,
		"leftborder", lb, "rightborder", rb, "autowidth", is_autowidth, NULL);
	if (outerborder) prtFreeBorder(outerborder);
	if (innerborder) prtFreeBorder(innerborder);
	if (shadow) prtFreeBorder(shadow);
	if (tb) prtFreeBorder(tb);
	if (bb) prtFreeBorder(bb);
	if (rb) prtFreeBorder(rb);
	if (lb) prtFreeBorder(lb);
	if (table_handle < 0)
	    {
	    mssError(0, "RPT", "Could not create table '%s'", table->Name);
	    goto error;
	    }

	/** Output data formats **/
	cxssPushContext();
	context_pushed = 1;
	rpt_internal_CheckFormats(inf, table);

	/** Record-count limiter? **/
	rpt_internal_GetInteger(inf, table, "reclimit", &reclimit, -1, 0);

	/** Set the style for the table **/
	if (rpt_internal_SetStyle(inf, table, rs, table_handle) < 0)
	    goto error;
	if (rpt_internal_SetMargins(inf, table, table_handle, 0, 0, 0, 0) < 0)
	    goto error;

	/** Suppress "no data returned" message on 0 rows? **/
	no_data_msg = rpt_internal_GetBool(inf, table, "nodatamsg", 1, 0);

	/** Check to see if table has sources specified. **/
	if (stLookup(table,"source")) has_source = 1;

	/** Start query if table has source(s), otherwise skip that. **/
	if (has_source)
	    {
	    /** Start the query. **/
	    if ((ac = rpt_internal_Activate(inf, table, rs)) == NULL)
	        {
		prtEndObject(table_handle);
		goto error;
	        }
    
	    /** Fetch the first row. **/
	    if ((rval = rpt_internal_NextRecord(ac, inf, table, rs, 1)) < 0)
	        {
		prtEndObject(table_handle);
		goto error;
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
		err = 0;
		goto out;
	        }
	    }
	else
	    {
	    ac = NULL;
	    }

	/** First, scan for any header rows in the table **/
	for(i=0;i<table->nSubInf;i++) if (stStructType(table->SubInf[i]) == ST_T_SUBGROUP)
	    {
	    rowinf = table->SubInf[i];
	    if (!strcmp(rowinf->UsrType,"report/table-row"))
		{
		if (rpt_internal_GetBool(inf, rowinf,"header",0,0))
		    {
		    if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle, colsep) < 0)
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
	    if (reclimit != -1 && reccnt >= reclimit) break;
	    if (ac && rpt_internal_UseRecord(ac) < 0)
	        {
		err = 1;
		break;
		}

	    /** If first record, prime any summary LastValue **/
	    if (reccnt == 0)
		{
		for(i=0;i<table->nSubInf;i++) if (stStructType(table->SubInf[i]) == ST_T_SUBGROUP)
		    {
		    rowinf = table->SubInf[i];
		    if (!strcmp(rowinf->UsrType,"report/table-row"))
			{
			if (rpt_internal_GetBool(inf, rowinf,"summary",0,0))
			    {
			    summarize_for_inf = stLookup(rowinf, "summarize_for");
			    if (summarize_for_inf)
				{
				ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (intptr_t)(summarize_for_inf->UserData));
				if (ud->LastValue)
				    {
				    expFreeExpression(ud->LastValue);
				    }
				/** First record; not initialized. **/
				if (expEvalTree(ud->Exp, inf->ObjList) < 0)
				    {
				    mssError(0,"RPT","Could not evaluate report/table-row '%s' summarize_for expression", rowinf->Name);
				    err = 1;
				    break;
				    }
				ud->LastValue = expAllocExpression();
				ud->LastValue->NodeType = expDataTypeToNodeType(ud->Exp->DataType);
				expCopyValue(ud->Exp, ud->LastValue, 1);
				}
			    }
			}
		    }
		}
            
	    /** Search for 'normal' rows **/
	    for(i=0;i<table->nSubInf;i++) if (stStructType(table->SubInf[i]) == ST_T_SUBGROUP)
		{
		rowinf = table->SubInf[i];
		if (!strcmp(rowinf->UsrType,"report/table-row"))
		    {
		    if (!rpt_internal_GetBool(inf, rowinf,"header",0,0) && !rpt_internal_GetBool(inf, rowinf,"summary",0,0))
			{
			if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle, colsep) < 0)
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
		    if (rpt_internal_GetBool(inf, rowinf,"summary",0,0))
			{
			if (rval != 0) 
			    {
			    /** End of table.  do the final summary. **/
			    if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle, colsep) < 0)
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
				ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (intptr_t)(summarize_for_inf->UserData));
				if (expEvalTree(ud->Exp, inf->ObjList) < 0)
				    {
				    mssError(0,"RPT","Could not evaluate report/table-row '%s' summarize_for expression", rowinf->Name);
				    err = 1;
				    break;
				    }
				if (ud->LastValue->DataType != ud->Exp->DataType)
				    {
				    /** Data type changed.  Issue summary. **/
				    if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle, colsep) < 0)
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
				    if ((ud->Exp->Flags & EXPR_F_NULL) != (ud->LastValue->Flags & EXPR_F_NULL))
					{
					/** Changed to/from NULL **/
					changed = 1;
					}
				    else if ((ud->Exp->Flags & EXPR_F_NULL) && (ud->LastValue->Flags & EXPR_F_NULL))
					{
					/** Both are null - do not compare **/
					changed = 0;
					}
				    else
					{
					/** Compare values **/
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
					}
				    if (changed)
					{
					/** Value changed.  Issue summary. **/
					if (rpt_internal_DoTableRow(inf, rowinf, rs, numcols, table_handle, colsep) < 0)
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

    out:
	/** Finish off the table object **/
	prtEndObject(table_handle);
	if (context_pushed)
	    cxssPopContext();
	context_pushed = 0;

	/** Remove the paramobject item for this query's result set **/
	if (ac) rpt_internal_Deactivate(inf, ac);

	/** If error, indicate such **/
	if (err)
	    goto error;

	return 0;

    error:
	if (context_pushed)
	    cxssPopContext();
	return -1;
    }


/*** rpt_internal_WriteExpResult - writes the result of an expression to the
 *** output.  Used by DoData.
 ***/
int
rpt_internal_WriteExpResult(pRptSession rs, pExpression exp, int container_handle, char* attrname, char* typename)
    {
    pXString str_data;
    ObjData od1, od2;
    pRptData inf = (pRptData)rs->Inf;

	/** Check the evaluated result value and output it in the report. **/
	str_data = xsNew();
	xsCopy(str_data,"",-1);
	if (!(exp->Flags & EXPR_F_NULL))
	    {
	    if (inf->ObfuscationSess)
		{
		expExpressionToPod(exp, exp->DataType, &od1);
		obfObfuscateDataSess(inf->ObfuscationSess, &od1, &od2, exp->DataType, attrname, NULL, typename);
		}
	    else
		{
		expExpressionToPod(exp, exp->DataType, &od2);
		}
	    switch(exp->DataType)
	        {
                case DATA_T_INTEGER:
                    objDataToString(str_data, exp->DataType, &(od2.Integer), 0);
                    break;
                case DATA_T_DOUBLE:
                    objDataToString(str_data, exp->DataType, &(od2.Double), 0);
                    break;
                case DATA_T_STRING:
                    objDataToString(str_data, exp->DataType, od2.String, 0);
                    break;
                case DATA_T_MONEY:
                    objDataToString(str_data, exp->DataType, od2.Money, 0);
                    break;
                case DATA_T_DATETIME:
                    objDataToString(str_data, exp->DataType, od2.DateTime, 0);
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
	xsFree(str_data);

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
	str_data = xsNew();
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
	xsFree(str_data);

    return 0;
    }


/*** rpt_internal_DoData - process an expression-based data element within
 *** the report system.  This element should eventually replace the comment
 *** and column entities, but not necessarily.
 ***/
int
rpt_internal_DoData(pRptData inf, pStructInf data, pRptSession rs, int container_handle)
    {
    int nl = 0;
    PrtTextStyle oldstyle;
    pPrtTextStyle oldstyleptr = &oldstyle;
    int t,rval;
    ObjData od;
    pStructInf value_inf;
    pRptUserData ud;
    int context_pushed = 0;
    int negative_fontcolor = -1;
    char* ptr;
    char* url = NULL;

	/** Conditional rendering **/
	rval = rpt_internal_CheckCondition(inf,data);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get style information **/
	prtGetTextStyle(container_handle, &oldstyleptr);
	cxssPushContext();
	context_pushed = 1;
	rpt_internal_CheckFormats(inf, data);
	rpt_internal_CheckGoto(rs,data,container_handle);
	if (rpt_internal_SetStyle(inf, data, rs, container_handle) < 0)
	    goto error;

	/** URL **/
	rpt_internal_GetString(inf, data, "url", &url, NULL, 0);
	if (url)
	    prtSetURL(container_handle, url);

	/** Negative font color? **/
	cxssGetVariable("rpt:neg_fc", &ptr, "");
	if (ptr[0] == '#')
	    negative_fontcolor = strtoi(ptr+1, NULL, 16);

	/** Need to enable auto-newline? **/
	nl = rpt_internal_GetBool(inf, data, "autonewline", 0, 0);
        
	/** Get the expression itself **/
	value_inf = stLookup(data,"value");
	if (!value_inf)
	    {
	    mssError(0,"RPT","%s '%s' must have a value expression", data->UsrType, data->Name);
	    goto error;
	    }
	t = stGetAttrType(value_inf, 0);
	if (t == DATA_T_CODE)
	    {
	    ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (intptr_t)(value_inf->UserData));
	    if (ud)
		{
		/** Evaluate the expression **/
		/*if (!strcmp(data->Name,"d2_1_start")) CxGlobals.Flags |= CX_F_DEBUG;*/
		rval = expEvalTree(ud->Exp, inf->ObjList);
		/*if (!strcmp(data->Name,"d2_1_start")) expDumpExpression(ud->Exp);
		CxGlobals.Flags &= ~CX_F_DEBUG;*/
		if (rval < 0)
		    {
		    mssError(0,"RPT","Could not evaluate %s '%s' value expression", data->UsrType, data->Name);
		    goto error;
		    }

		/** Use negative fontcolor? **/
		t = ud->Exp->DataType;
		if (negative_fontcolor >= 0 && ((t == DATA_T_INTEGER && ud->Exp->Integer < 0) || (t == DATA_T_MONEY && ud->Exp->Types.Money.WholePart < 0) || (t == DATA_T_DOUBLE && ud->Exp->Types.Double < 0)))
		    {
		    if (prtSetColor(container_handle, negative_fontcolor) < 0)
			goto error;
		    }

		/** Output the result **/
		prtSetDataHints(container_handle, t, 0);
		rpt_internal_WriteExpResult(rs, ud->Exp, container_handle, data->Name, NULL);
		}
	    else
		{
		rpt_internal_WritePOD(rs, t, NULL, container_handle);
		}
	    }
	else if (t == DATA_T_STRING || t == DATA_T_DOUBLE || t == DATA_T_INTEGER || t == DATA_T_MONEY)
	    {
	    /** Use negative fontcolor? **/
	    if (negative_fontcolor >= 0 && ((t == DATA_T_INTEGER && od.Integer < 0) || (t == DATA_T_MONEY && od.Money->WholePart < 0) || (t == DATA_T_DOUBLE && od.Double < 0)))
		{
		if (prtSetColor(container_handle, negative_fontcolor) < 0)
		    goto error;
		}

	    rval = stGetAttrValue(stLookup(data,"value"), t, POD(&od), 0);
	    if (rval == 0) rpt_internal_WritePOD(rs, t, &od, container_handle);
	    else if (rval == 1) rpt_internal_WritePOD(rs, t, NULL, container_handle);
	    prtSetDataHints(container_handle, t, 0);
	    }
	else
	    {
	    mssError(1,"RPT","Unknown data value for %s '%s' element", data->UsrType, data->Name);
	    goto error;
	    }

	/** Emit a newline? **/
	if (nl) prtWriteNL(container_handle);

	/** Put the fonts etc back **/
	prtSetTextStyle(container_handle, &oldstyle);
	cxssPopContext();

	return 0;

    error:
	prtSetTextStyle(container_handle, &oldstyle);
	if (url)
	    prtSetURL(container_handle, NULL);
	if (context_pushed)
	    cxssPopContext();
	return -1;
    }


/*** rpt_internal_DoForm - creates a free-form style report element, 
 *** which can contain fields, comments, tables, and other forms.
 ***/
int
rpt_internal_DoForm(pRptData inf, pStructInf form, pRptSession rs, int container_handle)
    {
    pRptSource qy;
    int ffsep=0;
    char* ptr;
    int err=0;
    /*int relylimit = -1;*/
    int reclimit = -1;
    int outer_mode = 0;
    pRptActiveQueries ac;
    PrtTextStyle oldstyle;
    pPrtTextStyle oldstyleptr = &oldstyle;
    int reccnt;
    int rval;
    int n;
    int context_pushed = 0;
    int last_inner_cnt = -1;

	/** Conditional check - do we print this form? **/
	rval = rpt_internal_CheckCondition(inf,form);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Issue horizontal rule between records? **/
	/** This feature currently isn't available **/
	/*if (stAttrValue(stLookup(form,"rulesep"),NULL,&ptr,0) >= 0 && ptr && !strcmp(ptr,"yes"))
	    {
	    rulesep=1;
	    }*/

	/** Issue form feed (page break) between records? **/
	ffsep = rpt_internal_GetBool(inf, form, "ffsep", 0, 0);

	/** Check for set-page-number? **/
	if (rpt_internal_GetInteger(inf, form, "page", &n, RPT_INT_UNSPEC, 0) >= 0)
	    prtSetPageNumber(rs->PageHandle, n);

	/** Check for a mode entry **/
	n = 0;
	while (rpt_internal_GetString(inf, form, "mode", &ptr, NULL, n) >= 0)
	    {
	    if (!strcmp(ptr,"outer")) outer_mode = 1;
	    n++;
	    }

	/** Relative-Y record limiter? (maximum vertical space we can use) **/
	/** This feature currently isn't available **/
	/*if (stAttrValue(stLookup(form,"relylimit"),&relylimit,NULL,0) >= 0)
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
	    }*/

	/** Record-count limiter? **/
	if (rpt_internal_GetInteger(inf, form, "reclimit", &reclimit, RPT_INT_UNSPEC, 0) >= 0)
	    {
	    if (outer_mode)
	        {
		mssError(1,"RPT","reclimit can only be used on an non-outer form ('%s' is outer).", form->Name);
		goto error;
		}
	    }

	/** Get style information **/
	prtGetTextStyle(container_handle, &oldstyleptr);
	cxssPushContext();
	context_pushed = 1;
	rpt_internal_CheckFormats(inf, form);
	rpt_internal_CheckGoto(rs,form,container_handle);
	if (rpt_internal_SetStyle(inf, form, rs, container_handle) < 0)
	    goto error;

	/** Start the query. **/
	if ((ac = rpt_internal_Activate(inf, form, rs)) == NULL)
	    goto error;

	/** Fetch the first row. **/
	if ((rval = rpt_internal_NextRecord(ac, inf, form, rs, 1)) < 0)
	    goto error;

	/** Enter the row retrieval loop.  For each row, do all sub-parts **/
	reccnt = 0;
	/*if (rulesep) prtWriteLine(rs->PSession);*/
	qy = ac->Queries[ac->Count-1];
	while(rval == 0)
	    {
	    /** Record limit reached? **/
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
            
	    /** Print out the contents of the form **/
	    rval = rpt_internal_DoContainer(inf, form, rs, container_handle);
	    if (rval < 0) err = 1;

	    /** Outer/Inner mode check **/
	    if (outer_mode && qy->InnerExecCnt == 0)
	        {
		mssError(err?0:1,"RPT","No inner-mode form/table was run for outer-mode '%s'",form->Name);
		err = 1;
		}
	    if (outer_mode && last_inner_cnt == qy->InnerExecCnt)
		{
		/** No inner iterations; exit out now. **/
		break;
		}
	    last_inner_cnt = qy->InnerExecCnt;

	    /** Emit a page break if requested **/
	    if (ffsep) prtWriteFF(container_handle);

	    expModifyParam(inf->ObjList, "this", inf->Obj); /* page number changed, force re-eval */
	    /*if (rulesep) prtWriteLine(rs->PSession);*/
	    rval = rpt_internal_NextRecord(ac, inf, form, rs, 0);
	    reccnt++;
	    if (err) break;
	    }

	/** We're finished with the query **/
	rpt_internal_Deactivate(inf, ac);

	/** Set formatting/style information back to how it was before **/
	prtSetTextStyle(container_handle, &oldstyle);
	cxssPopContext();
	context_pushed = 0;

	/** Error? **/
	if (err)
	    goto error;

	return 0;

    error:
	if (context_pushed)
	    cxssPopContext();
	return -1;
    }


/*** rpt_internal_MglColor() takes a color hex value and finds the nearest MathGL
 *** color that matches it.
 ***/
char*
rpt_internal_MglColor(int hexcolor)
    {
    static char mglcolor[8];
    int i;
    int found = -1;
    int found_dist = 1024;
    int dist;

	/** Search the hex list **/
	for(i=0; i<sizeof(rpt_mgl_color_hex)/sizeof(int); i++)
	    {
	    dist = abs((hexcolor & 0xff0000) - (rpt_mgl_color_hex[i] & 0xff0000)) >> 16;
	    dist += (abs((hexcolor & 0x00ff00) - (rpt_mgl_color_hex[i] & 0x00ff00)) >> 8);
	    dist += abs((hexcolor & 0x0000ff) - (rpt_mgl_color_hex[i] & 0x0000ff));
	    if (dist < found_dist)
		{
		found_dist = dist;
		found = i;
		}
	    }
	
	/** Build the color spec **/
	mglcolor[0] = rpt_mgl_colors[found];
	mglcolor[1] = '\0';

    return mglcolor;
    }


/*** rpt_internal_GraphToImage - convert a MathGL graph to a PrtImage.
 ***/
pPrtImage
rpt_internal_GraphToImage(pRptChartContext ctx)
    {
    pPrtImage img = NULL;
    unsigned char* rawdata = NULL;
    int xstart, ystart;
    int x, y;
    int src, dst;

	/** Get the raw raster data from MathGL **/
	rawdata = (unsigned char*)mgl_get_rgba(ctx->gr);
	if (!rawdata)
	    {
	    mssError(1, "RPT", "Could not convert chart to raster data");
	    goto error;
	    }

	/** Create the image **/
	img = prtAllocImage(ctx->x_pixels, ctx->y_pixels, PRT_COLOR_T_FULL);
	if (!img)
	    goto error;

	/** Copy the image data **/
	xstart = round(ctx->rend_x_pixels * ctx->trim.left);
	ystart = round(ctx->rend_y_pixels * ctx->trim.top);
	for(y=0; y<ctx->y_pixels; y++)
	    {
	    for(x=0; x<ctx->x_pixels; x++)
		{
		src = (x + xstart + (y + ystart) * ctx->rend_x_pixels) * 4;
		dst = (x + y * ctx->x_pixels) * 4;
		img->Data.Byte[dst] = rawdata[src+2];
		img->Data.Byte[dst+1] = rawdata[src+1];
		img->Data.Byte[dst+2] = rawdata[src];
		img->Data.Byte[dst+3] = rawdata[src+3];
		}
	    }

	return img;

    error:
	if (img)
	    prtFreeImage(img);
	if (rawdata)
	    free(rawdata);
	return NULL;
    }


/*** rpt_internal_FreeChartValues() - free memory for a chart tuple
 ***/
int
rpt_internal_FreeChartValues(pRptChartValues value)
    {
	if (value->Label)
	    nmSysFree(value->Label);
	if (value->Values)
	    nmSysFree(value->Values);
	if (value->Types)
	    nmSysFree(value->Types);
	nmFree(value, sizeof(RptChartValues));

    return 0;
    }


/*** rpt_internal_NewChartValues() - allocate a tuple for a chart
 ***/
pRptChartValues
rpt_internal_NewChartValues(char* label, int n_values)
    {
    pRptChartValues value;

	value = (pRptChartValues)nmMalloc(sizeof(RptChartValues));
	if (!value)
	    goto error;
	value->Values = NULL;
	value->Types = NULL;
	value->Label = nmSysStrdup(label);
	if (!value->Label)
	    goto error;
	value->Values = (double*)nmSysMalloc(sizeof(double) * n_values);
	if (!value->Values)
	    goto error;
	memset(value->Values, 0, sizeof(double) * n_values);
	value->Types = (char*)nmSysMalloc(sizeof(char) * n_values);
	if (!value->Types)
	    goto error;

	return value;

    error:
	if (value)
	    rpt_internal_FreeChartValues(value);
	return NULL;
    }


/*** Returns the tick distance that is 1x10^n, 2x10^n, or 5x10^n.
 *** The number of tick marks is between 4 and 10
 ***/
int
rpt_internal_GetTickDist(double maxBar)
    {
    int tickDist;

	tickDist = pow(10,floor(log10(maxBar)));
	if(maxBar/tickDist < 5)
	    {
	    tickDist /= 2;
	    
	    }
	if(tickDist == 0) tickDist = 1;

    return tickDist;
    }


/*** Get an Mgl color based on report config
 ***/
char*
rpt_internal_GetMglColor(pRptChartContext ctx, pStructInf obj, char* attrname, char* defval, int nval)
    {
    int colorcode;
    char* color = NULL;

	rpt_internal_GetString(ctx->inf, obj, attrname, &color, NULL, nval);
	if (color && (colorcode = prtLookupColor(color)) != -1)
	    color = rpt_internal_MglColor(colorcode);
	else
	    color = defval;

    return color;
    }


/*** Get decimal precision of Y values.  n and ser indicate which point and
 *** series to work with; if they are set to -1 then we return the greatest
 *** decimal precision of the matching values.  Maximum precision returned
 *** is 8.
 ***/
int
rpt_internal_GetYDecimalPrecision(pRptChartContext ctx, int n, int ser)
    {
    int i,j,k;
    int nstart, nlen;
    int serstart, serlen;
    double val;
    int prec = 0;

	/** Evaluation range **/
	if (n == -1)
	    {
	    nstart = 0;
	    nlen = ctx->values->nItems;
	    }
	else
	    {
	    nstart = n;
	    nlen = 1;
	    }
	if (ser == -1)
	    {
	    serstart = 0;
	    serlen = ctx->series->nItems;
	    }
	else
	    {
	    serstart = ser;
	    serlen = 1;
	    }

	/** Iterate **/
	for(i=nstart; i<nstart+nlen; i++)
	    {
	    for(j=serstart; j<serstart+serlen; j++)
		{
		val = ((pRptChartValues)ctx->values->Items[i])->Values[j];
		for(k=0; k<8; k++)
		    {
		    if (val == 0 || ceil(abs(val)*exp10(k)*0.99999999999999) != ceil(abs(val)*exp10(k)*1.00000000000001))
			break;
		    }
		if (k > prec)
		    prec = k;
		}
	    }

    return prec;
    }


/*** Generate value strings
 ***/
pXArray
rpt_internal_GetValueStrings(pRptChartContext ctx, int startval, int n_vals, int show_pct, int ser)
    {
    pXArray labels;
    char str[32];
    int i;
    double val;
    int prec;

	labels = xaNew(n_vals);
	if (!labels)
	    return NULL;

	/** Format the label strings **/
	for(i=startval; i<startval+n_vals; i++)
	    {
	    val = ((pRptChartValues)ctx->values->Items[i])->Values[ser];
	    prec = rpt_internal_GetYDecimalPrecision(ctx, i, ser);

	    if (prec == 0)
		{
		/** Integer **/
		snprintf(str, sizeof(str), "%d%s", (int)round(val), show_pct?"%":"");
		}
	    else
		{
		/** Double **/
		snprintf(str, sizeof(str), "%.*f%s", prec, val, show_pct?"%":"");
		}
	    xaAddItem(labels, nmSysStrdup(str));
	    }

    return labels;
    }


/*** Free value strings
 ***/
int
rpt_internal_FreeValueStrings(pXArray labels)
    {
    int i;

	for(i=0; i<labels->nItems; i++)
	    if (labels->Items[i])
		nmSysFree(labels->Items[i]);
	xaFree(labels);

    return 0;
    }


/*** Line/Bar Labels
 ***/
int
rpt_internal_DrawValueLabels(pRptChartContext ctx, int startval, int n_vals, int total_n_vals, int ser, int n_ser, int bar, double fontsize, int show_pct, double offset)
    {
    int i;
    double val, valoffset;
    pXArray labels;
    int maxstrlen;
    double fs;

	labels = rpt_internal_GetValueStrings(ctx, startval, n_vals, show_pct, ser);
	if (!labels)
	    return -1;

	/** Font size - scale for longer labels / smaller bars **/
	maxstrlen = 0;
	for(i=0; i<n_vals; i++)
	    {
	    if (maxstrlen < strlen(labels->Items[i]))
		maxstrlen = strlen(labels->Items[i]);
	    }
	fs = ctx->stand_w * ctx->rend_x_pixels / ctx->x_pixels * 6.8 / (maxstrlen * total_n_vals * (bar?n_ser:1));
	if (fs > fontsize) fs = fontsize;
	if (fs < 2) fs = 2;

	/** Add the labels **/
	for(i=startval; i<startval+n_vals; i++)
	    {
	    val = ((pRptChartValues)ctx->values->Items[i])->Values[ser];
	    valoffset = (ctx->max - ctx->min)*0.02;
	    if (val < 0)
		valoffset = 0 - valoffset - (fs * ctx->font_scale_factor) * (ctx->max - ctx->min) * 0.013;
#ifdef HAVE_MGL2
	    mgl_puts(ctx->gr, offset + i*2 + (bar?(-0.7 + (0.5 + ser) * (1.4 / n_ser)):0.0), val + valoffset, 0.0, (char*)labels->Items[i - startval], "", fs * ctx->font_scale_factor);
#else
	    mgl_puts_ext(ctx->gr, offset + i*2 + (bar?(-0.7 + (0.5 + ser) * (1.4 / n_ser)):0.0), val + valoffset, 0.0, (char*)labels->Items[i - startval], "", fs * ctx->font_scale_factor, '\0');
#endif
	    }

	rpt_internal_FreeValueStrings(labels);

    return 0;
    }


/*** Draws X tick labels
 ***/
int
rpt_internal_DrawXTickLabels(pRptChartContext ctx, int startval, int n_vals, int total_n_vals, int offset)
    {
    int i;
    float b[n_vals];
#ifdef HAVE_MGL2
    char blank[64];
    int n_labels;
#else
    char* blank[n_vals];
#endif
    int axis_fontsize;
    double fs, cfs;
    int maxstrlen = 0;
    pXString labels;

	rpt_internal_GetInteger(ctx->inf, ctx->x_axis, "fontsize", &axis_fontsize, ctx->fontsize, 0);
	fs = axis_fontsize;

	for(i=0; i<n_vals-1; i++)
	    {
	    if (maxstrlen < (strlen(ctx->x_labels[i+startval]) + strlen(ctx->x_labels[i+startval+1]))/2)
		maxstrlen = (strlen(ctx->x_labels[i+startval]) + strlen(ctx->x_labels[i+startval+1]))/2;
	    }
	if (n_vals == 1)
	    {
	    if (maxstrlen < strlen(ctx->x_labels[startval]))
		maxstrlen = strlen(ctx->x_labels[startval]);
	    }
	cfs = ctx->stand_w * ctx->rend_x_pixels / ctx->x_pixels * 12.0 / (maxstrlen * total_n_vals);

#ifdef HAVE_MGL2
	labels = xsNew();
	cfs = cfs * 0.8;
	n_labels = total_n_vals*2 - 1;
	if (n_labels > sizeof(blank))
	    n_labels = sizeof(blank);
	memset(blank, '\n', n_labels - 1);
	blank[n_labels - 1] = '\0';
	mgl_set_ticks_str(ctx->gr, 'x', blank, 0);
	if (ctx->rotation)
	    {
	    for(i=0; i<n_vals; i++)
		{
		xsConcatPrintf(labels, "\n%s\n", ctx->x_labels[i+startval]);
		}
	    xsConcatenate(labels, "\n", 1);
	    mgl_set_ticks_str(ctx->gr, 'x', xsString(labels), 0);
	    }
#endif

	if (cfs < 2)
	    cfs = 2;
	else if (cfs > fs)
	    cfs = fs;

	for(i=0; i<n_vals; i++)
	    {
	    b[i] = (i + startval) * 2.0;
	    //mgl_puts_dir(ctx->gr, b[i], 0.0, 0.0, b[i] + 1, 0.0, 0.0, ctx->x_labels[i], 8);
#ifdef HAVE_MGL2
	    if (!ctx->rotation)
		{
		mgl_puts(ctx->gr, b[i] + offset, ctx->minaxis - (ctx->max - ctx->min)*0.015 * cfs * ctx->font_scale_factor, 0.0, ctx->x_labels[i+startval], "", cfs * ctx->font_scale_factor);
		}
#else
	    mgl_puts_ext(ctx->gr, b[i] + offset, ctx->minaxis - ((ctx->max - ctx->min)*0.015 * cfs * ctx->font_scale_factor), 0.0, ctx->x_labels[i+startval], "", cfs * ctx->font_scale_factor, 'x');
	    blank[i] = "";
#endif
	    }

#ifdef HAVE_MGL2
	xsFree(labels);
#else
	mgl_set_ticks_vals(ctx->gr, 'x', n_vals, b, (const char**)blank);
#endif

    return 0;
    }


/*** Bar chart
 ***/
int
rpt_internal_BarChart_PreProcess(pRptChartContext ctx)
    {
    pRptChartValues value;

	/** Add blank one at beginning and end to make chart more readable **/
#ifndef HAVE_MGL2
	value = rpt_internal_NewChartValues(" ", ctx->series->nItems);
	if (!value)
	    return -1;
	xaAddItem(ctx->values, value);
	value = rpt_internal_NewChartValues(" ", ctx->series->nItems);
	if (!value)
	    return -1;
	xaInsertBefore(ctx->values, 0, value);
#endif

	/** Trim factors **/
	ctx->trim.left = 0.07;
	ctx->trim.right = 0.17;

    return 0;    
    }

int
rpt_internal_BarChart_Setup(pRptChartContext ctx)
    {

	//mgl_set_zoom(ctx->gr, -0.05, 0, 0.95, 1.0);

    return 0;
    }

int
rpt_internal_BarChart_Generate(pRptChartContext ctx)
    {
    int reccnt = ctx->values->nItems;
    pStructInf one_series;
    int series_fontsize;
    int axis_fontsize, xaxis_fontsize;
    double fs, xfs;
    char* color;
    int show_value;
    int show_percent;
    char* ptr;
    char barcolors[256] = "";
    int i;
    
	/** Draw the chart **/
	//mgl_set_alpha(gr, 1);
	//mgl_set_alpha_default(gr, 1.0);
#ifdef HAVE_MGL2
	//mgl_set_range_val(ctx->gr, 'x', 0, (reccnt-1)*2);
	mgl_set_range_val(ctx->gr, 'x', 0, reccnt*2);
	mgl_set_range_val(ctx->gr, 'y', ctx->minaxis, ctx->maxaxis);
#else
	mgl_set_axis_2d(ctx->gr, 0, ctx->minaxis, (reccnt-1)*2, ctx->maxaxis);
#endif
	//mgl_set_axis_3d(ctx->gr, 0, 0, 0.0, (reccnt-1)*2, ctx->max+tickDist, 0.0);
	mgl_set_origin(ctx->gr, 0.0, 0.0, NAN);
	//mgl_set_tick_origin(gr, 0.0, 0.0, 1.0);

	/** Handle each data series **/
	for(i=0; i<ctx->series->nItems; i++)
	    {
	    /** Bar color **/
	    one_series = (pStructInf)ctx->series->Items[i];
	    color = rpt_internal_GetMglColor(ctx, one_series, "color", ctx->color, 0);
	    if (strlen(barcolors) + strlen(color) >= sizeof(barcolors))
		break;
	    strcat(barcolors, color);

	    /** Value label **/
	    rpt_internal_GetInteger(ctx->inf, one_series, "fontsize", &series_fontsize, ctx->fontsize, 0);
	    rpt_internal_GetString(ctx->inf, one_series, "show_value", &ptr, "yes", 0);
	    show_value = (!strcmp(ptr, "no"))?0:1;
	    rpt_internal_GetString(ctx->inf, one_series, "show_percent", &ptr, "no", 0);
	    show_percent = (!strcmp(ptr, "no"))?0:1;

	    /** Generate the numeric bar labels **/
	    if (show_value || show_percent)
#ifdef HAVE_MGL2
		rpt_internal_DrawValueLabels(ctx, 0, reccnt, reccnt, i, ctx->series->nItems, 1, series_fontsize, show_percent, 1.0);
#else
		rpt_internal_DrawValueLabels(ctx, 1, reccnt-2, reccnt, i, ctx->series->nItems, 1, series_fontsize, show_percent, 0.0);
#endif
	    }

	/** Font size for y axis **/
	rpt_internal_GetInteger(ctx->inf, ctx->y_axis, "fontsize", &axis_fontsize, ctx->fontsize, 0);
	fs = ctx->stand_w * ctx->rend_x_pixels / ctx->x_pixels * 6.0 / 50.0;
	if (fs > axis_fontsize) fs = axis_fontsize;
	if (fs < 2) fs = 2;
	mgl_set_font_size(ctx->gr, fs * ctx->font_scale_factor);
#ifdef HAVE_MGL2
	rpt_internal_GetInteger(ctx->inf, ctx->x_axis, "fontsize", &xaxis_fontsize, ctx->fontsize, 0);
	xfs = ctx->stand_w * ctx->rend_x_pixels / ctx->x_pixels * 6.0 / 50.0;
	if (xfs > xaxis_fontsize) xfs = xaxis_fontsize;
	if (xfs < 2) xfs = 2;
#endif

#ifdef HAVE_MGL2
	rpt_internal_DrawXTickLabels(ctx, 0, reccnt, reccnt, 1);
#else
	rpt_internal_DrawXTickLabels(ctx, 1, reccnt-2, reccnt, 0);
#endif
	mgl_adjust_ticks(ctx->gr, "y");
	//mgl_set_ticks(ctx->gr, -((reccnt-1)*2+1), tickDist, 1);
	mgl_tune_ticks(ctx->gr, ctx->scale, 1.15);
#ifdef HAVE_MGL2
	mgl_set_ticks(ctx->gr, 'x', 1000.0, 0, 1000.0);
	mgl_set_flag(ctx->gr, 1, MGL_NO_ORIGIN);
	char opt[32];
	snprintf(opt, sizeof(opt), "size %.1f", xfs * ctx->font_scale_factor);
	mgl_axis(ctx->gr, "x", "", opt);
	snprintf(opt, sizeof(opt), "size %.1f", fs * ctx->font_scale_factor);
	mgl_axis(ctx->gr, "y", "", opt);
	mgl_axis_grid(ctx->gr, "y", "W-", "");
	mgl_bars(ctx->gr, ctx->chart_data, barcolors, "");
#else
	mgl_axis(ctx->gr, "xy");
	mgl_axis_grid(ctx->gr, "y", "W-");
	mgl_bars(ctx->gr, ctx->chart_data, barcolors);
#endif

    return 0;
    }


/*** Line chart
 ***/
int
rpt_internal_LineChart_PreProcess(pRptChartContext ctx)
    {

	/** Trim factors **/
	ctx->trim.left = 0.07;
	ctx->trim.right = 0.14;

    return 0;
    }

int
rpt_internal_LineChart_Setup(pRptChartContext ctx)
    {

	//mgl_set_zoom(ctx->gr, -0.05, 0, 0.95, 1.0);

    return 0;
    }

int
rpt_internal_LineChart_Generate(pRptChartContext ctx)
    {
    int reccnt = ctx->values->nItems;
    pStructInf one_series = (pStructInf)ctx->series->Items[0];
    int tickDist;
    int series_fontsize, axis_fontsize;
    double fs;
    char lineStyle[8];
    char linecolors[256] = "";
    int i;
    char* ptr;
    int show_value, show_percent;
    char* color;
    
	tickDist = rpt_internal_GetTickDist(ctx->max);
	rpt_internal_GetInteger(ctx->inf, one_series, "fontsize", &series_fontsize, ctx->fontsize, 0);
        
#ifdef HAVE_MGL2
	mgl_set_range_val(ctx->gr, 'x', 0, (reccnt-1)*2);
	mgl_set_range_val(ctx->gr, 'y', ctx->minaxis, ctx->maxaxis);
#else
	mgl_set_axis_2d(ctx->gr, 0, ctx->minaxis, (reccnt-1)*2, ctx->maxaxis);
#endif
	mgl_set_origin(ctx->gr, 0.0, 0.0, 0.0);
	//mgl_set_ticks(ctx->gr, -((reccnt-1)*2+1), tickDist, 1);

	/** Handle each data series **/
	for(i=0; i<ctx->series->nItems; i++)
	    {
	    /** Bar color **/
	    one_series = (pStructInf)ctx->series->Items[i];
	    color = rpt_internal_GetMglColor(ctx, one_series, "color", ctx->color, 0);
	    snprintf(lineStyle, sizeof(lineStyle), "%s-4#d", color);
	    if (strlen(linecolors) + strlen(lineStyle) >= sizeof(linecolors))
		break;
	    strcat(linecolors, lineStyle);

	    /** Value label **/
	    rpt_internal_GetInteger(ctx->inf, one_series, "fontsize", &series_fontsize, ctx->fontsize, 0);
	    rpt_internal_GetString(ctx->inf, one_series, "show_value", &ptr, "yes", 0);
	    show_value = (!strcmp(ptr, "no"))?0:1;
	    rpt_internal_GetString(ctx->inf, one_series, "show_percent", &ptr, "no", 0);
	    show_percent = (!strcmp(ptr, "no"))?0:1;

	    /** Generate the numeric bar labels **/
	    if (show_value || show_percent)
		rpt_internal_DrawValueLabels(ctx, 0, reccnt, reccnt, i, 1, 0, series_fontsize, show_percent, 0.0);
	    }
        
	/** Font size for y axis **/
	rpt_internal_GetInteger(ctx->inf, ctx->y_axis, "fontsize", &axis_fontsize, ctx->fontsize, 0);
	fs = ctx->stand_w * ctx->rend_x_pixels / ctx->x_pixels * 6.0 / 50.0;
	if (fs > axis_fontsize) fs = axis_fontsize;
	if (fs < 2) fs = 2;
	mgl_set_font_size(ctx->gr, fs * ctx->font_scale_factor);

	rpt_internal_DrawXTickLabels(ctx, 0, reccnt, reccnt, 0);
	mgl_adjust_ticks(ctx->gr, "y");
	mgl_tune_ticks(ctx->gr, ctx->scale, 1.15);
#ifdef HAVE_MGL2
	mgl_set_ticks(ctx->gr, 'x', 1000.0, 0, 1000.0);
	mgl_set_flag(ctx->gr, 1, MGL_NO_ORIGIN);
	mgl_axis(ctx->gr, "xy", "", "");
	mgl_axis_grid(ctx->gr, "y", "W-", "");
	mgl_plot(ctx->gr, ctx->chart_data, linecolors, "");
#else
	mgl_axis(ctx->gr, "xy");
	mgl_axis_grid(ctx->gr, "y", "W-");
	mgl_plot(ctx->gr, ctx->chart_data, linecolors);
#endif

    return 0;
    }


/*** Pie chart
 ***/
int
rpt_internal_PieChart_PreProcess(pRptChartContext ctx)
    {

	if (ctx->series->nItems > 1)
	    {
	    mssError(1, "RPT", "Pie charts can only have one series; %d provided", ctx->series->nItems);
	    return -1;
	    }

	if (ctx->values->nItems > 28)
	    {
	    mssError(1, "RPT", "Pie charts can show a maximum of 28 values; %d provided", ctx->values->nItems);
	    return -1; /* There are only 28 colors for the pie chart*/
	    }

	/** Trim factors **/
	ctx->trim.left = 0.0;
	ctx->trim.right = 0.1;

    return 0;
    }

int
rpt_internal_PieChart_Setup(pRptChartContext ctx)
    {

	//mgl_set_zoom(ctx->gr, -0.1, 0, 0.9, 1.0);

    return 0;
    }

int
rpt_internal_PieChart_Generate(pRptChartContext ctx)
    {
    int reccnt = ctx->values->nItems;
    pStructInf series = (pStructInf)ctx->series->Items[0];
    int i;
    double t,r=1.3;
    double sumValues=0.0;
    double sumPreviousAngles = 0.0;
    double angle;
    char string[256];
    char colorString[3];
    char* pcolor;
    double val;
    int axis_fontsize;
    int series_fontsize;
    char* show_percent;
    char* show_value;
    int pct;
    pXArray labels;
   
	/** Setup **/
	rpt_internal_GetInteger(ctx->inf, ctx->x_axis, "fontsize", &axis_fontsize, ctx->fontsize, 0);
	rpt_internal_GetInteger(ctx->inf, series, "fontsize", &series_fontsize, ctx->fontsize, 0);
	rpt_internal_GetString(ctx->inf, series, "show_percent", &show_percent, "pie", 0);
	rpt_internal_GetString(ctx->inf, series, "show_value", &show_value, "legend", 0);
#ifdef HAVE_MGL2
	mgl_set_func(ctx->gr, "(y+1)/2*cos(pi*x)", "(y+1)/2*sin(pi*x)", NULL, NULL);   //make it to a cylinder 
#else
	mgl_set_func(ctx->gr, "(y+1)/2*cos(pi*x)", "(y+1)/2*sin(pi*x)", 0);   //make it to a cylinder 
#endif
	mgl_tune_ticks(ctx->gr, ctx->scale, 1.15);
	pcolor = ":bgrhBGRHWcmywpCMYkPlenuqLENUQ"; 
	for(i=0; i<reccnt; i++)
	    sumValues += ((pRptChartValues)ctx->values->Items[i])->Values[0];
	labels = rpt_internal_GetValueStrings(ctx, 0, reccnt, 0, 0);
	if (!labels)
	    return -1;

	/** Generate the legend **/
	for(i=0;i<reccnt;i++)
	    {
	    val = ((pRptChartValues)ctx->values->Items[i])->Values[0];
	    pct = (int)(val/sumValues*100+0.5);
	    if (!strcmp(show_percent, "legend") && !strcmp(show_value, "legend"))
		snprintf(string, sizeof(string), "%s: %s (%d%%)", ctx->x_labels[i], (char*)labels->Items[i], pct);
	    else if (!strcmp(show_percent, "legend"))
		snprintf(string, sizeof(string), "%s: %d%%", ctx->x_labels[i], pct);
	    else if (!strcmp(show_value, "legend"))
		snprintf(string, sizeof(string), "%s: %s", ctx->x_labels[i], (char*)labels->Items[i]);
	    else
		snprintf(string, sizeof(string), "%s", ctx->x_labels[i]);
	    snprintf(colorString, sizeof(colorString), "%c%c", pcolor[i+1], '7');
	    mgl_add_legend(ctx->gr, string, colorString);
	    }
#ifdef HAVE_MGL2
	char opt[32];
	snprintf(opt, sizeof(opt), "size %.1f; value 0.1", axis_fontsize * ctx->font_scale_factor);
	mgl_legend_pos(ctx->gr, -0.04, 0.0, "A", opt);
#else
	mgl_legend_xy(ctx->gr, -0.28, 0.0, "", axis_fontsize * ctx->font_scale_factor, 0.1);
#endif

	/** Generate the pie chart itself **/
#ifdef HAVE_MGL2
	mgl_chart(ctx->gr, ctx->chart_data, pcolor, "");
#else
	mgl_chart(ctx->gr, ctx->chart_data, pcolor);
#endif

	/** Correctly position percentages/values around the chart **/
	for(i=0; i<reccnt; i++)
	    {
	    val = ((pRptChartValues)ctx->values->Items[i])->Values[0];
	    pct = (int)(val/sumValues*100+0.5);
	    angle = val / sumValues * 2*M_PI;
	    t = sumPreviousAngles + angle / 2.0 - M_PI;
	    t /= 3; /* We don't know how this works, but this line fixed the shifting problem. */
	    if (!strcmp(show_percent, "pie") && !strcmp(show_value, "pie"))
		snprintf(string, sizeof(string), "%s (%d%%)", (char*)labels->Items[i], pct);
	    else if (!strcmp(show_percent, "pie"))
		snprintf(string, sizeof(string), "%d%%", pct);
	    else if (!strcmp(show_value, "pie"))
		snprintf(string, sizeof(string), "%s", (char*)labels->Items[i]);
	    else
		strtcpy(string, "", sizeof(string));
#ifdef HAVE_MGL2
	    mgl_puts(ctx->gr, t, r, 0, string, "", series_fontsize * ctx->font_scale_factor);
#else
	    mgl_puts_ext(ctx->gr, t, r, 0, string, "", series_fontsize * ctx->font_scale_factor, '\0');
#endif
	    sumPreviousAngles += angle;
	    }
	
	rpt_internal_FreeValueStrings(labels);

    return 0;
    }


/*** rpt_internal_DoChart prints a bar, line, or pie chart on the report based on the
 *** information provided by the user.  MathGL is used for the rendering.
 ***/
int
rpt_internal_DoChart(pRptData inf, pStructInf chart, pRptSession rs, int container_handle)
    {
    int errval = -1; /* default */
    pRptActiveQueries ac = NULL;
    int rval;
    int i,j;
    pPrtImage img = NULL;;
    int flags;
    double x,y,w,h;
    char* chart_type;
    char* series_type;
    char* title;
    char* x_axis_label;
    char* y_axis_label;
    char* ptr;
    int box = 0;
    int stacked;
    int axis_fontsize;
    pStructInf subobj;
    pStructInf one_series;
    pRptChartValues value = NULL;
    pRptChartDriver drv;
    pRptChartContext ctx = NULL;
    double xoffset, yoffset;
    char color[8];
    int prec;
    char precstr[32];
    
        /** Conditional rendering of the chart **/
	rval = rpt_internal_CheckCondition(inf, chart);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

        /** Determine chart type **/
	if (rpt_internal_GetString(inf, chart, "chart_type", &chart_type, NULL, 0) < 0)
            {
            mssError(0, "RPT", "Chart type required for chart '%s'", chart->Name);
	    goto error;
            }
	for(i=0; i<RPT_INF.ChartDrivers.nItems; i++)
	    {
	    drv = (pRptChartDriver)RPT_INF.ChartDrivers.Items[i];
	    if (!strcmp(drv->Type, chart_type))
		break;
	    drv = NULL;
	    }
	if (!drv)
	    {
            mssError(0, "RPT", "Invalid chart type '%s' for chart '%s'", chart_type, chart->Name);
            goto error;
	    }

	/** Set up rendering context **/
	ctx = (pRptChartContext)nmMalloc(sizeof(RptChartContext));
	if (!ctx)
	    goto error;
	memset(ctx, 0, sizeof(RptChartContext));
	ctx->inf = inf;

	/** Determine axis/series counts **/
	ctx->series = xaNew(4);
	if (!ctx->series)
	    goto error;
	for(i=0; i<chart->nSubInf; i++)
	    {
	    subobj = chart->SubInf[i];
	    if (stStructType(subobj) == ST_T_SUBGROUP)
		{
		if (!strcmp(subobj->UsrType, "report/chart-series"))
		    {
		    /** Get chart type - eventually we should be able to combine line
		     ** and bar charts, but not right now.
		     **/
		    rpt_internal_GetString(inf, subobj, "chart_type", &series_type, chart_type, 0);
		    if (strcmp(series_type, chart_type) != 0)
			{
			mssError(1, "RPT", "Chart types cannot be intermixed (chart '%s', series '%s')", chart->Name, subobj->Name);
			goto error;
			}
		    xaAddItem(ctx->series, subobj);
		    }
		else if (!strcmp(subobj->UsrType, "report/chart-axis"))
		    {
		    if (rpt_internal_GetString(inf, subobj, "axis", &ptr, NULL, 0) < 0 || !ptr)
			{
			mssError(1, "RPT", "Chart axis '%s' must have an 'axis' property", subobj->Name);
			goto error;
			}
		    if (!strcmp(ptr, "x"))
			{
			if (ctx->x_axis)
			    {
			    mssError(1, "RPT", "Chart '%s' must have only one x axis", chart->Name);
			    goto error;
			    }
			ctx->x_axis = subobj;
			}
		    else if (!strcmp(ptr, "y"))
			{
			if (ctx->y_axis)
			    {
			    mssError(1, "RPT", "Chart '%s' must have only one y axis", chart->Name);
			    goto error;
			    }
			ctx->y_axis = subobj;
			}
		    else
			{
			mssError(1, "RPT", "Chart axis '%s' must have an 'axis' property of 'x' or 'y'", subobj->Name);
			goto error;
			}
		    }
		}
	    }

	/** Verify at least one series and two axes **/
	if (ctx->series->nItems == 0 || !ctx->x_axis || !ctx->y_axis)
	    {
	    mssError(1, "RPT", "chart '%s' must have x and y axes and at least one series", chart->Name);
	    goto error;
	    }

	/** Title and labels **/
	rpt_internal_GetString(inf, chart, "title", &title, "", 0);
	rpt_internal_GetString(inf, ctx->x_axis, "label", &x_axis_label, "", 0);
	rpt_internal_GetString(inf, ctx->y_axis, "label", &y_axis_label, "", 0);

	/** Start the query to get the chart values. **/
	if ((ac = rpt_internal_Activate(inf, chart, rs)) == NULL)
	    goto error;

	/** Fetch the first row. **/
	if ((rval = rpt_internal_NextRecord(ac, inf, chart, rs, 1)) < 0)
	    goto error;

	/** Enter the row retrieval loop. **/
	ctx->values = xaNew(16);
	if (!ctx->values)
	    goto error;
	while(rval == 0)
            {
            /** Get the labels for the x-axis **/
	    rpt_internal_GetString(inf, (pStructInf)(ctx->series->Items[0]), "x_value", &ptr, "", 0);

	    /** Allocate the tuple **/
	    value = rpt_internal_NewChartValues(ptr, ctx->series->nItems);
	    if (!value)
		goto error;
            
            /** Get the y values **/
	    for(i=0; i<ctx->series->nItems; i++)
		{
		one_series = (pStructInf)xaGetItem(ctx->series, i);
		if (rpt_internal_GetDouble(inf, one_series, "y_value", &(value->Values[i]), 0.0, 0) < 0)
		    {
		    mssError(0, "RPT", "Could not get y_value for series '%s'", one_series->Name);
		    goto error;
		    }
		}

	    /** Next record **/
	    xaAddItem(ctx->values, value);
	    value = NULL;
	    rval = rpt_internal_NextRecord(ac, inf, chart, rs, 0);
	    }

	/** We're finished with the query **/
	rpt_internal_Deactivate(inf, ac);
	ac = NULL;

	/** Bar chart?  Add empty space on each end. **/
	if (drv->PreProcessData(ctx) < 0)
	    goto error;

	/** Allocate the chart data **/
	ctx->chart_data = mgl_create_data_size(ctx->values->nItems, ctx->series->nItems, 1);
	if (!ctx->chart_data)
	    goto error;
	ctx->x_labels = nmSysMalloc(sizeof(char*) * ctx->values->nItems);
	if (!ctx->x_labels)
	    goto error;

	/** Transfer our chart values to the MGL data and while we're at
	 ** it, determine min/max data values too.
	 **/
	ctx->min = 0;
	ctx->max = 0;
	for(i=0; i<ctx->values->nItems; i++)
	    {
	    value = (pRptChartValues)ctx->values->Items[i];
	    ctx->x_labels[i] = value->Label;
	    for(j=0; j<ctx->series->nItems; j++)
		{
		mgl_data_set_value(ctx->chart_data, (float)value->Values[j], i, j, 0);
		if (ctx->min > value->Values[j]) ctx->min = value->Values[j];
		if (ctx->max < value->Values[j]) ctx->max = value->Values[j];
		}
	    }
	value = NULL;

        /** Get area geometry in given units **/
        rpt_internal_GetDouble(inf, chart, "x", &x, -1.0, 0);
	rpt_internal_GetDouble(inf, chart, "y", &y, -1.0, 0);
	if (rpt_internal_GetDouble(inf, chart, "width", &w, NAN, 0) < 0 || w == 0) goto error;
	if (rpt_internal_GetDouble(inf, chart, "height", &h, NAN, 0) < 0 || h == 0) goto error;

	/** Chart configuration **/
	box = rpt_internal_GetBool(inf, chart, "box", 0, 0);
	ctx->scale = rpt_internal_GetBool(inf, chart, "scale", 0, 0);
	stacked = rpt_internal_GetBool(inf, chart, "stacked", 0, 0);
	ctx->rotation = rpt_internal_GetBool(inf, chart, "text_rotation", 0, 0);
	rpt_internal_GetDouble(inf, chart, "zoom", &ctx->zoom, 1.0, 0);
	rpt_internal_GetInteger(inf, chart, "fontsize", &ctx->fontsize, prtGetFontSize(container_handle), 0);
	if (ctx->fontsize < 1)
	    goto error;

        /* Convert to standard units */
        ctx->stand_w = prtUnitX(rs->PSession, w);
        ctx->stand_h = prtUnitY(rs->PSession, h);
        
        /* Get the resolution */
        prtGetResolution(rs->PSession, &ctx->xres, &ctx->yres);
        
        /* Get Actual pixel dimensions */
        ctx->x_pixels = ctx->stand_w * (double)ctx->xres / 10;
        ctx->y_pixels = ctx->stand_h * (double)ctx->yres / 6;

	/** Trim for absence of title / label **/
	if (!*title)
	    ctx->trim.top = 0.14 + (ctx->stand_h - 8) / 42.0 * 0.025;
	else
//#ifdef HAVE_MGL2
//	    ctx->trim.top = (ctx->stand_h - 8) / 42.0 * 0.01;
//#else
	    ctx->trim.top = (ctx->stand_h - 8) / 42.0 * 0.03;
//#endif
	if (!*x_axis_label && !ctx->rotation)
	    ctx->trim.bottom = 0.075 + (ctx->stand_h - 8) / 42.0 * 0.08;
	else
	    ctx->trim.bottom = (ctx->stand_h < 12)?0.0:((ctx->stand_h - 12) / 42.0 * 0.13);

	/** Rendering dimensions **/
	ctx->rend_x_pixels = round(ctx->x_pixels / (1.0 - ctx->trim.left - ctx->trim.right));
	ctx->rend_y_pixels = round(ctx->y_pixels / (1.0 - ctx->trim.top - ctx->trim.bottom));

	/** Font scaling **/
#ifdef HAVE_MGL2
	ctx->font_scale_factor = 6.4 / ctx->stand_h * ctx->y_pixels / ctx->rend_y_pixels * ctx->zoom;
#else
	ctx->font_scale_factor = 8.0 / ctx->stand_h * ctx->y_pixels / ctx->rend_y_pixels * ctx->zoom;
#endif
        
	/** Chart color **/
	strtcpy(color, rpt_internal_GetMglColor(ctx, chart, "color", "b", 0), sizeof(color));
	ctx->color = color;

	/** Create the chart **/
#ifdef HAVE_MGL2
	ctx->gr = mgl_create_graph(ctx->rend_x_pixels, ctx->rend_y_pixels);
#else
	ctx->gr = mgl_create_graph_zb(ctx->rend_x_pixels, ctx->rend_y_pixels);
#endif
	if (!ctx->gr)
	    goto error;
	mgl_set_rotated_text(ctx->gr, ctx->rotation?1:0);
	if (ctx->zoom < 0.999 || ctx->zoom > 1.001)
	    mgl_set_plotfactor(ctx->gr, 1.55*ctx->zoom);

	/** Decimal precision **/
	prec = rpt_internal_GetYDecimalPrecision(ctx, -1, -1);
	snprintf(precstr, sizeof(precstr), "%%.%df", prec);
#ifdef HAVE_MGL2
	mgl_set_tick_templ(ctx->gr, 'y', precstr);
#else
	mgl_set_ytt(ctx->gr, precstr);
#endif

	/** Graphics setup for chart type **/
	if (drv->SetupFormat(ctx) < 0)
	    goto error;

	/** Y axis min/max/ticks **/
	ctx->tickdist = rpt_internal_GetTickDist(ctx->max - ctx->min);
	ctx->minaxis = (ctx->min == 0)?0.0:(ctx->min - ctx->tickdist);
	ctx->maxaxis = ctx->max + ctx->tickdist;

        /** Draw the chart, depending on type **/
	if (drv->Generate(ctx) < 0)
	    goto error;
      
	/** Title and axis labels **/
	if (*title)
#ifdef HAVE_MGL2
	    mgl_puts(ctx->gr, 0.5, 0.9, 0.0, title, "A", ctx->fontsize * ctx->font_scale_factor);
#else
	    mgl_title(ctx->gr, title, "", ctx->fontsize * ctx->font_scale_factor);
#endif
	if (*x_axis_label)
	    {
	    rpt_internal_GetInteger(inf, ctx->x_axis, "fontsize", &axis_fontsize, ctx->fontsize, 0);
#ifdef HAVE_MGL2
	    char opt[32];
	    snprintf(opt, sizeof(opt), "size %.1f; value %.2f", axis_fontsize * ctx->font_scale_factor, axis_fontsize * ctx->font_scale_factor / 50.0 + ctx->stand_h / 100.0);
	    mgl_puts(ctx->gr, 0.5, 0.04, 0.0, x_axis_label, "A", ctx->fontsize * ctx->font_scale_factor);
	    //mgl_label(ctx->gr, 'x', x_axis_label, 0, opt);
#else
	    //mgl_label_ext(ctx->gr, 'x', x_axis_label, 0, axis_fontsize * ctx->font_scale_factor, axis_fontsize * ctx->font_scale_factor / 50.0 + (ctx->stand_h - ctx->min * 0.0006) / 50.0);
	    //mgl_puts_ext(ctx->gr, ctx->values->nItems * 0.5, 0.04, 0.0, x_axis_label, "", axis_fontsize * ctx->font_scale_factor, 'x');
	    mgl_label_xy(ctx->gr, 0.5, ctx->trim.bottom + 0.01, x_axis_label, "", axis_fontsize * ctx->font_scale_factor);
#endif
	    }
	if (*y_axis_label)
	    {
	    rpt_internal_GetInteger(inf, ctx->y_axis, "fontsize", &axis_fontsize, ctx->fontsize, 0);
	    //mgl_label_ext(ctx->gr, 'y', y_axis_label, 0, axis_fontsize * ctx->font_scale_factor, -0.0);
	    xoffset = (ctx->values->nItems - 0.8) / -4.2;
#ifdef HAVE_MGL2
	    yoffset = (ctx->maxaxis + ctx->minaxis)/2.0;
	    mgl_set_rotated_text(ctx->gr, 1);
	    mgl_puts_dir(ctx->gr, xoffset, yoffset, 0.0, xoffset, yoffset+1.0, 0.0, y_axis_label, "", axis_fontsize * ctx->font_scale_factor);
	    mgl_set_rotated_text(ctx->gr, ctx->rotation?1:0);
#else
	    yoffset = (ctx->max - ctx->min)/2.0 - (strlen(y_axis_label) * axis_fontsize) * (ctx->max - ctx->min) / 520.0 * ctx->font_scale_factor + ctx->min;
	    mgl_puts_dir(ctx->gr, xoffset, yoffset, 0.0, 0.0, 1.0, 0.0, y_axis_label, axis_fontsize * ctx->font_scale_factor);
#endif
	    }

	/** Chart box **/
	if (box)
#ifdef HAVE_MGL2
	    mgl_box(ctx->gr);
#else
	    mgl_box(ctx->gr, 1);
#endif

	/** Render the chart **/
	img = rpt_internal_GraphToImage(ctx);
	if (!img)
	    {
	    mssError(0, "RPT", "Could not create chart");
	    goto error;
	    }
	mgl_delete_data(ctx->chart_data);
	ctx->chart_data = NULL;
	mgl_delete_graph(ctx->gr);
	ctx->gr = NULL;

	/** Check flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;

	/** Add it to its container **/
	if (prtWriteImage(container_handle, img, x,y,w,h, flags) < 0)
	    goto error;
	img = NULL;

	/** Fall through to error handler, to cleanup our resources,
	 ** but return success.
	 **/
	errval = 0;

    error:
	if (ctx->x_labels)
	    nmSysFree(ctx->x_labels);
	if (ctx->series)
	    xaFree(ctx->series);
	if (value)
	    rpt_internal_FreeChartValues(value);
	if (ctx->values)
	    {
	    for(i=0; i<ctx->values->nItems; i++)
		{
		rpt_internal_FreeChartValues((pRptChartValues)xaGetItem(ctx->values, i));
		}
	    xaFree(ctx->values);
	    }
	if (ac)
	    rpt_internal_Deactivate(inf, ac);
	if (ctx->chart_data)
	    mgl_delete_data(ctx->chart_data);
	if (ctx->gr)
	    mgl_delete_graph(ctx->gr);
	if (img)
	    prtFreeImage(img);
	if (ctx)
	    nmFree(ctx, sizeof(RptChartContext));
	return errval;
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
	    ud = (pRptUserData)xaGetItem(&(inf->UserDataSlots), (intptr_t)(condition_inf->UserData));
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
	    if (rpt_internal_GetBool(inf, config,"condition",1,0))
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
    double lh;
    int j;

	/** Check for font, size, color **/
	if (rpt_internal_GetString(inf, config, "font", &ptr, NULL, 0) >= 0)
	    {
	    if (prtSetFont(prt_obj, ptr) < 0)
		return -1;
	    }
	if (rpt_internal_GetInteger(inf, config, "fontsize", &n, RPT_INT_UNSPEC, 0) >= 0)
	    {
	    if (prtSetFontSize(prt_obj, n) < 0)
		return -1;
	    }
	if (rpt_internal_GetString(inf, config, "fontcolor", &ptr, NULL, 0) >= 0)
	    {
	    if (ptr[0] == '#')
		{
		n = strtoi(ptr+1, NULL, 16);
		if (prtSetColor(prt_obj, n) < 0)
		    return -1;
		}
	    }
	if (rpt_internal_GetString(inf, config, "bgcolor", &ptr, NULL, 0) >= 0)
	    {
	    if (ptr[0] == '#')
		{
		n = strtoi(ptr+1, NULL, 16);
		if (prtSetBGColor(prt_obj, n) < 0)
		    return -1;
		}
	    }
	if (rpt_internal_GetDouble(inf, config, "lineheight", &lh, NAN, 0) >= 0)
	    {
	    if (prtSetLineHeight(prt_obj, lh) < 0)
		return -1;
	    }

	/** Check justification **/
	if (rpt_internal_GetString(inf, config, "align", &ptr, NULL, 0) >= 0)
	    {
	    if (!strcmp(ptr,"left"))
		j = PRT_JUST_T_LEFT;
	    else if (!strcmp(ptr,"right"))
		j = PRT_JUST_T_RIGHT;
	    else if (!strcmp(ptr,"center"))
		j = PRT_JUST_T_CENTER;
	    else if (!strcmp(ptr,"full"))
		j = PRT_JUST_T_FULL;
	    else
		return -1;
	    if (prtSetJustification(prt_obj, j) < 0)
		return -1;
	    }

	/** Check attrs (bold/italic/underline) **/
	attr = prtGetAttr(prt_obj);
	for(i=0;i<16;i++)
	    {
	    if (rpt_internal_GetString(inf, config, "style", &ptr, NULL, i) < 0) break;
	    if (!strcmp(ptr,"bold")) attr |= PRT_OBJ_A_BOLD;
	    else if (!strcmp(ptr,"italic")) attr |= PRT_OBJ_A_ITALIC;
	    else if (!strcmp(ptr,"underline")) attr |= PRT_OBJ_A_UNDERLINE;
	    else if (!strcmp(ptr,"plain")) attr = 0;
	    }
	if (prtSetAttr(prt_obj, attr) < 0)
	    return -1;

    return 0;
    }


/*** rpt_internal_SetMargins - sets the margins for a print object
 ***/
int
rpt_internal_SetMargins(pRptData inf, pStructInf config, int prt_obj, double dt, double db, double dl, double dr)
    {
    double ml, mr, mt, mb;

	/** Set the margins **/
	rpt_internal_GetDouble(inf, config, "margintop", &mt, dt, 0);
	rpt_internal_GetDouble(inf, config, "marginbottom", &mb, db, 0);
	rpt_internal_GetDouble(inf, config, "marginleft", &ml, dl, 0);
	rpt_internal_GetDouble(inf, config, "marginright", &mr, dr, 0);
	if (mt >= 0.0 || mb >= 0.0 || ml >= 0.0 || mr >= 0.0) 
	    {
	    if (prtSetMargins(prt_obj, mt, mb, ml, mr) < 0)
		return -1;
	    }

    return 0;
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

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"RPT","Could not generate report: resource exhaustion occurred");
	    return -1;
	    }

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
                else if (!strcmp(subobj->UsrType, "report/svg"))
                    {
                    rval = rpt_internal_DoSvg(inf, subobj, rs, NULL, container_handle);
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
                else if (!strcmp(subobj->UsrType, "report/section"))
                    {
                    rval = rpt_internal_DoSection(inf, subobj, rs, container_handle);
		    if (rval < 0) break;
                    }
                else if (!strcmp(subobj->UsrType, "report/chart"))
                    {
                    rval = rpt_internal_DoChart(inf, subobj, rs, container_handle);
		    if (rval < 0) break;
                    }
		}
	    }

    return rval;
    }


/*** rpt_internal_DoImage - put a picture on the report.
 ***/
int
rpt_internal_DoImage(pRptData inf, pStructInf image, pRptSession rs, pRptSource this_qy, int container_handle)
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
	rpt_internal_GetDouble(inf, image, "x", &x, -1.0, 0);
	rpt_internal_GetDouble(inf, image, "y", &y, -1.0, 0);
	if (rpt_internal_GetDouble(inf, image, "width", &w, NAN, 0) < 0)
	    {
	    mssError(1,"RPT","report/image must have a valid 'width' attribute");
	    return -1;
	    }
	if (rpt_internal_GetDouble(inf, image, "height", &h, NAN, 0) < 0)
	    {
	    mssError(1,"RPT","report/image must have a valid 'height' attribute");
	    return -1;
	    }

	/** Load the image **/
	if (stGetAttrValueOSML(stLookup(image,"source"), DATA_T_STRING, POD(&imgsrc), 0, inf->Obj->Session) != 0)
	    {
	    mssError(1,"RPT","report/image object must have a valid 'source' attribute");
	    return -1;
	    }
	if ((imgobj = objOpen(inf->Obj->Session, imgsrc, O_RDONLY, 0400, "image/png")) == NULL)
	    {
	    mssError(0,"RPT","Could not open 'source' image for report/image object");
	    return -1;
	    }
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


/*** rpt_internal_DoSvg - put an svg image on the report.
 ***/
int
rpt_internal_DoSvg(pRptData inf, pStructInf image, pRptSession rs, pRptSource this_qy, int container_handle)
    {
    char* svgsrc;
    pPrtSvg svg;
    pObject svgobj;
    int flags;
    double x,y,w,h;
    int rval;

	rval = rpt_internal_CheckCondition(inf,image);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get area geometry **/
	rpt_internal_GetDouble(inf, image, "x", &x, -1.0, 0);
	rpt_internal_GetDouble(inf, image, "y", &y, -1.0, 0);
	if (rpt_internal_GetDouble(inf, image, "width", &w, NAN, 0) < 0)
	    {
	    mssError(1,"RPT","report/svg must have a valid 'width' attribute");
	    return -1;
	    }
	if (rpt_internal_GetDouble(inf, image, "height", &h, NAN, 0) < 0)
	    {
	    mssError(1,"RPT","report/svg must have a valid 'height' attribute");
	    return -1;
	    }

	/** Load the image **/
	if (stGetAttrValueOSML(stLookup(image,"source"), DATA_T_STRING, POD(&svgsrc), 0, inf->Obj->Session) != 0)
	    {
	    mssError(1,"RPT","report/svg object must have a valid 'source' attribute");
	    return -1;
	    }
	if ((svgobj = objOpen(inf->Obj->Session, svgsrc, O_RDONLY, 0400, "image/svg+xml")) == NULL)
	    {
	    mssError(0,"RPT","Could not open 'source' image for report/svg object");
	    return -1;
	    }
	svg = prtReadSvg(objRead, svgobj);
	objClose(svgobj);
	if (!svg) return -1;

	/** Check flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;

	/** Add it to its container **/
	if (prtWriteSvgToContainer(container_handle, svg, x,y,w,h, flags) < 0) return -1;

    return 0;
    }


/*** rpt_internal_DoArea - build an 'area' on the report in which textflow type
 *** content can be placed.  V3 prtmgmt.
 ***/
int
rpt_internal_DoArea(pRptData inf, pStructInf area, pRptSession rs, pRptSource this_qy, int container_handle)
    {
    int area_handle = -1;
    double x, y, w, h, bw;
    int flags;
    pPrtBorder bdr = NULL;
    int rval = 0;

	rval = rpt_internal_CheckCondition(inf,area);
	if (rval < 0) return rval;
	if (rval == 0) return 0;

	/** Get area geometry **/
	rpt_internal_GetDouble(inf, area, "x", &x, -1.0, 0);
	rpt_internal_GetDouble(inf, area, "y", &y, -1.0, 0);
	rpt_internal_GetDouble(inf, area, "width", &w, -1.0, 0);
	rpt_internal_GetDouble(inf, area, "height", &h, -1.0, 0);

	/** Check for flags **/
	flags = 0;
	if (x >= 0.0) flags |= PRT_OBJ_U_XSET;
	if (y >= 0.0) flags |= PRT_OBJ_U_YSET;
	if (rpt_internal_GetBool(inf, area, "allowbreak", 1, 0)) flags |= PRT_OBJ_U_ALLOWBREAK;
	if (rpt_internal_GetBool(inf, area, "fixedsize", 0, 0)) flags |= PRT_OBJ_U_FIXEDSIZE;

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
	if (area_handle < 0)
	    goto error;

	/** Set the style for the area **/
	if (rpt_internal_SetStyle(inf, area, rs, area_handle) < 0)
	    goto error;
	if (rpt_internal_SetMargins(inf, area, area_handle, 0, 0, 0, 0) < 0)
	    goto error;

	/** Do stuff that is contained in the area **/
	if (stLookup(area, "value"))
	    {
	    if (rpt_internal_DoData(inf, area, rs, area_handle) < 0)
		goto error;
	    }
	rval = rpt_internal_DoContainer(inf, area, rs, area_handle);

	/** End the area **/
	prtEndObject(area_handle);

	return rval;

    error:
	if (area_handle >= 0)
	    prtEndObject(area_handle);
	return -1;
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
    //int t, rval;
    pRptUserData ud;
    pExpression exp;

	/** See if we have a real expression here **/
	exp = stGetExpression(obj, 0);
	if (exp)
	//t = stGetAttrType(obj, 0);
	//if (t == DATA_T_CODE)
	//    {
	//    rval = stGetAttrValue(obj, t, POD(&exp), 0);
	//    if (rval == 0)
		{
		/** Okay, got an expression.  Bind it to the current objlist **/
		ud = (pRptUserData)nmMalloc(sizeof(RptUserData));
		if (!ud) return -1;
		ud->LastValue = NULL;
		ud->Exp = expDuplicateExpression(exp);
		expBindExpression(ud->Exp, objlist, EXPR_F_RUNSERVER);

		/** ... and set it up in our user data slots structure **/
		if (!(obj->UserData)) 
		    obj->UserData = (void*)(intptr_t)(inf->NextUserDataSlot++);
		else if ((intptr_t)(obj->UserData) >= inf->NextUserDataSlot)
		    inf->NextUserDataSlot = ((intptr_t)(obj->UserData))+1;
		xaSetItem(&(inf->UserDataSlots), (intptr_t)(obj->UserData), (void*)ud);
		}
	    else
		{
		/** Fail if couldn't get the expression **/
		return -1;
		}
	//    }
	//else if (t < 0)
	//    {
	//    /** Fail if couldn't determine type, or obj is invalid **/
	//    return -1;
	//    }

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
    int i, j;
    //pXArray xa = NULL;
    //pStructInf expr_inf;
    //pStructInf x_labels;
    pStructInf attr;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"RPT","Could not generate report: resource exhaustion occurred");
	    goto error;
	    }

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

	/** Handle attributes and subobjects **/
	for(i=0;i<object->nSubInf;i++) 
	    {
	    if (stStructType(object->SubInf[i]) == ST_T_SUBGROUP)
		{
		/** Process subobject **/
		if (rpt_internal_PreProcess(inf, object->SubInf[i], rs, use_objlist) < 0)
		    {
		    goto error;
		    }
		}
	    else
		{
		/** Process an attribute **/
		attr = object->SubInf[i];
		attr->UserData = NULL;

		/** Search the list of objects/attrs we set up UserData for. **/
		for(j=0; j<sizeof(rpt_ud_list) / sizeof(RptUDItem) - 1; j++)
		    {
		    if (!strcmp(object->UsrType, rpt_ud_list[j].Object) && !strcmp(attr->Name, rpt_ud_list[j].Attr))
			{
			if (rpt_internal_PreBuildExp(inf, attr, use_objlist) < 0)
			    {
			    mssError(1,"RPT","%s on '%s' must have a valid expression.", attr->Name, object->Name);
			    goto error;
			    }
			}
		    }
		}
	    }

	return 0;

    error:
	return -1;
    }


/*** rpt_internal_UnPreProcess - undo the preprocessing step to free up
 *** the "UserData" stuff.
 ***/
int
rpt_internal_UnPreProcess(pRptData inf, pStructInf object, pRptSession rs)
    {
    int i;
    pRptUserData ud;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"RPT","Could not generate report: resource exhaustion occurred");
	    return -1;
	    }

	/** Visit all sub-inf structures that are groups **/
	for(i=0;i<object->nSubInf;i++) if (stStructType(object->SubInf[i]) == ST_T_SUBGROUP)
	    {
	    rpt_internal_UnPreProcess(inf, object->SubInf[i], rs);
	    }

	/** Free our copy of report expressions in the UserData **/
	for(i=1;i<inf->UserDataSlots.nItems;i++) if (inf->UserDataSlots.Items[i])
	    {
	    ud = (pRptUserData)(inf->UserDataSlots.Items[i]);
	    expFreeExpression(ud->Exp);
	    inf->UserDataSlots.Items[i] = NULL;
	    nmFree(ud, sizeof(RptUserData));
	    }

    return 0;
    }


/*** rpt_internal_FreeSource - callback routine for xhClear to free a query
 *** conn structure.
 ***/
int
rpt_internal_FreeSource(pRptSource src)
    {
    int i;
    pRptSourceAgg aggregate;

	/** Clear the reference in the object list **/
	expModifyParam(src->Inf->ObjList, src->Name, NULL);
   	
	/** Release src aggregates **/
	for(i=0;i<src->Aggregates.nItems;i++) 
	    {
	    aggregate = (pRptSourceAgg)src->Aggregates.Items[i];
	    if (aggregate->Value) expFreeExpression(aggregate->Value);
	    if (aggregate->Where) expFreeExpression(aggregate->Where);
	    if (aggregate->Name) nmSysFree(aggregate->Name);
	    nmFree(aggregate, sizeof(RptSourceAgg));
	    }
	xaDeInit(&(src->Aggregates));

	/** Release src data, and src structure. **/
    	if (src->DataBuf) nmSysFree(src->DataBuf);
    	if (src->ObjList) expFreeParamList(src->ObjList);
    	nmFree(src, sizeof(RptSource));

    return 0;
    }


/*** rpt_internal_LoadEndorsements - look for security endorsements
 ***/
int
rpt_internal_LoadEndorsements(pRptData inf, pStructInf req)
    {
    char* endorsement_sql = NULL;
    pObjQuery eqy;
    pObject eobj;
    char* one_endorsement;
    char* one_context;

	/** Check for endorsements to load **/
	if (rpt_internal_GetString(inf, req, "add_endorsements_sql", &endorsement_sql, NULL, 0) >= 0 && endorsement_sql)
	    {
	    /** Run the SQL to fetch the endorsements **/
	    eqy = objMultiQuery(inf->Obj->Session, endorsement_sql, NULL, 0);
	    if (eqy)
		{
		/** Loop through returned rows **/
		while ((eobj = objQueryFetch(eqy, O_RDONLY)) != NULL)
		    {
		    if (objGetAttrValue(eobj, "endorsement", DATA_T_STRING, POD(&one_endorsement)) == 0)
			{
			if (objGetAttrValue(eobj, "context", DATA_T_STRING, POD(&one_context)) == 0)
			    {
			    cxssAddEndorsement(one_endorsement, one_context);
			    }
			}
		    objClose(eobj);
		    }
		objQueryClose(eqy);
		}
	    }

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
    XHashTable queries;
    pStructInf subreq,req = inf->Node->Data;
    pObjSession s = inf->Obj->Session;
    int i,j;
    pRptSource src;
    pRptSession rsess;
    int err = 0;
    pXString title_str = NULL;
    pXString subst_str;
    pExpression exp;
    int resolution;
    double pagewidth, pageheight;
    pRptSourceAgg aggregate = NULL;

	/** Report has a title? **/
	rpt_internal_GetParamValue(inf, "title", DATA_T_STRING, POD(&title), inf->ObjList, s);

	/** If had a title, do param subst on it. **/
	if (title) 
	    {
	    title_str = rpt_internal_SubstParam(inf, title);
	    title = title_str->String;
	    }

	/** Resolution specified? **/
	if (rpt_internal_GetParamValue(inf, "resolution", DATA_T_INTEGER, POD(&resolution), inf->ObjList, s) == 0)
	    {
	    prtSetResolution(ps, resolution);
	    }

	/** Units of measure? **/
	ptr = NULL;
	if (rpt_internal_GetString(inf, req, "units", &ptr, NULL, 0) >= 0)
	    {
	    if (prtSetUnits(ps, ptr) < 0)
		{
		mssError(1,"RPT","Invalid units-of-measure '%s' specified", ptr);
		err = 1;
		}
	    }

	/** Security endorsements? **/
	rpt_internal_LoadEndorsements(inf, req);

	/** Page geometry? **/
	prtGetPageGeometry(ps, &pagewidth, &pageheight);
	rpt_internal_GetDouble(inf, req, "pagewidth", &pagewidth, pagewidth, 0);
	rpt_internal_GetDouble(inf, req, "pageheight", &pageheight, pageheight, 0);
	prtSetPageGeometry(ps, pagewidth, pageheight);

	/** Ok, now look through the request for queries, comments, tables, forms **/
	xhInit(&queries, 31, 0);
	rsess = (pRptSession)nmMalloc(sizeof(RptSession));
	rsess->Inf = (void*)inf;
	rsess->PSession = ps;
	rsess->FD = out_fd;
	rsess->Queries = &queries;
	rsess->ObjSess = s;
	rsess->HeaderInf = NULL;
	rsess->FooterInf = NULL;
	inf->RSess = rsess;
	rsess->PageHandle = prtGetPageRef(ps);
	prtSetPageNumber(rsess->PageHandle, 1);
	rpt_internal_SetMargins(inf, req, rsess->PageHandle, 3.0, 3.0, 0, 0);

	/** Build the object list. **/
	for(i=0;i<req->nSubInf;i++) if (stStructType(req->SubInf[i]) == ST_T_SUBGROUP && !strcmp(req->SubInf[i]->UsrType,"report/query"))
	    {
	    expAddParamToList(inf->ObjList, req->SubInf[i]->Name, NULL, 0);
	    expSetParamFunctions(inf->ObjList, req->SubInf[i]->Name, rpt_internal_QyGetAttrType, 
	    	rpt_internal_QyGetAttrValue, NULL);
	    }

	/** Evaluate defaults on all parameters **/
	if (rpt_internal_SetParamDefaults(inf) < 0)
	    err = 1;

	/** Preprocess the report structure, compile its expressions **/
	if (rpt_internal_PreProcess(inf, req, rsess, inf->ObjList) < 0) err = 1;

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

	/** Set top-level formatting and report defaults. **/
	cxssPushContext();
	cxssSetVariable("dfmt", obj_default_date_fmt, 0);
	cxssSetVariable("mfmt", obj_default_money_fmt, 0);
	cxssSetVariable("nfmt", obj_default_null_fmt, 0);
	rpt_internal_CheckFormats(inf, req);

	/** Now do the 'normal' report stuff **/
	for(i=0;i<req->nSubInf;i++)
	    {
	    if (stStructType(req->SubInf[i]) == ST_T_SUBGROUP)
		{
		subreq = req->SubInf[i];
		if (!strcmp(subreq->UsrType,"report/query"))
		    {
		    /** First, build the query information structure **/
		    src=(pRptSource)nmMalloc(sizeof(RptSource));
		    if (!src) break;
		    src->UserInf = subreq;
		    src->Inf = inf;
		    ptr=NULL;
		    /*if (stAttrValue(stLookup(subreq,"name"),NULL,&ptr,0) < 0) continue;*/
		    src->Name = subreq->Name;
		    src->Query = NULL;
		    src->QueryItem = NULL;
		    src->DataBuf = NULL;
		    src->ObjList = expCreateParamList();
		    xaInit(&(src->Aggregates),16);
		    expAddParamToList(src->ObjList, "this", NULL, EXPR_O_CURRENT);
		    expSetParamFunctions(src->ObjList, "this", rpt_internal_QyGetAttrType, rpt_internal_QyGetAttrValue, NULL);
		    xhAdd(&queries, src->Name, (char*)src);

		    /** Now, check for any aggregate/summary elements **/
		    for(j=0;j<subreq->nSubInf;j++)
		        {
			if (stStructType(subreq->SubInf[j]) == ST_T_SUBGROUP && !strcmp(subreq->SubInf[j]->UsrType,"report/aggregate"))
			    {
			    aggregate = (pRptSourceAgg)nmMalloc(sizeof(RptSourceAgg));
			    if (!aggregate)
				{
				err = 1;
				break;
				}
			    memset(aggregate, 0, sizeof(RptSourceAgg));
			    aggregate->Name = nmSysStrdup(subreq->SubInf[j]->Name);

			    /** Value of the aggregate itself **/
			    stAttrValue(stLookup(subreq->SubInf[j], "compute"), NULL, &ptr, 0);
			    if (!ptr)
			        {
				mssError(1,"RPT","report/aggregate element '%s' must have a compute= attribute", subreq->SubInf[j]->Name);
				err = 1;
				break;
				}
			    subst_str = rpt_internal_SubstParam(inf, ptr);
	    		    exp = expCompileExpression(subst_str->String, src->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
			    xsFree(subst_str);
			    if (!exp)
			        {
				mssError(0,"RPT","invalid compute expression in report/aggregate '%s'", subreq->SubInf[j]->Name);
				err = 1;
				break;
				}
			    exp->DataType = DATA_T_UNAVAILABLE;
		            expResetAggregates(exp, -1,1);
			    expEvalTree(exp,src->ObjList);
			    aggregate->Value = exp;

			    /** Whether to reset the value on each read **/
			    aggregate->DoReset = 1;
			    stAttrValue(stLookup(subreq->SubInf[j], "reset"), &aggregate->DoReset, NULL, 0);

			    /** Where value - what objects this aggregate will summarize **/
			    ptr = NULL;
			    stAttrValue(stLookup(subreq->SubInf[j], "where"), NULL, &ptr, 0);
			    if (ptr)
			        {
			        subst_str = rpt_internal_SubstParam(inf, ptr);
				exp = expCompileExpression(subst_str->String, src->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
				xsFree(subst_str);
				if (!exp)
				    {
				    mssError(0,"RPT","invalid where expression in report/aggregate '%s'", subreq->SubInf[j]->Name);
				    err = 1;
				    break;
				    }
				aggregate->Where = exp;
				}

			    xaAddItem(&(src->Aggregates), (void*)aggregate);
			    aggregate = NULL;
			    }
			}
		    if (err && aggregate)
			{
			if (aggregate->Where)
			    expFreeExpression(aggregate->Where);
			if (aggregate->Value)
			    expFreeExpression(aggregate->Value);
			if (aggregate->Name)
			    nmSysFree(aggregate->Name);
			nmFree(aggregate, sizeof(RptSourceAgg));
			}
		    }
                else if (!strcmp(subreq->UsrType,"report/section"))
                    {
                    if (rpt_internal_DoSection(inf, subreq, rsess, rsess->PageHandle) <0)
                        {
                        err = 1;
                        break;
                        }
                    }
		else if (!strcmp(subreq->UsrType,"report/table"))
		    {
		    if (rpt_internal_DoTable(inf, subreq,rsess,rsess->PageHandle) <0)
		        {
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/area"))
		    {
		    if (rpt_internal_DoArea(inf, subreq, rsess, NULL, rsess->PageHandle) < 0)
			{
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/image"))
		    {
		    if (rpt_internal_DoImage(inf, subreq, rsess, NULL, rsess->PageHandle) < 0)
			{
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType, "report/svg"))
                    {
                    if (rpt_internal_DoSvg(inf, subreq, rsess, NULL, rsess->PageHandle) < 0)
                        {
                        err = 1;
                        break;
                        }
                    }
                else if (!strcmp(subreq->UsrType,"report/data"))
		    {
		    if (rpt_internal_DoData(inf, subreq, rsess, rsess->PageHandle) <0)
		        {
			err = 1;
			break;
			}
		    }
		else if (!strcmp(subreq->UsrType,"report/form"))
		    {
		    if (rpt_internal_DoForm(inf, subreq,rsess, rsess->PageHandle) <0)
		        {
			err = 1;
			break;
			}
		    }
                else if (!strcmp(subreq->UsrType,"report/chart"))
		    {
		    if (rpt_internal_DoChart(inf, subreq,rsess, rsess->PageHandle) <0)
		        {
			err = 1;
			break;
			}
		    }
		}
	    if (err) break;
	    }

	/** Clean up queries **/
	xhClear(&queries, rpt_internal_FreeSource, NULL);
	xhDeInit(&queries);
	inf->RSess = NULL;

	/** Undo formatting changes **/
	cxssPopContext();

	/** Undo the preprocessing - release the expression trees **/
	rpt_internal_UnPreProcess(inf, req, rsess);
	nmFree(rsess,sizeof(RptSession));

	/** End with a form feed, close up and get out. **/
	/*prtWriteFF(ps);*/

	/** Need to free up title string? **/
	if (title_str)
	    {
	    xsFree(title_str);
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
	ps = prtOpenSession(inf->DocumentFormat, fdWrite, inf->SlaveFD, PRT_OBJ_U_ALLOWBREAK);
	if (!ps) 
	    {
	    ps = prtOpenSession("text/html", fdWrite, inf->SlaveFD, PRT_OBJ_U_ALLOWBREAK);
	    if (ps) strcpy(inf->DocumentFormat, "text/html");
	    }
	if (!ps)
	    {
	    inf->Flags |= RPT_F_ERROR;
	    mssError(1,"RPT","Could not locate an appropriate content generator for type '%s'", inf->DocumentFormat);
	    fdWrite(inf->SlaveFD, "<PRE>\r\n",7,0,0);
	    mssPrintError(inf->SlaveFD);
	    fdWrite(inf->SlaveFD, "</PRE>\r\n",8,0,0);
	    fdClose(inf->SlaveFD,0);
	    thExit();
	    }

	/** Set image store location **/
	prtSetImageStore(ps, "/tmp/", "/tmp/", inf->Obj->Session, (void*(*)())objOpen, objWrite, objClose);

	/** Some output specific options **/
	if (rpt_internal_GetBool(inf, inf->Node->Data, "text_pagebreak", 1, 0) == 0)
	    prtSetSessionParam(ps, "text_pagebreak", "no");

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
	inf->SlaveFD = NULL;
	rpt_internal_Close(inf, NULL);

    thExit();
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
	fdSetOptions(inf->SlaveFD, FD_UF_WRBUF);

	/** Create a new thread to start the report generation. **/
	inf->LinkCnt++;
	if (!thCreate(rpt_internal_Generator, 0, (void*)inf))
	    {
	    inf->LinkCnt--;
	    mssError(1,"RPT","Failed to create thread for report generator");
	    fdClose(inf->MasterFD,0);
	    inf->MasterFD = NULL;
	    fdClose(inf->SlaveFD,0);
	    inf->SlaveFD = NULL;
	    return -1;
	    }

    return 0;
    }


/*** rpt_internal_GenerateName - from the structure file, obtain a value or an
 *** expression, evaluate it, and assign a filename in place of the '*' auto-
 *** name.
 ***/
int
rpt_internal_GenerateName(pRptData inf, char* prop_name)
    {
    char* strval = NULL;
    pParamObjects objlist;
    pExpression exp;
    int t, rval = 0;

	/** Get the name from the rpt file **/
	t = stGetAttrType(stLookup(inf->Node->Data, prop_name), 0);
	if (t == DATA_T_STRING)
	    {
	    if (stGetAttrValue(stLookup(inf->Node->Data, prop_name), DATA_T_STRING, POD(&strval), 0) != 0)
		return -1;
	    if (strval)
		strval = nmSysStrdup(strval);
	    }
	else
	    {
	    if (stGetAttrValue(stLookup(inf->Node->Data, prop_name), DATA_T_CODE, POD(&exp), 0) != 0)
		return -1;
	    exp = expDuplicateExpression(exp);
	    if (exp)
		{
		objlist = expCreateParamList();
		expAddParamToList(objlist, "this", inf->Obj, EXPR_O_PARENT | EXPR_O_CURRENT);
		objlist->Session = inf->Obj->Session;
		expBindExpression(exp, objlist, EXPR_F_RUNSERVER);
		rval = (expEvalTree(exp, objlist) == 0 && exp->DataType == DATA_T_STRING && !(exp->Flags & EXPR_F_NULL));
		if (rval)
		    strval = nmSysStrdup(exp->String);
		expFreeParamList(objlist);
		expFreeExpression(exp);
		}
	    if (!rval)
		return -1;
	    }

	if (strval)
	    {
	    strtcpy(inf->DownloadAs, strval, sizeof(inf->DownloadAs));
	    nmSysFree(strval);
	    }

    return 0;
    }


/*** rpt_internal_SetAutoname - from the DownloadAs name, generated by GenerateName(),
 *** replace the * in the pathname (used by autoname) with the newly generated download-
 *** as name.
 ***/
int
rpt_internal_SetAutoname(pRptData inf, char* strval)
    {

	/** Rename the * in the path **/
	if (obj_internal_RenamePath(inf->Obj->Pathname, inf->NameIndex, strval) != 0)
	    return -1;

	/** Save a copy of the pathname **/
	strtcpy(inf->Pathname, obj_internal_PathPart(inf->Obj->Pathname,0,inf->Obj->SubPtr), sizeof(inf->Pathname));
	obj_internal_PathPart(inf->Obj->Pathname, 0, 0);

    return 0;
    }


/*** rpt_internal_SetParamDefaults - activate any default values on
 *** parameters by triggering presentation hints.
 ***/
int
rpt_internal_SetParamDefaults(pRptData inf)
    {
    pRptParam rptparam = NULL;
    int i;

	/** Go through the parameter list and do an eval on each one. **/
	for(i=0; i<inf->Params.nItems; i++)
	    {
	    rptparam = (pRptParam)inf->Params.Items[i];
	    if (rpt_internal_EvalParam(rptparam, inf->ObjList, inf->ObjList->Session) < 0)
		return -1;
	    }

    return 0;
    }


/*** rpt_internal_LoadParamsFromOpen - go through the OpenCtl params
 *** load them in.
 ***/
int
rpt_internal_LoadParamsFromOpen(pRptData inf, pStruct openctl)
    {
    pRptParam rptparam = NULL;
    pStruct openctl_param;
    int i, j;

	/** Scan through object open params **/
	for(i=0;i<openctl->nSubInf;i++)
	    {
	    openctl_param = openctl->SubInf[i];

	    /** See if param is already created **/
	    for(j=0; j<inf->Params.nItems; j++)
		{
		rptparam = (pRptParam)inf->Params.Items[j];
		if (!strcmp(rptparam->Param->Name, openctl_param->Name))
		    {
		    break;
		    }
		rptparam = NULL;
		}

	    /** If it already exists, try to set its value **/
	    if (rptparam && (rptparam->Flags & RPT_PARAM_F_IN))
		{
		paramSetValueFromInfNe(rptparam->Param, openctl_param, 0, inf->ObjList, inf->ObjList->Session);
		rptparam->Flags &= ~RPT_PARAM_F_DEFAULT;
		}
	    else if (!rptparam)
		{
		/** Otherwise, create a new "in" param **/
		rptparam = (pRptParam)nmMalloc(sizeof(RptParam));
		if (!rptparam)
		    goto error;
		rptparam->Inf = inf;
		rptparam->Param = NULL;
		rptparam->Flags = RPT_PARAM_F_IN;
		xaInit(&rptparam->ValueObjs, 16);
		xaInit(&rptparam->ValueAttrs, 16);

		/** Create the Param **/
		rptparam->Param = paramCreate(openctl_param->Name);
		if (!rptparam->Param)
		    goto error;
		xaAddItem(&inf->Params, (void*)rptparam);

		/** Set value **/
		rpt_internal_SetParamValue(rptparam, DATA_T_STRING, POD(&openctl_param->StrVal), inf->ObjList, inf->ObjList->Session);
		}
	    rptparam = NULL;
	    }

	return 0;

    error:
	if (rptparam)
	    {
	    rpt_internal_FreeParam(rptparam);
	    }
	return -1;
    }


/*** rpt_internal_LoadParamsFromAttrs - some top-level attributes are
 *** treated as parameters, even in report format 2.
 ***/
int
rpt_internal_LoadParamsFromAttrs(pRptData inf)
    {
    pRptParam rptparam = NULL;
    char* attr_params[] = { "document_format", "content_type", "title", "resolution", "rpt__obkey", "rpt__obrulefile", NULL };
    char* attrname;
    int i, j;
    int t;
    ObjData od;

	/** Scan through object open params **/
	for(i=0; attr_params[i]; i++)
	    {
	    attrname = attr_params[i];

	    /** Is this attribute provided? **/
	    t = stGetAttrType(stLookup(inf->Node->Data, attrname), 0);
	    if (t > 0)
		{
		if (stGetAttrValue(stLookup(inf->Node->Data, attrname), t, &od, 0) == 0)
		    {
		    /** See if param is already created **/
		    for(j=0; j<inf->Params.nItems; j++)
			{
			rptparam = (pRptParam)inf->Params.Items[j];
			if (!strcmp(rptparam->Param->Name, attr_params[i]))
			    {
			    break;
			    }
			rptparam = NULL;
			}

		    /** Expression?  Eval if so **/
		    if (t == DATA_T_CODE)
			{
			t = rpt_internal_GetValue(inf, inf->Node->Data, attrname, DATA_T_CODE, &od, NULL, 0);
			}

		    /** If it already exists, try to set its value **/
		    if (rptparam && (rptparam->Flags & RPT_PARAM_F_IN))
			{
			rpt_internal_SetParamValue(rptparam, t, &od, inf->ObjList, inf->ObjList->Session);
			}
		    else if (!rptparam)
			{
			/** Otherwise, create a new "in" param **/
			rptparam = (pRptParam)nmMalloc(sizeof(RptParam));
			if (!rptparam)
			    goto error;
			rptparam->Inf = inf;
			rptparam->Param = NULL;
			rptparam->Flags = RPT_PARAM_F_IN;
			xaInit(&rptparam->ValueObjs, 16);
			xaInit(&rptparam->ValueAttrs, 16);

			/** Create the Param **/
			rptparam->Param = paramCreate(attrname);
			if (!rptparam->Param)
			    goto error;
			xaAddItem(&inf->Params, (void*)rptparam);

			/** Set value **/
			rpt_internal_SetParamValue(rptparam, t, &od, inf->ObjList, inf->ObjList->Session);
			}
		    rptparam = NULL;
		    }
		}
	    }

	return 0;

    error:
	if (rptparam)
	    {
	    rpt_internal_FreeParam(rptparam);
	    }
	return -1;
    }


/*** rpt_internal_LoadParamsFromReport - go through the report's
 *** "report/parameter" objects and create parameters from them.
 ***/
int
rpt_internal_LoadParamsFromReport(pRptData inf)
    {
    pStructInf report = inf->Node->Data;
    pStructInf infparam  = NULL;
    pRptParam rptparam = NULL;
    int i;
    char* dir;
	
	/** Find parameters **/
	for(i=0; i<report->nSubInf; i++)
	    {
	    infparam = (pStructInf)report->SubInf[i];
	    if (stStructType(infparam) == ST_T_SUBGROUP && !strcmp(infparam->UsrType, "report/parameter"))
		{
		/** Create the RptParam **/
		rptparam = (pRptParam)nmMalloc(sizeof(RptParam));
		if (!rptparam)
		    goto error;
		rptparam->Inf = inf;
		rptparam->Flags = 0;
		rptparam->Param = NULL;
		xaInit(&rptparam->ValueObjs, 16);
		xaInit(&rptparam->ValueAttrs, 16);

		/** Create the Param **/
		rptparam->Param = paramCreateFromInf(infparam);
		if (!rptparam->Param)
		    goto error;

		/** Param direction **/
		dir = NULL;
		stAttrValue(stLookup(infparam, "direction"), NULL, &dir, 0);
		if (dir)
		    {
		    if (!strcasecmp(dir, "in"))
			rptparam->Flags |= RPT_PARAM_F_IN;
		    else if (!strcasecmp(dir, "out"))
			rptparam->Flags |= RPT_PARAM_F_OUT;
		    else if (!strcasecmp(dir, "both"))
			rptparam->Flags |= (RPT_PARAM_F_IN | RPT_PARAM_F_OUT);
		    else
			{
			mssError(1, "RPT", "Invalid direction '%s' for parameter '%s", dir, rptparam->Param->Name);
			goto error;
			}
		    }
		else
		    {
		    rptparam->Flags |= RPT_PARAM_F_IN;
		    }

		/** Determine constituent properties supporting the Default, if
		 ** this is an "out" parameter, so the query logic can update
		 ** this param's value easily.
		 **/
		if ((rptparam->Flags & RPT_PARAM_F_OUT) && rptparam->Param->Hints && rptparam->Param->Hints->DefaultExpr)
		    {
		    if (expGetPropList((pExpression)rptparam->Param->Hints->DefaultExpr, &rptparam->ValueObjs, &rptparam->ValueAttrs) < 0)
			{
			mssError(1, "RPT", "Could not evaluate default expression dependencies for parameter '%s'", rptparam->Param->Name);
			goto error;
			}
		    }

		/** Add it. **/
		xaAddItem(&inf->Params, (void*)rptparam);
		rptparam = NULL;
		}
	    }

	return 0;

    error:
	if (rptparam)
	    {
	    rpt_internal_FreeParam(rptparam);
	    }
	return -1;
    }


/*** rpt_internal_Close - clean up an RptData structure.
 ***/
int
rpt_internal_Close(void* inf_v, pObjTrxTree* oxt)
    {
    pRptData inf = RPT(inf_v);
    pRptParam rptparam;
    int i;

	inf->LinkCnt--;
	if (inf->LinkCnt > 0) return 0;

	/** Free the objlist **/
	if (inf->ObjList)
	    expFreeParamList(inf->ObjList);

	/** Release the memory **/
	if (inf->Node)
	    inf->Node->OpenCnt --;
	if (inf->StartSem)
	    syDestroySem(inf->StartSem, SEM_U_HARDCLOSE);
	if (inf->IOSem)
	    syDestroySem(inf->IOSem, SEM_U_HARDCLOSE);
	xaDeInit(&(inf->UserDataSlots));

	/** Params **/
	for(i=0; i<inf->Params.nItems; i++)
	    {
	    rptparam = (pRptParam)inf->Params.Items[i];
	    rpt_internal_FreeParam(rptparam);
	    }
	xaDeInit(&inf->Params);

	/** Release the main structure **/
	nmFree(inf,sizeof(RptData));

    return 0;
    }


/*** rptClose - close an open file or directory.
 ***/
int
rptClose(void* inf_v, pObjTrxTree* oxt)
    {
    pRptData inf = RPT(inf_v);

	if (inf->ObfuscationSess)
	    obfCloseSession(inf->ObfuscationSess);

    	/** Is the worker thread running?? **/
	if (inf->MasterFD != NULL)
	    {
	    fdClose(inf->MasterFD, 0);
	    inf->MasterFD = NULL;
	    }

	/** close it, if linkcnt goes to 0 **/
	rpt_internal_Close(inf_v, oxt);

    return 0;
    }


/*** rptOpen - open a new report for report generation.
 ***/
void*
rptOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pRptData inf = NULL;
    int rval;
    pSnNode node = NULL;
    pStruct paramdata;
    char* ptr;
    char* endorsement_name;
    int obfuscating = 0;

    	/** This driver doesn't support sub-nodes.  Yet.  Check for that. **/
	/*if (obj->SubPtr != obj->Pathname->nElements)
	    {
	    return NULL;
	    }*/

	/** try to open it **/
	node = snReadNode(obj->Prev);

	/** If node access failed, quit out. **/
	if (!node)
	    {
	    mssError(0,"RPT","Could not open report node object");
	    goto error;
	    }

	/** Security check **/
	if (endVerifyEndorsements(node->Data, stGetObjAttrValue, &endorsement_name) < 0)
	    {
	    mssError(1,"RPT","Security check failed - endorsement '%s' required", endorsement_name);
	    goto error;
	    }

	/** Allocate the structure **/
	inf = (pRptData)nmMalloc(sizeof(RptData));
	memset(inf, 0, sizeof(RptData));
	strtcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,obj->SubPtr), sizeof(inf->Pathname));
	if (!inf)
	    goto error;
	inf->Obj = obj;
	inf->Mask = mask;
	inf->LinkCnt = 1;
	if (usrtype)
	    strtcpy(inf->DocumentFormat, usrtype, sizeof(inf->DocumentFormat));
	else
	    strcpy(inf->DocumentFormat, "text/plain");
	xaInit(&(inf->UserDataSlots), 16);
	xaInit(&inf->Params, 16);
	inf->NextUserDataSlot = 1;

	/** Content type must be application/octet-stream or more specific. **/
	rval = objIsA(inf->DocumentFormat, "application/octet-stream");
	if (rval == OBJSYS_NOT_ISA)
	    {
	    mssError(1,"RPT","Requested content type must be at least application/octet-stream");
	    goto error;
	    }
	if (rval < 0)
	    {
	    strcpy(inf->DocumentFormat,"application/octet-stream");
	    }

	/** Check format version **/
	if (stAttrValue(stLookup(node->Data,"version"),&(inf->Version),NULL,0) < 0)
	    inf->Version = 2;
	if (inf->Version < 2)
	    {
	    mssError(1, "RPT", "Report version %d not supported: must be greater than or equal to 2", inf->Version);
	    goto error;
	    }

	/** Set object params. **/
	inf->Node = node;
	inf->Node->OpenCnt++;
	inf->StartSem = syCreateSem(0,0);
	inf->IOSem = syCreateSem(0,0);

	/** Create the parameter object list, adding the report object itself as 'this' **/
	inf->ObjList = expCreateParamList();
	expAddParamToList(inf->ObjList, "this", inf->Obj, EXPR_O_PARENT | EXPR_O_CURRENT);
	inf->ObjList->Session = inf->Obj->Session;

	/** create a unique object name? **/
	if (obj->SubPtr < obj->Pathname->nElements && !strcmp(obj->Pathname->Elements[obj->SubPtr], "*") && (obj->Mode & OBJ_O_AUTONAME))
	    obj->SubCnt = 2;
	else
	    obj->SubCnt = 1;
	inf->NameIndex = obj->SubPtr - 1 + obj->SubCnt - 1;

	/** Load in parameters from node **/
	if (rpt_internal_LoadParamsFromReport(inf) < 0)
	    goto error;

	/** Top level attribute based parameters **/
	if (rpt_internal_LoadParamsFromAttrs(inf) < 0)
	    goto error;

	/** Lookup parameter/value data in the openctl **/
	if (obj->Pathname->OpenCtl[inf->NameIndex])
	    {
	    paramdata = obj->Pathname->OpenCtl[inf->NameIndex];

	    /** Load in parameters from open operation **/
	    if (rpt_internal_LoadParamsFromOpen(inf, paramdata) < 0)
		goto error;

	    /** Obfuscation **/
	    if (rpt_internal_GetParamValue(inf, "rpt__obkey", DATA_T_STRING, POD(&ptr), inf->ObjList, inf->ObjList->Session) == 0)
		{
		obfuscating = 1;
		strtcpy(inf->ObKey, ptr, sizeof(inf->ObKey));
		}
	    if (rpt_internal_GetParamValue(inf, "rpt__obrulefile", DATA_T_STRING, POD(&ptr), inf->ObjList, inf->ObjList->Session) == 0)
		{
		strtcpy(inf->ObRulefile, ptr, sizeof(inf->ObRulefile));
		}
	    }

	/** Obfuscating data? (for test/demo purposes) **/
	if (obfuscating)
	    inf->ObfuscationSess = obfOpenSession(inf->Obj->Session, inf->ObRulefile, inf->ObKey);

	/** Lookup forced content type param **/
	if (rpt_internal_GetParamValue(inf, "document_format", DATA_T_STRING, POD(&ptr), inf->ObjList, inf->ObjList->Session) == 0)
	    strtcpy(inf->DocumentFormat, ptr, sizeof(inf->DocumentFormat));

	/** Alternate content type? **/
	if (rpt_internal_GetParamValue(inf, "content_type", DATA_T_STRING, POD(&ptr), inf->ObjList, inf->ObjList->Session) == 0)
	    strtcpy(inf->ContentType, ptr, sizeof(inf->ContentType));
	else
	    strtcpy(inf->ContentType, inf->DocumentFormat, sizeof(inf->ContentType));

	/** Hint to OSML to not automatically cascade the open operation **/
	if (rpt_internal_GetBool(inf, inf->Node->Data, "force_leaf", 1, 0) == 1)
	    {
	    inf->Flags |= RPT_F_FORCELEAF;
	    }

	return (void*)inf;

    error:
	if (inf)
	    rptClose(inf, NULL);
	return NULL;
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
    pSnNode node;

    	/** This driver doesn't support sub-nodes.  Yet.  Check for that. **/
	if (obj->SubPtr != obj->Pathname->nElements)
	    {
	    return -1;
	    }

	/** Determine node path **/
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
    int t;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname, "inner_type") ||
	    !strcmp(attrname,"outer_type") || !strcmp(attrname, "cx__download_as"))
		return DATA_T_STRING;

	/** Current page number is a predefined one for reports **/
	if (!strcmp(attrname,"page")) return DATA_T_INTEGER;

	/** Report Attribute **/
	if ((t = rpt_internal_GetParamType(inf, attrname, inf->ObjList, inf->ObjList->Session)) >= 0)
	    {
	    return t;
	    }

	/** Report attribute valid but no type available? **/
	if (rpt_internal_GetParam(inf, attrname))
	    {
	    return DATA_T_UNAVAILABLE;
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
    int rval;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"RPT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}

	    /** Generate filename if opened as /path/to/reportname.rpt/{asterisk}  **/
	    if (inf->Obj->SubCnt == 2 && inf->Obj->Pathname->Elements[inf->NameIndex][0] == '*')
		{
		if (rpt_internal_GenerateName(inf, "filename") == 0)
		    rpt_internal_SetAutoname(inf, inf->DownloadAs);
		}

	    /* val->String = inf->Node->Data->Name;*/
	    val->String = obj_internal_PathPart(inf->Obj->Pathname, inf->NameIndex, 0);
	    return 0;
	    }

	if (!strcmp(attrname, "cx__download_as"))
	    {
	    if (!*(inf->DownloadAs))
		{
		rpt_internal_GenerateName(inf, "filename");
		if (*inf->DownloadAs && inf->Obj->SubCnt == 2 && inf->Obj->Pathname->Elements[inf->NameIndex][0] == '*')
		    {
		    rpt_internal_SetAutoname(inf, inf->DownloadAs);
		    }
		}
	    val->String = inf->DownloadAs;
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

	/** Report Attribute? **/
	if ((rval = rpt_internal_GetParamValue(inf, attrname, datatype, val, inf->ObjList, inf->ObjList->Session)) >= 0)
	    {
	    return rval;
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
    pRptParam rptparam;

	/** Get the next one from the list of parameters **/
	if (inf->AttrID < inf->Params.nItems)
	    {
	    rptparam = (pRptParam)inf->Params.Items[inf->AttrID];
	    inf->AttrID += 1;
	    return rptparam->Param->Name;
	    }

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
    pRptParam rptparam;

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

	/** Report Attribute? **/
	rptparam = rpt_internal_GetParam(inf, attrname);
	if (rptparam)
	    {
	    if (rptparam->Flags & RPT_PARAM_F_IN)
		{
		if (rpt_internal_SetParamValue(rptparam, datatype, val, inf->ObjList, inf->ObjList->Session) >= 0)
		    return 0;
		}
	    else
		{
		mssError(1, "RPT", "Cannot set readonly attribute '%s'", attrname);
		return -1;
		}
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

int
rptInfo(void* inf_v, pObjectInfo info)
    {
    pRptData inf = RPT(inf_v);

	info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ | 
	    OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CAN_HAVE_CONTENT | 
	    OBJ_INFO_F_HAS_CONTENT );

	if (inf->Flags & RPT_F_FORCELEAF)
	    info->Flags |= OBJ_INFO_F_FORCED_LEAF;

	return 0;
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
	xaInit(&RPT_INF.ChartDrivers,16);

	/** Chart drivers **/
	rpt_internal_RegisterChartDriver("bar", rpt_internal_BarChart_PreProcess, rpt_internal_BarChart_Setup, rpt_internal_BarChart_Generate);
	rpt_internal_RegisterChartDriver("line", rpt_internal_LineChart_PreProcess, rpt_internal_LineChart_Setup, rpt_internal_LineChart_Generate);
	rpt_internal_RegisterChartDriver("pie", rpt_internal_PieChart_PreProcess, rpt_internal_PieChart_Setup, rpt_internal_PieChart_Generate);

	/** Setup the structure **/
	strcpy(drv->Name,"RPT - Reporting Translation Driver");
	drv->Capabilities = OBJDRV_C_DOWNLOADAS;
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
	drv->Info = rptInfo;
	/*drv->PresentationHints = rptPresentationHints*/;

	nmRegister(sizeof(RptData),"RptData");
	nmRegister(sizeof(RptQuery),"RptQuery");
	nmRegister(sizeof(RptSource),"RptSource");
	nmRegister(sizeof(RptSourceAgg),"RptSourceAgg");
	nmRegister(sizeof(RptSession),"RptSession");
	nmRegister(sizeof(PrintDriver),"PrintDriver");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

