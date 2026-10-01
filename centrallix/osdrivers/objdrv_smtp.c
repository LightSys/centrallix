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
/* Module: 	objdrv_smtp.c						*/
/* Authors:	Hazen Johnson, Justin Southworth			*/
/* Creation:	May 29, 2014						*/
/* Description:	Provides an email interface for Centrallix through the	*/
/*		ObjectSystem.						*/
/*									*/
/*		Current Shortcomings:					*/
/*		  - All functionality is perfect... There is no		*/
/*		  functionality.					*/
/*									*/
/************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "centrallix.h"
#include "cxlib/expect.h"
#include "cxlib/magic.h"
#include "cxlib/strtcpy.h"
#include "cxlib/xarray.h"
#include "cxss/cxss.h"
#include "obj.h"
#include "st_node.h"


/** Debugging mode **/
#define	SMTP_DEBUG	0

/** Define types of SMTP objects. **/
#define SMTP_T_ROOT	0
#define SMTP_T_EML	1

/** Seconds to keep a sent or failed email (3 days). **/
#define SMTP_DEFAULT_EXPIRE_TIME	(3 * 24 * 60 * 60)

/** Minimum seconds between sweeps of one spool directory. **/
#define SMTP_SWEEP_INTERVAL	(60 * 60)

/** Seconds to wait for sendmail before killing it. **/
#define SMTP_SENDMAIL_TIMEOUT	60

/** Bytes in the status line at the start of a sendmail result file. **/
#define SMTP_RESULT_HEADER_LEN	16

/** Bytes of sendmail output kept in last_try_msg. **/
#define SMTP_TRY_MSG_MAX	1024

/** The default log where Postfix records the results of sending emails. **/
#define SMTP_DEFAULT_LOG_PATH	"/var/log/maillog"

/** Seconds an email may stay Pending before it becomes Error (6 days). **/
#define SMTP_PENDING_TIMEOUT	(6 * 24 * 60 * 60)

/** Seconds to keep results read from the mail log (pending timeout + 1 day). **/
#define SMTP_LOG_KEEP_TIME	(SMTP_PENDING_TIMEOUT + 24 * 60 * 60)

/** Bytes for a Postfix queue ID, including the null terminator. **/
#define SMTP_QUEUE_ID_SIZE	32

/** Recipient results read from the mail log. **/
#define SMTP_RCPT_SENT		0
#define SMTP_RCPT_DEFERRED	1
#define SMTP_RCPT_BOUNCED	2

/*** Structure to store attribute information. ***/
typedef struct
    {
    Magic_t	Magic;
    char*	Name;
    int		Type; /* DATA_T_xxx */
    ObjData	Value;
    }
    SmtpAttribute, *pSmtpAttribute;

#define SMTP_ATTR(x) ((pSmtpAttribute)(x))


/*** Structure used by this driver internally. ***/
typedef struct
    {
    Magic_t		Magic;
    char*		Name;
    int			Type;
    pObject		Obj;
    int			Mask;
    pSnNode		Node;
    pXArray		AttributeNames; /* XArray of char*. */
    pXHashTable		Attributes; /* Hash of attribute name to SmtpAttribute. */
    int			CurAttr;

    /** Root node specific attributes. **/

    /** Email node specific attributes. **/
    pXHashTable		RootAttributes; /* Hash of root attribute name to SmtpAttribute. */
    pFile		ContentFile;
    XString		EmailPath;
    XString		EmailStructPath;
    XString		ResultPath;
    }
    SmtpData, *pSmtpData;

#define SMTP(x) ((pSmtpData)(x))


/*** Structure used by queries in this driver. ***/
typedef struct
    {
    Magic_t		Magic;
    pSmtpData	Data;
    DIR*	Directory;
    }
    SmtpQueryData, *pSmtpQueryData;

#define SMTP_QY(x) ((pSmtpQueryData)(x))


/*** Structure to track sweeps of a spool directory. ***/
typedef struct
    {
    Magic_t	Magic;
    char*	Path;
    time_t	LastSweep;
    }
    SmtpSpool, *pSmtpSpool;


/*** Structure to store the mail log result for one recipient. ***/
typedef struct
    {
    Magic_t	Magic;
    char*	Address;
    int		Status; /* SMTP_RCPT_xxx */
    char*	Reply;
    }
    SmtpLogRcpt, *pSmtpLogRcpt;


/*** Structure to store what the mail log says about one queued email. ***/
typedef struct _SLM
    {
    Magic_t		Magic;
    char		QueueID[SMTP_QUEUE_ID_SIZE];
    char*		MessageID;
    int			RcptCount; /* From nrcpt, or 0 until it is logged. */
    XArray		Rcpts; /* XArray of pSmtpLogRcpt. */
    bool		Expired; /* Postfix gave up and returned the email. */
    time_t		ReadTime;
    struct _SLM*	Next; /* The next newer record. */
    }
    SmtpLogMsg, *pSmtpLogMsg;


/*** Global data structure for the SMTP module. ***/
struct
    {
    XArray		DefaultRootAttributes;		/* XArray of pSmtpAttribute */
    XArray		DefaultEmailAttributes;		/* XArray of pSmtpAttribute */
    XHashTable		Spools;				/* Hash of spool_dir to pSmtpSpool */
    char		LogPath[PATH_MAX];		/* Path of the mail log */
    FILE*		Log;				/* The mail log, or NULL if not open */
    dev_t		LogDev;				/* Device of the open mail log */
    ino_t		LogIno;				/* Inode of the open mail log */
    XHashTable		LogByQueueID;			/* Hash of Postfix queue ID to pSmtpLogMsg */
    XHashTable		LogByMessageID;			/* Hash of Message-ID to newest pSmtpLogMsg */
    pSmtpLogMsg		LogOldest;			/* Oldest record, pruned first */
    pSmtpLogMsg		LogNewest;			/* Newest record, appended to */
    }
    SMTP_INF;


/** Forward declarations for functions that need them. **/
int smtp_internal_Close(pSmtpData inf);
int smtpQueryClose(void* qy_v, pObjTrxTree* oxt);
int smtp_internal_AddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt);
int smtp_internal_SetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt);


/*** smtp_internal_SpawnSendmail - launch the sendmail process to actually
 *** send off an email message.  This also works with Postfix, via its
 *** "sendmail compatibility interface".
 ***
 *** This function also spawns a detached supervisor process that waits for
 *** sendmail and writes the result to resultPath; starting with a status line
 *** ("exit N", "signal N", "timeout", or "error") padded with spaces to
 *** SMTP_RESULT_HEADER_LEN bytes, then the output of sendmail.
 ***/
int
smtp_internal_SpawnSendmail(char* emailPath, char* resultPath, pSmtpAttribute envFrom, pSmtpAttribute envTo)
    {
    int pid, fd, maxfiles;
    pXArray argv = NULL;
    char *envp[] = {NULL};
    int wstatus, wait_rval;
    int resultFd = -1;
    char tmpPath[PATH_MAX];
    char result[SMTP_RESULT_HEADER_LEN];
    char header[SMTP_RESULT_HEADER_LEN + 1];
    struct timespec pollInterval = {0, 100 * 1000 * 1000};
    int polls;
    bool tmpCreated = false;
    int rval = -1;

	/** Magic. **/
	ASSERTMAGIC(envFrom, MGK_SMTP_ATTRIBUTE);
	ASSERTMAGIC(envTo, MGK_SMTP_ATTRIBUTE);

	/** Build the sendmail argument list **/
	XArray argv_buf;
	if (UNLIKELY(xaInit(&argv_buf, 11) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the sendmail argument list.");
	    goto end;
	    }
	argv = &argv_buf;
	if (UNLIKELY(xaAddItem(argv, "/usr/sbin/sendmail") < 0		/* also compatible with Postfix */
	    || xaAddItem(argv, "-t") < 0				/* extract recipients from headers */
#if SMTP_DEBUG
	    || xaAddItem(argv, "-N") < 0				/* delivery status notifications (debug) */
	    || xaAddItem(argv, "delay, failure, success") < 0		/* ... for delay/fail/success (debug) */
	    || xaAddItem(argv, "-v") < 0				/* verbose (debug) */
#endif
	    || xaAddItem(argv, "-bm") < 0				/* mail sending from STDIN */
	    || xaAddItem(argv, "-i") < 0				/* don't end on . on a line by itself */
	))   {
	    mssError(1, "SMTP", "Failed to add options to the sendmail argument list.");
	    goto end;
	    }

	/** Add envelope To and From **/
	if (envFrom && envFrom->Value.String[0] != '\0')
	    {
	    if (UNLIKELY(xaAddItem(argv, "-f") < 0
		|| xaAddItem(argv, envFrom->Value.String) < 0
	    ))   {
		mssError(1, "SMTP",
		    "Failed to add envelope from '%s' to the sendmail argument list.",
		    envFrom->Value.String
		);
		goto end;
		}
	    }
	if (envTo && envTo->Value.String[0] != '\0')
	    {
	    if (UNLIKELY(xaAddItem(argv, "--") < 0
		|| xaAddItem(argv, envTo->Value.String) < 0
	    ))   {
		mssError(1, "SMTP",
		    "Failed to add envelope to '%s' to the sendmail argument list.",
		    envTo->Value.String
		);
		goto end;
		}
	    }

	if (UNLIKELY(xaAddItem(argv, NULL) < 0))
	    {
	    mssError(1, "SMTP", "Failed to terminate the sendmail argument list.");
	    goto end;
	    }

	/** Remove the result of any earlier send. **/
	if (UNLIKELY(unlink(resultPath) != 0 && errno != ENOENT))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to remove old sendmail result file (%s).", resultPath);
	    goto end;
	    }

	/** Create the result file, which the supervisor renames when done. **/
	if (UNLIKELY(snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", resultPath) >= (int)sizeof(tmpPath)))
	    {
	    mssError(1, "SMTP", "Sendmail result file path is too long: \"%s.tmp\".", resultPath);
	    goto end;
	    }
	resultFd = open(tmpPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (UNLIKELY(resultFd < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to create sendmail result file (%s).", tmpPath);
	    goto end;
	    }
	tmpCreated = true;

	/** Reserve the status line. **/
	snprintf(header, sizeof(header), "%-*s\n", SMTP_RESULT_HEADER_LEN - 1, "");
	if (UNLIKELY(write(resultFd, header, SMTP_RESULT_HEADER_LEN) != SMTP_RESULT_HEADER_LEN))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to write to sendmail result file (%s).", tmpPath);
	    goto end;
	    }

	/*** Create a child process to launch sendmail.  This lets us detach
	 *** from it so that we don't block all of centrallix while we wait
	 *** for an email to send.
	 ***
	 *** Note: Children don't have our error session so failures should
	 *** not call mssError().  Thus, we use fprintf(stderr) instead.
	 ***/
	pid = fork();
	if (UNLIKELY(pid < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Unable to fork (1).");
	    goto end;
	    }
	if (pid == 0)
	    {
	    /** we're in the child process -- disable MTask context switches to be safe **/
	    thLock();

	    /** close all open fds (except for 0-2 -- std{in,out,err}) **/
	    maxfiles = sysconf(_SC_OPEN_MAX);
	    if (maxfiles <= 0)
		{
		fprintf(stderr,
		    "Warning: sysconf(_SC_OPEN_MAX) returned %d; using maxfiles=2048.\n",
		    maxfiles
		);
		maxfiles = 2048;
		}

	    for(fd=3;fd<maxfiles;fd++) if (fd != resultFd) close(fd);

	    /** Open the email. **/
	    fd = open(emailPath, O_RDONLY);
	    if (UNLIKELY(fd < 0))
		{
		fprintf(stderr,
		    "SMTP: Failed to open email file (%s) for sendmail. (%s)\n",
		    emailPath, strerror(errno)
		);
		_exit(EXIT_FAILURE);
		}

	    /** Hopefully this makes our file stdin so we don't have to cat it into sendmail. **/
	    if (UNLIKELY(dup2(fd, 0) < 0))
		{
		fprintf(stderr,
		    "SMTP: Failed to redirect email file (%s) to stdin for sendmail. (%s)\n",
		    emailPath, strerror(errno)
		);
		_exit(EXIT_FAILURE);
		}

	    /** NOTE: We're currently double forking to get rid of zombie processes. **/
	    pid = fork();
	    if (UNLIKELY(pid < 0))
		{
		fprintf(stderr, "SMTP: Unable to fork (2). (%s)\n", strerror(errno));
		_exit(EXIT_FAILURE);
		}
	    if (pid == 0)
		{
		/** we're in the supervisor process -- disable MTask context switches to be safe **/
		thLock();

		/** close all open fds (except for 0-2 -- std{in,out,err}) **/
		maxfiles = sysconf(_SC_OPEN_MAX);
		if (maxfiles <= 0)
		    {
		    fprintf(stderr,
			"Warning: sysconf(_SC_OPEN_MAX) returned %d; using maxfiles=2048.\n",
			maxfiles
		    );
		    maxfiles = 2048;
		    }

		for(fd=3;fd<maxfiles;fd++) if (fd != resultFd) close(fd);

		/** Start sendmail, with its output in the result file. **/
		pid = fork();
		if (pid == 0)
		    {
		    if (UNLIKELY(dup2(resultFd, 1) < 0 || dup2(resultFd, 2) < 0))
			{
			dprintf(resultFd, "SMTP: Failed to redirect sendmail output. (%s)\n", strerror(errno));
			_exit(EXIT_FAILURE);
			}
		    close(resultFd);

		    /** Execve. **/
		    execve("/usr/sbin/sendmail", (char**)(argv->Items), envp);

		    /** if execve() is successful, this is never reached **/
		    fprintf(stderr,
			"SMTP: execve(\"/usr/sbin/sendmail\") failed: \"%s\"\n",
			strerror(errno)
		    );
		    _exit(EXIT_FAILURE);
		    }

		/** Supervise sendmail. **/
		if (UNLIKELY(pid < 0))
		    {
		    dprintf(resultFd, "SMTP: Unable to fork (3). (%s)\n", strerror(errno));
		    snprintf(result, sizeof(result), "error");
		    }
		else
		    {
		    /** Wait for sendmail, killing it after the timeout. **/
		    polls = 0;
		    while ((wait_rval = waitpid(pid, &wstatus, WNOHANG)) == 0 && polls < SMTP_SENDMAIL_TIMEOUT * 10)
			{
			nanosleep(&pollInterval, NULL);
			polls++;
			}
		    if (wait_rval == 0)
			{
			kill(pid, SIGKILL);
			waitpid(pid, &wstatus, 0);
			snprintf(result, sizeof(result), "timeout");
			}
		    else if (UNLIKELY(wait_rval < 0))
			{
			dprintf(resultFd, "SMTP: Failed to wait for sendmail (pid %d). (%s)\n", pid, strerror(errno));
			snprintf(result, sizeof(result), "error");
			}
		    else if (WIFSIGNALED(wstatus))
			{
			snprintf(result, sizeof(result), "signal %d", WTERMSIG(wstatus));
			}
		    else
			{
			snprintf(result, sizeof(result), "exit %d", WEXITSTATUS(wstatus));
			}
		    }

		/** Write the status line, then publish the result. **/
		snprintf(header, sizeof(header), "%-*s\n", SMTP_RESULT_HEADER_LEN - 1, result);
		if (UNLIKELY(pwrite(resultFd, header, SMTP_RESULT_HEADER_LEN, 0) != SMTP_RESULT_HEADER_LEN
		    || close(resultFd) != 0
		    || rename(tmpPath, resultPath) != 0 /* Publish. */
		))  {
		    fprintf(stderr,
			"SMTP: Failed to write sendmail result file (%s). (%s)\n",
			resultPath, strerror(errno)
		    );
		    _exit(EXIT_FAILURE);
		    }
		_exit(EXIT_SUCCESS);
		}
	    else
		{
		/** We're the parent. Exit so centrallix can move on. **/
		_exit(EXIT_SUCCESS);
		}
	    }

	/** Reap the launcher, yielding to other threads while it runs. **/
	while ((wait_rval = waitpid(pid, &wstatus, WNOHANG)) == 0)
	    thSleep(10);
	if (UNLIKELY(wait_rval < 0 && errno == ECHILD))
	    {
	    fprintf(stderr,
		"Warning: Sendmail launcher process (pid %d) was reaped elsewhere; "
		"the sendmail result file reports the send.\n",
		pid
	    );
	    wstatus = 0; /* Treat as a successful exit. */
	    }
	else if (UNLIKELY(wait_rval < 0))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to wait for child sendmail launcher process (pid %d).",
		pid
	    );
	    goto end;
	    }
	if (UNLIKELY(WIFSIGNALED(wstatus)))
	    {
	    mssError(1, "SMTP",
		"Sendmail launcher process (pid %d) was killed by signal %d.",
		pid, WTERMSIG(wstatus)
	    );
	    goto end;
	    }
	if (UNLIKELY(!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != EXIT_SUCCESS))
	    {
	    mssError(1, "SMTP",
		"Sendmail launcher process (pid %d) exited with status %d.",
		pid, WEXITSTATUS(wstatus)
	    );
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to spawn sendmail for email file (%s).", emailPath);

	if (resultFd >= 0) close(resultFd);
	if (UNLIKELY(rval != 0 && tmpCreated && unlink(tmpPath) != 0))
	    fprintf(stderr,
		"Warning: Failed to remove partial sendmail result file (%s): %s.\n",
		tmpPath, strerror(errno)
	    );

	if (LIKELY(argv != NULL)) xaDeInit(argv);

	return rval;
    }


/*** smtp_internal_ClearAttribute - Clears all the elements of the attributes
 *** hash table.
 ***/
int
smtp_internal_ClearAttribute(char* inf_c, void* customParams)
    {
    pSmtpAttribute attr = SMTP_ATTR(inf_c);

	/** Edge cases. **/
	if (UNLIKELY(attr == NULL))
	    {
	    mssError(1, "SMTP", "Failed to clear NULL attribute.");
	    return -1;
	    }
	ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);

	if (attr->Name)
	    nmSysFree(attr->Name);

	if (attr->Type == DATA_T_STRING && attr->Value.String)
	    {
	    nmSysFree(attr->Value.String);
	    }
	else if (attr->Type == DATA_T_DATETIME && attr->Value.DateTime)
	    {
	    nmFree(attr->Value.DateTime, sizeof(DateTime));
	    }

	nmFree(attr, sizeof(SmtpAttribute));

    return 0;
    }


/*** smtp_internal_NewAttributes - Allocates an empty attributes hash table.
 *** Returns the table on success and NULL on failure.
 ***/
pXHashTable
smtp_internal_NewAttributes()
    {
    pXHashTable attributes;

	attributes = (pXHashTable)nmMalloc(sizeof(XHashTable));
	if (UNLIKELY(attributes == NULL))
	    {
	    mssError(1, "SMTP", "Failed to create attributes hash table.");
	    return NULL;
	    }
	memset(attributes, 0, sizeof(XHashTable));
	if (UNLIKELY(xhInit(attributes, 17, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize attributes hash table.");
	    nmFree(attributes, sizeof(XHashTable));
	    return NULL;
	    }

    return attributes;
    }


/*** smtp_internal_FreeAttributes - Frees an attributes hash table and the
 *** attributes in it.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_FreeAttributes(pXHashTable attributes)
    {
    int rval = 0;

	if (UNLIKELY(xhClear(attributes, smtp_internal_ClearAttribute, NULL) != 0
	    || xhDeInit(attributes) != 0
	))   {
	    mssError(1, "SMTP", "Failed to free attributes.");
	    rval = -1;
	    }
	nmFree(attributes, sizeof(XHashTable));

    return rval;
    }


/*** smtp_internal_CreateAttribute - Creates an attribute with the given values.
 *** Note that this function only works for integer and string attribute types.
 ***/
pSmtpAttribute
smtp_internal_CreateAttribute(char* name, int type, int intVal, char* strVal)
    {
    pSmtpAttribute inf = NULL;

	/** Allocate the new SmtpAttribute. **/
	inf = nmMalloc(sizeof(SmtpAttribute));
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP",
		"Failed to allocate %zu bytes for an attribute.",
		sizeof(SmtpAttribute)
	    );
	    goto error;
	    }
	memset(inf, 0, sizeof(SmtpAttribute));
	SETMAGIC(inf, MGK_SMTP_ATTRIBUTE);

	/** Set attribute name. **/
	inf->Name = nmSysStrdup(name);
	if (UNLIKELY(inf->Name == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy attribute name.");
	    goto error;
	    }
	inf->Type = type;

	/** Set attribute value. **/
	if (type == DATA_T_INTEGER)
	    {
	    inf->Value.Integer = intVal;
	    }
	else if (type == DATA_T_STRING && strVal)
	    {
	    inf->Value.String = nmSysStrdup(strVal);
	    if (UNLIKELY(inf->Value.String == NULL))
		{
		mssError(1, "SMTP", "Failed to copy attribute value \"%s\".", strVal);
		goto error;
		}
	    }
	else
	    {
	    mssError(1, "SMTP",
		"Unsupported attribute type %s or missing string value.",
		objTypeToStr(type)
	    );
	    goto error;
	    }

	return inf;

    error:
	mssError(0, "SMTP", "Failed to create attribute '%s'.", name);

	if (inf != NULL) smtp_internal_ClearAttribute((char*)inf, NULL);

	return NULL;
    }


/*** smtp_internal_AddDefault - Creates an attribute and adds it to a list of
 *** default attributes.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_AddDefault(pXArray defaults, char* name, int type, int intVal, char* strVal)
    {
    pSmtpAttribute attr = NULL;

	/** Create the attribute. **/
	attr = smtp_internal_CreateAttribute(name, type, intVal, strVal);
	if (UNLIKELY(attr == NULL))
	    return -1;

	/** Add it to the list. **/
	if (UNLIKELY(xaAddItem(defaults, attr) < 0))
	    {
	    mssError(1, "SMTP", "Failed to add default attribute '%s'.", name);
	    smtp_internal_ClearAttribute((char*)attr, NULL);
	    return -1;
	    }

	return 0;
    }


/*** smtp_internal_FreeLogMsg - free a record read from the mail log.
 ***/
void
smtp_internal_FreeLogMsg(pSmtpLogMsg msg)
    {
    pSmtpLogRcpt rcpt;
    int i;

	ASSERTMAGIC(msg, MGK_SMTP_LOG_MSG);
	for (i = 0; i < msg->Rcpts.nItems; i++)
	    {
	    rcpt = (pSmtpLogRcpt)msg->Rcpts.Items[i];
	    ASSERTMAGIC(rcpt, MGK_SMTP_LOG_RCPT);
	    if (rcpt->Address != NULL) nmSysFree(rcpt->Address);
	    if (rcpt->Reply != NULL) nmSysFree(rcpt->Reply);
	    nmFree(rcpt, sizeof(SmtpLogRcpt));
	    }
	xaDeInit(&msg->Rcpts);
	if (msg->MessageID != NULL) nmSysFree(msg->MessageID);
	nmFree(msg, sizeof(SmtpLogMsg));

    return;
    }


/*** smtp_internal_AddLogMsg - start a record for an email Postfix queued,
 *** replacing older records with the same queue ID or Message-ID.
 ***
 *** @param queueId The Postfix queue ID.
 *** @param messageId The Message-ID, without angle brackets.
 *** @param now The current time.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_AddLogMsg(char* queueId, char* messageId, time_t now)
    {
    pSmtpLogMsg msg = NULL;
    bool inQueueTable = false;
    int rval = -1;

	/** Create the record. **/
	msg = nmMalloc(sizeof(SmtpLogMsg));
	if (UNLIKELY(msg == NULL))
	    {
	    mssError(1, "SMTP", "nmMalloc(%zu) failed.", sizeof(SmtpLogMsg));
	    goto end;
	    }
	memset(msg, 0, sizeof(SmtpLogMsg));
	SETMAGIC(msg, MGK_SMTP_LOG_MSG);
	if (UNLIKELY(xaInit(&msg->Rcpts, 4) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the recipients of a mail log record.");
	    goto end;
	    }
	strtcpy(msg->QueueID, queueId, sizeof(msg->QueueID));
	msg->MessageID = nmSysStrdup(messageId);
	if (UNLIKELY(msg->MessageID == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy Message-ID <%s>.", messageId);
	    goto end;
	    }
	msg->ReadTime = now;

	/** Index it, replacing older records. **/
	xhRemove(&SMTP_INF.LogByQueueID, msg->QueueID);
	if (UNLIKELY(xhAdd(&SMTP_INF.LogByQueueID, msg->QueueID, (char*)msg) != 0))
	    {
	    mssError(1, "SMTP", "Failed to index mail log record for queue ID %s.", msg->QueueID);
	    goto end;
	    }
	inQueueTable = true;
	xhRemove(&SMTP_INF.LogByMessageID, msg->MessageID);
	if (UNLIKELY(xhAdd(&SMTP_INF.LogByMessageID, msg->MessageID, (char*)msg) != 0))
	    {
	    mssError(1, "SMTP", "Failed to index mail log record for Message-ID <%s>.", msg->MessageID);
	    goto end;
	    }

	/** Append it to the list. **/
	if (SMTP_INF.LogNewest != NULL)
	    SMTP_INF.LogNewest->Next = msg;
	else
	    SMTP_INF.LogOldest = msg;
	SMTP_INF.LogNewest = msg;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0 && msg != NULL))
	    {
	    if (inQueueTable) xhRemove(&SMTP_INF.LogByQueueID, msg->QueueID);
	    smtp_internal_FreeLogMsg(msg);
	    }

	return rval;
    }


/*** smtp_internal_SetLogRcpt - record the latest result for one recipient
 *** of a queued email.
 ***
 *** @param msg The record of the email.
 *** @param address The recipient address.
 *** @param status The result (SMTP_RCPT_xxx).
 *** @param reply The reply or reason Postfix logged.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_SetLogRcpt(pSmtpLogMsg msg, char* address, int status, char* reply)
    {
    pSmtpLogRcpt rcpt = NULL;
    pSmtpLogRcpt newRcpt = NULL;
    char* newReply = NULL;
    int i;
    int rval = -1;

	/** Copy the reply. **/
	newReply = nmSysStrdup(reply);
	if (UNLIKELY(newReply == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy reply \"%s\".", reply);
	    goto end;
	    }

	/** Find the recipient. **/
	for (i = 0; i < msg->Rcpts.nItems; i++)
	    {
	    rcpt = (pSmtpLogRcpt)msg->Rcpts.Items[i];
	    ASSERTMAGIC(rcpt, MGK_SMTP_LOG_RCPT);
	    if (strcmp(rcpt->Address, address) == 0)
		break;
	    rcpt = NULL;
	    }

	/** Recipient not found: add it. **/
	if (rcpt == NULL)
	    {
	    newRcpt = nmMalloc(sizeof(SmtpLogRcpt));
	    if (UNLIKELY(newRcpt == NULL))
		{
		mssError(1, "SMTP", "Failed to allocate %zu bytes for a mail log recipient.", sizeof(SmtpLogRcpt));
		goto end;
		}
	    memset(newRcpt, 0, sizeof(SmtpLogRcpt));
	    SETMAGIC(newRcpt, MGK_SMTP_LOG_RCPT);
	    newRcpt->Address = nmSysStrdup(address);
	    if (UNLIKELY(newRcpt->Address == NULL))
		{
		mssError(1, "SMTP", "Failed to copy recipient address <%s>.", address);
		goto end;
		}
	    if (UNLIKELY(xaAddItem(&msg->Rcpts, newRcpt) < 0))
		{
		mssError(1, "SMTP", "Failed to add recipient to the mail log record for queue ID %s.", msg->QueueID);
		goto end;
		}
	    rcpt = newRcpt;
	    newRcpt = NULL;
	    }

	/** Record the result. **/
	if (rcpt->Reply != NULL) nmSysFree(rcpt->Reply);
	rcpt->Reply = newReply;
	newReply = NULL;
	rcpt->Status = status;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(newReply != NULL)) nmSysFree(newReply);
	if (UNLIKELY(newRcpt != NULL))
	    {
	    if (newRcpt->Address != NULL) nmSysFree(newRcpt->Address);
	    nmFree(newRcpt, sizeof(SmtpLogRcpt));
	    }

	return rval;
    }


/*** smtp_internal_ParseLogLine - record the result in one line of the mail
 *** log, if it has one.  Lines look like this:
 ***   <date> <host> postfix/<program>[<pid>]: <queue ID>: <message>
 *** where the messages used are:
 ***   message-id=<id>                          (a new queued email)
 ***   from=<addr>, size=N, nrcpt=N ...         (its recipient count)
 ***   from=<addr>, status=expired, ...         (Postfix gave up)
 ***   from=<addr>, status=force-expired, ...   (an admin made it give up)
 ***   to=<addr>, ..., status=<status> (reply)  (a recipient result)
 ***
 *** @param line The line, without a newline.  Modified to end the values.
 *** @param now The current time.
 *** @returns 0 on success (including lines with no result), or -1 on failure.
 ***/
int
smtp_internal_ParseLogLine(char* line, time_t now)
    {
    static const char queueIdChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char* program;
    char* message;
    char* queueId;
    int queueIdLen;
    pSmtpLogMsg msg;
    char* value;
    int valueLen;
    char* address;
    char* reply;
    int replyLen;
    int status;

	/** Find the message, and skip lines not logged by Postfix. **/
	message = strstr(line, "]: ");
	if (message == NULL)
	    return 0;
	program = message;
	while (program > line && *program != '[') program--;
	while (program > line && program[-1] != ' ') program--;
	if (strncmp(program, "postfix", 7) != 0)
	    return 0;
	message += 3; /* Consume the "]: ". */

	/** Get the queue ID. **/
	queueIdLen = strspn(message, queueIdChars);
	if (queueIdLen == 0 || queueIdLen >= SMTP_QUEUE_ID_SIZE || strncmp(message + queueIdLen, ": ", 2) != 0)
	    return 0;
	queueId = message;
	queueId[queueIdLen] = '\0';
	message += queueIdLen + 2;

	/** Detect a new queued email. **/
	if (strncmp(message, "message-id=", 11) == 0)
	    {
	    value = message + 11;
	    valueLen = strlen(value);
	    if (valueLen >= 2 && value[0] == '<' && value[valueLen - 1] == '>')
		{
		value[valueLen - 1] = '\0';
		value++;
		}
	    if (*value == '\0')
		return 0;
	    return smtp_internal_AddLogMsg(queueId, value, now);
	    }

	/** Other lines update a queued email. **/
	msg = (pSmtpLogMsg)xhLookup(&SMTP_INF.LogByQueueID, queueId);
	if (msg == NULL)
	    return 0;
	ASSERTMAGIC(msg, MGK_SMTP_LOG_MSG);

	/** Detect a recipient count, or Postfix giving up. **/
	if (strncmp(message, "from=<", 6) == 0)
	    {
	    /** Detect recipient count. **/
	    if ((value = strstr(message, ", nrcpt=")) != NULL)
		msg->RcptCount = atoi(value + 8);

	    /** Detect Postfix giving up. **/
	    else if (strstr(message, ", status=expired,") != NULL || strstr(message, ", status=force-expired,") != NULL)
		msg->Expired = true;

	    return 0;
	    }

	/** Detect a recipient result. **/
	if (strncmp(message, "to=<", 4) == 0)
	    {
	    address = message + 4;
	    value = strchr(address, '>');
	    if (value == NULL)
		return 0;
	    *value = '\0';
	    value = strstr(value + 1, ", status="); /* Search after the address. */
	    if (value == NULL)
		return 0;
	    value += 9;
	    valueLen = strcspn(value, " ");
	    if (valueLen == 4 && strncmp(value, "sent", 4) == 0)
		status = SMTP_RCPT_SENT;
	    else if (valueLen == 8 && strncmp(value, "deferred", 8) == 0)
		status = SMTP_RCPT_DEFERRED;
	    else if (valueLen == 7 && strncmp(value, "bounced", 7) == 0)
		status = SMTP_RCPT_BOUNCED;
	    else
		return 0;

	    /** Get the reply, without its parentheses. **/
	    reply = value + valueLen;
	    if (strncmp(reply, " (", 2) == 0)
		reply += 2;
	    replyLen = strlen(reply);
	    if (replyLen > 0 && reply[replyLen - 1] == ')')
		reply[replyLen - 1] = '\0';

	    return smtp_internal_SetLogRcpt(msg, address, status, reply);
	    }

    return 0;
    }


/*** smtp_internal_OpenLog - open the mail log as root, since only root can
 *** read it.
 ***
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_OpenLog(void)
    {
    uid_t uid = geteuid();
    struct stat st;
    int fd = -1;
    int openErrno;
    int rval = -1;

	/** Open the log as root. **/
	if (UNLIKELY(uid != 0 && seteuid(0) != 0))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to become root to open the mail log \"%s\". (Centrallix is not running as root.)",
		SMTP_INF.LogPath
	    );
	    goto end;
	    }
	fd = open(SMTP_INF.LogPath, O_RDONLY);
	openErrno = errno;
	if (UNLIKELY(uid != 0 && seteuid(uid) != 0))
	    {
	    fprintf(stderr,
		"SMTP: Failed to switch back to uid %d after opening the mail log (%s); aborting.\n",
		(int)uid, strerror(errno)
	    );
	    abort(); /* Never keep running as root. */
	    }
	if (UNLIKELY(fd < 0))
	    {
	    errno = openErrno;
	    mssErrorErrno(1, "SMTP", "Failed to open the mail log \"%s\".", SMTP_INF.LogPath);
	    goto end;
	    }

	/** Remember this log file, to detect rotation. **/
	if (UNLIKELY(fstat(fd, &st) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to check the mail log \"%s\".", SMTP_INF.LogPath);
	    goto end;
	    }
	SMTP_INF.Log = fdopen(fd, "r");
	if (UNLIKELY(SMTP_INF.Log == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to read the mail log \"%s\".", SMTP_INF.LogPath);
	    goto end;
	    }
	fd = -1; /* Closed with SMTP_INF.Log. */
	SMTP_INF.LogDev = st.st_dev;
	SMTP_INF.LogIno = st.st_ino;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(fd >= 0)) close(fd);

	return rval;
    }


/*** smtp_internal_ReadLogLines - parse the complete lines added to the open
 *** mail log since the last read.
 ***
 *** @param now The current time.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_ReadLogLines(time_t now)
    {
    char* line = NULL;
    size_t lineSize = 0;
    ssize_t len;
    int rval = -1;

	while ((len = getline(&line, &lineSize, SMTP_INF.Log)) > 0)
	    {
	    /** Leave a partial line until the rest is written. **/
	    if (line[len - 1] != '\n')
		{
		if (UNLIKELY(fseeko(SMTP_INF.Log, -len, SEEK_CUR) != 0))
		    {
		    mssErrorErrno(1, "SMTP", "Failed to rewind a partial line in the mail log \"%s\".", SMTP_INF.LogPath);
		    goto end;
		    }
		break;
		}
	    line[len - 1] = '\0';
	    if (UNLIKELY(smtp_internal_ParseLogLine(line, now) != 0))
		goto end;
	    }
	if (UNLIKELY(ferror(SMTP_INF.Log)))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to read the mail log \"%s\".", SMTP_INF.LogPath);
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	clearerr(SMTP_INF.Log); /* Allow reading lines written later. */
	if (LIKELY(line != NULL)) free(line);

	return rval;
    }


/*** smtp_internal_ReadLog - read the lines added to the mail log since the
 *** last read, reopening it if it was rotated or is not open, and forget
 *** results too old for any Pending email to need.
 ***
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_ReadLog(void)
    {
    time_t now = time(NULL);
    pSmtpLogMsg msg;
    struct stat st;

	/** Drop old results. **/
	while (SMTP_INF.LogOldest != NULL && now - SMTP_INF.LogOldest->ReadTime > SMTP_LOG_KEEP_TIME)
	    {
	    msg = SMTP_INF.LogOldest;
	    SMTP_INF.LogOldest = msg->Next;
	    if (SMTP_INF.LogOldest == NULL)
		SMTP_INF.LogNewest = NULL;
	    if ((pSmtpLogMsg)xhLookup(&SMTP_INF.LogByQueueID, msg->QueueID) == msg)
		xhRemove(&SMTP_INF.LogByQueueID, msg->QueueID);
	    if ((pSmtpLogMsg)xhLookup(&SMTP_INF.LogByMessageID, msg->MessageID) == msg)
		xhRemove(&SMTP_INF.LogByMessageID, msg->MessageID);
	    smtp_internal_FreeLogMsg(msg);
	    }

	/** Read the rest of the open log, then close it if it was rotated. **/
	if (SMTP_INF.Log != NULL)
	    {
	    if (UNLIKELY(smtp_internal_ReadLogLines(now) != 0))
		return -1;
	    if (stat(SMTP_INF.LogPath, &st) != 0)
		{
		if (errno == ENOENT)
		    return 0; /* Mid-rotation, so read the new log next time. */
		mssErrorErrno(1, "SMTP", "Failed to check the mail log \"%s\".", SMTP_INF.LogPath);
		return -1;
		}
	    if (st.st_dev == SMTP_INF.LogDev && st.st_ino == SMTP_INF.LogIno)
		return 0;
	    fclose(SMTP_INF.Log);
	    SMTP_INF.Log = NULL;
	    }

	/** Open the current log and read it. **/
	if (UNLIKELY(smtp_internal_OpenLog() != 0))
	    return -1;
	if (UNLIKELY(smtp_internal_ReadLogLines(now) != 0))
	    return -1;

    return 0;
    }


/*** smtp_internal_LookupLog - get what the mail log says about an email,
 *** after reading any new lines.
 ***
 *** @param messageId The Message-ID of the email, without angle brackets.
 *** @param msg Set to the newest record for messageId, or NULL if none.
 *** @returns 0 on success, or -1 if the mail log could not be read.
 ***/
int
smtp_internal_LookupLog(char* messageId, pSmtpLogMsg* msg)
    {
	*msg = NULL;
	if (UNLIKELY(smtp_internal_ReadLog() != 0))
	    {
	    mssError(0, "SMTP", "Failed to check the mail log for Message-ID <%s>.", messageId);
	    return -1;
	    }
	*msg = (pSmtpLogMsg)xhLookup(&SMTP_INF.LogByMessageID, messageId);
	ASSERTMAGIC(*msg, MGK_SMTP_LOG_MSG);

    return 0;
    }


/*** smtp_internal_InitGlobals - Initializes global information for the SMTP
 *** driver.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_InitGlobals()
    {
    char local_host_name[HOST_NAME_MAX];
    char* logPath;

	/** Initialize the global attributes. **/
	if (UNLIKELY(xaInit(&SMTP_INF.DefaultRootAttributes, 16) != 0
	    || xaInit(&SMTP_INF.DefaultEmailAttributes, 16) != 0
	))   {
	    mssError(1, "SMTP", "Failed to initialize default attribute lists.");
	    goto error;
	    }
	if (UNLIKELY(xhInit(&SMTP_INF.Spools, 17, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the spool directory table.");
	    goto error;
	    }
	if (UNLIKELY(xhInit(&SMTP_INF.LogByQueueID, 1021, 0) != 0
	    || xhInit(&SMTP_INF.LogByMessageID, 1021, 0) != 0
	))   {
	    mssError(1, "SMTP", "Failed to initialize the mail log tables.");
	    goto error;
	    }

	/** Add all the required attributes. Yay hardcoding! **/
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
	    {
	    strtcpy(local_host_name, "localhost.localdomain", sizeof(local_host_name));
	    fprintf(stderr,
		"Warning: gethostname() failed (%s); using \"%s\".\n",
		strerror(errno), local_host_name
	    );
	    }
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "local_host_name",	DATA_T_STRING,	0,	local_host_name) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "send_method",		DATA_T_STRING,	0,	"sendmail") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "server",		DATA_T_STRING,	0,	"127.0.0.1") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "port",			DATA_T_INTEGER,	25,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "spool_dir",		DATA_T_STRING,	0,	"/var/spool/mail/_centrallix") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "log_dir",		DATA_T_STRING,	0,	"/var/log") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "log_date_attr",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "log_msgid_attr",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "log_info_attr",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "ratelimit_time",	DATA_T_INTEGER,	1,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "domlimit_time",		DATA_T_INTEGER,	5,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "expire_time",		DATA_T_INTEGER,	SMTP_DEFAULT_EXPIRE_TIME,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "content_has_headers",	DATA_T_INTEGER,	1,	NULL) < 0)) goto error;

	/** Add all the required email attributes. Behold the hard code; standeth it against all but the hardest hammer. **/
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "envelope_from",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "envelope_to",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "tag",			DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_from",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_to",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_cc",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_bcc",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_reply_to",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_list_unsubscribe",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_list_unsubscribe_post",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_subject",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_user_agent",	DATA_T_STRING,	0,	"Centrallix/" PACKAGE_VERSION) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "header_mime_version",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "status",		DATA_T_STRING,	0,	"Draft") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "is_ready",		DATA_T_INTEGER,	0,	0) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "try_count",		DATA_T_INTEGER,	0,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "last_try_status",	DATA_T_STRING,	0,	"None") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "last_try_msg",		DATA_T_STRING,	0,	"") < 0)) goto error;

	/** Get the mail log path. **/
	if (stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "smtp"), "mail_log"), NULL, &logPath, 0) != 0)
	    logPath = SMTP_DEFAULT_LOG_PATH;
	if (UNLIKELY(strtcpy(SMTP_INF.LogPath, logPath, sizeof(SMTP_INF.LogPath)) < 0))
	    {
	    mssError(1, "SMTP", "Failed to set the mail log path: \"%s\" is too long.", logPath);
	    goto error;
	    }

	/** Read the results of emails already handed to Postfix. **/
	if (UNLIKELY(smtp_internal_ReadLog() != 0))
	    mssWarnError("Failed to read the mail log, so Pending emails cannot be checked.");

	return 0;

    error:
	mssError(0, "SMTP", "Failed to initialize SMTP driver globals.");
	return -1;
    }


/*** smtp_internal_GetString - get the value of a string attribute.
 ***
 *** @param attributes The attributes to search.
 *** @param name The attribute name.
 *** @returns The value, or NULL if the attribute is missing or not a string.
 ***/
char*
smtp_internal_GetString(pXHashTable attributes, char* name)
    {
    pSmtpAttribute attr = SMTP_ATTR(xhLookup(attributes, name));

	ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);
	if (attr == NULL || attr->Type != DATA_T_STRING)
	    return NULL;

    return attr->Value.String;
    }


/*** smtp_internal_IsEmail - Returns 1 if the filename is an email.
 ***/
bool
smtp_internal_IsEmail(char* filename)
    {
    int l = strlen(filename);
    return l >= 4 && (strcmp(filename + l - 4, ".msg") == 0 || strcmp(filename + l - 4, ".eml") == 0);
    }


/*** smtp_internal_IsReadOnly - Checks whether an email attribute is set
 *** only by the driver.
 *** @param attrname The attribute name.
 *** @returns true if the attribute is read-only, false otherwise.
 ***/
bool
smtp_internal_IsReadOnly(char* attrname)
    {
    static char* readOnly[] =
	{
	"status",
	"expire_date",
	"try_count",
	"first_try_date",
	"last_try_date",
	"last_try_status",
	"last_try_msg",
	};
    const int n_readOnly = sizeof(readOnly) / sizeof(readOnly[0]);
    int i;

	for (i = 0; i < n_readOnly; i++)
	    if (strcmp(attrname, readOnly[i]) == 0) return true;

    return false;
    }


/*** smtp_internal_IsExpired - Checks whether a sent or failed email has
 *** passed its expire_date.  Drafts never expire.
 *** @param structPath The path of the email's struct file.
 *** @param now The current date.
 *** @returns 1 if expired, 0 if not, or -1 if the struct is unreadable.
 ***/
int
smtp_internal_IsExpired(char* structPath, pDateTime now)
    {
    pFile structFile = NULL;
    pStructInf emailStruct = NULL;
    char* status = NULL;
    char* expireStr = NULL;
    DateTime expireDate;
    int rval = -1;

	/** Parse the struct file, which a new email may not have yet. **/
	structFile = fdOpen(structPath, O_RDONLY, 0);
	if (structFile == NULL)
	    {
	    if (errno != ENOENT)
		{
		mssErrorErrno(1, "SMTP", "Failed to open email struct file \"%s\".", structPath);
		goto end;
		}
	    rval = 0; /* No struct yet. */
	    goto end;
	    }
	emailStruct = stParseMsg(structFile, 0);
	if (UNLIKELY(emailStruct == NULL))
	    {
	    mssError(0, "SMTP", "Failed to parse email struct file \"%s\".", structPath);
	    goto end;
	    }

	/** Only sent and failed emails expire. **/
	if (stAttrValue(stLookup(emailStruct, "status"), NULL, &status, 0) != 0
	    || (strcmp(status, "Sent") != 0 && strcmp(status, "Error") != 0))
	    {
	    rval = 0;
	    goto end;
	    }

	/** An expire_date of 01 Jan 1900 means none. **/
	if (stAttrValue(stLookup(emailStruct, "expire_date"), NULL, &expireStr, 0) != 0)
	    {
	    rval = 0;
	    goto end;
	    }

	/** Get expire_date. **/
	memset(&expireDate, 0, sizeof(DateTime));
	if (UNLIKELY(objDataToDateTime(DATA_T_STRING, expireStr, &expireDate, NULL) != 0))
	    {
	    mssError(1, "SMTP", "Invalid expire_date \"%s\" in \"%s\".", expireStr, structPath);
	    goto end;
	    }

	/** Success. **/
	rval = (expireDate.Value != 0 && expireDate.Value <= now->Value) ? 1 : 0;

    end:
	if (LIKELY(structFile != NULL)) fdClose(structFile, 0);
	if (LIKELY(emailStruct != NULL)) stFreeInf(emailStruct);

	return rval;
    }


/*** smtp_internal_RemoveEmail - delete the files of an email: the email,
 *** its struct, and its sendmail result, including a partial one.  Missing
 *** files are not an error.
 ***
 *** @param emailPath The path of the email file.
 *** @param structPath The path of the email struct file.
 *** @param resultPath The path of the sendmail result file.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_RemoveEmail(char* emailPath, char* structPath, char* resultPath)
    {
    char tmpPath[PATH_MAX];
    int i;

	/** Build the partial result path. **/
	if (UNLIKELY(snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", resultPath) >= (int)sizeof(tmpPath)))
	    {
	    mssError(1, "SMTP", "Failed to build partial sendmail result path: \"%s.tmp\" is too long.", resultPath);
	    return -1;
	    }

	/** Delete the files, email first. **/
	char* paths[] =
	    {
	    emailPath,
	    structPath,
	    resultPath,
	    tmpPath,
	    };
	for (i = 0; i < (int)(sizeof(paths) / sizeof(paths[0])); i++)
	    {
	    if (UNLIKELY(remove(paths[i]) != 0 && errno != ENOENT))
		{
		mssErrorErrno(1, "SMTP", "Failed to delete email file (%s).", paths[i]);
		return -1;
		}
	    }

    return 0;
    }


/*** smtp_internal_SweepSpool - Deletes expired emails from a spool
 *** directory, at most once per SMTP_SWEEP_INTERVAL.  Callers continue
 *** without the sweep, so it resolves its own errors with a warning.
 *** @param spoolDir The spool directory to sweep.
 ***/
void
smtp_internal_SweepSpool(char* spoolDir)
    {
    pSmtpSpool spool = NULL;
    pSmtpSpool newSpool = NULL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;
    char emailPath[PATH_MAX];
    char structPath[PATH_MAX];
    char resultPath[PATH_MAX];
    DateTime now;
    time_t curTime = time(NULL);
    int nameLen;
    int expired;
    bool successful = false;

	/** Track each spool directory. **/
	spool = (pSmtpSpool)xhLookup(&SMTP_INF.Spools, spoolDir);
	ASSERTMAGIC(spool, MGK_SMTP_SPOOL);
	if (spool == NULL)
	    {
	    newSpool = nmMalloc(sizeof(SmtpSpool));
	    if (UNLIKELY(newSpool == NULL))
		{
		mssError(1, "SMTP",
		    "Failed to allocate sweep state for spool directory \"%s\".",
		    spoolDir
		);
		goto end;
		}
	    memset(newSpool, 0, sizeof(SmtpSpool));
	    SETMAGIC(newSpool, MGK_SMTP_SPOOL);
	    newSpool->Path = nmSysStrdup(spoolDir);
	    if (UNLIKELY(newSpool->Path == NULL))
		{
		mssError(1, "SMTP", "Failed to set spool directory path: \"%s\".", spoolDir);
		goto end;
		}
	    if (UNLIKELY(xhAdd(&SMTP_INF.Spools, newSpool->Path, (char*)newSpool) != 0))
		{
		mssError(1, "SMTP",
		    "Failed to add spool directory to hashtable: \"%s\".",
		    spoolDir
		);
		goto end;
		}
	    spool = newSpool;
	    newSpool = NULL;
	    }

	/** Throttle sweeps. **/
	if (curTime - spool->LastSweep < SMTP_SWEEP_INTERVAL)
	    {
	    /** No sweep needed, we're done. **/
	    successful = true;
	    goto end;
	    }
	spool->LastSweep = curTime;

	/** Open the spool directory. **/
	if (UNLIKELY(objCurrentDate(&now) != 0))
	    {
	    mssError(1, "SMTP", "Failed to get the current date to sweep \"%s\".", spoolDir);
	    goto end;
	    }
	dir = opendir(spoolDir);
	if (UNLIKELY(dir == NULL))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to open spool directory \"%s\" to sweep it.",
		spoolDir
	    );
	    goto end;
	    }

	/** Delete each expired email with its struct and result files. **/
	while (1)
	    {
	    /** Get the next file. **/
	    errno = 0;
	    entry = readdir(dir);
	    if (entry == NULL)
		{
		if (UNLIKELY(errno != 0))
		    {
		    mssErrorErrno(1, "SMTP", "Failed to read spool directory \"%s\".", spoolDir);
		    goto end;
		    }
		break; /* No more files. */
		}

	    /** Skip non-email files. **/
	    if (!smtp_internal_IsEmail(entry->d_name))
		continue;

	    /** Build the file paths. **/
	    nameLen = strlen(entry->d_name) - 4;
	    if (UNLIKELY(snprintf(emailPath, sizeof(emailPath), "%s/%s", spoolDir, entry->d_name) >= (int)sizeof(emailPath)
		|| snprintf(structPath, sizeof(structPath), "%s/%.*s.struct", spoolDir, nameLen, entry->d_name) >= (int)sizeof(structPath)
		|| snprintf(resultPath, sizeof(resultPath), "%s/%.*s.result", spoolDir, nameLen, entry->d_name) >= (int)sizeof(resultPath)
	    ))  {
		fprintf(stderr,
		    "Warning: Path of email \"%s\" in \"%s\" is too long to sweep, skipping.\n",
		    entry->d_name, spoolDir
		);
		continue;
		}

	    /** Check if the email is expired. **/
	    expired = smtp_internal_IsExpired(structPath, &now);
	    if (UNLIKELY(expired < 0))
		{
		mssWarnError("Failed to check whether email \"%s\" expired, skipping.", emailPath);
		continue;
		}
	    if (expired != 1) continue;

	    /** Delete the expired email. **/
	    if (UNLIKELY(smtp_internal_RemoveEmail(emailPath, structPath, resultPath) != 0))
		mssWarnError("Failed to delete expired email \"%s\", skipping.", emailPath);
	    }

	/** Success. **/
	successful = true;

    end:
	if (dir != NULL) closedir(dir);
	if (UNLIKELY(newSpool != NULL))
	    {
	    if (newSpool->Path != NULL) nmSysFree(newSpool->Path);
	    nmFree(newSpool, sizeof(SmtpSpool));
	    }

	/** Resolve sweep errors. **/
	if (UNLIKELY(!successful))
	    mssWarnError("Failed to sweep spool directory \"%s\"; continuing.", spoolDir);
    }


/*** smtp_internal_GetStructAttributes - Loads the attributes from the node into
 *** a hash table, and adds their names to a list if one is given.
 ***
 *** @param structInf  The struct to read attributes from.
 *** @param attributes The hash table to add the attributes to.
 *** @param names      The list to add attribute names to, or NULL.
 *** @returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_GetStructAttributes(pStructInf structInf, pXHashTable attributes, pXArray names)
    {
    pSmtpAttribute attr = NULL;
    pStructInf currentAttr = NULL;
    int i;
    pDateTime dt;

	/** Edge cases. **/
	if (UNLIKELY(structInf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to load attributes from NULL struct.");
	    return -1; /* Skip error handler, which expects a valid structInf. */
	    }
	ASSERTMAGIC(structInf, MGK_STRUCTINF);
	if (UNLIKELY(attributes == NULL))
	    {
	    mssError(1, "SMTP", "Failed to load attributes into NULL hash table.");
	    return -1; /* Skip error handler, which expects a valid hash table. */
	    }

	for (i = 0; i < structInf->nSubInf; i++)
	    {
	    /** Get the struct attribute. **/
	    currentAttr = structInf->SubInf[i];
	    if (UNLIKELY(currentAttr == NULL))
		{
		mssError(1, "SMTP", "Struct attribute is NULL.");
		goto error;
		}
	    ASSERTMAGIC(currentAttr, MGK_STRUCTINF);
	    if (UNLIKELY(currentAttr->Value == NULL))
		{
		mssError(1, "SMTP", "Struct attribute '%s' has a NULL value.", currentAttr->Name);
		goto error;
		}
	    ASSERTMAGIC(currentAttr->Value, MGK_EXPRESSION);

	    attr = nmMalloc(sizeof(SmtpAttribute));
	    if (UNLIKELY(attr == NULL))
		{
		mssError(1,"SMTP","Failed to create new attribute object.");
		goto error;
		}
	    memset(attr, 0, sizeof(SmtpAttribute));
	    SETMAGIC(attr, MGK_SMTP_ATTRIBUTE);

	    attr->Name = nmSysStrdup(currentAttr->Name);
	    if (UNLIKELY(attr->Name == NULL))
		{
		mssError(1, "SMTP", "Failed to copy attribute name.");
		goto error;
		}
	    attr->Type = currentAttr->Value->DataType;

	    if (currentAttr->Value->DataType == DATA_T_STRING && (
		strcmp(attr->Name, "expire_date") == 0
		|| strcmp(attr->Name, "first_try_date") == 0
		|| strcmp(attr->Name, "last_try_date") == 0
		|| strcmp(attr->Name, "header_date") == 0
	    ))  {
		/** DateTime attribute, but from a string **/
		attr->Type = DATA_T_DATETIME;
		attr->Value.DateTime = NULL;
		char* dateStr = NULL;
		if (stAttrValue(currentAttr, NULL, &dateStr, 0) < 0 || !dateStr)
		    {
		    attr->Value.DateTime = NULL;
		    }
		else
		    {
		    dt = nmMalloc(sizeof(DateTime));
		    if (UNLIKELY(dt == NULL))
			{
			mssError(1, "SMTP",
			    "Failed to allocate %zu bytes for a date.",
			    sizeof(DateTime)
			);
			goto error;
			}
		    if (UNLIKELY(objDataToDateTime(DATA_T_STRING, dateStr, dt, NULL) != 0))
			{
			mssError(1, "SMTP", "Failed to parse date \"%s\".", dateStr);
			nmFree(dt, sizeof(DateTime));
			goto error;
			}
		    attr->Value.DateTime = dt;
		    }
		}
	    else if (currentAttr->Value->DataType == DATA_T_STRING)
		{
		/** String attribute **/
		attr->Value.String = NULL;
		if (stAttrValue(currentAttr, NULL, &attr->Value.String, 0) < 0 || !attr->Value.String)
		    {
		    attr->Value.String = NULL;
		    }
		else
		    {
		    attr->Value.String = nmSysStrdup(attr->Value.String);
		    if (UNLIKELY(attr->Value.String == NULL))
			{
			mssError(1, "SMTP", "Failed to copy attribute value.");
			goto error;
			}
		    }
		}
	    else if (currentAttr->Value->DataType == DATA_T_INTEGER)
		{
		/** Integer attribute **/
		if (stAttrValue(currentAttr, &attr->Value.Integer, NULL, 0) < 0)
		    {
		    attr->Value.Integer = 0;
		    }
		}
	    else
		{
		mssError(1, "SMTP",
		    "Unsupported attribute type %s in structure file.",
		    objTypeToStr(currentAttr->Value->DataType)
		);
		goto error;
		}

	    /** Store the attribute. **/
	    if (UNLIKELY(xhAdd(attributes, attr->Name, (char*)attr) != 0))
		{
		mssError(1, "SMTP", "Failed to add attribute (it may be a duplicate).");
		goto error;
		}
	    if (UNLIKELY(names != NULL && xaAddItem(names, attr->Name) < 0))
		{
		mssError(1, "SMTP", "Failed to add attribute name to list.");
		xhRemove(attributes, attr->Name);
		goto error;
		}
	    }

	return 0;

    error:
	mssError(0, "SMTP",
	    "Failed to load attribute #%d/%d (%s).",
	    i, structInf->nSubInf, (currentAttr != NULL) ? currentAttr->Name : "NULL"
	);

	if (attr != NULL) smtp_internal_ClearAttribute((char*)attr, NULL);

	return -1;
    }


/*** smtp_internal_ApplyHeaders - Writes the headers from the header_*
 *** and message_id attributes into the email file, replacing existing
 *** headers of the same name.  The date defaults to the current time.
 *** If content_has_headers is false, a blank line separates the headers
 *** from the content.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_ApplyHeaders(pSmtpData inf)
    {
    struct { char* Attr; char* Name; char* Value; } headers[] =
	{
	{ "message_id",                     "Message-ID",              NULL },
	{ "header_date",                    "Date",                    NULL },
	{ "header_from",                    "From",                    NULL },
	{ "header_to",                      "To",                      NULL },
	{ "header_cc",                      "Cc",                      NULL },
	{ "header_bcc",                     "Bcc",                     NULL },
	{ "header_reply_to",                "Reply-To",                NULL },
	{ "header_list_unsubscribe",        "List-Unsubscribe",        NULL },
	{ "header_list_unsubscribe_post",   "List-Unsubscribe-Post",   NULL },
	{ "header_subject",                 "Subject",                 NULL },
	{ "header_user_agent",              "User-Agent",              NULL },
	{ "header_mime_version",            "MIME-Version",            NULL },
	};
    const int n_headers = sizeof(headers) / sizeof(headers[0]);
    pSmtpAttribute attr = NULL;
    pXString new_headers = NULL;
    pXString content = NULL;
    pFile emailFile = NULL;
    DateTime now;
    pDateTime date = NULL;
    struct tm date_tm;
    char date_str[64];
    char buf[1024];
    int i, cnt, name_len, line, line_end, newline;
    void* value;
    int has_headers;
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to apply headers to NULL smtp object.");
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Build the headers set by header attributes. **/
	new_headers = xsNew();
	if (UNLIKELY(new_headers == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate an xstring for the email headers.");
	    goto end;
	    }
	ASSERTMAGIC(new_headers, MGK_XSTRING);
	for (i = 0; i < n_headers; i++)
	    {
	    attr = SMTP_ATTR(xhLookup(inf->Attributes, headers[i].Attr));
	    ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);
	    if (strcmp(headers[i].Attr, "header_date") == 0)
		{
		/** Get the date, defaulting to now. **/
		if (attr == NULL || (attr->Type == DATA_T_DATETIME && attr->Value.DateTime == NULL))
		    {
		    if (UNLIKELY(objCurrentDate(&now) != 0))
			{
			mssError(1, "SMTP",
			    "Unable to obtain the current date for the Date header."
			);
			goto end;
			}
		    date = &now;
		    }
		else if (attr->Type == DATA_T_DATETIME)
		    {
		    date = attr->Value.DateTime;
		    }
		else
		    {
		    mssError(1, "SMTP",
			"Attribute '%s' must be a datetime (got %s).",
			headers[i].Attr, objTypeToStr(attr->Type)
		    );
		    goto end;
		    }

		/** Format the date for the header. **/
		memset(&date_tm, 0, sizeof(date_tm));
		date_tm.tm_sec = date->Part.Second;
		date_tm.tm_min = date->Part.Minute;
		date_tm.tm_hour = date->Part.Hour;
		date_tm.tm_mday = date->Part.Day + 1;
		date_tm.tm_mon = date->Part.Month;
		date_tm.tm_year = date->Part.Year;
		date_tm.tm_isdst = -1;
		if (UNLIKELY(mktime(&date_tm) == (time_t)-1
		    || strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %z", &date_tm) == 0
		))   {
		    mssError(1, "SMTP",
			"Failed to format date %04d-%02d-%02d %02d:%02d:%02d for the Date header.",
			date->Part.Year + 1900, date->Part.Month + 1, date->Part.Day + 1,
			date->Part.Hour, date->Part.Minute, date->Part.Second
		    );
		    goto end;
		    }
		headers[i].Value = date_str;
		}
	    else if (attr != NULL && attr->Type == DATA_T_STRING && attr->Value.String != NULL && attr->Value.String[0] != '\0')
		{
		headers[i].Value = attr->Value.String;
		}
	    else
		{
		continue;
		}

	    /** Add the header. **/
	    if (UNLIKELY(strpbrk(headers[i].Value, "\r\n") != NULL))
		{
		mssError(1, "SMTP",
		    "Attribute '%s' contains a line break: \"%s\".",
		    headers[i].Attr, headers[i].Value
		);
		goto end;
		}
	    if (UNLIKELY(xsConcatPrintf(new_headers,
		(strcmp(headers[i].Attr, "message_id") == 0) ? "%s: <%s>\n" : "%s: %s\n",
		headers[i].Name, headers[i].Value) < 0))
		{
		mssError(1, "SMTP",
		    "Failed to add header '%s: %s'.",
		    headers[i].Name, headers[i].Value
		);
		goto end;
		}
	    }

	/** Read the email. **/
	emailFile = fdOpen(inf->EmailPath.String, O_RDWR, inf->Mask);
	if (UNLIKELY(emailFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to open email file (%s).", inf->EmailPath.String);
	    goto end;
	    }
	content = xsNew();
	if (UNLIKELY(content == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate an xstring for the email content.");
	    goto end;
	    }
	ASSERTMAGIC(content, MGK_XSTRING);
	while ((cnt = fdRead(emailFile, buf, sizeof(buf), content->Length, FD_U_SEEK)) > 0)
	    {
	    if (UNLIKELY(xsConcatenate(content, buf, cnt) < 0))
		{
		mssError(1, "SMTP", "Failed to store %d bytes of email content.", cnt);
		goto end;
		}
	    }
	if (UNLIKELY(cnt < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to read email file at offset %d.", content->Length);
	    goto end;
	    }

	/** Check whether the content starts with headers. **/
	attr = SMTP_ATTR(xhLookup(inf->RootAttributes, "content_has_headers"));
	ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);
	has_headers = 1;
	if (attr != NULL)
	    {
	    value = (attr->Type == DATA_T_INTEGER) ? (void*)&attr->Value.Integer : attr->Value.Generic;
	    has_headers = objDataToBoolean(attr->Type, value, 1);
	    if (UNLIKELY(has_headers < 0))
		{
		fprintf(stderr,
		    "Warning: Ignored unrecognized value '%s' for boolean attribute "
		    "'content_has_headers' of \"%s\" (defaulting to 1).\n",
		    objDataToStringTmp(attr->Type, value, 0), inf->EmailPath.String
		);
		has_headers = 1;
		}
	    }

	/** Add the blank line before the body. **/
	if (!has_headers && UNLIKELY(xsConcatenate(new_headers, "\n", 1) < 0))
	    {
	    mssError(1, "SMTP", "Failed to add the blank line after the headers.");
	    goto end;
	    }

	/** Remove existing headers that the new headers replace. **/
	line = 0;
	while (has_headers && line < content->Length && content->String[line] != '\n' && strncmp(content->String + line, "\r\n", 2) != 0)
	    {
	    /** Find the end of the header, including continuation lines. **/
	    line_end = line;
	    do  {
		newline = xsFind(content, "\n", 1, line_end);
		line_end = (newline < 0) ? content->Length : newline + 1;
		} while (line_end < content->Length && (content->String[line_end] == ' ' || content->String[line_end] == '\t'));

	    /** Check whether a header attribute replaces it. **/
	    for (i = 0; i < n_headers; i++)
		{
		if (headers[i].Value == NULL) continue;
		name_len = strlen(headers[i].Name);
		if (strncasecmp(content->String + line, headers[i].Name, name_len) == 0 && content->String[line + name_len] == ':')
		    break;
		}

	    /** Remove it or move past it. **/
	    if (i < n_headers)
		{
		if (UNLIKELY(xsSubst(content, line, line_end - line, "", 0) < 0))
		    {
		    mssError(1, "SMTP", "Failed to remove existing '%s' header.", headers[i].Name);
		    goto end;
		    }
		}
	    else
		{
		line = line_end;
		}
	    }

	/** Write the email back with the new headers. **/
	if (UNLIKELY(xsConcatenate(new_headers, content->String, content->Length) < 0))
	    {
	    mssError(1, "SMTP",
		"Failed to add %d bytes of email content after the headers.",
		content->Length
	    );
	    goto end;
	    }
	cnt = fdWrite(emailFile, new_headers->String, new_headers->Length, 0, FD_U_SEEK | FD_U_TRUNCATE | FD_U_PACKET);
	if (UNLIKELY(cnt != new_headers->Length))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to write %d bytes to email file (wrote %d).",
		new_headers->Length, cnt
	    );
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP",
		"Failed to apply header attributes to email file (%s).",
		inf->EmailPath.String
	    );

	if (LIKELY(new_headers != NULL)) xsFree(new_headers);
	if (LIKELY(content != NULL)) xsFree(content);
	if (LIKELY(emailFile != NULL)) fdClose(emailFile, 0);

	return rval;
    }


/*** smtp_internal_SetExpireDate - set the expire_date of an email that
 *** became Sent or Error to expire_time seconds from now.  A negative
 *** expire_time keeps the email indefinitely.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_SetExpireDate(pSmtpData inf)
    {
    pSmtpAttribute expireTimeAttr = NULL;
    int expireTime = SMTP_DEFAULT_EXPIRE_TIME;
    DateTime expireDate;
    ObjData pod;

	/** Get the expire time. **/
	expireTimeAttr = SMTP_ATTR(xhLookup(inf->RootAttributes, "expire_time"));
	ASSERTMAGIC(expireTimeAttr, MGK_SMTP_ATTRIBUTE);
	if (expireTimeAttr != NULL)
	    {
	    if (UNLIKELY(expireTimeAttr->Type != DATA_T_INTEGER))
		{
		mssError(1, "SMTP",
		    "Attribute 'expire_time' must be an integer (got %s).",
		    objTypeToStr(expireTimeAttr->Type)
		);
		return -1;
		}
	    expireTime = expireTimeAttr->Value.Integer;
	    }
	if (expireTime < 0)
	    return 0; /* It never expires. */

	/** Calculate and record the expire date. **/
	if (UNLIKELY(objCurrentDate(&expireDate) != 0 || objDateAdd(&expireDate, expireTime, 0, 0, 0, 0, 0) != 0))
	    {
	    mssError(0, "SMTP",
		"Failed to calculate the expire date (%d seconds from now).",
		expireTime
	    );
	    return -1;
	    }
	pod.DateTime = &expireDate;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "expire_date", DATA_T_DATETIME, &pod, NULL) != 0))
	    return -1;

    return 0;
    }


/*** smtp_internal_SendEmail - hand the email message to sendmail, then set
 *** its status (Pending, or Error if the hand-off failed) and the try_count,
 *** *_try_date, and last_try_* attributes of this try.  A Pending email
 *** never expires, so its expire_date is cleared.
 *** @returns 0 if the email was handed off, or -1 if it was not.
 ***/
int
smtp_internal_SendEmail(pSmtpData inf)
    {
    pSmtpAttribute envFrom = NULL;
    pSmtpAttribute envTo = NULL;
    pSmtpAttribute tryCountAttr = NULL;
    pSmtpAttribute firstTryAttr = NULL;
    char* messageId = NULL;
    DateTime noExpireDate;
    DateTime tryDate;
    pXString tryMsg = NULL;
    char* tryMsgStr = "";
    ObjData pod;
    char* status = "Error";
    bool recordFailed = false;
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to send NULL smtp object.");
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Add the header attributes to the email. **/
	if (UNLIKELY(smtp_internal_ApplyHeaders(inf) < 0))
	    goto end;

	/** Drop the mail log results from earlier tries. **/
	messageId = smtp_internal_GetString(inf->Attributes, "message_id");
	if (messageId != NULL)
	    xhRemove(&SMTP_INF.LogByMessageID, messageId);

	/** Get the to and from. **/
	envFrom = SMTP_ATTR(xhLookup(inf->Attributes, "envelope_from"));
	envTo = SMTP_ATTR(xhLookup(inf->Attributes, "envelope_to"));

	/** Send it using sendmail. **/
	if (UNLIKELY(smtp_internal_SpawnSendmail(inf->EmailPath.String, inf->ResultPath.String, envFrom, envTo) < 0))
	    {
	    mssError(0, "SMTP", "Failed to send the mail.");
	    goto end;
	    }

	/** Success. **/
	status = "Pending";
	rval = 0;

    end:
	/** Get the errors of a failed try, before recording adds more. **/
	if (UNLIKELY(rval != 0))
	    {
	    tryMsg = xsNew();
	    if (UNLIKELY(tryMsg == NULL || mssUserError(tryMsg) != 0))
		{
		mssError(0, "SMTP", "Failed to get the error message of the failed try.");
		recordFailed = true;
		}
	    else
		{
		tryMsgStr = tryMsg->String;
		}
	    }

	/** Record the status. **/
	pod.String = status;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "status", DATA_T_STRING, &pod, NULL) != 0))
	    recordFailed = true;

	/** Record the try result. **/
	pod.String = (rval == 0) ? "None" : "Fail";
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_status", DATA_T_STRING, &pod, NULL) != 0))
	    recordFailed = true;
	pod.String = tryMsgStr;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_msg", DATA_T_STRING, &pod, NULL) != 0))
	    recordFailed = true;

	/** Count the try. **/
	tryCountAttr = SMTP_ATTR(xhLookup(inf->Attributes, "try_count"));
	ASSERTMAGIC(tryCountAttr, MGK_SMTP_ATTRIBUTE);
	pod.Integer = (tryCountAttr != NULL && tryCountAttr->Type == DATA_T_INTEGER) ? tryCountAttr->Value.Integer + 1 : 1;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "try_count", DATA_T_INTEGER, &pod, NULL) != 0))
	    recordFailed = true;

	/** Record the try dates, keeping the first one. **/
	if (UNLIKELY(objCurrentDate(&tryDate) != 0))
	    {
	    mssError(0, "SMTP", "Failed to get the current date for the try dates.");
	    recordFailed = true;
	    }
	else
	    {
	    pod.DateTime = &tryDate;
	    if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_date", DATA_T_DATETIME, &pod, NULL) != 0))
		recordFailed = true;
	    firstTryAttr = SMTP_ATTR(xhLookup(inf->Attributes, "first_try_date"));
	    ASSERTMAGIC(firstTryAttr, MGK_SMTP_ATTRIBUTE);
	    if (firstTryAttr == NULL || firstTryAttr->Type != DATA_T_DATETIME
		|| firstTryAttr->Value.DateTime == NULL || firstTryAttr->Value.DateTime->Value == 0)
		{
		if (UNLIKELY(smtp_internal_SetAttrValue(inf, "first_try_date", DATA_T_DATETIME, &pod, NULL) != 0))
		    recordFailed = true;
		}
	    }

	/** Record no expire date. (Pending emails don't expire.) **/
	if (rval == 0)
	    {
	    memset(&noExpireDate, 0, sizeof(DateTime));
	    pod.DateTime = &noExpireDate;
	    if (UNLIKELY(smtp_internal_SetAttrValue(inf, "expire_date", DATA_T_DATETIME, &pod, NULL) != 0))
		recordFailed = true;
	    }
	else if (UNLIKELY(smtp_internal_SetExpireDate(inf) != 0))
	    recordFailed = true;

	/** Resolve recording errors, since the email was handed off. **/
	if (UNLIKELY(recordFailed && rval == 0))
	    mssWarnError("Handed email \"%s\" to sendmail but failed to record all of the results.", inf->Name);

	if (tryMsg != NULL) xsFree(tryMsg);

	return rval;
    }


/*** smtp_internal_ReadResult - read the result file that the sendmail
 *** supervisor writes once sendmail finishes.
 ***
 *** @param resultPath The path of the result file.
 *** @param header Set to the status line without its padding, such as
 ***   "exit 0".  Must hold SMTP_RESULT_HEADER_LEN + 1 bytes.
 *** @param output Set to the start of sendmail's output, on one line.
 *** @returns 1 if the result was read, 0 if sendmail has not finished, or -1
 ***   on failure.
 ***/
int
smtp_internal_ReadResult(char* resultPath, char* header, pXString output)
    {
    char buf[SMTP_RESULT_HEADER_LEN + SMTP_TRY_MSG_MAX];
    int fd = -1;
    int len = 0;
    int n;
    int i;
    int rval = -1;

	/** Open the result file. **/
	fd = open(resultPath, O_RDONLY);
	if (fd < 0)
	    {
	    if (errno == ENOENT)
		rval = 0; /* Sendmail has not finished. */
	    else
		mssErrorErrno(1, "SMTP", "Failed to open sendmail result file (%s).", resultPath);
	    goto end;
	    }

	/** Read the result file. **/
	while (len < (int)sizeof(buf))
	    {
	    n = read(fd, buf + len, sizeof(buf) - len);
	    if (UNLIKELY(n < 0))
		{
		mssErrorErrno(1, "SMTP", "Failed to read sendmail result file (%s).", resultPath);
		goto end;
		}
	    if (n == 0)
		break;
	    len += n;
	    }
	if (UNLIKELY(len < SMTP_RESULT_HEADER_LEN))
	    {
	    mssError(1, "SMTP",
		"Failed to read sendmail result file (%s): it has only %d bytes.",
		resultPath, len
	    );
	    goto end;
	    }

	/** Get the status line. **/
	memcpy(header, buf, SMTP_RESULT_HEADER_LEN);
	header[SMTP_RESULT_HEADER_LEN] = '\0';
	for (i = SMTP_RESULT_HEADER_LEN - 1; i >= 0 && (header[i] == ' ' || header[i] == '\n'); i--)
	    header[i] = '\0';

	/** Get the output on one line. **/
	for (i = SMTP_RESULT_HEADER_LEN; i < len; i++)
	    if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\t')
		buf[i] = ' ';
	while (len > SMTP_RESULT_HEADER_LEN && buf[len - 1] == ' ')
	    len--;
	if (UNLIKELY(xsCopy(output, buf + SMTP_RESULT_HEADER_LEN, len - SMTP_RESULT_HEADER_LEN) != 0))
	    {
	    mssError(1, "SMTP", "Failed to copy the output in sendmail result file (%s).", resultPath);
	    goto end;
	    }

	/** Success. **/
	rval = 1;

    end:
	if (fd >= 0) close(fd);

	return rval;
    }


/*** smtp_internal_UpdateStatus - update a Pending email from the result of
 *** handing it to sendmail and the results Postfix logged.  It becomes Sent
 *** or Error once Postfix finishes, or Error if there is still no result
 *** SMTP_PENDING_TIMEOUT seconds after last_try_date.
 ***
 *** @returns 0 on success (including no change), or -1 on failure.
 ***/
int
smtp_internal_UpdateStatus(pSmtpData inf)
    {
    char header[SMTP_RESULT_HEADER_LEN + 1];
    XString output;
    XString tryMsg;
    bool initialized = false;
    char* status = NULL; /* The final status, or NULL while Pending. */
    char* tryStatus = NULL; /* The new last_try_status, or NULL to keep it. */
    char* messageId;
    char* current;
    pSmtpLogMsg msg = NULL;
    pSmtpLogRcpt rcpt;
    pSmtpAttribute lastTryAttr;
    DateTime cutoff;
    DateTime now;
    ObjData pod;
    char* sep = " ";
    int result;
    int printed;
    int code;
    int nSent = 0;
    int nBounced = 0;
    int nDeferred = 0;
    int nTotal;
    int i;
    int rval = -1;

	/** Initialize send status strings. **/
	if (UNLIKELY(xsInit(&output) != 0 || xsInit(&tryMsg) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the send status strings.");
	    goto end;
	    }
	initialized = true;

	/** Check whether the hand-off to sendmail failed. **/
	result = smtp_internal_ReadResult(inf->ResultPath.String, header, &output);
	if (UNLIKELY(result < 0))
	    goto end;
	if (result == 1 && strcmp(header, "exit 0") != 0)
	    {
	    status = "Error";
	    tryStatus = "Fail";
	    if (sscanf(header, "exit %d", &code) == 1)
		printed = xsPrintf(&tryMsg, "Sendmail exited with status %d", code);
	    else if (sscanf(header, "signal %d", &code) == 1)
		printed = xsPrintf(&tryMsg, "Sendmail was killed by signal %d", code);
	    else if (strcmp(header, "timeout") == 0)
		printed = xsPrintf(&tryMsg, "Sendmail was killed after %d seconds", SMTP_SENDMAIL_TIMEOUT);
	    else if (strcmp(header, "error") == 0)
		printed = xsPrintf(&tryMsg, "Failed to run sendmail");
	    else
		printed = xsPrintf(&tryMsg, "Unknown sendmail result \"%s\"", header);
	    if (UNLIKELY(printed < 0
		|| (output.Length > 0 && xsConcatPrintf(&tryMsg, ": %s", output.String) < 0)
		|| xsConcatenate(&tryMsg, ".", 1) < 0
	    ))  {
		mssError(1, "SMTP", "Failed to describe the sendmail result \"%s\".", header);
		goto end;
		}
	    }

	/** Find what Postfix logged, once sendmail has the email. **/
	else if (result == 1)
	    {
	    messageId = smtp_internal_GetString(inf->Attributes, "message_id");
	    if (messageId != NULL && UNLIKELY(smtp_internal_LookupLog(messageId, &msg) != 0))
		goto end;
	    }

	/** Check the recipient results. **/
	if (msg != NULL)
	    {
	    for (i = 0; i < msg->Rcpts.nItems; i++)
		{
		rcpt = (pSmtpLogRcpt)msg->Rcpts.Items[i];
		ASSERTMAGIC(rcpt, MGK_SMTP_LOG_RCPT);
		if (rcpt->Status == SMTP_RCPT_SENT) nSent++;
		else if (rcpt->Status == SMTP_RCPT_BOUNCED) nBounced++;
		else nDeferred++;
		}
	    nTotal = (msg->RcptCount > msg->Rcpts.nItems) ? msg->RcptCount : msg->Rcpts.nItems;

	    /** Describe the recipients that were not sent the email. **/
	    if (nBounced > 0 || nDeferred > 0)
		{
		if (UNLIKELY(xsPrintf(&tryMsg, "Sent to %d of %d recipients.", nSent, nTotal) < 0))
		    {
		    mssError(1, "SMTP", "Failed to describe the recipients of Message-ID <%s>.", msg->MessageID);
		    goto end;
		    }
		for (i = 0; i < msg->Rcpts.nItems; i++)
		    {
		    rcpt = (pSmtpLogRcpt)msg->Rcpts.Items[i];
		    if (rcpt->Status == SMTP_RCPT_SENT)
			continue;
		    if (UNLIKELY(xsConcatPrintf(&tryMsg, "%s%s: %s", sep, rcpt->Address, rcpt->Reply) < 0))
			{
			mssError(1, "SMTP", "Failed to describe recipient <%s>.", rcpt->Address);
			goto end;
			}
		    sep = "; ";
		    }
		}

	    /** Postfix is done when every recipient is sent or bounced, or it gives up. **/
	    if (msg->Expired || (msg->RcptCount > 0 && nSent + nBounced >= msg->RcptCount))
		{
		status = (nBounced == 0 && nDeferred == 0 && !msg->Expired) ? "Sent" : "Error";
		tryStatus = (strcmp(status, "Sent") == 0) ? "None" : "Fail";
		}
	    else if (nDeferred > 0)
		{
		tryStatus = "TempFail";
		}
	    }

	/** Give up on an email with no result in time. **/
	lastTryAttr = SMTP_ATTR(xhLookup(inf->Attributes, "last_try_date"));
	ASSERTMAGIC(lastTryAttr, MGK_SMTP_ATTRIBUTE);
	if (status == NULL && lastTryAttr != NULL && lastTryAttr->Type == DATA_T_DATETIME
	    && lastTryAttr->Value.DateTime != NULL && lastTryAttr->Value.DateTime->Value != 0)
	    {
	    cutoff = *lastTryAttr->Value.DateTime;
	    if (UNLIKELY(objDateAdd(&cutoff, SMTP_PENDING_TIMEOUT, 0, 0, 0, 0, 0) != 0 || objCurrentDate(&now) != 0))
		{
		mssError(0, "SMTP", "Failed to check whether the send status timed out.");
		goto end;
		}
	    if (now.Value >= cutoff.Value)
		{
		status = "Error";
		tryStatus = "Fail";
		if (UNLIKELY(xsPrintf(&tryMsg,
		    "Send status unknown %d days after the last try.",
		    SMTP_PENDING_TIMEOUT / (24 * 60 * 60)
		) < 0))
		    {
		    mssError(1, "SMTP", "Failed to describe the send status timeout.");
		    goto end;
		    }
		}
	    }

	/** Record the try result, if it changed. **/
	if (tryStatus != NULL)
	    {
	    current = smtp_internal_GetString(inf->Attributes, "last_try_status");
	    if (current == NULL || strcmp(current, tryStatus) != 0)
		{
		pod.String = tryStatus;
		if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_status", DATA_T_STRING, &pod, NULL) != 0))
		    goto end;
		}
	    current = smtp_internal_GetString(inf->Attributes, "last_try_msg");
	    if (current == NULL || strcmp(current, tryMsg.String) != 0)
		{
		pod.String = tryMsg.String;
		if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_msg", DATA_T_STRING, &pod, NULL) != 0))
		    goto end;
		}
	    }

	/** Record the final status last, so a failure leaves the email Pending. **/
	if (status != NULL)
	    {
	    if (UNLIKELY(smtp_internal_SetExpireDate(inf) != 0))
		goto end;
	    pod.String = status;
	    if (UNLIKELY(smtp_internal_SetAttrValue(inf, "status", DATA_T_STRING, &pod, NULL) != 0))
		goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (LIKELY(initialized))
	    {
	    xsDeInit(&output);
	    xsDeInit(&tryMsg);
	    }

	return rval;
    }


/*** smtp_internal_CreateRootNode - Creates a root smtp node.
 *** Returns the newly created root node or NULL (if creation failed).
 ***/
pSnNode
smtp_internal_CreateRootNode(pObject obj, int mask)
    {
    pSnNode node = NULL;
    pSmtpAttribute currentAttr = NULL;
    pStructInf currentParam = NULL;
    pSnNode rval = NULL;
    int i;

	/** Create the node object **/
	node = snNewNode(obj, "system/smtp");
	if (UNLIKELY(node == NULL))
	    {
	    mssError(0, "SMTP", "Failed to create new node object");
	    goto end;
	    }
	ASSERTMAGIC(node, MGK_STNODE);

	/** Iterate through all the default root attributes. **/
	for (i = 0; i < SMTP_INF.DefaultRootAttributes.nItems; i ++)
	    {
	    /** Get the default attribute. **/
	    currentAttr = SMTP_ATTR(SMTP_INF.DefaultRootAttributes.Items[i]);
	    if (UNLIKELY(currentAttr == NULL))
		{
		mssError(1, "SMTP", "Default root attribute %d is NULL.", i);
		goto end;
		}
	    ASSERTMAGIC(currentAttr, MGK_SMTP_ATTRIBUTE);

	    /** Add the attribute to the node. **/
	    currentParam = stAddAttr(node->Data, currentAttr->Name);
	    if (UNLIKELY(currentParam == NULL))
		{
		mssError(0, "SMTP", "Failed to add attribute value %s", currentAttr->Name);
		goto end;
		}

	    /** Set the attribute to its default value. **/
	    if (UNLIKELY(stSetAttrValue(currentParam, currentAttr->Type, &currentAttr->Value, 0) != 0))
		{
		mssError(1, "SMTP", "Failed to set attribute value %s", currentAttr->Name);
		goto end;
		}
	    }

	/** Write the root node structure file. **/
	if (UNLIKELY(snWriteNode(obj, node) < 0))
	    {
	    mssError(0, "SMTP", "Failed to write the root node structure file.");
	    goto end;
	    }

	/** Success. **/
	rval = node;

    end:
	if (UNLIKELY(rval == NULL))
	    {
	    mssError(0, "SMTP", "Failed to create root node.");
	    if (node != NULL) snDelete(node);
	    }

	return rval;
    }


/*** smtp_internal_CreateEmail - Create a new email file.
 ***/
int
smtp_internal_CreateEmail(pSmtpData inf)
    {
    pXString autoName = NULL;

    pSmtpAttribute hostName = NULL;

    pStructInf emailStruct = NULL;
    pStructInf createdStruct = NULL;
    pSmtpAttribute currentAttr = NULL;
    pDateTime attrDate = NULL;

    pFile checkFile = NULL;
    pFile emailFile = NULL;
    pFile emailStructFile = NULL;
    char message_id[80];
    ObjData pod;
    int i;
    unsigned char email_id[8];
    char local_host_name[128] = "localhost.localdomain";

    bool emailCreated = false;
    bool structCreated = false;
    int prefix_len;
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to create email for NULL smtp object.");
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf->Obj == NULL))
	    {
	    mssError(1, "SMTP", "Failed to create email for smtp object with NULL object.");
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf->Obj, MGK_OBJECT);

	autoName = xsNew();
	if (UNLIKELY(autoName == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate an xstring for the email name.");
	    goto end;
	    }
	ASSERTMAGIC(autoName, MGK_XSTRING);

	/** Resolve autonaming. **/
	prefix_len = inf->EmailPath.Length - 1;
	if (inf->Obj->Mode & OBJ_O_AUTONAME && strcmp(inf->Name, "*") == 0)
	    {
	    for(i=0; i<100; i++)
		{
		/** Generate a random email name. **/
		if (UNLIKELY(cxssGenerateKey(email_id, 8) < 0))
		    {
		    mssError(1, "SMTP", "Failed to generate a random email name.");
		    goto end;
		    }
		if (UNLIKELY(xsQPrintf(autoName, "%4STR&HEX-%4STR&HEX.eml", email_id, email_id+4) < 0))
		    {
		    mssError(1, "SMTP", "Failed to format a random email name.");
		    goto end;
		    }

		/** Build the full email path. **/
		if (UNLIKELY(xsSubst(&inf->EmailPath, prefix_len, inf->EmailPath.Length - prefix_len, autoName->String, autoName->Length) < 0))
		    {
		    mssError(1, "SMTP",
			"Failed to substitute email name \"%s\" into email path.",
			autoName->String
		    );
		    goto end;
		    }

		/** Continue generating new filenames until no file is found. **/
		checkFile = fdOpen(inf->EmailPath.String, 0, 0);
		if (!checkFile)
		    break;
		fdClose(checkFile, 0);
		checkFile = NULL;
		}
	    if (i >= 100)
		{
		mssError(1, "SMTP",
		    "Unable to auto-generate a unique filename. May have exceeded allowable range of filenames."
		);
		goto end;
		}

	    /** Set a new object name. **/
	    nmSysFree(inf->Name);
	    inf->Name = nmSysStrdup(autoName->String);
	    if (UNLIKELY(inf->Name == NULL))
		{
		mssError(1, "SMTP", "Failed to copy email name \"%s\".", autoName->String);
		goto end;
		}
	    }

	/** Create the email file. **/
	emailFile = fdOpen(inf->EmailPath.String, O_WRONLY | O_CREAT | O_EXCL, inf->Mask);
	if (UNLIKELY(emailFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to create a new email file (%s).",
		inf->EmailPath.String
	    );
	    goto end;
	    }
	emailCreated = true;

	/** Construct the email struct file path. **/
	if (UNLIKELY(xsCopy(&inf->EmailStructPath, inf->EmailPath.String, -1) != 0))
	    {
	    mssError(1, "SMTP", "Failed to copy email struct path.");
	    goto end;
	    }
	if (UNLIKELY(xsSubst(&inf->EmailStructPath, inf->EmailStructPath.Length - 4, 4, ".struct", 7) < 0))
	    {
	    mssError(1, "SMTP", "Failed to substitute .struct into email struct path.");
	    goto end;
	    }

	/** Construct the sendmail result file path. **/
	if (UNLIKELY(xsCopy(&inf->ResultPath, inf->EmailPath.String, -1) != 0))
	    {
	    mssError(1, "SMTP", "Failed to copy sendmail result path.");
	    goto end;
	    }
	if (UNLIKELY(xsSubst(&inf->ResultPath, inf->ResultPath.Length - 4, 4, ".result", 7) < 0))
	    {
	    mssError(1, "SMTP", "Failed to substitute .result into sendmail result path.");
	    goto end;
	    }

	/** Create the email node. **/
	emailStruct = stCreateStruct(inf->Name, "system/structure");
	if (UNLIKELY(emailStruct == NULL))
	    {
	    mssError(0, "SMTP", "Failed to create new email struct.");
	    goto end;
	    }
	if (UNLIKELY(stSetVersion(emailStruct, 2) != 0))
	    {
	    mssError(1, "SMTP", "Failed to set email struct version.");
	    goto end;
	    }

	/** Add the default static attributes. **/
	for (i=0; i < SMTP_INF.DefaultEmailAttributes.nItems; i++)
	    {
	    /** Get the attribute from the default attribute array. **/
	    currentAttr = (pSmtpAttribute)xaGetItem(&SMTP_INF.DefaultEmailAttributes, i);
	    if (UNLIKELY(currentAttr == NULL))
		{
		mssError(1, "SMTP", "Unable to get default attribute %d.", i);
		goto end;
		}
	    ASSERTMAGIC(currentAttr, MGK_SMTP_ATTRIBUTE);

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emailStruct, currentAttr->Name);
	    if (UNLIKELY(createdStruct == NULL))
		{
		mssError(1, "SMTP",
		    "Unable to add new attribute (%s) to the email struct.",
		    currentAttr->Name
		);
		goto end;
		}

	    /** Set the default attribute value. **/
	    if (UNLIKELY(stSetAttrValue(createdStruct, currentAttr->Type, &currentAttr->Value, 0) != 0))
		{
		mssError(1, "SMTP",
		    "Unable to write to the default attribute (%s).",
		    currentAttr->Name
		);
		goto end;
		}
	    }

	/** Add dynamic attributes which have object specific defaults. **/
	/** Calculate the message id (name without suffix). **/
	hostName = SMTP_ATTR(xhLookup(inf->RootAttributes, "local_host_name"));
	ASSERTMAGIC(hostName, MGK_SMTP_ATTRIBUTE);
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
	    fprintf(stderr,
		"Warning: gethostname() failed (%s); using \"%s\".\n",
		strerror(errno), local_host_name
	    );
	strtcpy(message_id, inf->Name, sizeof(message_id));
	if (strrchr(message_id, '.'))
	    *(strrchr(message_id, '.')) = '\0';
	strtcat(message_id, "@", sizeof(message_id));
	strtcat(message_id, hostName?(hostName->Value.String):local_host_name, sizeof(message_id));

	/** Create the message_id attribute. **/
	createdStruct = stAddAttr(emailStruct, "message_id");
	if (UNLIKELY(createdStruct == NULL))
	    {
	    mssError(1, "SMTP", "Unable to add new attribute (message_id) to the email struct.");
	    goto end;
	    }

	/** Set the default message_id value. **/
	pod.String = message_id;
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_STRING, &pod, 0) != 0))
	    {
	    mssError(1, "SMTP", "Unable to write to the default attribute (message_id).");
	    goto end;
	    }

	/** Allocate an empty expire_date, which is set when sent. **/
	attrDate = (pDateTime)nmMalloc(sizeof(DateTime));
	if (UNLIKELY(attrDate == NULL))
	    {
	    mssError(1, "SMTP",
		"Failed to allocate a date structure for the default attribute (expire_date)."
	    );
	    goto end;
	    }
	memset(attrDate, 0, sizeof(DateTime));

	/** Create the expire_date attribute. **/
	createdStruct = stAddAttr(emailStruct, "expire_date");
	if (UNLIKELY(createdStruct == NULL))
	    {
	    mssError(1, "SMTP", "Unable to add new attribute (expire_date) to the email struct.");
	    goto end;
	    }

	/** Set the default expire_date value. **/
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0) != 0))
	    {
	    mssError(1, "SMTP", "Unable to write to the default attribute (expire_date).");
	    goto end;
	    }
	nmFree(attrDate, sizeof(DateTime));
	attrDate = NULL;

	/** Allocate an empty first_try_date, which is set on the first try. **/
	attrDate = (pDateTime)nmMalloc(sizeof(DateTime));
	if (UNLIKELY(attrDate == NULL))
	    {
	    mssError(1, "SMTP",
		"Failed to allocate a date structure for the default attribute (first_try_date)."
	    );
	    goto end;
	    }
	memset(attrDate, 0, sizeof(DateTime));

	/** Create the first_try_date attribute. **/
	createdStruct = stAddAttr(emailStruct, "first_try_date");
	if (UNLIKELY(createdStruct == NULL))
	    {
	    mssError(1, "SMTP",
		"Unable to add new attribute (first_try_date) to the email struct."
	    );
	    goto end;
	    }

	/** Set the default first_try_date value. **/
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0) != 0))
	    {
	    mssError(1, "SMTP", "Unable to write to the default attribute (first_try_date).");
	    goto end;
	    }
	nmFree(attrDate, sizeof(DateTime));
	attrDate = NULL;

	/** Allocate a new date data structure. **/
	attrDate = (pDateTime)nmMalloc(sizeof(DateTime));
	if (UNLIKELY(attrDate == NULL))
	    {
	    mssError(1, "SMTP",
		"Failed to allocate a date structure for the default attribute (last_try_date)."
	    );
	    goto end;
	    }
	memset(attrDate, 0, sizeof(DateTime));

	/** Create the last_try_date attribute. **/
	createdStruct = stAddAttr(emailStruct, "last_try_date");
	if (UNLIKELY(createdStruct == NULL))
	    {
	    mssError(1, "SMTP", "Unable to add new attribute (last_try_date) to the email struct.");
	    goto end;
	    }

	/** Set the default last_try_date value. **/
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0) != 0))
	    {
	    mssError(1, "SMTP", "Unable to write to the default attribute (last_try_date).");
	    goto end;
	    }
	nmFree(attrDate, sizeof(DateTime));
	attrDate = NULL;

	/** Create the struct file. **/
	emailStructFile = fdOpen(inf->EmailStructPath.String, O_CREAT | O_RDWR | O_EXCL, 0755);
	if (UNLIKELY(emailStructFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP",
		"Unable to create the email struct file (%s).",
		inf->EmailStructPath.String
	    );
	    goto end;
	    }
	structCreated = true;

	/** Write the struct file. **/
	if (UNLIKELY(stGenerateMsg(emailStructFile, emailStruct, 0) != 0))
	    {
	    mssError(1, "SMTP",
		"Failed to write the email struct file: %s.",
		inf->EmailStructPath.String
	    );
	    goto end;
	    }

	/** Fill the email file with some basic attributes. **/

	/** Fill in the non-static default headers. **/
	// TODO: Add current date to the header... once we implement date support in the MIME driver

	/** Add an empty line for header separation to the file. **/
	if (UNLIKELY(fdWrite(emailFile, "\n", 1, 0, 0) < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to write default header separator to new message.");
	    goto end;
	    }

	/** Mark this object so the OSML doesn't automatically layer the MIME driver **/
	inf->Obj->Flags |= OBJ_F_NOCASCADE;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to create email (%s).", inf->EmailPath.String);

	if (LIKELY(autoName != NULL)) xsFree(autoName);
	if (LIKELY(emailFile != NULL)) fdClose(emailFile, 0);
	if (LIKELY(emailStructFile != NULL)) fdClose(emailStructFile, 0);
	if (UNLIKELY(attrDate != NULL)) nmFree(attrDate, sizeof(DateTime));
	if (LIKELY(emailStruct != NULL)) stFreeInf(emailStruct);

	/** Remove the files this call created if it failed. **/
	if (UNLIKELY(rval != 0))
	    {
	    if (emailCreated && remove(inf->EmailPath.String) != 0)
		fprintf(stderr,
		    "Warning: Failed to remove partial email file (%s): %s.\n",
		    inf->EmailPath.String, strerror(errno)
		);
	    if (structCreated && remove(inf->EmailStructPath.String) != 0)
		fprintf(stderr,
		    "Warning: Failed to remove partial email struct file (%s): %s.\n",
		    inf->EmailStructPath.String, strerror(errno)
		);
	    }

	return rval;
    }


/*** smtp_internal_OpenGeneral - Loads attributes common to all SMTP objects.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenGeneral(pSmtpData inf, char* usrtype)
    {
    pSnNode node = NULL;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to open NULL smtp object.");
	    goto error;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf->Obj == NULL))
	    {
	    mssError(1, "SMTP", "Failed to open smtp object with NULL object.");
	    goto error;
	    }
	ASSERTMAGIC(inf->Obj, MGK_OBJECT);
	if (UNLIKELY(inf->Obj->Prev == NULL))
	    {
	    mssError(1, "SMTP", "Failed to open smtp object with NULL parent object.");
	    goto error;
	    }

	/** Try to open the root node first. **/
	if (!node)
	    {
	    node = snReadNode(inf->Obj->Prev);
	    }

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((inf->Obj->Mode & O_CREAT) && (inf->Obj->Mode & O_EXCL) && (inf->Obj->SubPtr == inf->Obj->Pathname->nElements))
	    {
	    if (UNLIKELY(node != NULL))
		{
		mssError(1, "SMTP",
		    "Node exists and CREAT and EXCL flags are set. Cannot create new node."
		);
		goto error;
		}

	    node = smtp_internal_CreateRootNode(inf->Obj, inf->Mask);
	    if (UNLIKELY(node == NULL))
		{
		mssError(0,"SMTP", "Failed to create new node object");
		goto error;
		}
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (inf->Obj->Mode & O_CREAT) && (inf->Obj->SubPtr == inf->Obj->Pathname->nElements))
	    {
	    node = smtp_internal_CreateRootNode(inf->Obj, inf->Mask);
	    }

	/** If _still_ no node, quit out. **/
	if (UNLIKELY(node == NULL))
	    {
	    ASSERTMAGIC(inf->Obj->Prev, MGK_OBJECT);
	    char* node_path = obj_internal_PathPart(inf->Obj->Prev->Pathname, 0, inf->Obj->Prev->SubPtr + inf->Obj->Prev->SubCnt - 1);
	    mssError(0, "SMTP",
		"Failed to open structure file: %s.",
		(node_path != NULL) ? node_path : "unknown path"
	    );
	    goto error;
	    }
	ASSERTMAGIC(node, MGK_STNODE);

	/** Store the node object. **/
	inf->Node = node;
	inf->Node->OpenCnt++;

	char* name = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->SubPtr + inf->Obj->SubCnt - 2, 1);
	if (UNLIKELY(name == NULL))
	    goto error;
	inf->Name = nmSysStrdup(name);
	if (UNLIKELY(inf->Name == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy object name \"%s\".", name);
	    goto error;
	    }

	inf->AttributeNames = xaNew(16);
	if (UNLIKELY(inf->AttributeNames == NULL))
	    {
	    mssError(1,"SMTP","Failed to create attribute names array.");
	    goto error;
	    }

	inf->Attributes = smtp_internal_NewAttributes();
	if (UNLIKELY(inf->Attributes == NULL))
	    goto error;

	inf->CurAttr = 0;

	return 0;

    error:
	mssError(0, "SMTP", "Failed to load SMTP node attributes.");
	return -1;
    }


/*** smtp_internal_OpenRoot - Open the root node of the smtp structure.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenRoot(pSmtpData inf, char* usrtype)
    {
	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to open root node for NULL smtp object.");
	    goto error;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Perform a general open. **/
	if (UNLIKELY(smtp_internal_OpenGeneral(inf, usrtype) < 0))
	    goto error;

	/** Set the node type. **/
	inf->Type = SMTP_T_ROOT;

	/** Load the root attributes. **/
	if (UNLIKELY(smtp_internal_GetStructAttributes(inf->Node->Data, inf->Attributes, inf->AttributeNames) != 0))
	    {
	    mssError(0, "SMTP", "Failed to load root attributes.");
	    goto error;
	    }

	return 0;

    error:
	mssError(0, "SMTP", "Failed to open root node.");
	return -1;
    }


/*** smtp_internal_OpenEml - Open an email file in the smtp structure.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenEml(pSmtpData inf, char* usrtype)
    {
    pFile fd = NULL;
    pFile emailStructureFile = NULL;
    pStructInf emailStructure = NULL;
    char* status = NULL;
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to open email for NULL smtp object.");
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf->Obj == NULL))
	    {
	    mssError(1, "SMTP", "Failed to open email for smtp object with NULL object.");
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf->Obj, MGK_OBJECT);

	/** Perform a general open. **/
	if (UNLIKELY(smtp_internal_OpenGeneral(inf, usrtype) < 0))
	    goto end;

	/** Set the node type. **/
	inf->Type = SMTP_T_EML;

	/** Load the root attributes. **/
	inf->RootAttributes = smtp_internal_NewAttributes();
	if (UNLIKELY(inf->RootAttributes == NULL))
	    goto end;
	if (UNLIKELY(smtp_internal_GetStructAttributes(inf->Node->Data, inf->RootAttributes, NULL) != 0))
	    {
	    mssError(0, "SMTP", "Failed to load root attributes.");
	    goto end;
	    }

	/** Calculate the real path of the email file. **/
	pSmtpAttribute spoolDir = SMTP_ATTR(xhLookup(inf->RootAttributes, "spool_dir"));
	ASSERTMAGIC(spoolDir, MGK_SMTP_ATTRIBUTE);
	if (UNLIKELY(spoolDir == NULL))
	    {
	    mssError(1, "SMTP", "The SMTP node does not have the required 'spool_dir' attribute.");
	    goto end;
	    }

	if (UNLIKELY(xsCopy(
	    &inf->EmailPath,
	    spoolDir->Value.String,
	    strlen(spoolDir->Value.String)
	) != 0))
	    {
	    mssError(1, "SMTP", "Unable to copy spool directory path into the email path.");
	    goto end;
	    }

	if (UNLIKELY(xsConcatPrintf(&inf->EmailPath, "/%s", inf->Name) < 0))
	    {
	    mssError(1, "SMTP", "Unable to append email name to email path.");
	    goto end;
	    }

	/** Check that the email file exists. **/
	fd = fdOpen(inf->EmailPath.String, 0, 0);
	if (UNLIKELY(fd == NULL))
	    {
	    /** Create the file if it doesn't exist and the create flag is set. **/
	    if (inf->Obj->Mode & OBJ_O_CREAT)
		{
		/** Sweep the spool dir to clean up expired emails. **/
		smtp_internal_SweepSpool(spoolDir->Value.String);

		/** Create the requested email. **/
		if (UNLIKELY(smtp_internal_CreateEmail(inf) < 0))
		    {
		    mssError(0, "SMTP", "Failed to create a new email.");
		    goto end;
		    }
		}
	    else
		{
		/** File does not exist, and creation not requested **/
		mssErrorErrno(1, "SMTP",
		    "Failed to open email file: \"%s\".",
		    inf->EmailPath.String
		);
		goto end;
		}
	    }
	else
	    {
	    /** Creation requested with exclude, but file exists? **/
	    if ((inf->Obj->Mode & OBJ_O_CREAT) && (inf->Obj->Mode & OBJ_O_EXCL))
		{
		mssError(1, "SMTP",
		    "Email creation request failed because the email already exists: %s.",
		    inf->EmailPath.String
		);
		goto end;
		}

	    /** Construct the email struct file path. **/
	    if (UNLIKELY(xsCopy(&inf->EmailStructPath, inf->EmailPath.String, -1) != 0))
		{
		mssError(1, "SMTP", "Failed to copy email struct path.");
		goto end;
		}
	    if (UNLIKELY(xsSubst(&inf->EmailStructPath, inf->EmailStructPath.Length - 4, 4, ".struct", 7) < 0))
		{
		mssError(1, "SMTP", "Failed to substitute .struct into email struct path.");
		goto end;
		}

	    /** Construct the sendmail result file path. **/
	    if (UNLIKELY(xsCopy(&inf->ResultPath, inf->EmailPath.String, -1) != 0))
		{
		mssError(1, "SMTP", "Failed to copy sendmail result path.");
		goto end;
		}
	    if (UNLIKELY(xsSubst(&inf->ResultPath, inf->ResultPath.Length - 4, 4, ".result", 7) < 0))
		{
		mssError(1, "SMTP", "Failed to substitute .result into sendmail result path.");
		goto end;
		}

	    fdClose(fd, 0);
	    fd = NULL;
	    }

	/** Open the email file. **/
	const int open_mode = inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL);
	if (UNLIKELY(inf->ContentFile == NULL))
	    inf->ContentFile = fdOpen(inf->EmailPath.String, open_mode, inf->Mask);
	if (UNLIKELY(inf->ContentFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to open email file (%s).", inf->EmailPath.String);
	    goto end;
	    }

	/** Open the email structure file. **/
	emailStructureFile = fdOpen(inf->EmailStructPath.String, O_RDONLY, inf->Mask);
	if (UNLIKELY(emailStructureFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to open email structure file: \"%s\".",
		inf->EmailStructPath.String
	    );
	    goto end;
	    }

	/** Parse the structure file. **/
	emailStructure = stParseMsg(emailStructureFile, 0);
	if (UNLIKELY(emailStructure == NULL))
	    {
	    mssError(0, "SMTP",
		"Failed to parse the email structure file: %s.",
		inf->EmailStructPath.String
	    );
	    goto end;
	    }

	/** Get the structure's attributes **/
	if (UNLIKELY(smtp_internal_GetStructAttributes(emailStructure, inf->Attributes, inf->AttributeNames) != 0))
	    {
	    mssError(0, "SMTP", "Failed to load email attributes.");
	    goto end;
	    }

	/** Update the send status of a Pending email. **/
	status = smtp_internal_GetString(inf->Attributes, "status");
	if (status != NULL && strcmp(status, "Pending") == 0 && smtp_internal_UpdateStatus(inf) != 0)
	    mssWarnError("Failed to update the send status of email \"%s\".", inf->Name);

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP",
		"Failed to open email: %s.",
		(inf->Name != NULL) ? inf->Name : "unknown name"
	    );

	if (UNLIKELY(fd != NULL)) fdClose(fd, 0);
	if (LIKELY(emailStructureFile != NULL)) fdClose(emailStructureFile, 0);
	if (LIKELY(emailStructure != NULL)) stFreeInf(emailStructure);

	return rval;
    }

/*** smtpOpen - open an object.
 ***/
void*
smtpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSmtpData inf = NULL;
    char* internalPath = NULL;

	/** Edge cases. **/
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(1, "SMTP", "Call to smtpOpen(NULL, ...);");
	    return NULL; /* Skip error handler, which expects a valid path. */
	    }
	ASSERTMAGIC(obj, MGK_OBJECT);

	/** Allocate driver struct. */
	inf = nmMalloc(sizeof(SmtpData));
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate SmtpData object.");
	    goto error;
	    }
	memset(inf, 0, sizeof(SmtpData));
	SETMAGIC(inf, MGK_SMTP_DATA);
	inf->Mask = mask;
	inf->Obj = obj;
	if (UNLIKELY(xsInit(&inf->EmailPath) != 0
	    || xsInit(&inf->EmailStructPath) != 0
	    || xsInit(&inf->ResultPath) != 0
	))   {
	    mssError(1, "SMTP", "Failed to init xstring.");
	    goto error;
	    }

	/** Calculate the path of the object relative to the root node. **/
	internalPath = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->SubPtr - 1, 2);
	if (UNLIKELY(internalPath == NULL)) goto error;

	/** Determine the type of the object. **/
	if (inf->Obj->SubPtr == inf->Obj->Pathname->nElements)
	    {
	    /** Open the SMTP node object itself. **/
	    inf->Obj->SubCnt = 1;
	    if (smtp_internal_OpenRoot(inf, usrtype) < 0)
		goto error;
	    }
	else if (smtp_internal_IsEmail(internalPath) ||
		(inf->Obj->Mode & OBJ_O_AUTONAME &&
		strcmp(internalPath + strlen(internalPath) - 2, "/*") == 0))
	    {
	    /** Open an email message, managed by the SMTP object. **/
	    inf->Obj->SubCnt = 2;
	    if (smtp_internal_OpenEml(inf, usrtype) < 0)
		goto error;
	    }
	else
	    {
	    mssError(1, "SMTP",
		"Failed to open \"%s\": expected an email file (.eml or .msg).",
		internalPath
	    );
	    goto error;
	    }

	/** Correct the pathname. **/
	objResetPathname(obj);

	return inf;

    error:
	mssError(0, "SMTP", "Failed to open smtp file: %s", objFilePath(obj));

	if (inf != NULL) smtp_internal_Close(inf);

	return NULL;
    }


/*** smtp_internal_Close() - close up.
 ***/
int
smtp_internal_Close(pSmtpData inf)
    {
    pObject obj = NULL;
    int rval = 0;

	if (UNLIKELY(inf == NULL))
	    return -1;
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	obj = inf->Obj; /* Save obj for error messages. */

	/** Check if the object is the root node. **/
	if (inf->AttributeNames)
	    {
	    if (UNLIKELY(xaFree(inf->AttributeNames) != 0))
		{
		mssError(1, "SMTP", "Failed to free attribute names.");
		rval = -1;
		}
	    }

	if (inf->Attributes)
	    {
	    if (UNLIKELY(smtp_internal_FreeAttributes(inf->Attributes) != 0))
		rval = -1;
	    }

	if (inf->RootAttributes)
	    {
	    if (UNLIKELY(smtp_internal_FreeAttributes(inf->RootAttributes) != 0))
		rval = -1;
	    }

	if (inf->ContentFile)
	    {
	    if (UNLIKELY(fdClose(inf->ContentFile, 0) != 0))
		{
		mssError(1, "SMTP", "Unable to close email file (%s).", inf->EmailPath.String);
		rval = -1;
		}
	    }

	if (inf->Name)
	    {
	    nmSysFree(inf->Name);
	    }

	/** We're closing the object... let the world know. **/
	if (inf->Node)
	    {
	    ASSERTMAGIC(inf->Node, MGK_STNODE);
	    inf->Node->OpenCnt--;
	    }

	if (UNLIKELY(xsDeInit(&inf->EmailPath) != 0
	    || xsDeInit(&inf->EmailStructPath) != 0
	    || xsDeInit(&inf->ResultPath) != 0
	))   {
	    mssError(1, "SMTP", "Failed to deinit xstring.");
	    rval = -1;
	    }
	nmFree(inf, sizeof(SmtpData));

	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to close smtp object in: %s", objFilePath(obj));

	return rval;
    }


/*** smtpClose - close an open object.
 ***/
int
smtpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to close NULL smtp object.");
	    return -1;
	    }

    return smtp_internal_Close(inf);
    }


/*** smtpCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
smtpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSnNode node = NULL;
    pSmtpData inf;

	/** Edge cases. **/
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(1, "SMTP", "Call to smtpCreate(NULL, ...);");
	    return -1; /* Skip error handler, which expects a valid path. */
	    }
	ASSERTMAGIC(obj, MGK_OBJECT);

	/** Determine the type of the object. **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    node = snReadNode(obj);
	    if (UNLIKELY(node != NULL))
		{
		mssError(1, "SMTP", "Unable to create root node because it already exists.");
		goto error;
		}

	    node = smtp_internal_CreateRootNode(obj, mask);
	    if (UNLIKELY(node == NULL))
		{
		mssError(0, "SMTP", "Unable to create root node.");
		goto error;
		}
	    }
	else if (obj->SubPtr+1 == obj->Pathname->nElements &&
		smtp_internal_IsEmail(obj->Pathname->Pathbuf))
	    {
	    /** Untested, but theoretically working... right? **/
	    inf = smtpOpen(obj, mask, systype, usrtype, oxt);
	    if (UNLIKELY(inf == NULL))
		goto error;
	    if (UNLIKELY(smtpClose(inf, oxt) != 0))
		goto error;
	    return 0;
	    }
	else
	    {
	    char* path = objResetPathname(obj);
	    mssError(1, "SMTP",
		"Failed to create \"%s\": expected an email file (.eml or .msg).",
		(path != NULL) ? path : "unknown path"
	    );
	    goto error;
	    }

	return 0;

    error:
	mssError(0, "SMTP", "Failed to create smtp object in: %s", objFilePath(obj));
	return -1;
    }


/*** smtpDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
smtpDelete(pObject obj, pObjTrxTree* oxt)
    {
    pSmtpData inf = NULL;
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(1, "SMTP", "Call to smtpDelete(NULL, ...);");
	    return -1; /* Skip error handler, which expects a valid path. */
	    }
	ASSERTMAGIC(obj, MGK_OBJECT);

	/** Try to open it first. **/
	obj->Mode = O_RDWR;
	inf = (pSmtpData)smtpOpen(obj, 0, NULL, "", oxt);
	if (UNLIKELY(inf == NULL))
	    goto end;
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Determine the type of the object. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    mssError(1, "SMTP", "Not handling deleting root nodes.");
	    goto end;
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Delete the email's files. **/
	    if (UNLIKELY(smtp_internal_RemoveEmail(
		inf->EmailPath.String,
		inf->EmailStructPath.String,
		inf->ResultPath.String
	    ) != 0))
		goto end;
	    }
	else
	    {
	    mssError(1, "SMTP", "Failed to delete indicated object (unknown type %d).", inf->Type);
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (LIKELY(inf != NULL) && UNLIKELY(smtp_internal_Close(inf) != 0))
	    rval = -1;

	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to delete smtp object in: %s", objFilePath(obj));

	return rval;
    }


/*** smtpRead - Read from the SMTP object
 ***/
int
smtpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to read from NULL smtp object.");
	    return -1;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Read the contents of emails directly. **/
	if (UNLIKELY(inf->Type != SMTP_T_EML))
	    {
	    mssError(1, "SMTP", "Unable to read content from smtp object of type %d.", inf->Type);
	    return -1;
	    }

	/** Refuse reads from a write-only email. **/
	if (UNLIKELY((inf->Obj->Mode & O_ACCMODE) == O_WRONLY))
	    {
	    mssError(1, "SMTP", "Failed to read email that was opened write-only.");
	    return -1;
	    }

	rval = fdRead(inf->ContentFile, buffer, maxcnt, offset, flags);
	if (UNLIKELY(rval < 0))
	    mssErrorErrno(1, "SMTP",
		"Failed to read %d bytes at offset %d from email file (%s).",
		maxcnt, offset, inf->EmailPath.String
	    );

	return rval;
    }


/*** smtpWrite - Write to the SMTP object
 ***/
int
smtpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to write to NULL smtp object.");
	    return -1;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Write the contents of emails directly. **/
	if (UNLIKELY(inf->Type != SMTP_T_EML))
	    {
	    mssError(1, "SMTP", "Unable to write content to smtp object of type %d.", inf->Type);
	    return -1;
	    }

	/** Refuse writes to a read-only email. **/
	if (UNLIKELY((inf->Obj->Mode & O_ACCMODE) == O_RDONLY))
	    {
	    mssError(1, "SMTP", "Failed to write to email that was opened read-only.");
	    return -1;
	    }

	rval = fdWrite(inf->ContentFile, buffer, cnt, offset, flags);
	if (UNLIKELY(rval < 0))
	    mssErrorErrno(1, "SMTP",
		"Failed to write %d bytes at offset %d to email file (%s).",
		cnt, offset, inf->EmailPath.String
	    );

	return rval;
    }


/*** smtpOpenQuery - open a directory query.  This driver is pretty
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
smtpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpQueryData qy = NULL;
    pSmtpAttribute attr = NULL;
    char* spoolPath = NULL;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to open a query on NULL smtp object.");
	    return NULL; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Allocate the query object. **/
	qy = (pSmtpQueryData)nmMalloc(sizeof(SmtpQueryData));
	if (UNLIKELY(qy == NULL))
	    {
	    mssError(1,"SMTP","Unable to allocate query object");
	    goto error;
	    }
	memset(qy, 0, sizeof(SmtpQueryData));
	SETMAGIC(qy, MGK_SMTP_QUERY_DATA);

	qy->Data = inf;

	/** Construct the query for the root node. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Find and open the spool directory path. **/
	    attr = (pSmtpAttribute)xhLookup(inf->Attributes, "spool_dir");
	    ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);
	    if (UNLIKELY(attr == NULL))
		{
		mssError(1, "SMTP", "The SMTP node is missing the required 'spool_dir' attribute.");
		goto error;
		}
	    spoolPath = attr->Value.String;

	    /** Sweep the spool dir to clean up expired emails. **/
	    smtp_internal_SweepSpool(spoolPath);

	    qy->Directory = opendir(spoolPath);
	    if (UNLIKELY(qy->Directory == NULL))
		{
		mssErrorErrno(1, "SMTP",
		    "Failed to open spool directory (%s) for query",
		    spoolPath
		);
		goto error;
		}

	    return qy;
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    mssError(1, "SMTP", "Unable to query on system/smtp-message type objects");
	    goto error;
	    }

	mssError(1, "SMTP", "Invalid smtp object type %d.", inf->Type);

    error:
	mssError(0, "SMTP",
	    "Failed to open query on smtp file: %s",
	    objFilePath(inf->Obj)
	);

	if (qy != NULL) smtpQueryClose(qy, NULL);

	return NULL;
    }


/*** smtpQueryFetch - get the next directory entry as an open object.
 ***/
void*
smtpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pSmtpQueryData qy = SMTP_QY(qy_v);
    pSmtpData inf = NULL;
    struct dirent *mailEntry = NULL;

	/** Edge cases. **/
	if (UNLIKELY(qy == NULL))
	    {
	    mssError(1, "SMTP", "Failed to fetch from NULL query object.");
	    return NULL; /* Skip error handler, which expects a valid query. */
	    }
	ASSERTMAGIC(qy, MGK_SMTP_QUERY_DATA);
	if (UNLIKELY(qy->Data == NULL))
	    {
	    mssError(1, "SMTP", "Failed to fetch from query object with NULL smtp object.");
	    return NULL; /* Skip error handler, which expects a valid query. */
	    }
	ASSERTMAGIC(qy->Data, MGK_SMTP_DATA);
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(1, "SMTP", "Failed to fetch query result into NULL object.");
	    return NULL; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(obj, MGK_OBJECT);

	if (qy->Data->Type == SMTP_T_ROOT)
	    {
	    /** Infinite while loops are better than GOTOs... probably. **/
	    while (1)
		{
		errno = 0;
		mailEntry = readdir(qy->Directory);
		if (!mailEntry || smtp_internal_IsEmail(mailEntry->d_name))
		    {
		    break;
		    }
		}

	    if (!mailEntry)
		{
		if (UNLIKELY(errno != 0))
		    {
		    mssErrorErrno(1, "SMTP", "Failed to read the spool directory.");
		    goto error;
		    }

		/** End of query **/
		return NULL;
		}

	    if (UNLIKELY(obj_internal_AddToPath(obj->Pathname, mailEntry->d_name) < 0))
		{
		mssError(0, "SMTP", "Query result pathname exceeds internal limits");
		goto error;
		}
	    obj->Mode = mode;

	    inf = (pSmtpData)nmMalloc(sizeof(SmtpData));
	    if (UNLIKELY(inf == NULL))
		{
		mssError(1, "SMTP", "Unable to create smtp data object");
		goto error;
		}
	    memset(inf, 0, sizeof(SmtpData));
	    SETMAGIC(inf, MGK_SMTP_DATA);
	    inf->Obj = obj;
	    if (UNLIKELY(xsInit(&inf->EmailPath) != 0
		|| xsInit(&inf->EmailStructPath) != 0
		|| xsInit(&inf->ResultPath) != 0
	    ))   {
		mssError(1, "SMTP", "Failed to init xstring.");
		goto error;
		}

	    if (UNLIKELY(smtp_internal_OpenEml(inf, "system/smtp-message") < 0))
		goto error;
	    }
	else if (qy->Data->Type == SMTP_T_EML)
	    {
	    mssError(1, "SMTP", "Unable to query smtp-message data objects");
	    goto error;
	    }

	return inf;

    error:
	mssError(0, "SMTP",
	    "Failed to fetch query result (%s) from smtp file: %s",
	    (mailEntry != NULL) ? mailEntry->d_name : "none",
	    objFilePath(qy->Data->Obj)
	);

	if (inf != NULL) smtp_internal_Close(inf);

	return NULL;
    }


/*** smtpQueryClose - close the query.
 ***/
int
smtpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pSmtpQueryData qy = SMTP_QY(qy_v);
    int rval = 0;

	/** Edge cases. **/
	if (UNLIKELY(qy == NULL))
	    {
	    mssError(1, "SMTP", "Failed to close NULL query object.");
	    return -1;
	    }
	ASSERTMAGIC(qy, MGK_SMTP_QUERY_DATA);

	if (qy->Directory)
	    {
	    if (UNLIKELY(closedir(qy->Directory) != 0))
		{
		mssErrorErrno(1,"SMTP","Unable to close directory");
		rval = -1;
		}
	    }
	nmFree(qy, sizeof(SmtpQueryData));

	return rval;
    }


/*** smtpGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
smtpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pSmtpData inf = NULL;
    pSmtpAttribute attr = NULL;

	/** If the attribute does not exist, return no type. **/
	if (!inf_v)
	    {
	    return -1;
	    }

	inf = SMTP(inf_v);
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Default values all happen to be strings. **/
	if (strcmp(attrname, "name") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "content_type") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "outer_type") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "inner_type") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "annotation") == 0) return DATA_T_STRING;

	/** Get the type of the stored attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);
	if (attr)
	    {
	    return attr->Type;
	    }

    return -1;
    }


/*** smtpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
smtpGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pSmtpData inf = NULL;
    pSmtpAttribute attr = NULL;

	if (!inf_v)
	    {
	    mssError(1, "SMTP", "Cannot get attribute '%s' of a NULL object.", attrname);
	    return -1;
	    }

	inf = SMTP(inf_v);
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	if (strcmp(attrname, "name") == 0)
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1, "SMTP",
		    "Type mismatch getting attribute '%s' (should be a string)",
		    attrname
		);
		return -1;
		}
	    //val->String = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements-1, 0);
	    val->String = inf->Name;
	    return 0;
	    }

	/** inner_type is an alias for content_type **/
	if (strcmp(attrname,"inner_type") == 0 || strcmp(attrname, "content_type") == 0)
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1, "SMTP",
		    "Type mismatch getting attribute '%s' (should be string)",
		    attrname
		);
		return -1;
		}

	    if (inf->Type == SMTP_T_ROOT)
		{
		val->String = "system/void";
		}
	    else if (inf->Type == SMTP_T_EML)
		{
		val->String = "message/rfc822";
		}

	    return 0;
	    }

	/** outer_type is the driver's type for the object **/
	if (strcmp(attrname,"outer_type") == 0)
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1, "SMTP",
		    "Type mismatch getting attribute '%s' (should be string)",
		    attrname
		);
		return -1;
		}
	    if (inf->Type == SMTP_T_ROOT)
		{
		val->String = "system/smtp";
		}
	    else if (inf->Type == SMTP_T_EML)
		{
		val->String = "system/smtp-message";
		}

	    return 0;
	    }

	if (strcmp(attrname, "annotation") == 0)
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1, "SMTP",
		    "Type mismatch getting attribute '%s' (should be string)",
		    attrname
		);
		return -1;
		}
	    val->String = "";
	    return 0;
	    }

	/** Get the type of the stored attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);
	if (attr)
	    {
	    if (datatype != attr->Type)
		{
		mssError(1, "SMTP",
		    "Type mismatch getting attribute '%s' (requested %s, should be %s)",
		    attrname, objTypeToStr(datatype), objTypeToStr(attr->Type)
		);
		return -1;
		}
	    switch (attr->Type)
		{
		case DATA_T_INTEGER:
		    val->Integer = attr->Value.Integer;
		    break;

		case DATA_T_STRING:
		    val->String = attr->Value.String;
		    break;

		case DATA_T_DATETIME:
		    val->DateTime = attr->Value.DateTime;
		    break;

		default:
		    mssError(1, "SMTP",
			"Cannot get attribute '%s' of unsupported type %s.",
			attrname, objTypeToStr(attr->Type)
		    );
		    return -1;
		}
	    return 0;
	    }

    return 1; /* null if not there presently */
    }


/*** smtpGetNextAttr - get the next attribute name for this object.
 ***/
char*
smtpGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to get next attribute from NULL smtp object.");
	    return NULL;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	if (inf->CurAttr < inf->AttributeNames->nItems)
	    {
	    return (char*)inf->AttributeNames->Items[inf->CurAttr++];
	    }

    return NULL;
    }


/*** smtpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
smtpGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to get first attribute from NULL smtp object.");
	    return NULL;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	inf->CurAttr = 0;

    return smtpGetNextAttr(inf_v, oxt);
    }


/*** smtp_internal_SetAttrValue - sets the value of an attribute, including
 *** read-only attributes and emails opened read-only.  'val' must point to
 *** an appropriate data type.
 ***/
int
smtp_internal_SetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute attr = NULL;

    pSnNode rootNode = NULL;
    pStructInf attrStruct = NULL;

    pFile emlStructFileRead = NULL;
    pFile emlStructFileWrite = NULL;
    pStructInf emlStruct = NULL;

    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to set attribute '%s' on NULL smtp object.", attrname);
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf->Obj == NULL))
	    {
	    mssError(1, "SMTP",
		"Failed to set attribute '%s' on smtp object with NULL object.",
		attrname
	    );
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf->Obj, MGK_OBJECT);

	/** Get the requested attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	if (!attr)
	    {
	    /** Add the attribute if it is not found. **/
	    if (UNLIKELY(smtp_internal_AddAttr(inf, attrname, datatype, val, oxt) != 0))
		{
		mssError(0, "SMTP", "Unable to create the requested attribute object.");
		goto end;
		}

	    /** Get the newly created attribute. **/
	    attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	    if (UNLIKELY(attr == NULL))
		{
		mssError(1, "SMTP", "Unable to open requested attribute object.");
		goto end;
		}
	    }
	ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);

	/** Check the requested datatype. **/
	if (attr->Type != datatype)
	    {
	    mssError(1, "SMTP",
		"Attempt to assign invalid data type to attribute. (Assigning %s to %s)",
		objTypeToStr(datatype), objTypeToStr(attr->Type)
	    );
	    goto end;
	    }

	/** We don't yet support null values **/
	if (UNLIKELY(val == NULL))
	    {
	    mssError(1, "SMTP", "Error setting attribute %s to NULL (not supported).", attrname);
	    goto end;
	    }

	/** Store the data according to its data type. **/
	if (datatype == DATA_T_STRING)
	    {
	    if (attr->Value.String)
		nmSysFree(attr->Value.String);
	    attr->Value.String = nmSysStrdup(val->String);
	    if (UNLIKELY(attr->Value.String == NULL))
		{
		mssError(1, "SMTP", "Failed to copy value \"%s\".", val->String);
		goto end;
		}
	    }
	else if (datatype == DATA_T_INTEGER)
	    {
	    attr->Value.Integer = val->Integer;
	    }
	else if (datatype == DATA_T_DATETIME)
	    {
	    if (!attr->Value.DateTime)
		attr->Value.DateTime = nmMalloc(sizeof(DateTime));
	    if (UNLIKELY(attr->Value.DateTime == NULL))
		{
		mssError(1, "SMTP", "Failed to allocate %zu bytes for a date.", sizeof(DateTime));
		goto end;
		}
	    memcpy(attr->Value.DateTime, val->DateTime, sizeof(DateTime));
	    }
	else
	    {
	    mssError(1, "SMTP", "Unsupported data type %s.", objTypeToStr(datatype));
	    goto end;
	    }

	/** Store the attribute into the correct file. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Read the root node into the node structure. **/
	    rootNode = snReadNode(inf->Obj->Prev);
	    if (UNLIKELY(rootNode == NULL))
		{
		mssError(0, "SMTP", "Unable to open root node for writing");
		goto end;
		}
	    ASSERTMAGIC(rootNode, MGK_STNODE);

	    /** Set the attribute value in the root node. **/
	    attrStruct = stLookup(rootNode->Data, attrname);
	    if (UNLIKELY(attrStruct == NULL))
		{
		mssError(1, "SMTP", "Attribute not found in the root node.");
		goto end;
		}
	    if (UNLIKELY(stSetAttrValue(attrStruct, datatype, val, 0) != 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto end;
		}

	    /** Mark root node DIRTY so that it will be written. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node back to the OS tree. **/
	    if (UNLIKELY(snWriteNode(inf->Obj->Prev, rootNode) != 0))
		{
		mssError(0, "SMTP", "Unable to write data to the root node");
		goto end;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Open the email structure file. **/
	    emlStructFileRead = fdOpen(inf->EmailStructPath.String, O_RDONLY, inf->Mask);
	    if (UNLIKELY(emlStructFileRead == NULL))
		{
		mssErrorErrno(1, "SMTP",
		    "Failed to open email structure file (%s).",
		    inf->EmailStructPath.String
		);
		goto end;
		}

	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFileRead, 0);
	    if (UNLIKELY(emlStruct == NULL))
		{
		mssError(0, "SMTP",
		    "Failed to parse the email structure file: %s.",
		    inf->EmailStructPath.String
		);
		goto end;
		}

	    /** Set the given attribute value. **/
	    attrStruct = stLookup(emlStruct, attrname);
	    if (UNLIKELY(attrStruct == NULL))
		{
		mssError(1, "SMTP", "Attribute not found in the email structure file.");
		goto end;
		}
	    if (UNLIKELY(stSetAttrValue(attrStruct, datatype, val, 0) < 0))
		{
		mssError(1, "SMTP", "Unable to set attribute '%s'", attrname);
		goto end;
		}

	    /** Done reading. **/
	    if (emlStructFileRead)
		{
		fdClose(emlStructFileRead, 0);
		emlStructFileRead = NULL;
		}

	    /** Open a fd with trunc to get rid of the old stuff. **/
	    emlStructFileWrite = fdOpen(inf->EmailStructPath.String, O_WRONLY | O_TRUNC, inf->Mask);
	    if (UNLIKELY(emlStructFileWrite == NULL))
		{
		mssErrorErrno(1, "SMTP",
		    "Failed to open email structure file (%s) for writing.",
		    inf->EmailStructPath.String
		);
		goto end;
		}

	    /** Write changes to the email struct file. **/
	    if (UNLIKELY(stGenerateMsg(emlStructFileWrite, emlStruct, O_WRONLY | O_TRUNC | O_CREAT) != 0))
		{
		mssError(1, "SMTP",
		    "Unable to write the attribute to the email struct file: %s.",
		    inf->EmailStructPath.String
		);
		goto end;
		}

	    /** If the email is ready to send, send it. **/
	    if (strcmp(attrname, "is_ready") == 0 && val->Integer == 1)
		{
		/** Flush the struct file so sending can update it. **/
		fdClose(emlStructFileWrite, 0);
		emlStructFileWrite = NULL;

		if (UNLIKELY(smtp_internal_SendEmail(inf) < 0))
		    {
		    goto end;
		    }
		}
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP",
		"Failed to set attribute '%s' of \"%s\" in: %s",
		attrname, inf->Name, objFilePath(inf->Obj)
	    );

	/** Free appropriate memory and close appropriate files. **/
	if (UNLIKELY(emlStructFileRead != NULL)) fdClose(emlStructFileRead, 0);
	if (emlStructFileWrite != NULL) fdClose(emlStructFileWrite, 0);
	if (emlStruct != NULL) stFreeInf(emlStruct);

	return rval;
    }


/*** smtpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
smtpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);

	/** Refuse writes to read-only emails and attributes. **/
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf != NULL && inf->Type == SMTP_T_EML && inf->Obj != NULL
	    && (inf->Obj->Mode & O_ACCMODE) == O_RDONLY))
	    {
	    mssError(1, "SMTP",
		"Failed to set attribute '%s' of \"%s\": the email was opened read-only.",
		attrname, inf->Name
	    );
	    return -1;
	    }
	if (UNLIKELY(inf != NULL && inf->Type == SMTP_T_EML && smtp_internal_IsReadOnly(attrname)))
	    {
	    mssError(1, "SMTP",
		"Failed to set attribute '%s' of \"%s\": it is read-only.",
		attrname, inf->Name
	    );
	    return -1;
	    }

	/** Refuse to send an email that is already Pending. **/
	if (UNLIKELY(inf != NULL && inf->Type == SMTP_T_EML && strcmp(attrname, "is_ready") == 0
	    && datatype == DATA_T_INTEGER && val != NULL && val->Integer == 1))
	    {
	    char* status = smtp_internal_GetString(inf->Attributes, "status");
	    if (status != NULL && strcmp(status, "Pending") == 0)
		{
		mssError(1, "SMTP",
		    "Failed to send \"%s\": it is already Pending.",
		    inf->Name
		);
		return -1;
		}
	    }

    return smtp_internal_SetAttrValue(inf_v, attrname, datatype, val, oxt);
    }


/*** smtp_internal_AddAttr - add an attribute to an object, including
 *** read-only attributes and emails opened read-only.
 ***/
int
smtp_internal_AddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute attr = NULL;
    pSmtpAttribute unstoredAttr = NULL;
    pStructInf createdStruct = NULL;
    pSnNode rootNode = NULL;
    pFile emlStructFileRead = NULL;
    pFile emlStructFileWrite = NULL;
    pStructInf emlStruct = NULL;
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to add attribute '%s' to NULL smtp object.", attrname);
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf->Obj == NULL))
	    {
	    mssError(1, "SMTP",
		"Failed to add attribute '%s' to smtp object with NULL object.",
		attrname
	    );
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf->Obj, MGK_OBJECT);

	/** Initialize the new attribute. **/
	attr = nmMalloc(sizeof(SmtpAttribute));
	if (UNLIKELY(attr == NULL))
	    {
	    mssError(1,"SMTP","Failed to create new attribute object.");
	    goto end;
	    }
	memset(attr, 0, sizeof(SmtpAttribute));
	SETMAGIC(attr, MGK_SMTP_ATTRIBUTE);
	unstoredAttr = attr;

	/** Set the meta-data fields of the new attribute. **/
	attr->Name = nmSysStrdup(attrname);
	if (UNLIKELY(attr->Name == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy attribute name.");
	    goto end;
	    }
	attr->Type = type;

	/** Set the default value based on the type. **/
	switch (attr->Type)
	    {
	    case DATA_T_STRING:
		attr->Value.String = nmSysStrdup("");
		if (UNLIKELY(attr->Value.String == NULL))
		    {
		    mssError(1, "SMTP", "Failed to allocate an empty string value.");
		    goto end;
		    }
		break;

	    case DATA_T_INTEGER:
		attr->Value.Integer = 0;
		break;

	    case DATA_T_DATETIME:
		attr->Value.DateTime = nmMalloc(sizeof(DateTime));
		if (UNLIKELY(attr->Value.DateTime == NULL))
		    {
		    mssError(1, "SMTP",
			"Failed to allocate %zu bytes for a date.",
			sizeof(DateTime)
		    );
		    goto end;
		    }
		memset(attr->Value.DateTime, 0, sizeof(DateTime));
		break;

	    default:
		mssError(1, "SMTP",
		    "Cannot add attribute '%s' of unsupported type %s.",
		    attr->Name, objTypeToStr(attr->Type)
		);
		goto end;
	    }

	/** Add the attribute to the attribute hash and the attribute name list. **/
	if (UNLIKELY(xhAdd(inf->Attributes, attr->Name, (char*)attr) != 0))
	    {
	    mssError(1, "SMTP", "Failed to add attribute (it may be a duplicate).");
	    goto end;
	    }
	if (UNLIKELY(xaAddItem(inf->AttributeNames, attr->Name) < 0))
	    {
	    mssError(1, "SMTP", "Failed to add attribute name to list.");
	    xhRemove(inf->Attributes, attr->Name);
	    goto end;
	    }
	unstoredAttr = NULL;

	/** Create the attribute in the correct location according to object type. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Open the root node. **/
	    rootNode = snReadNode(inf->Obj->Prev);
	    if (UNLIKELY(rootNode == NULL))
		{
		mssError(0, "SMTP", "Unable to open root node.");
		goto end;
		}
	    ASSERTMAGIC(rootNode, MGK_STNODE);

	    /** Add the attribute to the root node. **/
	    createdStruct = stAddAttr(rootNode->Data, attr->Name);
	    if (UNLIKELY(createdStruct == NULL))
		{
		mssError(1, "SMTP", "Unable to add new attribute to the root node.");
		goto end;
		}

	    /** Set the default attribute value. **/
	    if (UNLIKELY(stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0) != 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto end;
		}

	    /** Set the root node to DIRTY so it will be written to the file. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node. **/
	    if (UNLIKELY(snWriteNode(inf->Obj->Prev, rootNode) != 0))
		{
		mssError(0, "SMTP", "Unable to write root node.");
		goto end;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Open the email structure file. **/
	    emlStructFileRead = fdOpen(inf->EmailStructPath.String, O_RDONLY, inf->Mask);
	    if (UNLIKELY(emlStructFileRead == NULL))
		{
		mssErrorErrno(1, "SMTP",
		    "Failed to open email structure file (%s).",
		    inf->EmailStructPath.String
		);
		goto end;
		}

	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFileRead, 0);
	    if (UNLIKELY(emlStruct == NULL))
		{
		mssError(0, "SMTP",
		    "Failed to parse the email structure file: %s.",
		    inf->EmailStructPath.String
		);
		goto end;
		}

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emlStruct, attr->Name);
	    if (UNLIKELY(createdStruct == NULL))
		{
		mssError(1, "SMTP",
		    "Failed to add attribute '%s' to the email struct.",
		    attr->Name
		);
		goto end;
		}

	    /** Set the attribute value. **/
	    if (UNLIKELY(stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0) < 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute '%s'", attr->Name);
		goto end;
		}

	    /** Done reading. **/
	    fdClose(emlStructFileRead, 0);
	    emlStructFileRead = NULL;

	    /** Open a fd with trunc to get rid of the old stuff. **/
	    emlStructFileWrite = fdOpen(inf->EmailStructPath.String, O_WRONLY | O_TRUNC, inf->Mask);
	    if (UNLIKELY(emlStructFileWrite == NULL))
		{
		mssErrorErrno(1, "SMTP",
		    "Failed to open email structure file (%s) for writing.",
		    inf->EmailStructPath.String
		);
		goto end;
		}

	    /** Write changes to the email struct file. **/
	    if (UNLIKELY(stGenerateMsg(emlStructFileWrite, emlStruct, O_WRONLY | O_TRUNC | O_CREAT) < 0))
		{
		mssError(1, "SMTP",
		    "Unable to write the updated email struct file: %s.",
		    inf->EmailStructPath.String
		);
		goto end;
		}
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP",
		"Failed to add attribute '%s' to \"%s\" in: %s",
		attrname, inf->Name, objFilePath(inf->Obj)
	    );

	/** Free appropriate memory and close appropriate files. **/
	if (UNLIKELY(unstoredAttr != NULL)) smtp_internal_ClearAttribute((char*)unstoredAttr, NULL);
	if (emlStructFileRead != NULL) fdClose(emlStructFileRead, 0);
	if (emlStructFileWrite != NULL) fdClose(emlStructFileWrite, 0);
	if (emlStruct != NULL) stFreeInf(emlStruct);

	return rval;
    }


/*** smtpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
smtpAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);

	/** Refuse to add attributes to read-only emails, or read-only attributes. **/
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf != NULL && inf->Type == SMTP_T_EML && inf->Obj != NULL
	    && (inf->Obj->Mode & O_ACCMODE) == O_RDONLY))
	    {
	    mssError(1, "SMTP",
		"Failed to add attribute '%s' to \"%s\": the email was opened read-only.",
		attrname, inf->Name
	    );
	    return -1;
	    }
	if (UNLIKELY(inf != NULL && inf->Type == SMTP_T_EML && smtp_internal_IsReadOnly(attrname)))
	    {
	    mssError(1, "SMTP",
		"Failed to add attribute '%s' to \"%s\": it is read-only.",
		attrname, inf->Name
	    );
	    return -1;
	    }

    return smtp_internal_AddAttr(inf_v, attrname, type, val, oxt);
    }


/*** smtpOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
smtpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** smtpGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
smtpGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** smtpGetNextMethod -- same as above.  Always fails.
 ***/
char*
smtpGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** smtpExecuteMethod - No methods to execute, so this fails.
 ***/
int
smtpExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** smtpInfo - Return the capabilities of the object
 ***/
int
smtpInfo(void* inf_v, pObjectInfo info)
    {
    return 0;
    }


/*** smtpInitialize - initialize this driver, which also causes it to
 *** register itself with the objectsystem.
 ***/
int
smtpInitialize()
    {
    pObjDriver drv = NULL;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (UNLIKELY(drv == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate %zu bytes for the driver.", sizeof(ObjDriver));
	    goto error;
	    }
	memset(drv, 0, sizeof(ObjDriver));

	/** If globals are not yet initialized, initialize them.			**/
	/** We don't always need globals, but when we do, they should be initialized.	**/
	/** jk. They are the globals we deserve, but not the ones we need right now.	**/
	/** jk. We need Batman. And globals.						**/
	if (UNLIKELY(smtp_internal_InitGlobals() != 0))
	    goto error;

	/** Setup the structure **/
	strcpy(drv->Name,"SMTP - Simple Mail Transfer Protocol OS Driver");
	drv->Capabilities = 0;
	if (UNLIKELY(xaInit(&(drv->RootContentTypes),1) != 0
	    || xaAddItem(&(drv->RootContentTypes),"system/smtp") < 0
	))   {
	    mssError(1, "SMTP", "Failed to set up root content types.");
	    goto error;
	    }

	/** Setup the function references. **/
	drv->Open = smtpOpen;
	drv->Close = smtpClose;
	drv->Create = smtpCreate;
	drv->Delete = smtpDelete;
	drv->OpenQuery = smtpOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = smtpQueryFetch;
	drv->QueryClose = smtpQueryClose;
	drv->Read = smtpRead;
	drv->Write = smtpWrite;
	drv->GetAttrType = smtpGetAttrType;
	drv->GetAttrValue = smtpGetAttrValue;
	drv->GetFirstAttr = smtpGetFirstAttr;
	drv->GetNextAttr = smtpGetNextAttr;
	drv->SetAttrValue = smtpSetAttrValue;
	drv->AddAttr = smtpAddAttr;
	drv->OpenAttr = smtpOpenAttr;
	drv->GetFirstMethod = smtpGetFirstMethod;
	drv->GetNextMethod = smtpGetNextMethod;
	drv->ExecuteMethod = smtpExecuteMethod;
	drv->PresentationHints = NULL;
	drv->Info = smtpInfo;

	/** Register structs for debugging. **/
	nmRegister(sizeof(SmtpAttribute), "SmtpAttribute");
	nmRegister(sizeof(SmtpData), "SmtpData");
	nmRegister(sizeof(SmtpQueryData), "SmtpQueryData");
	nmRegister(sizeof(SmtpSpool), "SmtpSpool");
	nmRegister(sizeof(SmtpLogMsg), "SmtpLogMsg");
	nmRegister(sizeof(SmtpLogRcpt), "SmtpLogRcpt");

	/** Register the driver **/
	if (UNLIKELY(objRegisterDriver(drv) < 0))
	    {
	    mssError(0, "SMTP", "Failed to register the driver.");
	    goto error;
	    }

	return 0;

    error:
	mssError(0, "SMTP", "Failed to initialize the SMTP driver.");

	if (drv != NULL) nmFree(drv, sizeof(ObjDriver));

	return -1;
    }

MODULE_INIT(smtpInitialize);
MODULE_PREFIX("smtp");
MODULE_DESC("SMTP ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);
