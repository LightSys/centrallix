#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	jsvm.h,jsvm.c                				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 19, 2001 					*/
/* Description:	This module provides the JavaScript VM and compiler /	*/
/*		generator functionality.				*/
/************************************************************************/



/*** JavaScript data value ***/
typedef struct _JD
    {
    unsigned char   Type;	/* DATA_T_xxx */
    unsigned char   Flags;	/* JS_D_F_xxx */
    union
        {
	int	    Integer;
	pXString    String;	/* null if not a string */
	DateTime    DateTime;
	MoneyType   Money;
	double	    Double;
	void*	    Code;	/* function reference, pJSCode pointer */
	}
	Value;
    struct
	{
	XArray	    Keys;	/* array of jsdata key names */
	XArray	    Values;	/* array of jsdata values */
	}
	Array;
    struct _JD*	    OnRead;	/* eval this on data read */
    struct _JD*	    OnModify;	/* eval this to allow/disallow mod. */
    int		    (*OnReadFn)(); /* call this on data read */
    int		    (*OnModifyFn)(); /* call this on data mod */
    }
    JSData, *pJSData;

#define JS_D_F_NULL	1


/*** Compiled JavaScript Tree Structure ***/
typedef struct _JC
    {
    unsigned char   NodeType;	/* JS_C_T_xxx */
    unsigned char   DataType;	/* DATA_T_xxx */
    unsigned char   Flags;	/* JS_C_F_xxx */
    JSData	    Value;
    union
	{
	JSData	    Selection;	/* for Selection nodes */
	pXString    VarName;	/* for variable references or definition */
	pXString    FnName;	/* for function calls */
	struct
	    {
	    pXString	Name;	/* fn definition name */
	    XArray	Params;	/* fn definition param list */
	    XArray	Defval;	/* fn definition default values */
	    }
	    FnDef;
	struct
	    {
	    struct _JC*	Init;	/* loop init condition */
	    struct _JC*	Pre;	/* loop pre-exec condition */
	    struct _JC*	Post;	/* loop post-exec condition */
	    struct _JC*	Iter;	/* loop exec action */
	    }
	    Loopinfo;
	struct
	    {
	    struct _JC*	VarName; /* variable to assign */
	    struct _JC*	Objref;	/* object to iter through */
	    }
	    Iterinfo;
	struct _JC*	Condition; /* if/else condition */
	struct _JC*	RetVal;	/* for return statements */
	struct
	    {
	    struct _JC*	SwitchExp; /* expression for switch */
	    XArray	CaseExps; /* expressions for each case, matches SubNodes for blocks */
	    }
	    Switchinfo;
	}
	NodeInfo;
    XArray	    SubNodes;	/* pJSCode xarray */
    }
    JSCode, *pJSCode;


#define	JS_C_T_CONSTANT	    1	/* a constant value */
#define JS_C_T_VARREF	    2	/* a variable name */
#define JS_C_T_SELECTION    3	/* selecting an array element or attribute */
#define JS_C_T_FNCALL	    4	/* function call */
#define JS_C_T_FUNCTION	    5	/* function definition */
#define JS_C_T_VARIABLE	    6	/* variable definition */
#define JS_C_T_BIOP	    7	/* binary operator */
#define JS_C_T_PREOP	    8	/* unary operator (prefix) */
#define JS_C_T_POSTOP	    9	/* unary operator (postfix) */
#define JS_C_T_BLOCK	    10	/* series of code elements (expr, ctl, etc) */
#define JS_C_T_LOOP	    11	/* normal loop (while, for, do) */
#define JS_C_T_ITER	    12	/* array/value loop (for x in y) */
#define JS_C_T_COND	    13	/* if/else condition */
#define JS_C_T_RETURN	    14	/* force return from function */
#define	JS_C_T_SWITCH	    15	/* switch/case statement */
#define JS_C_T_CONTINUE	    16	/* continue statement */
#define JS_C_T_BREAK	    17	/* break statement */
#define JS_C_T_DEFAULT	    18	/* default case in a switch */
#define JS_C_T_QUESCOLON    19	/* ? : construct */


/*** Script Parser definition. ***/
typedef struct _JL
    {
    char	Name[32];
    char	Description[256];
    int		(*Compile)();
    int		(*Generate)();
    }
    JSVMScriptLanguage, *pJSVMScriptLanguage;


/*** jsvm data item function handler list ***/
typedef struct _JF
    {
    int		(*Get)();
    int		(*Set)();
    int		(*FnCall)();
    int		(*Add)();
    int		(*Del)();
    int		(*DelSelf)();
    }
    JSVMDataFunctions, *pJSVMDataFunctions;


/*** jsvm symbol ***/
typedef struct _JS
    {
    char	SymName[64];
    pJSData	Value;
    pJSVMDataFunctions FnList;
    int		Flags;		/* JS_SYM_F_xxx */
    int		RefCount;
    }
    JSVMSymbol, *pJSVMSymbol;


/*** jsvm symbol table definition ***/
typedef struct _JT
    {
    XHashTable	SymbolHash;
    XArray	Symbols;
    }
    JSVMSymbolTable, *pJSVMSymbolTable;


/*** JSVM main methods ***/
int jsvmInitialize();
int jsvmRegisterLanguage(pJSVMScriptLanguage l);

/*** Compiler functions ***/
int jsvmCompile(char* language, pLxSession lexer_session, pJSCode* code, pJSVMSymbolTable* symtab);
int jsvmFreeCode(pJSCode code);

/*** Generator functions ***/
int jsvmGenerate(char* language, int (*write_fn)(), int write_arg, pJSCode code, pJSVMSymbolTable symtab);

/*** Executive functions ***/
pJSVMExecEnv jsvmCreateExecEnv(pJSVMSymbolTable symtab);
int jsvmExec(pJSCode code, pJSVMExecEnv env);
int jsvmFreeExecEnv(pJSVMExecEnv env);

/*** Symbol Table Functions ***/
pJSVMSymbolTable jsvmCreateSymbolTable();
int jsvmAddStaticSymbol(pJSVMSymbolTable symtab, char* symname, pJSData symvalue);
int jsvmAddDynamicSymbol(pJSVMSymbolTable symtab, char* symname, pJSVMDataFunctions fn_list, int flags);
int jsvmRemoveSymbol(pJSVMSymbolTable symtab, char* symname);
int jsvmSetSymbolValue(pJSVMSymbolTable symtab, char* symname, pJSData symvalue); /* only for static symbols */
int jsvmFreeSymbolTable(pJSVMSymbolTable symtab);


/*** JSVM error codes ***/
#define	JSVM_E_ERROR		(-1)	/* a nondescript ;) error occurred */
#define JSVM_E_NOSYM		(-2)	/* no such symbol */
#define JSVM_E_NOTAFUNCTION	(-3)	/* function call to something not a function */
#define JSVM_E_ISAFUNCTION	(-4)	/* attempt to get/set the value of a function */
#define JSVM_E_SYMEXISTS	(-5)	/* symbol already exists */
#define JSVM_E_PARSE		(-6)	/* generic parse error */

