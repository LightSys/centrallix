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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "centrallix.h"
#include "cxlib/expect.h"
#include "cxlib/xarray.h"
#include "obj.h"
#include "st_node.h"


/** Debugging mode **/
#define	SMTP_DEBUG	1

/** Define types of SMTP objects. **/
#define SMTP_T_ROOT	0
#define SMTP_T_EML	1

/*** Structure to store attribute information. ***/
typedef struct
    {
    char*	Name;
    int		Type; /* DATA_T_xxx */
    ObjData	Value;
    }
    SmtpAttribute, *pSmtpAttribute;

#define SMTP_ATTR(x) ((pSmtpAttribute)(x))


/*** Structure used by this driver internally. ***/
typedef struct
    {
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
    pFile		ContentFile;
    XString		EmailPath;
    XString		EmailStructPath;
    }
    SmtpData, *pSmtpData;

#define SMTP(x) ((pSmtpData)(x))


/*** Structure used by queries in this driver. ***/
typedef struct
    {
    pSmtpData	Data;
    DIR*	Directory;
    }
    SmtpQueryData, *pSmtpQueryData;

#define SMTP_QY(x) ((pSmtpQueryData)(x))


/*** Global data structure for the SMTP module. ***/
struct
    {
    XArray		DefaultRootAttributes;		/* XArray of pSmtpAttribute */
    XArray		DefaultEmailAttributes;		/* XArray of pSmtpAttribute */
    XArray		DefaultEmailHeaders;		/* XArray of pSmtpAttribute */
    }
    SMTP_INF;


/** Forward declarations for functions that need them. **/
int smtp_internal_Close(pSmtpData inf);
int smtpQueryClose(void* qy_v, pObjTrxTree* oxt);
int smtpAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt);


/*** smtp_internal_SpawnSendmail - launch the sendmail process to actually
 *** send off an email message.  This also works with Postfix, via its
 *** "sendmail compatibility interface".
 ***/
int
smtp_internal_SpawnSendmail(char* emailPath, pSmtpAttribute envFrom, pSmtpAttribute envTo)
    {
    int pid, fd, maxfiles;
    pXArray argv = NULL;
    char *envp[] = {NULL};
    int wstatus;
    int rval = -1;

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
		mssError(1, "SMTP", "Failed to add envelope from '%s' to the sendmail argument list.", envFrom->Value.String);
		goto end;
		}
	    }
	if (envTo && envTo->Value.String[0] != '\0')
	    {
	    if (UNLIKELY(xaAddItem(argv, envTo->Value.String) < 0))
		{
		mssError(1, "SMTP", "Failed to add envelope to '%s' to the sendmail argument list.", envTo->Value.String);
		goto end;
		}
	    }

	if (UNLIKELY(xaAddItem(argv, NULL) < 0))
	    {
	    mssError(1, "SMTP", "Failed to terminate the sendmail argument list.");
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
		fprintf(stderr, "Warning: sysconf(_SC_OPEN_MAX) returned %d; using maxfiles=2048.\n", maxfiles);
		maxfiles = 2048;
		}

	    for(fd=3;fd<maxfiles;fd++) close(fd);

	    /** Open the email. **/
	    fd = open(emailPath, O_RDONLY);
	    if (UNLIKELY(fd < 0))
		{
		fprintf(stderr, "SMTP: Could not open email file (%s) for sendmail. (%s)\n", emailPath, strerror(errno));
		_exit(EXIT_FAILURE);
		}

	    /** Hopefully this makes our file stdin so we don't have to cat it into sendmail. **/
	    if (UNLIKELY(dup2(fd, 0) < 0))
		{
		fprintf(stderr, "SMTP: Could not redirect email file (%s) to stdin for sendmail. (%s)\n", emailPath, strerror(errno));
		_exit(EXIT_FAILURE);
		}

	    /** NOTE: We're currently double forking to get rid of zombie processes. **/
	    /** TODO: Change this to look at the return value of sendmail and act accordingly. **/
	    pid = fork();
	    if (UNLIKELY(pid < 0))
		{
		fprintf(stderr, "SMTP: Unable to fork (2). (%s)\n", strerror(errno));
		_exit(EXIT_FAILURE);
		}
	    if (pid == 0)
		{
		/** we're in the child process -- disable MTask context switches to be safe **/
		thLock();

		/** close all open fds (except for 0-2 -- std{in,out,err}) **/
		maxfiles = sysconf(_SC_OPEN_MAX);
		if (maxfiles <= 0)
		    {
		    fprintf(stderr, "Warning: sysconf(_SC_OPEN_MAX) returned %d; using maxfiles=2048.\n", maxfiles);
		    maxfiles = 2048;
		    }

		for(fd=3;fd<maxfiles;fd++) close(fd);

		/** Execve. **/
		execve("/usr/sbin/sendmail", (char**)(argv->Items), envp);

		/** if execve() is successful, this is never reached **/
		fprintf(stderr, "SMTP: execve(\"/usr/sbin/sendmail\") failed: \"%s\"\n", strerror(errno));
		_exit(EXIT_FAILURE);
		}
	    else
		{
		/** We're the parent. Exit so centrallix can move on. **/
		_exit(EXIT_SUCCESS);
		}
	    }

	/** Get status of child process, releasing it from the process table. **/
	int wait_rval = waitpid(pid, &wstatus, WNOHANG);
	if (wait_rval == 0)
	    {
	    /** Try again in 1 msec if not immediately ready to reap; this lets
	     ** us yield to other threads.
	     **/
	    thSleep(1);
	    wait_rval = waitpid(pid, &wstatus, 0);
	    }
	if (UNLIKELY(wait_rval < 0))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to wait for child sendmail process (pid %d).", pid);
	    goto end;
	    }
	if (UNLIKELY(WEXITSTATUS(wstatus) != EXIT_SUCCESS))
	    {
	    mssError(1, "SMTP", "Failed to start child sendmail process (%d)", WEXITSTATUS(wstatus));
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to spawn sendmail for email file (%s).", emailPath);

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
	    mssError(1, "SMTP", "Failed to allocate %d bytes for an attribute.", (int)sizeof(SmtpAttribute));
	    goto error;
	    }
	memset(inf, 0, sizeof(SmtpAttribute));

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
	    mssError(1, "SMTP", "Unsupported attribute type %d or missing string value.", type);
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


/*** smtp_internal_InitGlobals - Initializes global information for the SMTP
 *** driver.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_InitGlobals()
    {
    char local_host_name[HOST_NAME_MAX];

	/** Initialize the global attributes. **/
	if (UNLIKELY(xaInit(&SMTP_INF.DefaultRootAttributes, 16) != 0
	    || xaInit(&SMTP_INF.DefaultEmailAttributes, 16) != 0
	    || xaInit(&SMTP_INF.DefaultEmailHeaders, 16) != 0
	))   {
	    mssError(1, "SMTP", "Failed to initialize default attribute lists.");
	    goto error;
	    }

	/** Add all the required attributes. Yay hardcoding! **/
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
	    {
	    strtcpy(local_host_name, "localhost.localdomain", sizeof(local_host_name));
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

	/** Add all the required email attributes. Behold the hard code; standeth it against all but the hardest hammer. **/
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "envelope_from",	DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "envelope_to",		DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "status",		DATA_T_STRING,	0,	"Draft") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "is_ready",		DATA_T_INTEGER,	0,	0) < 0)) goto error;
	/** Not strictly necessary. **/
	/** xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("try_count",	DATA_T_INTEGER,	5,	0)); **/
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "last_try_status",	DATA_T_STRING,	0,	"None") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailAttributes, "last_try_msg",		DATA_T_STRING,	0,	"") < 0)) goto error;


	/** Add all the default headers for an email file. **/
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailHeaders, "User-Agent",		DATA_T_STRING,	0,	"Centrallix/" PACKAGE_VERSION) < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailHeaders, "Subject",			DATA_T_STRING,	0,	"") < 0)) goto error;
	if (UNLIKELY(smtp_internal_AddDefault(&SMTP_INF.DefaultEmailHeaders, "MIME-Version",		DATA_T_STRING,	0,	"1.0") < 0)) goto error;

	return 0;

    error:
	mssError(0, "SMTP", "Failed to initialize SMTP driver globals.");
	return -1;
    }


/*** smtp_internal_IsEmail - Returns 1 if the filename is an email.
 ***/
int
smtp_internal_IsEmail(char* filename)
    {
    int l = strlen(filename);
    return l >= 4 && (strcmp(filename + l - 4, ".msg") == 0 || strcmp(filename + l - 4, ".eml") == 0);
    }


/*** smtp_internal_GetStructAttributes - Loads the attributes from the node into
 *** the SMTP object.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_GetStructAttributes(pStructInf structInf, pSmtpData inf)
    {
    pSmtpAttribute attr = NULL;
    pStructInf currentAttr = NULL;
    int i;
    pDateTime dt;

	for (i = 0; i < structInf->nSubInf; i++)
	    {
	    currentAttr = structInf->SubInf[i];

	    attr = nmMalloc(sizeof(SmtpAttribute));
	    if (UNLIKELY(attr == NULL))
		{
		mssError(1,"SMTP","Could not create new attribute object.");
		goto error;
		}
	    memset(attr, 0, sizeof(SmtpAttribute));

	    attr->Name = nmSysStrdup(currentAttr->Name);
	    if (UNLIKELY(attr->Name == NULL))
		{
		mssError(1, "SMTP", "Failed to copy attribute name.");
		goto error;
		}
	    attr->Type = currentAttr->Value->DataType;

	    if (currentAttr->Value->DataType == DATA_T_STRING && (strcmp(attr->Name, "expire_date") == 0 || strcmp(attr->Name, "last_try_date") == 0))
		{
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
			mssError(1, "SMTP", "Failed to allocate %d bytes for a date.", (int)sizeof(DateTime));
			goto error;
			}
		    if (UNLIKELY(objDataToDateTime(DATA_T_STRING, dateStr, dt, NULL) != 0))
			{
			mssError(0, "SMTP", "Failed to parse date \"%s\".", dateStr);
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
		mssError(1, "SMTP", "Unsupported attribute type %d in email data file", currentAttr->Value->DataType);
		goto error;
		}

	    /** Store the attribute. **/
	    if (UNLIKELY(xhAdd(inf->Attributes, attr->Name, (char*)attr) != 0))
		{
		mssError(1, "SMTP", "Failed to add attribute (it may be a duplicate).");
		goto error;
		}
	    if (UNLIKELY(xaAddItem(inf->AttributeNames, attr->Name) < 0))
		{
		mssError(1, "SMTP", "Failed to add attribute name to list.");
		xhRemove(inf->Attributes, attr->Name);
		goto error;
		}
	    }

	return 0;

    error:
	mssError(0, "SMTP", "Failed to load attribute #%d/%d (%s).", i, structInf->nSubInf, currentAttr->Name);

	if (attr != NULL) smtp_internal_ClearAttribute((char*)attr, NULL);

	return -1;
    }


/*** smtp_internal_SendEmail - fire off the email message.
 ***/
int
smtp_internal_SendEmail(pSmtpData inf)
    {
    pSmtpAttribute envFrom = NULL;
    pSmtpAttribute envTo = NULL;

	/** Get the to and from. **/
	envFrom = SMTP_ATTR(xhLookup(inf->Attributes, "envelope_from"));
	envTo = SMTP_ATTR(xhLookup(inf->Attributes, "envelope_to"));

	/** Send it using sendmail. **/
	if (UNLIKELY(smtp_internal_SpawnSendmail(inf->EmailPath.String, envFrom, envTo) < 0))
	    {
	    mssError(0, "SMTP", "Could not send the mail.");
	    goto error;
	    }

	return 0;

    error:
	return -1;
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
    int i;

	/** Create the node object **/
	node = snNewNode(obj, "system/smtp");
	if (UNLIKELY(node == NULL))
	    {
	    mssError(0, "SMTP", "Could not create new node object");
	    goto error;
	    }

	/** Iterate through all the default root attributes. **/
	for (i = 0; i < SMTP_INF.DefaultRootAttributes.nItems; i ++)
	    {
	    currentAttr = SMTP_ATTR(SMTP_INF.DefaultRootAttributes.Items[i]);

	    /** Add the attribute to the node. **/
	    currentParam = stAddAttr(node->Data, currentAttr->Name);
	    if (UNLIKELY(currentParam == NULL))
		{
		mssError(0, "SMTP", "Could not add attribute value %s", currentAttr->Name);
		goto error;
		}

	    /** Set the attribute to its default value. **/
	    if (UNLIKELY(stSetAttrValue(currentParam, currentAttr->Type, &currentAttr->Value, 0) != 0))
		{
		mssError(0, "SMTP", "Could not set attribute value %s", currentAttr->Name);
		goto error;
		}
	    }

	/** Write the root node structure file. **/
	if (UNLIKELY(snWriteNode(obj, node) < 0))
	    {
	    mssError(0, "SMTP", "Could not write the root node structure file.");
	    goto error;
	    }

	return node;

    error:
	mssError(0, "SMTP", "Failed to create root node.");
	return NULL;
    }


/*** smtp_internal_CreateEmail - Create a new email file.
 ***/
int
smtp_internal_CreateEmail(pSmtpData inf)
    {
    pXString autoName = NULL;

    pSmtpAttribute currentHeader = NULL;
    pSmtpAttribute hostName = NULL;

    pStructInf emailStruct = NULL;
    pStructInf createdStruct = NULL;
    pSmtpAttribute currentAttr = NULL;
    pDateTime attrDate = NULL;
    DateTime currentDate;

    pFile checkFile = NULL;
    pFile emailStructFile = NULL;
    char message_id[80];
    ObjData pod;
    int i;
    unsigned char email_id[8];
    char local_host_name[128] = "localhost.localdomain";

    int prefix_len;
    int rval = -1;

	autoName = xsNew();
	if (UNLIKELY(autoName == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate an xstring for the email name.");
	    goto end;
	    }

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
		if (UNLIKELY(xsQPrintf(autoName, "%STR&HEX&8LEN-%STR&HEX&8LEN.eml", email_id, email_id+4) < 0))
		    {
		    mssError(1, "SMTP", "Failed to format a random email name.");
		    goto end;
		    }

		/** Build the full email path. **/
		if (UNLIKELY(xsSubst(&inf->EmailPath, prefix_len, inf->EmailPath.Length - prefix_len, autoName->String, autoName->Length) < 0))
		    {
		    mssError(1, "SMTP", "Failed to substitute email name \"%s\" into email path.", autoName->String);
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
		mssError(1, "SMTP", "Unable to auto-generate a unique filename. May have exceeded allowable range of filenames.");
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

	/** Initialize the file descriptor for the content. **/
	inf->ContentFile = NULL;

	/** Create the email file. **/
	inf->ContentFile = fdOpen(inf->EmailPath.String, inf->Obj->Mode & ~(O_TRUNC), inf->Mask);
	if (UNLIKELY(inf->ContentFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Failed to create a new email file (%s).", inf->EmailPath.String);
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

	/** Create the email node. **/
	emailStruct = stCreateStruct(inf->Name, "system/structure");
	if (UNLIKELY(emailStruct == NULL))
	    {
	    mssError(0, "SMTP", "Could not create new email struct.");
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

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emailStruct, currentAttr->Name);
	    if (UNLIKELY(createdStruct == NULL))
		{
		mssError(1, "SMTP", "Unable to add new attribute (%s) to the email struct.", currentAttr->Name);
		goto end;
		}

	    /** Set the default attribute value. **/
	    if (UNLIKELY(stSetAttrValue(createdStruct, currentAttr->Type, &currentAttr->Value, 0) != 0))
		{
		mssError(1, "SMTP", "Unable to write to the default attribute (%s).", currentAttr->Name);
		goto end;
		}
	    }

	/** Add dynamic attributes which have object specific defaults. **/
	/** Calculate the message id (name without suffix). **/
	hostName = SMTP_ATTR(xhLookup(inf->Attributes, "local_host_name"));
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
	    fprintf(stderr, "Warning: gethostname() failed (%s); using \"%s\".\n", strerror(errno), local_host_name);
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

	/** Get the current date. **/
	if (UNLIKELY(objCurrentDate(&currentDate) != 0))
	    {
	    mssError(1, "SMTP", "Unable to obtain the current date.");
	    goto end;
	    }

	/** Allocate a new date data structure. **/
	attrDate = (pDateTime)nmMalloc(sizeof(DateTime));
	if (UNLIKELY(attrDate == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate a date structure for a default attribute.");
	    goto end;
	    }
	memset(attrDate, 0, sizeof(DateTime));

	/** Calculate the default expire date for the object. **/
	memcpy(attrDate, &currentDate, sizeof(DateTime));
	if (UNLIKELY(objDateAddPart(attrDate, 72, "hour") != 0))
	    {
	    mssError(0, "SMTP", "Failed to calculate the default expire date.");
	    goto end;
	    }

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

	/** Allocate a new date data structure. **/
	attrDate = (pDateTime)nmMalloc(sizeof(DateTime));
	if (UNLIKELY(attrDate == NULL))
	    {
	    mssError(1, "SMTP", "Failed to allocate a date structure for a default attribute.");
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
	    mssErrorErrno(1, "SMTP", "Unable to create the email struct file (%s).", inf->EmailStructPath.String);
	    goto end;
	    }

	/** Write the struct file. **/
	if (UNLIKELY(stGenerateMsg(emailStructFile, emailStruct, 0) != 0))
	    {
	    mssError(0, "SMTP", "Failed to write the email struct file.");
	    goto end;
	    }

	/** Fill the email file with some basic attributes. **/

	/** Fill in the non-static default headers. **/
	// TODO: Add current date to the header... once we implement date support in the MIME driver

	/** Add the dynamic attributes to the file. **/
	if (UNLIKELY(fdPrintf(inf->ContentFile, "Message-ID: <%s>\n", message_id) < 0))
	    {
	    mssError(0, "SMTP", "Failed to write message id to new message");
	    goto end;
	    }

	/** Iterate through all the default email headers. **/
	for (i = 0; i < SMTP_INF.DefaultEmailHeaders.nItems; i ++)
	    {
	    currentHeader = SMTP_ATTR(SMTP_INF.DefaultEmailHeaders.Items[i]);

	    /** Add the attribute to the file. **/
	    if (UNLIKELY(fdPrintf(inf->ContentFile, "%s: %s\n", currentHeader->Name, currentHeader->Value.String) < 0))
		{
		mssError(0, "SMTP", "Failed to write default header to new message (%s: %s).",
			currentHeader->Name, currentHeader->Value.String);
		goto end;
		}
	    }

	/** Add an empty line for header separation to the file. **/
	if (UNLIKELY(fdWrite(inf->ContentFile, "\n", 1, 0, 0) < 0))
	    {
	    mssError(0, "SMTP", "Failed to write default header separator to new message.");
	    goto end;
	    }

	/** Mark this object so the OSML doesn't automatically layer the MIME driver **/
	inf->Obj->Flags |= OBJ_F_NOCASCADE;

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    {
	    mssError(0, "SMTP", "Failed to create email (%s).", inf->EmailPath.String);

	    if (inf->ContentFile != NULL) fdClose(inf->ContentFile, 0);
	    inf->ContentFile = NULL;
	    }

	if (LIKELY(autoName != NULL)) xsFree(autoName);
	if (LIKELY(emailStructFile != NULL)) fdClose(emailStructFile, 0);
	if (UNLIKELY(attrDate != NULL)) nmFree(attrDate, sizeof(DateTime));
	if (LIKELY(emailStruct != NULL)) stFreeInf(emailStruct);

	return rval;
    }


/*** smtp_internal_OpenGeneral - Loads attributes common to all SMTP objects.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenGeneral(pSmtpData inf, char* usrtype)
    {
    pSnNode node = NULL;

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
		mssError(0, "SMTP", "Node exists and CREAT and EXCL flags are set. Cannot create new node.");
		goto error;
		}

	    node = smtp_internal_CreateRootNode(inf->Obj, inf->Mask);
	    if (UNLIKELY(node == NULL))
		{
		mssError(0,"SMTP", "Could not create new node object");
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
	    mssError(0,"SMTP","Could not open structure file");
	    goto error;
	    }

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
	    mssError(1,"SMTP","Could not create attribute names array.");
	    goto error;
	    }

	pXHashTable attributes = (pXHashTable)nmMalloc(sizeof(XHashTable));
	if (UNLIKELY(attributes == NULL))
	    {
	    mssError(1,"SMTP","Could not create attributes hash table.");
	    goto error;
	    }
	memset(attributes, 0, sizeof(XHashTable));
	if (UNLIKELY(xhInit(attributes, 17, 0) != 0))
	    {
	    mssError(1, "SMTP", "Failed to initialize attributes hash table.");
	    nmFree(attributes, sizeof(XHashTable));
	    goto error;
	    }
	inf->Attributes = attributes;

	inf->CurAttr = 0;

	if (UNLIKELY(smtp_internal_GetStructAttributes(inf->Node->Data, inf) != 0))
	    {
	    mssError(0, "SMTP", "Could not load root attributes.");
	    goto error;
	    }

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
	/** Perform a general open. **/
	if (UNLIKELY(smtp_internal_OpenGeneral(inf, usrtype) < 0))
	    goto error;

	/** Set the node type. **/
	inf->Type = SMTP_T_ROOT;

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
    int rval = -1;

	/** Perform a general open. **/
	if (UNLIKELY(smtp_internal_OpenGeneral(inf, usrtype) < 0))
	    goto end;

	/** Set the node type. **/
	inf->Type = SMTP_T_EML;

	/** Calculate the real path of the email file. **/
	pSmtpAttribute spoolDir = SMTP_ATTR(xhLookup(inf->Attributes, "spool_dir"));
	if (UNLIKELY(spoolDir == NULL))
	    {
	    mssError(1, "SMTP", "Unable to get the spool directory path.");
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
		if (UNLIKELY(smtp_internal_CreateEmail(inf) < 0))
		    {
		    mssError(0, "SMTP", "Failed to create a new email.");
		    goto end;
		    }
		}
	    else
		{
		/** File does not exist, and creation not requested **/
		mssErrorErrno(1, "SMTP", "Could not open email file: \"%s\".", inf->EmailPath.String);
		goto end;
		}
	    }
	else
	    {
	    /** Creation requested with exclude, but file exists? **/
	    if ((inf->Obj->Mode & OBJ_O_CREAT) && (inf->Obj->Mode & OBJ_O_EXCL))
		{
		mssError(1, "SMTP", "Email creation request failed because the email already exists.");
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

	    fdClose(fd, 0);
	    fd = NULL;
	    }

	/** Open the email file. **/
	const int open_mode = inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL);
	if (UNLIKELY(inf->ContentFile == NULL))
	    inf->ContentFile = fdOpen(inf->EmailPath.String, open_mode, inf->Mask);
	if (UNLIKELY(inf->ContentFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Could not open email file (%s).", inf->EmailPath.String);
	    goto end;
	    }

	/** Open the email structure file. **/
	emailStructureFile = fdOpen(inf->EmailStructPath.String, open_mode, inf->Mask);
	if (UNLIKELY(emailStructureFile == NULL))
	    {
	    mssErrorErrno(1, "SMTP", "Could not open email structure file: \"%s\".", inf->EmailStructPath.String);
	    goto end;
	    }

	/** Parse the structure file. **/
	emailStructure = stParseMsg(emailStructureFile, 0);
	if (UNLIKELY(emailStructure == NULL))
	    {
	    mssError(0, "SMTP", "Could not parse the email structure file.");
	    goto end;
	    }

	/** Get the structure's attributes **/
	if (UNLIKELY(smtp_internal_GetStructAttributes(emailStructure, inf) != 0))
	    {
	    mssError(0, "SMTP", "Could not load email attributes.");
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to open email.");

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
	    mssError(0, "SMTP", "Call to smtpOpen(NULL, ...);");
	    goto error;
	    }
	ASSERTMAGIC(obj, MGK_OBJECT);

	/** Allocate driver struct. */
	inf = nmMalloc(sizeof(SmtpData));
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "SMTP", "Could not allocate SmtpData object.");
	    goto error;
	    }
	memset(inf, 0, sizeof(SmtpData));
	inf->Mask = mask;
	inf->Obj = obj;
	if (UNLIKELY(xsInit(&inf->EmailPath) != 0
	    || xsInit(&inf->EmailStructPath) != 0
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
	    mssError(1,"SMTP","Could not open file");
	    goto error;
	    }

	/** Correct the pathname. **/
	obj_internal_PathPart(obj->Pathname, 0, 0);

	return inf;

    error:
	mssError(0, "SMTP", "Failed to open smtp file.");

	if (inf != NULL) smtp_internal_Close(inf);

	return NULL;
    }


/*** smtp_internal_Close() - close up.
 ***/
int
smtp_internal_Close(pSmtpData inf)
    {
    int rval = 0;

	if (UNLIKELY(inf == NULL))
	    return -1;

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
	    if (UNLIKELY(xhClear(inf->Attributes, smtp_internal_ClearAttribute, NULL) != 0
		|| xhDeInit(inf->Attributes) != 0
	    ))   {
		mssError(1, "SMTP", "Failed to free attributes.");
		rval = -1;
		}
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
	    inf->Node->OpenCnt--;
	    }

	if (UNLIKELY(xsDeInit(&inf->EmailPath) != 0
	    || xsDeInit(&inf->EmailStructPath) != 0
	))   {
	    mssError(1, "SMTP", "Failed to deinit xstring.");
	    rval = -1;
	    }
	nmFree(inf, sizeof(SmtpData));

	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to close smtp object.");

	return rval;
    }


/*** smtpClose - close an open object.
 ***/
int
smtpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);

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
	    mssError(1,"SMTP","Could not create file");
	    goto error;
	    }

	return 0;

    error:
	mssError(0, "SMTP", "Failed to create smtp object.");
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

	/** Try to open it first. **/
	obj->Mode = O_RDWR;
	inf = (pSmtpData)smtpOpen(obj, 0, NULL, "", oxt);
	if (UNLIKELY(inf == NULL))
	    goto end;

	/** Determine the type of the object. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    mssError(1, "SMTP", "Not handling deleting root nodes.");
	    goto end;
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Delete the email file. **/
	    if (UNLIKELY(remove(inf->EmailPath.String) != 0))
		{
		mssErrorErrno(1, "SMTP", "Could not delete the email file (%s).", inf->EmailPath.String);
		goto end;
		}

	    /** Delete the email struct. **/
	    if (UNLIKELY(remove(inf->EmailStructPath.String) != 0))
		{
		mssErrorErrno(1, "SMTP", "Could not delete the email struct file (%s).", inf->EmailStructPath.String);
		goto end;
		}
	    }
	else
	    {
	    mssError(1, "SMTP", "Could not delete indicated object (unknown type %d).", inf->Type);
	    goto end;
	    }

	/** Success. **/
	rval = 0;

    end:
	if (LIKELY(inf != NULL) && UNLIKELY(smtp_internal_Close(inf) != 0))
	    rval = -1;

	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to delete smtp object.");

	return rval;
    }


/*** smtpRead - Read from the SMTP object
 ***/
int
smtpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    int rval = -1;

	/** Read the contents of emails directly. **/
	if (UNLIKELY(inf->Type != SMTP_T_EML))
	    {
	    mssError(1, "SMTP", "Unable to read content from smtp object of type %d.", inf->Type);
	    return -1;
	    }

	rval = fdRead(inf->ContentFile, buffer, maxcnt, offset, flags);
	if (UNLIKELY(rval < 0))
	    mssErrorErrno(1, "SMTP", "Failed to read %d bytes at offset %d from email file (%s).", maxcnt, offset, inf->EmailPath.String);

	return rval;
    }


/*** smtpWrite - Write to the SMTP object
 ***/
int
smtpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    int rval = -1;

	/** Write the contents of emails directly. **/
	if (UNLIKELY(inf->Type != SMTP_T_EML))
	    {
	    mssError(1, "SMTP", "Unable to write content to smtp object of type %d.", inf->Type);
	    return -1;
	    }

	rval = fdWrite(inf->ContentFile, buffer, cnt, offset, flags);
	if (UNLIKELY(rval < 0))
	    mssErrorErrno(1, "SMTP", "Failed to write %d bytes at offset %d to email file (%s).", cnt, offset, inf->EmailPath.String);

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

	/** Allocate the query object. **/
	qy = (pSmtpQueryData)nmMalloc(sizeof(SmtpQueryData));
	if (UNLIKELY(qy == NULL))
	    {
	    mssError(1,"SMTP","Unable to allocate query object");
	    goto error;
	    }
	memset(qy, 0, sizeof(SmtpQueryData));

	qy->Data = inf;

	/** Construct the query for the root node. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Find and open the spool directory path. **/
	    attr = (pSmtpAttribute)xhLookup(inf->Attributes, "spool_dir");
	    if (UNLIKELY(attr == NULL))
		{
		mssError(1,"SMTP","Unable to locate spool directory");
		goto error;
		}
	    spoolPath = attr->Value.String;

	    qy->Directory = opendir(spoolPath);
	    if (UNLIKELY(qy->Directory == NULL))
		{
		mssErrorErrno(1, "SMTP", "Could not open spool directory (%s) for query", spoolPath);
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
	mssError(0, "SMTP", "Failed to open query on smtp object.");

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
		mssError(1, "SMTP", "Query result pathname exceeds internal limits");
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
	    inf->Obj = obj;
	    if (UNLIKELY(xsInit(&inf->EmailPath) != 0
		|| xsInit(&inf->EmailStructPath) != 0
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
	mssError(0, "SMTP", "Failed to fetch query result (%s).", (mailEntry != NULL) ? mailEntry->d_name : "none");

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

	/** Default values all happen to be strings. **/
	if (strcmp(attrname, "name") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "content_type") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "outer_type") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "inner_type") == 0) return DATA_T_STRING;
	if (strcmp(attrname, "annotation") == 0) return DATA_T_STRING;

	/** Get the type of the stored attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
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
	    mssError(1, "SMTP", "Attribute not found '%s'", attrname);
	    return -1;
	    }

	inf = SMTP(inf_v);

	if (strcmp(attrname, "name") == 0)
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be a string)", attrname);
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
		mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be string)", attrname);
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
		mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be string)", attrname);
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
		mssError(1, "SMTP", "Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "";
	    return 0;
	    }

	/** Get the type of the stored attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	if (attr)
	    {
	    if (datatype != attr->Type)
		{
		mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be %s)", attrname, obj_type_names[attr->Type]);
		return -1;
		}
	    if (attr->Type == DATA_T_INTEGER)
		val->Integer = attr->Value.Integer;
	    else
		val->String = attr->Value.String;
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

	inf->CurAttr = 0;

    return smtpGetNextAttr(inf_v, oxt);
    }


/*** smtpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
smtpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute attr = NULL;

    pSnNode rootNode = NULL;
    pStructInf attrStruct = NULL;

    pFile emlStructFileRead = NULL;
    pFile emlStructFileWrite = NULL;
    pStructInf emlStruct = NULL;

    int old_int_val = -1;
    int rval = -1;

	/** Get the requested attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	if (!attr)
	    {
	    /** Add the attribute if it is not found. **/
	    if (UNLIKELY(smtpAddAttr(inf, attrname, datatype, val, oxt) != 0))
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

	/** Check the requested datatype. **/
	if (attr->Type != datatype)
	    {
	    if (datatype < OBJ_TYPE_NAMES_CNT && attr->Type < OBJ_TYPE_NAMES_CNT && datatype >= 0 && attr->Type >= 0)
		{
		mssError(1 ,"SMTP", "Attempt to assign invalid data type to attribute. (Assigning %s to %s)", obj_type_names[datatype], obj_type_names[attr->Type]);
		}
	    else
		{
		mssError(1 ,"SMTP", "Attempt to assign invalid data type to attribute. (Assigning %d to %d)", datatype, attr->Type);
		}
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
	    old_int_val = attr->Value.Integer;
	    attr->Value.Integer = val->Integer;
	    }
	else if (datatype == DATA_T_DATETIME)
	    {
	    if (!attr->Value.DateTime)
		attr->Value.DateTime = nmMalloc(sizeof(DateTime));
	    if (UNLIKELY(attr->Value.DateTime == NULL))
		{
		mssError(1, "SMTP", "Failed to allocate %d bytes for a date.", (int)sizeof(DateTime));
		goto end;
		}
	    memcpy(attr->Value.DateTime, val->DateTime, sizeof(DateTime));
	    }
	else
	    {
	    mssError(1, "SMTP", "Unsupported data type %d.", datatype);
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
	    emlStructFileRead = fdOpen(inf->EmailStructPath.String,
					    inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL),
					    inf->Mask);
	    if (UNLIKELY(emlStructFileRead == NULL))
		{
		mssErrorErrno(1, "SMTP", "Could not open email structure file (%s).", inf->EmailStructPath.String);
		goto end;
		}

	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFileRead, 0);
	    if (UNLIKELY(emlStruct == NULL))
		{
		mssError(0, "SMTP", "Could not parse the email structure file.");
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
	    emlStructFileWrite = fdOpen(inf->EmailStructPath.String,
					    (inf->Obj->Mode | (O_TRUNC)) & ~(O_EXCL | O_CREAT),
					    inf->Mask);
	    if (UNLIKELY(emlStructFileWrite == NULL))
		{
		mssErrorErrno(1, "SMTP", "Could not open email structure file (%s) for writing.", inf->EmailStructPath.String);
		goto end;
		}

	    /** Write changes to the email struct file. **/
	    if (UNLIKELY(stGenerateMsg(emlStructFileWrite, emlStruct, O_WRONLY | O_TRUNC | O_CREAT) != 0))
		{
		mssError(1, "SMTP", "Unable to write the attribute to the email struct file.");
		goto end;
		}

	    /** If the email is ready to send, send it. **/
	    if (strcmp(attrname, "is_ready") == 0 && val->Integer == 1 && old_int_val == 0)
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
	    mssError(0, "SMTP", "Failed to set attribute '%s'.", attrname);

	/** Free appropriate memory and close appropriate files. **/
	if (UNLIKELY(emlStructFileRead != NULL)) fdClose(emlStructFileRead, 0);
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
    pSmtpAttribute attr = NULL;
    pSmtpAttribute unstoredAttr = NULL;
    pStructInf createdStruct = NULL;

    pSnNode rootNode = NULL;

    pFile emlStructFile = NULL;
    pStructInf emlStruct = NULL;
    int rval = -1;

	/** Initialize the new attribute. **/
	attr = nmMalloc(sizeof(SmtpAttribute));
	if (UNLIKELY(attr == NULL))
	    {
	    mssError(1,"SMTP","Could not create new attribute object.");
	    goto end;
	    }
	memset(attr, 0, sizeof(SmtpAttribute));
	unstoredAttr = attr;

	/** Set the meta-data fields of the new attribute. **/
	attr->Name = nmSysStrdup(attrname);
	if (UNLIKELY(attr->Name == NULL))
	    {
	    mssError(1, "SMTP", "Failed to copy attribute name.");
	    goto end;
	    }
	attr->Type = type;

	/** Set the default value appropriately if it is a string. **/
	if (attr->Type == DATA_T_STRING)
	    {
	    attr->Value.String = nmSysStrdup("");
	    if (UNLIKELY(attr->Value.String == NULL))
		{
		mssError(1, "SMTP", "Failed to allocate an empty string value.");
		goto end;
		}
	    }

	/** Set the default value appropriately if it is an integer. **/
	if (attr->Type == DATA_T_INTEGER)
	    {
	    attr->Value.Integer = 0;
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
	    emlStructFile = fdOpen(inf->EmailStructPath.String,
					    inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL),
					    inf->Mask);
	    if (UNLIKELY(emlStructFile == NULL))
		{
		mssErrorErrno(1, "SMTP", "Could not open email structure file (%s).", inf->EmailStructPath.String);
		goto end;
		}

	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFile, 0);
	    if (UNLIKELY(emlStruct == NULL))
		{
		mssError(0, "SMTP", "Could not parse the email structure file.");
		goto end;
		}

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emlStruct, attr->Name);
	    if (UNLIKELY(createdStruct == NULL))
		{
		mssError(1, "SMTP", "Could not add attribute '%s' to the email struct.", attr->Name);
		goto end;
		}

	    /** Set the attribute value. **/
	    if (UNLIKELY(stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0) < 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute '%s'", attr->Name);
		goto end;
		}

	    /** Write changes to the email struct file. **/
	    if (UNLIKELY(stGenerateMsg(emlStructFile, emlStruct, O_WRONLY | O_TRUNC | O_CREAT) < 0))
		{
		mssError(0, "SMTP", "Unable to write the updated email struct file.");
		goto end;
		}
	    }

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(0, "SMTP", "Failed to add attribute '%s'.", attrname);

	/** Free appropriate memory and close appropriate files. **/
	if (UNLIKELY(unstoredAttr != NULL)) smtp_internal_ClearAttribute((char*)unstoredAttr, NULL);
	if (emlStructFile != NULL) fdClose(emlStructFile, 0);
	if (emlStruct != NULL) stFreeInf(emlStruct);

	return rval;
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
	    mssError(1, "SMTP", "Failed to allocate %d bytes for the driver.", (int)sizeof(ObjDriver));
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
