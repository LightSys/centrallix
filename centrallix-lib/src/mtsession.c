#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <shadow.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include "mtask.h"
#include "mtlexer.h"
#include "newmalloc.h"
#include "mtsession.h"
#include "xarray.h"
#include "xstring.h"
#include "xhash.h"

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
/* Module:	MTSession session manager (mtsession.c, mtsession.h)    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	November 4, 1998                                        */
/* Description:	Session management module to complement the MTASK	*/
/*		module.  Maintains user/password authentication.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: mtsession.c,v 1.5 2002/05/03 03:46:29 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/mtsession.c,v $

    $Log: mtsession.c,v $
    Revision 1.5  2002/05/03 03:46:29  gbeeley
    Modifications to xhandle to support clearing the handle list.  Added
    a param to xhClear to provide support for xhnClearHandles.  Added a
    function in mtask.c to allow the retrieval of ticks-since-boot without
    making a syscall.  Fixed an MTASK bug in the scheduler relating to
    waiting on timers and some modulus arithmetic.

    Revision 1.4  2002/03/23 06:25:10  gbeeley
    Updated MSS to have a larger error string buffer, as a lot of errors
    were getting chopped off.  Added BDQS protocol files with some very
    simple initial implementation.

    Revision 1.3  2002/02/14 00:51:17  gbeeley
    Fixed a problem where if an error occurs before mssInitialize(), it
    would default progname to "".  Now it defaults to "error", which causes
    the error messages to look nicer.

    Revision 1.2  2002/02/14 00:41:54  gbeeley
    Added configurable logging and authentication to the mtsession module,
    and made sure mtsession cleared MtSession data structures when it is
    through with them since they contain sensitive data.

    Revision 1.1.1.1  2001/08/13 18:04:22  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:02:52  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/

#ifndef crypt
char *crypt(const char* key, const char* salt);
#endif

/*** Globals ***/
static struct
    {
    XArray	Sessions;
    char	AuthMethod[32];
    char	AuthFile[256];
    char	LogMethod[32];
    int		LogAllErrors;
    char	AppName[32];
    }
    MSS;


/*** mssMemoryErr - called by newmalloc when a memory allocation fails.
 ***/
int
mssMemoryErr(char* message)
    {
    mssError(1,"NM",message);
    return 0;
    }


/*** mssInitialize - init the globals, etc.
 ***/
int 
mssInitialize(char* authmethod, char* authfile, char* logmethod, int logall, char* log_progname)
    {

	/** Setup auth method & log method settings **/
	memccpy(MSS.AuthMethod, authmethod, 0, 31);
	MSS.AuthMethod[31] = '\0';
	memccpy(MSS.AuthFile, authfile, 0, 255);
	MSS.AuthFile[255] = '\0';
	memccpy(MSS.LogMethod, logmethod, 0, 31);
	MSS.LogMethod[31] = '\0';
	memccpy(MSS.AppName, log_progname, 0, 31);
	MSS.AppName[31] = '\0';
	MSS.LogAllErrors = logall;

	/** Setup syslog, if requested. **/
	if (!strcmp(MSS.LogMethod, "syslog"))
	    {
	    openlog(MSS.AppName, LOG_PID, LOG_USER);
	    syslog(LOG_INFO, "%s initializing...", MSS.AppName);
	    }
   
	/** Setup the sessions list **/
	xaInit(&(MSS.Sessions),16);
	nmRegister(sizeof(MtSession),"MtSession");
	nmSetErrFunction(mssMemoryErr);

    return 0;
    }


/*** mssUserName - get the name of the current user.
 ***/
char* 
mssUserName()
    {
    pMtSession s;

	/** Get session info from mtask. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return NULL;

    return s->UserName;
    }


/*** mssPassword - get the password used by the current user
 *** to authenticate.
 ***/
char* 
mssPassword()
    {
    pMtSession s;

	/** Get session info from mtask. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return NULL;

    return s->Password;
    }


/*** mssAuthenticate - start a new session, overwriting previous
 *** (inherited) session information.
 ***/
int 
mssAuthenticate(char* username, char* password)
    {
    pMtSession s;
    char* encrypted_pwd;
    char* pwd;
    struct passwd* pw;
    struct spwd* spw;
    char salt[3];
    pFile altpass_fd;
    pLxSession altpass_lxs;
    char pwline[80];
    int t;
    int found_user;

	/** Allocate a new session structure. **/
	s = (pMtSession)nmMalloc(sizeof(MtSession));
	if (!s) return -1;
	strncpy(s->UserName, username, 31);
	s->UserName[31]=0;
	strncpy(s->Password, password, 31);
	s->Password[31]=0;

	/** Attempt to authenticate. **/
	if (!strcmp(MSS.AuthMethod,"system"))
	    {
	    /** Use system auth (passwd/shadow files) **/
	    pw = getpwnam(s->UserName);
	    if (!pw)
		{
		memset(s, 0, sizeof(MtSession));
		nmFree(s,sizeof(MtSession));
		return -1;
		}
	    spw = getspnam(s->UserName);
	    if (!spw)
		{
		pwd = pw->pw_passwd;
		}
	    else
		{
		pwd = spw->sp_pwdp;
		}
	    strncpy(salt,pwd,2);
	    salt[2]=0;
	    encrypted_pwd = (char*)crypt(s->Password,pwd);
	    if (strcmp(encrypted_pwd,pwd))
		{
		memset(s, 0, sizeof(MtSession));
		nmFree(s,sizeof(MtSession));
		return -1;
		}
	    }
	else if (!strcmp(MSS.AuthMethod, "altpasswd"))
	    {
	    /** Sanity checking. **/
	    if (strchr(username,':'))
		{
		mssError(1, "MSS", "Attempt to use invalid username '%s'", username);
		memset(s, 0, sizeof(MtSession));
		nmFree(s,sizeof(MtSession));
		return -1;
		}

	    /** Open the alternate password file **/
	    altpass_fd = fdOpen(MSS.AuthFile, O_RDONLY, 0600);
	    if (!altpass_fd)
		{
		mssErrorErrno(1, "MSS", "Could not open auth file '%s'", MSS.AuthFile);
		memset(s, 0, sizeof(MtSession));
		nmFree(s,sizeof(MtSession));
		return -1;
		}
	    altpass_lxs = mlxOpenSession(altpass_fd, MLX_F_LINEONLY | MLX_F_EOF);

	    /** Scan it for the user name **/
	    found_user = 0;
	    while ((t = mlxNextToken(altpass_lxs)) != MLX_TOK_EOF)
		{
		if (t == MLX_TOK_ERROR)
		    {
		    mssError(0, "MSS", "Could not read auth file '%s'", MSS.AuthFile);
		    memset(s, 0, sizeof(MtSession));
		    nmFree(s,sizeof(MtSession));
		    mlxCloseSession(altpass_lxs);
		    fdClose(altpass_fd, 0);
		    return -1;
		    }
		mlxCopyToken(altpass_lxs, pwline, 80);
		if (strlen(username) < strlen(pwline) && !strncmp(pwline, username, strlen(username)) && pwline[strlen(username)] == ':')
		    {
		    found_user = 1;
		    break;
		    }
		}

	    /** Close the alternate password file **/
	    mlxCloseSession(altpass_lxs);
	    fdClose(altpass_fd, 0);

	    /** Did we find the user in the file? **/
	    if (found_user)
		{
		if (pwline[strlen(pwline)-1] == '\n')
		    pwline[strlen(pwline)-1] = '\0';
		pwd = pwline + strlen(username) + 1;
		encrypted_pwd = (char*)crypt(s->Password,pwd);
		if (strcmp(encrypted_pwd,pwd))
		    {
		    memset(s, 0, sizeof(MtSession));
		    nmFree(s,sizeof(MtSession));
		    return -1;
		    }
		}
	    else
		{
		memset(s, 0, sizeof(MtSession));
		nmFree(s,sizeof(MtSession));
		return -1;
		}
	    }
	else
	    {
	    mssError(1, "MSS", "Invalid auth method '%s'", MSS.AuthMethod);
	    return -1;
	    }

	/** Set the session information **/
	if (!strcmp(MSS.AuthMethod,"system"))
	    s->UserID = pw->pw_uid;
	else
	    s->UserID = geteuid();
	thSetParam(NULL,"mss",(void*)s);
	thSetUserID(NULL,s->UserID);

	/** Initialize the error info **/
	xaInit(&(s->ErrList), 16);
	xhInit(&s->Params, 17, 0);

	/** Add to session list **/
	xaAddItem(&(MSS.Sessions), (void*)s);

    return 0;
    }


/*** mssEndSession - end a session and free the session information.
 ***/
int 
mssEndSession()
    {
    pMtSession s;
    int i;

	/** Get session info. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

	/** Free the session info and error list **/
	thSetParam(NULL,"mss",NULL);
	thSetUserID(NULL,0);
	for(i=0;i<s->ErrList.nItems;i++) nmSysFree(s->ErrList.Items[i]);
	xhClear(&s->Params, NULL, NULL);
	xhDeInit(&s->Params);
	xaDeInit(&(s->ErrList));
	xaRemoveItem(&(MSS.Sessions),xaFindItem(&(MSS.Sessions),(void*)s));
	memset(s, 0, sizeof(MtSession));
	nmFree(s,sizeof(MtSession));

    return 0;
    }


/*** mssError - Add an error message to the error stack, optionally 
 *** clearing the existing contents thereof.
 ***/
int 
mssError(int clr, char* module, char* message, ...)
    {
    va_list vl;
    char* msg;
    pMtSession s;
    XString xs;
    char* ptr;
    char* cur_pos;
    char* str;
    int i;
    char nbuf[16];

    	/** Build the real error msg. **/
	xsInit(&xs);
	cur_pos = message;
	va_start(vl, message);
	while((ptr = strchr(cur_pos, '%')))
	    {
	    xsConcatenate(&xs, cur_pos, ptr - cur_pos);
	    switch(ptr[1])
	        {
		case '\0':
		    xsConcatenate(&xs, "%", 1);
		    cur_pos = ptr+1;
		    break;
		case '%':
		    xsConcatenate(&xs, "%", 1);
		    cur_pos = ptr+2;
		    break;
		case 's':
		    str = va_arg(vl, char*);
		    xsConcatenate(&xs, str?str:"(NULL)", -1);
		    cur_pos = ptr + 2;
		    break;
		case 'd':
		    i = va_arg(vl, int);
		    sprintf(nbuf,"%d",i);
		    xsConcatenate(&xs, nbuf, -1);
		    cur_pos = ptr + 2;
		    break;
		default:
		    cur_pos = ptr + 2;
		    break;
		}
	    }
	va_end(vl);
	if (*cur_pos) xsConcatenate(&xs, cur_pos, -1);

	/** Get current session **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s || MSS.LogAllErrors) 
	    {
	    /*printf("mssError: Error occurred outside of session context.\n");*/
	    if (!strcmp(MSS.LogMethod,"syslog"))
		{
		if (!s)
		    syslog(LOG_ERR, "System: %s: %.256s\n", module, xs.String);
		else
		    syslog(LOG_WARNING, "User '%s': %s: %.256s\n", s->UserName, module, xs.String);
		}
	    else
		{
		printf("%s: %s: %s\n",MSS.AppName[0]?MSS.AppName:"error",module,xs.String);
		}
	    if (!s) return -1;
	    }

	/** Need to clear? **/
	if (clr) mssClearError();

	/** Allocate space and construct the error text. **/
	msg = (char*)nmSysMalloc(strlen(module)+strlen(xs.String)+3);
	if (!msg)
	    {
	    perror("mssError: Could not allocate error");
	    printf("mssError: %s: %s\n",module,xs.String);
	    return -1;
	    }
	sprintf(msg,"%s: %s",module,xs.String);
	xaAddItem(&(s->ErrList),(void*)msg);
	xsDeInit(&xs);

    return 0;
    }


/*** mssErrorErrno - Adds an error to the error stack, but in this
 *** case it takes the error information from the current errno.
 ***/
int 
mssErrorErrno(int clr, char* module, char* message, ...)
    {
    va_list vl;
    char* msg;
    char* err;
    pMtSession s;
    int en;
    char* str;
    int i;
    XString xs;
    char nbuf[16];
    char* cur_pos;
    char* ptr;

    	/** Build the real error msg. **/
	xsInit(&xs);
	cur_pos = message;
	va_start(vl, message);
	while((ptr = strchr(cur_pos, '%')))
	    {
	    xsConcatenate(&xs, cur_pos, ptr - cur_pos);
	    switch(ptr[1])
	        {
		case '\0':
		    xsConcatenate(&xs, "%", 1);
		    cur_pos = ptr+1;
		    break;
		case '%':
		    xsConcatenate(&xs, "%", 1);
		    cur_pos = ptr+2;
		    break;
		case 's':
		    str = va_arg(vl, char*);
		    xsConcatenate(&xs, str?str:"(NULL)", -1);
		    cur_pos = ptr + 2;
		    break;
		case 'd':
		    i = va_arg(vl, int);
		    sprintf(nbuf,"%d",i);
		    xsConcatenate(&xs, nbuf, -1);
		    cur_pos = ptr + 2;
		    break;
		default:
		    cur_pos = ptr + 2;
		    break;
		}
	    }
	va_end(vl);
	if (*cur_pos) xsConcatenate(&xs, cur_pos, -1);

	/** Get current errno. **/
	en = errno;
	err = strerror(en);

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s || MSS.LogAllErrors) 
	    {
	    /*printf("mssErrorErrno: Error occurred outside of session context.\n");*/
	    if (!strcmp(MSS.LogMethod,"syslog"))
		{
		if (!s)
		    syslog(LOG_ERR, "System: %s: %.256s (%s)\n", module, xs.String, err);
		else
		    syslog(LOG_WARNING, "User '%s': %s: %.256s (%s)\n", s->UserName, module, xs.String, err);
		}
	    else
		{
		printf("%s: %s: %s (%s)\n",MSS.AppName[0]?MSS.AppName:"error",module,xs.String,err);
		}
	    if (!s) return -1;
	    }

	/** Need to clear? **/
	if (clr) mssClearError();

	/** Allocate space and construct the error text. **/
	msg = (char*)nmSysMalloc(strlen(module)+strlen(xs.String)+6 + strlen(err));
	if (!msg)
	    {
	    perror("mssErrorErrno: Could not allocate error");
	    printf("mssErrorErrno: %s: %s (%s)\n",module,xs.String,err);
	    return -1;
	    }
	sprintf(msg,"%s: %s (%s)",module,xs.String,err);
	xaAddItem(&(s->ErrList),(void*)msg);
	xsDeInit(&xs);

    return 0;
    }


/*** mssClearError - removes all error messages from the current error
 *** stack.
 ***/
int 
mssClearError()
    {
    int i;
    pMtSession s;

	/** Get session pointer **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

	/** Scan through list, freeing items **/
	for(i=0;i<s->ErrList.nItems;i++) nmSysFree(s->ErrList.Items[i]);

	/** Zero the list. **/
	s->ErrList.nItems = 0;

    return 0;
    }


/*** mssPrintError - prints the current error stack out to the given file
 *** descriptor.
 ***/
int 
mssPrintError(pFile fd)
    {
    int i;
    pMtSession s;
    char sbuf[200];

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

	/** Print the error stack. **/
	sprintf(sbuf,"ERROR - Session By Username [%s]\r\n",s->UserName);
	fdWrite(fd,sbuf,strlen(sbuf),0,0);
	for(i=s->ErrList.nItems-1;i>=0;i--)
	    {
	    snprintf(sbuf,200,"--- %s\r\n",(char*)(s->ErrList.Items[i]));
	    fdWrite(fd,sbuf,strlen(sbuf),0,0);
	    }

    return 0;
    }


/*** mssStringError - copies the current error information into a newly
 *** allocated string.
 ***/
int
mssStringError(pXString str)
    {
    char sbuf[200];
    int i;
    pMtSession s;

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

	/** Print the error stack. **/
	sprintf(sbuf,"ERROR - Session By Username [%s]\r\n",s->UserName);
	xsConcatenate(str,sbuf,-1);
	for(i=s->ErrList.nItems-1;i>=0;i--)
	    {
	    snprintf(sbuf,200,"--- %s\r\n",(char*)(s->ErrList.Items[i]));
	    xsConcatenate(str,sbuf, -1);
	    }
    	
    return 0;
    }


/*** mssSetParam - sets a session parameter, for generic use.
 ***/
int
mssSetParam(char* paramname, void* value)
    {
    pMtSession s;
    pMtParam p;
    int is_new = 0;

	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

    	/** Need to delete first? **/
	if (!(p = (pMtParam)xhLookup(&s->Params, paramname)))
	    {
	    p = (pMtParam)nmMalloc(sizeof(MtParam));
	    memccpy(p->Name, paramname, 0, 31);
	    p->Name[31] = 0;
	    is_new = 1;
	    }
	else
	    {
	    if (p->Value != p->ValueBuf && p->Value) nmSysFree(p->Value);
	    }

	if (strlen(value) < 64)
	    {
	    p->Value = p->ValueBuf;
	    }
	else
	    {
	    p->Value = (char*)nmSysMalloc(strlen(value)+1);
	    }
	strcpy(p->Value, value);
	if (is_new) xhAdd(&s->Params, paramname, (void*)p);

    return 0;
    }


/*** mssGetParam - returns the value of the named session parameter.
 ***/
void*
mssGetParam(char* paramname)
    {
    pMtSession s;
    pMtParam p;

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return NULL;

    	p = (pMtParam) xhLookup(&s->Params, paramname);
	if (!p) return NULL;

    return p->Value;
    }

