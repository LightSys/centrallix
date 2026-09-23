#ifdef HAVE_CONFIG_H
#include "cxlibconfig.h"
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <stdbool.h>
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
#include "mtask.h"
#include "mtlexer.h"
#include "newmalloc.h"
#include "mtsession.h"
#include "xarray.h"
#include "xstring.h"
#include "xhash.h"
#include "strtcpy.h"
#include "warn.h"
#include "cxsec.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
    int		IsInitialized;
    }
    MSS;


/*** mssMemoryErr - called by newmalloc when a memory allocation fails.
 ***/
int
mssMemoryErr(char* message)
    {
    mssError(1,"NM","Memory error: %s",message);
    return 0;
    }


/*** mssFreeParam - release a session parameter and any value allocated
 *** for it.
 ***/
static int
mssFreeParam(void* param, void* arg)
    {
    pMtParam p = (pMtParam)param;

	if (p->IsAlloc) nmSysFree(p->Value);
	nmFree(p, sizeof(MtParam));

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
	strtcpy(MSS.AuthMethod, authmethod, sizeof(MSS.AuthMethod));
	strtcpy(MSS.AuthFile, authfile, sizeof(MSS.AuthFile));
	strtcpy(MSS.LogMethod, logmethod, sizeof(MSS.LogMethod));
	strtcpy(MSS.AppName, log_progname, sizeof(MSS.AppName));
	MSS.LogAllErrors = logall;

	/** Setup syslog **/
	openlog(MSS.AppName, LOG_PID, LOG_USER);
	if (!strcmp(MSS.LogMethod, "syslog"))
	    {
	    syslog(LOG_INFO, "%s initializing...", MSS.AppName);
	    }
   
	/** Setup the session list and the allocator hooks, once **/
	if (!MSS.IsInitialized)
	    {
	    xaInit(&(MSS.Sessions),16);
	    nmRegister(sizeof(MtSession),"MtSession");
	    nmSetErrFunction(mssMemoryErr);
	    MSS.IsInitialized = 1;
	    }

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
    char salt_buf[MSS_SALT_SIZE * 2 + 1];
    char *ptr;
    char *dstptr;
	
	/** Minimum salt length is 1 byte **/
	if (salt_len < 1) return -1;

	/** Expand the salt to (up to) 8 bytes **/
	if (salt_len > MSS_SALT_SIZE) salt_len = MSS_SALT_SIZE;
	ptr = salt;
	dstptr = salt_buf;
	while (ptr < salt + salt_len)
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
 ***
 *** bypass_crypt: set to 1 if we already know the credentials are
 *** correct and we just need to set up a new session.
 ***/
int 
mssAuthenticate(char* username, char* password, int bypass_crypt)
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

	/** Allocate a new session structure. **/
	s = (pMtSession)nmMalloc(sizeof(MtSession));
	if (!s) return -1;
	memset(s, 0, sizeof(MtSession));
	s->LinkCnt = 1;
	strtcpy(s->UserName, username, sizeof(s->UserName));
	strtcpy(s->Password, password, sizeof(s->Password));

	/** Sanity checking. **/
	if (strchr(username,':'))
	    {
	    mssError(1, "MSS", "Attempt to use invalid username '%s'", username);
	    cxsecShred(s, sizeof(MtSession));
	    nmFree(s,sizeof(MtSession));
	    return -1;
	    }

	/** Attempt to authenticate. **/
	if (!strcmp(MSS.AuthMethod,"system"))
	    {
	    /** Use system auth (passwd/shadow files) **/
	    pw = getpwnam(s->UserName);
	    if (!pw)
		{
		cxsecShred(s, sizeof(MtSession));
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

	    if (!bypass_crypt)
		{
		encrypted_pwd = (char*)crypt(s->Password,pwd);
		if (!encrypted_pwd || strcmp(encrypted_pwd,pwd))
		    {
		    cxsecShred(s, sizeof(MtSession));
		    nmFree(s,sizeof(MtSession));
		    return -1;
		    }
		}
	    }
	else if (!strcmp(MSS.AuthMethod, "altpasswd"))
	    {
	    /** Open the alternate password file **/
	    altpass_fd = fdOpen(MSS.AuthFile, O_RDONLY, 0600);
	    if (!altpass_fd)
		{
		mssErrorErrno(1, "MSS", "Could not open auth file '%s'", MSS.AuthFile);
		cxsecShred(s, sizeof(MtSession));
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
		    cxsecShred(s, sizeof(MtSession));
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

		if (!bypass_crypt)
		    {
		    encrypted_pwd = (char*)crypt(s->Password,pwd);
		    if (!encrypted_pwd || strcmp(encrypted_pwd,pwd))
			{
			cxsecShred(s, sizeof(MtSession));
			nmFree(s,sizeof(MtSession));
			return -1;
			}
		    }
		}
	    else
		{
		cxsecShred(s, sizeof(MtSession));
		nmFree(s,sizeof(MtSession));
		return -1;
		}
	    }
	else
	    {
	    mssError(1, "MSS", "Invalid auth method '%s'", MSS.AuthMethod);
	    cxsecShred(s, sizeof(MtSession));
	    nmFree(s,sizeof(MtSession));
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

	/** Unlink from thread with the unlink function off; it re-enters here **/
	if (s == cur_s)
	    {
	    thSetParamFunctions(NULL, mssLinkSession, NULL);
	    thSetParam(NULL,"mss",NULL);
	    thSetParamFunctions(NULL, mssLinkSession, mssUnlinkSession);
	    thSetUserID(NULL,0);
	    }

	/** Free the session info and error list **/
	for(i=0;i<s->ErrList.nItems;i++) nmSysFree(s->ErrList.Items[i]);
	xhClear(&s->Params, mssFreeParam, NULL);
	xhDeInit(&s->Params);
	xaDeInit(&(s->ErrList));
	xaRemoveItem(&(MSS.Sessions),xaFindItem(&(MSS.Sessions),(void*)s));
	cxsecShred(s, sizeof(MtSession));
	nmFree(s,sizeof(MtSession));

    return 0;
    }


/*** mss_i_error - Displays error text to the user (but no stack trace).
 *** Does not exit the program, allowing the calling function to fail,
 *** creating a cascade of error messages which provides useful info.
 ***
 *** Note: The format is parsed using vsnprintf(), so edge cases like a %s on
 *** 	a value that isn't a valid string rely on glibc's implementation of C
 *** 	undefined behavior.
 ***
 *** @param clr Whether to clear the current error stack.  As a rule of thumb,
 ***	if you are the first one to detect the error, clear the stack so that
 ***	other unrelated messages are not shown.  If you are detecting an error
 ***	from another function that may also call an mssError() function, do
 ***	not clear the stack.
 *** @param module The name or abbreviation of the module in which this 
 ***	function is being called, to help developers narrow down the location
 ***	of the error.
 *** @param file The name of the file where the error was detected.
 *** @param line The line number where the error was detected.
 *** @param format The format text for the error, which accepts any format
 ***	specifier that would be accepted by printf().
 *** @param ... Variables matching format specifiers in the format.
 ***/
void
mss_i_error(int clr, char* module, char* file, int line, char* message, ...)
    {
    char* err_msg = NULL;
    char fallback_msg[256];
    XString err_msg_xstring; err_msg_xstring.String = NULL;

	/** Prevent issues from interlacing this function with prints to stdout. **/
	warnFail(fflush(stdout));

	/** Attempt to format the error into an XString. **/
	if (warnFail(xsInit(&err_msg_xstring)) == 0)
	    {
	    bool format_ok = true;

	    /*** Write the source location and the module in front of the message.
	     *** xsConcatPrintf() only implements a subset of printf(), but %s and %d
	     *** are both implemented.
	     ***/
	    format_ok &= (warnNeg(xsConcatPrintf(&err_msg_xstring, "%s:%d: %s: ", file, line, module)) >= 0);

	    /*** Append the caller's message.  This goes through xsGenPrintf_va()
	     *** rather than xsConcatPrintf() because the latter does not use
	     *** vsnprintf() so it only supports some printf() functionality.
	     *** xsWrite() appends when given no XS_U_SEEK.
	     ***/
	    va_list args;
	    va_start(args, message);
	    format_ok &= (warnNeg(xsGenPrintf_va(xsWrite, &err_msg_xstring, NULL, NULL, message, args)) >= 0);
	    va_end(args);

	    /** Get the error message from the xstring. **/
	    if (format_ok) err_msg = warnNull(xsString(&err_msg_xstring));
	    }

	/*** Fallback: If formatting fails, format into a fixed-size buffer on
	 *** the stack instead.  A truncated message is better than a generic
	 *** fail or a silent error that logs nothing.
	 ***/
	if (UNLIKELY(err_msg == NULL))
	    {
	    fprintf(stderr, "%s:%d: %s: Failed to format the error message with an XString.\n", file, line, module);

	    /** Write the source location and the module in front of the message. **/
	    int prefix_len = snprintf(fallback_msg, sizeof(fallback_msg), "%s:%d: %s: ", file, line, module);
	    if (prefix_len < 0 || (size_t)prefix_len >= sizeof(fallback_msg)) prefix_len = 0;

	    /** Append the caller's message. **/
	    va_list args;
	    va_start(args, message);
	    if (warnNeg(vsnprintf(fallback_msg + prefix_len, sizeof(fallback_msg) - prefix_len, message, args)) >= 0)
		err_msg = fallback_msg;
	    va_end(args);
	    }

	/** Fallback: If all formatting fails, just use the unformatted message. **/
	if (UNLIKELY(err_msg == NULL))
	    {
	    fprintf(stderr, "Failed to format the error message at all.\n");
	    err_msg = message;
	    }

	/** Get current session (returns NULL if running outside session context). **/
	pMtSession s = thGetParam(NULL, "mss");
	const bool log_error = (s == NULL || MSS.LogAllErrors);

	/** Use standard logging without a session context, if needed. **/
	if (log_error)
	    {
	    /** Use the requested logging method. **/
	    if (strcmp(MSS.LogMethod, "syslog") == 0)
		{
		if (s == NULL)
		    syslog(LOG_ERR, "System: %.256s\n", err_msg);
		else
		    syslog(LOG_WARNING, "User '%s': %.256s\n", s->UserName, err_msg);
		}
	    else if (strcmp(MSS.LogMethod, "stdout") == 0)
		{
		printf("%s: %s\n", (MSS.AppName[0]) ? MSS.AppName : "error", err_msg);
		warnFail(fflush(stdout));
		}
	    }

	/** If a session is available, try to add the error to the error list. **/
	if (s != NULL)
	    {
	    /** Clear the error context, if requested. **/
	    if (clr) mssClearError();

	    /** Allocate space and construct the error text. **/
	    char* allocated_err_msg = warnNull(nmSysStrdup(err_msg));
	    if (UNLIKELY(allocated_err_msg == NULL))
		{
		fprintf(stderr, "Failed to store error message: %s\n", err_msg);
		goto end; /* Give up. */
		}

	    /** Store the error. **/
	    if (warnNeg(xaAddItem(&(s->ErrList), (void*)allocated_err_msg)) < 0)
		{
		fprintf(stderr, "Failed to add error message to session error list: %s\n", err_msg);
		nmSysFree(allocated_err_msg);
		goto end; /* Give up. */
		}
	    }

    end:
	/** Clean up. **/
	if (LIKELY(err_msg_xstring.String != NULL))
	    warnFail(xsDeInit(&err_msg_xstring));

	/** Force all warnings/errors to be printed. **/
	warnFail(fflush(stderr));

	return;
    }


/*** mssClearError - removes all error messages from the current error
 *** stack.
 ***/
void
mssClearError()
    {
	/** Get session pointer. **/
	pMtSession s = warnNull(thGetParam(NULL, "mss"));
	if (s == NULL) return; /* No errors to clear. */

	/** Free all error strings in the error list/error stack. **/
	warnFail(xaClear(&s->ErrList, (void*)nmSysFree, NULL));
    }


/*** mssPrintError - prints the current error stack out to the given file
 *** descriptor.  Error handling in this function is a bit strange because
 *** it's on the error handling path, so we can't call mssError().
 ***/
int
mssPrintError(pFile fd)
    {
    XString str; str.String = NULL;
    int rval = -1, tmp;

	if (UNLIKELY(fd == NULL)) goto end;
	if (warnFail(xsInit(&str))) goto end;

	/** Format the stack once, so both error printers agree on the layout. **/
	tmp = warnFail(mssStringError(&str));
	if (UNLIKELY(tmp != 0))
	    {
	    rval = tmp;
	    goto end;
	    }
	if (warnNeg(fdWrite(fd, xsString(&str), xsLength(&str), 0, 0)) < 0) goto end;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0)) /* Make sure we print something if a failure happenned. */
	    fprintf(stderr, "Warning: Failed to print session errors.\n");

	/** Clean up. **/
	if (LIKELY(str.String != NULL))
	    warnFail(xsDeInit(&str));

	return rval;
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
    char* sep;

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s) return -1;

	/*** Create a space-separated string of the messages, without the source
	 *** location and module code that mss_i_error() writes in front
	 *** of each one.  Both end in ": ", which the message itself may also
	 *** contain, so only the first two are skipped.
	 ***/
	for(i=s->ErrList.nItems-1;i>=0;i--)
	    {
	    item = (char*)(s->ErrList.Items[i]);
	    if (item)
		{
		sep = strstr(item, ": ");
		if (sep) sep = strstr(sep + 2, ": ");
		if (sep)
		    item = sep + 2;
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
    char name[MSS_PARAMNAME_SIZE];

	/** Handle edge cases. **/
	if (paramname == NULL)
	    {
	    mssError(1, "MSS", "paramname cannot be null.");
	    goto error;
	    }

	/** Get session context. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (s == NULL)
	    {
	    mssError(1, "MSS", "Cannot set session param ptr outside of session context.");
	    goto error;
	    }

	/** The name has to fit the field it is kept in **/
	if (strtcpy(name, paramname, sizeof(name)) < 0)
	    {
	    mssError(1, "MSS", "Failed to copy paramname: \"%s\".", paramname);
	    goto error;
	    }

    	/** Need to delete first? **/
	if (!(p = (pMtParam)xhLookup(&s->Params, name)))
	    {
	    p = nmMalloc(sizeof(MtParam));
	    if (p == NULL)
		{
		mssError(1, "MSS", "Failed to allocate MtParam.");
		goto error;
		}
	    strcpy(p->Name, name);
	    is_new = 1;
	    }
	else if (p->Value == ptr)
	    {
	    /** Nothing changes, and the value stays whosever it was **/
	    return 0;
	    }
	else if (p->IsAlloc)
	    {
	    nmSysFree(p->Value);
	    }

	p->Value = ptr;
	p->IsAlloc = 0;
	if (is_new && xhAdd(&s->Params, p->Name, (void*)p) != 0)
	    {
	    mssError(1, "MSS", "Failed to add param pointer to xhash.");
	    goto error;
	    }

	return 0;

    error:
	mssError(1, "MSS", "Failed to set session parameter pointer.");
	return -1;
    }


/*** mssSetParam - sets a session parameter, for generic use.
 ***/
int
mssSetParam(char* paramname, void* value)
    {
    pMtSession s;
    pMtParam p;
    char* new_value;
    int is_new = 0;
    char name[MSS_PARAMNAME_SIZE];

	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s || !paramname || !value) return -1;

	/** The name has to fit the field it is kept in **/
	if (strtcpy(name, paramname, sizeof(name)) < 0) return -1;

    	/** Need to delete first? **/
	if (!(p = (pMtParam)xhLookup(&s->Params, name)))
	    {
	    p = (pMtParam)nmMalloc(sizeof(MtParam));
	    if (!p) return -1;
	    strcpy(p->Name, name);
	    p->IsAlloc = 0;
	    is_new = 1;
	    }

	/** Take the new value in before letting go of the old one **/
	if (strlen(value) < sizeof(p->ValueBuf))
	    {
	    new_value = p->ValueBuf;
	    }
	else
	    {
	    new_value = (char*)nmSysMalloc(strlen(value)+1);
	    if (!new_value)
		{
		if (is_new) nmFree(p, sizeof(MtParam));
		return -1;
		}
	    }
	memmove(new_value, value, strlen(value)+1);
	if (p->IsAlloc && p->Value != new_value) nmSysFree(p->Value);
	p->Value = new_value;
	p->IsAlloc = (new_value != p->ValueBuf);
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
    char name[MSS_PARAMNAME_SIZE];

	/** Get session. **/
	s = (pMtSession)thGetParam(NULL,"mss");
	if (!s || !paramname) return NULL;

	/** The name has to fit the field it is kept in **/
	if (strtcpy(name, paramname, sizeof(name)) < 0) return NULL;

    	p = (pMtParam) xhLookup(&s->Params, name);
	if (!p) return NULL;

    return p->Value;
    }
