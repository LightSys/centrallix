#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "obj.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
#ifdef HAVE_READLINE
/* Some versions of readline get upset if HAVE_CONFIG_H is defined! */
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#include <readline/readline.h>
#define HAVE_CONFIG_H
#else
#include <readline/readline.h>
#endif
#include <readline/history.h>
#endif
#ifndef CENTRALLIX_CONFIG
#define CENTRALLIX_CONFIG /usr/local/etc/centrallix.conf
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

/**CVSDATA***************************************************************

    $Id: test_obj.c,v 1.36 2007/02/17 04:34:51 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/test_obj.c,v $

    $Log: test_obj.c,v $
    Revision 1.36  2007/02/17 04:34:51  gbeeley
    - (bugfix) test_obj should open destination objects with O_TRUNC
    - (bugfix) prtmgmt should remember 'configured' line height, so it can
      auto-adjust height only if the line height is not explicitly set.
    - (change) report writer should assume some default margin settings on
      tables/table cells, so that tables aren't by default ugly :)
    - (bugfix) various floating point comparison fixes
    - (feature) allow top/bottom/left/right border options on the entire table
      itself in a report.
    - (feature) allow setting of text line height with "lineheight" attribute
    - (change) allow table to auto-scale columns should the total of column
      widths and separations exceed the available inner width of the table.
    - (feature) full justification of text.

    Revision 1.35  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.34  2004/12/31 04:18:17  gbeeley
    - bug fix for printing Binary type attributes
    - bug fix for memory leaks due to open LxSession's

    Revision 1.33  2004/08/30 03:20:41  gbeeley
    - objInfo() can return NULL if it is not supported for an object.

    Revision 1.32  2004/07/02 00:23:24  mmcgill
    Changes include, but are not necessarily limitted to:
        - fixed test_obj hints printing, added printing of hints to show command
        to make them easier to read.
        - added objDuplicateHints, for making deep copies of hints structures.
        - made sure GroupID and VisualLength2 were set to their proper defualts
          inf objPresentationHints() [obj_attr.c]
        - did a bit of restructuring in the sybase OS driver:
    	* moved the type conversion stuff in sybdGetAttrValue into a seperate
    	  function (sybd_internal_GetCxValue, sybd_internal_GetCxType). In
    	* Got rid of the Types union, made it an ObjData struct instead
    	* Stored column lengths in ColLengths
    	* Fixed a couple minor bugs
        - Roughed out a preliminary hints implementation for the sybase driver,
          in such a way that it shouldn't be *too* big a deal to add support for
          user-defined types.

    Revision 1.31  2004/05/04 18:22:59  gbeeley
    - Adding DATA_T_BINARY data type for counted (non-zero-terminated)
      strings of data.

    Revision 1.30  2004/02/24 20:25:40  gbeeley
    - misc changes: runclient check in evaltree in stparse, eval() function
      rejected in sybase driver, update version in centrallix.conf, .cmp
      extension added for component-decl in types.cfg

    Revision 1.29  2003/09/02 15:37:13  gbeeley
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

    Revision 1.28  2003/05/30 17:39:47  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.27  2003/04/25 04:09:29  gbeeley
    Adding insert and autokeying support to OSML and to CSV datafile
    driver on a limited basis (in rowidkey mode only, which is the only
    mode currently supported by the csv driver).

    Revision 1.26  2003/04/04 05:02:44  gbeeley
    Added more flags to objInfo dealing with content and seekability.
    Added objInfo capability to objdrv_struct.

    Revision 1.25  2003/04/03 21:41:07  gbeeley
    Fixed xstring modification problem in test_obj as well as const path
    modification problem in the objOpen process.  Both were causing the
    cxsec stuff in xstring to squawk.

    Revision 1.24  2003/03/31 23:23:39  gbeeley
    Added facility to get additional data about an object, particularly
    with regard to its ability to have subobjects.  Added the feature at
    the driver level to objdrv_ux, and to the "show" command in test_obj.

    Revision 1.23  2003/03/30 22:49:24  jorupp
     * get rid of some compile warnings -- compiles with zero warnings under gcc 3.2.2

    Revision 1.22  2003/03/10 15:41:39  lkehresman
    The CSV objectsystem driver (objdrv_datafile.c) now presents the presentation
    hints to the OSML.  To do this I had to:
      * Move obj_internal_InfToHints() to a global function objInfToHints.  This
        is now located in utility/hints.c and the include is in include/hints.h.
      * Added the presentation hints function to the CSV driver and called it
        datPresentationHints() which returns a valid objPresentationHints object.
      * Modified test_obj.c to fix a crash bug and reformatted the output to be
        a little bit easier to read.
      * Added utility/hints.c to Makefile.in (somebody please check and make sure
        that I did this correctly).  Note that you will have to reconfigure
        centrallix for this change to take effect.

    Revision 1.21  2003/03/03 21:33:31  lkehresman
    Fixed a bug in test_obj that would segfault if no attributes were returned
    from an object.

    Revision 1.20  2003/02/26 01:32:59  jorupp
     * added presentation hints support to test_obj
    	one little problem -- for some reason, asking about just one attribute doesn't work

    Revision 1.19  2003/02/25 03:31:39  gbeeley
    Completed the 'help' message in test_obj.

    Revision 1.18  2002/09/28 01:05:30  jorupp
     * added tab completion
     * fixed bug where list/ls was relying on the pointers returned by getAttrValue being valid after another getAttrValue

    Revision 1.17  2002/09/27 22:26:03  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.16  2002/09/06 02:47:30  jorupp
     * removed luke's username and password hack

    Revision 1.15  2002/09/06 02:43:35  lkehresman
    Hmm.. probably shouldn't have committed my username and password with
    test_obj.

    See: http://dman.ddts.net/~dman/humorous/shooting.html
    Add:
      Centrallix:
        % cd /usr/src/centrallix
        % cvs commit

    Revision 1.14  2002/09/06 02:39:11  lkehresman
    Got OSML interaction to work with the MIME libraries thanks to
    jorupp magic.

    Revision 1.13  2002/08/13 14:22:50  lkehresman
    * removed unused variables in test_obj
    * added an incomplete "help" command to test_obj

    Revision 1.12  2002/08/10 02:43:19  gbeeley
    Test-obj now automatically displays 'system' attributes on a show
    command.  This includes inner_type, outer_type, name, and annotation,
    all of which are not supposed to be returned by the attribute enum
    functions.  FYI inner_type and content_type are synonyms (neither
    should be returned by GetFirst/Next Attr).

    Revision 1.11  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.10  2002/06/13 15:21:04  mattphillips
    Adding autoconf support to centrallix

    Revision 1.9  2002/06/09 23:44:45  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.8  2002/06/01 19:08:46  mattphillips
    A littl ebit of code cleanup...  getting rid of some compiler warnings.

    Revision 1.7  2002/05/02 01:14:56  gbeeley
    Added dynamic module loading support in Centrallix, starting with the
    Sybase driver, using libdl.

    Revision 1.6  2002/02/14 01:05:07  gbeeley
    Fixed test_obj so that it works with the new config file stuff.

    Revision 1.5  2001/11/12 20:43:43  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

    Revision 1.4  2001/10/02 16:24:24  gbeeley
    Changed %f printf conversion to more intuitive %g.

    Revision 1.3  2001/09/28 19:06:18  gbeeley
    Fixed EOF handling on readline()==NULL; fixed "query" command to use inbuf
    instead of sbuf.

    Revision 1.2  2001/09/18 15:39:23  mattphillips
    Added GNU Readline support.  This adds full commandline editting support, and
    scrollback support.  No tab completion yet, though.

    NOTE: The readline and readline-devel packages (for RPM based distributions)
    are required for building now.

    Revision 1.1.1.1  2001/08/13 18:00:46  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:30:51  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

void* my_ptr;
unsigned long ticks_last_tab=0;
pObjSession s;

struct
    {
    char    UserName[32];
    char    Password[32];
    char    CmdFile[256];
    pFile   Output;
    char    Command[1024];
    }
    TESTOBJ;

#define BUFF_SIZE 1024

typedef struct
    {
    char *buffer;
    int buflen;
    } WriteStruct, *pWriteStruct;

int text_gen_callback(pWriteStruct dst, char *src, int len, int a, int b)
    {
    dst->buffer = (char*)realloc(dst->buffer,dst->buflen+len+1);
    if(!dst->buffer)
	return -1;
    memcpy(dst->buffer+dst->buflen,src,len);
    dst->buflen+=len;
    return 0;
    }

int 
printExpression(pExpression exp)
    {
    pWriteStruct dst;
    pParamObjects tmplist;

	if(!exp)
	    return -1;
	dst = (pWriteStruct)nmMalloc(sizeof(WriteStruct));
	dst->buffer=(char*)malloc(1);
	dst->buflen=0;

	tmplist = expCreateParamList();
	expAddParamToList(tmplist,"this",NULL,EXPR_O_CURRENT);
	expGenerateText(exp,tmplist,text_gen_callback,dst,'"',"javascript");
	dst->buffer[dst->buflen]='\0';
	expFreeParamList(tmplist);

	printf("%s ",dst->buffer);

	free(dst->buffer);
	nmFree(dst,sizeof(WriteStruct));
	return 0;
    }



int
testobj_show_hints(pObject obj, char* attrname)
    {
    pObjPresentationHints hints;
    int i;

    hints = objPresentationHints(obj, attrname);
    if(!hints)
	{
	mssError(1,"unable to get presentation hints for %s",attrname);
	return -1;
	}

    printf("Presentation Hints for \"%s\":\n",attrname);
    printf("  Constraint   : ");
    printExpression(hints->Constraint); printf("\n");
    printf("  DefaultExpr  : ");
    printExpression(hints->DefaultExpr); printf("\n");
    printf("  MinValue     : ");
    printExpression(hints->MinValue); printf("\n");
    printf("  MaxValue     : ");
    printExpression(hints->MaxValue); printf("\n");
    printf("  EnumList     : ");
    for(i=0;i<hints->EnumList.nItems;i++)
	{
	printf("    %s\n",(char*)xaGetItem(&hints->EnumList,i));
	}
    printf("\n");
    printf("  EnumQuery    : %s\n",hints->EnumQuery);
    printf("  Format       : %s\n",hints->Format);
    printf("  VisualLength : %i\n",hints->VisualLength);
    printf("  VisualLength2: %i\n",hints->VisualLength2);
    printf("  BitmaskRO    : ");
    for(i=0;i<32;i++)
	{
	printf("%i",hints->BitmaskRO>>(31-i) & 0x01);
	}
    printf("\n");
    printf("  Style        : %i\n",hints->Style);
    printf("  GroupID      : %i\n",hints->GroupID);
    printf("  GroupName    : %s\n",hints->GroupName);
    printf("  FriendlyName : %s\n",hints->FriendlyName);

    objFreeHints(hints);
    return 0;
    }



int
testobj_show_attr(pObject obj, char* attrname)
    {
    int type;
    int intval;
    char* stringval;
    pDateTime dt;
    double dblval;
    pMoneyType m;
    int i;
    pStringVec sv;
    pIntVec iv;
    Binary bn;
    pObjPresentationHints hints;

	type = objGetAttrType(obj,attrname);
	if (type < 0) 
	    {
	    printf("  %20.20s: (no such attribute)\n", attrname);
	    return -1;
	    }
	switch(type)
	    {
	    case DATA_T_INTEGER:
		if (objGetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&intval)) == 1)
		    printf("  %20.20s: NULL",attrname);
		else
		    printf("  %20.20s: %d",attrname, intval);
		break;

	    case DATA_T_STRING:
		if (objGetAttrValue(obj,attrname,DATA_T_STRING,POD(&stringval)) == 1)
		    printf("  %20.20s: NULL",attrname);
		else
		    printf("  %20.20s: \"%s\"",attrname, stringval);
		break;

	    case DATA_T_BINARY:
		if (objGetAttrValue(obj,attrname,DATA_T_BINARY,POD(&bn)) == 1)
		    printf("  %20.20s: NULL", attrname);
		else
		    {
		    printf("  %20.20s: %d bytes: ", attrname, bn.Size);
		    for(i=0;i<bn.Size;i++)
			{
			printf("%2.2x  ", bn.Data[i]);
			}
		    }
		break;

	    case DATA_T_DATETIME:
		if (objGetAttrValue(obj,attrname,DATA_T_DATETIME,POD(&dt)) == 1 || dt==NULL)
		    printf("  %20.20s: NULL",attrname);
		else
		    printf("  %20.20s: %2.2d-%2.2d-%4.4d %2.2d:%2.2d:%2.2d", 
			attrname,dt->Part.Month+1, dt->Part.Day+1, dt->Part.Year+1900,
			dt->Part.Hour, dt->Part.Minute, dt->Part.Second);
		break;
	    
	    case DATA_T_DOUBLE:
		if (objGetAttrValue(obj,attrname,DATA_T_DOUBLE,POD(&dblval)) == 1)
		    printf("  %20.20s: NULL",attrname);
		else
		    printf("  %20.20s: %g", attrname, dblval);
		break;

	    case DATA_T_MONEY:
		if (objGetAttrValue(obj,attrname,DATA_T_MONEY,POD(&m)) == 1 || m == NULL)
		    printf("  %20.20s: NULL", attrname);
		else
		    printf("  %20.20s: %s", attrname, objDataToStringTmp(DATA_T_MONEY, m, 0));
		break;

	    case DATA_T_INTVEC:
		if (objGetAttrValue(obj,attrname,DATA_T_INTVEC,POD(&iv)) == 1 || iv == NULL)
		    {
		    printf("  %20.20s: NULL",attrname);
		    }
		else
		    {
		    printf("  %20.20s: ", attrname);
		    for(i=0;i<iv->nIntegers;i++) 
			printf("%d%s",iv->Integers[i],(i==iv->nIntegers-1)?"":",");
		    }
		break;

	    case DATA_T_STRINGVEC:
		if (objGetAttrValue(obj,attrname,DATA_T_STRINGVEC,POD(&sv)) == 1 || sv == NULL)
		    {
		    printf("  %20.20s: NULL",attrname);
		    }
		else
		    {
		    printf("  %20.20s: ",attrname);
		    for(i=0;i<sv->nStrings;i++) 
			printf("\"%s\"%s",sv->Strings[i],(i==sv->nStrings-1)?"":",");
		    }
		break;

	    default:
		printf("  %20.20s: <unknown type>",attrname);
		break;
	    }
	hints = objPresentationHints(obj, attrname);
	if (hints)
	    {
	    printf(" [Hints: ");
	    if (hints->EnumQuery != NULL) printf("EnumQuery=[%s] ", hints->EnumQuery);
	    if (hints->Format != NULL) printf("Format=[%s] ", hints->Format);
	    if (hints->AllowChars != NULL) printf("AllowChars=[%s] ", hints->AllowChars);
	    if (hints->BadChars != NULL) printf("BadChars=[%s] ", hints->BadChars);
	    if (hints->Length != 0) printf("Length=%d ", hints->Length);
	    if (hints->VisualLength != 0) printf("VisualLength=%d ", hints->VisualLength);
	    if (hints->VisualLength2 != 1) printf("VisualLength2=%d ", hints->VisualLength2);
	    if (hints->Style != 0) printf("Style=%d ", hints->Style);
	    if (hints->StyleMask != 0) printf("StyleMask=%d ", hints->StyleMask);
	    if (hints->GroupID != -1) printf("GroupID=%d ", hints->GroupID);
	    if (hints->GroupName != NULL) printf("GroupName=[%s] ", hints->GroupName);
	    if (hints->FriendlyName != NULL) printf("FriendlyName=[%s] ", hints->FriendlyName);
	    if (hints->Constraint != NULL) { printf("Constraint="); printExpression(hints->Constraint); }
	    if (hints->DefaultExpr != NULL) { printf("DefaultExpr="); printExpression(hints->DefaultExpr); }
	    if (hints->MinValue != NULL) { printf("MinValue="); printExpression(hints->MinValue); }
	    if (hints->MaxValue != NULL) { printf("MaxValue="); printExpression(hints->MaxValue); }
	    nmFree(hints, sizeof(ObjPresentationHints));
	    printf("]\n");
	    }

    return 0;
    }

int handle_tab(int unused_1, int unused_2)
    {
    pXString xstrInput;
    pXString xstrLastInputParam;
    pXString xstrPath;
    pXString xstrPartialName;
    pXString xstrQueryString;
    pXString xstrMatched;
    int i;
    pObject obj,qobj;
    pObjQuery qry;
    int count=0;
    short secondtab=0;
    pObjectInfo info;

#define DOUBLE_TAB_DELAY 50
    
    /** init all the XStrings **/
    xstrInput=nmMalloc(sizeof(XString));
    xstrLastInputParam=nmMalloc(sizeof(XString));
    xstrPath=nmMalloc(sizeof(XString));
    xstrPartialName=nmMalloc(sizeof(XString));
    xstrQueryString=nmMalloc(sizeof(XString));
    xstrMatched=nmMalloc(sizeof(XString));
    xsInit(xstrInput);
    xsInit(xstrLastInputParam);
    xsInit(xstrPath);
    xsInit(xstrPartialName);
    xsInit(xstrQueryString);
    xsInit(xstrMatched);

    /** figure out if this is a 'double' tab (should list the possibilities) **/
    if(ticks_last_tab+DOUBLE_TAB_DELAY>mtRealTicks())
	{
	secondtab=1;
	/* don't want this branch to be taken next time, even if it's within DOUBLE_TAB_DELAY */
	ticks_last_tab=0;
	}
    else
	ticks_last_tab=mtRealTicks();

    /** beep **/
    printf("%c",0x07);

    /** get the current line into an XString **/
    xsCopy(xstrInput,rl_copy_text (0,256),-1);
    
    /** get a new line if we're going to print a list **/
    if(secondtab) printf("\n");
    
    /** find the space, grab everything after it (or the whole thing if there is no space) **/
    i=xsFindRev(xstrInput," ",-1,0);
    if(i < 0)
	xsCopy(xstrLastInputParam,xstrInput->String,xstrInput->Length);
    else
	xsCopy(xstrLastInputParam,xstrInput->String+i+1,xstrInput->Length-i+1);


    /** is this a relative or absolute path **/
    if(xstrLastInputParam->String[0]!='/')
	{
	/* objGetWD has the trailing slash aready at the root */
	if(strlen(objGetWD(s))>1)
	    xsInsertAfter(xstrLastInputParam,"/",-1,0);
	xsInsertAfter(xstrLastInputParam,objGetWD(s),-1,0);
	}

    /** split on the last slash **/
    i=xsFindRev(xstrLastInputParam,"/",-1,0);
    xsCopy(xstrPath,xstrLastInputParam->String,i+1);
    xsCopy(xstrPartialName,xstrLastInputParam->String+i+1,-1);

    /** open the directory **/
    obj=objOpen(s,xstrPath->String,O_RDONLY,0400,NULL);

    /** build the query **/
    xsPrintf(xstrQueryString,"substring(:name,0,%d)=\"%s\"",xstrPartialName->Length,xstrPartialName->String);

    /** open the query **/
    info = objInfo(obj);
    if (!info || info->Flags & OBJ_INFO_F_CAN_HAVE_SUBOBJ)
	qry = objOpenQuery(obj,xstrQueryString->String,NULL,NULL,NULL);
    else
	qry = NULL;

    /** fetch and compare **/
    while(qry && (qobj=objQueryFetch(qry,O_RDONLY)))
	{
	char *ptr;
	int i;
	/* this is our second one -- output the first one (if second tab...)*/
	if(count==1)
	    if(secondtab) 
		printf("%s\n",xstrMatched->String);

	/* get this entry's name */
	objGetAttrValue(qobj,"name",DATA_T_STRING,POD(&ptr));
	/* compare with xstrMatched and shorten xstrMatched so that it contains the longest string
	 *   that starts both of them */
	for(i=0;i<=strlen(ptr) && i<=strlen(xstrMatched->String);i++)
	    {
	    if(ptr[i]!=xstrMatched->String[i])
		{
		xsSubst(xstrMatched, i, -1, "", 0);
		}
	    }
	/** if this isn't the first returned object, output it unless second tab.. **/
	if(count>0)
	    {
	    if(secondtab) 
		printf("%s\n",ptr);
	    }
	else
	    /* otherwise, put the entire thing in xstrMatched */
	    xsCopy(xstrMatched,ptr,-1);

	count++;
	objClose(qobj);
	}

    if(qry)
	objQueryClose(qry);

    /** there were objects found -- add the longest common string to the input line **/
    if(count)
	{
	rl_insert_text(xstrMatched->String+xstrPartialName->Length);
	}

    if(count==1)
	{
	/** decide if this is a 'directory' that we should put a '/' on the end of **/
	pXString xstrTemp;
	pObject obj2;

	/* next tab should always be a 'first' after a successful match */
	ticks_last_tab=0;

	/** setup XString **/
	xstrTemp=nmMalloc(sizeof(XString));
	xsInit(xstrTemp);

	/** build path for open **/
	xsPrintf(xstrTemp,"%s/%s",xstrPath->String,xstrMatched->String);

	obj2=objOpen(s,xstrTemp->String,O_RDONLY,0400,NULL);
	if(obj2)
	    {
	    /** see if there are any subobjects -- only need to fetch 1 to check **/
	    info = objInfo(obj2);
	    if (!info || info->Flags & OBJ_INFO_F_CAN_HAVE_SUBOBJ)
		qry=objOpenQuery(obj2,NULL,NULL,NULL,NULL);
	    else
		qry=NULL;
	    if(qry && (qobj=objQueryFetch(qry,O_RDONLY)))
		{
		rl_insert_text("/");
		objClose(qobj);
		}
	    else
		{
		/** put a newline after the errors that were probably thrown **/
		printf("\n");
		rl_on_new_line();
		}
	    
	    /** close the object and query we opened **/
	    if(qry) objQueryClose(qry);
	    objClose(obj2);
	    }

	/** cleanup **/
	xsDeInit(xstrTemp);
	nmFree(xstrTemp,sizeof(XString));
	}

    if(obj)
	objClose(obj);

    
    /** get rid of all the XStrings **/
    xsDeInit(xstrInput);
    xsDeInit(xstrLastInputParam);
    xsDeInit(xstrPath);
    xsDeInit(xstrPartialName);
    xsDeInit(xstrQueryString);
    xsDeInit(xstrMatched);
    nmFree(xstrInput,sizeof(XString));
    nmFree(xstrLastInputParam,sizeof(XString));
    nmFree(xstrPath,sizeof(XString));
    nmFree(xstrPartialName,sizeof(XString));
    nmFree(xstrQueryString,sizeof(XString));
    nmFree(xstrMatched,sizeof(XString));

    /** if we output lines, go to an new one and inform readline **/
    if(secondtab)
	{
	printf("\n");
	rl_on_new_line();
	}

    return 0;
    }


int
testobj_do_cmd(pObjSession s, char* cmd, int batch_mode)
    {
    char sbuf[BUFF_SIZE];
    char* ptr;
    char cmdname[64];
    pObject obj,child_obj,to_obj;
    pObjQuery qy;
    char* filename;
    char* filetype;
    char* fileannot;
    int cnt;
    char* attrname;
    char* methodname;
    int type;
    DateTime dtval;
    pDateTime dt;
    char* stringval;
    int intval;
    int is_where, is_orderby;
    pLxSession ls = NULL;
    char where[256];
    char orderby[256];
    double dblval;
    pMoneyType m;
    MoneyType mval;
    pObjData pod;
    int use_srctype;
    char mname[64];
    char mparam[256];
    char* mptr;
    int t,i;
    pObjectInfo info;
    Binary bn;

	    /** Open a lexer session **/
	    ls = mlxStringSession(cmd,MLX_F_ICASE | MLX_F_EOF);
	    if (mlxNextToken(ls) != MLX_TOK_KEYWORD)
		{
		mlxCloseSession(ls);
		return -1;
		}
	    ptr = mlxStringVal(ls,NULL);
	    if (!ptr) 
		{
		mlxCloseSession(ls);
		return -1;
		}
	    strcpy(cmdname,ptr);
	    mlxSetOptions(ls,MLX_F_IFSONLY);
	    if ((t=mlxNextToken(ls)) != MLX_TOK_STRING) ptr = NULL;
	    else ptr = mlxStringVal(ls,NULL);
	    mlxUnsetOptions(ls,MLX_F_IFSONLY);

	    /** What command? **/
	    if (!strcmp(cmdname,"cd"))
		{
		if (!ptr)
		    {
		    printf("Usage: cd <directory>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/directory");
		if (!obj)
		    {
		    printf("cd: could not change to '%s'.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		objSetWD(s,obj);
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"query"))
	        {
		if (!ptr)
		    {
		    printf("Usage: query \"<query-text>\"\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		qy = objMultiQuery(s, cmd + 6);
		if (!qy)
		    {
		    printf("query: could not open query!\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		while((obj=objQueryFetch(qy, O_RDONLY)))
		    {
		    for(attrname=objGetFirstAttr(obj);attrname;attrname=objGetNextAttr(obj))
		        {
			type = objGetAttrType(obj,attrname);
			switch(type)
			    {
			    case DATA_T_INTEGER:
			        if (objGetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&intval)) == 1)
			            printf("Attribute: [%s]  INTEGER  NULL\n", attrname);
				else
			            printf("Attribute: [%s]  INTEGER  %d\n", attrname,intval);
				break;
			    case DATA_T_STRING:
			        if (objGetAttrValue(obj,attrname,DATA_T_STRING,POD(&stringval)) == 1)
			            printf("Attribute: [%s]  STRING  NULL\n", attrname);
				else
			            printf("Attribute: [%s]  STRING  \"%s\"\n", attrname,stringval);
			        break;
			    case DATA_T_DOUBLE:
			        if (objGetAttrValue(obj,attrname,DATA_T_DOUBLE,POD(&dblval)) == 1)
				    printf("Attribute: [%s]  DOUBLE  NULL\n", attrname);
				else
				    printf("Attribute: [%s]  DOUBLE  %g\n", attrname, dblval);
				break;
			    case DATA_T_BINARY:
				if (objGetAttrValue(obj,attrname,DATA_T_BINARY,POD(&bn)) == 1)
				    printf("  %20.20s: NULL\n", attrname);
				else
				    {
				    printf("  %20.20s: %d bytes: ", attrname, bn.Size);
				    for(i=0;i<bn.Size;i++)
					{
					printf("%2.2x  ", bn.Data[i]);
					}
				    printf("\n");
				    }
				break;
			    case DATA_T_DATETIME:
			        if (objGetAttrValue(obj,attrname,DATA_T_DATETIME,POD(&dt)) == 1)
				    printf("Attribute: [%s]  DATETIME  NULL\n", attrname);
				else
				    printf("Attribute: [%s]  DATETIME  %s\n", attrname, objDataToStringTmp(type, dt, 0));
				break;
			    case DATA_T_MONEY:
			        if (objGetAttrValue(obj,attrname,DATA_T_MONEY,POD(&m)) == 1)
				    printf("Attribute: [%s]  MONEY  NULL\n", attrname);
				else
				    printf("Attribute: [%s]  MONEY  %s\n", attrname, objDataToStringTmp(type, m, 0));
				break;
			    }
			}
		    objClose(obj);
		    }
		objQueryClose(qy);
		}
	    else if (!strcmp(cmdname,"annot"))
	        {
		if (!ptr)
		    {
		    printf("Usage: annot <filename> <annotation>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		obj = objOpen(s, ptr, O_RDWR, 0600, "system/object");
		if (!obj)
		    {
		    printf("annot: could not open '%s'.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: annot <filename> <annotation>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		ptr = mlxStringVal(ls,NULL);
		objSetAttrValue(obj, "annotation", DATA_T_STRING,POD(&ptr));
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"list") || !strcmp(cmdname, "ls"))
		{
		is_where = 0;
		is_orderby = 0;
		if (ptr && !strcmp(ptr,"where"))
		    { 
		    is_where = 1;
		    ptr = "";
		    }
		if (ptr && !strcmp(ptr,"orderby"))
		    {
		    is_orderby = 1;
		    ptr = "";
		    }
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/directory");
		if (!obj)
		    {
		    printf("list: could not open directory '%s'.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (!is_where && !is_orderby)
		    {
		    if (t != MLX_TOK_EOF && (t=mlxNextToken(ls)) != MLX_TOK_KEYWORD) ptr = NULL;
		    else ptr = mlxStringVal(ls,NULL);
		    if (ptr && !strcmp(ptr,"where")) is_where = 1;
		    if (ptr && !strcmp(ptr,"orderby")) is_orderby = 1;
		    }
		if (is_where)
		    {
		    if (t != MLX_TOK_EOF && (t=mlxNextToken(ls)) != MLX_TOK_STRING) ptr = NULL;
		    else ptr = mlxStringVal(ls,NULL);
		    printf("where: '%s'\n",ptr);
		    strcpy(where,ptr);
		    if (((t=mlxNextToken(ls)) == MLX_TOK_KEYWORD) && !strcmp("orderby",mlxStringVal(ls,NULL)))
		        {
			if ((t=mlxNextToken(ls)) == MLX_TOK_STRING)
			    strcpy(orderby, mlxStringVal(ls,NULL));
			else
			    orderby[0] = 0;
			}
		    else
		        {
			orderby[0] = 0;
			}
		    qy = objOpenQuery(obj,where,orderby[0]?orderby:NULL,NULL,NULL);
		    }
		else if (is_orderby)
		    {
		    t = mlxNextToken(ls);
		    if (t != MLX_TOK_STRING) 
			{
			objClose(obj);
			mlxCloseSession(ls);
			return -1;
			}
		    strcpy(orderby, mlxStringVal(ls,NULL));
		    qy = objOpenQuery(obj,NULL,orderby,NULL,NULL);
		    }
		else
		    {
		    qy = objOpenQuery(obj,"",NULL,NULL,NULL);
		    }
		if (!qy)
		    {
		    objClose(obj);
		    printf("list: object '%s' doesn't support directory queries.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		while(NULL != (child_obj = objQueryFetch(qy,O_RDONLY)))
		    {
		    if (objGetAttrValue(child_obj,"name",DATA_T_STRING,POD(&filename)) >= 0)
			{
			char *name,*type,*annot;
#define MALLOC_AND_COPY(dest,src) \
			dest=(char*)malloc(strlen(src)+1);\
			if(!dest) break;\
			strcpy(dest,src);
			MALLOC_AND_COPY(name,filename);
			objGetAttrValue(child_obj,"outer_type",DATA_T_STRING,POD(&filetype));
			MALLOC_AND_COPY(type,filetype);
			objGetAttrValue(child_obj,"annotation",DATA_T_STRING,POD(&fileannot));
			MALLOC_AND_COPY(annot,fileannot);
			printf("%-32.32s  %-32.32s    %s\n",name,annot,type);
			free(name);
			free(type);
			free(annot);
			}
		    objClose(child_obj);
		    }
		objQueryClose(qy);
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"show"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/object");
		if (!obj)
		    {
		    printf("show: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		info = objInfo(obj);
		if (info)
		    {
		    if (info->Flags)
			{
			printf("Flags: ");
			if (info->Flags & OBJ_INFO_F_NO_SUBOBJ) printf("no_subobjects ");
			if (info->Flags & OBJ_INFO_F_HAS_SUBOBJ) printf("has_subobjects ");
			if (info->Flags & OBJ_INFO_F_CAN_HAVE_SUBOBJ) printf("can_have_subobjects ");
			if (info->Flags & OBJ_INFO_F_CANT_HAVE_SUBOBJ) printf("cant_have_subobjects ");
			if (info->Flags & OBJ_INFO_F_SUBOBJ_CNT_KNOWN) printf("subobject_cnt_known ");
			if (info->Flags & OBJ_INFO_F_CAN_ADD_ATTR) printf("can_add_attrs ");
			if (info->Flags & OBJ_INFO_F_CANT_ADD_ATTR) printf("cant_add_attrs ");
			if (info->Flags & OBJ_INFO_F_CAN_SEEK_FULL) printf("can_seek_full ");
			if (info->Flags & OBJ_INFO_F_CAN_SEEK_REWIND) printf("can_seek_rewind ");
			if (info->Flags & OBJ_INFO_F_CANT_SEEK) printf("cant_seek ");
			if (info->Flags & OBJ_INFO_F_CAN_HAVE_CONTENT) printf("can_have_content ");
			if (info->Flags & OBJ_INFO_F_CANT_HAVE_CONTENT) printf("cant_have_content ");
			if (info->Flags & OBJ_INFO_F_HAS_CONTENT) printf("has_content ");
			if (info->Flags & OBJ_INFO_F_NO_CONTENT) printf("no_content ");
			if (info->Flags & OBJ_INFO_F_SUPPORTS_INHERITANCE) printf("supports_inheritance ");
			printf("\n");
			if (info->Flags & OBJ_INFO_F_SUBOBJ_CNT_KNOWN)
			    {
			    printf("Subobject count: %d\n", info->nSubobjects);
			    }
			}
		    }
		puts("Attributes:");
		testobj_show_attr(obj,"outer_type");
		testobj_show_attr(obj,"inner_type");
		testobj_show_attr(obj,"content_type");
		testobj_show_attr(obj,"name");
		testobj_show_attr(obj,"annotation");
		testobj_show_attr(obj,"last_modification");
		attrname = objGetFirstAttr(obj);
		while(attrname)
		    {
		    testobj_show_attr(obj,attrname);
		    attrname = objGetNextAttr(obj);
		    }
		puts("\nMethods:");
		methodname = objGetFirstMethod(obj);
		if (methodname)
		    {
		    while(methodname)
			{
			printf("  %20.20s()\n",methodname);
			methodname = objGetNextMethod(obj);
			}
		    }
		else
		    {
		    puts("  (no methods)");
		    }
		puts("");
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"print"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "text/plain");
		if (!obj)
		    {
		    printf("print: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		while((cnt=objRead(obj, sbuf, 255, 0, 0)) >0)
		    {
		    sbuf[cnt] = 0;
		    write(1,sbuf,cnt);
		    }
		puts("");
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"copy"))
	        {
		if (!ptr)
		    {
		    printf("copy1: must specify <dsttype/srctype> <source> <destination>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (!strcmp(ptr,"srctype")) use_srctype = 1; else use_srctype = 0;
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("copy2: must specify <dsttype/srctype> <source> <destination>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		mlxCopyToken(ls, sbuf, 1023);
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("copy3: must specify <dsttype/srctype> <source> <destination>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		ptr = mlxStringVal(ls, NULL);
		if (use_srctype)
		    {
		    obj = objOpen(s, sbuf, O_RDONLY, 0600, "application/octet-stream");
		    if (!obj) 
			{
			mlxCloseSession(ls);
			return -1;
			}
		    objGetAttrValue(obj, "inner_type", DATA_T_STRING,POD(&stringval));
		    to_obj = objOpen(s, ptr, O_RDWR | O_CREAT | O_TRUNC, 0600, stringval);
		    if (!to_obj)
		        {
			objClose(obj);
			mlxCloseSession(ls);
			return -1;
			}
		    }
		else
		    {
		    to_obj = objOpen(s, ptr, O_RDWR | O_CREAT | O_TRUNC, 0600, "application/octet-stream");
		    if (!to_obj)
			{
			mlxCloseSession(ls);
			return -1;
			}
		    objGetAttrValue(to_obj, "inner_type", DATA_T_STRING,POD(&stringval));
		    obj = objOpen(s, sbuf, O_RDONLY, 0600, stringval);
		    if (!obj)
		        {
			objClose(to_obj);
			mlxCloseSession(ls);
			return -1;
			}
		    }
		while((cnt = objRead(obj, sbuf, 255, 0, 0)) > 0)
		    {
		    objWrite(to_obj, sbuf, cnt, 0, 0);
		    }
		objClose(obj);
		objClose(to_obj);
		}
	    else if (!strcmp(cmdname,"delete"))
	        {
		if (!ptr)
		    {
		    printf("delete: must specify object.\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (objDelete(s, ptr) < 0)
		    {
		    printf("delete: failed.\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		}
	    else if (!strcmp(cmdname,"create"))
	        {
		if (!ptr) 
		    {
		    printf("create: must specify object.\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (!strcmp(ptr,"*"))
		    obj = objOpen(s, ptr, O_RDWR | O_CREAT | OBJ_O_AUTONAME, 0600, "system/object");
		else
		    obj = objOpen(s, ptr, O_RDWR | O_CREAT, 0600, "system/object");
		if (!obj)
		    {
		    printf("create: could not create object.\n");
		    mssPrintError(TESTOBJ.Output);
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (!strcmp(ptr,"*"))
		    {
		    objGetAttrValue(obj, "name", DATA_T_STRING, POD(&stringval));
		    printf("New object name is '%s'\n", stringval);
		    }
		puts("Enter attributes, blank line to end.");
		rl_bind_key ('\t', rl_insert_text);
		while(1)
		    {
		    char* slbuf = readline("");
		    strncpy(sbuf, slbuf, BUFF_SIZE-1);
		    sbuf[BUFF_SIZE-1] = 0;
		    if (sbuf[0] == 0) break;
		    attrname = strtok(sbuf,"=");
		    stringval = strtok(NULL,"=");
		    while(*stringval == ' ') stringval++;
		    while(attrname[strlen(attrname)-1] == ' ') attrname[strlen(attrname)-1]=0;
		    type = objGetAttrType(obj,attrname);
		    if (type < 0 || type == DATA_T_UNAVAILABLE)
		        {
		        if (*stringval >= '0' && *stringval <= '9')
		            {
			    intval = strtol(stringval,NULL,10);
			    objSetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&intval));
			    }
		        else
		            {
			    objSetAttrValue(obj,attrname,DATA_T_STRING,POD(&stringval));
			    }
			}
		    else
		        {
			switch(type)
			    {
			    case DATA_T_INTEGER: intval = objDataToInteger(DATA_T_STRING,(void*)stringval,NULL); pod = POD(&intval); break;
			    case DATA_T_STRING: pod = POD(&stringval); break;
			    case DATA_T_DOUBLE: dblval = objDataToDouble(DATA_T_STRING,(void*)stringval); pod = POD(&dblval); break;
			    case DATA_T_DATETIME: dt = &dtval; objDataToDateTime(DATA_T_STRING,(void*)stringval,dt,NULL); pod = POD(&dt); break;
			    case DATA_T_MONEY: m = &mval; objDataToMoney(DATA_T_STRING,(void*)stringval, m); pod = POD(&m); break;
			    default:
				printf("create: warning - invalid attribute type for attr '%s'\n", attrname);
				pod = NULL;
				break;
			    }
			if (pod) objSetAttrValue(obj,attrname,type,pod);
			}
		    }
		if (objClose(obj) < 0) mssPrintError(TESTOBJ.Output);
		rl_bind_key ('\t', handle_tab);
		}
	    else if (!strcmp(cmdname,"quit"))
		{
		mlxCloseSession(ls);
		return 1;
		}
	    else if (!strcmp(cmdname,"exec"))
	        {
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "application/octet-stream");
		if (!obj)
		    {
		    printf("exec: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: exec <obj> <method> <parameter>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		mlxCopyToken(ls, mname, 63);
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: exec <obj> <method> <parameter>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		mlxCopyToken(ls, mparam, 255);
		mptr = mparam;
		objExecuteMethod(obj, mname, POD(&mptr));
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"hints"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/object");
		if (!obj)
		    {
		    printf("hints: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		attrname=NULL;
		if (mlxNextToken(ls) == MLX_TOK_STRING)
		    {
		    attrname=nmMalloc(64);
		    mlxCopyToken(ls, attrname, 63);
		    attrname[63]='\0';
		    }
		if (attrname)
		    {
		    testobj_show_hints(obj, attrname);
		    nmFree(attrname,64);
		    }
		else
		    {
		    attrname = objGetFirstAttr(obj);
		    do
			{
			if (attrname != NULL)
			    {
			    testobj_show_hints(obj, attrname);
			    }
			}
		    while ((attrname = objGetNextAttr(obj)));
		    }
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"help"))
		{
		printf("Available Commands:\n");
		printf("  annot    - Add or change the annotation on an object.\n");
		printf("  cd       - Change the current working \"directory\" in the objectsystem.\n");
		printf("  copy     - Copy one object's content to another.\n");
		printf("  create   - Create a new object.\n");
		printf("  delete   - Delete an object.\n");
		printf("  exec     - Call a method on an object.\n");
		printf("  hints    - Show the presentation hints of an attribute (or object)\n");
		printf("  help     - Displays this help screen.\n");
		printf("  list, ls - Lists the objects in the current \"directory\" in the objectsystem.\n");
		printf("  print    - Displays an object's content.\n");
		printf("  query    - Runs a SQL query.\n");
		printf("  quit     - Exits this application.\n");
		printf("  show     - Displays an object's attributes and methods.\n");
		}
	    else
		{
		printf("Unknown command '%s'\n",cmdname);
		return -1;
		}
	
	    mlxCloseSession(ls);

    return 0;
    }


void
start(void* v)
    {
    int rval,t;
    char* inbuf = NULL;
    char* user;
    char* pwd;
    char prompt[1024];
    pFile cmdfile;
    pLxSession input_lx;
    char* ptr;

	/** Initialize. **/
	cxInitialize();

	/** enable tab completion **/
	rl_bind_key ('\t', handle_tab);

	/** Authenticate **/
	if (!TESTOBJ.UserName[0]) 
	    user = readline("Username: ");
	else
	    user = TESTOBJ.UserName;
	if (!TESTOBJ.Password[0])
	    pwd = getpass("Password: ");
	else
	    pwd = TESTOBJ.Password;

	if (mssAuthenticate(user,pwd) < 0)
	    puts("Warning: auth failed, running outside session context.");
	TESTOBJ.Output = fdOpen("/dev/tty", O_RDWR, 0600);

	/** Open a session **/
	s = objOpenSession("/");

	/** -C cmd provided on command line? **/
	if (TESTOBJ.Command[0])
	    {
	    rval = testobj_do_cmd(s, TESTOBJ.Command, 1);
	    }

	/** Command file provided? **/
	if (TESTOBJ.CmdFile[0])
	    {
	    cmdfile = fdOpen(TESTOBJ.CmdFile, O_RDONLY, 0600);
	    if (!cmdfile)
		{
		perror(TESTOBJ.CmdFile);
		}
	    else
		{
		input_lx = mlxOpenSession(cmdfile, MLX_F_LINEONLY | MLX_F_EOF);
		while((t = mlxNextToken(input_lx)) > 0)
		    {
		    if (t == MLX_TOK_EOF || t == MLX_TOK_ERROR) break;
		    ptr = mlxStringVal(input_lx, NULL);
		    if (ptr) 
			{
			rval = testobj_do_cmd(s, ptr, 1);
			if (rval == 1) break;
			}
		    }
		mlxCloseSession(input_lx);
		fdClose(cmdfile, 0);
		}
	    }

	/** Loop, putting prompt and getting commands **/
	if (!TESTOBJ.Command[0] && !TESTOBJ.CmdFile[0]) 
	  while(1)
	    {
	    sprintf(prompt,"OSML:%.1000s> ",objGetWD(s));

	    /** If the buffer has already been allocated, return the memory to the free pool. **/
	    if (inbuf)
		{
		free (inbuf);
		inbuf = (char *)NULL;
		}   

	    /** Get a line from the user using readline library call. **/
	    inbuf = readline (prompt);   

	    /** If the line has any text in it, save it on the readline history. **/
	    if (inbuf && *inbuf)
		add_history (inbuf);

	    /** If inbuf is null (end of file, etc.), exit **/
	    if (!inbuf)
	        {
		printf("quit\n");
		thExit();
		}

	    rval = testobj_do_cmd(s, inbuf, 0);
	    if (rval == 1) break;
	    }

	objCloseSession(s);

    thExit();
    }


int 
main(int argc, char* argv[])
    {
    int ch;

	/** Default global values **/
	strcpy(CxGlobals.ConfigFileName, CENTRALLIX_CONFIG);
	CxGlobals.QuietInit = 0;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
	memset(&TESTOBJ,0,sizeof(TESTOBJ));
    
	/** Check for config file options on the command line **/
	while ((ch=getopt(argc,argv,"hc:qu:p:f:C:")) > 0)
	    {
	    switch (ch)
	        {
		case 'C':	memccpy(TESTOBJ.Command, optarg, 0, 1023);
				TESTOBJ.Command[1023] = 0;
				break;
		case 'f':	memccpy(TESTOBJ.CmdFile, optarg, 0, 255);
				TESTOBJ.CmdFile[255] = 0;
				break;
		case 'u':	memccpy(TESTOBJ.UserName, optarg, 0, 31);
				TESTOBJ.UserName[31] = 0;
				break;
		case 'p':	memccpy(TESTOBJ.Password, optarg, 0, 31);
				TESTOBJ.Password[31] = 0;
				break;
		case 'c':	memccpy(CxGlobals.ConfigFileName, optarg, 0, 255);
				CxGlobals.ConfigFileName[255] = '\0';
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'h':	printf("Usage:  test_obj [-c <config-file>]\n");
				exit(0);

		case '?':
		default:	printf("Usage:  test_obj [-c <config-file>]\n");
				exit(1);
		}
	    }

    mtInitialize(0, start);
    return 0;
    }
