/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module:	test_obj.c                                              */
/* Author:	Greg Beeley                                             */
/* Date:	November 1998                                           */
/*									*/
/* Description:	This module provides command-line access to the OSML	*/
/*		for testing purposes.  It does not provide a network	*/
/*		interface and does not include the DHTML generation	*/
/*		subsystem when compiled.				*/
/*									*/
/*		THIS MODULE IS **NOT** SECURE AND SHOULD NEVER BE USED	*/
/*		IN PRODUCTION WHERE THE DEVELOPER IS NOT CONTROLLING	*/
/*		ALL ASPECTS OF INPUTS AND DATA BEING HANDLED.  FIXME ;)	*/
/************************************************************************/

#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#ifndef CENTRALLIX_CONFIG
 #define CENTRALLIX_CONFIG /usr/local/etc/centrallix.conf
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "application.h"
#include "centrallix.h"
#include "cxlib/expect.h"
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "cxlib/strtcpy.h"
#include "cxlib/util.h"
#include "cxlib/warn.h"
#include "cxss/cxss.h"
#include "obfuscate.h"
#include "obj.h"

/*** The readline.h files assume some other .h files were included first,
 *** so they should be included last.
 ***/
#ifdef HAVE_READLINE
 /* Some versions of readline get upset if HAVE_CONFIG_H is defined! */
 #ifdef HAVE_CONFIG_H
  #undef HAVE_CONFIG_H
  #include <readline/readline.h>
  #define HAVE_CONFIG_H 1
 #else
  #include <readline/readline.h>
 #endif
 #include <readline/history.h>
#endif


void* my_ptr;
unsigned long ticks_last_tab=0;
pObjSession s;

struct
    {
    char		UserName[CX_USERNAME_SIZE];
    char		Password[CX_PASSWORD_SIZE];
    char		PasswordFile[256];
    char		CmdFile[256];
    pFile		Output;
    char		OutputFilename[256];
    char		Command[1024];
    unsigned int	WaitSecs;
    bool		StopOnError;
    pObfSession		ObfuscationSession;
    char		ObfRuleFile[256];
    char		ObfKey[256];
    }
    TESTOBJ;

#define BUFF_SIZE 1024

#define CSV_MAX_ATTRS	640

typedef struct
    {
    char *buffer;
    int buflen;
    } WriteStruct, *pWriteStruct;

void set_output(pFile output)
    {
    TESTOBJ.Output = output;

    // redirect the multiquery print statements as well
    mqRedirectPrint(output);

    /** Redirect test suite required output **/
    cxRedirectTestOutput(output);
    }

/*** text_gen_callback - appends text from expGenerateText() to a buffer,
 *** leaving room for a null terminator.
 ***
 *** @param dst The buffer to append to.
 *** @param src The text to append.
 *** @param len The length of the text.
 *** @param unused_1 Unused.
 *** @param unused_2 Unused.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
text_gen_callback(pWriteStruct dst, char* src, int len, int unused_1, int unused_2)
    {
	char* const new_buffer = realloc(dst->buffer, dst->buflen + len + 1);
	if (UNLIKELY(new_buffer == NULL))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to grow the expression text buffer to %d bytes.",
		dst->buflen + len + 1
	    );
	    return -1;
	    }
	dst->buffer = new_buffer;

	memcpy(dst->buffer + dst->buflen, src, len);
	dst->buflen += len;

    return 0;
    }

void
setup_test_ids(char* id_str)
    {
    char* endptr;
    uintptr_t id;

	/** Remove all existing test IDs **/
	cxTestEnable(CX_TEST_NONE);

	if (*id_str)
	    {
	    /** Setup tests **/
	    CxGlobals.Flags |= CX_F_SHOWTESTOUTPUT;

	    /** Work through the comma and/or space separated list **/
	    while(*id_str)
		{
		id = strtol(id_str, &endptr, 0);
		if (UNLIKELY(endptr == id_str))
		    {
		    fprintf(stderr, "Warning: Failed to set up tests for '%s'.\n", endptr);
		    break;
		    }
		cxTestEnable(id);
		while(*endptr == ',' || *endptr == ' ') endptr++;
		id_str = endptr;
		}
	    }

    return;
    }

/*** printExpression - prints an expression as CXSQL text, followed by a
 *** space, to the output file.
 ***
 *** @param exp The expression to print, or NULL to print nothing.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
printExpression(pExpression exp)
    {
    bool successful = false;
    WriteStruct text = {.buffer = NULL, .buflen = 0};
    pParamObjects params = NULL;

	if (exp == NULL) return 0;
	if (UNLIKELY(TESTOBJ.Output == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to find an open output file.");
	    goto end;
	    }

	/** Generate the text. **/
	text.buffer = malloc(1);
	if (UNLIKELY(text.buffer == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to allocate the expression text buffer.");
	    goto end;
	    }
	params = expCreateParamList();
	if (UNLIKELY(params == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to create an expression parameter list.");
	    goto end;
	    }
	if (UNLIKELY(expAddParamToList(params, "this", NULL, EXPR_O_CURRENT) < 0))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to add parameter \"this\" to the expression parameter list."
	    );
	    goto end;
	    }
	if (UNLIKELY(expGenerateText(exp, params, text_gen_callback, &text, '"', "cxsql", 0) < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to generate CXSQL text for an expression.");
	    goto end;
	    }
	text.buffer[text.buflen] = '\0';

	/** Print it. **/
	if (UNLIKELY(fdPrintf(TESTOBJ.Output, "%s ", text.buffer) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to print expression \"%s\".", text.buffer);
	    goto end;
	    }

	successful = true;

    end:
	/** Clean up. **/
	if (LIKELY(params != NULL)) warnNeg(expFreeParamList(params));
	if (LIKELY(text.buffer != NULL)) free(text.buffer);

	return (successful) ? 0 : -1;
    }



/*** testobj_show_hints - prints the presentation hints of an attribute.
 ***
 *** @param obj The object that has the attribute.
 *** @param attrname The name of the attribute.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
testobj_show_hints(pObject obj, char* attrname)
    {
    bool successful = false;
    pObjPresentationHints hints = NULL;

	if (UNLIKELY(TESTOBJ.Output == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to find an open output file.");
	    goto end;
	    }

	hints = objPresentationHints(obj, attrname);
	if (UNLIKELY(hints == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to get the presentation hints.");
	    goto end;
	    }

	/** Expressions. **/
	bool print_ok = true;
	print_ok &= (fdPrintf(TESTOBJ.Output, "Presentation Hints for \"%s\":\n", attrname) >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  Constraint   : ") >= 0);
	if (UNLIKELY(printExpression(hints->Constraint) < 0)) goto end;
	print_ok &= (fdPrintf(TESTOBJ.Output, "\n") >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  DefaultExpr  : ") >= 0);
	if (UNLIKELY(printExpression(hints->DefaultExpr) < 0)) goto end;
	print_ok &= (fdPrintf(TESTOBJ.Output, "\n") >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  MinValue     : ") >= 0);
	if (UNLIKELY(printExpression(hints->MinValue) < 0)) goto end;
	print_ok &= (fdPrintf(TESTOBJ.Output, "\n") >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  MaxValue     : ") >= 0);
	if (UNLIKELY(printExpression(hints->MaxValue) < 0)) goto end;
	print_ok &= (fdPrintf(TESTOBJ.Output, "\n") >= 0);

	/** Enum. **/
	print_ok &= (fdPrintf(TESTOBJ.Output, "  EnumList     : ") >= 0);
	for (int i = 0; i < hints->EnumList.nItems; i++)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "    %s\n", (char*)xaGetItem(&hints->EnumList, i)) >= 0);
	    }
	print_ok &= (fdPrintf(TESTOBJ.Output, "\n") >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  EnumQuery    : %s\n", hints->EnumQuery) >= 0);

	/** Formatting. **/
	print_ok &= (fdPrintf(TESTOBJ.Output, "  Format       : %s\n", hints->Format) >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  VisualLength : %i\n", hints->VisualLength) >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  VisualLength2: %i\n", hints->VisualLength2) >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  BitmaskRO    : ") >= 0);
	for (int i = 0; i < 32; i++)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "%i", hints->BitmaskRO >> (31 - i) & 0x01) >= 0);
	    }
	print_ok &= (fdPrintf(TESTOBJ.Output, "\n") >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  Style        : %i\n", hints->Style) >= 0);

	/** Grouping. **/
	print_ok &= (fdPrintf(TESTOBJ.Output, "  GroupID      : %i\n", hints->GroupID) >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  GroupName    : %s\n", hints->GroupName) >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "  FriendlyName : %s\n", hints->FriendlyName) >= 0);
	if (UNLIKELY(!print_ok))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write to output file \"%s\".",
		TESTOBJ.OutputFilename
	    );
	    goto end;
	    }

	successful = true;

    end:
	if (UNLIKELY(!successful))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to show the presentation hints for \"%s\".",
		attrname
	    );
	    }

	/** Clean up. **/
	if (LIKELY(hints != NULL)) warnNeg(objFreeHints(hints));

	return (successful) ? 0 : -1;
    }



/*** testobj_show_attr - prints a line with an attribute's value and its
 *** presentation hints, or "(no such attribute)" if it is missing.
 ***
 *** @param obj The object that has the attribute.
 *** @param attrname The name of the attribute.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
testobj_show_attr(pObject obj, char* attrname)
    {
    bool successful = false;
    pObjPresentationHints hints = NULL;

	if (UNLIKELY(TESTOBJ.Output == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to find an open output file.");
	    goto end;
	    }

	/** Show missing attributes, since callers list optional ones. **/
	const int type = objGetAttrType(obj, attrname);
	if (type < 0)
	    {
	    mssClearError(); /* Error handled. */

	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, "  %20.20s: (no such attribute)\n", attrname) < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write to output file \"%s\".",
		    TESTOBJ.OutputFilename
		);
		goto end;
		}
	    return 0;
	    }

	/** Print the value. **/
	bool print_ok = true;
	int value_status = -1;
	switch (type)
	    {
	    case DATA_T_INTEGER:
		{
		int intval = 0;
		value_status = objGetAttrValue(obj, attrname, DATA_T_INTEGER, POD(&intval));
		if (value_status == 0) print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: %d", attrname, intval) >= 0);
		break;
		}

	    case DATA_T_STRING:
		{
		char* stringval = NULL;
		value_status = objGetAttrValue(obj, attrname, DATA_T_STRING, POD(&stringval));
		if (value_status == 0) print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: \"%s\"", attrname, stringval) >= 0);
		break;
		}

	    case DATA_T_BINARY:
		{
		Binary bn = {0};
		value_status = objGetAttrValue(obj, attrname, DATA_T_BINARY, POD(&bn));
		if (value_status != 0) break;
		print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s:  %d bytes: ", attrname, bn.Size) >= 0);
		for (int i = 0; i < bn.Size; i++)
		    {
		    print_ok &= (fdPrintf(TESTOBJ.Output, "%2.2x ", bn.Data[i]) >= 0);
		    }
		break;
		}

	    case DATA_T_DATETIME:
		{
		pDateTime dt = NULL;
		value_status = objGetAttrValue(obj, attrname, DATA_T_DATETIME, POD(&dt));
		if (value_status == 0 && dt == NULL) value_status = 1;
		if (value_status != 0) break;
		print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: %2.2d-%2.2d-%4.4d %2.2d:%2.2d:%2.2d",
		    attrname, dt->Part.Month + 1, dt->Part.Day + 1, dt->Part.Year + 1900,
		    dt->Part.Hour, dt->Part.Minute, dt->Part.Second) >= 0);
		break;
		}

	    case DATA_T_DOUBLE:
		{
		double dblval = 0.0;
		value_status = objGetAttrValue(obj, attrname, DATA_T_DOUBLE, POD(&dblval));
		if (value_status == 0) print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: %g", attrname, dblval) >= 0);
		break;
		}

	    case DATA_T_MONEY:
		{
		pMoneyType m = NULL;
		value_status = objGetAttrValue(obj, attrname, DATA_T_MONEY, POD(&m));
		if (value_status == 0 && m == NULL) value_status = 1;
		if (value_status != 0) break;
		print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: %s", attrname, objDataToStringTmp(DATA_T_MONEY, m, 0)) >= 0);
		break;
		}

	    case DATA_T_INTVEC:
		{
		pIntVec iv = NULL;
		value_status = objGetAttrValue(obj, attrname, DATA_T_INTVEC, POD(&iv));
		if (value_status == 0 && iv == NULL) value_status = 1;
		if (value_status != 0) break;
		print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: ", attrname) >= 0);
		for (int i = 0; i < iv->nIntegers; i++)
		    {
		    print_ok &= (fdPrintf(TESTOBJ.Output, "%d%s", iv->Integers[i], (i == iv->nIntegers - 1) ? "" : ",") >= 0);
		    }
		break;
		}

	    case DATA_T_STRINGVEC:
		{
		pStringVec sv = NULL;
		value_status = objGetAttrValue(obj, attrname, DATA_T_STRINGVEC, POD(&sv));
		if (value_status == 0 && sv == NULL) value_status = 1;
		if (value_status != 0) break;
		print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: ", attrname) >= 0);
		for (int i = 0; i < sv->nStrings; i++)
		    {
		    print_ok &= (fdPrintf(TESTOBJ.Output, "\"%s\"%s", sv->Strings[i], (i == sv->nStrings - 1) ? "" : ",") >= 0);
		    }
		break;
		}

	    default:
		{
		value_status = 0;
		print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: <unknown type>", attrname) >= 0);
		break;
		}
	    }
	if (value_status == 1) print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: NULL", attrname) >= 0);
	else if (value_status != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s: Error", attrname) >= 0);

	/** Print the hints. **/
	hints = objPresentationHints(obj, attrname);
	if (UNLIKELY(hints == NULL))
	    {
	    warnNeg(fdPrintf(TESTOBJ.Output, "\n"));
	    mssError(1, "TESTOBJ", "Failed to get the presentation hints.");
	    goto end;
	    }
	print_ok &= (fdPrintf(TESTOBJ.Output, " [Hints: ") >= 0);
	if (hints->EnumQuery != NULL) print_ok &= (fdPrintf(TESTOBJ.Output, "EnumQuery=[%s] ", hints->EnumQuery) >= 0);
	if (hints->Format != NULL) print_ok &= (fdPrintf(TESTOBJ.Output, "Format=[%s] ", hints->Format) >= 0);
	if (hints->AllowChars != NULL) print_ok &= (fdPrintf(TESTOBJ.Output, "AllowChars=[%s] ", hints->AllowChars) >= 0);
	if (hints->BadChars != NULL) print_ok &= (fdPrintf(TESTOBJ.Output, "BadChars=[%s] ", hints->BadChars) >= 0);
	if (hints->Length != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "Length=%d ", hints->Length) >= 0);
	if (hints->VisualLength != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "VisualLength=%d ", hints->VisualLength) >= 0);
	if (hints->VisualLength2 != 1) print_ok &= (fdPrintf(TESTOBJ.Output, "VisualLength2=%d ", hints->VisualLength2) >= 0);
	if (hints->Style != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "Style=%d ", hints->Style) >= 0);
	if (hints->StyleMask != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "StyleMask=%d ", hints->StyleMask) >= 0);
	if (hints->GroupID != -1) print_ok &= (fdPrintf(TESTOBJ.Output, "GroupID=%d ", hints->GroupID) >= 0);
	if (hints->GroupName != NULL) print_ok &= (fdPrintf(TESTOBJ.Output, "GroupName=[%s] ", hints->GroupName) >= 0);
	if (hints->FriendlyName != NULL) print_ok &= (fdPrintf(TESTOBJ.Output, "FriendlyName=[%s] ", hints->FriendlyName) >= 0);
	if (hints->Constraint != NULL)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "Constraint=") >= 0);
	    if (UNLIKELY(printExpression(hints->Constraint) < 0)) goto end;
	    }
	if (hints->DefaultExpr != NULL)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "DefaultExpr=") >= 0);
	    if (UNLIKELY(printExpression(hints->DefaultExpr) < 0)) goto end;
	    }
	if (hints->MinValue != NULL)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "MinValue=") >= 0);
	    if (UNLIKELY(printExpression(hints->MinValue) < 0)) goto end;
	    }
	if (hints->MaxValue != NULL)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "MaxValue=") >= 0);
	    if (UNLIKELY(printExpression(hints->MaxValue) < 0)) goto end;
	    }
	print_ok &= (fdPrintf(TESTOBJ.Output, "]\n") >= 0);

	/** Fail now that the line is finished. **/
	if (UNLIKELY(value_status != 0 && value_status != 1))
	    {
	    mssError(0, "TESTOBJ", "Failed to get the %s value.", objTypeToStr(type));
	    goto end;
	    }
	if (UNLIKELY(!print_ok))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write to output file \"%s\".",
		TESTOBJ.OutputFilename
	    );
	    goto end;
	    }

	successful = true;

    end:
	if (UNLIKELY(!successful))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to show attribute \"%s\".",
		attrname
	    );
	    }

	/** Clean up. **/
	if (LIKELY(hints != NULL)) warnNeg(objFreeHints(hints));

	return (successful) ? 0 : -1;
    }

/*** testobj_show_info - prints an object's info flags and, if known, its
 *** subobject count.  Prints nothing if no flags are set, and only a
 *** warning if the info is unavailable.
 ***
 *** @param obj The object to show.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
testobj_show_info(pObject obj)
    {
	if (UNLIKELY(TESTOBJ.Output == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to find an open output file.");
	    return -1;
	    }

	const pObjectInfo info = objInfo(obj);
	if (UNLIKELY(info == NULL))
	    {
	    mssWarnError("Failed to get info for \"%s\".", objGetPathname(obj));
	    return 0;
	    }
	if (info->Flags == 0) return 0;

	/** Flags. **/
	bool print_ok = true;
	print_ok &= (fdPrintf(TESTOBJ.Output, "Flags: ") >= 0);
	if ((info->Flags & OBJ_INFO_F_NO_SUBOBJ) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "no_subobjects ") >= 0);
	if ((info->Flags & OBJ_INFO_F_HAS_SUBOBJ) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "has_subobjects ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CAN_HAVE_SUBOBJ) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "can_have_subobjects ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CANT_HAVE_SUBOBJ) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "cant_have_subobjects ") >= 0);
	if ((info->Flags & OBJ_INFO_F_SUBOBJ_CNT_KNOWN) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "subobject_cnt_known ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CAN_ADD_ATTR) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "can_add_attrs ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CANT_ADD_ATTR) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "cant_add_attrs ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CAN_SEEK_FULL) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "can_seek_full ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CAN_SEEK_REWIND) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "can_seek_rewind ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CANT_SEEK) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "cant_seek ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CAN_HAVE_CONTENT) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "can_have_content ") >= 0);
	if ((info->Flags & OBJ_INFO_F_CANT_HAVE_CONTENT) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "cant_have_content ") >= 0);
	if ((info->Flags & OBJ_INFO_F_HAS_CONTENT) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "has_content ") >= 0);
	if ((info->Flags & OBJ_INFO_F_NO_CONTENT) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "no_content ") >= 0);
	if ((info->Flags & OBJ_INFO_F_SUPPORTS_INHERITANCE) != 0) print_ok &= (fdPrintf(TESTOBJ.Output, "supports_inheritance ") >= 0);
	print_ok &= (fdPrintf(TESTOBJ.Output, "\n") >= 0);

	/** Subobject count. **/
	if ((info->Flags & OBJ_INFO_F_SUBOBJ_CNT_KNOWN) != 0)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "Subobject count: %d\n", info->nSubobjects) >= 0);
	    }
	if (UNLIKELY(!print_ok))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write the info for \"%s\" to output file \"%s\".",
		objGetPathname(obj), TESTOBJ.OutputFilename
	    );
	    return -1;
	    }

    return 0;
    }

/*** testobj_show_attrs - prints the common attributes of an object, then
 *** all of its attributes.  Prints a warning for each attribute that fails.
 ***
 *** @param obj The object to show.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
testobj_show_attrs(pObject obj)
    {
    char* const common_attrs[] = {"outer_type", "inner_type", "content_type", "name", "annotation", "last_modification"};

	if (UNLIKELY(TESTOBJ.Output == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to find an open output file.");
	    return -1;
	    }
	if (UNLIKELY(fdPrintf(TESTOBJ.Output, "Attributes:\n") < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write to output file \"%s\".",
		TESTOBJ.OutputFilename
	    );
	    return -1;
	    }

	for (size_t i = 0; i < sizeof(common_attrs) / sizeof(common_attrs[0]); i++)
	    {
	    if (UNLIKELY(testobj_show_attr(obj, common_attrs[i]) < 0))
		{
		mssWarnError(
		    "Failed to show attribute \"%s\" of \"%s\".",
		    common_attrs[i], objGetPathname(obj)
		);
		}
	    }
	for (char* attrname = objGetFirstAttr(obj); attrname != NULL; attrname = objGetNextAttr(obj))
	    {
	    if (UNLIKELY(testobj_show_attr(obj, attrname) < 0))
		{
		mssWarnError(
		    "Failed to show attribute \"%s\" of \"%s\".",
		    attrname, objGetPathname(obj)
		);
		}
	    }

    return 0;
    }

/*** testobj_show_methods - prints the names of an object's methods.
 ***
 *** @param obj The object.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
testobj_show_methods(pObject obj)
    {
    bool print_ok = true;

	print_ok &= (fdPrintf(TESTOBJ.Output, "Methods:\n") >= 0);
	char* methodname = objGetFirstMethod(obj);
	if (methodname == NULL) print_ok &= (fdPrintf(TESTOBJ.Output, "  (no methods)\n") >= 0);
	while (methodname != NULL)
	    {
	    print_ok &= (fdPrintf(TESTOBJ.Output, "  %20.20s()\n", methodname) >= 0);
	    methodname = objGetNextMethod(obj);
	    }
	if (UNLIKELY(!print_ok))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write the methods of \"%s\" to output file \"%s\".",
		objGetPathname(obj), TESTOBJ.OutputFilename
	    );
	    return -1;
	    }

    return 0;
    }

/*** testobj_show_content - prints an object's content.  Prints only a
 *** warning if the content is unreadable, as for objects without content.
 ***
 *** @param obj The object to read.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
testobj_show_content(pObject obj)
    {
    char sbuf[255];
    int cnt;

	if (UNLIKELY(TESTOBJ.Output == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to find an open output file.");
	    return -1;
	    }

	while ((cnt = objRead(obj, sbuf, sizeof(sbuf), 0, 0)) > 0)
	    {
	    if (UNLIKELY(fdWrite(TESTOBJ.Output, sbuf, cnt, 0, 0) < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write the content of \"%s\".",
		    objGetPathname(obj)
		);
		return -1;
		}
	    }
	if (UNLIKELY(cnt < 0))
	    {
	    mssWarnError(
		"Failed to read the content of \"%s\".",
		objGetPathname(obj)
	    );
	    }

    return 0;
    }

/*** testobj_i_hasChildren - checks whether an object has any subobjects.
 ***
 *** @param path The path of the object.
 *** @param has_children Set to whether the object has subobjects.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_hasChildren(char* path, bool* has_children)
    {
    int rval = -1;
    pObject obj = NULL;
    pObjQuery query = NULL;
    pObject child = NULL;

	/** Handle edge cases. **/
	if (UNLIKELY(path == NULL))
	    {
	    mssError(1, "TESTOBJ", "testobj_i_hasChildren(): path pointer cannot be NULL.");
	    goto end;
	    }
	if (UNLIKELY(has_children == NULL))
	    {
	    mssError(1, "TESTOBJ", "testobj_i_hasChildren(): has_children pointer cannot be NULL.");
	    goto end;
	    }

	/** Assume no children. **/
	*has_children = false;

	/** Open the object and get its info. **/
	obj = objOpen(s, path, O_RDONLY, 0400, NULL);
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open \"%s\".", path);
	    goto end;
	    }
	const pObjectInfo info = objInfo(obj);
	if (UNLIKELY(info == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to get info for \"%s\".", path);
	    goto end;
	    }

	/** Use the driver's info, if it answers the question. **/
	if ((info->Flags & (OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ)) != 0)
	    {
	    rval = 0;
	    goto end;
	    }
	if ((info->Flags & OBJ_INFO_F_HAS_SUBOBJ) != 0)
	    {
	    *has_children = true;
	    rval = 0;
	    goto end;
	    }
	if ((info->Flags & OBJ_INFO_F_SUBOBJ_CNT_KNOWN) != 0)
	    {
	    *has_children = (info->nSubobjects > 0);
	    rval = 0;
	    goto end;
	    }

	/** Fetch one child. **/
	query = objOpenQuery(obj, NULL, NULL, NULL, NULL, 0);
	if (UNLIKELY(query == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to query \"%s\".", path);
	    goto end;
	    }
	child = objQueryFetch(query, O_RDONLY);
	*has_children = (child != NULL);

	/** Success. **/
	rval = 0;

    end:
	if (child != NULL) warnNeg(objClose(child));
	if (query != NULL) warnNeg(objQueryClose(query));
	if (LIKELY(obj != NULL)) warnNeg(objClose(obj));

	return rval;
    }


/*** handle_tab - completes the OSML path that ends at the cursor, or lists
 *** the possible completions on a double tab.  Failures print a warning and
 *** leave the line unchanged.
 ***
 *** @param unused_1 The readline count argument (unused).
 *** @param unused_2 The key that was pressed (unused).
 *** @returns 0, as readline expects from a key binding.
 ***/
int
handle_tab(int unused_1, int unused_2)
    {
    bool successful = false;
    char* line = NULL;
    char* word = "";
    pXString path = NULL;
    pXString dir = NULL;
    pXString query_text = NULL;
    pXString matched = NULL;
    pXString child_path = NULL;
    pObject obj = NULL;
    pObjQuery query = NULL;
    pObject query_obj = NULL;
    bool secondtab = false;
    int count = 0;

#define DOUBLE_TAB_DELAY 50

	/** No session yet at the login prompt. **/
	if (s == NULL) return 0;

	/** Detect a double tab. **/
	if (ticks_last_tab + DOUBLE_TAB_DELAY > mtRealTicks())
	    {
	    secondtab = true;
	    ticks_last_tab = 0; /* A third tab starts over. */
	    }
	else ticks_last_tab = mtRealTicks();

	/** Beep. **/
	printf("%c", 0x07);
	if (secondtab) printf("\n");

	/** Find the word before the cursor. **/
	line = rl_copy_text(0, rl_point);
	if (UNLIKELY(line == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to copy the input line.");
	    goto end;
	    }
	char* const space = strrchr(line, ' ');
	word = (space == NULL) ? line : space + 1;

	/** Allocate strings. **/
	path = xsNew();
	dir = xsNew();
	query_text = xsNew();
	matched = xsNew();
	child_path = xsNew();
	if (UNLIKELY(path == NULL || dir == NULL || query_text == NULL || matched == NULL || child_path == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to allocate strings.");
	    goto end;
	    }

	/** Make the word an absolute path. **/
	if (word[0] == '/')
	    {
	    if (UNLIKELY(xsCopy(path, word, -1) < 0))
		{
		mssError(1, "TESTOBJ", "Failed to copy path \"%s\".", word);
		goto end;
		}
	    }
	else
	    {
	    char* const wd = objGetWD(s);
	    if (UNLIKELY(wd == NULL))
		{
		mssError(1, "TESTOBJ", "Failed to get the working directory.");
		goto end;
		}

	    /** The root directory already ends with a slash. **/
	    if (UNLIKELY(xsPrintf(path, "%s%s%s", wd, (strlen(wd) > 1) ? "/" : "", word) < 0))
		{
		mssError(1, "TESTOBJ", "Failed to build a path from \"%s\" and \"%s\".", wd, word);
		goto end;
		}
	    }

	/** Split off the partial name. **/
	const int last_slash = xsFindRev(path, "/", -1, 0);
	if (UNLIKELY(xsCopy(dir, xsString(path), last_slash + 1) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to copy the directory of \"%s\".", xsString(path));
	    goto end;
	    }
	char* const partial = xsString(path) + last_slash + 1;
	const int partial_len = strlen(partial);

	/** Skip a directory with no children. **/
	bool dir_has_children = false;
	if (UNLIKELY(testobj_i_hasChildren(xsString(dir), &dir_has_children) < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to check \"%s\" for children.", xsString(dir));
	    goto end;
	    }
	if (!dir_has_children)
	    {
	    successful = true; /* Nothing to complete. */
	    goto end;
	    }

	/** Open the directory. **/
	obj = objOpen(s, xsString(dir), O_RDONLY, 0400, NULL);
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open directory \"%s\".", xsString(dir));
	    goto end;
	    }

	/** Query for children that start with the partial name. **/
	if (UNLIKELY(xsPrintf(query_text, "substring(:name,0,%d)=\"%s\"", partial_len, partial) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to build the query for \"%s\".", partial);
	    goto end;
	    }
	query = objOpenQuery(obj, xsString(query_text), NULL, NULL, NULL, 0);
	if (UNLIKELY(query == NULL))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to query \"%s\" with: %s",
		xsString(dir), xsString(query_text)
	    );
	    goto end;
	    }

	/** Find the longest prefix shared by all matches. **/
	while ((query_obj = objQueryFetch(query, O_RDONLY)) != NULL)
	    {
	    char* name = NULL;
	    if (UNLIKELY(objGetAttrValue(query_obj, "name", DATA_T_STRING, POD(&name)) != 0))
		{
		mssError(0, "TESTOBJ",
		    "Failed to get the name of a child of \"%s\".",
		    xsString(dir)
		);
		goto end;
		}

	    /** List matches on a double tab. **/
	    if (secondtab)
		{
		if (count == 1) printf("%s\n", xsString(matched));
		if (count >= 1) printf("%s\n", name);
		}

	    /** Shorten to the common prefix. **/
	    if (count == 0)
		{
		if (UNLIKELY(xsCopy(matched, name, -1) < 0))
		    {
		    mssError(1, "TESTOBJ", "Failed to copy name \"%s\".", name);
		    goto end;
		    }
		}
	    else
		{
		int common = 0;
		while (name[common] != '\0' && name[common] == xsString(matched)[common]) common++;
		if (UNLIKELY(xsSubst(matched, common, -1, "", 0) < 0))
		    {
		    mssError(1, "TESTOBJ",
			"Failed to shorten \"%s\" to %d characters.",
			xsString(matched), common
		    );
		    goto end;
		    }
		}
	    count++;

	    const int close_rval = objClose(query_obj);
	    query_obj = NULL;
	    if (UNLIKELY(close_rval < 0))
		{
		mssError(0, "TESTOBJ", "Failed to close a child of \"%s\".", xsString(dir));
		goto end;
		}
	    }

	/** Build the path of a single match. **/
	if (UNLIKELY(count == 1 && xsPrintf(child_path, "%s%s", xsString(dir), xsString(matched)) < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to build a path from \"%s\" and \"%s\".",
		xsString(dir), xsString(matched)
	    );
	    goto end;
	    }

	/** Insert the common prefix. **/
	if (xsLength(matched) > partial_len) rl_insert_text(xsString(matched) + partial_len);

	/** Add a slash after a single match that has children. **/
	if (count == 1)
	    {
	    ticks_last_tab = 0; /* The next tab is a first tab. */

	    /** The name is already inserted, so only warn if the check fails. **/
	    bool has_children = false;
	    if (UNLIKELY(testobj_i_hasChildren(xsString(child_path), &has_children) < 0))
		{
		mssWarnError("Failed to check \"%s\" for children.", xsString(child_path));
		rl_on_new_line();
		}
	    else if (has_children) rl_insert_text("/");
	    }

	successful = true;

    end:
	if (UNLIKELY(!successful))
	    {
	    mssWarnError("Failed to complete \"%s\".", word);
	    rl_on_new_line();
	    }

	/** Clean up. **/
	if (query_obj != NULL) warnNeg(objClose(query_obj));
	if (query != NULL) warnNeg(objQueryClose(query));
	if (LIKELY(obj != NULL)) warnNeg(objClose(obj));
	if (LIKELY(child_path != NULL)) xsFree(child_path);
	if (LIKELY(matched != NULL)) xsFree(matched);
	if (LIKELY(query_text != NULL)) xsFree(query_text);
	if (LIKELY(dir != NULL)) xsFree(dir);
	if (LIKELY(path != NULL)) xsFree(path);
	if (LIKELY(line != NULL)) free(line);

	/** Move readline below the listing. **/
	if (secondtab)
	    {
	    printf("\n");
	    rl_on_new_line();
	    }

    return 0;
    }


/*** testobj_i_readString - reads the next token of a command, which must be
 *** a string.
 ***
 *** @param ls The lexer to read from.
 *** @param usage The usage message to report if the token is not a string.
 *** @returns The string, allocated with nmSysMalloc(), or NULL on failure.
 ***/
static char*
testobj_i_readString(pLxSession ls, char* usage)
    {
	/** Get the next token, failing if it's not a string token. **/
	const int token = mlxNextToken(ls);
	if (UNLIKELY(token == MLX_TOK_ERROR))
	    {
	    mssError(0, "TESTOBJ", "Failed to read the next token.");
	    return NULL;
	    }
	if (UNLIKELY(token != MLX_TOK_STRING))
	    {
	    mssError(1, "TESTOBJ", "%s", usage);
	    return NULL;
	    }

	/** Copy the string out of the lexer. **/
	int allocated = 0;
	char* const value = mlxStringVal(ls, &allocated);
	if (UNLIKELY(value == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to read a string token.");
	    return NULL;
	    }
	if (allocated == 1) return value;
	char* const copy = nmSysStrdup(value);
	if (UNLIKELY(copy == NULL)) mssError(1, "TESTOBJ", "Failed to copy string \"%s\".", value);

    return copy;
    }


/*** testobj_i_readKeyword - reads the next token of a command, which must be
 *** a keyword or the end of the command.
 ***
 *** @param ls The lexer to read from.
 *** @param usage The usage message to report if the token is something else.
 *** @param keyword Set to the keyword, which is valid until the next token is
 *** 	read, or to NULL at the end of the command.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_readKeyword(pLxSession ls, char* usage, char** keyword)
    {
	/** Handle edge cases. **/
	if (UNLIKELY(keyword == NULL))
	    {
	    mssError(1, "TESTOBJ", "keyword pointer cannot be NULL.");
	    return -1;
	    }

	*keyword = NULL;

	/** Get the next token as a keyword string. **/
	const int token = mlxNextToken(ls);
	if (token == MLX_TOK_EOF) return 0;
	if (UNLIKELY(token == MLX_TOK_ERROR))
	    {
	    mssError(0, "TESTOBJ", "Failed to read the next token.");
	    return -1;
	    }
	if (UNLIKELY(token != MLX_TOK_KEYWORD))
	    {
	    mssError(1, "TESTOBJ", "%s", usage);
	    return -1;
	    }
	*keyword = mlxStringVal(ls, NULL);
	if (UNLIKELY(*keyword == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to read a keyword token.");
	    return -1;
	    }

    return 0;
    }


/*** testobj_i_obfuscate - obfuscates a value if obfuscation is enabled.
 ***
 *** @param attrname The attribute the value came from.
 *** @param type The type of the value.
 *** @param value The value.
 *** @param shown Set to the value to show.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_obfuscate(char* attrname, int type, pObjData value, pObjData shown)
    {
	if (TESTOBJ.ObfuscationSession == NULL)
	    {
	    memcpy(shown, value, sizeof(ObjData));
	    return 0;
	    }

	memset(shown, 0, sizeof(ObjData)); /* Obfuscation leaves unsupported types unset. */
	if (UNLIKELY(obfObfuscateDataSess(TESTOBJ.ObfuscationSession, value, shown, type, attrname, NULL, NULL) < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to obfuscate attribute \"%s\".", attrname);
	    return -1;
	    }

    return 0;
    }


/*** testobj_i_cmdCd - changes the working directory.
 ***
 *** @param s The OSML session.
 *** @param path The directory, or NULL if none was given.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdCd(pObjSession s, char* path)
    {
    int rval = -1;
    pObject dir = NULL;

	if (UNLIKELY(path == NULL))
	    {
	    mssError(1, "TESTOBJ", "Usage: cd <directory>");
	    goto end;
	    }

	dir = objOpen(s, path, O_RDONLY, 0600, "system/directory");
	if (UNLIKELY(dir == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open directory \"%s\".", path);
	    goto end;
	    }
	if (UNLIKELY(objSetWD(s, dir) < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to change to directory \"%s\".", path);
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (LIKELY(dir != NULL)) warnNeg(objClose(dir));

	return rval;
    }


/*** testobj_i_getCsvText - gets the text of one csv field.
 ***
 *** @param row The query result row.
 *** @param attrname The attribute to get.
 *** @param type Set to the type of the attribute.
 *** @param text Set to the text, which is valid until the next call to
 *** 	objDataToStringTmp().
 *** @returns 0 on success, 1 if the field is empty, or -1 on failure.
 ***/
static int
testobj_i_getCsvText(pObject row, char* attrname, int* type, char** text)
    {
    ObjData value;
    ObjData shown;

	/** Check value. **/
	*type = objGetAttrType(row, attrname);
	if (UNLIKELY(*type < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to get the type of attribute \"%s\".", attrname);
	    return -1;
	    }
	if (*type == DATA_T_UNAVAILABLE || *type == DATA_T_CODE) return 1;

	/** Get value. **/
	const int get_result = objGetAttrValue(row, attrname, *type, &value);
	if (UNLIKELY(get_result < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to get the value of attribute \"%s\".", attrname);
	    return -1;
	    }
	if (get_result == 1) return 1;
	if (UNLIKELY(testobj_i_obfuscate(attrname, *type, &value, &shown) < 0)) return -1;

	/** Convert the value to text. **/
	if (*type == DATA_T_INTEGER || *type == DATA_T_DOUBLE || *type == DATA_T_BINARY)
	    {
	    *text = objDataToStringTmp(*type, &shown, 0);
	    }
	else
	    {
	    *text = objDataToStringTmp(*type, shown.Generic, 0);
	    }
	if (UNLIKELY(*text == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to convert attribute \"%s\" to text.", attrname);
	    return -1;
	    }

	return 0;
    }


/*** testobj_i_printCsvField - prints one csv field.  A field that cannot be
 *** read gets a warning and is left empty.
 ***
 *** @param row The query result row.
 *** @param attrname The attribute to print.
 *** @param present Whether the row has this attribute.  If not, the field is
 *** 	left empty.
 *** @param first Whether this is the first field of the row.
 *** @returns 0 on success, or -1 if writing the output fails.
 ***/
static int
testobj_i_printCsvField(pObject row, char* attrname, bool present, bool first)
    {
    int type = DATA_T_UNAVAILABLE;
    char* text = NULL;
    int write_result;

	const int text_result = (present) ? testobj_i_getCsvText(row, attrname, &type, &text) : 1;
	if (UNLIKELY(text_result < 0))
	    {
	    mssWarnError(
		"Failed to read csv field \"%s\", leaving it empty.",
		attrname
	    );
	    }
	if (text_result != 0)
	    {
	    write_result = fdQPrintf(TESTOBJ.Output, "%[,%]", !first);
	    }
	else if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE || type == DATA_T_MONEY || type == DATA_T_DATETIME)
	    {
	    write_result = fdQPrintf(TESTOBJ.Output, "%[,%]%STR", !first, text);
	    }
	else
	    {
	    /** Flatten line breaks. **/
	    for (char* cur = strpbrk(text, "\r\n"); cur != NULL; cur = strpbrk(cur, "\r\n")) *cur = ' ';
	    write_result = fdQPrintf(TESTOBJ.Output, "%[,%]\"%STR&DSYB\"", !first, text);
	    }
	if (UNLIKELY(write_result < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write csv field \"%s\" to output file \"%s\".",
		attrname, TESTOBJ.OutputFilename
	    );
	    return -1;
	    }

	return 0;
    }


/*** testobj_i_cmdCsv - runs a query and prints the results as csv.
 ***
 *** @param s The OSML session.
 *** @param query_text The query, or NULL if none was given.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdCsv(pObjSession s, char* query_text)
    {
    int rval = -1;
    pObjQuery query = NULL;
    pObject row = NULL;
    char* attrnames[CSV_MAX_ATTRS];
    int n_attrs = 0;
    int n_rows = 0;
    bool present[CSV_MAX_ATTRS];
    bool unused_present;
    XHashTable columns_buf;
    pXHashTable columns = NULL;
    XArray extras_buf;
    pXArray extras = NULL;

	if (UNLIKELY(query_text == NULL))
	    {
	    mssError(1, "TESTOBJ", "Usage: csv <query-text>");
	    goto end;
	    }

	/** Map each attribute name to its presence flag. **/
	if (UNLIKELY(xhInit(&columns_buf, 257, 0) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to initialize the csv column table.");
	    goto end;
	    }
	columns = &columns_buf;

	/** Names of attributes left out of the header. **/
	if (UNLIKELY(xaInit(&extras_buf, 16) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to initialize the csv extra attribute list.");
	    goto end;
	    }
	extras = &extras_buf;

	query = objMultiQuery(s, query_text, NULL, 0);
	if (UNLIKELY(query == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open query: %s", query_text);
	    goto end;
	    }

	/** Print the header from the first row. **/
	row = objQueryFetch(query, O_RDONLY);
	if (row == NULL)
	    {
	    rval = 0;
	    goto end;
	    }
	for (char* attrname = objGetFirstAttr(row); attrname != NULL; attrname = objGetNextAttr(row))
	    {
	    if (n_attrs >= CSV_MAX_ATTRS) continue;
	    attrnames[n_attrs] = nmSysStrdup(attrname);
	    if (UNLIKELY(attrnames[n_attrs] == NULL))
		{
		mssError(1, "TESTOBJ", "Failed to copy attribute name \"%s\".", attrname);
		goto end;
		}
	    n_attrs++;
	    if (UNLIKELY(xhAdd(columns, attrnames[n_attrs - 1], (char*)&present[n_attrs - 1]) < 0))
		{
		mssError(1, "TESTOBJ", "Failed to add csv column \"%s\".", attrname);
		goto end;
		}
	    if (UNLIKELY(fdQPrintf(TESTOBJ.Output, "%[,%]\"%STR&DSYB\"", n_attrs > 1, attrname) < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write csv header \"%s\" to output file \"%s\".",
		    attrname, TESTOBJ.OutputFilename
		);
		goto end;
		}
	    }
	if (UNLIKELY(fdPrintf(TESTOBJ.Output, "\n") < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write the end of the csv header to output file \"%s\".",
		TESTOBJ.OutputFilename
	    );
	    goto end;
	    }

	while (row != NULL)
	    {
	    n_rows++;

	    /** Find the columns this row has. **/
	    memset(present, 0, sizeof(present));
	    for (char* attrname = objGetFirstAttr(row); attrname != NULL; attrname = objGetNextAttr(row))
		{
		bool* flag = (bool*)xhLookup(columns, attrname);
		if (flag != NULL)
		    {
		    *flag = true;
		    continue;
		    }

		/*** Warn once for each attribute left out of the header.
		 *** Add the attribute to the to prevent repeat warnings.
		 ***/
		char* extra = nmSysStrdup(attrname);
		if (UNLIKELY(extra == NULL))
		    {
		    mssError(1, "TESTOBJ", "Failed to copy attribute name \"%s\".", attrname);
		    goto end;
		    }
		if (UNLIKELY(xaAddItem(extras, extra) < 0))
		    {
		    mssError(1, "TESTOBJ", "Failed to record extra attribute \"%s\".", attrname);
		    nmSysFree(extra);
		    goto end;
		    }
		if (UNLIKELY(xhAdd(columns, extra, (char*)&unused_present) < 0))
		    {
		    mssError(1, "TESTOBJ", "Failed to add extra attribute \"%s\".", attrname);
		    goto end;
		    }
		fprintf(stderr,
		    "Warning: Attribute \"%s\" in csv row %d is not in the header, so it is left out.\n",
		    attrname, n_rows
		);
		}

	    /** Print the row. **/
	    for (int i = 0; i < n_attrs; i++)
		{
		if (UNLIKELY(testobj_i_printCsvField(row, attrnames[i], present[i], i == 0) < 0)) goto end;
		}
	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, "\n") < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write the end of a csv row to output file \"%s\".",
		    TESTOBJ.OutputFilename
		);
		goto end;
		}

	    warnNeg(objClose(row));
	    row = objQueryFetch(query, O_RDONLY);
	    }

	rval = 0;

    end:
	if (row != NULL) warnNeg(objClose(row));
	if (LIKELY(query != NULL)) warnNeg(objQueryClose(query));
	if (LIKELY(columns != NULL))
	    {
	    warnNeg(xhClear(columns, NULL, NULL));
	    warnNeg(xhDeInit(columns));
	    }
	if (LIKELY(extras != NULL))
	    {
	    for (int i = 0; i < xaCount(extras); i++)
		nmSysFree(xaGetItem(extras, i));
	    warnNeg(xaDeInit(extras));
	    }
	for (int i = 0; i < n_attrs; i++)
	    nmSysFree(attrnames[i]);

	return rval;
    }


/*** testobj_i_getQueryText - gets the text that the query command prints for
 *** one attribute of a result row.
 ***
 *** @param row The query result row.
 *** @param attrname The attribute to get.
 *** @param type Set to the type of the attribute.
 *** @param shown Set to the value to show, unless the value is NULL.
 *** @param text Set to the text, which is valid until the next call to
 *** 	objDataToStringTmp().
 *** @returns 0 on success, 1 if the value is NULL (the text is "NULL"), 2 if
 *** 	the attribute is unavailable, or -1 on failure.
 ***/
static int
testobj_i_getQueryText(pObject row, char* attrname, int* type, pObjData shown, char** text)
    {
    ObjData value;

	*type = objGetAttrType(row, attrname);
	if (UNLIKELY(*type < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to get the type of attribute \"%s\".", attrname);
	    return -1;
	    }
	if (*type == DATA_T_UNAVAILABLE) return 2;

	const int get_result = objGetAttrValue(row, attrname, *type, &value);
	if (UNLIKELY(get_result < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to get the value of attribute \"%s\".", attrname);
	    return -1;
	    }
	if (get_result == 1)
	    {
	    *text = "NULL";
	    return 1;
	    }
	if (UNLIKELY(testobj_i_obfuscate(attrname, *type, &value, shown) < 0)) return -1;

	/** Convert the value to text. **/
	switch (*type)
	    {
	    case DATA_T_INTEGER:
	    case DATA_T_DOUBLE:
		{
		*text = objDataToStringTmp(*type, shown, 0);
		break;
		}
	    case DATA_T_STRINGVEC:
		{
		*text = objDataToStringTmp(*type, shown->StringVec, DATA_F_QUOTED | DATA_F_BRACKETS);
		break;
		}
	    case DATA_T_INTVEC:
		{
		*text = objDataToStringTmp(*type, shown->IntVec, DATA_F_QUOTED | DATA_F_BRACKETS);
		break;
		}
	    case DATA_T_STRING:
	    case DATA_T_DATETIME:
	    case DATA_T_MONEY:
		{
		*text = objDataToStringTmp(*type, shown->Generic, DATA_F_QUOTED);
		break;
		}
	    case DATA_T_BINARY:
		{
		*text = ""; /* The caller prints the bytes. */
		break;
		}
	    default:
		{
		*text = "<unsupported type>";
		break;
		}
	    }
	if (UNLIKELY(*text == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to convert attribute \"%s\" to text.", attrname);
	    return -1;
	    }

	return 0;
    }


/*** testobj_i_printQueryAttr - prints one attribute of a query result row.
 *** An attribute that cannot be read gets a warning instead.
 ***
 *** @param row The query result row.
 *** @param attrname The attribute to print.
 *** @returns 0 on success, or -1 if writing the output fails.
 ***/
static int
testobj_i_printQueryAttr(pObject row, char* attrname)
    {
    int type = DATA_T_UNAVAILABLE;
    ObjData shown;
    char* text = NULL;

	const int text_result = testobj_i_getQueryText(row, attrname, &type, &shown, &text);
	if (UNLIKELY(text_result < 0))
	    {
	    mssWarnError("Failed to read attribute \"%s\", skipping it.", attrname);
	    return 0;
	    }
	if (text_result == 2) return 0;

	/** Print the attribute. **/
	if (UNLIKELY(fdPrintf(TESTOBJ.Output, "Attribute [%s]: %8.8s  %s", attrname, obj_type_names[type], text) < 0)) goto error;
	if (text_result == 0 && type == DATA_T_BINARY)
	    {
	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, " %d bytes: ", shown.Binary.Size) < 0)) goto error;
	    for (int i = 0; i < shown.Binary.Size; i++)
		{
		if (UNLIKELY(fdPrintf(TESTOBJ.Output, "%2.2x ", shown.Binary.Data[i]) < 0)) goto error;
		}
	    }
	if (UNLIKELY(fdPrintf(TESTOBJ.Output, "\n") < 0)) goto error;

	return 0;

    error:
	mssError(1, "TESTOBJ",
	    "Failed to write attribute \"%s\" to output file \"%s\".",
	    attrname, TESTOBJ.OutputFilename
	);

	return -1;
    }


/*** testobj_i_cmdQuery - runs a query and prints each attribute of each
 *** result row.
 ***
 *** @param s The OSML session.
 *** @param query_text The query, or NULL if none was given.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdQuery(pObjSession s, char* query_text)
    {
    int rval = -1;
    pObjQuery query = NULL;
    pObject row = NULL;

	if (UNLIKELY(query_text == NULL))
	    {
	    mssError(1, "TESTOBJ", "Usage: query <query-text>");
	    goto end;
	    }

	query = objMultiQuery(s, query_text, NULL, 0);
	if (UNLIKELY(query == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open query: %s", query_text);
	    goto end;
	    }

	while ((row = objQueryFetch(query, O_RDONLY)) != NULL)
	    {
	    for (char* attrname = objGetFirstAttr(row); attrname != NULL; attrname = objGetNextAttr(row))
		{
		if (UNLIKELY(testobj_i_printQueryAttr(row, attrname) < 0)) goto end;
		}
	    warnNeg(objClose(row));
	    row = NULL;
	    }

	rval = 0;

    end:
	if (row != NULL) warnNeg(objClose(row));
	if (LIKELY(query != NULL)) warnNeg(objQueryClose(query));

	return rval;
    }


/*** testobj_i_appendQueryLine - appends a line to a multiline query.
 ***
 *** @param query_text The query so far.
 *** @param line The line to append.
 *** @returns 0 on success, 1 if the line is blank (ending the query), or -1
 *** 	on failure.
 ***/
static int
testobj_i_appendQueryLine(pXString query_text, char* line)
    {
	if (line[strspn(line, "\r\n\t ")] == '\0') return 1;

	if (UNLIKELY(xsConcatenate(query_text, " ", 1) < 0 || xsConcatenate(query_text, line, -1) < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to append \"%s\" to query: %s",
		line, xsString(query_text)
	    );
	    return -1;
	    }

	return 0;
    }


/*** testobj_i_cmdMlquery - reads a query that continues until a blank line,
 *** then runs it.
 ***
 *** @param s The OSML session.
 *** @param first_line The start of the query, or NULL if none was given.
 *** @param inp_lx The command file to read more lines from, or NULL to read
 *** 	them from the terminal.  Its end of file is left for the caller to
 *** 	read.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdMlquery(pObjSession s, char* first_line, pLxSession inp_lx)
    {
    int rval = -1;
    pXString query_text = NULL;

	if (UNLIKELY(first_line == NULL))
	    {
	    mssError(1, "TESTOBJ", "Usage: mlquery <query-text>");
	    goto end;
	    }

	query_text = xsNew();
	if (UNLIKELY(query_text == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to allocate a string for query: %s", first_line);
	    goto end;
	    }
	if (UNLIKELY(xsCopy(query_text, first_line, -1) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to copy query: %s", first_line);
	    goto end;
	    }

	/** Read lines until a blank line or the end of input. **/
	while (true)
	    {
	    int append_rval;
	    if (inp_lx != NULL)
		{
		const int token = mlxNextToken(inp_lx);
		if (UNLIKELY(token == MLX_TOK_ERROR))
		    {
		    mssError(0, "TESTOBJ",
			"Failed to read the next line of query: %s",
			xsString(query_text)
		    );
		    goto end;
		    }
		if (token == MLX_TOK_EOF)
		    {
		    if (UNLIKELY(mlxHoldToken(inp_lx) < 0))
			{
			mssError(1, "TESTOBJ",
			    "Failed to leave the end of the command file unread after query: %s",
			    xsString(query_text)
			);
			goto end;
			}
		    break;
		    }
		int allocated = 0;
		char* const line = mlxStringVal(inp_lx, &allocated);
		if (UNLIKELY(line == NULL))
		    {
		    mssError(0, "TESTOBJ",
			"Failed to read the next line of query: %s",
			xsString(query_text)
		    );
		    goto end;
		    }
		append_rval = testobj_i_appendQueryLine(query_text, line);
		if (allocated == 1) nmSysFree(line);
		}
	    else
		{
		char* const line = readline("");
		if (line == NULL) break;
		append_rval = testobj_i_appendQueryLine(query_text, line);
		free(line);
		}
	    if (UNLIKELY(append_rval < 0)) goto end;
	    if (append_rval == 1) break;
	    }

	if (UNLIKELY(testobj_i_cmdQuery(s, xsString(query_text)) < 0)) goto end;

	rval = 0;

    end:
	if (LIKELY(query_text != NULL)) xsFree(query_text);

	return rval;
    }


/*** testobj_i_cmdAnnot - sets the annotation of an object.
 ***
 *** @param s The OSML session.
 *** @param path The object, or NULL if none was given.
 *** @param ls The lexer for the rest of the command, positioned before the
 *** 	annotation.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdAnnot(pObjSession s, char* path, pLxSession ls)
    {
    int rval = -1;
    char* const usage = "Usage: annot <filename> \"<annotation>\"";
    char* annotation = NULL;
    pObject obj = NULL;

	if (UNLIKELY(path == NULL))
	    {
	    mssError(1, "TESTOBJ", "%s", usage);
	    goto end;
	    }
	annotation = testobj_i_readString(ls, usage);
	if (UNLIKELY(annotation == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to read the annotation for \"%s\".", path);
	    goto end;
	    }

	/** Set the annotation. **/
	obj = objOpen(s, path, O_RDWR, 0600, "system/object");
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open \"%s\".", path);
	    goto end;
	    }
	if (UNLIKELY(objSetAttrValue(obj, "annotation", DATA_T_STRING, POD(&annotation)) < 0))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to set the annotation of \"%s\" to \"%s\".",
		path, annotation
	    );
	    goto end;
	    }

	rval = 0;

    end:
	if (LIKELY(obj != NULL)) warnNeg(objClose(obj));
	if (LIKELY(annotation != NULL)) nmSysFree(annotation);

	return rval;
    }


/*** testobj_i_copyStringAttr - copies the value of a string attribute.
 ***
 *** @param obj The object to read from.
 *** @param attrname The attribute to read.
 *** @returns The value (free with nmSysFree()), "" if the value is NULL, or
 *** 	NULL on failure.
 ***/
static char*
testobj_i_copyStringAttr(pObject obj, char* attrname)
    {
    char* value = NULL;

	const int get_result = objGetAttrValue(obj, attrname, DATA_T_STRING, POD(&value));
	if (UNLIKELY(get_result < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to get attribute \"%s\".", attrname);
	    return NULL;
	    }
	if (get_result == 1) value = "";

	char* const copy = nmSysStrdup(value);
	if (UNLIKELY(copy == NULL))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to copy attribute \"%s\" with value \"%s\".",
		attrname, value
	    );
	    }

	return copy;
    }


/*** testobj_i_printListRow - prints the name, annotation, and type of an
 *** object in a directory listing.  An object that cannot be read gets a
 *** warning instead.
 ***
 *** @param obj The object to print.
 *** @param dir_path The directory being listed.
 *** @returns 0 on success, or -1 if writing the output fails.
 ***/
static int
testobj_i_printListRow(pObject obj, char* dir_path)
    {
    int rval = -1;
    char* name = NULL;
    char* type = NULL;
    char* annotation = NULL;

	/** Copy each value, since getting the next may overwrite it. **/
	name = testobj_i_copyStringAttr(obj, "name");
	if (name != NULL) type = testobj_i_copyStringAttr(obj, "outer_type");
	if (type != NULL) annotation = testobj_i_copyStringAttr(obj, "annotation");
	if (UNLIKELY(annotation == NULL))
	    {
	    mssWarnError("Failed to read a child of \"%s\", skipping it.", dir_path);
	    rval = 0;
	    goto end;
	    }

	if (UNLIKELY(fdPrintf(TESTOBJ.Output, "%-32.32s  %-32.32s    %s\n", name, annotation, type) < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to write \"%s\" in the listing of \"%s\" to output file \"%s\".",
		name, dir_path, TESTOBJ.OutputFilename
	    );
	    goto end;
	    }

	rval = 0;

    end:
	if (annotation != NULL) nmSysFree(annotation);
	if (type != NULL) nmSysFree(type);
	if (name != NULL) nmSysFree(name);

	return rval;
    }


/*** testobj_i_cmdList - lists the objects in a directory.
 ***
 *** @param s The OSML session.
 *** @param first_arg The first argument (a directory, "where", or
 *** 	"orderby"), or NULL if none was given.
 *** @param ls The lexer for the rest of the command, positioned after
 *** 	first_arg.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdList(pObjSession s, char* first_arg, pLxSession ls)
    {
    int rval = -1;
    char* const usage = "Usage: list [<directory>] [where \"<criteria>\"] [orderby \"<order>\"]";
    char* dir_path = "";
    char* where = NULL;
    char* order_by = NULL;
    pObject dir = NULL;
    pObjQuery query = NULL;
    pObject child = NULL;

	/** Read the directory. **/
	char* keyword = NULL;
	if (first_arg != NULL && (strcmp(first_arg, "where") == 0 || strcmp(first_arg, "orderby") == 0))
	    {
	    keyword = first_arg;
	    }
	else if (first_arg != NULL)
	    {
	    dir_path = first_arg;
	    if (UNLIKELY(testobj_i_readKeyword(ls, usage, &keyword) < 0)) goto end;
	    }

	/** Read the clauses. **/
	if (keyword != NULL && strcmp(keyword, "where") == 0)
	    {
	    where = testobj_i_readString(ls, usage);
	    if (UNLIKELY(where == NULL)) goto end;
	    printf("where: '%s'\n", where);
	    if (UNLIKELY(testobj_i_readKeyword(ls, usage, &keyword) < 0)) goto end;
	    }
	if (keyword != NULL && strcmp(keyword, "orderby") == 0)
	    {
	    order_by = testobj_i_readString(ls, usage);
	    if (UNLIKELY(order_by == NULL)) goto end;
	    if (UNLIKELY(testobj_i_readKeyword(ls, usage, &keyword) < 0)) goto end;
	    }
	if (UNLIKELY(keyword != NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to parse unknown clause \"%s\". %s", keyword, usage);
	    goto end;
	    }

	/** Query the directory. **/
	dir = objOpen(s, dir_path, O_RDONLY, 0600, "system/directory");
	if (UNLIKELY(dir == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open directory \"%s\".", dir_path);
	    goto end;
	    }
	char* criteria = "";
	if (where != NULL)
	    {
	    criteria = where;
	    }
	else if (order_by != NULL)
	    {
	    criteria = NULL;
	    }
	query = objOpenQuery(dir, criteria, order_by, NULL, NULL, 0);
	if (UNLIKELY(query == NULL))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to query directory \"%s\" with where \"%s\" and orderby \"%s\".",
		dir_path, (where != NULL) ? where : "", (order_by != NULL) ? order_by : ""
	    );
	    goto end;
	    }

	/** Print the children. **/
	while ((child = objQueryFetch(query, O_RDONLY)) != NULL)
	    {
	    if (UNLIKELY(testobj_i_printListRow(child, dir_path) < 0)) goto end;
	    warnNeg(objClose(child));
	    child = NULL;
	    }

	rval = 0;

    end:
	if (child != NULL) warnNeg(objClose(child));
	if (LIKELY(query != NULL)) warnNeg(objQueryClose(query));
	if (LIKELY(dir != NULL)) warnNeg(objClose(dir));
	if (order_by != NULL) nmSysFree(order_by);
	if (where != NULL) nmSysFree(where);

	return rval;
    }


/*** testobj_i_cmdShow - prints an object for the show, printshow, and print
 *** commands.
 ***
 *** @param s The OSML session.
 *** @param path The path of the object.
 *** @param type The type to open the object as.
 *** @param show_content Whether to print the object's content.
 *** @param show_details Whether to print the object's info, attributes, and
 *** 	methods (after the content).
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdShow(pObjSession s, char* path, char* type, bool show_content, bool show_details)
    {
    int rval = -1;
    pObject obj = NULL;

	obj = objOpen(s, path, O_RDONLY, 0600, type);
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open \"%s\" as \"%s\".", path, type);
	    goto end;
	    }

	/** Print the content. **/
	if (show_content)
	    {
	    if (UNLIKELY(testobj_show_content(obj) < 0))
		{
		mssError(0, "TESTOBJ", "Failed to print the content of \"%s\".", path);
		goto end;
		}
	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, "\n") < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write the content of \"%s\" to output file \"%s\".",
		    path, TESTOBJ.OutputFilename
		);
		goto end;
		}
	    }

	/** Print the details. **/
	if (show_details)
	    {
	    if (UNLIKELY(testobj_show_info(obj) < 0 || testobj_show_attrs(obj) < 0))
		{
		mssError(0, "TESTOBJ", "Failed to print the attributes of \"%s\".", path);
		goto end;
		}
	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, "\n") < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write the attributes of \"%s\" to output file \"%s\".",
		    path, TESTOBJ.OutputFilename
		);
		goto end;
		}
	    if (UNLIKELY(testobj_show_methods(obj) < 0))
		{
		mssError(0, "TESTOBJ", "Failed to print the methods of \"%s\".", path);
		goto end;
		}
	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, "\n") < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write the methods of \"%s\" to output file \"%s\".",
		    path, TESTOBJ.OutputFilename
		);
		goto end;
		}
	    }

	rval = 0;

    end:
	if (LIKELY(obj != NULL)) warnNeg(objClose(obj));

	return rval;
    }


/*** testobj_i_cmdCopy - copies one object's content to another for the
 *** command: copy <dsttype/srctype> <source> <destination>
 ***
 *** @param s The OSML session.
 *** @param mode "srctype" to create the destination with the source's type,
 *** 	or anything else to read the source as the destination's type, or NULL
 *** 	if none was given.
 *** @param ls The command's lexer session, positioned after the mode.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdCopy(pObjSession s, char* mode, pLxSession ls)
    {
    int rval = -1;
    char* const usage = "Usage: copy <dsttype/srctype> <source> <destination>";
    char* src_path = NULL;
    char* dst_path = NULL;
    char buf[BUFF_SIZE];
    char* type = NULL;
    pObject src = NULL;
    pObject dst = NULL;

	/** Parse the arguments. **/
	if (UNLIKELY(mode == NULL))
	    {
	    mssError(1, "TESTOBJ", "%s", usage);
	    goto end;
	    }
	src_path = testobj_i_readString(ls, usage);
	if (UNLIKELY(src_path == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to read the source path.");
	    goto end;
	    }
	dst_path = testobj_i_readString(ls, usage);
	if (UNLIKELY(dst_path == NULL))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to read the destination path for source \"%s\".",
		src_path
	    );
	    goto end;
	    }

	/** Open the objects, one using the other's type. **/
	if (strcmp(mode, "srctype") == 0)
	    {
	    src = objOpen(s, src_path, O_RDONLY, 0600, "application/octet-stream");
	    if (UNLIKELY(src == NULL))
		{
		mssError(0, "TESTOBJ", "Failed to open source \"%s\".", src_path);
		goto end;
		}
	    if (UNLIKELY(objGetAttrValue(src, "inner_type", DATA_T_STRING, POD(&type)) != 0))
		{
		mssError(0, "TESTOBJ", "Failed to get the inner_type of source \"%s\".", src_path);
		goto end;
		}
	    dst = objOpen(s, dst_path, O_RDWR | O_CREAT | O_TRUNC, 0600, type);
	    if (UNLIKELY(dst == NULL))
		{
		mssError(0, "TESTOBJ",
		    "Failed to open destination \"%s\" as \"%s\".",
		    dst_path, type
		);
		goto end;
		}
	    }
	else
	    {
	    dst = objOpen(s, dst_path, O_RDWR | O_CREAT | O_TRUNC, 0600, "application/octet-stream");
	    if (UNLIKELY(dst == NULL))
		{
		mssError(0, "TESTOBJ", "Failed to open destination \"%s\".", dst_path);
		goto end;
		}
	    if (UNLIKELY(objGetAttrValue(dst, "inner_type", DATA_T_STRING, POD(&type)) != 0))
		{
		mssError(0, "TESTOBJ",
		    "Failed to get the inner_type of destination \"%s\".",
		    dst_path
		);
		goto end;
		}
	    src = objOpen(s, src_path, O_RDONLY, 0600, type);
	    if (UNLIKELY(src == NULL))
		{
		mssError(0, "TESTOBJ", "Failed to open source \"%s\" as \"%s\".", src_path, type);
		goto end;
		}
	    }

	/** Copy the content. **/
	int count;
	while ((count = objRead(src, buf, sizeof(buf), 0, 0)) > 0)
	    {
	    if (UNLIKELY(objWrite(dst, buf, count, 0, 0) < 0))
		{
		mssError(0, "TESTOBJ", "Failed to write %d bytes to \"%s\".", count, dst_path);
		goto end;
		}
	    }
	if (UNLIKELY(count < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to read \"%s\".", src_path);
	    goto end;
	    }

	rval = 0;

    end:
	if (LIKELY(src != NULL)) warnNeg(objClose(src));
	if (LIKELY(dst != NULL)) warnNeg(objClose(dst));
	if (LIKELY(dst_path != NULL)) nmSysFree(dst_path);
	if (LIKELY(src_path != NULL)) nmSysFree(src_path);

	return rval;
    }


/*** testobj_i_setAttrFromLine - sets an attribute from a line of the create
 *** command's input, formatted as "name=value".  The value's type comes from
 *** the attribute, or for a new attribute, from the value.
 ***
 *** @param obj The object to set the attribute on.
 *** @param line The line of input.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_setAttrFromLine(pObject obj, char* line)
    {
    char buf[BUFF_SIZE];
    int int_val;
    double double_val;
    DateTime date_val;
    pDateTime date = &date_val;
    MoneyType money_val;
    pMoneyType money = &money_val;
    pObjData pod = NULL;
    int type;

	/** Split the name and value. **/
	if (UNLIKELY(strtcpy(buf, line, sizeof(buf)) < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to read line \"%s\": longer than %d characters.",
		line, (int)sizeof(buf) - 1
	    );
	    return -1;
	    }
	char* const name = strtok(buf, "=");
	char* value = strtok(NULL, "=");
	if (UNLIKELY(name == NULL || value == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to parse \"%s\" as <attribute>=<value>.", line);
	    return -1;
	    }
	while (*value == ' ') value++;
	size_t name_len = strlen(name);
	while (name_len > 0 && name[name_len - 1] == ' ') name[--name_len] = '\0';
	if (UNLIKELY(name_len == 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to find an attribute name before the \"=\" in \"%s\".",
		line
	    );
	    return -1;
	    }

	/** Convert the value. **/
	const int attr_type = objGetAttrType(obj, name);
	if (attr_type < 0 || attr_type == DATA_T_UNAVAILABLE)
	    {
	    mssClearError(); /* A new attribute. */
	    if (value[0] >= '0' && value[0] <= '9')
		{
		int_val = strtoi(value, NULL, 10);
		type = DATA_T_INTEGER;
		pod = POD(&int_val);
		}
	    else
		{
		type = DATA_T_STRING;
		pod = POD(&value);
		}
	    }
	else
	    {
	    type = attr_type;
	    switch (attr_type)
		{
		case DATA_T_INTEGER:
		    {
		    int_val = objDataToInteger(DATA_T_STRING, value, NULL);
		    pod = POD(&int_val);
		    break;
		    }
		case DATA_T_STRING:
		    {
		    pod = POD(&value);
		    break;
		    }
		case DATA_T_DOUBLE:
		    {
		    double_val = objDataToDouble(DATA_T_STRING, value);
		    pod = POD(&double_val);
		    break;
		    }
		case DATA_T_DATETIME:
		    {
		    if (UNLIKELY(objDataToDateTime(DATA_T_STRING, value, date, NULL) < 0))
			{
			mssError(0, "TESTOBJ",
			    "Failed to convert \"%s\" to a date for \"%s\".",
			    value, name
			);
			return -1;
			}
		    pod = POD(&date);
		    break;
		    }
		case DATA_T_MONEY:
		    {
		    if (UNLIKELY(objDataToMoney(DATA_T_STRING, value, money) < 0))
			{
			mssError(0, "TESTOBJ",
			    "Failed to convert \"%s\" to money for \"%s\".",
			    value, name
			);
			return -1;
			}
		    pod = POD(&money);
		    break;
		    }
		default:
		    {
		    mssError(1, "TESTOBJ",
			"Failed to set \"%s\" to \"%s\": unsupported type %d.",
			name, value, attr_type
		    );
		    return -1;
		    }
		}
	    }

	/** Set the attribute. **/
	if (UNLIKELY(objSetAttrValue(obj, name, type, pod) < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to set \"%s\" to \"%s\".", name, value);
	    return -1;
	    }

	return 0;
    }


/*** testobj_i_cmdCreate - creates an object for the create command, setting
 *** the attributes entered on the following lines until a blank line.  A path
 *** ending in "*" lets the driver name the object.
 ***
 *** @param s The OSML session.
 *** @param path The path of the new object, or NULL if none was given.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdCreate(pObjSession s, char* path)
    {
    int rval = -1;
    pObject obj = NULL;
    char* line = NULL;
    char* name = NULL;
    bool name_was_null = false;

	if (UNLIKELY(path == NULL))
	    {
	    mssError(1, "TESTOBJ", "Usage: create <object>");
	    goto end;
	    }

	/** Open the object. **/
	char* const star = strchr(path, '*');
	const bool auto_name = (star != NULL && star[1] == '\0');
	obj = objOpen(s, path, O_RDWR | O_CREAT | (auto_name ? OBJ_O_AUTONAME : 0), 0600, "system/object");
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to create \"%s\".", path);
	    goto end;
	    }

	/** Print the new name. **/
	if (strcmp(path, "*") == 0)
	    {
	    const int name_rval = objGetAttrValue(obj, "name", DATA_T_STRING, POD(&name));
	    if (UNLIKELY(name_rval < 0))
		{
		mssWarnError("Failed to get the name of new object \"%s\".", path);
		}
	    else if (name_rval == 1)
		{
		printf("New object name cannot yet be determined - please enter attributes first.\n");
		name_was_null = true;
		}
	    else
		{
		printf("New object name is '%s'\n", name);
		}
	    }

	/** Set attributes until a blank line or EOF. **/
	puts("Enter attributes, blank line to end.");
	warnFail(rl_bind_key('\t', rl_insert));
	while (true)
	    {
	    line = readline("");
	    if (line == NULL || line[0] == '\0') break;
	    if (UNLIKELY(testobj_i_setAttrFromLine(obj, line) < 0))
		{
		mssWarnError("Failed to set an attribute from \"%s\", skipping it.", line);
		}
	    free(line);
	    line = NULL;
	    }

	/** Print the name the driver chose. **/
	if (name_was_null)
	    {
	    const int name_rval = objGetAttrValue(obj, "name", DATA_T_STRING, POD(&name));
	    if (UNLIKELY(name_rval < 0))
		{
		mssWarnError("Failed to get the name of new object \"%s\".", path);
		}
	    else if (name_rval == 1)
		{
		printf("New object name cannot be determined - something went wrong.\n");
		}
	    else
		{
		printf("New object name is '%s'\n", name);
		}
	    }

	/** Save the object. **/
	const int close_rval = objClose(obj);
	obj = NULL;
	if (UNLIKELY(close_rval < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to save new object \"%s\".", path);
	    goto end;
	    }

	rval = 0;

    end:
	warnFail(rl_bind_key('\t', handle_tab));
	if (obj != NULL) warnNeg(objClose(obj));
	if (line != NULL) free(line);

	return rval;
    }


/*** testobj_i_cmdExec - calls a method for the command:
 *** exec <obj> <method> <parameter>
 ***
 *** @param s The OSML session.
 *** @param path The path of the object, or NULL if none was given.
 *** @param ls The command's lexer session, positioned after the path.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdExec(pObjSession s, char* path, pLxSession ls)
    {
    int rval = -1;
    char* const usage = "Usage: exec <obj> <method> <parameter>";
    char* method = NULL;
    char* param = NULL;
    pObject obj = NULL;

	/** Parse the arguments. **/
	if (UNLIKELY(path == NULL))
	    {
	    mssError(1, "TESTOBJ", "%s", usage);
	    goto end;
	    }
	method = testobj_i_readString(ls, usage);
	if (UNLIKELY(method == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to read the method name for \"%s\".", path);
	    goto end;
	    }
	param = testobj_i_readString(ls, usage);
	if (UNLIKELY(param == NULL))
	    {
	    mssError(0, "TESTOBJ",
		"Failed to read the parameter of method \"%s\" for \"%s\".",
		method, path
	    );
	    goto end;
	    }

	/** Call the method. **/
	obj = objOpen(s, path, O_RDONLY, 0600, "application/octet-stream");
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open \"%s\".", path);
	    goto end;
	    }
	if (UNLIKELY(objExecuteMethod(obj, method, POD(&param)) < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to execute %s(\"%s\") on \"%s\".", method, param, path);
	    goto end;
	    }

	rval = 0;

    end:
	if (LIKELY(obj != NULL)) warnNeg(objClose(obj));
	if (LIKELY(param != NULL)) nmSysFree(param);
	if (LIKELY(method != NULL)) nmSysFree(method);

	return rval;
    }


/*** testobj_i_cmdHints - prints presentation hints for the command:
 *** hints <obj> [attribute]
 ***
 *** @param s The OSML session.
 *** @param path The path of the object, or NULL for the working directory.
 *** @param ls The command's lexer session, positioned after the path.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdHints(pObjSession s, char* path, pLxSession ls)
    {
    int rval = -1;
    char* const obj_path = (path != NULL) ? path : "";
    char* attr_name = NULL;
    pObject obj = NULL;

	/** Read the optional attribute, which can only follow a path. **/
	if (path != NULL)
	    {
	    const int token = mlxNextToken(ls);
	    if (UNLIKELY(token == MLX_TOK_ERROR))
		{
		mssError(0, "TESTOBJ", "Failed to read the attribute name for \"%s\".", path);
		goto end;
		}
	    if (token == MLX_TOK_STRING)
		{
		if (UNLIKELY(mlxHoldToken(ls) < 0))
		    {
		    mssError(1, "TESTOBJ",
			"Failed to hold the attribute name token for \"%s\".",
			path
		    );
		    goto end;
		    }
		attr_name = testobj_i_readString(ls, "Usage: hints [<obj> [<attribute>]]");
		if (UNLIKELY(attr_name == NULL))
		    {
		    mssError(0, "TESTOBJ", "Failed to read the attribute name for \"%s\".", path);
		    goto end;
		    }
		}
	    }

	obj = objOpen(s, obj_path, O_RDONLY, 0600, "system/object");
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open \"%s\".", obj_path);
	    goto end;
	    }

	/** Print the hints. **/
	if (attr_name != NULL)
	    {
	    if (UNLIKELY(testobj_show_hints(obj, attr_name) < 0))
		{
		mssError(0, "TESTOBJ",
		    "Failed to print the hints for \"%s\" on \"%s\".",
		    attr_name, obj_path
		);
		goto end;
		}
	    }
	else
	    {
	    for (char* name = objGetFirstAttr(obj); name != NULL; name = objGetNextAttr(obj))
		{
		if (UNLIKELY(testobj_show_hints(obj, name) < 0))
		    {
		    mssWarnError(
			"Failed to print the hints for \"%s\" on \"%s\", skipping them.",
			name, obj_path
		    );
		    }
		}
	    }

	rval = 0;

    end:
	if (LIKELY(obj != NULL)) warnNeg(objClose(obj));
	if (attr_name != NULL) nmSysFree(attr_name);

	return rval;
    }


/*** testobj_i_cmdTrunc - truncates an object's content for the command:
 *** trunc <filename> <offset>
 ***
 *** @param s The OSML session.
 *** @param path The path of the object, or NULL if none was given.
 *** @param ls The command's lexer session, positioned after the path.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_cmdTrunc(pObjSession s, char* path, pLxSession ls)
    {
    int rval = -1;
    pObject obj = NULL;

	if (UNLIKELY(path == NULL || mlxNextToken(ls) != MLX_TOK_INTEGER))
	    {
	    mssError(1, "TESTOBJ", "Usage: trunc <filename> <offset>");
	    goto end;
	    }
	const int offset = mlxIntVal(ls);

	/** Truncate the content. **/
	obj = objOpen(s, path, O_RDWR, 0600, "system/object");
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to open \"%s\".", path);
	    goto end;
	    }
	if (UNLIKELY(objWrite(obj, "", 0, offset, OBJ_U_SEEK | OBJ_U_TRUNCATE) < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to truncate \"%s\" to %d bytes.", path, offset);
	    goto end;
	    }

	rval = 0;

    end:
	if (LIKELY(obj != NULL)) warnNeg(objClose(obj));

	return rval;
    }


/*** testobj_do_cmd - runs one command line.
 ***
 *** @param s The OSML session.
 *** @param cmd The command line.
 *** @param batch_mode Unused.
 *** @param inp_lx The command file that cmd came from, which mlquery reads
 *** 	more lines from, or NULL to read them from the terminal.
 *** @returns 0 on success (including a blank or comment line), 1 to quit, or
 *** 	-1 on failure.
 ***/
int
testobj_do_cmd(pObjSession s, char* cmd, int batch_mode, pLxSession inp_lx)
    {
    int rval = -1;
    pLxSession ls = NULL;
    char cmdname[64];
    char first_arg[256];
    char* arg = NULL;

	if (cmd[0] == '#') return 0;

	ls = mlxStringSession(cmd, MLX_F_ICASE | MLX_F_EOF);
	if (UNLIKELY(ls == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to open a lexer for command \"%s\".", cmd);
	    goto end;
	    }

	/** Read the command name. **/
	const int name_token = mlxNextToken(ls);
	if (name_token == MLX_TOK_EOF || name_token == MLX_TOK_POUND)
	    {
	    rval = 0; /* A blank or indented comment line. */
	    goto end;
	    }
	if (UNLIKELY(name_token != MLX_TOK_KEYWORD))
	    {
	    mssError((name_token == MLX_TOK_ERROR) ? 0 : 1, "TESTOBJ",
		"Failed to read a command name from \"%s\".",
		cmd
	    );
	    goto end;
	    }
	char* const name = mlxStringVal(ls, NULL);
	if (UNLIKELY(name == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to read a command name from \"%s\".", cmd);
	    goto end;
	    }
	if (UNLIKELY(strtcpy(cmdname, name, sizeof(cmdname)) < 0))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to run unknown command \"%s\". Run \"help\" to list commands.",
		name
	    );
	    goto end;
	    }

	/** Read the first argument, if any. **/
	if (UNLIKELY(mlxSetOptions(ls, MLX_F_IFSONLY) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to set lexer options for command \"%s\".", cmd);
	    goto end;
	    }
	const int arg_token = mlxNextToken(ls);
	if (UNLIKELY(arg_token == MLX_TOK_ERROR))
	    {
	    mssError(0, "TESTOBJ", "Failed to read the first argument of command \"%s\".", cmd);
	    goto end;
	    }
	if (arg_token == MLX_TOK_STRING)
	    {
	    char* const arg_text = mlxStringVal(ls, NULL);
	    if (UNLIKELY(arg_text == NULL))
		{
		mssError(0, "TESTOBJ", "Failed to read the first argument of command \"%s\".", cmd);
		goto end;
		}

	    /** Copy it: the next token read reuses the lexer's buffer. **/
	    if (UNLIKELY(strtcpy(first_arg, arg_text, sizeof(first_arg)) < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to copy the first argument of command \"%s\": longer than %d characters.",
		    cmd, (int)sizeof(first_arg) - 1
		);
		goto end;
		}
	    arg = first_arg;
	    }
	if (UNLIKELY(mlxUnsetOptions(ls, MLX_F_IFSONLY) < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to unset lexer options for command \"%s\".", cmd);
	    goto end;
	    }

	/** Run the command. **/
	if (strcmp(cmdname, "cd") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdCd(s, arg) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "csv") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdCsv(s, (arg != NULL) ? cmd + 4 : NULL) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "query") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdQuery(s, (arg != NULL) ? cmd + 6 : NULL) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "mlquery") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdMlquery(s, (arg != NULL) ? cmd + 8 : NULL, inp_lx) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "annot") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdAnnot(s, arg, ls) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "list") == 0 || strcmp(cmdname, "ls") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdList(s, arg, ls) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "show") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdShow(s, (arg != NULL) ? arg : "", "system/object", false, true) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "printshow") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdShow(s, (arg != NULL) ? arg : "", "system/object", true, true) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "print") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdShow(s, (arg != NULL) ? arg : "", "text/plain", true, false) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "copy") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdCopy(s, arg, ls) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "delete") == 0)
	    {
	    if (UNLIKELY(arg == NULL))
		{
		mssError(1, "TESTOBJ", "Usage: delete <object>");
		goto end;
		}
	    if (UNLIKELY(objDelete(s, arg) < 0))
		{
		mssError(0, "TESTOBJ", "Failed to delete \"%s\".", arg);
		goto end;
		}
	    }
	else if (strcmp(cmdname, "create") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdCreate(s, arg) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "echo") == 0)
	    {
	    char* const text = (arg != NULL) ? cmd + 5 : "";
	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, "%.*s\n", (int)strcspn(text, "\r\n"), text) < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write \"%.*s\" to output file \"%s\".",
		    (int)strcspn(text, "\r\n"), text, TESTOBJ.OutputFilename
		);
		goto end;
		}
	    }
	else if (strcmp(cmdname, "mem") == 0)
	    {
	    nmStats();
	    nmDeltas();
	    }
	else if (strcmp(cmdname, "pwd") == 0)
	    {
	    char* const wd = objGetWD(s);
	    if (UNLIKELY(wd == NULL))
		{
		mssError(1, "TESTOBJ", "Failed to get the working directory.");
		goto end;
		}
	    if (UNLIKELY(fdPrintf(TESTOBJ.Output, "%s\n", wd) < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to write working directory \"%s\" to output file \"%s\".",
		    wd, TESTOBJ.OutputFilename
		);
		goto end;
		}
	    }
	else if (strcmp(cmdname, "quit") == 0 || strcmp(cmdname, "exit") == 0)
	    {
	    rval = 1;
	    goto end;
	    }
	else if (strcmp(cmdname, "exec") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdExec(s, arg, ls) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "hints") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdHints(s, arg, ls) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "output") == 0)
	    {
	    char* const filename = (arg != NULL) ? arg : "/dev/tty";
	    const pFile output_file = fdOpen(filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
	    if (UNLIKELY(output_file == NULL))
		{
		mssErrorErrno(1, "TESTOBJ", "Failed to open output file \"%s\"", filename);
		goto end;
		}
	    if (TESTOBJ.Output != NULL) warnNeg(fdClose(TESTOBJ.Output, 0));
	    set_output(output_file);
	    if (UNLIKELY(strtcpy(TESTOBJ.OutputFilename, filename, sizeof(TESTOBJ.OutputFilename)) < 0))
		{
		fprintf(stderr,
		    "Warning: Failed to store the full output filename \"%s\", so it is shown as \"%s\".\n",
		    filename, TESTOBJ.OutputFilename
		);
		}
	    }
	else if (strcmp(cmdname, "obfuscate") == 0)
	    {
	    if (TESTOBJ.ObfuscationSession != NULL)
		{
		warnNeg(obfCloseSession(TESTOBJ.ObfuscationSession));
		TESTOBJ.ObfuscationSession = NULL;
		}

	    /** Split the key and rule file. **/
	    if (UNLIKELY(strtcpy(TESTOBJ.ObfKey, (arg != NULL) ? arg : "", sizeof(TESTOBJ.ObfKey)) < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to read the obfuscation key: longer than %d characters.",
		    (int)sizeof(TESTOBJ.ObfKey) - 1
		);
		goto end;
		}
	    char* const comma = strchr(TESTOBJ.ObfKey, ',');
	    if (comma != NULL)
		{
		*comma = '\0';
		if (UNLIKELY(strtcpy(TESTOBJ.ObfRuleFile, comma + 1, sizeof(TESTOBJ.ObfRuleFile)) < 0))
		    {
		    mssError(1, "TESTOBJ",
			"Failed to read rule file \"%s\": longer than %d characters.",
			comma + 1, (int)sizeof(TESTOBJ.ObfRuleFile) - 1
		    );
		    goto end;
		    }
		}

	    /** An empty key leaves obfuscation off. **/
	    if (TESTOBJ.ObfKey[0] != '\0')
		{
		TESTOBJ.ObfuscationSession = obfOpenSession(s, TESTOBJ.ObfRuleFile, TESTOBJ.ObfKey);
		if (UNLIKELY(TESTOBJ.ObfuscationSession == NULL))
		    {
		    mssError(0, "TESTOBJ",
			"Failed to start obfuscation with rule file \"%s\".",
			TESTOBJ.ObfRuleFile
		    );
		    goto end;
		    }
		}
	    }
	else if (strcmp(cmdname, "crash") == 0)
	    {
	    if (UNLIKELY(raise(SIGSEGV) != 0))
		{
		mssError(1, "TESTOBJ", "Failed to crash with SIGSEGV.");
		goto end;
		}
	    }
	else if (strcmp(cmdname, "sleep") == 0)
	    {
	    if (UNLIKELY(arg == NULL))
		{
		mssError(1, "TESTOBJ", "Usage: sleep <seconds>");
		goto end;
		}
	    char* num_end = NULL;
	    const int seconds = strtoi(arg, &num_end, 10);
	    if (UNLIKELY(num_end == arg || *num_end != '\0' || seconds < 0 || seconds == INT_MAX))
		{
		mssError(1, "TESTOBJ",
		    "Failed to parse \"%s\" as a number of seconds below %d.",
		    arg, INT_MAX
		);
		goto end;
		}
	    sleep(seconds);
	    }
	else if (strcmp(cmdname, "test") == 0)
	    {
	    setup_test_ids((arg != NULL) ? arg : "");
	    }
	else if (strcmp(cmdname, "trunc") == 0)
	    {
	    if (UNLIKELY(testobj_i_cmdTrunc(s, arg, ls) < 0)) goto end;
	    }
	else if (strcmp(cmdname, "help") == 0)
	    {
	    printf("Available Commands:\n");
	    printf("  annot     - Add or change the annotation on an object.\n");
	    printf("  cd        - Change the current working \"directory\" in the objectsystem.\n");
	    printf("  copy      - Copy one object's content to another.\n");
	    printf("  create    - Create a new object.\n");
	    printf("  csv       - Run a SQL query and print the results in CSV format.\n");
	    printf("  delete    - Delete an object.\n");
	    printf("  echo      - Print a line of text.\n");
	    printf("  exec      - Call a method on an object.\n");
	    printf("  hints     - Show the presentation hints of an attribute (or object)\n");
	    printf("  help      - Displays this help screen.\n");
	    printf("  list, ls  - Lists the objects in the current \"directory\" in the objectsystem.\n");
	    printf("  mem       - Print memory statistics and the allocation changes since the last mem.\n");
	    printf("  mlquery   - Runs a SQL query, reading in multiple lines until a blank line.\n");
	    printf("  obfuscate - Begins obfuscation of CSV and query output, given an obfuscation key and optional rule file\n");
	    printf("  output    - Change where output goes.\n");
	    printf("  print     - Displays an object's content.\n");
	    printf("  printshow - Displays an object's content, followed by its attributes and methods.\n");
	    printf("  pwd       - Print the current working \"directory\" in the objectsystem.\n");
	    printf("  query     - Runs a SQL query.\n");
	    printf("  quit/exit - Exits this application.\n");
	    printf("  show      - Displays an object's attributes and methods.\n");
	    printf("  test      - Enables test suite output for the given list of ids.\n");
	    printf("  trunc     - Truncates an object's content to a given point.\n");
	    }
	else
	    {
	    mssError(1, "TESTOBJ",
		"Failed to run unknown command \"%s\". Run \"help\" to list commands.",
		cmdname
	    );
	    goto end;
	    }

	rval = 0;

    end:
	if (LIKELY(ls != NULL)) warnNeg(mlxCloseSession(ls));

	return rval;
    }


/*** testobj_i_copyLine - copies the current token of a line-only lexer,
 *** including its line ending.
 ***
 *** @param lx The lexer, opened with MLX_F_LINEONLY.
 *** @returns The line, which the caller frees with nmSysFree(), or NULL on
 *** 	failure.
 ***/
static char*
testobj_i_copyLine(pLxSession lx)
    {
	int alloc = 0;
	char* line = mlxStringVal(lx, &alloc);
	if (UNLIKELY(line == NULL))
	    {
	    mssError(0, "TESTOBJ", "Failed to get the text of a line.");
	    return NULL;
	    }

	/** Own the text, since the lexer may reuse its buffer for the next token. **/
	if (alloc != 1)
	    {
	    char* const copy = nmSysStrdup(line);
	    if (UNLIKELY(copy == NULL))
		{
		mssError(1, "TESTOBJ", "Failed to copy line \"%s\".", line);
		return NULL;
		}
	    line = copy;
	    }

	return line;
    }


/*** testobj_i_openHistory - opens the history file and loads its lines into
 *** the readline history.  Failures print a warning, since history is
 *** optional.
 ***
 *** @returns The open history file, or NULL if history is unavailable.
 ***/
static pFile
testobj_i_openHistory(void)
    {
    bool successful = false;
    pFile histfile = NULL;
    pLxSession lx = NULL;
    char histname[256];

	/** Find the file. **/
	const char* const home = getenv("HOME");
	if (UNLIKELY(home == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to find the history file because HOME is not set.");
	    goto end;
	    }
	const int len = snprintf(histname, sizeof(histname), "%s/.cxhistory", home);
	if (UNLIKELY(len < 0 || (size_t)len >= sizeof(histname)))
	    {
	    mssError(1, "TESTOBJ", "Failed to build the history file path from HOME \"%s\".", home);
	    goto end;
	    }

	/** Open the file. **/
	histfile = fdOpen(histname, O_RDWR | O_CREAT, 0600);
	if (UNLIKELY(histfile == NULL))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to open history file \"%s\": %s",
		histname, strerror(errno)
	    );
	    goto end;
	    }
	lx = mlxOpenSession(histfile, MLX_F_LINEONLY | MLX_F_EOF);
	if (UNLIKELY(lx == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to start reading history file \"%s\".", histname);
	    goto end;
	    }

	/** Load the lines. **/
	while (true)
	    {
	    const int t = mlxNextToken(lx);
	    if (t == MLX_TOK_EOF) break;
	    if (UNLIKELY(t == MLX_TOK_ERROR))
		{
		mssError(0, "TESTOBJ", "Failed to read history file \"%s\".", histname);
		goto end;
		}

	    char* const line = testobj_i_copyLine(lx);
	    if (UNLIKELY(line == NULL))
		{
		mssError(0, "TESTOBJ", "Failed to read a line of history file \"%s\".", histname);
		goto end;
		}
	    line[strcspn(line, "\r\n")] = '\0';
	    if (line[0] != '\0') add_history(line);
	    nmSysFree(line);
	    }

	successful = true;

    end:
	/** Clean up. **/
	if (LIKELY(lx != NULL)) warnNeg(mlxCloseSession(lx));

	/** Appending after a partial read could corrupt the file. **/
	if (UNLIKELY(!successful))
	    {
	    mssWarnError("Failed to load the command history.");
	    if (histfile != NULL) warnNeg(fdClose(histfile, 0));
	    histfile = NULL;
	    }

	return histfile;
    }


/*** testobj_i_readPassword - reads the password from the password file into
 *** TESTOBJ.Password, without its line ending.
 ***
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
testobj_i_readPassword(void)
    {
    bool successful = false;
    pFile pwfile = NULL;
    char buf[sizeof(TESTOBJ.Password) + 2];

	pwfile = fdOpen(TESTOBJ.PasswordFile, O_RDONLY, 0600);
	if (UNLIKELY(pwfile == NULL))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to open password file \"%s\": %s",
		TESTOBJ.PasswordFile, strerror(errno)
	    );
	    goto end;
	    }

	/** Read until end of file, with room for a line ending. **/
	const int max_len = sizeof(buf) - 1;
	int len = 0;
	while (len < max_len)
	    {
	    const int read_len = fdRead(pwfile, buf + len, max_len - len, 0, 0);
	    if (read_len == 0) break;
	    if (UNLIKELY(read_len < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to read password file \"%s\".",
		    TESTOBJ.PasswordFile
		);
		goto end;
		}
	    len += read_len;
	    }
	if (len == max_len)
	    {
	    char extra = '\0';
	    const int extra_len = fdRead(pwfile, &extra, 1, 0, 0);
	    cxssShred(&extra, sizeof(extra));
	    if (UNLIKELY(extra_len < 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to read password file \"%s\".",
		    TESTOBJ.PasswordFile
		);
		goto end;
		}
	    if (UNLIKELY(extra_len > 0))
		{
		mssError(1, "TESTOBJ",
		    "Failed to read password file \"%s\" because it is over %d bytes.",
		    TESTOBJ.PasswordFile, max_len
		);
		goto end;
		}
	    }

	/** Strip the line ending. **/
	buf[len] = '\0';
	if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
	if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';

	/** Store the password. **/
	if (UNLIKELY(strtcpy(TESTOBJ.Password, buf, sizeof(TESTOBJ.Password)) < 0))
	    {
	    cxssShred(TESTOBJ.Password, sizeof(TESTOBJ.Password));
	    mssError(1, "TESTOBJ",
		"Failed to use the password in \"%s\" because it is over %d characters.",
		TESTOBJ.PasswordFile, (int)sizeof(TESTOBJ.Password) - 1
	    );
	    goto end;
	    }

	successful = true;

    end:
	/** Clean up. **/
	cxssShred(buf, sizeof(buf));
	if (LIKELY(pwfile != NULL)) warnNeg(fdClose(pwfile, 0));

	return (successful) ? 0 : -1;
    }


/*** start - the main thread.  Initializes Centrallix, logs in, and runs the
 *** -C command, the -f command file, or the interactive prompt.  Never
 *** returns: exits with status 1 if startup or the command file fails.
 *** Failed commands print a warning and do not stop the run, unless -e
 *** was given.
 ***
 *** @param unused Unused.
 ***/
void
start(void* unused)
    {
    bool successful = false;
    char* user_buf = NULL;
    pFile histfile = NULL;
    pApplication app = NULL;
    bool pushed_context = false;
    pFile cmdfile = NULL;
    pLxSession cmd_lx = NULL;
    char* inbuf = NULL;
    char prompt[1024];
    const bool interactive = (TESTOBJ.Command[0] == '\0' && TESTOBJ.CmdFile[0] == '\0');

	/** Initialize. **/
	if (UNLIKELY(cxInitialize() < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to initialize Centrallix.");
	    goto end;
	    }
	if (UNLIKELY(cxDriverInit() < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to initialize the drivers.");
	    goto end;
	    }
	if (interactive) histfile = testobj_i_openHistory();

	/** Enable tab completion. **/
	if (UNLIKELY(rl_bind_key('\t', handle_tab) != 0))
	    fprintf(stderr, "Warning: Failed to bind tab completion.\n");

	/** Authenticate. **/
	char* user = TESTOBJ.UserName;
	if (user[0] == '\0')
	    {
	    user_buf = readline("Username: ");
	    user = (user_buf != NULL) ? user_buf : "";
	    }
	if (UNLIKELY(TESTOBJ.PasswordFile[0] != '\0' && testobj_i_readPassword() < 0))
	    {
	    mssWarnError("Failed to use password file \"%s\".", TESTOBJ.PasswordFile);
	    }
	char* pwd = TESTOBJ.Password;
	char* typed_pwd = NULL;
	if (pwd[0] == '\0')
	    {
	    typed_pwd = getpass("Password: ");
	    pwd = (typed_pwd != NULL) ? typed_pwd : "";
	    }
	const bool authenticated = (mssAuthenticate(user, pwd, 0) >= 0);
	cxssShred(TESTOBJ.Password, sizeof(TESTOBJ.Password));
	if (typed_pwd != NULL) cxssShred(typed_pwd, strlen(typed_pwd));
	if (UNLIKELY(!authenticated))
	    {
	    mssWarnError(
		"Failed to authenticate user \"%s\", so running outside a session context.",
		user
	    );
	    }

	/** Open the output, falling back to the terminal, stdout, then nowhere. **/
	const bool output_requested = (strcmp(TESTOBJ.OutputFilename, "/dev/tty") != 0);
	pFile outfile = fdOpen(TESTOBJ.OutputFilename, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (UNLIKELY(outfile == NULL && output_requested))
	    {
	    fprintf(stderr,
		"Warning: Failed to open output file \"%s\": %s\n",
		TESTOBJ.OutputFilename, strerror(errno)
	    );
	    }
	if (outfile == NULL)
	    {
	    strcpy(TESTOBJ.OutputFilename, "/dev/tty");
	    outfile = fdOpen(TESTOBJ.OutputFilename, O_RDWR, 0600);
	    }
	if (outfile == NULL)
	    {
	    strcpy(TESTOBJ.OutputFilename, "/dev/stdout");
	    outfile = fdOpen(TESTOBJ.OutputFilename, O_WRONLY, 0600);
	    }
	if (outfile == NULL)
	    {
	    strcpy(TESTOBJ.OutputFilename, "/dev/null");
	    outfile = fdOpen(TESTOBJ.OutputFilename, O_RDWR, 0600);
	    }
	if (UNLIKELY(outfile == NULL))
	    {
	    mssError(1, "TESTOBJ",
		"Failed to open any output file, including \"/dev/null\": %s",
		strerror(errno)
	    );
	    goto end;
	    }
	set_output(outfile);

	/** Application context. **/
	if (UNLIKELY(cxssPushContext() < 0))
	    {
	    mssError(1, "TESTOBJ", "Failed to push a security context.");
	    goto end;
	    }
	pushed_context = true;
	app = appCreate("test_obj");
	if (UNLIKELY(app == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to create application \"test_obj\".");
	    goto end;
	    }
	if (UNLIKELY(cxssAddEndorsement("system:from_application", "*") < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to add endorsement \"system:from_application\".");
	    goto end;
	    }
	if (UNLIKELY(cxssAddEndorsement("system:from_appgroup", "*") < 0))
	    {
	    mssError(0, "TESTOBJ", "Failed to add endorsement \"system:from_appgroup\".");
	    goto end;
	    }

	/** Open a session. **/
	s = objOpenSession("/");
	if (UNLIKELY(s == NULL))
	    {
	    mssError(1, "TESTOBJ", "Failed to open an OSML session at \"/\".");
	    goto end;
	    }

	/** Obfuscation from -O. **/
	if (TESTOBJ.ObfKey[0] != '\0')
	    {
	    TESTOBJ.ObfuscationSession = obfOpenSession(s, TESTOBJ.ObfRuleFile, TESTOBJ.ObfKey);
	    if (UNLIKELY(TESTOBJ.ObfuscationSession == NULL))
		{
		mssError(0, "TESTOBJ",
		    "Failed to start obfuscation with rule file \"%s\".",
		    TESTOBJ.ObfRuleFile
		);
		goto end;
		}
	    }

	/** Command from -C. **/
	if (TESTOBJ.Command[0] != '\0')
	    {
	    if (UNLIKELY(testobj_do_cmd(s, TESTOBJ.Command, 1, NULL) < 0))
		{
		if (TESTOBJ.StopOnError)
		    {
		    mssError(0, "TESTOBJ", "Failed to run command \"%s\".", TESTOBJ.Command);
		    goto end;
		    }
		mssWarnError("Failed to run command \"%s\".", TESTOBJ.Command);
		}
	    }

	/** Command file from -f. **/
	if (TESTOBJ.CmdFile[0] != '\0')
	    {
	    cmdfile = fdOpen(TESTOBJ.CmdFile, O_RDONLY, 0600);
	    if (UNLIKELY(cmdfile == NULL))
		{
		mssError(1, "TESTOBJ",
		    "Failed to open command file \"%s\": %s",
		    TESTOBJ.CmdFile, strerror(errno)
		);
		goto end;
		}
	    cmd_lx = mlxOpenSession(cmdfile, MLX_F_LINEONLY | MLX_F_EOF);
	    if (UNLIKELY(cmd_lx == NULL))
		{
		mssError(1, "TESTOBJ",
		    "Failed to start reading command file \"%s\".",
		    TESTOBJ.CmdFile
		);
		goto end;
		}
	    while (true)
		{
		const int t = mlxNextToken(cmd_lx);
		if (t == MLX_TOK_EOF) break;
		if (UNLIKELY(t == MLX_TOK_ERROR))
		    {
		    mssError(0, "TESTOBJ", "Failed to read command file \"%s\".", TESTOBJ.CmdFile);
		    goto end;
		    }

		char* const line = testobj_i_copyLine(cmd_lx);
		if (UNLIKELY(line == NULL))
		    {
		    mssError(0, "TESTOBJ",
			"Failed to read a line of command file \"%s\".",
			TESTOBJ.CmdFile
		    );
		    goto end;
		    }
		const int cmd_rval = testobj_do_cmd(s, line, 1, cmd_lx);
		if (UNLIKELY(cmd_rval < 0 && TESTOBJ.StopOnError))
		    {
		    mssError(0, "TESTOBJ",
			"Failed to run command \"%.*s\".",
			(int)strcspn(line, "\r\n"), line
		    );
		    }
		else if (UNLIKELY(cmd_rval < 0))
		    {
		    mssWarnError(
			"Failed to run command \"%.*s\".",
			(int)strcspn(line, "\r\n"), line
		    );
		    }
		nmSysFree(line);
		if (UNLIKELY(cmd_rval < 0 && TESTOBJ.StopOnError)) goto end;
		if (cmd_rval == 1) break;
		}
	    }

	/** Interactive prompt. **/
	if (interactive)
	    {
	    while (true)
		{
		char* const wd = objGetWD(s);
		if (UNLIKELY(wd == NULL))
		    {
		    mssError(1, "TESTOBJ", "Failed to get the working directory for the prompt.");
		    goto end;
		    }
		if (UNLIKELY(snprintf(prompt, sizeof(prompt), "OSML:%.1000s> ", wd) < 0))
		    {
		    mssError(1, "TESTOBJ", "Failed to build the prompt for \"%s\".", wd);
		    goto end;
		    }

		/** NULL means end of input. **/
		inbuf = readline(prompt);
		if (inbuf == NULL)
		    {
		    printf("quit\n");
		    break;
		    }

		/** Save the line in the history. **/
		if (inbuf[0] != '\0')
		    {
		    add_history(inbuf);
		    if (UNLIKELY(histfile != NULL && fdPrintf(histfile, "%s\n", inbuf) < 0))
			{
			fprintf(stderr,
			    "Warning: Failed to save \"%s\" to the history file.\n",
			    inbuf
			);
			}
		    }

		const int cmd_rval = testobj_do_cmd(s, inbuf, 0, NULL);
		if (UNLIKELY(cmd_rval < 0)) mssWarnError("Failed to run command \"%s\".", inbuf);
		free(inbuf);
		inbuf = NULL;
		if (cmd_rval == 1) break;
		}
	    }

	successful = true;

    end:
	if (UNLIKELY(!successful)) mssWarnError("Failed to run test_obj, so exiting.");

	/** Clean up. **/
	cxssShred(TESTOBJ.Password, sizeof(TESTOBJ.Password));
	if (inbuf != NULL) free(inbuf);
	if (cmd_lx != NULL) warnNeg(mlxCloseSession(cmd_lx));
	if (cmdfile != NULL) warnNeg(fdClose(cmdfile, 0));
	if (LIKELY(app != NULL)) warnNeg(appDestroy(app));
	if (LIKELY(pushed_context)) warnNeg(cxssPopContext());
	if (LIKELY(s != NULL))
	    {
	    warnNeg(objCloseSession(s));
	    s = NULL;
	    }
	if (histfile != NULL) warnNeg(fdClose(histfile, 0));
	if (user_buf != NULL) free(user_buf);

	if (UNLIKELY(!successful)) exit(1);
	thExit();
    }


void
show_usage()
    {
    printf("Usage:  test_obj [-c <config-file>] [-f <command-file>] [-C <command>]\n"
	   "                 [-u <user>] [-p <password>] [-P <read-password-from-file>]\n"
	   "                 [-o <output file>] [-O obfkey[,obfrulefile] ]\n"
	   "                 [-i <wait-seconds>] [-t id,... ] [-e] [-h] [-q]\n"
	   "        -c file       Specify configuration file\n"
	   "        -C command    Run a single command\n"
	   "        -e            Stop at the first failed command and exit with status 1\n"
	   "        -f file       Run commands from a file\n"
	   "        -h            Show this message\n"
	   "        -i secs       Terminate test_obj after secs with SIGALRM (for test suite purposes)\n"
	   "        -o file       Send output to specified file\n"
	   "        -O key[,file] Obfuscate CSV and query output data with a given key and optional rule file\n"
	   "        -p pass       Specify password\n"
	   "        -P file       Read user password from file\n"
	   "        -q            Initialize and run quietly (minimal output)\n"
	   "        -t id,...     Show testing output for test logging IDs (for debugging and running test suites)\n"
	   "        -u user       Login as user\n"
	   "\n");
    return;
    }


/*** testobj_i_copyOption - copies the value of a command line option into a
 *** buffer, printing an error to stderr if it does not fit.
 ***
 *** @param option The option letter.
 *** @param value The value of the option.
 *** @param dst The buffer to copy into.
 *** @param dst_size The size of dst.
 *** @returns 0 on success, or -1 if the value does not fit.
 ***/
static int
testobj_i_copyOption(char option, char* value, char* dst, size_t dst_size)
    {
	if (UNLIKELY(strtcpy(dst, value, dst_size) < 0))
	    {
	    fprintf(stderr,
		"Error: Failed to use -%c \"%s\" because it is over %zu characters.\n",
		option, value, dst_size - 1
	    );
	    return -1;
	    }

	return 0;
    }


/*** main - reads the command line options, then runs start() as the first
 *** thread.
 ***
 *** @param argc The number of arguments.
 *** @param argv The arguments.
 *** @returns 0 on success, or 1 if an option is invalid or startup fails.
 ***/
int
main(int argc, char* argv[])
    {
    int rval = 1;

	/** Initial setup. **/
	cxSetupGlobals(argc, argv);
	memset(&TESTOBJ, 0, sizeof(TESTOBJ));
	strcpy(TESTOBJ.OutputFilename, "/dev/tty");

	/** Read the options, printing errors directly since mss is not initialized yet. **/
	int ch;
	while ((ch = getopt(argc, argv, "ho:c:qu:p:f:C:i:O:t:P:e")) > 0)
	    {
	    switch (ch)
		{
		case 'i':
		    {
		    char* parse_end = NULL;
		    errno = 0;
		    const long secs = strtol(optarg, &parse_end, 10);
		    if (UNLIKELY(parse_end == optarg || *parse_end != '\0' || errno != 0 || secs < 0 || secs > UINT_MAX))
			{
			fprintf(stderr,
			    "Error: Failed to use -i \"%s\" because it is not a whole number of seconds.\n",
			    optarg
			);
			goto end;
			}
		    TESTOBJ.WaitSecs = (unsigned int)secs;
		    break;
		    }
		case 'C':
		    {
		    if (UNLIKELY(testobj_i_copyOption('C', optarg, TESTOBJ.Command, sizeof(TESTOBJ.Command)) < 0)) goto end;
		    break;
		    }
		case 'f':
		    {
		    if (UNLIKELY(testobj_i_copyOption('f', optarg, TESTOBJ.CmdFile, sizeof(TESTOBJ.CmdFile)) < 0)) goto end;
		    break;
		    }
		case 'u':
		    {
		    if (UNLIKELY(testobj_i_copyOption('u', optarg, TESTOBJ.UserName, sizeof(TESTOBJ.UserName)) < 0)) goto end;
		    break;
		    }
		case 'p':
		    {
		    if (UNLIKELY(strtcpy(TESTOBJ.Password, optarg, sizeof(TESTOBJ.Password)) < 0))
			{
			fprintf(stderr,
			    "Error: Failed to use -p because the password is over %zu characters.\n",
			    sizeof(TESTOBJ.Password) - 1
			);
			goto end;
			}
		    break;
		    }
		case 'P':
		    {
		    if (UNLIKELY(testobj_i_copyOption('P', optarg, TESTOBJ.PasswordFile, sizeof(TESTOBJ.PasswordFile)) < 0)) goto end;
		    break;
		    }
		case 'c':
		    {
		    if (UNLIKELY(testobj_i_copyOption('c', optarg, CxGlobals.ConfigFileName, sizeof(CxGlobals.ConfigFileName)) < 0)) goto end;
		    break;
		    }
		case 'q':
		    {
		    CxGlobals.QuietInit = 1;
		    break;
		    }
		case 'e':
		    {
		    TESTOBJ.StopOnError = true;
		    break;
		    }
		case 'o':
		    {
		    if (UNLIKELY(testobj_i_copyOption('o', optarg, TESTOBJ.OutputFilename, sizeof(TESTOBJ.OutputFilename)) < 0)) goto end;
		    break;
		    }
		case 'O':
		    {
		    if (UNLIKELY(strtcpy(TESTOBJ.ObfKey, optarg, sizeof(TESTOBJ.ObfKey)) < 0))
			{
			fprintf(stderr,
			    "Error: Failed to use -O because the key and rule file are over %zu characters.\n",
			    sizeof(TESTOBJ.ObfKey) - 1
			);
			goto end;
			}
		    char* const comma = strchr(TESTOBJ.ObfKey, ',');
		    if (comma != NULL)
			{
			if (UNLIKELY(testobj_i_copyOption('O', comma + 1, TESTOBJ.ObfRuleFile, sizeof(TESTOBJ.ObfRuleFile)) < 0)) goto end;
			*comma = '\0';
			}
		    break;
		    }
		case 't':
		    {
		    setup_test_ids(optarg);
		    break;
		    }
		case 'h':
		    {
		    show_usage();
		    rval = 0;
		    goto end;
		    }
		default:
		    {
		    show_usage();
		    goto end;
		    }
		}
	    }

	/** Skip the time limit at the interactive prompt. **/
	if (UNLIKELY(TESTOBJ.WaitSecs != 0 && TESTOBJ.Command[0] == '\0' && TESTOBJ.CmdFile[0] == '\0'))
	    {
	    fprintf(stderr,
		"Warning: -i specified but no command or command file given.  Forcing -i to 0.\n"
	    );
	    TESTOBJ.WaitSecs = 0;
	    }
	if (TESTOBJ.WaitSecs != 0) alarm(TESTOBJ.WaitSecs);

	/** Run start(). **/
	if (UNLIKELY(mtInitialize((CxGlobals.QuietInit != 0) ? MT_F_QUIET : 0, start) == NULL))
	    {
	    fprintf(stderr, "Error: Failed to initialize the thread system.\n");
	    goto end;
	    }

	rval = 0;

    end:
	/** Clean up. **/
	cxssShred(TESTOBJ.Password, sizeof(TESTOBJ.Password));

	return rval;
    }
