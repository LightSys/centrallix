#ifdef HAVE_CONFIG_H
#include "cxlibconfig.h"
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include <locale.h>
#include "mtask.h"
#include "mtlexer.h"
#include "newmalloc.h"
#include "mtsession.h"
#include "xarray.h"
#include "xstring.h"
#include "xhash.h"
#include "strtcpy.h"

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


/*** mssLog - write to syslog
 ***/
int
mssLog(int level, char* msg)
    {
    syslog(level, "%s", msg);
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

	/** Setup syslog **/
	openlog(MSS.AppName, LOG_PID, LOG_USER);
	if (!strcmp(MSS.LogMethod, "syslog"))
	    {
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


/*** mssGenCred - generate credential used for authentication.  This can
 *** be stored in a cxpasswd file.  The credential points to a buffer where
 *** we can store the encrypted version of the password.  'salt' and 'salt_len'
 *** should indicate a series of random bytes.  How many we use depends on
 *** whether the system supports MD5 passwords or not.  cred_maxlen indicates
 *** the size of the buffer pointed to by credential.  If there is not enough
 *** room, then we fail (return -1).
 ***
 *** Optimum salt length for MD5 passwords is 4 bytes - we expand it to 8 hex
 *** bytes.  Salt should be full-range random bytes (0x00 - 0xFF).
 ***
 *** Successful return is 0.
 ***/
int
mssGenCred(char* salt, int salt_len, char* password, char* credential, int cred_maxlen)
    {
    char salt_chars[] = "0123456789abcdef";
    char salt_buf[9];
    char *ptr;
    char *dstptr;
	
	/** Minimum salt length is 1 byte **/
	if (salt_len < 1) return -1;

	/** Expand the salt to (up to) 8 bytes **/
	ptr = salt;
	dstptr = salt_buf;
	while (*ptr)
	    {
	    *(dstptr++) = salt_chars[ptr[0] & 0xF];
	    *(dstptr++) = salt_chars[(ptr[0]>>4) & 0xF];
	    ptr++;
	    }
	*dstptr = '\0';

	/** Try MD5 style **/
	if (cred_maxlen >= 35)
	    {
	    /** Create initial salt for crypt() **/
	    snprintf(credential, 35, "$1$%s$", salt_buf);

	    /** Encrypt the password **/
	    ptr = crypt(password, credential);

	    /** Success? **/
	    if (ptr && strlen(ptr) >= 27)
		{
		strtcpy(credential, ptr, cred_maxlen);
		return 0;
		}
	    }

	/** Try DES style **/
	if (cred_maxlen >= 14)
	    {
	    /** Create initial salt for crypt() **/
	    snprintf(credential, 14, "%.2s", salt_buf);

	    /** Encrypt the password **/
	    ptr = crypt(password, credential);

	    /** Success? **/
	    if (ptr)
		{
		strtcpy(credential, ptr, cred_maxlen);
		return 0;
		}
	    }

    return -1;
    }


/*** mssLinkSess -- keep track of how many threads are using this
 *** session structure.
 ***/
int
mssLinkSession(pMtSession s)
    {
    s->LinkCnt++;
    return 0;
    }


/*** mssUnlinkSess -- on final unlink, we end the session.
 ***/
int
mssUnlinkSession(pMtSession s)
    {
    s->LinkCnt--;
    if (s->LinkCnt <= 0)
	mssEndSession(s);
    return 0;
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
    struct passwd* pw = NULL;
#ifdef HAVE_SHADOW_H
    struct spwd* spw;
#endif
    char salt[3];
    pFile altpass_fd;
    pLxSession altpass_lxs;
    char pwline[80];
    int t;
    int found_user;
    gid_t grps[16];
    int n_grps;
    int mlxFlags;
    char * locale;

	/** Allocate a new session structure. **/
	s = (pMtSession)nmMalloc(sizeof(MtSession));
	if (!s) return -1;
	s->LinkCnt = 1;
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
#ifdef HAVE_SHADOW_H
	    spw = getspnam(s->UserName);
	    if (!spw)
		{
#endif
		pwd = pw->pw_passwd;
#ifdef HAVE_SHADOW_H
		}
	    else
		{
		pwd = spw->sp_pwdp;
		}
#endif
	    strncpy(salt,pwd,2);
	    salt[2]=0;
	    encrypted_pwd = (char*)crypt(s->Password,pwd);
	    if (!encrypted_pwd || strcmp(encrypted_pwd,pwd))
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
	    mlxFlags = MLX_F_LINEONLY | MLX_F_EOF;
	    locale = setlocale(LC_CTYPE, NULL);
	    if(strstr(locale, "UTF-8") || strstr(locale, "UTF8") || strstr(locale, "utf-8") || strstr(locale, "utf8"))
		mlxFlags |= MLX_F_ENFORCEUTF8;
	    altpass_lxs = mlxOpenSession(altpass_fd, mlxFlags);

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
	    {
	    s->UserID = pw->pw_uid;
	    s->GroupID = pw->pw_gid;
	    initgroups(username, s->GroupID);
	    }
	else
	    {
	    s->UserID = geteuid();
	    s->GroupID = getegid();
	    }
	n_grps = getgroups(sizeof(grps) / sizeof(gid_t), grps);
	if (n_grps < 0 || n_grps > sizeof(grps) / sizeof(gid_t))
	    n_grps = 0;
	thSetParam(NULL,"mss",(void*)s);
	thSetParamFunctions(NULL, mssLinkSession, mssUnlinkSession);
	thSetSupplementalGroups(NULL, n_grps, grps);
	thSetGroupID(NULL,s->GroupID);
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
mssEndSession(pMtSession s)
    {
    int i;
    pMtSession cur_s;

	/** Get session info. **/
	cur_s = thGetParam(NULL, "mss");
	if (!s)
	    {
	    s = cur_s;
	    if (!s) return -1;
	    }

	/** Unlink from thread if this is the current thread's session **/
	if (s == cur_s)
	    {
	    thSetParam(NULL,"mss",NULL);
	    thSetUserID(NULL,0);
	    }

	/** Free the session info and error list **/
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
    char ch;

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
		case 'c':
		    ch = va_arg(vl, int);
		    xsConcatenate(&xs, &ch, 1);
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
	    else if (!strcmp(MSS.LogMethod, "stdout"))
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
	snprintf(sbuf,200,"ERROR - Session By Username [%s]\r\n",s->UserName);
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
    int i;
    pMtSession s;

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

	/** Print the error stack. **/
	xsConcatPrintf(str, "ERROR - Session By Username [%s]\r\n", s->UserName);
	for(i=s->ErrList.nItems-1;i>=0;i--)
	    {
	    xsConcatPrintf(str, "--- %s\r\n", (char*)(s->ErrList.Items[i]));
	    }
    	
    return 0;
    }


/*** mssUserError() - returns a user-friendly version of the error message
 *** stack (i.e., without the module codes).
 ***/
int
mssUserError(pXString str)
    {
    int i;
    pMtSession s;
    char* item;
    char* colon;

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

	/** Create a space-separated string of the messages, without module codes **/
	for(i=s->ErrList.nItems-1;i>=0;i--)
	    {
	    item = (char*)(s->ErrList.Items[i]);
	    if (item)
		{
		colon = strchr(item, ':');
		if (colon)
		    item = colon + 2;
		xsConcatenate(str, item, -1);
		if (i > 0)
		    xsConcatenate(str, " ", 1);
		}
	    }

    return 0;
    }


/*** mssSetParamPtr - sets a session parameter, given an opaque
 *** pointer.  For strings, use mssSetParam().
 ***/
int
mssSetParamPtr(char* paramname, void* ptr)
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

	p->Value = ptr;
	if (is_new) xhAdd(&s->Params, p->Name, (void*)p);

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
	if (is_new) xhAdd(&s->Params, p->Name, (void*)p);

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

