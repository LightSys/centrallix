/**
#include <unistd.h>
#include <fcntl.h>
#include "cxlib/mtask.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "cxlib/mtsession.h"
#include "cxlib/util.h" **/
/** module definintions **/
/**#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
**/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "obj.h"
#include "st_node.h"
#include "cxlib/xarray.h"
#include "centrallix.h"
#include <sys/types.h>

#include <errno.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2013 LightSys Technology Services, Inc.		*/
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
/* Module: 	Email Objectsystem driver				*/
/* Authors:	Hazen Johnson, Justin Southworth			*/
/* Creation:	May 29, 2014						*/
/* Description:	Provides an email interface for Centrallix through the	*/
/*		ObjectSystem.						*/
/*									*/
/*		Current Shortcomings:					*/
/*		  - All functionality is perfect... There is no		*/
/*		  functionality.*/
/*									*/
/************************************************************************/

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


/*** smtp_internal_SpawnSendmail - launch the sendmail process to actually
 *** send off an email message.  This also works with Postfix, via its
 *** "sendmail compatibility interface".
 ***/
int
smtp_internal_SpawnSendmail(char* emailPath, pSmtpAttribute envFrom, pSmtpAttribute envTo)
    {
    int pid, fd, maxfiles;
    XArray argv;
    char *envp[] = {NULL};
    int wstatus;

	/** Build the sendmail argument list **/
	xaInit(&argv, 11);
	xaAddItem(&argv, "/usr/sbin/sendmail");		/* also compatible with Postfix */
	xaAddItem(&argv, "-t");				/* extract recipients from headers */
#if SMTP_DEBUG
	xaAddItem(&argv, "-N");				/* delivery status notifications (debug) */
	xaAddItem(&argv, "delay, failure, success");	/* ... for delay/fail/success (debug) */
	xaAddItem(&argv, "-v");				/* verbose (debug) */
#endif
	xaAddItem(&argv, "-bm");			/* mail sending from STDIN */
	xaAddItem(&argv, "-i");				/* don't end on . on a line by itself */

	/** Add envelope To and From **/
	if (envFrom && strcmp(envFrom->Value.String, ""))
	    {
	    xaAddItem(&argv, "-f");
	    xaAddItem(&argv, envFrom->Value.String);
	    }
	if (envTo && strcmp(envTo->Value.String, ""))
	    {
	    xaAddItem(&argv, envTo->Value.String);
	    }

	xaAddItem(&argv, NULL);

	/** Fork. **/
	pid = fork();
	if (pid < 0)
	    {
	    mssErrorErrno(1, "SMTP", "Unable to fork (1).");
	    goto error;
	    }
	if (!pid)
	    {
	    /** we're in the child process -- disable MTask context switches to be safe **/
	    thLock();

	    /** close all open fds (except for 0-2 -- std{in,out,err}) **/
	    maxfiles = sysconf(_SC_OPEN_MAX);
	    if (maxfiles <= 0)
		{
		mssError(1, "SMTP", "Warning: sysconf(_SC_OPEN_MAX) returned <= 0; using maxfiles=2048.");
		maxfiles = 2048;
		}

	    for(fd=3;fd<maxfiles;fd++) close(fd);

	    /** Open the email. **/
	    fd = open(emailPath, O_RDONLY);

	    /** Hopefully this makes our file stdin so we don't have to cat it into sendmail. **/
	    dup2(fd, 0);

	    /** NOTE: We're currently double forking to get rid of zombie processes. **/
	    /** TODO: Change this to look at the return value of sendmail and act accordingly. **/
	    pid = fork();
	    if (pid < 0)
		{
		mssErrorErrno(1, "SMTP", "Unable to fork (2).");
		exit(EXIT_FAILURE);
		}
	    if (!pid)
		{
		/** we're in the child process -- disable MTask context switches to be safe **/
		thLock();

		/** close all open fds (except for 0-2 -- std{in,out,err}) **/
		maxfiles = sysconf(_SC_OPEN_MAX);
		if (maxfiles <= 0)
		    {
		    mssError(1, "SMTP", "Warning: sysconf(_SC_OPEN_MAX) returned <= 0; using maxfiles=2048.");
		    maxfiles = 2048;
		    }

		for(fd=3;fd<maxfiles;fd++) close(fd);

		/** Execve. **/
		execve("/usr/sbin/sendmail", (char**)(argv.Items), envp);

		/** if execve() is successfull, this is never reached **/
		mssErrorErrno(1, "SMTP", "execve() failed: %s", strerror(errno));
		_exit(EXIT_FAILURE);
		}
	    else
		{
		/** We're the parent. Exit so centrallix can move on. **/
		_exit(EXIT_SUCCESS);
		}
	    }

	/** Get status of child process, releasing it from the process table. **/
	if (waitpid(pid, &wstatus, WNOHANG) == 0)
	    {
	    /** Try again in 1 msec if not immediately ready to reap; this lets
	     ** us yield to other threads.
	     **/
	    thSleep(1);
	    waitpid(pid, &wstatus, 0);
	    }
	if (WEXITSTATUS(wstatus) != EXIT_SUCCESS)
	    {
	    mssError(1, "SMTP", "Failed to start child sendmail process (%d)", WEXITSTATUS(wstatus));
	    goto error;
	    }

	/** We're a parent. Deinit stuff and return happily. **/
	xaDeInit(&argv);

	return 0;

    error:
	xaDeInit(&argv);
	return -1;
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

	inf = (pSmtpAttribute)nmMalloc(sizeof(SmtpAttribute));
	if (!inf)
	    goto error;
	memset(inf, 0, sizeof(SmtpAttribute));

	inf->Name = nmSysStrdup(name);
	if (!inf->Name)
	    goto error;
	inf->Type = type;

	if (type == DATA_T_INTEGER)
	    {
	    inf->Value.Integer = intVal;
	    }
	else if (type == DATA_T_STRING && strVal)
	    {
	    inf->Value.String = nmSysStrdup(strVal);
	    if (!inf->Value.String)
		goto error;
	    }
	else
	    {
	    goto error;
	    }

	return inf;

    error:
	if (inf)
	    smtp_internal_ClearAttribute((char*)inf, NULL);
	return NULL;
    }


/*** smtp_internal_InitGlobals - Initializes global information for the SMTP
 *** driver.
 ***/
int
smtp_internal_InitGlobals()
    {
    char local_host_name[HOST_NAME_MAX];

	/** Initialize the global attributes. **/
	xaInit(&SMTP_INF.DefaultRootAttributes, 16);
	xaInit(&SMTP_INF.DefaultEmailAttributes, 16);
	xaInit(&SMTP_INF.DefaultEmailHeaders, 16);

	/** Add all the required attributes. Yay hardcoding! **/
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
	    {
	    strtcpy(local_host_name, "localhost.localdomain", sizeof(local_host_name));
	    }
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("local_host_name",	DATA_T_STRING,	0,	local_host_name));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("send_method",		DATA_T_STRING,	0,	"sendmail"));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("server",		DATA_T_STRING,	0,	"127.0.0.1"));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("port",		DATA_T_INTEGER,	23,	NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("spool_dir",		DATA_T_STRING,	0,	"/var/spool/mail/_centrallix"));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_dir",		DATA_T_STRING,	0,	"/var/log"));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_date_attr",	DATA_T_STRING,	0,	""));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_msgid_attr",	DATA_T_STRING,	0,	""));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_info_attr",	DATA_T_STRING,	0,	""));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("ratelimit_time",	DATA_T_INTEGER,	1,	NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("domlimit_time",	DATA_T_INTEGER,	5,	NULL));

	/** Add all the required email attributes. Behold the hard code; standeth it against all but the hardest hammer. **/
	xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("envelope_from",	DATA_T_STRING,	0,	""));
	xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("envelope_to",	DATA_T_STRING,	0,	""));
	xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("status",		DATA_T_STRING,	0,	"Draft"));
	xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("is_ready",		DATA_T_INTEGER,	0,	0));
	/** Not strictly necessary. **/
	/** xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("try_count",	DATA_T_INTEGER,	5,	0)); **/
	xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("last_try_status",	DATA_T_STRING,	0,	"None"));
	xaAddItem(&SMTP_INF.DefaultEmailAttributes, smtp_internal_CreateAttribute("last_try_msg",	DATA_T_STRING,	0,	""));


	/** Add all the default headers for an email file. **/
	xaAddItem(&SMTP_INF.DefaultEmailHeaders, smtp_internal_CreateAttribute("User-Agent",		DATA_T_STRING,	0,	"Centrallix/" PACKAGE_VERSION));
	xaAddItem(&SMTP_INF.DefaultEmailHeaders, smtp_internal_CreateAttribute("Subject",		DATA_T_STRING,	0,	""));
	xaAddItem(&SMTP_INF.DefaultEmailHeaders, smtp_internal_CreateAttribute("MIME-Version",		DATA_T_STRING,	0,	"1.0"));

    return 0;
    }


/*** smtp_internal_IsEmail - Returns 1 if the filename is an email.
 ***/
int
smtp_internal_IsEmail(char* filename)
    {
    int l = strlen(filename);
    return l >= 4 && (!strcmp(filename + l - 4, ".msg") || !strcmp(filename + l - 4, ".eml"));
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
	    if (!attr)
		{
		mssError(1,"SMTP","Could not create new attribute object.");
		goto error;
		}
	    memset(attr, 0, sizeof(SmtpAttribute));

	    attr->Name = nmSysStrdup(currentAttr->Name);
	    if (!attr->Name)
		goto error;
	    attr->Type = currentAttr->Value->DataType;

	    xaAddItem(inf->AttributeNames, attr->Name);

	    if (currentAttr->Value->DataType == DATA_T_STRING && (!strcmp(attr->Name, "expire_date") || !strcmp(attr->Name, "last_try_date")))
		{
		/** DateTime attribute, but from a string **/
		attr->Type = DATA_T_DATETIME;
		attr->Value.DateTime = NULL;
		if (stAttrValue(currentAttr, NULL, &attr->Value.String, 0) < 0 || !attr->Value.String)
		    {
		    attr->Value.DateTime = NULL;
		    }
		else
		    {
		    dt = nmMalloc(sizeof(DateTime));
		    if (!dt)
			goto error;
		    if (objDataToDateTime(DATA_T_STRING, attr->Value.String, dt, NULL) != 0)
			{
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
		    if (!attr->Value.String)
			goto error;
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
		mssError(1, "SMTP", "Unsupported attribute type in email data file");
		goto error;
		}
	    xhAdd(inf->Attributes, currentAttr->Name, (char*)attr);
	    }

	return 0;

    error:
	if (attr)
	    smtp_internal_ClearAttribute((char*)attr, NULL);
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
	if (smtp_internal_SpawnSendmail(inf->EmailPath.String, envFrom, envTo))
	    {
	    mssError(0, "SMTP", "Could not send the mail.");
	    goto error;
	    }

	return 0;

    error:
	return -1;
    }


/*** smtp_internal_CreateRoot - Creates a root smtp node.
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
	if (!node)
	    {
	    mssError(0, "SMTP", "Could not create new node object");
	    return NULL;
	    }

	/** Iterate through all the default root attributes. **/
	for (i = 0; i < SMTP_INF.DefaultRootAttributes.nItems; i ++)
	    {
	    currentAttr = SMTP_ATTR(SMTP_INF.DefaultRootAttributes.Items[i]);

	    /** Add the attribute to the node. **/
	    currentParam = stAddAttr(node->Data, currentAttr->Name);
	    if (!currentParam)
		{
		mssError(0, "SMTP", "Could not add attribute value %s", currentAttr->Name);
		return NULL;
		}

	    /** Set the attribute to it's default value. **/
	    if (stSetAttrValue(currentParam, currentAttr->Type, &currentAttr->Value, 0))
		{
		mssError(0, "SMTP", "Could not set attribute value %s", currentAttr->Name);
		return NULL;
		}
	    }

	/** Write the root node structure file. **/
	snWriteNode(obj, node);

    return node;
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
    char message_id[80];;
    ObjData pod;
    int i;
    unsigned char email_id[8];
    char local_host_name[128] = "localhost.localdomain";

    int prefix_len;

	autoName = xsNew();
	if (!autoName)
	    goto error;

	/** Resolve autonaming. **/
	prefix_len = inf->EmailPath.Length - 1;
	if (inf->Obj->Mode & OBJ_O_AUTONAME && !strcmp(inf->Name, "*"))
	    {
	    for(i=0; i<100; i++)
		{
		/** Generate a random email name. **/
		cxssGenerateKey(email_id, 8);
		xsQPrintf(autoName, "%STR&HEX&8LEN-%STR&HEX&8LEN.eml", email_id, email_id+4);

		/** Build the full email path. **/
		xsSubst(&inf->EmailPath, prefix_len, inf->EmailPath.Length - prefix_len, autoName->String, autoName->Length);

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
		goto error;
		}

	    /** Set a new object name. **/
	    nmSysFree(inf->Name);
	    inf->Name = nmSysStrdup(autoName->String);
	    if (!inf->Name)
		goto error;
	    }

	/** Initialize the file descriptor for the content. **/
	inf->ContentFile = NULL;

	/** Create the email file. **/
	inf->ContentFile = fdOpen(inf->EmailPath.String, inf->Obj->Mode & ~(O_TRUNC), inf->Mask);
	if (!inf->ContentFile)
	    {
	    mssError(0, "SMTP", "Failed to create a new email file.");
	    goto error;
	    }

	/** Construct the email struct file path. **/
	xsCopy(&inf->EmailStructPath, inf->EmailPath.String, -1);
	if (xsSubst(&inf->EmailStructPath, inf->EmailStructPath.Length - 4, 4, ".struct", 7) < 0)
	    goto error;

	/** Create the email node. **/
	emailStruct = stCreateStruct(inf->Name, "system/structure");
	if (!emailStruct)
	    {
	    mssError(0, "SMTP", "Could not create new email struct.");
	    goto error;
	    }
	stSetVersion(emailStruct, 2);

	/** Add the default static attributes. **/
	for (i=0; i < SMTP_INF.DefaultEmailAttributes.nItems; i++)
	    {
	    /** Get the attribute from the default attribute array. **/
	    currentAttr = (pSmtpAttribute)xaGetItem(&SMTP_INF.DefaultEmailAttributes, i);
	    if (!currentAttr)
		{
		mssError(1, "SMTP", "Unable to get default attribute %d.", i);
		goto error;
		}

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emailStruct, currentAttr->Name);
	    if (!createdStruct)
		{
		mssError(1, "SMTP", "Unable to add new attribute to the email struct.");
		goto error;
		}

	    /** Set the default attribute value. **/
	    if (stSetAttrValue(createdStruct, currentAttr->Type, &currentAttr->Value, 0))
		{
		mssError(1, "SMTP", "Unable to write to the default attribute (%s).", currentAttr->Name);
		goto error;
		}
	    }

	/** Add dynamic attributes which have object specific defaults. **/
	/** Calculate the message id (name without suffix). **/
	hostName = SMTP_ATTR(xhLookup(inf->Attributes, "local_host_name"));
	gethostname(local_host_name, sizeof(local_host_name));
	strtcpy(message_id, inf->Name, sizeof(message_id));
	if (strrchr(message_id, '.'))
	    *(strrchr(message_id, '.')) = '\0';
	strtcat(message_id, "@", sizeof(message_id));
	strtcat(message_id, hostName?(hostName->Value.String):local_host_name, sizeof(message_id));

	/** Create the message_id attribute. **/
	createdStruct = stAddAttr(emailStruct, "message_id");
	if (!createdStruct)
	    {
	    mssError(1, "SMTP", "Unable to add new attribute to the email struct.");
	    goto error;
	    }

	/** Set the default name value. **/
	pod.String = message_id;
	if (stSetAttrValue(createdStruct, DATA_T_STRING, &pod, 0))
	    {
	    mssError(1, "SMTP", "Unable to write to the default attribute (%s).", currentAttr->Name);
	    goto error;
	    }

	/** Get the current date. **/
	if (objCurrentDate(&currentDate))
	    {
	    mssError(1, "SMTP", "Unable to obtain the current date.");
	    goto error;
	    }

	/** Allocate a new date data structure. **/
	attrDate = (pDateTime)nmMalloc(sizeof(DateTime));
	if (!attrDate)
	    {
	    mssError(1, "SMTP", "Failed to allocate a date structure for a default attribute.");
	    goto error;
	    }
	memset(attrDate, 0, sizeof(DateTime));

	/** Calculate the default expire date for the object. **/
	memcpy(attrDate, &currentDate, sizeof(DateTime));
	objDateAddPart(attrDate, 72, "hour");

	/** Create the expire_date attribute. **/
	createdStruct = stAddAttr(emailStruct, "expire_date");
	if (!createdStruct)
	    {
	    mssError(1, "SMTP", "Unable to add new attribute to the email struct.");
	    goto error;
	    }

	/** Set the default name value. **/
	if (stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0))
	    {
	    mssError(1, "SMTP", "Unable to write to the default attribute (%s).", currentAttr->Name);
	    goto error;
	    }
	nmFree(attrDate, sizeof(DateTime));
	attrDate = NULL;

	/** Allocate a new date datastructure. **/
	attrDate = (pDateTime)nmMalloc(sizeof(DateTime));
	if (!attrDate)
	    {
	    mssError(1, "SMTP", "Failed to allocate a date structure for a default attribute.");
	    goto error;
	    }
	memset(attrDate, 0, sizeof(DateTime));

	/** Create the message_id attribute. **/
	createdStruct = stAddAttr(emailStruct, "last_try_date");
	if (!createdStruct)
	    {
	    mssError(1, "SMTP", "Unable to add new attribute to the email struct.");
	    goto error;
	    }

	/** Set the default name value. **/
	if (stSetAttrValue(createdStruct, DATA_T_DATETIME, POD(&attrDate), 0))
	    {
	    mssError(1, "SMTP", "Unable to write to the default attribute (%s).", currentAttr->Name);
	    goto error;
	    }
	nmFree(attrDate, sizeof(DateTime));
	attrDate = NULL;

	/** Create the struct file. **/
	emailStructFile = fdOpen(inf->EmailStructPath.String, O_CREAT | O_RDWR | O_EXCL, 0755);
	if (!emailStructFile)
	    {
	    mssError(1, "SMTP", "Unable to create the email struct file.");
	    goto error;
	    }

	/** Write the struct file. **/
	if (stGenerateMsg(emailStructFile, emailStruct, 0))
	    {
	    mssError(0, "SMTP", "Failed to write the email struct file.");
	    goto error;
	    }

	/** Fill the email file with some basic attributes. **/

	/** Fill in the non-static default headers. **/
	// TODO: Add current date to the header... once we implement date support in the MIME driver

	/** Add the dynamic attributes to the file. **/
	if (fdPrintf(inf->ContentFile, "Message-ID: <%s>\n", message_id) < 0)
	    {
	    mssError(0, "SMTP", "Failed to write message id to new message");
	    goto error;
	    }

	/** Iterate through all the default email headers. **/
	for (i = 0; i < SMTP_INF.DefaultEmailHeaders.nItems; i ++)
	    {
	    currentHeader = SMTP_ATTR(SMTP_INF.DefaultEmailHeaders.Items[i]);

	    /** Add the attribute to the file. **/
	    if (fdPrintf(inf->ContentFile, "%s: %s\n", currentHeader->Name, currentHeader->Value.String) < 0)
		{
		mssError(0, "SMTP", "Failed to write default header to new message (%s: %s).",
			currentHeader->Name, currentHeader->Value.String);
		goto error;
		}
	    }

	/** Add an empty line for header separation to the file. **/
	if (fdWrite(inf->ContentFile, "\n", 1, 0, 0) < 0)
	    {
	    mssError(0, "SMTP", "Failed to write default header separator to new message.");
	    goto error;
	    }

	/** Mark this object so the OSML doesn't automatically layer the MIME driver **/
	inf->Obj->Flags |= OBJ_F_NOCASCADE;

	xsFree(autoName);

	/** Close the struct file. **/
	fdClose(emailStructFile, 0);

	return 0;

    error:
	if (autoName)
	    xsFree(autoName);

	if (inf->ContentFile) fdClose(inf->ContentFile, 0);
	if (emailStructFile) fdClose(emailStructFile, 0);
	if (attrDate) nmFree(attrDate, sizeof(DateTime));
	if (emailStruct) stFreeInf(emailStruct);

	return -1;
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
	    if (node)
		{
		mssError(0, "SMTP", "Node exists and CREAT and EXCL flags are set. Cannot create new node.");
		goto error;
		}

	    node = smtp_internal_CreateRootNode(inf->Obj, inf->Mask);
	    if (!node)
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
	if (!node)
	    {
	    mssError(0,"SMTP","Could not open structure file");
	    goto error;
	    }

	/** Store the node object. **/
	inf->Node = node;
	inf->Node->OpenCnt++;

	inf->Name = nmSysStrdup(obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->SubPtr + inf->Obj->SubCnt - 2, 1));
	if (!inf->Name)
	    goto error;

	inf->AttributeNames = xaNew(16);
	if (!inf->AttributeNames)
	    {
	    mssError(1,"SMTP","Could not create attribute names array.");
	    goto error;
	    }

	inf->Attributes = (pXHashTable)nmMalloc(sizeof(XHashTable));
	if (!inf->Attributes)
	    {
	    mssError(1,"SMTP","Could not create attributes hash table.");
	    goto error;
	    }
	memset(inf->Attributes, 0, sizeof(XHashTable));
	xhInit(inf->Attributes, 17, 0);

	inf->CurAttr = 0;

	if (smtp_internal_GetStructAttributes(inf->Node->Data, inf))
	    {
	    mssError(0, "SMTP", "Could not load root attributes.");
	    goto error;
	    }

	return 0;

    error:
	return -1;
    }


/*** smtp_internal_OpenRoot - Open the root node of the smtp structure.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenRoot(pSmtpData inf)
    {

	inf->Type = SMTP_T_ROOT;

    return 0;
    }


/*** smtp_internal_OpenEml - Open an email file in the smtp structure.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenEml(pSmtpData inf)
    {
    pSmtpAttribute spoolDir = NULL;
    pFile emailStructureFile = NULL;
    pStructInf emailStructure = NULL;
    pFile fd = NULL;

	inf->Type = SMTP_T_EML;

	/** Calculate the real path of the email file. **/
	spoolDir = SMTP_ATTR(xhLookup(inf->Attributes, "spool_dir"));
	if (!spoolDir)
	    {
	    mssError(1, "SMTP", "Unable to get the spool directory path.");
	    goto error;
	    }

	if (xsCopy(&inf->EmailPath, spoolDir->Value.String, strlen(spoolDir->Value.String)))
	    {
	    mssError(1, "SMTP", "Unable to copy spool directory path into the email path.");
	    goto error;
	    }

	if (xsConcatPrintf(&inf->EmailPath, "/%s", inf->Name) < 0)
	    {
	    mssError(1, "SMTP", "Unable to append email name to email path.");
	    goto error;
	    }

	/** Check that the email file exists. **/
	fd = fdOpen(inf->EmailPath.String, 0, 0);
	if (!fd)
	    {
	    /** Create the file if it doesn't exist and the create flag is set. **/
	    if (inf->Obj->Mode & OBJ_O_CREAT)
		{
		if (smtp_internal_CreateEmail(inf) < 0)
		    {
		    mssError(0, "SMTP", "Failed to create a new email.");
		    goto error;
		    }
		}
	    else
		{
		/** File does not exist, and creation not requested **/
		mssErrorErrno(1, "SMTP", "Could not open email file.");
		goto error;
		}
	    }
	else
	    {
	    /** Creation requested with exclude, but file exists? **/
	    if ((inf->Obj->Mode & OBJ_O_CREAT) && (inf->Obj->Mode & OBJ_O_EXCL))
		{
		mssError(1, "SMTP", "Email creation request failed because the email already exists.");
		goto error;
		}

	    /** Construct the email struct file path. **/
	    xsCopy(&inf->EmailStructPath, inf->EmailPath.String, -1);
	    if (xsSubst(&inf->EmailStructPath, inf->EmailStructPath.Length - 4, 4, ".struct", 7) < 0)
		goto error;
	    }
	fdClose(fd, 0);
	fd = NULL;

	/** Open the email file. **/
	if (!inf->ContentFile)
	    {
	    inf->ContentFile = fdOpen(inf->EmailPath.String, inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL), inf->Mask);
	    if (!inf->ContentFile)
		{
		mssErrorErrno(1, "SMTP", "Could not open email file (%s).", inf->EmailPath.String);
		goto error;
		}
	    }

	/** Open the email structure file. **/
	emailStructureFile = fdOpen(inf->EmailStructPath.String,
					inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL),
					inf->Mask);
	if (!emailStructureFile)
	    {
	    mssError(1, "SMTP", "Could not open email structure file (%s).", inf->EmailStructPath.String);
	    goto error;
	    }

	/** Parse the structure file. **/
	emailStructure = stParseMsg(emailStructureFile, 0);
	if (!emailStructure)
	    {
	    mssError(0, "SMTP", "Could not parse the email structure file.");
	    goto error;
	    }

	/** Get the structure's attribues **/
	if (smtp_internal_GetStructAttributes(emailStructure, inf))
	    {
	    mssError(0, "SMTP", "Could not load email attributes.");
	    goto error;
	    }

	/** Close the open files. **/
	fdClose(emailStructureFile, 0);
	stFreeInf(emailStructure);

    return 0;

    error:

	if (fd)
	    fdClose(fd, 0);
	if (emailStructureFile)
	    fdClose(emailStructureFile, 0);
	if (emailStructure)
	    stFreeInf(emailStructure);

	return -1;
    }

/*** smtpOpen - open an object.
 ***/
void*
smtpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSmtpData inf = NULL;
    char *internalPath = NULL;

	inf = (pSmtpData)nmMalloc(sizeof(SmtpData));
	if (!inf)
	    {
	    mssError(1, "SMTP", "Could not allocate SmtpData object.");
	    goto error;
	    }
	memset(inf, 0, sizeof(SmtpData));
	inf->Mask = mask;
	inf->Obj = obj;
	xsInit(&inf->EmailPath);
	xsInit(&inf->EmailStructPath);

	/** Calculate the path of the object relative to the root node. **/
	internalPath = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->SubPtr - 1, 2);

	/** Determine the type of the object. **/
	if (inf->Obj->SubPtr == inf->Obj->Pathname->nElements)
	    {
	    /** Opening the SMTP node object itself **/
	    inf->Obj->SubCnt = 1;

	    if (smtp_internal_OpenGeneral(inf, usrtype) < 0)
		{
		goto error;
		}

	    if (smtp_internal_OpenRoot(inf) < 0)
		{
		goto error;
		}
	    }
	else if (smtp_internal_IsEmail(internalPath) ||
		(inf->Obj->Mode & OBJ_O_AUTONAME &&
		!strcmp(internalPath + strlen(internalPath) - 2, "/*")))
	    {
	    /** Opening an email message to be managed by the SMTP object **/
	    inf->Obj->SubCnt = 2;

	    if (smtp_internal_OpenGeneral(inf, usrtype) < 0)
		{
		goto error;
		}

	    if (smtp_internal_OpenEml(inf) < 0)
		{
		goto error;
		}
	    }
	else
	    {
	    mssError(1,"SMTP","Could not open file");
	    goto error;
	    }

	/** Correct the the pathname. **/
	obj_internal_PathPart(obj->Pathname, 0, 0);

	return inf;

    error:
	if (inf)
	    {
	    smtp_internal_Close(inf);
	    }

	return NULL;
    }


/*** smtp_internal_Close() - close up.
 ***/
int
smtp_internal_Close(pSmtpData inf)
    {

	/** Check if the object is the root node. **/
	if (inf->AttributeNames)
	    {
	    xaFree(inf->AttributeNames);
	    }

	if (inf->Attributes)
	    {
	    xhClear(inf->Attributes, smtp_internal_ClearAttribute, NULL);
	    xhDeInit(inf->Attributes);
	    }

	if (inf->ContentFile)
	    {
	    if (fdClose(inf->ContentFile, 0))
		{
		mssError(0, "SMTP", "Unable to close email file.");
		return -1;
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

	xsDeInit(&inf->EmailPath);
	xsDeInit(&inf->EmailStructPath);
	nmFree(inf, sizeof(SmtpData));

    return 0;
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
	    if (node)
		{
		mssError(1, "SMTP", "Unable to create root node because it already exists.");
		goto error;
		}

	    node = smtp_internal_CreateRootNode(obj, mask);
	    if (!node)
		{
		mssError(1, "SMTP", "Unable to create root node.");
		goto error;
		}
	    }
	else if (obj->SubPtr+1 == obj->Pathname->nElements &&
		smtp_internal_IsEmail(obj->Pathname->Pathbuf))
	    {
	    /** Untested, but theoretically working... right? **/
	    inf = smtpOpen(obj, mask, systype, usrtype, oxt);
	    smtpClose(inf, oxt);
	    return 0;
	    }
	else
	    {
	    mssError(1,"SMTP","Could not create file");
	    goto error;
	    }

	return 0;

    error:
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

	/** Try to open it first. **/
	obj->Mode = O_RDWR;
	inf = (pSmtpData)smtpOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Determine the type of the object. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    mssError(1, "SMTP", "Not handling deleting root nodes.");
	    goto error;
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Delete the email file. **/
	    if (remove(inf->EmailPath.String))
		{
		mssErrorErrno(1, "SMTP", "Could not delete the email file.");
		goto error;
		}

	    /** Delete the email struct. **/
	    if (remove(inf->EmailStructPath.String))
		{
		mssErrorErrno(1, "SMTP", "Could not delete the email struct file.");
		goto error;
		}
	    }
	else
	    {
	    mssError(1,"SMTP","Could not delete indicated object.");
	    goto error;
	    }

	return smtp_internal_Close(inf);

    error:
	return -1;
    }


/*** smtpRead - Read from the SMTP object
 ***/
int
smtpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    int rval = -1;

	/** Read the contents of emails directly. **/
	if (inf->Type == SMTP_T_EML)
	    {
	    rval = fdRead(inf->ContentFile, buffer, maxcnt, offset, flags);
	    }

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
	if (inf->Type == SMTP_T_EML)
	    {
	    rval = fdWrite(inf->ContentFile, buffer, cnt, offset, flags);
	    }

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
	if (!qy)
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
	    if (!attr)
		{
		mssError(1,"SMTP","Unable to locate spool directory");
		goto error;
		}
	    spoolPath = attr->Value.String;

	    qy->Directory = opendir(spoolPath);
	    if (!qy->Directory)
		{
		mssErrorErrno(1,"SMTP","Could not open spool directory for query");
		goto error;
		}

	    return qy;
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    mssError(1, "SMTP", "Unable to query on system/smtp-message type objects");
	    goto error;
	    }

	mssError(1, "SMTP", "Invalid smtp object type.");

    error:
	if (qy)
	    {
	    smtpQueryClose(qy, NULL);
	    }

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
		mailEntry = readdir(qy->Directory);
		if (!mailEntry || smtp_internal_IsEmail(mailEntry->d_name))
		    {
		    break;
		    }
		}

	    if (!mailEntry)
		{
		/** End of query **/
		return NULL;
		}

	    if (obj_internal_AddToPath(obj->Pathname, mailEntry->d_name) < 0)
		{
		mssError(1, "SMTP", "Query result pathname exceeds internal limits");
		goto error;
		}
	    obj->Mode = mode;

	    inf = (pSmtpData)nmMalloc(sizeof(SmtpData));
	    if (!inf)
		{
		mssError(1, "SMTP", "Unable to create smtp data object");
		goto error;
		}
	    memset(inf, 0, sizeof(SmtpData));
	    inf->Obj = obj;

	    if (smtp_internal_OpenGeneral(inf, "system/smtp-message") < 0)
		goto error;

	    if (smtp_internal_OpenEml(inf) < 0)
		goto error;
	    }
	else if (qy->Data->Type == SMTP_T_EML)
	    {
	    mssError(1, "SMTP", "Unable to query smtp-message data objects");
	    goto error;
	    }

	return inf;

    error:
	if (inf)
	    smtp_internal_Close(inf);
	return NULL;
    }


/*** smtpQueryClose - close the query.
 ***/
int
smtpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pSmtpQueryData qy = SMTP_QY(qy_v);

	if (qy->Directory)
	    {
	    if (closedir(qy->Directory))
		{
		mssErrorErrno(1,"SMTP","Unable to close directory");
		}
	    }
	nmFree(qy, sizeof(SmtpQueryData));

    return 0;
    }


/*** smtpGetAttrType - get the type (DATA_T_json) of an attribute by name.
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
	if (!strcmp(attrname, "name")) return DATA_T_STRING;
	if (!strcmp(attrname, "content_type")) return DATA_T_STRING;
	if (!strcmp(attrname, "outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname, "inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname, "annotation")) return DATA_T_STRING;

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

	if (!strcmp(attrname, "name"))
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
	if (!strcmp(attrname,"inner_type") || !strcmp(attrname, "content_type"))
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

	/** If outer type, and it wasn't specified in the JSON **/
	if (!strcmp(attrname,"outer_type"))
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

	if (!strcmp(attrname, "annotation"))
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

    pFile emlStructFileRead = NULL;
    pFile emlStructFileWrite = NULL;
    pStructInf emlStruct = NULL;

    int old_int_val = -1;

	/** Get the requested attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	if (!attr)
	    {
	    /** Add the attribute if it is not found. **/
	    if (smtpAddAttr(inf, attrname, datatype, val, oxt))
		{
		mssError(0, "SMTP", "Unable to create the requested attribute object.");
		goto error;
		}

	    /** Get the newly created attribute. **/
	    attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	    if (!attr)
		{
		mssError(1, "SMTP", "Unable to open requested attribute object.");
		goto error;
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
	    goto error;
	    }

	/** We don't yet support null values **/
	if (!val)
	    {
	    mssError(1, "SMTP", "Error setting attribute %s to NULL (not supported).", attrname);
	    goto error;
	    }

	/** Store the data according to its data type. **/
	if (datatype == DATA_T_STRING)
	    {
	    if (attr->Value.String)
		nmSysFree(attr->Value.String);
	    attr->Value.String = nmSysStrdup(val->String);
	    if (!attr->Value.String)
		goto error;
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
	    if (!attr->Value.DateTime)
		goto error;
	    memcpy(attr->Value.DateTime, val->DateTime, sizeof(DateTime));
	    }
	else
	    {
	    goto error;
	    }

	/** Store the attribute into the correct file. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Read the root node into the node structure. **/
	    rootNode = snReadNode(inf->Obj->Prev);
	    if (!rootNode)
		{
		mssError(1, "SMTP", "Unable to open root node for writing");
		goto error;
		}

	    /** Set the attribute value in the root node. **/
	    if (stSetAttrValue(stLookup(rootNode->Data, attrname), datatype, val, 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto error;
		}

	    /** Mark root node DIRTY so that it will be written. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node back to the OS tree. **/
	    if (snWriteNode(inf->Obj->Prev, rootNode))
		{
		mssError(1, "SMTP", "Unable to write data to the root node");
		goto error;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Open the email structure file. **/
	    emlStructFileRead = fdOpen(inf->EmailStructPath.String,
					    inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL),
					    inf->Mask);
	    if (!emlStructFileRead)
		{
		mssError(1, "SMTP", "Could not open email structure file (%s).", inf->EmailStructPath.String);
		goto error;
		}

	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFileRead, 0);
	    if (!emlStruct)
		{
		mssError(0, "SMTP", "Could not parse the email structure file.");
		goto error;
		}

	    /** Set the given attribute value. **/
	    if (stSetAttrValue(stLookup(emlStruct, attrname), datatype, val, 0) < 0)
		{
		mssError(1, "SMTP", "Unable to set attribute '%s'", attrname);
		goto error;
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

	    /** Write changes to the email struct file. **/
	    if (stGenerateMsg(emlStructFileWrite, emlStruct, O_WRONLY | O_TRUNC | O_CREAT))
		{
		mssError(1, "SMTP", "Unable to write to the attribute to the email struct file.");
		goto error;
		}

	    /** If the email is ready to send, send it. **/
	    if (!strcmp(attrname, "is_ready") && val->Integer == 1 && old_int_val == 0)
		{
		if (smtp_internal_SendEmail(inf) < 0)
		    {
		    goto error;
		    }
		}
	    }

	/** Free appropriate memory and close appropriate files. **/
	if (emlStructFileWrite)
	    fdClose(emlStructFileWrite, 0);

	return 0;

    error:
	if (emlStructFileRead)
	    fdClose(emlStructFileRead, 0);

	if (emlStructFileWrite)
	    fdClose(emlStructFileWrite, 0);

	return -1;
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
    pStructInf createdStruct = NULL;

    pSnNode rootNode = NULL;

    pFile emlStructFile = NULL;
    pStructInf emlStruct = NULL;

	/** Initialize the new attribute. **/
	attr = nmMalloc(sizeof(SmtpAttribute));
	if (!attr)
	    {
	    mssError(1,"SMTP","Could not create new attribute object.");
	    return -1;
	    }

	/** Set the meta-data fields of the new attribute. **/
	attr->Name = nmSysStrdup(attrname);
	if (!attr->Name)
	    goto error;
	attr->Type = type;

	/** Add the new attribute to the attribute name list. **/
	xaAddItem(inf->AttributeNames, attr->Name);

	/** Set the default value appropriately if it is a string. **/
	if (attr->Type == DATA_T_STRING)
	    {
	    attr->Value.String = nmSysStrdup("");
	    if (!attr->Value.String)
		goto error;
	    }

	/** Set the default value appropriately if it is a integer. **/
	if (attr->Type == DATA_T_INTEGER)
	    {
	    attr->Value.Integer = 0;
	    }

	/** Add the attribute to the attribute hash. **/
	xhAdd(inf->Attributes, attr->Name, (char*)attr);

	/** Create the attribute in the correct location according to object type. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Open the root node. **/
	    rootNode = snReadNode(inf->Obj->Prev);
	    if (!rootNode)
		{
		mssError(1, "SMTP", "Unable to open root node.");
		goto error;
		}

	    /** Add the attribute to the root node. **/
	    createdStruct = stAddAttr(rootNode->Data, attr->Name);
	    if (!createdStruct)
		{
		mssError(1, "SMTP", "Unable to add new attribute to the root node.");
		goto error;
		}

	    /** Set the default attribute value. **/
	    if (stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto error;
		}

	    /** Set the root node to DIRTY so it will be written to the file. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node. **/
	    if (snWriteNode(inf->Obj->Prev, rootNode))
		{
		mssError(1, "SMTP", "Unable to write root node.");
		goto error;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Open the email structure file. **/
	    emlStructFile = fdOpen(inf->EmailStructPath.String,
					    inf->Obj->Mode & ~(O_TRUNC | O_CREAT | O_EXCL),
					    inf->Mask);
	    if (!emlStructFile)
		{
		mssError(1, "SMTP", "Could not open email structure file (%s).", inf->EmailStructPath.String);
		goto error;
		}

	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFile, 0);
	    if (!emlStruct)
		{
		mssError(0, "SMTP", "Could not parse the email structure file.");
		goto error;
		}

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emlStruct, attr->Name);
	    if (!createdStruct)
		{
		mssError(1, "SMTP", "Could not add attribute '%s' to the email struct.", attr->Name);
		goto error;
		}

	    /** Set the attribute value. **/
	    if (stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0) < 0)
		{
		mssError(1, "SMTP", "Unable to write to the given attribute '%s'", attr->Name);
		goto error;
		}

	    /** Write changes to the email struct file. **/
	    if (stGenerateMsg(emlStructFile, emlStruct, O_WRONLY | O_TRUNC | O_CREAT) < 0)
		{
		mssError(0, "SMTP", "Unable to write the updated email struct file.");
		goto error;
		}
	    }

	/** Free appropriate memory and close appropriate files. **/
	if (emlStructFile)
	    fdClose(emlStructFile, 0);

	return 0;

    error:
	/** Free appropriate memory and close appropriate files. **/
	if (emlStructFile)
	    fdClose(emlStructFile, 0);

	return -1;
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
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** If globals are not yet initialized, initialize them.			**/
	/** We don't always need globals, but when we do, they should be initialized.	**/
	/** jk. They are the globals we deserve, but not the ones we need right now.	**/
	/** jk. We need Batman. And globals.						**/
	smtp_internal_InitGlobals();

	/** Setup the structure **/
	strcpy(drv->Name,"SMTP - Simple Mail Transfer Protocol OS Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),1);
	xaAddItem(&(drv->RootContentTypes),"system/smtp");

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

	/** nmRegister(sizeof(JsonData),"JsonData"); **/

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(smtpInitialize);
MODULE_PREFIX("smtp");
MODULE_DESC("SMTP ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);
