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
/************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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


/** Define whether to use debugging mode. **/
#define	SMTP_DEBUG	0

/** Define attribute defaults. **/
#define SMTP_DEFAULT_LOG_PATH		"/var/log/maillog"
#define SMTP_DEFAULT_EXPIRE_TIME	(3 * 24 * 60 * 60)
#define SMTP_DEFAULT_LOG_READ_INTERVAL	(60 * 60)
#define SMTP_DEFAULT_SWEEP_INTERVAL	(60 * 60)

/** Define file names. **/
#define SMTP_CURSOR_FILE        ".mail_log_cursor" /* Stores a spool dir's mail log position. */
#define SMTP_LOCK_FILE          ".spool_lock"      /* Locks a spool dir and alerts when pending emails update. */

/** Define sizes. **/
#define SMTP_RESULT_HEADER_LEN	 16                /* Bytes in the status line at the start of a sendmail result file. */
#define SMTP_TRY_MSG_MAX	 1024              /* Bytes of sendmail output kept in last_try_msg. */
#define SMTP_QUEUE_ID_SIZE	 32                /* Bytes for a Postfix queue ID (including the NUL-terminator). */
#define SMTP_LOG_READ_SIZE	(64 * 1024)        /* Bytes of the mail log to read before yielding. */
#define SMTP_SERIAL_LEN		 21                /* Bytes of the serial count in the lock file, including the newline. */

/** Define timeouts & intervals. **/
#define SMTP_LOCK_POLL_INTERVAL	 50                /* Milliseconds between tries to get a spool dir lock. */
#define SMTP_LOCK_TIMEOUT	 60                /* Seconds to wait for a spool dir lock before failing. */
#define SMTP_PENDING_TIMEOUT	(6 * 24 * 60 * 60) /* Seconds to wait before a Pending email times out to Error. */
#define SMTP_SENDMAIL_TIMEOUT	 60                /* Seconds to wait for sendmail before killing it. */
#define SMTP_RESULT_TIMEOUT	(2 * SMTP_SENDMAIL_TIMEOUT) /* Seconds a Pending email waits for the sendmail supervisor
							     * or Postfix Queue ID before timing out to Error. */

/** Define the group type of a recipient result in an email struct. **/
#define SMTP_RCPT_TYPE		"system/smtp-recipient"

/** Define the types of recipient results read from the mail log. **/
#define SMTP_RCPT_SENT		0
#define SMTP_RCPT_DEFERRED	1
#define SMTP_RCPT_BOUNCED	2

/** Define the mail log line types. **/
#define SMTP_LINE_NONE		0	/* No result. */
#define SMTP_LINE_QUEUED	1	/* message-id=<id> */
#define SMTP_LINE_RCPT_COUNT	2	/* from=<addr>, size=N, nrcpt=N */
#define SMTP_LINE_EXPIRED	3	/* from=<addr>, status=expired */
#define SMTP_LINE_RCPT		4	/* to=<addr>, ..., status=<status> (reply) */

/** Define SMTP driver object types. **/
#define SMTP_T_ROOT	0
#define SMTP_T_EML	1


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
    struct stat		StructInfo; /* The struct file that Attributes were loaded from. */
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


/*** Structure to track a spool directory. ***/
typedef struct
    {
    Magic_t	Magic;
    char*	Path;
    time_t	LastSweep;
    time_t	LastRead;	/* When the last read of the mail log started. */
    bool	Loaded;		/* Log position and indexes below are in memory. */
    bool	HasCursor;	/* A log position is known from a prior read. */
    dev_t	LogDev;		/* Device of the file that the cursor points into. */
    ino_t	LogIno;		/* Inode of the file that the cursor points into. */
    off_t	LogOffset;	/* Position of the cursor in its file. */
    XHashTable	ByMessageID;	/* Hash (Message-ID -> pSmtpIndexEntry): For Pending emails without a queue ID. */
    XHashTable	ByQueueID;	/* Hash (Postfix Queue ID -> pSmtpIndexEntry): For Pending emails with a queue ID. */
    unsigned long long Serial;	/* The serial of the last change to the Pending emails that this process knows of. */
    int		LockFd;		/* The open lock file, or -1. */
    pSemaphore	LockSem;	/* Held by the thread that has the spool directory locked. */
    pThread	LockOwner;	/* The thread that has the spool directory locked, or NULL. */
    int		LockDepth;	/* Nesting depth of smtp_internal_LockSpool() in LockOwner. */
    }
    SmtpSpool, *pSmtpSpool;


/*** Structure to find a Pending email in a spool directory. ***/
typedef struct
    {
    Magic_t	Magic;
    char*	Key;	/* The Message-ID or queue ID. */
    char*	Name;	/* The email file name. */
    }
    SmtpIndexEntry, *pSmtpIndexEntry;


/*** Structure to hold an email updated from the mail log until it is written. ***/
typedef struct
    {
    Magic_t	Magic;
    char*	Name;		/* The email file name. */
    pStructInf	Struct;		/* The parsed struct file. */
    bool	Changed;	/* A mail log line updated the struct. */
    bool	Expired;	/* Postfix gave up and returned the email. */
    }
    SmtpLogEmail, *pSmtpLogEmail;


/*** Structure to hold the lines and emails of one read of the mail log. ***/
typedef struct
    {
    XArray	Lines;		/* XArray of copies of the lines about Pending emails. */
    XHashTable	QueueIDs;	/* Hash of queue ID to its copy, for queue IDs claimed by those lines. */
    XHashTable	ByName;		/* Hash of email file name to pSmtpLogEmail. */
    XArray	Emails;		/* XArray of pSmtpLogEmail, in the order they were read. */
    }
    SmtpLogBatch, *pSmtpLogBatch;


/*** Structure for a rotated mail log that a spool directory still needs to read. ***/
typedef struct
    {
    char*	Path;
    struct timespec MTime;	/* When it was last written, before it was rotated. */
    bool	IsCursor;	/* The spool directory's cursor is in this log. */
    }
    SmtpRotatedLog, *pSmtpRotatedLog;


/*** Structure for the parts of one mail log line. ***/
typedef struct
    {
    int		Kind;		/* SMTP_LINE_xxx */
    char*	QueueID;
    char*	MessageID;	/* SMTP_LINE_QUEUED, without angle brackets. */
    int		RcptCount;	/* SMTP_LINE_RCPT_COUNT */
    char*	Address;	/* SMTP_LINE_RCPT */
    int		Status;		/* SMTP_LINE_RCPT, as SMTP_RCPT_xxx (see below). */
    char*	Reply;		/* SMTP_LINE_RCPT, without parentheses. */
    }
    SmtpLogLine, *pSmtpLogLine;

/** Recipient results as stored in an email struct, indexed by SMTP_RCPT_xxx. **/
static char* smtp_rcpt_status_names[] = { "sent", "deferred", "bounced" };


/*** Global data structure for the SMTP module. ***/
struct
    {
    XArray		DefaultRootAttributes;		/* XArray of pSmtpAttribute */
    XArray		DefaultEmailAttributes;		/* XArray of pSmtpAttribute */
    XHashTable		Spools;				/* Hash of spool_dir to pSmtpSpool */
    char		LogPath[PATH_MAX];		/* Path of the mail log */
    }
    SMTP_INF;


/** Forward declarations for functions that need them. **/
int smtp_internal_Close(pSmtpData inf);
int smtp_internal_UpdateFromLog(char* spoolDir, pXHashTable rootAttributes, bool throttle);
void smtp_internal_UnloadSpool(pSmtpSpool spool);
void smtp_internal_UnlockSpool(pSmtpSpool spool);
int smtp_internal_RefreshStatus(pStructInf emailStruct, char* resultPath, bool expired, pXHashTable rootAttributes, bool* changed);
int smtp_internal_ReadResult(char* resultPath, char* header, pXString output);
int smtpQueryClose(void* qy_v, pObjTrxTree* oxt);
int smtp_internal_AddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt);
int smtp_internal_SetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt);


/*** smtp_internal_SpawnSendmail - launch the sendmail process to actually
 *** send off an email message.  This also works with Postfix, via its
 *** "sendmail compatibility interface".
 ***
 *** This function also spawns a detached supervisor process that waits for
 *** sendmail and writes the result to resultPath, starting with a status line
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
    struct stat emailStat;
    char result[SMTP_RESULT_HEADER_LEN];
    char header[SMTP_RESULT_HEADER_LEN + 1];
    struct timespec pollInterval = {0, 100 * 1000 * 1000};
    int polls;
    int exitStatus = EXIT_SUCCESS;
    int found;
    XString output;
    bool outputInitialized = false;
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

	/** Give the result file the permissions of the email, without execute. **/
	if (UNLIKELY(stat(emailPath, &emailStat) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to check email file (%s).", emailPath);
	    goto end;
	    }

	/** Create the result file, which the supervisor renames when done. **/
	if (UNLIKELY(snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", resultPath) >= (int)sizeof(tmpPath)))
	    {
	    mssError(1, "SMTP", "Sendmail result file path is too long: \"%s.tmp\".", resultPath);
	    goto end;
	    }
	resultFd = open(tmpPath, O_WRONLY | O_CREAT | O_TRUNC, emailStat.st_mode & 0666);
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
	 *** not call mssError().  Thus, they write to the result file, or to
	 *** stderr when that is not possible.
	 ***/
	pid = fork();
	if (UNLIKELY(pid < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to fork (1).");
	    goto end;
	    }
	if (pid == 0)
	    {
	    /** we're in the child process -- disable MTask context switches to be safe **/
	    thLock();

	    /** close all open fds (except for 0-2 -- std{in,out,err} -- and the result file) **/
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
		dprintf(resultFd,
		    "Failed to open email file (%s) for sendmail. (%s)",
		    emailPath, strerror(errno)
		);
		_exit(EXIT_FAILURE);
		}

	    /** Make the email file stdin for sendmail. **/
	    if (UNLIKELY(dup2(fd, 0) < 0))
		{
		dprintf(resultFd,
		    "Failed to redirect email file (%s) to stdin for sendmail. (%s)",
		    emailPath, strerror(errno)
		);
		_exit(EXIT_FAILURE);
		}

	    /** NOTE: We're currently double forking to get rid of zombie processes. **/
	    pid = fork();
	    if (UNLIKELY(pid < 0))
		{
		dprintf(resultFd, "Failed to fork (2). (%s)", strerror(errno));
		_exit(EXIT_FAILURE);
		}
	    if (pid == 0)
		{
		/** we're in the supervisor process -- disable MTask context switches to be safe **/
		thLock();

		/** close all open fds (except for 0-2 -- std{in,out,err} -- and the result file) **/
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
		    dprintf(resultFd, "SMTP: Failed to fork (3). (%s)\n", strerror(errno));
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

		/** Write the status line.  A blank one still publishes as an unknown result. **/
		snprintf(header, sizeof(header), "%-*s\n", SMTP_RESULT_HEADER_LEN - 1, result);
		if (UNLIKELY(pwrite(resultFd, header, SMTP_RESULT_HEADER_LEN, 0) != SMTP_RESULT_HEADER_LEN))
		    {
		    fprintf(stderr,
			"SMTP: Failed to write status line \"%s\" to sendmail result file (%s). (%s)\n",
			result, tmpPath, strerror(errno)
		    );
		    exitStatus = EXIT_FAILURE;
		    }
		if (UNLIKELY(close(resultFd) != 0))
		    {
		    fprintf(stderr,
			"SMTP: Failed to close sendmail result file (%s). (%s)\n",
			tmpPath, strerror(errno)
		    );
		    exitStatus = EXIT_FAILURE;
		    }

		/** Publish the result, even after a failure, so it reaches the parent. **/
		if (UNLIKELY(rename(tmpPath, resultPath) != 0))
		    {
		    fprintf(stderr,
			"SMTP: Failed to publish sendmail result file (%s) as (%s). (%s)\n",
			tmpPath, resultPath, strerror(errno)
		    );
		    exitStatus = EXIT_FAILURE;
		    }
		_exit(exitStatus);
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
	    /** Report the reason the launcher wrote to the result file. **/
	    if (UNLIKELY(xsInit(&output) != 0))
		{
		mssError(1, "SMTP", "Failed to initialize the sendmail launcher output string.");
		goto end;
		}
	    outputInitialized = true;
	    found = smtp_internal_ReadResult(tmpPath, header, &output);
	    if (found == 1 && output.Length > 0)
		mssError(1, "SMTP",
		    "Sendmail launcher process (pid %d) exited with status %d: %s.",
		    pid, WEXITSTATUS(wstatus), output.String
		);
	    else
		mssError((found < 0) ? 0 : 1, "SMTP",
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
	if (outputInitialized) xsDeInit(&output);

	return rval;
    }


/*** smtp_internal_ClearAttribute - Frees an attribute.  Also used as the
 *** xhClear() callback for attributes hash tables.
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


/*** smtp_internal_SpoolPath - build the path of a file of an email in a
 *** spool directory, by replacing the extension of the email name.
 ***
 *** @param path Set to the path.  Must hold PATH_MAX bytes.
 *** @param spoolDir The spool directory.
 *** @param name The email file name, such as "x.eml".
 *** @param ext The extension of the file, such as ".struct".
 *** @returns 0 on success, or -1 if the path is too long.
 ***/
int
smtp_internal_SpoolPath(char* path, char* spoolDir, char* name, char* ext)
    {
    int nameLen = strlen(name) - 4; /* Without ".eml" or ".msg". */

	if (UNLIKELY(nameLen < 0 || snprintf(path, PATH_MAX, "%s/%.*s%s", spoolDir, nameLen, name, ext) >= PATH_MAX))
	    {
	    mssError(1, "SMTP", "Failed to build the %s path of email \"%s\" in \"%s\".", ext, name, spoolDir);
	    return -1;
	    }

    return 0;
    }


/*** smtp_internal_ReadStruct - parse an email struct file.
 ***
 *** @param path The path of the struct file.
 *** @param emailStruct Set to the parsed struct, which the caller frees, or
 ***   NULL if there is none.
 *** @returns 1 on success, 0 if the file does not exist, or -1 on failure.
 ***/
int
smtp_internal_ReadStruct(char* path, pStructInf* emailStruct)
    {
    pFile structFile;

	*emailStruct = NULL;
	structFile = fdOpen(path, O_RDONLY, 0);
	if (structFile == NULL)
	    {
	    if (errno == ENOENT)
		return 0;
	    mssErrorErrno(1, "SMTP", "Failed to open email struct file \"%s\".", path);
	    return -1;
	    }
	*emailStruct = stParseMsg(structFile, 0);
	fdClose(structFile, 0);
	if (UNLIKELY(*emailStruct == NULL))
	    {
	    mssError(0, "SMTP", "Failed to parse email struct file \"%s\".", path);
	    return -1;
	    }

    return 1;
    }


/*** smtp_internal_WriteStruct - replace an existing email struct file.  The
 *** struct is written to "<path>.tmp", which is then renamed over the old
 *** file, so a crash leaves either the old struct or the new one.
 ***
 *** @param path The path of the struct file.
 *** @param emailStruct The struct to write.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_WriteStruct(char* path, pStructInf emailStruct)
    {
    char tmpPath[PATH_MAX];
    struct stat st;
    pFile structFile = NULL;
    bool tmpCreated = false;
    int rval = -1;

	/** Build the temporary path. **/
	if (UNLIKELY(snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path) >= (int)sizeof(tmpPath)))
	    {
	    mssError(1, "SMTP", "Failed to build the temporary path of email struct file: \"%s.tmp\" is too long.", path);
	    goto end;
	    }

	/** Get the permissions of the old file. **/
	if (UNLIKELY(stat(path, &st) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to check email struct file \"%s\".", path);
	    goto end;
	    }

	/** Write the new struct. **/
	structFile = fdOpen(tmpPath, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 07777);
	if (UNLIKELY(structFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to create email struct file \"%s\".", tmpPath);
	    goto end;
	    }
	tmpCreated = true;
	if (UNLIKELY(stGenerateMsg(structFile, emailStruct, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to write email struct file \"%s\".", tmpPath);
	    goto end;
	    }
	if (UNLIKELY(fdClose(structFile, 0) != 0))
	    {
	    structFile = NULL;
	    mssErrorErrno(1, "SMTP", "Failed to close email struct file \"%s\".", tmpPath);
	    goto end;
	    }
	structFile = NULL;

	/** Replace the old struct. **/
	if (UNLIKELY(rename(tmpPath, path) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to replace email struct file \"%s\".", path);
	    goto end;
	    }
	tmpCreated = false;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(structFile != NULL)) fdClose(structFile, 0);
	if (UNLIKELY(tmpCreated && remove(tmpPath) != 0))
	    fprintf(stderr,
		"Warning: Failed to remove partial email struct file (%s): %s.\n",
		tmpPath, strerror(errno)
	    );

	return rval;
    }


/*** smtp_internal_StructString - get a string attribute of a struct.
 ***
 *** @param inf The struct.
 *** @param name The attribute name.
 *** @returns The value, or NULL if the attribute is missing or not a string.
 ***/
char*
smtp_internal_StructString(pStructInf inf, char* name)
    {
    char* value = NULL;

	if (stAttrValue(stLookup(inf, name), NULL, &value, 0) != 0)
	    return NULL;

    return value;
    }


/*** smtp_internal_StructInteger - get an integer attribute of a struct.
 ***
 *** @param inf The struct.
 *** @param name The attribute name.
 *** @returns The value, or 0 if the attribute is missing or not an integer.
 ***/
int
smtp_internal_StructInteger(pStructInf inf, char* name)
    {
    int value = 0;

	if (stAttrValue(stLookup(inf, name), &value, NULL, 0) != 0)
	    return 0;

    return value;
    }


/*** smtp_internal_StructSet - set an attribute of a struct, adding it if
 *** it is missing.
 ***
 *** @param inf The struct.
 *** @param name The attribute name.
 *** @param type The type of the value (DATA_T_xxx).
 *** @param val The value.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_StructSet(pStructInf inf, char* name, int type, pObjData val)
    {
    pStructInf attr = stLookup(inf, name);

	if (attr == NULL)
	    attr = stAddAttr(inf, name);
	if (UNLIKELY(attr == NULL || stSetAttrValue(attr, type, val, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to set attribute '%s' in an email struct.", name);
	    return -1;
	    }

    return 0;
    }


/*** smtp_internal_IsRcpt - check whether part of an email struct is a
 *** recipient result.
 ***
 *** @param inf The part of the struct.
 *** @returns true if it is a recipient result, false otherwise.
 ***/
bool
smtp_internal_IsRcpt(pStructInf inf)
    {
    return stStructType(inf) == ST_T_SUBGROUP
	&& inf->UsrType != NULL
	&& strcmp(inf->UsrType, SMTP_RCPT_TYPE) == 0;
    }


/*** smtp_internal_SetRcpt - record the latest result for one recipient in
 *** an email struct.
 ***
 *** @param emailStruct The email struct.
 *** @param address The recipient address.
 *** @param status The result (SMTP_RCPT_xxx).
 *** @param reply The reply or reason Postfix logged.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_SetRcpt(pStructInf emailStruct, char* address, int status, char* reply)
    {
    pStructInf rcpt = NULL;
    pStructInf part;
    char* rcptAddress;
    char name[32];
    ObjData pod;
    int nRcpts = 0;
    int i;

	/** Find the recipient. **/
	for (i = 0; i < emailStruct->nSubInf; i++)
	    {
	    part = emailStruct->SubInf[i];
	    if (!smtp_internal_IsRcpt(part))
		continue;
	    nRcpts++;
	    rcptAddress = smtp_internal_StructString(part, "address");
	    if (rcptAddress != NULL && strcmp(rcptAddress, address) == 0)
		{
		rcpt = part;
		break;
		}
	    }

	/** Recipient not found: add it. **/
	if (rcpt == NULL)
	    {
	    snprintf(name, sizeof(name), "rcpt_%d", nRcpts + 1);
	    rcpt = stAddGroup(emailStruct, name, SMTP_RCPT_TYPE);
	    if (UNLIKELY(rcpt == NULL))
		{
		mssError(1, "SMTP", "Failed to add recipient <%s> to an email struct.", address);
		return -1;
		}
	    pod.String = address;
	    if (UNLIKELY(smtp_internal_StructSet(rcpt, "address", DATA_T_STRING, &pod) != 0))
		return -1;
	    }

	/** Record the result. **/
	pod.String = smtp_rcpt_status_names[status];
	if (UNLIKELY(smtp_internal_StructSet(rcpt, "status", DATA_T_STRING, &pod) != 0))
	    return -1;
	pod.String = reply;
	if (UNLIKELY(smtp_internal_StructSet(rcpt, "reply", DATA_T_STRING, &pod) != 0))
	    return -1;

    return 0;
    }


/*** smtp_internal_ClearRcpts - remove the recipient results from the
 *** struct file of an email in a locked spool directory.
 ***
 *** @param structPath The path of the struct file.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_ClearRcpts(char* structPath)
    {
    pStructInf emailStruct = NULL;
    bool removed = false;
    int found;
    int i;
    int rval = -1;

	/** Read the struct. **/
	found = smtp_internal_ReadStruct(structPath, &emailStruct);
	if (UNLIKELY(found == 0))
	    mssError(1, "SMTP", "Failed to clear the recipient results of missing email struct \"%s\".", structPath);
	if (UNLIKELY(found != 1))
	    goto end;

	/** Remove the recipient results. **/
	for (i = emailStruct->nSubInf - 1; i >= 0; i--)
	    {
	    if (!smtp_internal_IsRcpt(emailStruct->SubInf[i]))
		continue;
	    stRemoveInf(emailStruct->SubInf[i]);
	    removed = true;
	    }

	/** Write the struct, if it changed. **/
	if (removed && UNLIKELY(smtp_internal_WriteStruct(structPath, emailStruct) != 0))
	    goto end;

	/** Success. **/
	rval = 0;

    end:
	if (emailStruct != NULL) stFreeInf(emailStruct);

	return rval;
    }


/*** smtp_internal_FreeIndexEntry - free an entry of a spool directory index.
 *** Matches the free function signature of xhClear().
 ***
 *** @param entry_c The entry.
 *** @param unused Unused.
 *** @returns 0.
 ***/
int
smtp_internal_FreeIndexEntry(char* entry_c, void* unused)
    {
    pSmtpIndexEntry entry = (pSmtpIndexEntry)entry_c;

	ASSERTMAGIC(entry, MGK_SMTP_INDEX_ENTRY);
	if (entry->Key != NULL) nmSysFree(entry->Key);
	if (entry->Name != NULL) nmSysFree(entry->Name);
	nmFree(entry, sizeof(SmtpIndexEntry));

    return 0;
    }


/*** smtp_internal_IndexAdd - add a Pending email to an index of a spool
 *** directory, replacing any entry with the same key.
 ***
 *** @param index The index (ByMessageID or ByQueueID).
 *** @param key The Message-ID or queue ID of the email.
 *** @param name The email file name.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_IndexAdd(pXHashTable index, char* key, char* name)
    {
    pSmtpIndexEntry entry = NULL;
    pSmtpIndexEntry old;
    int rval = -1;

	/** Create the entry. **/
	entry = nmMalloc(sizeof(SmtpIndexEntry));
	if (UNLIKELY(entry == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate %zu bytes to track email \"%s\".", sizeof(SmtpIndexEntry), name);
	    goto end;
	    }
	memset(entry, 0, sizeof(SmtpIndexEntry));
	SETMAGIC(entry, MGK_SMTP_INDEX_ENTRY);
	entry->Key = nmSysStrdup(key);
	entry->Name = nmSysStrdup(name);
	if (UNLIKELY(entry->Key == NULL || entry->Name == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy key \"%s\" to track email \"%s\".", key, name);
	    goto end;
	    }

	/** Replace any old entry. **/
	old = (pSmtpIndexEntry)xhLookup(index, key);
	if (old != NULL)
	    {
	    xhRemove(index, key);
	    smtp_internal_FreeIndexEntry((char*)old, NULL);
	    }
	if (UNLIKELY(xhAdd(index, entry->Key, (char*)entry) != 0))
	    {
	    mssError(1, "SMTP", "Failed to index email \"%s\" by \"%s\".", name, key);
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0 && entry != NULL)) smtp_internal_FreeIndexEntry((char*)entry, NULL);

	return rval;
    }


/*** smtp_internal_IndexRemove - remove an email from an index of a spool
 *** directory, if it is there.
 ***
 *** @param index The index (ByMessageID or ByQueueID).
 *** @param key The Message-ID or queue ID of the email.
 ***/
void
smtp_internal_IndexRemove(pXHashTable index, char* key)
    {
    pSmtpIndexEntry entry = (pSmtpIndexEntry)xhLookup(index, key);

	if (entry == NULL)
	    return;
	xhRemove(index, key);
	smtp_internal_FreeIndexEntry((char*)entry, NULL);

    return;
    }


/*** smtp_internal_GetSpool - get the tracking data of a spool directory,
 *** creating it the first time.
 ***
 *** @param spoolDir The spool directory.
 *** @returns The spool, or NULL on failure.
 ***/
pSmtpSpool
smtp_internal_GetSpool(char* spoolDir)
    {
    pSmtpSpool spool = NULL;
    bool messageIdsInitialized = false;
    bool queueIdsInitialized = false;

	/** Find the spool. **/
	spool = (pSmtpSpool)xhLookup(&SMTP_INF.Spools, spoolDir);
	ASSERTMAGIC(spool, MGK_SMTP_SPOOL);
	if (spool != NULL)
	    return spool;

	/** Not found: create it. **/
	spool = nmMalloc(sizeof(SmtpSpool));
	if (UNLIKELY(spool == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate tracking data for spool directory \"%s\".", spoolDir);
	    goto error;
	    }
	memset(spool, 0, sizeof(SmtpSpool));
	SETMAGIC(spool, MGK_SMTP_SPOOL);
	spool->LockFd = -1;
	spool->Path = nmSysStrdup(spoolDir);
	if (UNLIKELY(spool->Path == NULL))
	    {
	    mssError(1, "SMTP", "Failed to set spool directory path: \"%s\".", spoolDir);
	    goto error;
	    }
	if (UNLIKELY(xhInit(&spool->ByMessageID, 257, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the Message-ID index of spool directory \"%s\".", spoolDir);
	    goto error;
	    }
	messageIdsInitialized = true;
	if (UNLIKELY(xhInit(&spool->ByQueueID, 257, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the queue ID index of spool directory \"%s\".", spoolDir);
	    goto error;
	    }
	queueIdsInitialized = true;
	spool->LockSem = syCreateSem(1, 0);
	if (UNLIKELY(spool->LockSem == NULL))
	    {
	    mssError(1, "SMTP", "Failed to create the lock semaphore of spool directory \"%s\".", spoolDir);
	    goto error;
	    }
	if (UNLIKELY(xhAdd(&SMTP_INF.Spools, spool->Path, (char*)spool) != 0))
	    {
	    mssError(1, "SMTP", "Failed to add spool directory to hashtable: \"%s\".", spoolDir);
	    goto error;
	    }

	return spool;

    error:
	if (spool != NULL)
	    {
	    if (messageIdsInitialized) xhDeInit(&spool->ByMessageID);
	    if (queueIdsInitialized) xhDeInit(&spool->ByQueueID);
	    if (spool->LockSem != NULL) syDestroySem(spool->LockSem, 0);
	    if (spool->Path != NULL) nmSysFree(spool->Path);
	    nmFree(spool, sizeof(SmtpSpool));
	    }

	return NULL;
    }


/*** smtp_internal_LockSpool - keep other threads and processes from using a
 *** spool directory until the matching smtp_internal_UnlockSpool(), waiting
 *** while they use it.  Calls may nest.  If another process changed the
 *** Pending emails since this one loaded them, they are reloaded at the
 *** next read of the mail log.
 ***
 *** @param spoolDir The spool directory, or NULL if the SMTP node has none.
 *** @returns The locked spool directory, or NULL on failure.
 ***/
pSmtpSpool
smtp_internal_LockSpool(char* spoolDir)
    {
    pSmtpSpool spool = NULL;
    pThread self = thCurrent();
    char path[PATH_MAX];
    char buf[SMTP_SERIAL_LEN + 1];
    struct flock lock;
    struct stat st;
    unsigned long long serial = 0;
    char* end;
    int polls = 0;
    int n;
    bool semHeld = false;
    bool fileLocked = false;
    pSmtpSpool rval = NULL;

	/** Edge cases. **/
	if (UNLIKELY(spoolDir == NULL))
	    {
	    mssError(1, "SMTP", "Failed to lock the spool directory: the SMTP node does not have a 'spool_dir' string.");
	    return NULL; /* Skip error handler, which expects a spool. */
	    }
	spool = smtp_internal_GetSpool(spoolDir);
	if (UNLIKELY(spool == NULL))
	    return NULL; /* Skip error handler, which expects a spool. */

	/** Nest in a lock this thread holds. **/
	if (spool->LockOwner == self)
	    {
	    spool->LockDepth++;
	    return spool;
	    }

	/** Wait for other threads. **/
	if (UNLIKELY(syGetSem(spool->LockSem, 1, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to wait for other threads to use spool directory \"%s\".", spoolDir);
	    goto end;
	    }
	semHeld = true;

	/** Open the lock file and keep it open to hold the process lock. **/
	if (spool->LockFd < 0)
	    {
	    if (UNLIKELY(snprintf(path, sizeof(path), "%s/%s", spoolDir, SMTP_LOCK_FILE) >= (int)sizeof(path)))
		{
		mssError(1, "SMTP", "Failed to build the spool lock path: \"%s/%s\" is too long.", spoolDir, SMTP_LOCK_FILE);
		goto end;
		}
	    spool->LockFd = open(path, O_RDWR | O_CREAT, 0666);
	    if (UNLIKELY(spool->LockFd < 0))
		{
		mssErrorErrno(1, "SMTP", "Failed to open spool lock \"%s\".", path);
		goto end;
		}

	    /** Let every user open the lock file, regardless of it's creator's umask. **/
	    if (UNLIKELY(fstat(spool->LockFd, &st) != 0))
		{
		mssErrorErrno(1, "SMTP", "Failed to check spool lock \"%s\".", path);
		goto end;
		}
	    if ((st.st_mode & 0666) != 0666 && (st.st_uid == geteuid() || geteuid() == 0) && UNLIKELY(fchmod(spool->LockFd, 0666) != 0))
		fprintf(stderr,
		    "Warning: Failed to let every user open spool lock \"%s\" (mode %04o): %s.\n",
		    path, (unsigned int)(st.st_mode & 07777), strerror(errno)
		);
	    }

	/** Wait for other processes. **/
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	while (fcntl(spool->LockFd, F_SETLK, &lock) != 0)
	    {
	    if (UNLIKELY(errno != EACCES && errno != EAGAIN && errno != EINTR))
		{
		mssErrorErrno(1, "SMTP", "Failed to lock spool directory \"%s/%s\".", spoolDir, SMTP_LOCK_FILE);
		goto end;
		}
	    if (UNLIKELY(polls++ >= SMTP_LOCK_TIMEOUT * 1000 / SMTP_LOCK_POLL_INTERVAL))
		{
		mssError(1, "SMTP",
		    "Failed to lock spool directory \"%s/%s\": another process held it for %d seconds.",
		    spoolDir, SMTP_LOCK_FILE, SMTP_LOCK_TIMEOUT
		);
		goto end;
		}
	    thSleep(SMTP_LOCK_POLL_INTERVAL);
	    }
	fileLocked = true;
	spool->LockOwner = self;
	spool->LockDepth = 1;

	/** Read the serial, which is empty until the first change. **/
	n = pread(spool->LockFd, buf, SMTP_SERIAL_LEN, 0);
	if (UNLIKELY(n < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to read the serial of spool directory \"%s/%s\".", spoolDir, SMTP_LOCK_FILE);
	    goto end;
	    }
	buf[n] = '\0';
	if (n > 0)
	    {
	    errno = 0;
	    serial = strtoull(buf, &end, 10);
	    if (UNLIKELY(errno != 0 || end == buf || *end != '\n'))
		{
		mssError(1, "SMTP", "Failed to read the serial of spool directory \"%s/%s\": \"%s\" is invalid.", spoolDir, SMTP_LOCK_FILE, buf);
		goto end;
		}
	    }

	/** Reload the Pending emails if another process changed them. **/
	if (serial != spool->Serial)
	    {
	    smtp_internal_UnloadSpool(spool);
	    spool->Serial = serial;
	    }

	/** Success. **/
	rval = spool;

    end:
	if (UNLIKELY(rval == NULL))
	    {
	    if (fileLocked)
		smtp_internal_UnlockSpool(spool);
	    else if (semHeld && UNLIKELY(syPostSem(spool->LockSem, 1, 0) != 0))
		fprintf(stderr, "Warning: Failed to let other threads use spool directory \"%s\".\n", spoolDir);
	    }

	return rval;
    }


/*** smtp_internal_UnlockSpool - let other threads and processes use a spool
 *** directory again, once every smtp_internal_LockSpool() has a matching
 *** unlock.
 ***
 *** @param spool The locked spool directory.
 ***/
void
smtp_internal_UnlockSpool(pSmtpSpool spool)
    {
    struct flock lock;

	ASSERTMAGIC(spool, MGK_SMTP_SPOOL);
	if (--spool->LockDepth > 0)
	    return;

	/** Let other processes in. **/
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	if (UNLIKELY(fcntl(spool->LockFd, F_SETLK, &lock) != 0))
	    fprintf(stderr,
		"Warning: Failed to unlock spool directory \"%s/%s\": %s.\n",
		spool->Path, SMTP_LOCK_FILE, strerror(errno)
	    );

	/** Let other threads in. **/
	spool->LockOwner = NULL;
	if (UNLIKELY(syPostSem(spool->LockSem, 1, 0) != 0))
	    fprintf(stderr, "Warning: Failed to let other threads use spool directory \"%s\".\n", spool->Path);

    return;
    }


/*** smtp_internal_BumpSerial - tell other processes to reload the Pending
 *** emails of a spool directory, before this one changes them.
 ***
 *** @param spool The locked spool directory.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_BumpSerial(pSmtpSpool spool)
    {
    char buf[SMTP_SERIAL_LEN + 1];
    int n;

	snprintf(buf, sizeof(buf), "%0*llu\n", SMTP_SERIAL_LEN - 1, spool->Serial + 1);
	n = pwrite(spool->LockFd, buf, SMTP_SERIAL_LEN, 0);
	if (UNLIKELY(n != SMTP_SERIAL_LEN))
	    {
	    if (n < 0)
		mssErrorErrno(1, "SMTP", "Failed to write serial %llu to \"%s/%s\".", spool->Serial + 1, spool->Path, SMTP_LOCK_FILE);
	    else
		mssError(1, "SMTP",
		    "Failed to write serial %llu to \"%s/%s\": wrote only %d of %d bytes.",
		    spool->Serial + 1, spool->Path, SMTP_LOCK_FILE, n, SMTP_SERIAL_LEN
		);
	    return -1;
	    }
	spool->Serial++;

    return 0;
    }


/*** smtp_internal_InitGlobals - Initializes global information for the SMTP
 *** driver.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_InitGlobals()
    {
    char local_host_name[HOST_NAME_MAX + 1];
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

	/** Add all the required attributes. Yay hardcoding! **/
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
	    {
	    strtcpy(local_host_name, "localhost.localdomain", sizeof(local_host_name));
	    fprintf(stderr,
		"Warning: gethostname() failed (%s); using \"%s\".\n",
		strerror(errno), local_host_name
	    );
	    }
	local_host_name[sizeof(local_host_name) - 1] = '\0'; /* Terminate a truncated name. */
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
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "log_read_interval",	DATA_T_INTEGER,	SMTP_DEFAULT_LOG_READ_INTERVAL,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultRootAttributes, "sweep_interval",	DATA_T_INTEGER,	SMTP_DEFAULT_SWEEP_INTERVAL,	NULL) < 0)) goto error;
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
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "is_ready",		DATA_T_INTEGER,	0,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "try_count",		DATA_T_INTEGER,	0,	NULL) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "last_try_status",	DATA_T_STRING,	0,	"None") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "last_try_msg",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "queue_id",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "rcpt_count",		DATA_T_INTEGER,	0,	NULL) < 0)) goto error;

	/** Get the mail log path. **/
	if (stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "smtp"), "mail_log"), NULL, &logPath, 0) != 0)
	    logPath = SMTP_DEFAULT_LOG_PATH;
	if (UNLIKELY(strtcpy(SMTP_INF.LogPath, logPath, sizeof(SMTP_INF.LogPath)) < 0))
	    {
	    mssError(1, "SMTP", "Failed to set the mail log path: \"%s\" is too long.", logPath);
	    goto error;
	    }

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


/*** smtp_internal_IsEmail - Returns true if the filename is an email.
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
	"message_id",
	"queue_id",
	"rcpt_count",
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


/*** smtp_internal_IsPending - Checks whether the struct file of an email
 *** says it is Pending, since another open of the email may have sent it.
 *** @param structPath The path of the email's struct file.
 *** @returns 1 if Pending, 0 if not, or -1 if the struct is unreadable.
 ***/
int
smtp_internal_IsPending(char* structPath)
    {
    pFile structFile = NULL;
    pStructInf emailStruct = NULL;
    char* status = NULL;
    int rval = -1;

	/** Parse the struct file. **/
	structFile = fdOpen(structPath, O_RDONLY, 0);
	if (UNLIKELY(structFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to open email struct file \"%s\".", structPath);
	    goto end;
	    }
	emailStruct = stParseMsg(structFile, 0);
	if (UNLIKELY(emailStruct == NULL))
	    {
	    mssError(0, "SMTP", "Failed to parse email struct file \"%s\".", structPath);
	    goto end;
	    }

	/** Success. **/
	rval = (stAttrValue(stLookup(emailStruct, "status"), NULL, &status, 0) == 0 && strcmp(status, "Pending") == 0) ? 1 : 0;

    end:
	if (LIKELY(structFile != NULL)) fdClose(structFile, 0);
	if (LIKELY(emailStruct != NULL)) stFreeInf(emailStruct);

	return rval;
    }


/*** smtp_internal_RemoveEmail - delete the files of an email: the email,
 *** its struct, and its sendmail result, including partial ones.  Missing
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
    char structTmpPath[PATH_MAX];
    int i;

	/** Build the partial file paths. **/
	if (UNLIKELY(snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", resultPath) >= (int)sizeof(tmpPath)))
	    {
	    mssError(1, "SMTP", "Failed to build partial sendmail result path: \"%s.tmp\" is too long.", resultPath);
	    return -1;
	    }
	if (UNLIKELY(snprintf(structTmpPath, sizeof(structTmpPath), "%s.tmp", structPath) >= (int)sizeof(structTmpPath)))
	    {
	    mssError(1, "SMTP", "Failed to build partial email struct path: \"%s.tmp\" is too long.", structPath);
	    return -1;
	    }

	/** Delete the files, email first. **/
	char* paths[] =
	    {
	    emailPath,
	    structPath,
	    structTmpPath,
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
 *** directory, at most once per sweep_interval seconds.  Callers continue
 *** without the sweep, so it resolves its own errors with a warning.
 *** @param spoolDir The spool directory to sweep.
 *** @param rootAttributes The attributes of the SMTP node.
 ***/
void
smtp_internal_SweepSpool(char* spoolDir, pXHashTable rootAttributes)
    {
    pSmtpSpool spool = NULL;
    pSmtpAttribute intervalAttr = NULL;
    int interval = SMTP_DEFAULT_SWEEP_INTERVAL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;
    char emailPath[PATH_MAX];
    char structPath[PATH_MAX];
    char resultPath[PATH_MAX];
    DateTime now;
    time_t curTime = time(NULL);
    int nameLen;
    int expired;
    bool locked = false;
    bool successful = false;

	/** Track each spool directory. **/
	spool = smtp_internal_GetSpool(spoolDir);
	if (UNLIKELY(spool == NULL))
	    goto end;

	/** Throttle sweeps. **/
	intervalAttr = SMTP_ATTR(xhLookup(rootAttributes, "sweep_interval"));
	ASSERTMAGIC(intervalAttr, MGK_SMTP_ATTRIBUTE);
	if (intervalAttr != NULL)
	    {
	    if (UNLIKELY(intervalAttr->Type != DATA_T_INTEGER))
		{
		mssError(1, "SMTP",
		    "Attribute 'sweep_interval' must be an integer (got %s).",
		    objTypeToStr(intervalAttr->Type)
		);
		goto end;
		}
	    interval = intervalAttr->Value.Integer;
	    }
	if (curTime - spool->LastSweep < interval)
	    {
	    /** No sweep needed, we're done. **/
	    successful = true;
	    goto end;
	    }
	spool->LastSweep = curTime;

	/** Keep other threads and processes out until the sweep is done. **/
	if (UNLIKELY(smtp_internal_LockSpool(spoolDir) == NULL))
	    goto end;
	locked = true;

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
	if (locked) smtp_internal_UnlockSpool(spool);

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
	    if (stStructType(currentAttr) == ST_T_SUBGROUP)
		continue; /* Subgroups, such as recipient results, are not attributes. */
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
	    attr = NULL; /* Owned by the hash table. */
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


/*** smtp_internal_ReloadStruct - reload the attributes of an email if its
 *** struct file has been replaced since they were loaded.  Attributes are
 *** updated in place, so attribute names already returned stay valid.
 ***
 *** @param inf The email.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_ReloadStruct(pSmtpData inf)
    {
    struct stat st;
    pFile structFile = NULL;
    pStructInf emailStruct = NULL;
    pXHashTable attributes = NULL;
    pXArray names = NULL;
    pSmtpAttribute newAttr;
    pSmtpAttribute oldAttr;
    ObjData oldValue;
    int oldType;
    char* name;
    int i;
    int rval = -1;

	ASSERTMAGIC(inf, MGK_SMTP_DATA);

	/** Skip an unchanged struct. **/
	if (stat(inf->EmailStructPath.String, &st) != 0)
	    {
	    if (errno == ENOENT)
		{
		rval = 0; /* Deleted, so keep the loaded attributes. */
		goto end;
		}
	    mssErrorErrno(1, "SMTP", "Failed to check email struct file \"%s\".", inf->EmailStructPath.String);
	    goto end;
	    }
	if (st.st_dev == inf->StructInfo.st_dev && st.st_ino == inf->StructInfo.st_ino
	    && st.st_mtim.tv_sec == inf->StructInfo.st_mtim.tv_sec
	    && st.st_mtim.tv_nsec == inf->StructInfo.st_mtim.tv_nsec)
	    {
	    rval = 0;
	    goto end;
	    }

	/** Parse the struct, checking the file it is read from. **/
	structFile = fdOpen(inf->EmailStructPath.String, O_RDONLY, 0);
	if (UNLIKELY(structFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to open email struct file \"%s\".", inf->EmailStructPath.String);
	    goto end;
	    }
	if (UNLIKELY(fstat(fdFD(structFile), &st) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to check email struct file \"%s\".", inf->EmailStructPath.String);
	    goto end;
	    }
	emailStruct = stParseMsg(structFile, 0);
	if (UNLIKELY(emailStruct == NULL))
	    {
	    mssError(0, "SMTP", "Failed to parse email struct file \"%s\".", inf->EmailStructPath.String);
	    goto end;
	    }

	/** Load the new attributes. **/
	attributes = smtp_internal_NewAttributes();
	if (UNLIKELY(attributes == NULL))
	    goto end;
	names = xaNew(16);
	if (UNLIKELY(names == NULL))
	    {
	    mssError(1, "SMTP", "Failed to create attribute names array.");
	    goto end;
	    }
	if (UNLIKELY(smtp_internal_GetStructAttributes(emailStruct, attributes, names) != 0))
	    goto end;

	/** Update the existing attributes and add the new ones. **/
	for (i = 0; i < names->nItems; i++)
	    {
	    name = (char*)names->Items[i];
	    newAttr = SMTP_ATTR(xhLookup(attributes, name));
	    ASSERTMAGIC(newAttr, MGK_SMTP_ATTRIBUTE);
	    oldAttr = SMTP_ATTR(xhLookup(inf->Attributes, name));
	    if (oldAttr != NULL)
		{
		/** Swap the values, so the old one is freed with the new table. **/
		ASSERTMAGIC(oldAttr, MGK_SMTP_ATTRIBUTE);
		oldType = oldAttr->Type;
		oldValue = oldAttr->Value;
		oldAttr->Type = newAttr->Type;
		oldAttr->Value = newAttr->Value;
		newAttr->Type = oldType;
		newAttr->Value = oldValue;
		continue;
		}

	    /** Move a new attribute over. **/
	    if (UNLIKELY(xhRemove(attributes, name) != 0))
		{
		mssError(1, "SMTP", "Failed to remove new attribute '%s' from the reloaded attributes.", name);
		goto end;
		}
	    if (UNLIKELY(xhAdd(inf->Attributes, newAttr->Name, (char*)newAttr) != 0))
		{
		mssError(1, "SMTP", "Failed to add new attribute '%s'.", name);
		smtp_internal_ClearAttribute((char*)newAttr, NULL);
		goto end;
		}
	    if (UNLIKELY(xaAddItem(inf->AttributeNames, newAttr->Name) < 0))
		{
		mssError(1, "SMTP", "Failed to add new attribute name '%s' to list.", name);
		xhRemove(inf->Attributes, newAttr->Name);
		smtp_internal_ClearAttribute((char*)newAttr, NULL);
		goto end;
		}
	    }

	/** Record which struct file the attributes came from. **/
	inf->StructInfo = st;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to reload the attributes of email \"%s\".", inf->Name);

	if (names != NULL) xaFree(names);
	if (attributes != NULL) smtp_internal_FreeAttributes(attributes);
	if (emailStruct != NULL) stFreeInf(emailStruct);
	if (structFile != NULL) fdClose(structFile, 0);

	return rval;
    }


/*** smtp_internal_ReloadAttributes - bring the attributes of an email up to
 *** date: record new results from the mail log, reload a replaced struct,
 *** then update the send status of a Pending email.
 ***
 *** @param inf The email.  Other objects are skipped.
 *** @param readLog Whether to first record the results in the mail log, if
 ***   the spool directory has not read it in the last log_read_interval
 ***   seconds.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_ReloadAttributes(pSmtpData inf, bool readLog)
    {
    pSmtpAttribute spoolDir;
    pSmtpSpool spool = NULL;
    pStructInf emailStruct = NULL;
    char* status;
    bool changed = false;
    int found;
    int rval = -1;

	/** Skip other objects. **/
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (inf->Type != SMTP_T_EML)
	    return 0;

	/** Keep other threads and processes out until the attributes are loaded. **/
	spoolDir = SMTP_ATTR(xhLookup(inf->RootAttributes, "spool_dir"));
	ASSERTMAGIC(spoolDir, MGK_SMTP_ATTRIBUTE);
	if (UNLIKELY(spoolDir == NULL || spoolDir->Type != DATA_T_STRING))
	    {
	    mssError(1, "SMTP", "Failed to reload the attributes of \"%s\": the SMTP node does not have a 'spool_dir' string.", inf->Name);
	    goto end;
	    }
	spool = smtp_internal_LockSpool(spoolDir->Value.String);
	if (UNLIKELY(spool == NULL))
	    goto end;

	/** Update the struct with results from the Postfix logs. **/
	if (UNLIKELY(readLog && smtp_internal_UpdateFromLog(spoolDir->Value.String, inf->RootAttributes, true) != 0))
	    mssWarnError("Failed to update the emails in \"%s\" from the mail log.", spoolDir->Value.String);

	/** Reload a replaced struct. **/
	if (UNLIKELY(smtp_internal_ReloadStruct(inf) != 0))
	    goto end;

	/** If the email isn't Pending, we're done. **/
	status = smtp_internal_GetString(inf->Attributes, "status");
	if (status == NULL || strcmp(status, "Pending") != 0)
	    {
	    rval = 0;
	    goto end;
	    }

	/** For Pending emails, update the send status from the sendmail result/timeout. **/
	found = smtp_internal_ReadStruct(inf->EmailStructPath.String, &emailStruct);
	if (found == 1)
	    {
	    status = smtp_internal_StructString(emailStruct, "status");
	    if (status != NULL && strcmp(status, "Pending") == 0 && UNLIKELY(
		smtp_internal_RefreshStatus(emailStruct, inf->ResultPath.String, false, inf->RootAttributes, &changed) != 0
		|| (changed && smtp_internal_WriteStruct(inf->EmailStructPath.String, emailStruct) != 0)
	    ))  {
		found = -1;
		}
	    }
	if (UNLIKELY(found < 0))
	    mssWarnError("Failed to update the send status of email \"%s\".", inf->Name);

	/** Load the updated status. **/
	if (found == 1 && changed && UNLIKELY(smtp_internal_ReloadStruct(inf) != 0))
	    goto end;

	/** Success. **/
	rval = 0;

    end:
	if (emailStruct != NULL) stFreeInf(emailStruct);
	if (spool != NULL) smtp_internal_UnlockSpool(spool);

	return rval;
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
			    "Failed to get the current date for the Date header."
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


/*** smtp_internal_CalcExpireDate - calculate the expire_date of an email
 *** that becomes Sent or Error now, which is expire_time seconds from now.
 *** A negative expire_time keeps the email indefinitely.
 *** @param rootAttributes The attributes of the SMTP node.
 *** @param expireDate Set to the expire date, if the email expires.
 *** @returns 1 if the email expires, 0 if it does not, or -1 on failure.
 ***/
int
smtp_internal_CalcExpireDate(pXHashTable rootAttributes, pDateTime expireDate)
    {
    pSmtpAttribute expireTimeAttr = NULL;
    int expireTime = SMTP_DEFAULT_EXPIRE_TIME;

	/** Get the expire time. **/
	expireTimeAttr = SMTP_ATTR(xhLookup(rootAttributes, "expire_time"));
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

	/** Calculate the expire date. **/
	if (UNLIKELY(objCurrentDate(expireDate) != 0 || objDateAdd(expireDate, expireTime, 0, 0, 0, 0, 0) != 0))
	    {
	    mssError(0, "SMTP",
		"Failed to calculate the expire date (%d seconds from now).",
		expireTime
	    );
	    return -1;
	    }

    return 1;
    }


/*** smtp_internal_SetExpireDate - set the expire_date of an email that
 *** became Sent or Error.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_SetExpireDate(pSmtpData inf)
    {
    DateTime expireDate;
    ObjData pod;
    int expires;

	/** Calculate and record the expire date. **/
	expires = smtp_internal_CalcExpireDate(inf->RootAttributes, &expireDate);
	if (expires <= 0)
	    return expires;
	pod.DateTime = &expireDate;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "expire_date", DATA_T_DATETIME, &pod, NULL) != 0))
	    return -1;

    return 0;
    }


/*** smtp_internal_SendEmail - mark the email Pending, clear the results of
 *** earlier tries, record the try_count and *_try_date attributes of this
 *** try, then hand the email to sendmail.  If that fails, the email becomes
 *** Error.  Refuses an email that is already Pending.  A Pending email never
 *** expires, so its expire_date is cleared.
 *** @returns 0 if the email was handed off, or -1 if it was not.
 ***/
int
smtp_internal_SendEmail(pSmtpData inf)
    {
    pSmtpAttribute envFrom = NULL;
    pSmtpAttribute envTo = NULL;
    pSmtpAttribute tryCountAttr = NULL;
    pSmtpAttribute firstTryAttr = NULL;
    pSmtpAttribute spoolDir = NULL;
    pSmtpSpool spool = NULL;
    char* messageId = NULL;
    DateTime noExpireDate;
    DateTime tryDate;
    pXString tryMsg = NULL;
    char* tryMsgStr = "";
    ObjData pod;
    bool recordFailed = false;
    int pending;
    int rval = -1;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to send NULL smtp object.");
	    return -1; /* Skip error handler, which expects a valid object. */
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	spoolDir = SMTP_ATTR(xhLookup(inf->RootAttributes, "spool_dir"));
	ASSERTMAGIC(spoolDir, MGK_SMTP_ATTRIBUTE);
	if (UNLIKELY(spoolDir == NULL || spoolDir->Type != DATA_T_STRING))
	    {
	    mssError(1, "SMTP", "Failed to send \"%s\": the SMTP node does not have a 'spool_dir' string.", inf->Name);
	    return -1; /* Skip error handler, which records a failed try. */
	    }

	/** Keep other threads and processes out until this try is recorded. **/
	spool = smtp_internal_LockSpool(spoolDir->Value.String);
	if (UNLIKELY(spool == NULL))
	    return -1; /* Skip error handler, which records a failed try. */

	/*** Read the mail log first, so any lines from earlier tries are not
	 *** discovered later and applied to this one after we've cleared it.
	 ***/
	if (UNLIKELY(smtp_internal_UpdateFromLog(spoolDir->Value.String, inf->RootAttributes, false) != 0))
	    mssWarnError("Failed to update the emails in \"%s\" from the mail log.", spoolDir->Value.String);

	/** Refuse to send an email that is already Pending, even from another open. **/
	pending = smtp_internal_IsPending(inf->EmailStructPath.String);
	if (UNLIKELY(pending != 0))
	    {
	    if (pending > 0)
		mssError(1, "SMTP", "Failed to send \"%s\": it is already Pending.", inf->Name);
	    else
		mssError(0, "SMTP", "Failed to check whether \"%s\" is already Pending.", inf->Name);
	    smtp_internal_UnlockSpool(spool);
	    return -1; /* Skip error handler, which records a failed try. */
	    }

	/** Load the results of earlier tries, which other opens may have recorded. **/
	if (UNLIKELY(smtp_internal_ReloadAttributes(inf, false) != 0))
	    goto end;

	/** Tell other processes that the Pending emails are changing. **/
	if (UNLIKELY(smtp_internal_BumpSerial(spool) != 0))
	    goto end;

	/** Mark the email Pending before handing it off, so other sends refuse it. **/
	pod.String = "Pending";
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "status", DATA_T_STRING, &pod, NULL) != 0))
	    goto end;

	/** Clear the results of earlier tries. **/
	pod.String = "";
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "queue_id", DATA_T_STRING, &pod, NULL) != 0))
	    goto end;
	pod.Integer = 0;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "rcpt_count", DATA_T_INTEGER, &pod, NULL) != 0))
	    goto end;
	if (UNLIKELY(smtp_internal_ClearRcpts(inf->EmailStructPath.String) != 0))
	    goto end;
	pod.String = "None";
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_status", DATA_T_STRING, &pod, NULL) != 0))
	    goto end;
	pod.String = "";
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_msg", DATA_T_STRING, &pod, NULL) != 0))
	    goto end;

	/** Record no expire date. (Pending emails don't expire.) **/
	memset(&noExpireDate, 0, sizeof(DateTime));
	pod.DateTime = &noExpireDate;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "expire_date", DATA_T_DATETIME, &pod, NULL) != 0))
	    goto end;

	/** Count the try. **/
	tryCountAttr = SMTP_ATTR(xhLookup(inf->Attributes, "try_count"));
	ASSERTMAGIC(tryCountAttr, MGK_SMTP_ATTRIBUTE);
	pod.Integer = (tryCountAttr != NULL && tryCountAttr->Type == DATA_T_INTEGER) ? tryCountAttr->Value.Integer + 1 : 1;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "try_count", DATA_T_INTEGER, &pod, NULL) != 0))
	    goto end;

	/** Record the try dates, keeping the first one. **/
	if (UNLIKELY(objCurrentDate(&tryDate) != 0))
	    {
	    mssError(0, "SMTP", "Failed to get the current date for the try dates.");
	    goto end;
	    }
	pod.DateTime = &tryDate;
	if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_date", DATA_T_DATETIME, &pod, NULL) != 0))
	    goto end;
	firstTryAttr = SMTP_ATTR(xhLookup(inf->Attributes, "first_try_date"));
	ASSERTMAGIC(firstTryAttr, MGK_SMTP_ATTRIBUTE);
	if (firstTryAttr == NULL || firstTryAttr->Type != DATA_T_DATETIME
	    || firstTryAttr->Value.DateTime == NULL || firstTryAttr->Value.DateTime->Value == 0)
	    {
	    if (UNLIKELY(smtp_internal_SetAttrValue(inf, "first_try_date", DATA_T_DATETIME, &pod, NULL) != 0))
		goto end;
	    }

	/** Track the email by its Message-ID until Postfix queues it. **/
	messageId = smtp_internal_GetString(inf->Attributes, "message_id");
	if (spool->Loaded && messageId != NULL && UNLIKELY(smtp_internal_IndexAdd(&spool->ByMessageID, messageId, inf->Name) != 0))
	    goto end;

	/** Add the header attributes to the email. **/
	if (UNLIKELY(smtp_internal_ApplyHeaders(inf) < 0))
	    goto end;

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
	rval = 0;

    end:
	/** Record a failed try. **/
	if (UNLIKELY(rval != 0))
	    {
	    /** Get the errors of the failed try, before recording adds more. **/
	    tryMsg = xsNew();
	    if (UNLIKELY(tryMsg == NULL || mssUserError(tryMsg) != 0))
		mssError(0, "SMTP", "Failed to get the error message of the failed try.");
	    else
		tryMsgStr = tryMsg->String;

	    /** Record the failure. **/
	    pod.String = "Error";
	    if (UNLIKELY(smtp_internal_SetAttrValue(inf, "status", DATA_T_STRING, &pod, NULL) != 0))
		recordFailed = true;
	    pod.String = "Fail";
	    if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_status", DATA_T_STRING, &pod, NULL) != 0))
		recordFailed = true;
	    pod.String = tryMsgStr;
	    if (UNLIKELY(smtp_internal_SetAttrValue(inf, "last_try_msg", DATA_T_STRING, &pod, NULL) != 0))
		recordFailed = true;
	    if (UNLIKELY(smtp_internal_SetExpireDate(inf) != 0))
		recordFailed = true;

	    /** Report a failure to record it, then restore the send error it cleared. **/
	    if (UNLIKELY(recordFailed))
		{
		fprintf(stderr, "Warning: Failed to record the failed try of \"%s\"; it may stay Pending.\n", inf->Name);
		mssError(1, "SMTP", "Failed to send \"%s\": %s", inf->Name, tryMsgStr);
		}
	    }
	smtp_internal_UnlockSpool(spool);

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


/*** smtp_internal_RefreshStatus - update the struct of a Pending email from
 *** its sendmail result and the recipient results recorded in it.  It
 *** becomes Sent or Error once Postfix finishes, or Error if there is still
 *** no result SMTP_PENDING_TIMEOUT seconds after last_try_date.  Without a
 *** sendmail result or queue ID, it becomes Error after SMTP_RESULT_TIMEOUT.
 ***
 *** @param emailStruct The struct of the email, read from its file.
 *** @param resultPath The path of the sendmail result file.
 *** @param expired Whether Postfix gave up and returned the email.
 *** @param rootAttributes The attributes of the SMTP node.
 *** @param changed Set to whether emailStruct changed. (Optional)
 *** @returns 0 on success (including no change), or -1 on failure.
 ***/
int
smtp_internal_RefreshStatus(pStructInf emailStruct, char* resultPath, bool expired, pXHashTable rootAttributes, bool* changed)
    {
    char header[SMTP_RESULT_HEADER_LEN + 1];
    XString output;
    XString tryMsg;
    bool initialized = false;
    char* status = NULL; /* The final status, or NULL while Pending. */
    char* tryStatus = NULL; /* The new last_try_status, or NULL to keep it. */
    char* queueId;
    char* current;
    char* dateStr;
    char* rcptStatus;
    char* rcptAddress;
    char* rcptReply;
    pStructInf rcpt;
    DateTime cutoff;
    DateTime now;
    DateTime expireDate;
    ObjData pod;
    char* sep = " ";
    bool logged = false;
    bool timedOut = false;
    bool unknown = false;
    int rcptCount = 0;
    int result;
    int printed;
    int code;
    int expires;
    int nSent = 0;
    int nBounced = 0;
    int nDeferred = 0;
    int nTotal = 0;
    int i;
    int rval = -1;

	/** No changes yet. **/
	if (changed != NULL)
	    *changed = false;

	/** Initialize send status strings. **/
	if (UNLIKELY(xsInit(&output) != 0 || xsInit(&tryMsg) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the send status strings.");
	    goto end;
	    }
	initialized = true;

	/** Check whether the hand-off to sendmail failed. **/
	result = smtp_internal_ReadResult(resultPath, header, &output);
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

	/** Count the recipient results, once Postfix queued the email. **/
	queueId = smtp_internal_StructString(emailStruct, "queue_id");
	if (status == NULL && queueId != NULL && queueId[0] != '\0')
	    {
	    logged = true;
	    for (i = 0; i < emailStruct->nSubInf; i++)
		{
		rcpt = emailStruct->SubInf[i];
		if (!smtp_internal_IsRcpt(rcpt))
		    continue;
		rcptStatus = smtp_internal_StructString(rcpt, "status");
		if (rcptStatus != NULL && strcmp(rcptStatus, smtp_rcpt_status_names[SMTP_RCPT_SENT]) == 0) nSent++;
		else if (rcptStatus != NULL && strcmp(rcptStatus, smtp_rcpt_status_names[SMTP_RCPT_BOUNCED]) == 0) nBounced++;
		else nDeferred++;
		}
	    rcptCount = smtp_internal_StructInteger(emailStruct, "rcpt_count");
	    nTotal = (rcptCount > nSent + nBounced + nDeferred) ? rcptCount : nSent + nBounced + nDeferred;

	    /** Postfix is done when every recipient is sent or bounced, or it gives up. **/
	    if (expired || (rcptCount > 0 && nSent + nBounced >= rcptCount))
		status = (nBounced == 0 && nDeferred == 0 && !expired) ? "Sent" : "Error";
	    }

	/** Give up on an email with no result in time, sooner if sendmail never reported back. **/
	dateStr = smtp_internal_StructString(emailStruct, "last_try_date");
	if (status == NULL && dateStr != NULL)
	    {
	    unknown = (result == 0 && !logged);
	    memset(&cutoff, 0, sizeof(DateTime));
	    if (UNLIKELY(objDataToDateTime(DATA_T_STRING, dateStr, &cutoff, NULL) != 0))
		{
		mssError(1, "SMTP", "Invalid last_try_date \"%s\".", dateStr);
		goto end;
		}
	    if (cutoff.Value != 0)
		{
		if (UNLIKELY(objDateAdd(&cutoff, (unknown) ? SMTP_RESULT_TIMEOUT : SMTP_PENDING_TIMEOUT, 0, 0, 0, 0, 0) != 0
		    || objCurrentDate(&now) != 0
		))  {
		    mssError(0, "SMTP", "Failed to check whether the send status timed out.");
		    goto end;
		    }
		if (now.Value >= cutoff.Value)
		    {
		    status = "Error";
		    timedOut = true;
		    if (unknown)
			printed = xsPrintf(&tryMsg,
			    "No sendmail result %d seconds after the last try.",
			    SMTP_RESULT_TIMEOUT
			);
		    else
			printed = xsPrintf(&tryMsg,
			    "Send status unknown %d days after the last try.",
			    SMTP_PENDING_TIMEOUT / (24 * 60 * 60)
			);
		    if (UNLIKELY(printed < 0))
			{
			mssError(1, "SMTP", "Failed to describe the send status timeout.");
			goto end;
			}
		    }
		}
	    }

	/** Describe the latest result of each recipient not sent the email. **/
	if (logged && (nBounced > 0 || nDeferred > 0))
	    {
	    if (UNLIKELY(xsConcatPrintf(&tryMsg,
		"%sSent to %d of %d recipients.",
		(timedOut) ? " " : "", nSent, nTotal
	    ) < 0))
		{
		mssError(1, "SMTP", "Failed to describe the recipients of queue ID %s.", queueId);
		goto end;
		}
	    for (i = 0; i < emailStruct->nSubInf; i++)
		{
		rcpt = emailStruct->SubInf[i];
		if (!smtp_internal_IsRcpt(rcpt))
		    continue;
		rcptStatus = smtp_internal_StructString(rcpt, "status");
		if (rcptStatus != NULL && strcmp(rcptStatus, smtp_rcpt_status_names[SMTP_RCPT_SENT]) == 0)
		    continue;
		rcptAddress = smtp_internal_StructString(rcpt, "address");
		rcptReply = smtp_internal_StructString(rcpt, "reply");
		if (UNLIKELY(xsConcatPrintf(&tryMsg, "%s%s: %s: %s",
		    sep, (rcptAddress != NULL) ? rcptAddress : "",
		    (rcptStatus != NULL && strcmp(rcptStatus, smtp_rcpt_status_names[SMTP_RCPT_BOUNCED]) == 0)
			? "bounced" : (expired ? "expired" : "deferred"),
		    (rcptReply != NULL) ? rcptReply : ""
		) < 0))
		    {
		    mssError(1, "SMTP", "Failed to describe a recipient of queue ID %s.", queueId);
		    goto end;
		    }
		sep = "; ";
		}
	    }

	/** Fail once a recipient fails for good, even while Pending. **/
	if (logged || timedOut)
	    {
	    if (status != NULL && strcmp(status, "Sent") == 0)
		tryStatus = "None";
	    else if (status != NULL || nBounced > 0)
		tryStatus = "Fail";
	    else if (nDeferred > 0)
		tryStatus = "TempFail";
	    else
		tryStatus = "None";
	    }

	/** Record the try result, if it changed. **/
	if (tryStatus != NULL)
	    {
	    current = smtp_internal_StructString(emailStruct, "last_try_status");
	    if (current == NULL || strcmp(current, tryStatus) != 0)
		{
		pod.String = tryStatus;
		if (UNLIKELY(smtp_internal_StructSet(emailStruct, "last_try_status", DATA_T_STRING, &pod) != 0))
		    goto end;
		if (changed != NULL)
		    *changed = true;
		}
	    current = smtp_internal_StructString(emailStruct, "last_try_msg");
	    if (current == NULL || strcmp(current, tryMsg.String) != 0)
		{
		pod.String = tryMsg.String;
		if (UNLIKELY(smtp_internal_StructSet(emailStruct, "last_try_msg", DATA_T_STRING, &pod) != 0))
		    goto end;
		if (changed != NULL)
		    *changed = true;
		}
	    }

	/** Record the final status. **/
	if (status != NULL)
	    {
	    expires = smtp_internal_CalcExpireDate(rootAttributes, &expireDate);
	    if (UNLIKELY(expires < 0))
		goto end;
	    pod.DateTime = &expireDate;
	    if (expires > 0 && UNLIKELY(smtp_internal_StructSet(emailStruct, "expire_date", DATA_T_DATETIME, &pod) != 0))
		goto end;
	    pod.String = status;
	    if (UNLIKELY(smtp_internal_StructSet(emailStruct, "status", DATA_T_STRING, &pod) != 0))
		goto end;
	    if (changed != NULL)
		*changed = true;
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


/*** smtp_internal_ParseLogLine - find the result in one line of the mail
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
 *** @param parsed Set to the parts of the line, which point into it.  Its
 ***   Kind is SMTP_LINE_NONE if the line has no result.
 ***/
void
smtp_internal_ParseLogLine(char* line, pSmtpLogLine parsed)
    {
    static const char queueIdChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char* program;
    char* message;
    char* queueId;
    int queueIdLen;
    char* value;
    int valueLen;
    char* address;
    char* reply;
    int replyLen;

	memset(parsed, 0, sizeof(SmtpLogLine));
	parsed->Kind = SMTP_LINE_NONE;

	/** Find the message, and skip lines not logged by Postfix. **/
	message = strstr(line, "]: ");
	if (message == NULL)
	    return;
	program = message;
	while (program > line && *program != '[') program--;
	while (program > line && program[-1] != ' ') program--;
	if (strncmp(program, "postfix", 7) != 0)
	    return;
	message += 3; /* Consume the "]: ". */

	/** Get the queue ID. **/
	queueIdLen = strspn(message, queueIdChars);
	if (queueIdLen == 0 || queueIdLen >= SMTP_QUEUE_ID_SIZE || strncmp(message + queueIdLen, ": ", 2) != 0)
	    return;
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
		return;
	    parsed->Kind = SMTP_LINE_QUEUED;
	    parsed->QueueID = queueId;
	    parsed->MessageID = value;
	    return;
	    }

	/** Detect a recipient count, or Postfix giving up. **/
	if (strncmp(message, "from=<", 6) == 0)
	    {
	    /** Detect recipient count. **/
	    if ((value = strstr(message, ", nrcpt=")) != NULL)
		{
		parsed->Kind = SMTP_LINE_RCPT_COUNT;
		parsed->QueueID = queueId;
		parsed->RcptCount = atoi(value + 8);
		}

	    /** Detect Postfix giving up. **/
	    else if (strstr(message, ", status=expired,") != NULL || strstr(message, ", status=force-expired,") != NULL)
		{
		parsed->Kind = SMTP_LINE_EXPIRED;
		parsed->QueueID = queueId;
		}

	    return;
	    }

	/** Detect a recipient result. **/
	if (strncmp(message, "to=<", 4) == 0)
	    {
	    address = message + 4;
	    value = strchr(address, '>');
	    if (value == NULL)
		return;
	    *value = '\0';
	    value = strstr(value + 1, ", status="); /* Search after the address. */
	    if (value == NULL)
		return;
	    value += 9;
	    valueLen = strcspn(value, " ");
	    if (valueLen == 4 && strncmp(value, "sent", 4) == 0)
		parsed->Status = SMTP_RCPT_SENT;
	    else if (valueLen == 8 && strncmp(value, "deferred", 8) == 0)
		parsed->Status = SMTP_RCPT_DEFERRED;
	    else if (valueLen == 7 && strncmp(value, "bounced", 7) == 0)
		parsed->Status = SMTP_RCPT_BOUNCED;
	    else
		return;

	    /** Get the reply, without its parentheses. **/
	    reply = value + valueLen;
	    if (strncmp(reply, " (", 2) == 0)
		reply += 2;
	    replyLen = strlen(reply);
	    if (replyLen > 0 && reply[replyLen - 1] == ')')
		reply[replyLen - 1] = '\0';

	    parsed->Kind = SMTP_LINE_RCPT;
	    parsed->QueueID = queueId;
	    parsed->Address = address;
	    parsed->Reply = reply;
	    }

    return;
    }


/*** smtp_internal_OpenLog - open a mail log as root, since only root can
 *** read it.
 ***
 *** @param path The path of the mail log.
 *** @returns The open log, or NULL on failure.
 ***/
pFile
smtp_internal_OpenLog(char* path)
    {
    uid_t uid = geteuid();
    pFile log = NULL;
    int fd = -1;
    int openErrno;

	/** Open the log as root. **/
	if (UNLIKELY(uid != 0 && seteuid(0) != 0))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to become root to open the mail log \"%s\". (Centrallix is not running as root.)",
		path
	    );
	    goto end;
	    }
	fd = open(path, O_RDONLY);
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
	    mssErrorErrno(1, "SMTP", "Failed to open the mail log \"%s\".", path);
	    goto end;
	    }

	/** Read it through MTask. **/
	log = fdOpenFD(fd, O_RDONLY);
	if (UNLIKELY(log == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to read the mail log \"%s\".", path);
	    goto end;
	    }
	fd = -1; /* Closed with log. */

    end:
	if (UNLIKELY(fd >= 0)) close(fd);

	return log;
    }


/*** smtp_internal_CompareRotatedLogs - qsort() callback that orders rotated
 *** mail logs from oldest to newest.
 ***/
int
smtp_internal_CompareRotatedLogs(const void* a, const void* b)
    {
    pSmtpRotatedLog logA = *(pSmtpRotatedLog*)a;
    pSmtpRotatedLog logB = *(pSmtpRotatedLog*)b;

	/** The cursor's log goes first, even if another has the same mtime. **/
	if (logA->MTime.tv_sec != logB->MTime.tv_sec)
	    return (logA->MTime.tv_sec < logB->MTime.tv_sec) ? -1 : 1;
	if (logA->MTime.tv_nsec != logB->MTime.tv_nsec)
	    return (logA->MTime.tv_nsec < logB->MTime.tv_nsec) ? -1 : 1;
	if (logA->IsCursor != logB->IsCursor)
	    return (logA->IsCursor) ? -1 : 1;

    return 0;
    }


/*** smtp_internal_FreeRotatedLogs - free the logs found by
 *** smtp_internal_FindRotatedLogs(), leaving the XArray empty.
 ***/
void
smtp_internal_FreeRotatedLogs(pXArray logs)
    {
    pSmtpRotatedLog log;
    int i;

	for (i = 0; i < logs->nItems; i++)
	    {
	    log = (pSmtpRotatedLog)logs->Items[i];
	    nmSysFree(log->Path);
	    nmFree(log, sizeof(SmtpRotatedLog));
	    }
	xaClear(logs, NULL, NULL);

    return;
    }


/*** smtp_internal_FindRotatedLogs - find the rotated mail logs that a spool
 *** directory has not finished reading: the one its cursor is in (after it
 *** was rotated to a name such as maillog-20261001 or maillog.1), then each
 *** log rotated after it.  Compressed logs are skipped.
 ***
 *** @param spool The spool directory, which has a cursor.
 *** @param logs An initialized XArray, set to the pSmtpRotatedLog of each log
 ***   from oldest to newest.  Free them with smtp_internal_FreeRotatedLogs().
 *** @returns 1 if the cursor's log was found, 0 if not, or -1 on failure.
 ***/
int
smtp_internal_FindRotatedLogs(pSmtpSpool spool, pXArray logs)
    {
    static char* compressed[] = { ".gz", ".bz2", ".xz", ".zst", ".lz4", ".Z" };
    char dirPath[PATH_MAX];
    char path[PATH_MAX];
    char* base;
    char* ext;
    int baseLen;
    DIR* dir = NULL;
    struct dirent* entry;
    struct stat st;
    pSmtpRotatedLog log = NULL;
    struct timespec cursorMTime = { 0, 0 };
    bool found = false;
    bool skip;
    int i;
    int rval = -1;

	/** Split the log path into its directory and base name. **/
	base = strrchr(SMTP_INF.LogPath, '/');
	if (base == NULL)
	    {
	    strtcpy(dirPath, ".", sizeof(dirPath));
	    base = SMTP_INF.LogPath;
	    }
	else
	    {
	    snprintf(dirPath, sizeof(dirPath), "%.*s", (int)(base - SMTP_INF.LogPath), SMTP_INF.LogPath);
	    if (dirPath[0] == '\0')
		strtcpy(dirPath, "/", sizeof(dirPath));
	    base++;
	    }
	baseLen = strlen(base);

	/** Open the log directory. **/
	dir = opendir(dirPath);
	if (UNLIKELY(dir == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to open mail log directory \"%s\".", dirPath);
	    goto end;
	    }

	/** Collect the rotated logs. **/
	while (1)
	    {
	    /** Get the next file. **/
	    errno = 0;
	    entry = readdir(dir);
	    if (entry == NULL)
		{
		if (UNLIKELY(errno != 0))
		    {
		    mssErrorErrno(1, "SMTP", "Failed to read mail log directory \"%s\".", dirPath);
		    goto end;
		    }
		break; /* No more files. */
		}

	    /** Check uncompressed files named like a rotated log. **/
	    if (strncmp(entry->d_name, base, baseLen) != 0 || (entry->d_name[baseLen] != '-' && entry->d_name[baseLen] != '.'))
		continue;
	    ext = strrchr(entry->d_name, '.');
	    skip = false;
	    for (i = 0; ext != NULL && i < (int)(sizeof(compressed) / sizeof(compressed[0])); i++)
		if (strcmp(ext, compressed[i]) == 0)
		    skip = true;
	    if (skip)
		continue;
	    if (snprintf(path, sizeof(path), "%s/%s", dirPath, entry->d_name) >= (int)sizeof(path))
		continue;
	    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
		continue;

	    /** Keep it. **/
	    log = nmMalloc(sizeof(SmtpRotatedLog));
	    if (UNLIKELY(log == NULL))
		{
		mssError(1, "SMTP", "Failed to allocate %zu bytes to track rotated mail log \"%s\".", sizeof(SmtpRotatedLog), path);
		goto end;
		}
	    memset(log, 0, sizeof(SmtpRotatedLog));
	    log->Path = nmSysStrdup(path);
	    if (UNLIKELY(log->Path == NULL))
		{
		mssError(1, "SMTP", "Failed to copy rotated mail log path \"%s\".", path);
		goto end;
		}
	    log->MTime = st.st_mtim;
	    log->IsCursor = (st.st_dev == spool->LogDev && st.st_ino == spool->LogIno);
	    if (UNLIKELY(xaAddItem(logs, log) < 0))
		{
		mssError(1, "SMTP", "Failed to track rotated mail log \"%s\".", path);
		goto end;
		}
	    if (log->IsCursor)
		{
		found = true;
		cursorMTime = log->MTime;
		}
	    log = NULL;
	    }

	/** Without the cursor's log, there is no way to tell which logs are newer. **/
	if (!found)
	    {
	    smtp_internal_FreeRotatedLogs(logs);
	    rval = 0;
	    goto end;
	    }

	/** Drop the logs rotated before the cursor's, then order the rest. **/
	for (i = logs->nItems - 1; i >= 0; i--)
	    {
	    log = (pSmtpRotatedLog)logs->Items[i];
	    if (log->MTime.tv_sec < cursorMTime.tv_sec
		|| (log->MTime.tv_sec == cursorMTime.tv_sec && log->MTime.tv_nsec < cursorMTime.tv_nsec))
		{
		xaRemoveItem(logs, i);
		nmSysFree(log->Path);
		nmFree(log, sizeof(SmtpRotatedLog));
		}
	    }
	log = NULL;
	qsort(logs->Items, logs->nItems, sizeof(void*), smtp_internal_CompareRotatedLogs);

	/** Success. **/
	rval = 1;

    end:
	if (UNLIKELY(log != NULL))
	    {
	    if (log->Path != NULL) nmSysFree(log->Path);
	    nmFree(log, sizeof(SmtpRotatedLog));
	    }
	if (UNLIKELY(rval < 0))
	    smtp_internal_FreeRotatedLogs(logs);
	if (dir != NULL) closedir(dir);

	return rval;
    }


/*** smtp_internal_UnloadSpool - forget the cursor and indexes of a spool
 *** directory, so the next read of the mail log loads them from its files.
 ***
 *** @param spool The spool directory.
 ***/
void
smtp_internal_UnloadSpool(pSmtpSpool spool)
    {
	xhClear(&spool->ByMessageID, smtp_internal_FreeIndexEntry, NULL);
	xhClear(&spool->ByQueueID, smtp_internal_FreeIndexEntry, NULL);
	spool->Loaded = false;
	spool->HasCursor = false;

    return;
    }


/*** smtp_internal_LoadSpool - read the mail log cursor of a spool directory
 *** and index its Pending emails.
 ***
 *** @param spool The spool directory.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_LoadSpool(pSmtpSpool spool)
    {
    char path[PATH_MAX];
    FILE* cursorFile = NULL;
    unsigned long long dev, ino;
    long long offset;
    DIR* dir = NULL;
    struct dirent* entry;
    pStructInf emailStruct = NULL;
    char* status;
    char* key;
    int found;
    int rval = -1;

	/** Read the cursor, which is missing until the mail log is first read. **/
	if (UNLIKELY(snprintf(path, sizeof(path), "%s/%s", spool->Path, SMTP_CURSOR_FILE) >= (int)sizeof(path)))
	    {
	    mssError(1, "SMTP", "Failed to build the mail log cursor path: \"%s/%s\" is too long.", spool->Path, SMTP_CURSOR_FILE);
	    goto end;
	    }
	spool->HasCursor = false;
	cursorFile = fopen(path, "r");
	if (cursorFile == NULL)
	    {
	    if (UNLIKELY(errno != ENOENT))
		{
		mssErrorErrno(1, "SMTP", "Failed to open mail log cursor \"%s\".", path);
		goto end;
		}
	    }
	else if (fscanf(cursorFile, "%llu %llu %lld", &dev, &ino, &offset) != 3 || offset < 0)
	    {
	    fprintf(stderr, "Warning: Ignored invalid mail log cursor \"%s\"; reading the mail log from the start.\n", path);
	    }
	else
	    {
	    spool->LogDev = (dev_t)dev;
	    spool->LogIno = (ino_t)ino;
	    spool->LogOffset = (off_t)offset;
	    spool->HasCursor = true;
	    }

	/** Open the spool directory. **/
	dir = opendir(spool->Path);
	if (UNLIKELY(dir == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to open spool directory \"%s\" to track its emails.", spool->Path);
	    goto end;
	    }

	/** Index each Pending email by its queue ID, or its Message-ID until it has one. **/
	while (1)
	    {
	    /** Get the next file. **/
	    errno = 0;
	    entry = readdir(dir);
	    if (entry == NULL)
		{
		if (UNLIKELY(errno != 0))
		    {
		    mssErrorErrno(1, "SMTP", "Failed to read spool directory \"%s\".", spool->Path);
		    goto end;
		    }
		break; /* No more files. */
		}

	    /** Read the struct of each email. **/
	    if (!smtp_internal_IsEmail(entry->d_name))
		continue;
	    if (UNLIKELY(smtp_internal_SpoolPath(path, spool->Path, entry->d_name, ".struct") != 0))
		{
		mssWarnError("Failed to track email \"%s\" in \"%s\", skipping.", entry->d_name, spool->Path);
		continue;
		}
	    found = smtp_internal_ReadStruct(path, &emailStruct);
	    if (UNLIKELY(found < 0))
		{
		mssError(0, "SMTP", "Failed to track email \"%s\".", path);
		goto end;
		}
	    if (found == 0)
		continue; /* No struct yet. */

	    /** Index it if it is Pending. **/
	    status = smtp_internal_StructString(emailStruct, "status");
	    if (status != NULL && strcmp(status, "Pending") == 0)
		{
		key = smtp_internal_StructString(emailStruct, "queue_id");
		if (key != NULL && key[0] != '\0')
		    {
		    if (UNLIKELY(smtp_internal_IndexAdd(&spool->ByQueueID, key, entry->d_name) != 0))
			goto end;
		    }
		else
		    {
		    key = smtp_internal_StructString(emailStruct, "message_id");
		    if (key != NULL && key[0] != '\0'
			&& UNLIKELY(smtp_internal_IndexAdd(&spool->ByMessageID, key, entry->d_name) != 0))
			goto end;
		    }
		}
	    stFreeInf(emailStruct);
	    emailStruct = NULL;
	    }

	/** Success. **/
	spool->Loaded = true;
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    smtp_internal_UnloadSpool(spool);

	if (cursorFile != NULL) fclose(cursorFile);
	if (dir != NULL) closedir(dir);
	if (UNLIKELY(emailStruct != NULL)) stFreeInf(emailStruct);

	return rval;
    }


/*** smtp_internal_SaveCursor - record how much of the mail log a spool
 *** directory has read, so it continues there after a restart.
 ***
 *** @param spool The spool directory.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_SaveCursor(pSmtpSpool spool)
    {
    char path[PATH_MAX];
    char tmpPath[PATH_MAX];
    FILE* cursorFile = NULL;
    bool tmpCreated = false;
    int rval = -1;

	/** Build the paths. **/
	if (UNLIKELY(snprintf(path, sizeof(path), "%s/%s", spool->Path, SMTP_CURSOR_FILE) >= (int)sizeof(path)
	    || snprintf(tmpPath, sizeof(tmpPath), "%s/%s.tmp", spool->Path, SMTP_CURSOR_FILE) >= (int)sizeof(tmpPath)
	))  {
	    mssError(1, "SMTP", "Failed to build the mail log cursor path: \"%s/%s.tmp\" is too long.", spool->Path, SMTP_CURSOR_FILE);
	    goto end;
	    }

	/** Write the cursor, then replace the old one. **/
	cursorFile = fopen(tmpPath, "w");
	if (UNLIKELY(cursorFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to create mail log cursor \"%s\".", tmpPath);
	    goto end;
	    }
	tmpCreated = true;
	if (UNLIKELY(fprintf(cursorFile, "%llu %llu %lld\n",
	    (unsigned long long)spool->LogDev, (unsigned long long)spool->LogIno, (long long)spool->LogOffset
	) < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to write mail log cursor \"%s\".", tmpPath);
	    goto end;
	    }
	if (UNLIKELY(fclose(cursorFile) != 0))
	    {
	    cursorFile = NULL;
	    mssErrorErrno(1, "SMTP", "Failed to write mail log cursor \"%s\".", tmpPath);
	    goto end;
	    }
	cursorFile = NULL;
	if (UNLIKELY(rename(tmpPath, path) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to replace mail log cursor \"%s\".", path);
	    goto end;
	    }
	tmpCreated = false;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(cursorFile != NULL)) fclose(cursorFile);
	if (UNLIKELY(tmpCreated && remove(tmpPath) != 0))
	    fprintf(stderr,
		"Warning: Failed to remove partial mail log cursor (%s): %s.\n",
		tmpPath, strerror(errno)
	    );

	return rval;
    }


/*** smtp_internal_FreeString - free a string.  Matches the free function
 *** signature of xhClear().
 ***
 *** @param str The string.
 *** @param unused Unused.
 *** @returns 0.
 ***/
int
smtp_internal_FreeString(char* str, void* unused)
    {
	nmSysFree(str);

    return 0;
    }


/*** smtp_internal_InitLogBatch - initialize the lines and emails of one read
 *** of the mail log.
 ***
 *** @param batch The batch.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_InitLogBatch(pSmtpLogBatch batch)
    {
    bool linesInitialized = false;
    bool queueIdsInitialized = false;
    bool namesInitialized = false;

	if (UNLIKELY(xaInit(&batch->Lines, 16) != 0))
	    goto error;
	linesInitialized = true;
	if (UNLIKELY(xhInit(&batch->QueueIDs, 257, 0) != 0))
	    goto error;
	queueIdsInitialized = true;
	if (UNLIKELY(xhInit(&batch->ByName, 257, 0) != 0))
	    goto error;
	namesInitialized = true;
	if (UNLIKELY(xaInit(&batch->Emails, 16) != 0))
	    goto error;

	return 0;

    error:
	mssError(1, "SMTP", "Failed to initialize the mail log updates.");
	if (linesInitialized) xaDeInit(&batch->Lines);
	if (queueIdsInitialized) xhDeInit(&batch->QueueIDs);
	if (namesInitialized) xhDeInit(&batch->ByName);

	return -1;
    }


/*** smtp_internal_FreeLogBatch - free the lines and emails of one read of
 *** the mail log.
 ***
 *** @param batch The batch, initialized by smtp_internal_InitLogBatch().
 ***/
void
smtp_internal_FreeLogBatch(pSmtpLogBatch batch)
    {
    pSmtpLogEmail email;
    int i;

	for (i = 0; i < batch->Lines.nItems; i++)
	    nmSysFree((char*)batch->Lines.Items[i]);
	xaDeInit(&batch->Lines);
	xhClear(&batch->QueueIDs, smtp_internal_FreeString, NULL);
	xhDeInit(&batch->QueueIDs);
	for (i = 0; i < batch->Emails.nItems; i++)
	    {
	    email = (pSmtpLogEmail)batch->Emails.Items[i];
	    ASSERTMAGIC(email, MGK_SMTP_LOG_EMAIL);
	    if (email->Struct != NULL) stFreeInf(email->Struct);
	    if (email->Name != NULL) nmSysFree(email->Name);
	    nmFree(email, sizeof(SmtpLogEmail));
	    }
	xhClear(&batch->ByName, NULL, NULL);
	xhDeInit(&batch->ByName);
	xaDeInit(&batch->Emails);

    return;
    }


/*** smtp_internal_GetLogEmail - get an email struct from the spool directory
 *** which includes any updates made from the mail log so far.  The struct is
 *** read from disk and cached for later calls in the same log read batch.
 ***
 *** @param spool The spool directory.
 *** @param batch The current batch, which holds the cache for the current mail
 *** 	log read batch.
 *** @param name The email file name.
 *** @param ret Set to the email, or NULL if it no longer exists.
 *** @returns 0 if the email was found,
 ***          1 if it no longer exists,
 ***         -1 on failure.
 ***/
int
smtp_internal_GetLogEmail(pSmtpSpool spool, pSmtpLogBatch batch, char* name, pSmtpLogEmail* ret)
    {
    pSmtpLogEmail email = NULL;
    char structPath[PATH_MAX];
    int found;
    int rval = -1;

	/** Find the email in cache. **/
	*ret = NULL;
	email = (pSmtpLogEmail)xhLookup(&batch->ByName, name);
	ASSERTMAGIC(email, MGK_SMTP_LOG_EMAIL);
	if (email != NULL)
	    {
	    *ret = email;
	    return 0;
	    }

	/** Not found: read its struct. **/
	email = nmMalloc(sizeof(SmtpLogEmail));
	if (UNLIKELY(email == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate %zu bytes to update email \"%s\".", sizeof(SmtpLogEmail), name);
	    goto end;
	    }
	memset(email, 0, sizeof(SmtpLogEmail));
	SETMAGIC(email, MGK_SMTP_LOG_EMAIL);
	email->Name = nmSysStrdup(name);
	if (UNLIKELY(email->Name == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy email name \"%s\".", name);
	    goto end;
	    }
	if (UNLIKELY(smtp_internal_SpoolPath(structPath, spool->Path, name, ".struct") != 0))
	    goto end;
	found = smtp_internal_ReadStruct(structPath, &email->Struct);
	if (UNLIKELY(found < 0))
	    goto end;
	if (found == 0)
	    {
	    rval = 1; /* Deleted. */
	    goto end;
	    }

	/** Add it to the batch. **/
	if (UNLIKELY(xaAddItem(&batch->Emails, email) < 0))
	    {
	    mssError(1, "SMTP", "Failed to add email \"%s\" to the mail log updates.", name);
	    goto end;
	    }
	if (UNLIKELY(xhAdd(&batch->ByName, email->Name, (char*)email) != 0))
	    {
	    mssError(1, "SMTP", "Failed to index email \"%s\" in the mail log updates.", name);
	    xaRemoveItem(&batch->Emails, batch->Emails.nItems - 1);
	    goto end;
	    }

	/** Success. **/
	*ret = email;
	email = NULL;
	rval = 0;

    end:
	if (UNLIKELY(rval < 0))
	    mssError(0, "SMTP", "Failed to read email \"%s\" in \"%s\" to update it from the mail log.", name, spool->Path);
	if (email != NULL)
	    {
	    if (email->Struct != NULL) stFreeInf(email->Struct);
	    if (email->Name != NULL) nmSysFree(email->Name);
	    nmFree(email, sizeof(SmtpLogEmail));
	    }

	return rval;
    }


/*** smtp_internal_ApplyLogLine - record the result in one line of the mail
 *** log in the struct of the Pending email it is about, if there is one.
 ***
 *** @param spool The spool directory.
 *** @param batch The emails updated so far.
 *** @param line The line, without a newline.  Modified to end the values.
 *** @returns 0 on success (including lines about no email), or -1 on failure.
 ***/
int
smtp_internal_ApplyLogLine(pSmtpSpool spool, pSmtpLogBatch batch, char* line)
    {
    SmtpLogLine parsed;
    pXHashTable index;
    char* key;
    pSmtpIndexEntry entry;
    pSmtpLogEmail email;
    char* value;
    ObjData pod;

	/** Find the email the line is about. **/
	smtp_internal_ParseLogLine(line, &parsed);
	if (parsed.Kind == SMTP_LINE_NONE)
	    return 0;
	index = (parsed.Kind == SMTP_LINE_QUEUED) ? &spool->ByMessageID : &spool->ByQueueID;
	key = (parsed.Kind == SMTP_LINE_QUEUED) ? parsed.MessageID : parsed.QueueID;
	entry = (pSmtpIndexEntry)xhLookup(index, key);
	if (entry == NULL)
	    return 0;
	ASSERTMAGIC(entry, MGK_SMTP_INDEX_ENTRY);
	if (UNLIKELY(smtp_internal_GetLogEmail(spool, batch, entry->Name, &email) < 0))
	    return -1;

	/** Stop tracking emails that were deleted or are no longer Pending. **/
	value = (email != NULL) ? smtp_internal_StructString(email->Struct, "status") : NULL;
	if (value == NULL || strcmp(value, "Pending") != 0)
	    {
	    smtp_internal_IndexRemove(index, key);
	    return 0;
	    }

	/** Record the queue ID of a newly queued email. **/
	value = smtp_internal_StructString(email->Struct, "queue_id");
	if (parsed.Kind == SMTP_LINE_QUEUED)
	    {
	    /** Skip Postfix queuing it again, such as for a .forward file. **/
	    if (value != NULL && value[0] != '\0')
		{
		smtp_internal_IndexRemove(index, key);
		return 0;
		}

	    pod.String = parsed.QueueID;
	    if (UNLIKELY(smtp_internal_StructSet(email->Struct, "queue_id", DATA_T_STRING, &pod) != 0))
		return -1;
	    email->Changed = true;
	    if (UNLIKELY(smtp_internal_IndexAdd(&spool->ByQueueID, parsed.QueueID, email->Name) != 0))
		return -1;
	    smtp_internal_IndexRemove(index, key);
	    return 0;
	    }

	/** Stop tracking a queue ID the email no longer has. **/
	if (value == NULL || strcmp(value, parsed.QueueID) != 0)
	    {
	    smtp_internal_IndexRemove(index, key);
	    return 0;
	    }

	/** Record the result. **/
	switch (parsed.Kind)
	    {
	    case SMTP_LINE_RCPT_COUNT:
		pod.Integer = parsed.RcptCount;
		if (UNLIKELY(smtp_internal_StructSet(email->Struct, "rcpt_count", DATA_T_INTEGER, &pod) != 0))
		    return -1;
		break;

	    case SMTP_LINE_EXPIRED:
		email->Expired = true;
		break;

	    case SMTP_LINE_RCPT:
		if (UNLIKELY(smtp_internal_SetRcpt(email->Struct, parsed.Address, parsed.Status, parsed.Reply) != 0))
		    return -1;
		break;
	    }
	email->Changed = true;

    return 0;
    }


/*** smtp_internal_ReadLogLines - keep copies of the complete lines of a
 *** mail log after an offset that are about the Pending emails of a spool
 *** directory, letting other threads run after each read of up to
 *** SMTP_LOG_READ_SIZE bytes.  Changes neither the indexes nor any email.
 ***
 *** @param spool The spool directory.
 *** @param batch The lines kept so far.
 *** @param log The open mail log.
 *** @param path The path of the mail log.
 *** @param offset The offset to start at.  Set to the end of the last
 ***   complete line read.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_ReadLogLines(pSmtpSpool spool, pSmtpLogBatch batch, pFile log, char* path, off_t* offset)
    {
    SmtpLogLine parsed;
    XString pending; /* Bytes read after the last complete line. */
    XString work;
    bool pendingInitialized = false;
    bool workInitialized = false;
    char* buf = NULL;
    char* line;
    char* newline;
    char* copy = NULL;
    int lineLen;
    int start;
    int n;
    bool keep;
    int rval = -1;

	/** Initialize read buffers. **/
	buf = nmSysMalloc(SMTP_LOG_READ_SIZE);
	if (UNLIKELY(buf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate %d bytes to read the mail log.", SMTP_LOG_READ_SIZE);
	    goto end;
	    }
	if (UNLIKELY(xsInit(&pending) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the mail log line buffer.");
	    goto end;
	    }
	pendingInitialized = true;
	if (UNLIKELY(xsInit(&work) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize the mail log line buffer.");
	    goto end;
	    }
	workInitialized = true;

	/** Start at the offset. **/
	if (UNLIKELY(lseek(fdFD(log), *offset, SEEK_SET) == (off_t)-1))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to seek to offset %lld in the mail log \"%s\".", (long long)*offset, path);
	    goto end;
	    }

	/** Read the log a chunk at a time. **/
	while ((n = fdRead(log, buf, SMTP_LOG_READ_SIZE, 0, 0)) > 0)
	    {
	    if (UNLIKELY(xsConcatenate(&pending, buf, n) < 0))
		{
		mssError(1, "SMTP", "Failed to store %d bytes of the mail log \"%s\".", n, path);
		goto end;
		}

	    /** Handle each complete line. **/
	    start = 0;
	    while ((newline = memchr(pending.String + start, '\n', pending.Length - start)) != NULL)
		{
		line = pending.String + start;
		lineLen = newline - line;
		*newline = '\0';

		/** Parse a copy, since parsing changes the line. **/
		if (UNLIKELY(xsCopy(&work, line, lineLen) != 0))
		    {
		    mssError(1, "SMTP", "Failed to copy a %d byte line of the mail log \"%s\".", lineLen, path);
		    goto end;
		    }
		smtp_internal_ParseLogLine(work.String, &parsed);

		/** Keep the lines about Pending emails, including the queue IDs they claim. **/
		if (parsed.Kind == SMTP_LINE_NONE)
		    keep = false;
		else if (parsed.Kind == SMTP_LINE_QUEUED)
		    keep = (xhLookup(&spool->ByMessageID, parsed.MessageID) != NULL);
		else
		    keep = (xhLookup(&spool->ByQueueID, parsed.QueueID) != NULL || xhLookup(&batch->QueueIDs, parsed.QueueID) != NULL);
		if (keep)
		    {
		    copy = nmSysStrdup(line);
		    if (UNLIKELY(copy == NULL || xaAddItem(&batch->Lines, copy) < 0))
			{
			mssError(1, "SMTP", "Failed to keep a line of the mail log \"%s\": %s", path, line);
			goto end;
			}
		    copy = NULL;
		    }
		if (keep && parsed.Kind == SMTP_LINE_QUEUED && xhLookup(&batch->QueueIDs, parsed.QueueID) == NULL)
		    {
		    copy = nmSysStrdup(parsed.QueueID);
		    if (UNLIKELY(copy == NULL || xhAdd(&batch->QueueIDs, copy, copy) != 0))
			{
			mssError(1, "SMTP", "Failed to track queue ID %s from the mail log \"%s\".", parsed.QueueID, path);
			goto end;
			}
		    copy = NULL;
		    }
		*offset += lineLen + 1;
		start += lineLen + 1;
		}

	    /** Keep the partial line until the rest is read. **/
	    if (start > 0 && UNLIKELY(xsSubst(&pending, 0, start, "", 0) < 0))
		{
		mssError(1, "SMTP", "Failed to drop %d bytes of read lines of the mail log \"%s\".", start, path);
		goto end;
		}

	    /** Let other threads run. **/
	    thYield();
	    }
	if (UNLIKELY(n < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to read the mail log \"%s\".", path);
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(copy != NULL)) nmSysFree(copy);
	if (LIKELY(buf != NULL)) nmSysFree(buf);
	if (LIKELY(pendingInitialized)) xsDeInit(&pending);
	if (LIKELY(workInitialized)) xsDeInit(&work);

	return rval;
    }


/*** smtp_internal_UpdateFromLog - record the results in the lines added to the
 *** mail log since a spool directory last read it in the structs of its
 *** Pending emails, then save where it stopped.  The spool directory stays
 *** locked throughout, so other uses of it wait for the read.
 ***
 *** @param spoolDir The spool directory.
 *** @param rootAttributes The attributes of the SMTP node.
 *** @param throttle Whether to skip the read if one started in the last
 ***   log_read_interval seconds.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtp_internal_UpdateFromLog(char* spoolDir, pXHashTable rootAttributes, bool throttle)
    {
    pSmtpSpool spool = NULL;
    SmtpLogBatch batch;
    bool batchInitialized = false;
    pSmtpLogEmail email;
    XArray rotatedLogs;
    bool rotatedLogsInitialized = false;
    pSmtpRotatedLog rotated;
    char structPath[PATH_MAX];
    char resultPath[PATH_MAX];
    char* status;
    char* queueId;
    struct stat st;
    pFile log = NULL;
    pFile rotatedLog = NULL;
    off_t offset;
    off_t rotatedOffset;
    pSmtpAttribute intervalAttr = NULL;
    int interval = SMTP_DEFAULT_LOG_READ_INTERVAL;
    bool sameLog;
    bool changed;
    bool applied = false;
    bool written = false;
    int found;
    int i;
    int rval = -1;

	/** Throttle reads. **/
	spool = smtp_internal_GetSpool(spoolDir);
	if (UNLIKELY(spool == NULL))
	    return -1; /* Skip error handler, which unlocks the spool directory. */
	if (throttle)
	    {
	    intervalAttr = SMTP_ATTR(xhLookup(rootAttributes, "log_read_interval"));
	    ASSERTMAGIC(intervalAttr, MGK_SMTP_ATTRIBUTE);
	    if (intervalAttr != NULL)
		{
		if (UNLIKELY(intervalAttr->Type != DATA_T_INTEGER))
		    {
		    mssError(1, "SMTP",
			"Attribute 'log_read_interval' must be an integer (got %s).",
			objTypeToStr(intervalAttr->Type)
		    );
		    return -1; /* Skip error handler, which unlocks the spool directory. */
		    }
		interval = intervalAttr->Value.Integer;
		}
	    if (time(NULL) - spool->LastRead < interval)
		return 0; /* Skip error handler, which unlocks the spool directory. */
	    }

	/** Keep other threads and processes out until the read is done. **/
	spool = smtp_internal_LockSpool(spoolDir);
	if (UNLIKELY(spool == NULL))
	    return -1; /* Skip error handler, which unlocks the spool directory. */
	spool->LastRead = time(NULL);

	/** Load the spool directory. **/
	if (!spool->Loaded && UNLIKELY(smtp_internal_LoadSpool(spool) != 0))
	    goto end;

	/** Skip reading a log with nothing new. **/
	if (stat(SMTP_INF.LogPath, &st) != 0)
	    {
	    if (errno == ENOENT)
		{
		rval = 0; /* Mid-rotation, so read the new log next time. */
		goto end;
		}
	    mssErrorErrno(1, "SMTP", "Failed to check the mail log \"%s\".", SMTP_INF.LogPath);
	    goto end;
	    }
	if (spool->HasCursor && st.st_dev == spool->LogDev && st.st_ino == spool->LogIno && st.st_size == spool->LogOffset)
	    {
	    rval = 0;
	    goto end;
	    }

	/** Open the current log. **/
	log = smtp_internal_OpenLog(SMTP_INF.LogPath);
	if (UNLIKELY(log == NULL))
	    goto end;
	if (UNLIKELY(fstat(fdFD(log), &st) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to check the mail log \"%s\".", SMTP_INF.LogPath);
	    goto end;
	    }
	sameLog = (spool->HasCursor && st.st_dev == spool->LogDev && st.st_ino == spool->LogIno);

	/** Initialize the batch. **/
	if (UNLIKELY(smtp_internal_InitLogBatch(&batch) != 0))
	    goto end;
	batchInitialized = true;

	/** Find the rotated log the cursor is in, and each log rotated after it. **/
	if (spool->HasCursor && !sameLog)
	    {
	    if (UNLIKELY(xaInit(&rotatedLogs, 8) != 0))
		{
		mssError(1, "SMTP", "Failed to initialize the list of rotated mail logs.");
		goto end;
		}
	    rotatedLogsInitialized = true;
	    found = smtp_internal_FindRotatedLogs(spool, &rotatedLogs);
	    if (UNLIKELY(found < 0))
		goto end;
	    if (found == 0)
		{
		fprintf(stderr,
		    "Warning: Failed to find the rotated mail log where \"%s\" stopped reading, "
		    "so results Postfix logged there and in later rotated logs are missed.\n",
		    spoolDir
		);
		}

	    /** Finish the cursor's log, then read each later one from the start. **/
	    for (i = 0; i < rotatedLogs.nItems; i++)
		{
		rotated = (pSmtpRotatedLog)rotatedLogs.Items[i];
		rotatedLog = smtp_internal_OpenLog(rotated->Path);
		if (UNLIKELY(rotatedLog == NULL))
		    goto end;
		rotatedOffset = (rotated->IsCursor) ? spool->LogOffset : 0;
		if (UNLIKELY(smtp_internal_ReadLogLines(spool, &batch, rotatedLog, rotated->Path, &rotatedOffset) != 0))
		    goto end;
		fdClose(rotatedLog, 0);
		rotatedLog = NULL;
		}
	    }

	/** Read the current log, from the start if it is new or was truncated. **/
	offset = (sameLog && st.st_size >= spool->LogOffset) ? spool->LogOffset : 0;
	if (UNLIKELY(smtp_internal_ReadLogLines(spool, &batch, log, SMTP_INF.LogPath, &offset) != 0))
	    goto end;

	/** Tell other processes that the Pending emails are changing. **/
	if (batch.Lines.nItems > 0 && UNLIKELY(smtp_internal_BumpSerial(spool) != 0))
	    goto end;

	/** Record each line in its email. **/
	applied = true;
	for (i = 0; i < batch.Lines.nItems; i++)
	    {
	    if (UNLIKELY(smtp_internal_ApplyLogLine(spool, &batch, (char*)batch.Lines.Items[i]) != 0))
		goto end;
	    }

	/** Write the updated emails. **/
	for (i = 0; i < batch.Emails.nItems; i++)
	    {
	    email = (pSmtpLogEmail)batch.Emails.Items[i];
	    ASSERTMAGIC(email, MGK_SMTP_LOG_EMAIL);
	    if (!email->Changed)
		continue;

	    /** Update the status, then write the struct. **/
	    if (UNLIKELY(smtp_internal_SpoolPath(resultPath, spoolDir, email->Name, ".result") != 0
		|| smtp_internal_RefreshStatus(email->Struct, resultPath, email->Expired, rootAttributes, &changed) != 0
		|| smtp_internal_SpoolPath(structPath, spoolDir, email->Name, ".struct") != 0
		|| smtp_internal_WriteStruct(structPath, email->Struct) != 0
	    ))  {
		mssError(0, "SMTP", "Failed to update email \"%s\" in \"%s\" from the mail log.", email->Name, spoolDir);
		goto end;
		}

	    /** Stop tracking a finished email. **/
	    status = smtp_internal_StructString(email->Struct, "status");
	    queueId = smtp_internal_StructString(email->Struct, "queue_id");
	    if (status != NULL && strcmp(status, "Pending") != 0 && queueId != NULL)
		smtp_internal_IndexRemove(&spool->ByQueueID, queueId);
	    }

	/** Save where the read stopped. **/
	written = true;
	spool->LogDev = st.st_dev;
	spool->LogIno = st.st_ino;
	spool->LogOffset = offset;
	spool->HasCursor = true;
	if (UNLIKELY(smtp_internal_SaveCursor(spool) != 0))
	    goto end;

	/** Success. **/
	rval = 0;

    end:
	/** Reload the indexes from the structs if recording failed partway. **/
	if (UNLIKELY(rval != 0 && applied && !written))
	    smtp_internal_UnloadSpool(spool);

	if (log != NULL) fdClose(log, 0);
	if (rotatedLog != NULL) fdClose(rotatedLog, 0);
	if (rotatedLogsInitialized)
	    {
	    smtp_internal_FreeRotatedLogs(&rotatedLogs);
	    xaDeInit(&rotatedLogs);
	    }
	if (batchInitialized) smtp_internal_FreeLogBatch(&batch);
	smtp_internal_UnlockSpool(spool);

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
    pXString messageId = NULL;

    pSmtpAttribute hostName = NULL;

    pStructInf emailStruct = NULL;
    pStructInf createdStruct = NULL;
    pSmtpAttribute currentAttr = NULL;
    pDateTime attrDate = NULL;

    pFile checkFile = NULL;
    pFile emailFile = NULL;
    pFile emailStructFile = NULL;
    ObjData pod;
    int i;
    unsigned char email_id[8];
    unsigned char message_key[8];
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
		    "Failed to auto-generate a unique filename. May have exceeded allowable range of filenames."
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
		mssError(1, "SMTP", "Failed to get default attribute %d.", i);
		goto end;
		}
	    ASSERTMAGIC(currentAttr, MGK_SMTP_ATTRIBUTE);

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emailStruct, currentAttr->Name);
	    if (UNLIKELY(createdStruct == NULL))
		{
		mssError(1, "SMTP",
		    "Failed to add new attribute (%s) to the email struct.",
		    currentAttr->Name
		);
		goto end;
		}

	    /** Set the default attribute value. **/
	    if (UNLIKELY(stSetAttrValue(createdStruct, currentAttr->Type, &currentAttr->Value, 0) != 0))
		{
		mssError(1, "SMTP",
		    "Failed to write to the default attribute (%s).",
		    currentAttr->Name
		);
		goto end;
		}
	    }

	/** Add dynamic attributes which have object specific defaults. **/
	/** Find the host name for the Message-ID. **/
	hostName = SMTP_ATTR(xhLookup(inf->RootAttributes, "local_host_name"));
	ASSERTMAGIC(hostName, MGK_SMTP_ATTRIBUTE);
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
	    {
	    strtcpy(local_host_name, "localhost.localdomain", sizeof(local_host_name));
	    fprintf(stderr,
		"Warning: gethostname() failed (%s); using \"%s\".\n",
		strerror(errno), local_host_name
	    );
	    }
	local_host_name[sizeof(local_host_name) - 1] = '\0'; /* Terminate a truncated name. */

	/** Generate a random Message-ID, so it is unique across spools. **/
	if (UNLIKELY(cxssGenerateKey(message_key, sizeof(message_key)) < 0))
	    {
	    mssError(1, "SMTP", "Failed to generate a random Message-ID.");
	    goto end;
	    }
	messageId = xsNew();
	if (UNLIKELY(messageId == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate an xstring for the Message-ID.");
	    goto end;
	    }
	if (UNLIKELY(xsQPrintf(messageId, "%8STR&HEX@%STR",
	    message_key, (hostName) ? hostName->Value.String : local_host_name
	) < 0))
	    {
	    mssError(1, "SMTP", "Failed to format a random Message-ID.");
	    goto end;
	    }

	/** Create the message_id attribute. **/
	createdStruct = stAddAttr(emailStruct, "message_id");
	if (UNLIKELY(createdStruct == NULL))
	    {
	    mssError(1, "SMTP", "Failed to add new attribute (message_id) to the email struct.");
	    goto end;
	    }

	/** Set the default message_id value. **/
	pod.String = messageId->String;
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_STRING, &pod, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to write to the default attribute (message_id).");
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
	    mssError(1, "SMTP", "Failed to add new attribute (expire_date) to the email struct.");
	    goto end;
	    }

	/** Set the default expire_date value. **/
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to write to the default attribute (expire_date).");
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
		"Failed to add new attribute (first_try_date) to the email struct."
	    );
	    goto end;
	    }

	/** Set the default first_try_date value. **/
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to write to the default attribute (first_try_date).");
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
	    mssError(1, "SMTP", "Failed to add new attribute (last_try_date) to the email struct.");
	    goto end;
	    }

	/** Set the default last_try_date value. **/
	if (UNLIKELY(stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to write to the default attribute (last_try_date).");
	    goto end;
	    }
	nmFree(attrDate, sizeof(DateTime));
	attrDate = NULL;

	/** Create the struct file with the permissions of the email, without execute. **/
	emailStructFile = fdOpen(inf->EmailStructPath.String, O_CREAT | O_RDWR | O_EXCL, inf->Mask & 0666);
	if (UNLIKELY(emailStructFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP",
		"Failed to create the email struct file (%s).",
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
	if (LIKELY(messageId != NULL)) xsFree(messageId);
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
		    "Failed to create new node: it exists and CREAT and EXCL flags are set."
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
    pSmtpSpool spool = NULL;
    char* status = NULL;
    bool changed;
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
	    mssError(1, "SMTP", "Failed to copy spool directory path into the email path.");
	    goto end;
	    }

	if (UNLIKELY(xsConcatPrintf(&inf->EmailPath, "/%s", inf->Name) < 0))
	    {
	    mssError(1, "SMTP", "Failed to append email name \"%s\" to email path.", inf->Name);
	    goto end;
	    }

	/** Keep other threads and processes out until the email is loaded. **/
	spool = smtp_internal_LockSpool(spoolDir->Value.String);
	if (UNLIKELY(spool == NULL))
	    goto end;

	/** Check that the email file exists. **/
	fd = fdOpen(inf->EmailPath.String, 0, 0);
	if (UNLIKELY(fd == NULL))
	    {
	    /** Create the file if it doesn't exist and the create flag is set. **/
	    if (inf->Obj->Mode & OBJ_O_CREAT)
		{
		/** Sweep the spool dir to clean up expired emails. **/
		smtp_internal_SweepSpool(spoolDir->Value.String, inf->RootAttributes);

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

	/** Record the results Postfix logged since the last read. **/
	if (UNLIKELY(smtp_internal_UpdateFromLog(spoolDir->Value.String, inf->RootAttributes, true) != 0))
	    mssWarnError("Failed to update the emails in \"%s\" from the mail log.", spoolDir->Value.String);

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

	/** Record which struct file the attributes come from. **/
	if (UNLIKELY(fstat(fdFD(emailStructureFile), &inf->StructInfo) != 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to check email struct file \"%s\".", inf->EmailStructPath.String);
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

	/** Update the send status of a Pending email from its sendmail result and the timeout. **/
	status = smtp_internal_StructString(emailStructure, "status");
	if (status != NULL && strcmp(status, "Pending") == 0)
	    {
	    if (UNLIKELY(smtp_internal_RefreshStatus(emailStructure, inf->ResultPath.String, false, inf->RootAttributes, &changed) != 0
		|| (changed && smtp_internal_WriteStruct(inf->EmailStructPath.String, emailStructure) != 0)
	    ))  {
		mssWarnError("Failed to update the send status of email \"%s\".", inf->Name);
		}
	    else if (changed && UNLIKELY(stat(inf->EmailStructPath.String, &inf->StructInfo) != 0))
		{
		mssErrorErrno(1, "SMTP", "Failed to check email struct file \"%s\".", inf->EmailStructPath.String);
		goto end;
		}
	    }

	/** Get the structure's attributes **/
	if (UNLIKELY(smtp_internal_GetStructAttributes(emailStructure, inf->Attributes, inf->AttributeNames) != 0))
	    {
	    mssError(0, "SMTP", "Failed to load email attributes.");
	    goto end;
	    }

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
	if (spool != NULL) smtp_internal_UnlockSpool(spool);

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

	/** Allocate driver struct. **/
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

	/** Free the attribute names. **/
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
		mssError(1, "SMTP", "Failed to close email file (%s).", inf->EmailPath.String);
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
		mssError(1, "SMTP", "Failed to create root node because it already exists.");
		goto error;
		}

	    node = smtp_internal_CreateRootNode(obj, mask);
	    if (UNLIKELY(node == NULL))
		{
		mssError(0, "SMTP", "Failed to create root node.");
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
    pSmtpSpool spool = NULL;
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
	    /** Keep other threads and processes out until the email is deleted. **/
	    spool = smtp_internal_LockSpool(smtp_internal_GetString(inf->RootAttributes, "spool_dir"));
	    if (UNLIKELY(spool == NULL))
		goto end;

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
	if (spool != NULL) smtp_internal_UnlockSpool(spool);
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
    pSmtpSpool spool;
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
	    mssError(1, "SMTP", "Failed to read content from smtp object of type %d.", inf->Type);
	    return -1;
	    }

	/** Refuse reads from a write-only email. **/
	if (UNLIKELY((inf->Obj->Mode & O_ACCMODE) == O_WRONLY))
	    {
	    mssError(1, "SMTP", "Failed to read email that was opened write-only.");
	    return -1;
	    }

	/** Keep other threads and processes from changing the email while it is read. **/
	spool = smtp_internal_LockSpool(smtp_internal_GetString(inf->RootAttributes, "spool_dir"));
	if (UNLIKELY(spool == NULL))
	    return -1;

	rval = fdRead(inf->ContentFile, buffer, maxcnt, offset, flags);
	if (UNLIKELY(rval < 0))
	    mssErrorErrno(1, "SMTP",
		"Failed to read %d bytes at offset %d from email file (%s).",
		maxcnt, offset, inf->EmailPath.String
	    );
	smtp_internal_UnlockSpool(spool);

	return rval;
    }


/*** smtpWrite - Write to the SMTP object
 ***/
int
smtpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpSpool spool;
    int pending;
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
	    mssError(1, "SMTP", "Failed to write content to smtp object of type %d.", inf->Type);
	    return -1;
	    }

	/** Refuse writes to a read-only email. **/
	if (UNLIKELY((inf->Obj->Mode & O_ACCMODE) == O_RDONLY))
	    {
	    mssError(1, "SMTP", "Failed to write to email that was opened read-only.");
	    return -1;
	    }

	/** Keep other threads and processes out until the content is written. **/
	spool = smtp_internal_LockSpool(smtp_internal_GetString(inf->RootAttributes, "spool_dir"));
	if (UNLIKELY(spool == NULL))
	    return -1;

	/** Refuse to change a Pending email, which sendmail may be reading. **/
	pending = smtp_internal_IsPending(inf->EmailStructPath.String);
	if (UNLIKELY(pending != 0))
	    {
	    if (pending > 0)
		mssError(1, "SMTP", "Failed to write to \"%s\": it is Pending.", inf->Name);
	    else
		mssError(0, "SMTP", "Failed to check whether \"%s\" is Pending.", inf->Name);
	    smtp_internal_UnlockSpool(spool);
	    return -1;
	    }

	rval = fdWrite(inf->ContentFile, buffer, cnt, offset, flags);
	if (UNLIKELY(rval < 0))
	    mssErrorErrno(1, "SMTP",
		"Failed to write %d bytes at offset %d to email file (%s).",
		cnt, offset, inf->EmailPath.String
	    );
	smtp_internal_UnlockSpool(spool);

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
	    mssError(1, "SMTP", "Failed to allocate query object.");
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
	    smtp_internal_SweepSpool(spoolPath, inf->Attributes);

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
	    mssError(1, "SMTP", "Failed to query system/smtp-message type objects.");
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
		mssError(1, "SMTP", "Failed to create smtp data object.");
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
	    mssError(1, "SMTP", "Failed to query smtp-message data objects.");
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
		mssErrorErrno(1, "SMTP", "Failed to close spool directory.");
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

	/** Edge cases. **/
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

	/** Reload the attributes if the struct was replaced. **/
	if (UNLIKELY(smtp_internal_ReloadAttributes(inf, true) != 0))
	    mssWarnError("Failed to reload the attributes of email \"%s\", using the loaded ones.", inf->Name);

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
	    mssError(1, "SMTP", "Failed to get attribute '%s' of a NULL object.", attrname);
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

	/** Reload the attributes if the struct was replaced. **/
	if (UNLIKELY(smtp_internal_ReloadAttributes(inf, true) != 0))
	    mssWarnError("Failed to reload the attributes of email \"%s\", using the loaded ones.", inf->Name);

	/** Get the value of the stored attribute. **/
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
			"Failed to get attribute '%s' of unsupported type %s.",
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

	/** Reload the attributes if the struct was replaced. **/
	if (UNLIKELY(smtp_internal_ReloadAttributes(inf, true) != 0))
	    mssWarnError("Failed to reload the attributes of email \"%s\", using the loaded ones.", inf->Name);

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
    pStructInf emlStruct = NULL;
    pSmtpSpool spool = NULL;

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
		mssError(0, "SMTP", "Failed to create the requested attribute '%s'.", attrname);
		goto end;
		}

	    /** Get the newly created attribute. **/
	    attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	    if (UNLIKELY(attr == NULL))
		{
		mssError(1, "SMTP", "Failed to open the requested attribute '%s'.", attrname);
		goto end;
		}
	    }
	ASSERTMAGIC(attr, MGK_SMTP_ATTRIBUTE);

	/** Check the requested datatype. **/
	if (attr->Type != datatype)
	    {
	    mssError(1, "SMTP",
		"Attempt to assign invalid data type to attribute '%s'. (Assigning %s to %s)",
		attrname, objTypeToStr(datatype), objTypeToStr(attr->Type)
	    );
	    goto end;
	    }

	/** We don't yet support null values **/
	if (UNLIKELY(val == NULL))
	    {
	    mssError(1, "SMTP", "Failed to set attribute '%s' to NULL (not supported).", attrname);
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
		mssError(0, "SMTP", "Failed to open root node for writing.");
		goto end;
		}
	    ASSERTMAGIC(rootNode, MGK_STNODE);

	    /** Set the attribute value in the root node. **/
	    attrStruct = stLookup(rootNode->Data, attrname);
	    if (UNLIKELY(attrStruct == NULL))
		{
		mssError(1, "SMTP", "Attribute '%s' not found in the root node.", attrname);
		goto end;
		}
	    if (UNLIKELY(stSetAttrValue(attrStruct, datatype, val, 0) != 0))
		{
		mssError(1, "SMTP", "Failed to write to attribute '%s'.", attrname);
		goto end;
		}

	    /** Mark root node DIRTY so that it will be written. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node back to the OS tree. **/
	    if (UNLIKELY(snWriteNode(inf->Obj->Prev, rootNode) != 0))
		{
		mssError(0, "SMTP", "Failed to write data to the root node.");
		goto end;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Keep other threads and processes out until the struct is written. **/
	    spool = smtp_internal_LockSpool(smtp_internal_GetString(inf->RootAttributes, "spool_dir"));
	    if (UNLIKELY(spool == NULL))
		goto end;

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
		mssError(1, "SMTP", "Attribute '%s' not found in the email structure file.", attrname);
		goto end;
		}
	    if (UNLIKELY(stSetAttrValue(attrStruct, datatype, val, 0) < 0))
		{
		mssError(1, "SMTP", "Failed to set attribute '%s'.", attrname);
		goto end;
		}

	    /** Done reading. **/
	    if (emlStructFileRead)
		{
		fdClose(emlStructFileRead, 0);
		emlStructFileRead = NULL;
		}

	    /** Write changes to the email struct file. **/
	    if (UNLIKELY(smtp_internal_WriteStruct(inf->EmailStructPath.String, emlStruct) != 0))
		goto end;
	    smtp_internal_UnlockSpool(spool);
	    spool = NULL;

	    /** If the email is ready to send, send it. **/
	    if (strcmp(attrname, "is_ready") == 0 && val->Integer == 1)
		{
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
	if (emlStruct != NULL) stFreeInf(emlStruct);
	if (spool != NULL) smtp_internal_UnlockSpool(spool);

	return rval;
    }


/*** smtpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
smtpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpSpool spool;
    int pending;
    int rval = -1;

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

	/** Reload the attributes if the struct was replaced. **/
	if (inf != NULL && UNLIKELY(smtp_internal_ReloadAttributes(inf, true) != 0))
	    mssWarnError("Failed to reload the attributes of email \"%s\", using the loaded ones.", inf->Name);

	/** Refuse to send an email that cannot be tracked. **/
	if (UNLIKELY(inf != NULL && inf->Type == SMTP_T_EML && strcmp(attrname, "is_ready") == 0
	    && datatype == DATA_T_INTEGER && val != NULL && val->Integer == 1))
	    {
	    char* messageId = smtp_internal_GetString(inf->Attributes, "message_id");
	    if (messageId == NULL || messageId[0] == '\0')
		{
		mssError(1, "SMTP",
		    "Failed to send \"%s\": it has no message_id, so its send status cannot be tracked.",
		    inf->Name
		);
		return -1;
		}
	    }

	/** Refuse to change a Pending email.  Sending checks is_ready itself. **/
	if (inf != NULL && inf->Type == SMTP_T_EML && strcmp(attrname, "is_ready") != 0)
	    {
	    spool = smtp_internal_LockSpool(smtp_internal_GetString(inf->RootAttributes, "spool_dir"));
	    if (UNLIKELY(spool == NULL))
		return -1;
	    pending = smtp_internal_IsPending(inf->EmailStructPath.String);
	    if (LIKELY(pending == 0))
		rval = smtp_internal_SetAttrValue(inf_v, attrname, datatype, val, oxt);
	    else if (pending > 0)
		mssError(1, "SMTP",
		    "Failed to set attribute '%s' of \"%s\": it is Pending.",
		    attrname, inf->Name
		);
	    else
		mssError(0, "SMTP", "Failed to check whether \"%s\" is Pending.", inf->Name);
	    smtp_internal_UnlockSpool(spool);
	    return rval;
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
    pStructInf emlStruct = NULL;
    pSmtpSpool spool = NULL;
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
		    "Failed to add attribute '%s' of unsupported type %s.",
		    attr->Name, objTypeToStr(attr->Type)
		);
		goto end;
	    }

	/** Add the attribute to the attribute hash and the attribute name list. **/
	if (UNLIKELY(xhAdd(inf->Attributes, attr->Name, (char*)attr) != 0))
	    {
	    mssError(1, "SMTP", "Failed to add attribute '%s' (it may be a duplicate).", attr->Name);
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
		mssError(0, "SMTP", "Failed to open root node.");
		goto end;
		}
	    ASSERTMAGIC(rootNode, MGK_STNODE);

	    /** Add the attribute to the root node. **/
	    createdStruct = stAddAttr(rootNode->Data, attr->Name);
	    if (UNLIKELY(createdStruct == NULL))
		{
		mssError(1, "SMTP", "Failed to add new attribute '%s' to the root node.", attr->Name);
		goto end;
		}

	    /** Set the default attribute value. **/
	    if (UNLIKELY(stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0) != 0))
		{
		mssError(1, "SMTP", "Failed to write to attribute '%s'.", attr->Name);
		goto end;
		}

	    /** Set the root node to DIRTY so it will be written to the file. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node. **/
	    if (UNLIKELY(snWriteNode(inf->Obj->Prev, rootNode) != 0))
		{
		mssError(0, "SMTP", "Failed to write root node.");
		goto end;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Keep other threads and processes out until the struct is written. **/
	    spool = smtp_internal_LockSpool(smtp_internal_GetString(inf->RootAttributes, "spool_dir"));
	    if (UNLIKELY(spool == NULL))
		goto end;

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

	    /** Refuse an attribute the struct already has. **/
	    if (UNLIKELY(stLookup(emlStruct, attr->Name) != NULL))
		{
		mssError(1, "SMTP",
		    "Failed to add attribute '%s' to the email struct: it already exists.",
		    attr->Name
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
		mssError(1, "SMTP", "Failed to write to attribute '%s'.", attr->Name);
		goto end;
		}

	    /** Done reading. **/
	    fdClose(emlStructFileRead, 0);
	    emlStructFileRead = NULL;

	    /** Write changes to the email struct file. **/
	    if (UNLIKELY(smtp_internal_WriteStruct(inf->EmailStructPath.String, emlStruct) != 0))
		goto end;
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
	if (emlStruct != NULL) stFreeInf(emlStruct);
	if (spool != NULL) smtp_internal_UnlockSpool(spool);

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

	/** Reload the attributes if the struct was replaced. **/
	if (inf != NULL && UNLIKELY(smtp_internal_ReloadAttributes(inf, true) != 0))
	    mssWarnError("Failed to reload the attributes of email \"%s\", using the loaded ones.", inf->Name);

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


/*** smtpGetFirstMethod - get the first method name for this object.  The
 *** root node has read_mail_log, and emails have no methods.
 ***/
char*
smtpGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Failed to get first method from NULL smtp object.");
	    return NULL;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);

    return (inf->Type == SMTP_T_ROOT) ? "read_mail_log" : NULL;
    }


/*** smtpGetNextMethod - get the next method name for this object.  No
 *** object has more than one method, so this always returns NULL.
 ***/
char*
smtpGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** smtpExecuteMethod - execute a method of this object.  The root node's
 *** read_mail_log records the results in the lines added to the mail log
 *** since its spool directory last read it, even if it read it recently.
 ***
 *** @param methodname The method to execute.
 *** @param param Unused.
 *** @returns 0 on success, or -1 on failure.
 ***/
int
smtpExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute spoolDir = NULL;

	/** Edge cases. **/
	if (UNLIKELY(inf == NULL || methodname == NULL))
	    {
	    mssError(1, "SMTP", "Failed to execute a method: the smtp object or method name is NULL.");
	    return -1;
	    }
	ASSERTMAGIC(inf, MGK_SMTP_DATA);
	if (UNLIKELY(inf->Type != SMTP_T_ROOT || strcmp(methodname, "read_mail_log") != 0))
	    {
	    mssError(1, "SMTP", "Failed to execute method '%s' of \"%s\": no such method.", methodname, inf->Name);
	    return -1;
	    }

	/** Read the mail log now. **/
	spoolDir = SMTP_ATTR(xhLookup(inf->Attributes, "spool_dir"));
	ASSERTMAGIC(spoolDir, MGK_SMTP_ATTRIBUTE);
	if (UNLIKELY(spoolDir == NULL || spoolDir->Type != DATA_T_STRING))
	    {
	    mssError(1, "SMTP", "Failed to read the mail log for \"%s\": the SMTP node does not have a 'spool_dir' string.", inf->Name);
	    return -1;
	    }
	if (UNLIKELY(smtp_internal_UpdateFromLog(spoolDir->Value.String, inf->Attributes, false) != 0))
	    {
	    mssError(0, "SMTP", "Failed to update the emails in \"%s\" from the mail log.", spoolDir->Value.String);
	    return -1;
	    }

    return 0;
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
    bool typesInitialized = false;

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

	/** Set up the structure **/
	strcpy(drv->Name,"SMTP - Simple Mail Transfer Protocol OS Driver");
	drv->Capabilities = 0;
	if (UNLIKELY(xaInit(&(drv->RootContentTypes),1) != 0))
	    {
	    mssError(1, "SMTP", "Failed to set up root content types.");
	    goto error;
	    }
	typesInitialized = true;
	if (UNLIKELY(xaAddItem(&(drv->RootContentTypes),"system/smtp") < 0))
	    {
	    mssError(1, "SMTP", "Failed to add root content type \"system/smtp\".");
	    goto error;
	    }

	/** Set up the function references. **/
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
	nmRegister(sizeof(SmtpIndexEntry), "SmtpIndexEntry");
	nmRegister(sizeof(SmtpLogEmail), "SmtpLogEmail");

	/** Register the driver **/
	if (UNLIKELY(objRegisterDriver(drv) < 0))
	    {
	    mssError(0, "SMTP", "Failed to register the driver.");
	    goto error;
	    }

	return 0;

    error:
	mssError(0, "SMTP", "Failed to initialize the SMTP driver.");

	if (typesInitialized) xaDeInit(&(drv->RootContentTypes));
	if (drv != NULL) nmFree(drv, sizeof(ObjDriver));

	return -1;
    }

MODULE_INIT(smtpInitialize);
MODULE_PREFIX("smtp");
MODULE_DESC("SMTP ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);
