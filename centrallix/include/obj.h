#ifndef _OBJ_H
#define _OBJ_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/************************************************************************/


#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "cxlib/xhashqueue.h"
#include "cxlib/mtask.h"
#include "cxlib/datatypes.h"
#include "stparse_ne.h"
#include "cxlib/newmalloc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "cxlib/xhandle.h"
#include "ptod.h"

#define OBJSYS_DEFAULT_ROOTNODE		"/usr/local/etc/rootnode"
#define OBJSYS_DEFAULT_ROOTTYPE		"/usr/local/etc/rootnode.type"
#define OBJSYS_DEFAULT_TYPES_CFG	"/usr/local/etc/types.cfg"

#define OBJSYS_MAX_PATH		256
#define OBJSYS_MAX_ELEMENTS	32
#define OBJSYS_MAX_ATTR		64

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define OBJSYS_NOT_RELATED	(0x80000000)

#define OBJ_U_SEEK	FD_U_SEEK
#define OBJ_U_PACKET	FD_U_PACKET
#define OBJ_U_TRUNCATE	FD_U_TRUNCATE

#define MGK_OXT		(0x12340ddf)

/** Pathname analysis structure **/
typedef struct
    {
    char		Pathbuf[OBJSYS_MAX_PATH];
    int			nElements;
    char*		Elements[OBJSYS_MAX_ELEMENTS];
    char*		OpenCtlBuf;
    int			OpenCtlLen;
    int			OpenCtlCnt;
    pStruct		OpenCtl[OBJSYS_MAX_ELEMENTS];
    int			LinkCnt;
    }
    Pathname, *pPathname;

#define OBJ_TYPE_NAMES_CNT  32
extern char* obj_type_names[];

extern char* obj_short_months[];
extern char* obj_long_months[];
extern unsigned char obj_month_days[];
extern char* obj_short_week[];
extern char* obj_long_week[];
extern char* obj_default_date_fmt;
extern char* obj_default_money_fmt;
extern char* obj_default_null_fmt;

#define IS_LEAP_YEAR(x) ((x)%4 == 0 && ((x)%100 != 0 || (x)%400 == 0))

/** Datatype conversion functions - flags **/
#define DATA_F_QUOTED		1
#define DATA_F_SINGLE		2
#define DATA_F_NORMALIZE	4
#define DATA_F_SYBQUOTE		8	/* use '' to quote a ', etc */
#define DATA_F_CONVSPECIAL	16	/* convert literal CR LF and TAB to \r \n and \t */
#define DATA_F_DATECONV		32	/* wrap date/time values using convert() */
#define DATA_F_BRACKETS		64	/* square brackets for intvec/stringvec */


/** Presentation Hints structure ---
 ** Any fields can be NULL to indicate that the hint doesn't apply.
 ** EnumQuery must return one or two columns: a data column followed by an
 ** optional description column.  Bitmasked fields can have a maximum of
 ** 32 distinct bits; the data column should be an integer from 0 to 31.
 **/
typedef struct _PH
    {
    void*		Constraint;	/* expression controlling what are valid values */
    void*		DefaultExpr;	/* expression defining the default value. */
    void*		MinValue;	/* minimum value expression */
    void*		MaxValue;	/* maximum value expression */
    XArray		EnumList;	/* list of string values */
    char*		EnumQuery;	/* query to enumerate the possible values */
    char*		Format;		/* presentation format - datetime or money */
    char*		AllowChars;	/* characters allowed in this string, if null all allowed */
    char*		BadChars;	/* characters *not* allowed in this string */
    int			Length;		/* Max length of data that can be entered */
    int			VisualLength;	/* length of data field as presented to user */
    int			VisualLength2;	/* used primarily for # lines in a multiline edit */
    unsigned int	BitmaskRO;	/* which bits, if any, in bitmask are read-only */
    int			Style;		/* style information - bitmask OBJ_PH_STYLE_xxx */
    int			StyleMask;	/* which style information is actually supplied */
    int			GroupID;	/* for grouping fields together.  -1 if not grouped */
    char*		GroupName;	/* Name of group, or NULL if no group or group named elsewhere */
    int			OrderID;	/* allows for reordering the fields (e.g., tab order) */
    char*		FriendlyName;	/* Presented 'friendly' name of this attribute */
    }
    ObjPresentationHints, *pObjPresentationHints;

/*** Hints style info - keep in sync with ht_utils_hints.js in centrallix-os ***/
#define OBJ_PH_STYLE_BITMASK	1	/* items from EnumQuery or EnumList are bitmasked */
#define OBJ_PH_STYLE_LIST	2	/* use a list style presentation for enum types */
#define OBJ_PH_STYLE_BUTTONS	4	/* use radio button or checkboxes for enum types */
#define OBJ_PH_STYLE_NOTNULL	8	/* field does not allow nulls */
#define OBJ_PH_STYLE_STRNULL	16	/* empty string == null */
#define OBJ_PH_STYLE_GROUPED	32	/* check GroupID for grouping fields together */
#define OBJ_PH_STYLE_READONLY	64	/* user can't modify */
#define OBJ_PH_STYLE_HIDDEN	128	/* don't present this field to the user */
#define OBJ_PH_STYLE_PASSWORD	256	/* hide string as user types */
#define OBJ_PH_STYLE_MULTILINE	512	/* string value allows multiline editing */
#define OBJ_PH_STYLE_HIGHLIGHT	1024	/* highlight this attribute */
#define OBJ_PH_STYLE_LOWERCASE	2048	/* This attribute is lowercase-only */
#define OBJ_PH_STYLE_UPPERCASE	4096	/* This attribute is uppercase-only */
#define OBJ_PH_STYLE_TABPAGE	8192	/* Prefer tabpage layout for grouped fields */
#define OBJ_PH_STYLE_SEPWINDOW	16384	/* Prefer separate windows for grouped fields */
#define OBJ_PH_STYLE_ALWAYSDEF	32768	/* Always reset default value on any modify */
#define OBJ_PH_STYLE_CREATEONLY	65536	/* Writable only during record creation */
#define OBJ_PH_STYLE_MULTISEL	131072	/* Multiple select */
#define OBJ_PH_STYLE_KEY	262144	/* Field is a primary key */
#define OBJ_PH_STYLE_APPLYCHG	524288	/* Apply hints on DataChange, not on DataModify */


/** objectsystem driver **/
typedef struct _OSD
    {
    char	Name[64];
    XArray	RootContentTypes;
    int		Capabilities;
    void*	(*Open)();
    void*	(*OpenChild)();
    int		(*Close)();
    int		(*Create)();
    int		(*Delete)();
    int		(*DeleteObj)();
    void*	(*OpenQuery)();
    int		(*QueryDelete)();
    void*	(*QueryFetch)();
    void*	(*QueryCreate)();
    int		(*QueryClose)();
    int		(*Read)();
    int		(*Write)();
    int		(*GetAttrType)();
    int		(*GetAttrValue)();
    char*	(*GetFirstAttr)();
    char*	(*GetNextAttr)();
    int		(*SetAttrValue)();
    int		(*AddAttr)();
    void*	(*OpenAttr)();
    char*	(*GetFirstMethod)();
    char*	(*GetNextMethod)();
    int		(*ExecuteMethod)();
    pObjPresentationHints (*PresentationHints)();
    int		(*Info)();
    int		(*Commit)();
    int		(*GetQueryCoverageMask)();
    int		(*GetQueryIdentityPath)();
    }
    ObjDriver, *pObjDriver;

#define OBJDRV_C_FULLQUERY	1
#define OBJDRV_C_TRANS		2
#define OBJDRV_C_LLQUERY	4
#define OBJDRV_C_ISTRANS	8
#define OBJDRV_C_ISMULTIQUERY	16
#define OBJDRV_C_INHERIT	32	/* driver desires inheritance support */
#define OBJDRV_C_ISINHERIT	64	/* driver is the inheritance layer */
#define OBJDRV_C_OUTERTYPE	128	/* driver layering depends on outer, not inner, type */
#define OBJDRV_C_NOAUTO		256	/* driver should never be automatically invoked */
#define OBJDRV_C_DOWNLOADAS	512	/* driver supports the cx__download_as attribute */

/** objxact transaction tree **/
typedef struct _OT
    {
    int			Magic;
    int			Status;		/* OXT_S_xxx */
    int			OpType;		/* OXT_OP_xxx */
    struct _OT*		Parent;		/* Parent tree node */
    struct _OT*		Parallel;	/* Linked-list stack of parallel ops */
    struct _OT*		Next;		/* Next link in time-ordered completion */
    struct _OT*		Prev;		/* With the above; doubly-linked */
    XArray		Children;	/* Child tree nodes */
    void*		Object;		/* pObject pointer */
    int			AllocObj;	/* if object was allocated for this */
    char		UsrType[80];	/* User requested create type */
    int			Mask;		/* User requested create mask */
    pObjDriver		LLDriver;
    void*		LLParam;
    int			LinkCnt;
    char*		PathPtr;
    char		AttrName[64];
    int			AttrType;
    void*		AttrValue;
    }
    ObjTrxTree, *pObjTrxTree;

#define OXT_S_PENDING	0
#define OXT_S_VISITED	1
#define OXT_S_COMPLETE	2
#define OXT_S_FAILED	3

#define OXT_OP_NONE	0
#define OXT_OP_CREATE	1
#define OXT_OP_SETATTR	2
#define OXT_OP_DELETE	3


/** objectsystem session **/
typedef struct _OSS
    {
    int			Magic;
    char		CurrentDirectory[OBJSYS_MAX_PATH];
    XArray		OpenObjects;
    XArray		OpenQueries;
    pObjTrxTree		Trx;
    XHashQueue		DirectoryCache;		/* directory entry cache */
    handle_t		Handle;
    int			Flags;
    }
    ObjSession, *pObjSession;

#define OBJ_SESS_F_CLOSING	1


/** Content type descriptor **/
typedef struct _CT
    {
    char	Name[128];
    char	Description[256];
    XArray	Extensions;
    XArray	IsA;
    int		Flags;
    XArray	RelatedTypes;
    XArray	RelationLevels;
    void*	TypeNameObjList;	/* pParamObjects */
    void*	TypeNameExpression;	/* pExpression */
    }
    ContentType, *pContentType;

#define CT_F_HASCONTENT		1
#define CT_F_HASCHILDREN	2
#define CT_F_TOPLEVEL		4

/** object additional information / capabilities **/
typedef struct _OA
    {
    int		Flags;		/* OBJ_INFO_F_xxx, below */
    int		nSubobjects;	/* count of subobjects, if known */
    }
    ObjectInfo, *pObjectInfo;

/** info flags **/
#define	OBJ_INFO_F_NO_SUBOBJ		(1<<0)	/* object has no subobjects */
#define OBJ_INFO_F_HAS_SUBOBJ		(1<<1)	/* object has at least one subobject */
#define OBJ_INFO_F_CAN_HAVE_SUBOBJ	(1<<2)	/* object *can* have subobjects */
#define OBJ_INFO_F_CANT_HAVE_SUBOBJ	(1<<3)	/* object *cannot* have subobjects */
#define OBJ_INFO_F_SUBOBJ_CNT_KNOWN	(1<<4)	/* number of subobjects is known */
#define OBJ_INFO_F_CAN_ADD_ATTR		(1<<5)	/* attributes can be added to object */
#define OBJ_INFO_F_CANT_ADD_ATTR	(1<<6)	/* attributes cannot be added to object */
#define OBJ_INFO_F_CAN_SEEK_FULL	(1<<7)	/* OBJ_U_SEEK fully supported on object */
#define OBJ_INFO_F_CAN_SEEK_REWIND	(1<<8)	/* OBJ_U_SEEK supported with '0' offset */
#define OBJ_INFO_F_CANT_SEEK		(1<<9)	/* OBJ_U_SEEK never honored */
#define OBJ_INFO_F_CAN_HAVE_CONTENT	(1<<10)	/* object can have content */
#define OBJ_INFO_F_CANT_HAVE_CONTENT	(1<<11)	/* object cannot have content */
#define OBJ_INFO_F_HAS_CONTENT		(1<<12)	/* object actually has content, even if zero-length */
#define OBJ_INFO_F_NO_CONTENT		(1<<13)	/* object does not have content, objRead() would fail */
#define OBJ_INFO_F_SUPPORTS_INHERITANCE	(1<<14)	/* object can support inheritance attr cx__inherit, etc. */
#define OBJ_INFO_F_FORCED_LEAF		(1<<15)	/* object is forced to be a 'leaf' unless ls__type used. */
#define OBJ_INFO_F_TEMPORARY		(1<<16)	/* this is a temporary object without a vaoid pathname. */


/** object virtual attribute - these are attributes which persist only while
 ** the object is open, and whose values and types are obtained by invoking
 ** external function calls.  Can be used to dynamically add an attribute to
 ** an open object.
 **/
typedef struct _OVA
    {
    struct _OVA*	Next;		/* next virtual attr in the list */
    char		Name[32];	/* name of the attribute */
    void*		Context;	/* arbitrary data from addvirtualattr() caller */
    int			(*TypeFn)();	/* GetAttrType */
    int			(*GetFn)();	/* GetAttrValue */
    int			(*SetFn)();	/* SetAttrValue */
    int			(*FinalizeFn)(); /* called when about to close the object */
    }
    ObjVirtualAttr, *pObjVirtualAttr;


/** objectsystem open fd **/
typedef struct _OF
    {
    int		Magic;		/* Magic number for object */
    pObjDriver	Driver;		/* os-Driver handling this object */
    pObjDriver	TLowLevelDriver; /* for transaction management */
    pObjDriver	ILowLevelDriver; /* for inheritance mechanism */
    void*	Data;		/* context param passed to os-Driver */
    struct _OF*	Obj;		/* if this is an attribute, this is its obj */
    XArray	Attrs;		/* Attributes that are open. */
    pPathname	Pathname;	/* Pathname of this object */
    short 	SubPtr;		/* First component of path handled by this driver */
    short 	SubCnt;		/* Number of path components used by this driver */
    short	Flags;		/* OBJ_F_xxx, see below */
    int		Mode;		/* Mode: O_RDONLY / O_CREAT / etc */
    pContentType Type;
    pObjSession	Session;
    int		LinkCnt;	/* Don't _really_ close until --LinkCnt == 0 */
    pXString	ContentPtr;	/* buffer for accessing obj:objcontent */
    struct _OF*	Prev;		/* open object for accessing the "node" */
    struct _OF*	Next;		/* next object for intermediate opens chain */
    ObjectInfo	AdditionalInfo;	/* see ObjectInfo definition above */
    void*	NotifyItem;	/* pObjReqNotifyItem; not-null when notifies are active on this */
    pObjVirtualAttr VAttrs;	/* virtual attributes - call external fn()'s to obtain the data */
    void*	EvalContext;	/* a pParamObjects list -- for evaluation of runserver() exprs */
    void*	AttrExp;	/* an expression used for the above */
    char*	AttrExpName;	/* Name of attr for above expression */
    DateTime	CacheExpire;	/* Date/time after which cached data is no longer valid */
    int		RowID;
    }
    Object, *pObject;

#define OBJ_F_ROOTNODE		1	/* is rootnode object, handle specially */
#define	OBJ_F_CREATED		2	/* O_CREAT requested; object didn't exist but was created */
#define OBJ_F_DELETE		4	/* object should be deleted on final close */
#define OBJ_F_NOCACHE		8	/* object should *not* be cached by the Directory Cache */
#define	OBJ_F_METAONLY		16	/* user opened '?' object */
#define OBJ_F_UNMANAGED		32	/* don't auto-close on session closure */
#define OBJ_F_TEMPORARY		64	/* created by objCreateTempObject() */


/** temporary collection indexes **/
typedef struct _TI
    {
    XArray	Fields;		/* List of fields in this index */
    XHashTable	Index;		/* FIXME: replace with B+ tree in the future */
    void*	OneObjList;	/* pParamObjects; for convenience */
    int		IsUnique:1;
    }
    ObjTempIndex, *pObjTempIndex;


/** structure for temporary objects **/
typedef struct _TO
    {
    handle_t	Handle;
    int		LinkCnt;
    void*	Data;		/* pStructInf */
    long long	CreateCnt;
    XArray	Indexes;	/* of pObjTempIndex */
    }
    ObjTemp, *pObjTemp;


/** structure used for sorting a query result set. **/
typedef struct _SRT
    {
    XArray	SortPtr[2];	/* ptrs to sort key data */
    XArray	SortPtrLen[2];	/* lengths of sort key data */
    XArray	SortNames[2];	/* names of objects */
    XString	SortDataBuf;	/* buffer for sort key data */
    XString	SortNamesBuf;	/* buffer for object names */
    int		Reopen;
    }
    ObjQuerySort, *pObjQuerySort;


/** object query information **/
typedef struct _OQ
    {
    int		Magic;
    pObject	Obj;
    char*	QyText;
    void*	Tree;	/* pExpression */
    void*	SortBy[16];	/* pExpression [] */
    void*	ObjList; /* pParamObjects */
    void*	Data;
    int		Flags;
    int		RowID;
    pObjQuerySort SortInf;
    pObjDriver	Drv;		/* used for multiquery only */
    pObjSession	QySession;	/* used for multiquery only */
    }
    ObjQuery, *pObjQuery;

#define OBJ_QY_F_ALLOCTREE	1
#define OBJ_QY_F_FULLQUERY	2
#define OBJ_QY_F_FULLSORT	4
#define OBJ_QY_F_FROMSORT	8
#define OBJ_QY_F_NOREOPEN	16


/*** Event and EventHandler structures ***/
typedef struct _OEH
    {
    char		ClassCode[16];
    int			(*HandlerFunction)();
    }
    ObjEventHandler, *pObjEventHandler;

typedef struct _OE
    {
    char		DirectoryPath[OBJSYS_MAX_PATH];
    char*		XData;
    char*		WhereClause;
    char		ClassCode[16];
    int			Flags;
    pObjEventHandler	Handler;
    }
    ObjEvent, *pObjEvent;

#define OBJ_EV_F_NOSAVE		1	/* internal - don't rewrite events file */


extern int obj_internal_DiscardDC(pXHashQueue hq, pXHQElement xe, int locked);

/** directory entry caching data **/
typedef struct _DC
    {
    char		Hashname[4 + OBJSYS_MAX_PATH*2];	/* allow room for params */
    char		Pathname[OBJSYS_MAX_PATH];
    pObject		NodeObj;
    }
    DirectoryCache, *pDirectoryCache;

#define DC_F_ISDIR	1


/*** Lock information structure ***/
typedef struct _LK
    {
    char	Path[OBJSYS_MAX_PATH];	/* Path to object or subtree to be locked */
    int		Flags;			/* Locking flags OBJ_LOCK_F_xxx */
    XArray	Holders;		/* A list of holders with access through this lock */
    pSemaphore	WriteLock;
    }
    ObjLock, *pObjLock;

#define OBJ_LOCK_F_WRITE	1	/* Lock is a write (exclusive) lock (otherwise, it is a reader/shared lock) */
#define OBJ_LOCK_F_SUBTREE	2	/* Lock applies to entire object subtree (otherwise, just to given tree or object) */
#define OBJ_LOCK_F_OBJECT	4	/* Lock applies to object itself (otherwise, to object's direct subobjects) */


/*** Lock holder information ***/
typedef struct _LKH
    {
    char	Path[OBJSYS_MAX_PATH];	/* Path that holder requested */
    int		Flags;			/* Flags that holder requested */
    pObjLock	Lock;			/* the lock in question */
    pObjSession	Session;		/* the holder's OSML session */
    }
    ObjLockHolder, *pObjLockHolder;


/*** ObjectSystem Globals ***/
typedef struct
    {
    XArray	OpenSessions;		/* open ObjectSystem sessions */
    XHashTable	TypeExtensions;		/* extension -> type mapping */
    XHashTable	DriverTypes;		/* type -> driver mapping */
    XArray	Drivers;		/* Registered driver list */
    XHashTable	Types;			/* Just a registered type list */
    XArray	TypeList;		/* Iterable registered type list */
    int		UseCnt;			/* for LRU cache list */
    pObjDriver	TransLayer;		/* Transaction layer */
    pObjDriver	MultiQueryLayer;	/* MQ module */
    pObjDriver	InheritanceLayer;	/* dynamic inheritance module */
    XHashTable	EventHandlers;		/* Event handlers, by class code */
    XHashTable	EventsByXData;		/* Events, by class code/xdata */
    XHashTable	EventsByPath;		/* Events, by pathname */
    XArray	Events;			/* Table of events */
    pContentType RootType;		/* Type of root node */
    char	RootPath[OBJSYS_MAX_PATH]; /* Path to root node file */
    pObjDriver	RootDriver;
    HandleContext SessionHandleCtx;	/* context for session handles */
    XHashTable	NotifiesByPath;		/* objects with RequestNotify() */
    long long	PathID;			/* pseudo-paths for multiquery */
    char	TrxLogPath[OBJSYS_MAX_PATH]; /* path to osml trx log */
    XArray	Locks;			/* Object and subtree locks */
    HandleContext TempObjects;		/* Handle table of temporary objects */
    pObjDriver	TempDriver;
    }
    OSYS_t;

extern OSYS_t OSYS;


/*** structure for managing request-notify ***/
typedef struct
    {
    int		TotalFlags;		/* OBJ_RN_F_xxx bitwise ORed from all */
    char	Pathname[OBJSYS_MAX_PATH];  /* path to object */
    XArray	Requests;		/* all requests for notification */
    }
    ObjReqNotify, *pObjReqNotify;


/*** structure for ONE request-notify ***/
typedef struct _ORNI
    {
    struct _ORNI* Next;
    pObjReqNotify NotifyStruct;		/* parent structure */
    pObject	Obj;			/* object handle */
    int		Flags;			/* OBJ_RN_F_xxx for this requestor */
    int		(*CallbackFn)();	/* callback function */
    void*	CallerContext;		/* passed in by caller */
    MTSecContext SavedSecContext;	/* security context of requestor */
    XArray	Notifications;		/* active notifications on this item */
    }
    ObjReqNotifyItem, *pObjReqNotifyItem;

/*** structure for delivery of notification ***/
typedef struct
    {
    pObjReqNotifyItem __Item;		/* for internal use only */
    pObject	Obj;			/* the object being modified */
    void*	Context;		/* context provided on ReqNotify() */
    int		What;			/* what is being modified - OBJ_RN_F_xxx */
    char	Name[MAX(OBJSYS_MAX_ATTR,OBJSYS_MAX_PATH)];	/* attribute name or object name, if needed */
    TObjData	NewAttrValue;		/* new value of attr */
    int		PtrSize;		/* size of Ptr data */
    int		Offset;			/* if content, offset to Ptr data */
    int		NewSize;		/* if content, new size of content */
    void*	Ptr;			/* actual information */
    int		IsDel;			/* 1 if Name is being deleted, 0 if added */
    pThread	Worker;			/* set if notify worker thread active */
    }
    ObjNotification, *pObjNotification;


/*** OSML debugging flags ***/
#define OBJ_DEBUG_F_APITRACE	1

/*** Debugging control ***/
/*#define OBJ_DEBUG		(OBJ_DEBUG_F_APITRACE)*/
#define OBJ_DEBUG		(0)

/*** Debugging output macro ***/
#define OSMLDEBUG(f,p ...) if (OBJ_DEBUG & (f)) printf(p);


/*** ObjectSystem Driver Library - parameters structure ***/
typedef struct
    {
    char	Name[32];
    int		Type;
    int		IntParam;
    char	StringParam[80];
    }
    ObjParam, *pObjParam;

/*** Flags to be passed to objOpen() in the 'mode' ***/
#define OBJ_O_CREAT	(O_CREAT)
#define OBJ_O_TRUNC	(O_TRUNC)
#define OBJ_O_RDWR	(O_RDWR)
#define OBJ_O_RDONLY	(O_RDONLY)
#define OBJ_O_WRONLY	(O_WRONLY)
#define OBJ_O_EXCL	(O_EXCL)

#define OBJ_O_ACCMODE	(O_ACCMODE)

#define OBJ_O_AUTONAME	(1<<30)
#define OBJ_O_NOINHERIT	(1<<29)

#define OBJ_O_CXOPTS	(OBJ_O_AUTONAME | OBJ_O_NOINHERIT)

#if (OBJ_O_CXOPTS & (O_CREAT | O_TRUNC | O_ACCMODE | O_EXCL))
#error "Conflict in objectsystem OBJ_O_xxx options, sorry!!!"
#endif


/*** Flags for objRequestNotify() replication function ***/
#define	OBJ_RN_F_INIT		(1<<0)		/* send initial state */
#define	OBJ_RN_F_ATTRIB		(1<<1)		/* attribute changes */
#define OBJ_RN_F_CONTENT	(1<<2)		/* content changes */
#define OBJ_RN_F_SUBOBJ		(1<<3)		/* subobject insert/delete */
#define OBJ_RN_F_SUBTREE	(1<<4)		/* all descendents */


/*** Flags for objMultiQuery() ***/
#define OBJ_MQ_F_ONESTATEMENT	(1<<0)		/* only permit one statement to run */
#define OBJ_MQ_F_NOUPDATE	(1<<1)		/* disallow any updates in this query */


/** objectsystem main functions **/
int objInitialize();
int rootInitialize();
int oxtInitialize();
int objRegisterDriver(pObjDriver drv);

/** objectsystem session functions **/
pObjSession objOpenSession(char* current_dir);
int objCloseSession(pObjSession this);
int objSetWD(pObjSession this, pObject wd);
char* objGetWD(pObjSession this);
int objSetDateFmt(pObjSession this, char* fmt);
char* objGetDateFmt(pObjSession this);
int objUnmanageObject(pObjSession this, pObject obj);
int objUnmanageQuery(pObjSession this, pObjQuery qy);
int objCommit(pObjSession this);
int objCommitObject(pObject this);
pObjTrxTree objSuspendTransaction(pObjSession this);
int objResumeTransaction(pObjSession this, pObjTrxTree trx);

/** objectsystem object functions **/
pObject objOpen(pObjSession session, char* path, int mode, int permission_mask, char* type);
pObject objOpenChild(pObject obj, char* name, int mode, int permission_mask, char* type);
int objClose(pObject this);
int objCreate(pObjSession session, char* path, int permission_mask, char* type);
int objDelete(pObjSession session, char* path);
int objDeleteObj(pObject this);
pObject objLinkTo(pObject this);
pObjectInfo objInfo(pObject this);
char* objGetPathname(pObject this);
int objImportFile(pObjSession sess, char* source_filename, char* dest_osml_dir, char* new_osml_name, int new_osml_name_len);
pContentType objTypeFromName(char* name);
int objIsRelatedType(char* type1, char* type2);

/** objectsystem directory/query functions **/
pObjQuery objMultiQuery(pObjSession session, char* query, void* objlist, int flags);
pObjQuery objOpenQuery(pObject obj, char* query, char* order_by, void* tree, void** orderby_exp, int flags);
int objQueryDelete(pObjQuery this);
pObject objQueryFetch(pObjQuery this, int mode);
pObject objQueryCreate(pObjQuery this, char* name, int mode, int permission_mask, char* type);
int objQueryClose(pObjQuery this);
int objGetQueryCoverageMask(pObjQuery this);
int objGetQueryIdentityPath(pObjQuery this, char* buf, int maxlen);

/** objectsystem content functions **/
int objRead(pObject this, char* buffer, int maxcnt, int offset, int flags);
int objWrite(pObject this, char* buffer, int cnt, int offset, int flags);

/** objectsystem attribute functions **/
int objGetAttrType(pObject this, char* attrname);
int objSetEvalContext(pObject this, void* objlist);
void* objGetEvalContext(pObject this);
#if 1
int objSetAttrValue(pObject this, char* attrname, int data_type, pObjData val);
int objGetAttrValue(pObject this, char* attrname, int data_type, pObjData val);
#else
#define _OBJATTR_CONV
int objSetAttrValue();
int objGetAttrValue();
#endif
char* objGetFirstAttr(pObject this);
char* objGetNextAttr(pObject this);
pObject objOpenAttr(pObject this, char* attrname, int mode);
int objAddAttr(pObject this, char* attrname, int type, pObjData val);
pObjPresentationHints objPresentationHints(pObject this, char* attrname);
int objFreeHints(pObjPresentationHints ph);
int objAddVirtualAttr(pObject this, char* attrname, void* context, int (*type_fn)(), int (*get_fn)(), int (*set_fn)(), int (*finalize_fn)());

/** objectsystem method functions **/
char* objGetFirstMethod(pObject this);
char* objGetNextMethod(pObject this);
int objExecuteMethod(pObject this, char* methodname, pObjData param);

/** objectsystem driver library functions **/
pXArray objParamsRead(pFile fd);
int objParamsWrite(pFile fd, pXArray params);
int objParamsLookupInt(pXArray params, char* name);
char* objParamsLookupString(pXArray params, char* name);
int objParamsSet(pXArray params, char* name, char* stringval, int intval);
int objParamsFree(pXArray params);
int obj_internal_FreePath(pPathname this);
int obj_internal_FreePathStruct(pPathname this);
pPathname obj_internal_NormalizePath(char* cwd, char* name);
int obj_internal_AddChildTree(pObjTrxTree parent_oxt, pObjTrxTree child_oxt);
pObject obj_internal_AllocObj();
int obj_internal_FreeObj(pObject);
int obj_internal_TrxLog(pObject this, char* op, char* fmt, ...);

/** objectsystem transaction functions **/
int obj_internal_FreeTree(pObjTrxTree oxt);
pObjTrxTree obj_internal_AllocTree();
pObjTrxTree obj_internal_FindTree(pObjTrxTree oxt, char* path);
int obj_internal_SetTreeAttr(pObjTrxTree oxt, int type, pObjData val);
pObjTrxTree obj_internal_FindAttrOxt(pObjTrxTree oxt, char* attrname);

/** objectsystem path manipulation **/
char* obj_internal_PathPart(pPathname path, int start_element, int length);
int obj_internal_PathPrefixCnt(pPathname full_path, pPathname prefix);
int obj_internal_CopyPath(pPathname dest, pPathname src);
int obj_internal_AddToPath(pPathname path, char* new_element);
int obj_internal_RenamePath(pPathname path, int element_id, char* new_element);

/** objectsystem datatype functions **/
int objDataToString(pXString dest, int data_type, void* data_ptr, int flags);
double objDataToDouble(int data_type, void* data_ptr);
int objDataToInteger(int data_type, void* data_ptr, char* format);
int objDataToDateTime(int data_type, void* data_ptr, pDateTime dt, char* format);
int objDataToMoney(int data_type, void* data_ptr, pMoneyType m);
char* objDataToStringTmp(int data_type, void* data_ptr, int flags);
int objDataCompare(int data_type_1, void* data_ptr_1, int data_type_2, void* data_ptr_2);
char* objDataToWords(int data_type, void* data_ptr);
int objCopyData(pObjData src, pObjData dst, int type);
int objTypeID(char* name);
int objDebugDate(pDateTime dt);
int objDataFromString(pObjData pod, int type, char* str);
int objDataFromStringAlloc(pObjData pod, int type, char* str);
char* objFormatMoneyTmp(pMoneyType m, char* format);
char* objFormatDateTmp(pDateTime dt, char* format);
int objCurrentDate(pDateTime dt);
int objBuildBinaryImage(char* buf, int buflen, void* /* pExpression* */ fields, int n_fields, void* /* pParamObjects */ objlist, int asciz);
int objBuildBinaryImageXString(pXString str, void* /* pExpression* */ fields, int n_fields, void* /* pParamObjects */ objlist, int asciz);
int objDateAdd(pDateTime dt, int diff_sec, int diff_min, int diff_hr, int diff_day, int diff_mo, int diff_yr);


/** objectsystem replication services - open object notification (Rn) system **/
int objRequestNotify(pObject this, int (*callback_fn)(), void* context, int what);
int obj_internal_RnDelete(pObjReqNotifyItem item);
int obj_internal_RnNotifyAttrib(pObject this, char* attrname, pTObjData newvalue, int send_this);
int objDriverAttrEvent(pObject this, char* attr_name, pTObjData newvalue, int send_this);


/** objectsystem event handler stuff -- for os drivers etc **/
int objRegisterEventHandler(char* class_code, int (*handler_function)());
int objRegisterEvent(char* class_code, char* pathname, char* where_clause, int flags, char* xdata);
int objUnRegisterEvent(char* class_code, char* xdata);


/** temporary objects **/
handle_t objCreateTempObject();
pObject objOpenTempObject(pObjSession session, handle_t tempobj, int mode);
int objDeleteTempObject(handle_t tempobj);

#endif /*_OBJ_H*/
