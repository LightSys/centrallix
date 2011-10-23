#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include "cxlib/mtask.h"
#include "cxlib/strtcpy.h"
#include "cxlib/mtsession.h"
#include "cxlib/xstring.h"
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

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2011 LightSys Technology Services, Inc.		*/
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
/* Module:	cxpasswd.c                                              */
/* Author:	Greg Beeley                                             */
/* Date:	15-Jun-2011                                             */
/*									*/
/* Description:	This file provides an interface to mssGenCred to create	*/
/*		a cxpasswd file entry.  htpasswd can also be used, but  */
/*		unfortunately Apache has to be installed for that to    */
/*		work.							*/
/************************************************************************/

struct
    {
    char    UserName[32];
    char    Password[32];
    char    PasswdFile[256];
    int	    ReadStdin;		/* read password from stdin */
    }
    CXPASSWD;


void
start(void* v)
    {
    char enc_pass[64];
    char salt[MSS_SALT_SIZE + 1];
    char passwd_line[128];
    XString passwd_contents;
    pFile passwd_file;
    char buf[256];
    int len;
    int offset;
    int pos;
    int uname_len;
    int found_user;
    size_t found_len;
    char* nlptr;
    char* ptr;

	cxssInitialize();

	/** No file specified? **/
	if (!CXPASSWD.PasswdFile[0]) 
	    {
	    puts("no passwd file specified (use option -f).");
	    exit(1);
	    }
	
	/** User asked us to read password from stdin? **/
	if (CXPASSWD.ReadStdin)
	    {
	    fgets(CXPASSWD.Password, sizeof(CXPASSWD.Password), stdin);
	    if (strchr(CXPASSWD.Password, '\n'))
		{
		*strchr(CXPASSWD.Password, '\n') = '\0';
		}
	    }

	/** Now get username and/or password if not supplied on command line **/
	if (!CXPASSWD.UserName[0]) 
	    {
	    ptr = readline("Username: ");
	    if (ptr) strtcpy(CXPASSWD.UserName, ptr, sizeof(CXPASSWD.UserName));
	    }
	if (!CXPASSWD.Password[0])
	    {
	    ptr = getpass("Password: ");
	    if (ptr) strtcpy(CXPASSWD.Password, ptr, sizeof(CXPASSWD.Password));
	    }

	/** Get random data for the salt **/
	if (cxssGenerateKey(salt, MSS_SALT_SIZE) < 0)
	    {
	    puts("could not generate random bytes for password salt.");
	    exit(1);
	    }

	/** Generate encrypted credential **/
	if (mssGenCred(salt, MSS_SALT_SIZE, CXPASSWD.Password, enc_pass, sizeof(enc_pass)) < 0)
	    {
	    puts("could not generate encrypted password.");
	    exit(1);
	    }

	/** Generate our password file line **/
	snprintf(passwd_line, sizeof(passwd_line), "%s:%s\n", CXPASSWD.UserName, enc_pass);

	/** Open the password file **/
	passwd_file = fdOpen(CXPASSWD.PasswdFile, O_RDWR | O_CREAT, 0600);
	if (!passwd_file)
	    {
	    puts("could not open passwd file.");
	    exit(1);
	    }

	/** Read it into the xstring **/
	xsInit(&passwd_contents);
	while((len = fdRead(passwd_file, buf, sizeof(buf), 0, 0)) > 0)
	    {
	    xsConcatenate(&passwd_contents, buf, len);
	    }
	fdClose(passwd_file, 0);
	
	/** Do we already have this user? **/
	uname_len = strlen(CXPASSWD.UserName);
	offset = 0;
	found_user = -1;
	while((pos = xsFind(&passwd_contents, CXPASSWD.UserName, uname_len, offset)) >= 0)
	    {
	    if ((pos == 0 || xsString(&passwd_contents)[pos-1] == '\n') && xsString(&passwd_contents)[pos+uname_len] == ':')
		{
		/** Found it **/
		found_user = pos;
		found_len = strlen(xsString(&passwd_contents)+pos);
		if ((nlptr = strchr(xsString(&passwd_contents)+pos, '\n')) != NULL)
		    found_len = (nlptr - (xsString(&passwd_contents)+pos)) + 1;
		break;
		}
	    offset = pos+1;
	    }

	/** Replace if found, otherwise add the new user **/
	if (found_user >= 0)
	    xsSubst(&passwd_contents, found_user, found_len, passwd_line, strlen(passwd_line));
	else
	    xsConcatenate(&passwd_contents, passwd_line, strlen(passwd_line));

	/** Rewrite the entire file **/
	passwd_file = fdOpen(CXPASSWD.PasswdFile, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (!passwd_file)
	    {
	    puts("could not open password file to write to it.");
	    exit(1);
	    }

	if (fdWrite(passwd_file, xsString(&passwd_contents), strlen(xsString(&passwd_contents)), 0, FD_U_SEEK | FD_U_PACKET) != strlen(xsString(&passwd_contents)))
	    {
	    puts("could not re-write new password file.");
	    exit(1);
	    }

	/** Close the file **/
	fdClose(passwd_file, 0);

    exit(0);
    }


int 
main(int argc, char* argv[])
    {
    int ch;

	/** Default global values **/
	memset(&CXPASSWD,0,sizeof(CXPASSWD));
	strcpy(CXPASSWD.PasswdFile, "");
	CXPASSWD.ReadStdin = 0;
    
	/** Check for config file options on the command line **/
	while ((ch=getopt(argc,argv,"f:u:p:P")) > 0)
	    {
	    switch (ch)
	        {
		case 'f':	strtcpy(CXPASSWD.PasswdFile, optarg, sizeof(CXPASSWD.PasswdFile));
				break;
		case 'u':	strtcpy(CXPASSWD.UserName, optarg, sizeof(CXPASSWD.UserName));
				break;
		case 'p':	strtcpy(CXPASSWD.Password, optarg, sizeof(CXPASSWD.Password));
				break;
		case 'P':	CXPASSWD.ReadStdin = 1;
				break;
		default:
		case '?':
		case 'h':	printf("Usage:  cxpasswd [-u <username>] [-p <password>] [-P] [-f <passwdfile>]\n");
				exit(0);
		}
	    }

    mtInitialize(0, start);
    return 0;
    }

