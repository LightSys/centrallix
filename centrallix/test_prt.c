#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "mtask.h"
#include "mtlexer.h"
#include "obj.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
/* Some versions of readline get upset if HAVE_CONFIG_H is defined! */
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#include <readline/readline.h>
#include <readline/history.h>
#define HAVE_CONFIG_H
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif
#ifndef CENTRALLIX_CONFIG
#define CENTRALLIX_CONFIG /usr/local/etc/centrallix.conf
#endif
#include "prtmgmt_v3.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2002 LightSys Technology Services, Inc.		*/
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
/* Module:	test_prt.c                                              */
/* Author:	Greg Beeley                                             */
/* Date:	April 8, 2002                                           */
/*									*/
/* Description:	This module provides a testing interface to the print	*/
/*		formatting subsystem.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: test_prt.c,v 1.18 2003/04/04 22:38:26 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/test_prt.c,v $

    $Log: test_prt.c,v $
    Revision 1.18  2003/04/04 22:38:26  gbeeley
    Added HTML formatter for new print subsystem, with just basic output
    capabilities at present.

    Revision 1.17  2003/03/18 04:06:22  gbeeley
    Added basic image (picture/bitmap) support; only PNG images supported
    at present.  Moved image and border (rectangles) functionality into a
    new file prtmgmt_v3_graphics.c.  Graphics are only monochrome at the
    present and work only under PCL (not plain text!!!).  PNG support is
    via libpng, so libpng was added to configure/autoconf.

    Revision 1.16  2003/03/15 04:45:58  gbeeley
    Added borders to tables.  Not fully tested yet.  Added a new component
    of the "PrtBorder" object: "Pad", which is the padding 'outside' of
    the border.  The reporting objdriver is going to have to really
    simplify the margins/borders stuff on tables because there are so many
    params that can be set - it can be confusing and hard to get right.

    Revision 1.15  2003/03/12 20:51:35  gbeeley
    Tables now working, but borders on tables not implemented yet.
    Completed the prt_internal_Duplicate routine and reworked the
    API interface to InitContainer on the layout managers.  Not all
    features/combinations on tables have been tested.  Footers on
    tables not working but (repeating) headers are.  Added a new
    prt obj stream field called "ContentSize" which provides the
    allocated memory size of the "Content" field.

    Revision 1.14  2003/03/07 06:16:12  gbeeley
    Added border-drawing functionality, and converted the multi-column
    layout manager to use that for column separators.  Added border
    capability to textareas.  Reworked the deinit/init kludge in the
    Reflow logic.

    Revision 1.13  2003/03/06 02:52:32  gbeeley
    Added basic rectangular-area support (example - border lines for tables
    and separator lines for multicolumn areas).  Works on both PCL and
    textonly.  Palette-based coloring of rectangles (via PCL) not seeming
    to work consistently on my system, however.  Warning: using large
    dimensions for the 'rectangle' command in test_prt may consume much
    printer ink!!  Now it's time to go watch the thunderstorms....

    Revision 1.12  2003/03/03 23:45:19  gbeeley
    Added support for multi-column formatting where columns are not equal
    in width.  Specifying width/height as negative when adding one object
    to another causes that object to fill its container in the respective
    dimension(s).  Fixed a bug in the Justification logic.

    Revision 1.11  2003/02/27 22:02:15  gbeeley
    Some improvements in the balanced multi-column output.  A lot of fixes
    in the multi-column output and in the text layout manager.  Added a
    facility to "schedule" reflows rather than having them take place
    immediately.

    Revision 1.10  2003/02/27 05:21:19  gbeeley
    Added multi-column layout manager functionality to support multi-column
    sections (this is newspaper-style multicolumn formatting).  Tested in
    test_prt "columns" command with various numbers of columns.  Balanced
    mode not yet working.

    Revision 1.9  2003/02/25 03:57:50  gbeeley
    Added incremental reflow capability and test in test_prt.  Added stub
    multi-column layout manager.  Reflow is horribly inefficient, but not
    worried about that at this point.

    Revision 1.8  2003/02/19 22:53:52  gbeeley
    Page break now somewhat operational, both with hard breaks (form feeds)
    and with soft breaks (page wrapping).  Some bugs in how my printer (870c)
    places the text on pages after a soft break (but the PCL seems to look
    correct), and in how word wrapping is done just after a page break has
    occurred.  Use "printfile" command in test_prt to test this.

    Revision 1.7  2002/10/22 04:12:55  gbeeley
    Added justification (left/center/right) support.  Full justification
    does not yet work.  Also, attempted a screen-based color text output
    mechanism which needs to be refined but unfortunately will not work
    on some/most/any pcl inkjets (tested: 870C) but may eventually work
    on lasers (tested: hp4550).  I will probably force the use of a
    postscript output driver if the user wants better color support; no
    real need to spend more time on it in the pcl output driver.  Reverted
    to palette-based color text support.

    Revision 1.6  2002/10/21 22:55:11  gbeeley
    Added font/size test in test_prt to test the alignment of different fonts
    and sizes on one line or on separate lines.  Fixed lots of bugs in the
    font baseline alignment logic.  Added prt_internal_Dump() to debug the
    document's structure.  Fixed a YSort bug where it was not sorting the
    YPrev/YNext pointers but the Prev/Next ones instead, and had a loop
    condition problem causing infinite looping as well.  Fixed some problems
    when adding an empty obj to a stream of objects and then modifying
    attributes which would change the object's geometry.

    There are still some glitches in the line spacing when different font
    sizes are used, however.

    Revision 1.5  2002/10/21 20:22:11  gbeeley
    Text foreground color attribute now basically operational.  Range of
    colors is limited however.  Tested on PCL output driver, on hp870c
    and hp4550 printers.  Also tested on an hp3si (black&white) to make
    sure the color pcl commands didn't garble things up there.  Use the
    "colors" test_prt command to test color output (and "output" to
    "/dev/lp0" if desired).

    Revision 1.4  2002/10/18 22:01:37  gbeeley
    Printing of text into an area embedded within a page now works.  Two
    testing options added to test_prt: text and printfile.  Use the "output"
    option to redirect output to a file or device instead of to the screen.
    Word wrapping has also been tested/debugged and is functional.  Added
    font baseline logic to the design.

    Revision 1.3  2002/10/17 20:23:17  gbeeley
    Got printing v3 subsystem open/close session working (basically)...

    Revision 1.2  2002/06/13 15:21:04  mattphillips
    Adding autoconf support to centrallix

    Revision 1.1  2002/04/25 04:30:13  gbeeley
    More work on the v3 print formatting subsystem.  Subsystem compiles,
    but report and uxprint have not been converted yet, thus problems.


 **END-CVSDATA***********************************************************/

void* my_ptr;


/*** Instantiate the globals from centrallix.h 
 ***/
CxGlobals_t CxGlobals;

/*** testWrite() - takes data being output by the printing subsystem and
 *** converts nonprintables (e.g., ESC sequences) into printables that can
 *** be debugged more easily.
 ***/
int
testWrite(void* arg, char* buffer, int len, int offset, int flags)
    {
    int i;

	for(i=0;i<len;i++)
	    {
	    if (buffer[i] >= 0x20 && buffer[i] <= 0x7D)
		printf("%c", buffer[i]);
	    else if (buffer[i] == 0x1B)
		printf("\\ESC\\");
	    else if (buffer[i] == 0x08)
		printf("\\BS\\");
	    else if (buffer[i] == 0x7E)
		printf("\\DEL\\");
	    else if (buffer[i] == '\n')
		printf("\n");
	    else if (buffer[i] == '\r')
		printf("\\CR\\");
	    else 
		printf("\\%3.3d\\",buffer[i]);
	    }
	printf("\n");

    return len;
    }


void
start(void* v)
    {
    pObjSession s;
    static char* inbuf = (char *)NULL;
    char prompt[1024];
    char* ptr;
    char cmdname[64];
    char sbuf[256];
    int is_where;
    char* user;
    char* pwd;
    pFile StdOut;
    pFile fd;
    pLxSession ls = NULL;
    pFile cxconf;
    pStructInf mss_conf;
    char* authmethod;
    char* authmethodfile;
    char* logmethod;
    char* logprog;
    int log_all_errors;
    pPrtSession prtsession;
    int rval;
    int pagehandle, areahandle, sectionhandle, recthandle, tablehandle, tablerowhandle, tablecellhandle;
    int rcnt;
    void* outputfn;
    void* outputarg;
    int t;
    int r,g,b;
    int i,j;
    char* fontnames[] = {"courier","helvetica","times"};
    pPrtObjStream prtobj;
    int ncols;
    int is_balanced=0, is_unequal=0;
    double x,y,w,h;
    int color;
    pPrtBorder bdr,bdr2;
    pPrtImage img;

	outputfn = testWrite;
	outputarg = NULL;

	/** Load the configuration file **/
	cxconf = fdOpen(CxGlobals.ConfigFileName, O_RDONLY, 0600);
	if (!cxconf)
	    {
	    printf("centrallix: could not open config file '%s'\n", CxGlobals.ConfigFileName);
	    thExit();
	    }
	CxGlobals.ParsedConfig = stParseMsg(cxconf, 0);
	if (!CxGlobals.ParsedConfig)
	    {
	    printf("centrallix: error parsing config file '%s'\n", CxGlobals.ConfigFileName);
	    thExit();
	    }
	fdClose(cxconf, 0);

	/** Init the session handler.  We have to extract the config data for this 
	 ** module ourselves, because mtsession is in the centrallix-lib, and thus can't
	 ** use the new stparse module's routines.
	 **/
	mss_conf = stLookup(CxGlobals.ParsedConfig, "mtsession");
	if (stAttrValue(stLookup(mss_conf,"auth_method"),NULL,&authmethod,0) < 0) authmethod = "system";
	if (stAttrValue(stLookup(mss_conf,"altpasswd_file"),NULL,&authmethodfile,0) < 0) authmethodfile = "/usr/local/etc/cxpasswd";
	if (stAttrValue(stLookup(mss_conf,"log_method"),NULL,&logmethod,0) < 0) logmethod = "stdout";
	if (stAttrValue(stLookup(mss_conf,"log_progname"),NULL,&logprog,0) < 0) logprog = "centrallix";
	log_all_errors = 0;
	if (stAttrValue(stLookup(mss_conf,"log_all_errors"),NULL,&ptr,0) < 0 || !strcmp(ptr,"yes")) log_all_errors = 1;

	/** Initialize the various parts **/
	mssInitialize(authmethod, authmethodfile, logmethod, log_all_errors, logprog);
	nmInitialize();
	expInitialize();
	if (objInitialize() < 0) exit(1);
	snInitialize();
	uxdInitialize();
	sybdInitialize();
	stxInitialize();
	qytInitialize();
	/*rptInitialize();*/
	datInitialize();
	/*uxpInitialize();*/
	uxuInitialize();
	audInitialize();
	mqInitialize();
	mqtInitialize();
	mqpInitialize();
	mqjInitialize();

	prtInitialize();
	prt_htmlfm_Initialize();
	prt_strictfm_Initialize();
	prt_pclod_Initialize();
	prt_textod_Initialize();

	/** Disable tab complete until we have a function to do something useful with it. **/
	rl_bind_key ('\t', rl_insert);

	/** Authenticate **/
	user = readline("Username: ");
	pwd = getpass("Password: ");
	if (mssAuthenticate(user,pwd) < 0)
	    puts("Warning: auth failed, running outside session context.");
	StdOut = fdOpen("/dev/tty", O_RDWR, 0600);
	free( user);

	/** Open a session **/
	s = objOpenSession("/");

	/** Loop, putting prompt and getting commands **/
	while(1)
	    {
	    is_where = 0;
	    sprintf(prompt,"PRT:%.1000s> ",objGetWD(s));

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
		break;
		}

	    if (ls) mlxCloseSession(ls);
	    ls = mlxStringSession(inbuf,MLX_F_ICASE | MLX_F_EOF);
	    if (mlxNextToken(ls) != MLX_TOK_KEYWORD) continue;
	    ptr = mlxStringVal(ls,NULL);
	    if (!ptr) continue;
	    strcpy(cmdname,ptr);

	    /** Process commands **/
	    if (!strcmp(cmdname,"quit"))
		{
		break;
		}
	    else if (!strcmp(cmdname,"help"))
		{
		printf("Print Management Subsystem Test Suite Command Help:\n"
		       "  colors      - outputs text in various colors\n"
		       "  columns     - prints a file into a multicolumn format\n"
		       "  fonts       - writes text in three fonts and five sizes\n"
		       "  help        - show this help message\n"
		       "  image       - tests a bitmap image\n"
		       "  justify     - writes text in each of four justification modes\n"
		       "  output      - redirects output to a file/device instead of screen\n"
		       "  printfile   - output contents of a file into a whole-page area\n"
		       "  rectangle   - draws a rectangle\n"
		       "  session     - test open/close of a session for a given content type\n"
		       "  table       - do a simple test of a table\n"
		       "  text        - puts a given string of text in a given content type\n"
		      );
		}
	    else if (!strcmp(cmdname,"image"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: image <mime type> <imagefile> <x> <y> <width> <height>\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, PRT_OBJ_U_ALLOWBREAK);
		printf("image: prtOpenSession returned %8.8X\n", (int)prtsession);
		rval = prtSetResolution(prtsession, 300);
		printf("image: prtSetResolution(300) returned %d\n", rval);
		pagehandle = prtGetPageRef(prtsession);
		printf("image: prtGetPageRef returned page handle %d\n", pagehandle);
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: image <mime type> <imagefile> <x> <y> <width> <height>\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		fd = fdOpen(ptr, O_RDONLY, 0600);
		if (!fd)
		    {
		    printf("image: %s: could not access file\n", ptr);
		    prtCloseSession(prtsession);
		    continue;
		    }
		img = prtCreateImageFromPNG(fdRead, fd);
		if (!img)
		    {
		    printf("image: %s: could not read PNG data\n", ptr);
		    prtCloseSession(prtsession);
		    continue;
		    }
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: image <mime type> <imagefile> <x> <y> <width> <height>\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		x = mlxDoubleVal(ls);
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: image <mime type> <imagefile> <x> <y> <width> <height>\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		y = mlxDoubleVal(ls);
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: image <mime type> <imagefile> <x> <y> <width> <height>\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		w = mlxDoubleVal(ls);
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: image <mime type> <imagefile> <x> <y> <width> <height>\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		h = mlxDoubleVal(ls);
		if (mlxNextToken(ls) == MLX_TOK_STRING)
		    {
		    ptr = nmSysStrdup(mlxStringVal(ls,NULL));
		    if (mlxNextToken(ls) == MLX_TOK_KEYWORD && !strcmp(mlxStringVal(ls,NULL),"border"))
			{
			bdr = prtAllocBorder(2,0.2,0.0, 0.2,0x0000FF, 0.05,0x00FFFF);
			areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, x+w+10, y, 80-(x+w+10), h, PRT_OBJ_U_XSET | PRT_OBJ_U_YSET, "border", bdr, NULL);
			prtSetMargins(areahandle,1.0,1.0,1.0,1.0);
			prtFreeBorder(bdr);
			}
		    else
			{
			areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, x+w+10, y, 80-(x+w+10), h, PRT_OBJ_U_XSET | PRT_OBJ_U_YSET, NULL);
			}
		    printf("text: prtAddObject(PRT_OBJ_T_AREA) returned area handle %d\n", 
			    areahandle);
		    rval = prtWriteString(areahandle, ptr);
		    printf("text: prtWriteString returned %d\n", rval);
		    rval = prtEndObject(areahandle);
		    printf("text: prtEndObject(area) returned %d\n", rval);
		    }
		rval = prtWriteImage(pagehandle, img, x,y,w,h, PRT_OBJ_U_XSET | PRT_OBJ_U_YSET);
		printf("image: prtWriteImage() returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("image: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"table"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: table <mime type>\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, PRT_OBJ_U_ALLOWBREAK);
		printf("table: prtOpenSession returned %8.8X\n", (int)prtsession);
		pagehandle = prtGetPageRef(prtsession);
		printf("table: prtGetPageRef returned page handle %d\n", pagehandle);
		bdr = prtAllocBorder(1,0.0,0.0, 0.05,0x00FFFF);
		bdr2 = prtAllocBorder(1,0.0,0.0, 0.2,0x0000FF);
		tablehandle = prtAddObject(pagehandle, PRT_OBJ_T_TABLE, 0, 0, 80, 0, PRT_OBJ_U_ALLOWBREAK, "numcols", 2, "border", bdr, "shadow", bdr2, NULL);
		prtFreeBorder(bdr);
		prtFreeBorder(bdr2);
		printf("table: prtAddObject(table) returned table handle %d\n", tablehandle);
		rval = prtSetMargins(tablehandle, 0.2, 0.2, 0.2, 0.2);
		printf("table: prtSetMargins(table) returned %d\n", rval);
		bdr = prtAllocBorder(1,0.0,0.7, 0.05,0x00FFFF);
		tablerowhandle = prtAddObject(tablehandle, PRT_OBJ_T_TABLEROW, 0, 0, 0, 0, 0, "header", 1, "bottomborder", bdr, NULL);
		printf("table: prtAddObject(tablerow) returned table-row handle %d\n", tablerowhandle);
		prtFreeBorder(bdr);
		rval = prtSetMargins(tablerowhandle, 0.0, 1.0, 0.0, 0.0);
		printf("table: prtSetMargins(tablerow) returned %d\n", rval);
		
		/** first header cell **/
		tablecellhandle = prtAddObject(tablerowhandle, PRT_OBJ_T_TABLECELL, 0, 0, 0, 0, 0, NULL);
		printf("table: prtAddObject(tablecell) returned table-cell handle %d\n", tablecellhandle);
		areahandle = prtAddObject(tablecellhandle, PRT_OBJ_T_AREA, 0, 0, -1, 1, 0, NULL);
		printf("table: prtAddObject(area) returned area handle %d\n", areahandle);
		snprintf(sbuf, 256, "Row Number:");
		rval = prtWriteString(areahandle, sbuf);
		printf("table: prtWriteString returned %d\n", rval);
		rval = prtEndObject(areahandle);
		printf("table: prtEndObject(area) returned %d\n", rval);
		rval = prtEndObject(tablecellhandle);
		printf("table: prtEndObject(tablecell) returned %d\n", rval);

		/** Second header **/
		tablecellhandle = prtAddObject(tablerowhandle, PRT_OBJ_T_TABLECELL, 0, 0, 0, 0, 0, NULL);
		printf("table: prtAddObject(tablecell) returned table-cell handle %d\n", tablecellhandle);
		areahandle = prtAddObject(tablecellhandle, PRT_OBJ_T_AREA, 0, 0, -1, 1, 0, NULL);
		printf("table: prtAddObject(area) returned area handle %d\n", areahandle);
		snprintf(sbuf, 256, "Description:");
		rval = prtWriteString(areahandle, sbuf);
		printf("table: prtWriteString returned %d\n", rval);
		rval = prtEndObject(areahandle);
		printf("table: prtEndObject(area) returned %d\n", rval);
		rval = prtEndObject(tablecellhandle);
		printf("table: prtEndObject(tablecell) returned %d\n", rval);

		rval = prtEndObject(tablerowhandle);
		printf("table: prtEndObject(tablerow) returned %d\n", rval);
		for(i=0;i<24;i++)
		    {
		    tablerowhandle = prtAddObject(tablehandle, PRT_OBJ_T_TABLEROW, 0, 0, 0, 0, 0, NULL);
		    printf("table: prtAddObject(tablerow) returned table-row handle %d\n", tablerowhandle);

		    /** First cell **/
		    tablecellhandle = prtAddObject(tablerowhandle, PRT_OBJ_T_TABLECELL, 0, 0, 0, 0, 0, NULL);
		    printf("table: prtAddObject(tablecell) returned table-cell handle %d\n", tablecellhandle);
		    areahandle = prtAddObject(tablecellhandle, PRT_OBJ_T_AREA, 0, 0, -1, 1, 0, NULL);
		    printf("table: prtAddObject(area) returned area handle %d\n", areahandle);
		    snprintf(sbuf, 256, "-- Number %d --\n--test--", i);
		    rval = prtWriteString(areahandle, sbuf);
		    printf("table: prtWriteString returned %d\n", rval);
		    rval = prtEndObject(areahandle);
		    printf("table: prtEndObject(area) returned %d\n", rval);
		    rval = prtEndObject(tablecellhandle);
		    printf("table: prtEndObject(tablecell) returned %d\n", rval);

		    /** Second cell **/
		    tablecellhandle = prtAddObject(tablerowhandle, PRT_OBJ_T_TABLECELL, 0, 0, 0, 0, 0, NULL);
		    printf("table: prtAddObject(tablecell) returned table-cell handle %d\n", tablecellhandle);
		    areahandle = prtAddObject(tablecellhandle, PRT_OBJ_T_AREA, 0, 0, -1, 1, 0, NULL);
		    printf("table: prtAddObject(area) returned area handle %d\n", areahandle);
		    snprintf(sbuf, 256, "This is\nthe description\nfor line %d", i);
		    rval = prtWriteString(areahandle, sbuf);
		    printf("table: prtWriteString returned %d\n", rval);
		    rval = prtEndObject(areahandle);
		    printf("table: prtEndObject(area) returned %d\n", rval);
		    rval = prtEndObject(tablecellhandle);
		    printf("table: prtEndObject(tablecell) returned %d\n", rval);

		    rval = prtEndObject(tablerowhandle);
		    printf("table: prtEndObject(tablerow) returned %d\n", rval);
		    }
		rval = prtEndObject(tablehandle);
		printf("table: prtEndObject(table) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("table: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"rectangle"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: rectangle <mime type> x y width height {color}\n"
			   "    (where x, y, width, and height are floating-point with a decimal pt.)\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, PRT_OBJ_U_ALLOWBREAK);
		printf("rectangle: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (!prtsession)
		    {
		    continue;
		    }
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: rectangle <mime type> x y width height {color}\n"
			   "    (where x, y, width, and height are floating-point with a decimal pt.)\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		x = mlxDoubleVal(ls);
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: rectangle <mime type> x y width height {color}\n"
			   "    (where x, y, width, and height are floating-point with a decimal pt.)\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		y = mlxDoubleVal(ls);
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: rectangle <mime type> x y width height {color}\n"
			   "    (where x, y, width, and height are floating-point with a decimal pt.)\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		w = mlxDoubleVal(ls);
		if (mlxNextToken(ls) != MLX_TOK_DOUBLE)
		    {
		    printf("test_prt: usage: rectangle <mime type> x y width height {color}\n"
			   "    (where x, y, width, and height are floating-point with a decimal pt.)\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		h = mlxDoubleVal(ls);
		if (mlxNextToken(ls) == MLX_TOK_INTEGER)
		    color = mlxIntVal(ls);
		else
		    color = 0x000000;
		pagehandle = prtGetPageRef(prtsession);
		printf("rectangle: prtGetPageRef returned page handle %d\n", pagehandle);
		prtSetColor(pagehandle, color);
		recthandle = prtAddObject(pagehandle, PRT_OBJ_T_RECT, x, y, w, h, PRT_OBJ_U_XSET | PRT_OBJ_U_YSET, NULL);
		printf("rectangle: prtAddObject(rectangle) returned rectangle handle %d\n", recthandle);
		rval = prtEndObject(recthandle);
		printf("rectangle: prtEndObject(rectangle) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("rectangle: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"columns"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: columns <mime type> <numcols> <filename> {balanced} {unequal}\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, PRT_OBJ_U_ALLOWBREAK);
		printf("columns: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (!prtsession)
		    {
		    continue;
		    }
		if (mlxNextToken(ls) != MLX_TOK_INTEGER)
		    {
		    printf("test_prt: usage: columns <mime type> <numcols> <filename> {balanced}\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		ncols = mlxIntVal(ls);
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: columns <mime type> <numcols> <filename> {balanced}\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		fd = fdOpen(ptr, O_RDONLY, 0600);
		if (!fd)
		    {
		    printf("columns: %s: could not access file\n", ptr);
		    prtCloseSession(prtsession);
		    continue;
		    }
		pagehandle = prtGetPageRef(prtsession);
		printf("columns: prtGetPageRef returned page handle %d\n", pagehandle);
		is_balanced = 0;
		is_unequal = 0;
		if (mlxNextToken(ls) == MLX_TOK_KEYWORD) 
		    {
		    if (!strcmp(mlxStringVal(ls,NULL),"balanced")) is_balanced=1;
		    if (!strcmp(mlxStringVal(ls,NULL),"unequal")) is_unequal=1;
		    if (mlxNextToken(ls) == MLX_TOK_KEYWORD) 
			{
			if (!strcmp(mlxStringVal(ls,NULL),"balanced")) is_balanced=1;
			if (!strcmp(mlxStringVal(ls,NULL),"unequal")) is_unequal=1;
			}
		    }
		if (is_unequal)
		    {
		    if (ncols == 1)
			sectionhandle = prtAddObject(pagehandle, PRT_OBJ_T_SECTION, 0, 0, 80, 0, PRT_OBJ_U_ALLOWBREAK, "numcols", ncols, "colsep", 2.0, "balanced", is_balanced, NULL);
		    else if (ncols == 2)
			sectionhandle = prtAddObject(pagehandle, PRT_OBJ_T_SECTION, 0, 0, 80, 0, PRT_OBJ_U_ALLOWBREAK, "numcols", ncols, "colsep", 2.0, "balanced", is_balanced, "colwidths", 30.0, 48.0, NULL);
		    else if (ncols == 3)
			sectionhandle = prtAddObject(pagehandle, PRT_OBJ_T_SECTION, 0, 0, 80, 0, PRT_OBJ_U_ALLOWBREAK, "numcols", ncols, "colsep", 2.0, "balanced", is_balanced, "colwidths", 28.0, 16.0, 32.0, NULL);
		    else if (ncols == 4)
			sectionhandle = prtAddObject(pagehandle, PRT_OBJ_T_SECTION, 0, 0, 80, 0, PRT_OBJ_U_ALLOWBREAK, "numcols", ncols, "colsep", 2.0, "balanced", is_balanced, "colwidths", 25.0, 15.0, 10.0, 24.0, NULL);
		    else /* sorry, didnt code any more of this test */
			sectionhandle = prtAddObject(pagehandle, PRT_OBJ_T_SECTION, 0, 0, 80, 0, PRT_OBJ_U_ALLOWBREAK, "numcols", ncols, "colsep", 2.0, "balanced", is_balanced, NULL);
		    }
		else
		    {
		    bdr = prtAllocBorder(2,0.1,0.0, 0.1,0x000000, 0.1,0x000000);
		    sectionhandle = prtAddObject(pagehandle, PRT_OBJ_T_SECTION, 0, 0, 80, 0, PRT_OBJ_U_ALLOWBREAK, "numcols", ncols, "colsep", 3.0, "balanced", is_balanced, "separator", bdr, NULL);
		    prtFreeBorder(bdr);
		    }
		printf("columns: prtAddObject(PRT_OBJ_T_SECTION) returned section handle %d\n", sectionhandle);
		/*areahandle = prtAddObject(sectionhandle, PRT_OBJ_T_AREA, 0, 0, (80-2*(ncols-1))/ncols, 0, PRT_OBJ_U_ALLOWBREAK, NULL);*/
		areahandle = prtAddObject(sectionhandle, PRT_OBJ_T_AREA, 0, 0, -1, 0, PRT_OBJ_U_ALLOWBREAK, NULL);
		printf("columns: prtAddObject(PRT_OBJ_T_AREA) returned area handle %d\n", 
			areahandle);
		while((rcnt = fdRead(fd, sbuf, 255, 0, 0)) > 0)
		    {
		    sbuf[rcnt] = '\0';
		    rval = prtWriteString(areahandle, sbuf);
		    printf("columns: prtWriteString returned %d\n", rval);
		    }
		fdClose(fd, 0);
		rval = prtEndObject(areahandle);
		printf("columns: prtEndObject(area) returned %d\n", rval);
		rval = prtEndObject(sectionhandle);
		printf("columns: prtEndObject(section) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("columns: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"session"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: session <mime type>\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, 0);
		printf("session: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (prtsession) 
		    {
		    rval = prtCloseSession(prtsession);
		    printf("session: prtCloseSession returned %d\n", rval);
		    }
		}
	    else if (!strcmp(cmdname,"printfile"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: printfile <mime type> <filename> {reflow}\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, PRT_OBJ_U_ALLOWBREAK);
		printf("printfile: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (!prtsession)
		    {
		    continue;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: printfile <mime type> <filename> {reflow}\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		fd = fdOpen(ptr, O_RDONLY, 0600);
		if (!fd)
		    {
		    printf("printfile: %s: could not access file\n", ptr);
		    prtCloseSession(prtsession);
		    continue;
		    }
		pagehandle = prtGetPageRef(prtsession);
		printf("printfile: prtGetPageRef returned page handle %d\n", pagehandle);
		areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, 0, 0, 80, 60, PRT_OBJ_U_ALLOWBREAK, NULL);
		printf("printfile: prtAddObject(PRT_OBJ_T_AREA) returned area handle %d\n", 
			areahandle);
		while((rcnt = fdRead(fd, sbuf, 255, 0, 0)) > 0)
		    {
		    sbuf[rcnt] = '\0';
		    rval = prtWriteString(areahandle, sbuf);
		    printf("printfile: prtWriteString returned %d\n", rval);
		    }
		fdClose(fd, 0);
		if (mlxNextToken(ls) == MLX_TOK_KEYWORD && !strcmp(mlxStringVal(ls,NULL),"reflow"))
		    {
		    /** Do NOT do this in "real life".  These functions are *internal* to the
		     ** printing system and should only be used in testing code like this outside
		     ** the printing subsystem.
		     **/
		    prtobj = (pPrtObjStream)prtHandlePtr(areahandle);
		    prtobj->Width = 50;
		    rval = prt_internal_Reflow(prtobj);
		    printf("printfile: prt_internal_Reflow() returned %d\n", rval);
		    }
		rval = prtEndObject(areahandle);
		printf("printfile: prtEndObject(area) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("printfile: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"text"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: text <mime type> 'text' {border}\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, 0);
		printf("text: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (!prtsession)
		    {
		    continue;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: text <mime type> 'text'\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		pagehandle = prtGetPageRef(prtsession);
		printf("text: prtGetPageRef returned page handle %d\n", pagehandle);
		ptr = nmSysStrdup(mlxStringVal(ls,NULL));
		if (mlxNextToken(ls) == MLX_TOK_KEYWORD && !strcmp(mlxStringVal(ls,NULL),"border"))
		    {
		    bdr = prtAllocBorder(2,0.2,0.0, 0.2,0x0000FF, 0.05,0x00FFFF);
		    areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, 0, 0, 80, 60, 0, "border", bdr, NULL);
		    prtSetMargins(areahandle,1.0,1.0,1.0,1.0);
		    prtFreeBorder(bdr);
		    }
		else
		    {
		    areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, 0, 0, 80, 60, 0, NULL);
		    }
		printf("text: prtAddObject(PRT_OBJ_T_AREA) returned area handle %d\n", 
			areahandle);
		rval = prtWriteString(areahandle, ptr);
		printf("text: prtWriteString returned %d\n", rval);
		rval = prtEndObject(areahandle);
		printf("text: prtEndObject(area) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("text: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"output"))
		{
		if (outputarg)
		    {
		    fdClose(outputarg,0);
		    }
		outputarg = NULL;
		outputfn = testWrite;
		if ((t=mlxNextToken(ls)) == MLX_TOK_EOF)
		    {
		    continue;
		    }
		if (t != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: output 'filename'\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		outputarg = fdOpen(ptr, O_WRONLY | O_TRUNC | O_CREAT, 0600);
		if (!outputarg)
		    {
		    printf("output: could not open '%s'\n", ptr);
		    continue;
		    }
		outputfn = fdWrite;
		}
	    else if (!strcmp(cmdname,"colors"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: colors <mime type>\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, 0);
		printf("colors: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (!prtsession)
		    {
		    continue;
		    }
		pagehandle = prtGetPageRef(prtsession);
		printf("colors: prtGetPageRef returned page handle %d\n", pagehandle);
		areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, 0, 0, 80, 60, 0, NULL);
		printf("colors: prtAddObject(PRT_OBJ_T_AREA) returned area handle %d\n", 
			areahandle);

		/** Print 6 shades of each of r,g, and b.  total lines = 42 **/
		for(r=0;r<=255;r+=51)
		    {
		    for(g=0;g<=255;g+=51)
			{
			for(b=0;b<=255;b+=51)
			    {
			    rval = prtSetColor(areahandle,(r<<16) | (g<<8) | b);
			    printf("colors: prtSetColor returned %d\n", rval);
			    snprintf(sbuf,256,"%2.2X%2.2X%2.2X  ",r,g,b);
			    rval = prtWriteString(areahandle, sbuf);
			    printf("colors: prtWriteString returned %d\n", rval);
			    }
			prtWriteNL(areahandle);
			}
		    prtWriteNL(areahandle);
		    }

		rval = prtEndObject(areahandle);
		printf("colors: prtEndObject(area) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("colors: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"fonts"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: fonts <mime type> 'text'\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, PRT_OBJ_U_ALLOWBREAK);
		printf("fonts: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (!prtsession)
		    {
		    continue;
		    }
		pagehandle = prtGetPageRef(prtsession);
		printf("fonts: prtGetPageRef returned page handle %d\n", pagehandle);
		areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, 0, 0, 80, 60, 0, NULL);
		printf("fonts: prtAddObject(PRT_OBJ_T_AREA) returned area handle %d\n", 
			areahandle);
		
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: fonts <mime type> 'text'\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);

		/** loop through the three fonts, and through five sizes **/
		for(i=0;i<=2;i++) /* fonts 0, 1, and 2 */
		    {
		    rval = prtSetFont(areahandle, fontnames[i]);
		    printf("fonts: prtSetFont returned %d\n", rval);
		    for(j=8; j<=16; j+=2) /* pt sizes 8, 10, 12, 14, and 16 */
			{
			rval = prtSetFontSize(areahandle, j);
			printf("fonts: prtSetFontSize returned %d\n", rval);
			rval = prtWriteString(areahandle, ptr);
			printf("fonts: prtWriteString returned %d\n", rval);
			}
		    }
		rval = prtEndObject(areahandle);
		printf("fonts: prtEndObject(area) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("fonts: prtCloseSession returned %d\n", rval);
		}
	    else if (!strcmp(cmdname,"justify"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: justify <mime type> 'text'\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, outputfn, outputarg, 0);
		printf("justify: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (!prtsession)
		    {
		    continue;
		    }
		pagehandle = prtGetPageRef(prtsession);
		printf("justify: prtGetPageRef returned page handle %d\n", pagehandle);
		areahandle = prtAddObject(pagehandle, PRT_OBJ_T_AREA, 0, 0, 80, 60, 0, NULL);
		printf("justify: prtAddObject(PRT_OBJ_T_AREA) returned area handle %d\n", 
			areahandle);
		
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: justify <mime type> 'text'\n");
		    prtCloseSession(prtsession);
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);

		for(i=0;i<=3;i++)
		    {
		    rval = prtSetJustification(areahandle, i);
		    printf("justify: prtSetJustification returned %d\n", rval);
		    rval = prtWriteString(areahandle, ptr);
		    printf("justify: prtWriteString returned %d\n", rval);
		    }
		rval = prtEndObject(areahandle);
		printf("justify: prtEndObject(area) returned %d\n", rval);
		rval = prtCloseSession(prtsession);
		printf("justify: prtCloseSession returned %d\n", rval);
		}
	    else
		{
		printf("test_prt: error: unknown command '%s'\n", cmdname);
		}
	    }

	/*objCloseSession(s);*/

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
    
	/** Check for config file options on the command line **/
	while ((ch=getopt(argc,argv,"hc:q")) > 0)
	    {
	    switch (ch)
	        {
		case 'c':	memccpy(CxGlobals.ConfigFileName, optarg, 0, 255);
				CxGlobals.ConfigFileName[255] = '\0';
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'h':	printf("Usage:  test_prt [-c <config-file>]\n");
				exit(0);

		case '?':
		default:	printf("Usage:  test_prt [-c <config-file>]\n");
				exit(1);
		}
	    }

    mtInitialize(0, start);
    }
