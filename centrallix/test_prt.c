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

    $Id: test_prt.c,v 1.3 2002/10/17 20:23:17 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/test_prt.c,v $

    $Log: test_prt.c,v $
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
    int is_where;
    char* user;
    char* pwd;
    pFile StdOut;
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
	prt_strictfm_Initialize();
	prt_pclod_Initialize();

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
	    else if (!strcmp(cmdname,"session"))
		{
		if (mlxNextToken(ls) != MLX_TOK_STRING) 
		    {
		    printf("test_prt: usage: session <mime type>\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		prtsession= prtOpenSession(ptr, testWrite, NULL);
		printf("session: prtOpenSession returned %8.8X\n", (int)prtsession);
		if (prtsession) 
		    {
		    rval = prtCloseSession(prtsession);
		    printf("session: prtCloseSession returned %d\n", rval);
		    }
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
