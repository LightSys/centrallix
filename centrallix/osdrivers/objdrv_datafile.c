#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "cxlib/strtcpy.h"
#include "st_node.h"
#include "stparse.h"
#include "stparse_ne.h"
#include "hints.h"
#include "cxlib/util.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_datafile.c     					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 17, 1999					*/
/* Description:	Objectsystem driver for flat datafiles, such as CSV 	*/
/*		files, DBF files, and DBL files.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_datafile.c,v 1.28 2010/09/10 00:46:31 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_datafile.c,v $

    $Log: objdrv_datafile.c,v $
    Revision 1.28  2010/09/10 00:46:31  gbeeley
    - (bugfix) Fix a data corruption bug in the flat datafile driver (for
      csv files and such) that could cause cross-contamination between two
      data files.

    Revision 1.27  2010/09/09 01:43:22  gbeeley
    - (feature) stubed out functionality for fixed-field flat files

    Revision 1.26  2008/04/06 20:43:52  gbeeley
    - (bugfix) all three rdbms-style objectsystem drivers had memory leak
      issues relating to pathname structures.  The corrected interface in
      obj.h allows us to fix this.

    Revision 1.25  2008/04/06 09:08:56  gbeeley
    - (bugfix) CSV / flat file driver was corrupting data fields during
      updates under certain conditions.

    Revision 1.24  2008/03/29 02:26:15  gbeeley
    - (change) Correcting various compile time warnings such as signed vs.
      unsigned char.

    Revision 1.23  2008/03/14 18:20:45  gbeeley
    - (bugfix) Due to initial miscounting of rows, inserts into a completely
      empty file were failing
    - (bugfix) Two type mismatch error messages had incorrect parameters

    Revision 1.22  2008/02/17 07:45:58  gbeeley
    - (bugfix) data corruption caused when a deleted row spanned a page
      boundary

    Revision 1.21  2007/09/18 18:08:20  gbeeley
    - (bugfix) incorrect page flush logic was leaving pages unwritten to the
      file under certain circumstances.

    Revision 1.20  2007/07/31 18:07:53  gbeeley
    - (bugfix) something's fishy here.  Not sure why ParseRow was being
      called when a colsobj was being opened.  Sigh...

    Revision 1.19  2007/06/02 16:29:27  gbeeley
    - (bugfix) crash occurred when an update to a CSV file crossed a 4K page
      boundary, and only when the boundary was within the end-of-row padding
      rather than in the data part of the row.

    Revision 1.18  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.17  2007/03/01 21:57:26  gbeeley
    - (bugfix) allow trailing spaces on a row to be used (properly) as space
      for the row to grow instead of being absorbed into the last field in
      the row.

    Revision 1.16  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.15  2004/06/23 21:33:55  mmcgill
    Implemented the ObjInfo interface for all the drivers that are currently
    a part of the project (in the Makefile, in other words). Authors of the
    various drivers might want to check to be sure that I didn't botch any-
    thing, and where applicable see if there's a neat way to keep track of
    whether or not an object actually has subobjects (I did not set this flag
    unless it was immediately obvious how to test for the condition).

    Revision 1.14  2003/11/12 22:21:39  gbeeley
    - addition of delete support to osml, mq, datafile, and ux modules
    - added objDeleteObj() API call which will replace objDelete()
    - stparse now allows strings as well as keywords for object names
    - sanity check - old rpt driver to make sure it isn't in the build

    Revision 1.13  2003/09/02 15:37:13  gbeeley
    - Added enhanced command line interface to test_obj.
    - Enhancements to v3 report writer.
    - Fix for v3 print formatter in prtSetTextStyle().
    - Allow spec pathname to be provided in the openctl (command line) for
      CSV files.
    - Report writer checks for params in the openctl.
    - Local filesystem driver fix for read-only files/directories.
    - Race condition fix in UX printer osdriver
    - Banding problem workaround installed for image output in PCL.
    - OSML objOpen() read vs. read+write fix.

    Revision 1.12  2003/06/03 20:29:12  gbeeley
    Fix to CSV driver due to uninitialized memory causing a segfault when
    opening CSV files from time to time.

    Revision 1.11  2003/05/30 17:39:52  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.10  2003/04/25 05:06:58  gbeeley
    Added insert support to OSML-over-HTTP, and very remedial Trx support
    with the objCommit API method and Commit osdriver method.  CSV datafile
    driver is the only driver supporting it at present.

    Revision 1.9  2003/04/25 04:31:22  gbeeley
    When creating new row with autokey, release the insert semaphore as
    soon as row has been inserted.

    Revision 1.8  2003/04/25 04:09:29  gbeeley
    Adding insert and autokeying support to OSML and to CSV datafile
    driver on a limited basis (in rowidkey mode only, which is the only
    mode currently supported by the csv driver).

    Revision 1.7  2003/03/10 15:41:42  lkehresman
    The CSV objectsystem driver (objdrv_datafile.c) now presents the presentation
    hints to the OSML.  To do this I had to:
      * Move obj_internal_InfToHints() to a global function objInfToHints.  This
        is now located in utility/hints.c and the include is in include/hints.h.
      * Added the presentation hints function to the CSV driver and called it
        datPresentationHints() which returns a valid objPresentationHints object.
      * Modified test_obj.c to fix a crash bug and reformatted the output to be
        a little bit easier to read.
      * Added utility/hints.c to Makefile.in (somebody please check and make sure
        that I did this correctly).  Note that you will have to reconfigure
        centrallix for this change to take effect.

    Revision 1.6  2002/08/10 02:09:45  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

    Revision 1.5  2002/06/19 23:29:34  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.4  2001/10/16 23:53:02  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.3  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.2  2001/09/25 18:04:22  gbeeley
    Fixed quote escaping problem in the CSV driver.

    Revision 1.1.1.1  2001/08/13 18:01:01  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:03  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/*** tuning and constants ***/
#define DAT_CACHE_MAXPAGES	64
#define DAT_CACHE_PAGESIZE	4096
#define DAT_CACHE_KEYSIZE	(sizeof(int)+sizeof(void*))
#define DAT_CACHE_MAXSEARCH	64
#define DAT_CACHE_MAXSEQ	64
#define DAT_NODE_ROWIDPTRS	128
#define DAT_NODE_ROWIDFACTOR	64
#define DAT_ROW_MAXPAGESPAN	2
#define DAT_MAXROWID		((1<<30) - 1)


/*** Structure for storing table-key information ***/
typedef struct
    {
    char		    Table[256];
    char*		    ColBuf;
    int			    ColBufSize;
    int			    ColBufLen;
    char*		    Cols[256];
    unsigned char	    ColIDs[256];
    unsigned char	    ColFlags[256];
    unsigned char	    ColTypes[256];
    unsigned char	    ColKeys[256];
    char*		    ColFmt[256];
    int			    ColLength[256];
    int			    nCols;
    char*		    Keys[8];
    int			    KeyCols[8];
    int			    nKeys;
    pParamObjects	    ObjList;
    pExpression		    RowAnnotExpr;
    char		    Annotation[256];
    }
    DatTableInf, *pDatTableInf;


#define DAT_CF_ALLOWNULL	1
#define DAT_CF_FOUND		2
#define DAT_CF_PRIKEY		4
#define DAT_CF_QUOTED		8
#define DAT_CF_NONQUOTED	16


/*** Structure for row-id-ptr cache ***/
typedef struct
    {
    unsigned int	RowID;
    unsigned int	Offset;
    unsigned int	LastAccess;
    unsigned short	nBits;
    unsigned short	Flags;
    }
    DatRowIDPtr, *pDatRowIDPtr;

#define DAT_RI_F_EMPTY		1


/*** Structure for table node file ***/
typedef struct
    {
    char		DataPath[320];
    char*		EndDataPath;
    char		SpecPath[320];
    char*		EndSpecPath;
    pSnNode		Node;
    int			NodeSerial;
    pDatTableInf 	TableInf;
    int			Type;
    char		Ext[8];
    unsigned char	FieldSep;
    int			Flags;
    int			NewRowPadding;
    pSemaphore		FlushSem;
    DatRowIDPtr		RowIDPtrCache[DAT_NODE_ROWIDPTRS];
    int			RowAccessCnt;
    int			MaxRowID;	/* highest row accessible by rowidptr cache */
    int			RealMaxRowID;	/* highest row accessed */
    DatTableInf		HeaderCols;
    int			nRows;		/* number of rows in table, or -1 if not yet determined */
    pSemaphore		InsertSem;	/* controls inserts at end of file */
    }
    DatNode, *pDatNode;

#define DAT_NODE_T_CSV		1
#define DAT_NODE_T_BCP		2
#define DAT_NODE_T_FIXED	3

#define DAT_NODE_F_HDRROW	1
#define DAT_NODE_F_HDRTITLE	2
#define DAT_NODE_F_ROWIDKEY	4
#define DAT_NODE_F_QUOTEALL	8
#define DAT_NODE_F_DBLQUOTEONLY	16
#define DAT_NODE_F_TWOQUOTEESC	32


/*** Session data indicates which sessions the cached page
 *** is accessible by and how
 ***/
#define DAT_MAXSESS		8	/* LRU discard policy */
#define DAT_SESS_F_READ		1	/* session can read data */
#define DAT_SESS_F_WRITE	2	/* session can write data */

/*** Structure for a single page of data ***/
typedef struct _DP
    {
    struct _DP*		Next;
    struct _DP*		Prev;
    pDatNode		Node;
    unsigned int	PageID;
    int			Flags;
    int			Length;
    unsigned int	AccessOrder;
    handle_t		Sessions[DAT_MAXSESS];
    int			SessFlags[DAT_MAXSESS];
    int			nSess;
    unsigned char	Data[DAT_CACHE_PAGESIZE];
    }
    DatPage, *pDatPage;

#define DAT_CACHE_F_DIRTY	1
#define DAT_CACHE_F_LOCKED	2


/*** Structure used for row information ***/
typedef struct
    {
    pDatPage		Pages[DAT_ROW_MAXPAGESPAN];
    unsigned int	StartPageID;
    unsigned short	nPages;
    unsigned short	Offset;
    unsigned short	Length;
    unsigned short	Flags;
    }
    DatRowInfo, *pDatRowInfo;

#define DAT_R_F_DELETED		1


/*** Structure used by this driver internally for open objects ***/
typedef struct 
    {
    char	TmpFile[64];
    pFile	TmpFD;
    Pathname	Pathname;
    char*	TablePtr;
    char*	TableSubPtr;
    char*	RowColPtr;
    int		Flags;
    int		Type;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    int		Size;
    char	UsrName[16];
    char	GrpName[16];
    int		IntAttr;
    pDatNode	Node;
    pDatTableInf TData;
    int		ColID;
    union
        {
	DateTime	Date;
	MoneyType	Money;
	IntVec		IV;
	StringVec	SV;
	}
	Types;
    int		   RowID;
    pDatRowInfo	   Row;
    unsigned char  RowBuf[(DAT_ROW_MAXPAGESPAN-1)*DAT_CACHE_PAGESIZE];
    int		   RowBufSize;
    unsigned char* ColPtrs[256];
    char  AutoName[256];
    char  SpecName[OBJSYS_MAX_PATH]; /* spec file if specified in openctl params */
    pObject	   DataObj;
    pObject	   SpecObj;
    }
    DatData, *pDatData;

#define DAT_F_ROWPRESENT	1
#define	DAT_F_ROWPARSED		2
#define DAT_F_HOLDINSERTSEM	4

#define DAT_T_TABLE		1
#define DAT_T_ROWSOBJ		2
#define DAT_T_COLSOBJ		3
#define DAT_T_COLUMN		4
#define DAT_T_ROW		5
#define DAT_T_ATTR		6

#define DAT_T_FILESPEC		21
#define DAT_T_FILESPECCOL	22

#define DAT(x) ((pDatData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pDatData	ObjInf;
    int		RowCnt;
    int		RowID;
    pDatTableInf TableInf;
    int		RowsSinceFetch;
    DatRowInfo	Row;
    }
    DatQuery, *pDatQuery;


/*** GLOBALS ***/
struct
    {
    XHashTable	DBNodes;
    int		AccessCnt;
    XHashTable	PagesByID;
    DatPage	PageList;
    int		AllocPages;
    XHashTable	TypesByExt;
    }
    DAT_INF;


/*** Attribute name list ***/
typedef struct
    {
    char* 	Name;
    int		Type;
    } 
    attr_t;

#if 0 /** GRB - commented out these until we actually use em **/
static attr_t dbattr_inf[] =
    {
    {"owner",DATA_T_STRING},
    {"group",DATA_T_STRING},
    {"last_modification",DATA_T_DATETIME},
    {"last_access",DATA_T_DATETIME},
    {"last_change",DATA_T_DATETIME},
    {"permissions",DATA_T_INTEGER},
    {"host",DATA_T_STRING},
    {"service",DATA_T_STRING},
    {"server",DATA_T_STRING},
    {"database",DATA_T_STRING},
    {"max_connections",DATA_T_INTEGER},
    {NULL,-1}
    };

/*** Attributes for tables ***/
static attr_t tabattr_inf[] =
    {
    {"rowcount",DATA_T_INTEGER},
    {"columncount",DATA_T_INTEGER},
    {NULL,-1}
    };
#endif

/*** dat_internal_InsertPage - take a page, optionally unlink it from an existing
 *** chain, and link it at the head of the page cache list.
 ***/
int
dat_internal_InsertPage(pDatPage this)
    {

    	/** First, unlink the page **/
	if (this->Prev) this->Prev->Next = this->Next;
	if (this->Next) this->Next->Prev = this->Prev;

	/** Now, relink the page **/
	this->Prev = &DAT_INF.PageList;
	this->Next = DAT_INF.PageList.Next;
	this->Prev->Next = this;
	this->Next->Prev = this;

    return 0;
    }


/*** dat_internal_FlushPages - flush dirty pages from the page cache.  This 
 *** routine is optimized to flush adjacent pages in sequence, and to search
 *** for such pages up to a certain limit.  The search goes via this->Prev.
 ***/
int
dat_internal_FlushPages(pDatData context, pDatPage this)
    {
    pDatPage seq_pages[DAT_CACHE_MAXSEQ*2];
    pDatPage tmp;
    int n_seq,n_srch;
    int rval = 0;
    unsigned int i,id,seq_tail,seq_head;
    unsigned int min_id, max_id;

    	/** Get the page flush semaphore **/
	syGetSem(this->Node->FlushSem, 1, 0);

	/** Is page still dirty? **/
	if (!(this->Flags & DAT_CACHE_F_DIRTY)) 
	    {
	    syPostSem(this->Node->FlushSem, 1, 0);
	    return 0;
	    }
	this->Flags &= ~DAT_CACHE_F_LOCKED;

    	/** Compile the page list. **/
	memset(seq_pages, 0, sizeof(pDatPage)*DAT_CACHE_MAXSEQ*2);
	seq_pages[DAT_CACHE_MAXSEQ] = this;
	n_seq = 1;
	n_srch = 0;
	seq_tail = DAT_CACHE_MAXSEQ;
	seq_head = DAT_CACHE_MAXSEQ;
	tmp = this->Prev;

	/** min and max id's to scan for.  Note: (~0U) is max unsigned int **/
        min_id = (this->PageID < DAT_CACHE_MAXSEQ)?0:(this->PageID - DAT_CACHE_MAXSEQ + 1);
        max_id = (this->PageID > (~0U) - DAT_CACHE_MAXSEQ)?(~0U):(this->PageID + DAT_CACHE_MAXSEQ - 1);

	/** Do the scan. **/
	while(tmp != &DAT_INF.PageList && n_srch < DAT_CACHE_MAXSEARCH && n_seq < DAT_CACHE_MAXSEQ)
	    {
	    if (!(tmp->Flags & DAT_CACHE_F_LOCKED) && tmp->Node == this->Node && 
	        tmp->PageID >= min_id && tmp->PageID <= max_id)
	        {
		if (tmp->PageID < this->PageID)
		    {
		    id = DAT_CACHE_MAXSEQ - (this->PageID - tmp->PageID);
		    seq_pages[id] = tmp;
		    if (seq_head > id) seq_head = id;
		    }
		else
		    {
		    id = DAT_CACHE_MAXSEQ + (tmp->PageID - this->PageID);
		    seq_pages[id] = tmp;
		    if (seq_tail < id) seq_tail = id;
		    }
		n_seq++;
		}
	    n_srch++;
	    tmp = tmp->Prev;
	    }

	/** Write the pages in order, even if not contiguous **/
	for(i=seq_head;i<=seq_tail;i++)
	    {
	    if (seq_pages[i] && !(seq_pages[i]->Flags & DAT_CACHE_F_LOCKED))
	        {
		seq_pages[i]->Flags |= DAT_CACHE_F_LOCKED;
		seq_pages[i]->Flags &= ~DAT_CACHE_F_DIRTY;
		if (objWrite(context->DataObj, (char*)seq_pages[i]->Data, seq_pages[i]->Length, DAT_CACHE_PAGESIZE*seq_pages[i]->PageID, FD_U_SEEK | FD_U_PACKET) < 0)
		    {
		    rval = -1;
		    }
		seq_pages[i]->Flags &= ~DAT_CACHE_F_LOCKED;
		}
	    }

	/** Release the flush semaphore **/
	syPostSem(this->Node->FlushSem, 1, 0);

    return rval;
    }


/*** dat_internal_GetPage - either alloc a new page, if cache isn't fully alloc'd,
 *** or grab a page from the cache list tail.
 ***/
pDatPage
dat_internal_GetPage(pDatData context)
    {
    pDatPage this,tmp;
    int cnt;

    	/** Just malloc a new one? **/
	if (DAT_INF.AllocPages < DAT_CACHE_MAXPAGES)
	    {
	    this = (pDatPage)nmMalloc(sizeof(DatPage));
	    if (!this) return NULL;
	    DAT_INF.AllocPages++;
	    }
	else
	    {
	    /** Grab one off of the cache tail. **/
	    while(1)
	        {
	        tmp = DAT_INF.PageList.Prev;

		/** Page can't be locked, and it can't be a dirty page from a different data file **/
		cnt = 0;
	        while(tmp->Flags & DAT_CACHE_F_LOCKED || ((tmp->Flags & DAT_CACHE_F_DIRTY) && tmp->Node != context->Node))
		    {
		    tmp=tmp->Prev;
		    cnt++;
		    }
		if (cnt > DAT_CACHE_MAXPAGES / 4)
		    printf("DAT - warning: %d pages were skipped during cache free page scan\n", cnt);
	        if (tmp == &DAT_INF.PageList)
	            {
		    mssError(1,"DAT","Bark!  All pages in page cache are locked or unavailable!");
		    return NULL;
		    }

	        /** Need to flush dirty pages? **/
	        if (tmp->Flags & DAT_CACHE_F_DIRTY)
	            {
		    tmp->Flags |= DAT_CACHE_F_LOCKED;
		    dat_internal_FlushPages(context,tmp);
		    if ((tmp->Flags & DAT_CACHE_F_DIRTY) || (tmp->Flags & DAT_CACHE_F_LOCKED)) continue;
		    }
		this = tmp;
		break;
		}
	    this->Next->Prev = this->Prev;
	    this->Prev->Next = this->Next;

	    /** Remove it from its current index in the hash table. **/
	    xhRemove(&DAT_INF.PagesByID, (void*)&(this->Node));
	    }

	/** Mark it unallocated **/
	this->Node = NULL;
	this->Next = NULL;
	this->Prev = NULL;
	this->PageID = 0;

    return this;
    }


/*** dat_internal_ReadPage - read a data page from the file in question or from
 *** the cache, if it is available, lock the page, and return the page pointer
 *** to the calling function.
 ***/
pDatPage
dat_internal_ReadPage(pDatData context, pDatNode node, int page_id)
    {
    pDatPage this;
    struct { pDatNode node; int page_id; } key;

    	/** Is the page in the cache? **/
	key.node = node;
	key.page_id = page_id;
	this = (pDatPage)xhLookup(&DAT_INF.PagesByID, (void*)&key);
	if (this)
	    {
	    while(this->Flags & DAT_CACHE_F_LOCKED) thYield();
	    this->Flags |= DAT_CACHE_F_LOCKED;
	    dat_internal_InsertPage(this);
	    return this;
	    }

	/** Not in cache.  Get a page and read the data. **/
	this = dat_internal_GetPage(context);
	if (!this) return NULL;
	this->Node = node;
	this->PageID = page_id;
	this->Flags |= DAT_CACHE_F_LOCKED;
	this->Length = objRead(context->DataObj, (char*)this->Data, DAT_CACHE_PAGESIZE, DAT_CACHE_PAGESIZE*page_id, FD_U_SEEK);
	if (this->Length <= 0)
	    {
	    if (this->Prev) this->Prev->Next = this->Next;
	    if (this->Next) this->Next->Prev = this->Prev;
	    nmFree(this,sizeof(DatPage));
	    DAT_INF.AllocPages--;
	    return NULL;
	    }
	xhAdd(&DAT_INF.PagesByID, (void*)&(this->Node), (void*)this);
	dat_internal_InsertPage(this);

    return this;
    }


/*** dat_internal_UnlockPage - unlock a locked page.  If the page has been marked
 *** as dirty by the caller, it may be written immediately or the write may be
 *** delayed.
 ***/
int
dat_internal_UnlockPage(pDatPage this)
    {
    this->Flags &= ~DAT_CACHE_F_LOCKED;
    return 0;
    }


/*** dat_internal_NewPage - create a new page at a given position within the
 *** source datafile.  The page will be locked until a call to UnlockPage is
 *** made.
 ***/
pDatPage
dat_internal_NewPage(pDatData context, pDatNode node, int page_id)
    {
    pDatPage this;

    	/** Grab a page **/
	this = dat_internal_GetPage(context);
	if (!this) return NULL;

	/** Zero the data and return the page **/
	memset(this->Data, 0, DAT_CACHE_PAGESIZE);
	this->Node = node;
	this->PageID = page_id;
	this->Length = 0;
	dat_internal_InsertPage(this);
	this->Flags |= DAT_CACHE_F_LOCKED;

	/** Add to the hash table index **/
	xhAdd(&DAT_INF.PagesByID, (void*)&(this->Node), (void*)this);

    return this;
    }


/*** dat_internal_ReleaseRow - release a row structure and release the lock
 *** held on the page(s).
 ***/
int
dat_internal_ReleaseRow(pDatRowInfo ri)
    {
    int i;

	/** Unlock the pages. **/
	for(i=0;i<ri->nPages;i++) dat_internal_UnlockPage(ri->Pages[i]);

	/** Free the structure **/
	nmFree(ri,sizeof(DatRowInfo));

    return 0;
    }


/*** dat_internal_nBits - count the number of trailing 1 bits in an int number
 ***/
int
dat_internal_nBits(unsigned int n)
    {
    int c=0;

    	while(n&1)
	    {
	    c++;
	    n >>= 1;
	    }
	
    return c;
    }


/*** dat_internal_UpdateRowIDPtrCache - update the information in the rowid cache
 *** based upon desiring a fairly even spread of rowid ptrs across the access 
 *** area.  This weights the rowid ptrs by 1) most recent access, 2) number of
 *** '1' bits in the rowid (for even spread).
 ***/
int
dat_internal_UpdateRowIDPtrCache(pDatNode node, pDatRowInfo ri, int rowid)
    {
    int i;
    int cur_prec,prec,best_prec = 0x7FFFFFFF;
    int best = -1;
    int n_bits;

	/** Calc the prec for this rowid **/
	n_bits = dat_internal_nBits(rowid);
	cur_prec = n_bits*(node->MaxRowID/DAT_NODE_ROWIDFACTOR);

	/** Update access counter **/
	if (rowid > node->MaxRowID) node->MaxRowID = rowid;
	if (rowid > node->RealMaxRowID) node->RealMaxRowID = rowid;
	node->RowAccessCnt++;

    	/** Scan the row id ptrs **/
	for(i=0;i<DAT_NODE_ROWIDPTRS;i++)
	    {
	    if (!(node->RowIDPtrCache[i].Flags & DAT_RI_F_EMPTY) && node->RowIDPtrCache[i].RowID == rowid)
		return 0;
	    if (node->RowIDPtrCache[i].Flags & DAT_RI_F_EMPTY)
	        {
		best = i;
		break;
		}
	    prec = node->RowIDPtrCache[i].nBits*(node->MaxRowID/DAT_NODE_ROWIDFACTOR)
	    		- (node->RowAccessCnt - node->RowIDPtrCache[i].LastAccess);
	    if (cur_prec > prec && prec < best_prec)
	        {
		best = i;
		best_prec = prec;
		}
	    }
	if (best == -1) return 0;

	/** Update the row id ptrs **/
	node->RowIDPtrCache[best].RowID = rowid;
	node->RowIDPtrCache[best].Offset = ri->StartPageID*DAT_CACHE_PAGESIZE + ri->Offset;
	node->RowIDPtrCache[best].nBits = n_bits;
	node->RowIDPtrCache[best].LastAccess = node->RowAccessCnt;
	node->RowIDPtrCache[best].Flags = 0;

    return 0;
    }


/*** dat_internal_GetRow - obtains a DatRowInfo structure containing locked pages
 *** for a given row id within a given node.  Returns NULL if the row does not
 *** exist.
 ***
 *** The terminating newline (\n) is NOT included in the RowInfo length returned,
 *** however, the page containing that terminating newline WILL be included in
 *** the page list for the row *unless* the row is at the end of the file and
 *** the file has no newline at the end.
 ***
 *** If a row past the end of the file is requested, this routine will still
 *** catalog the last row in the file so that future searches are more
 *** efficient and so that the # of rows in the file can be more easily
 *** determined.
 ***/
pDatRowInfo
dat_internal_GetRow(pDatData context, pDatNode node, int rowid)
    {
    int scan_rowid;
    pDatRowInfo di;
    int closest = -1;
    unsigned int closest_dist = 0xFFFFFFFF;
    unsigned int dist;
    unsigned char* ptr;
    int closest_rowid;
    int closest_offset;
    int i;
    pDatPage pg;
    int cur_page, cur_offset, cur_row;
    int prevrow_page, prevrow_offset, prevrow;
    int found, found_nl;
    int len;
    unsigned char* nlptr;

	/** First row is header? **/
	if (node->Flags & DAT_NODE_F_HDRROW) rowid++;

    	/** Sanity check **/
	if (rowid < 0)
	    {
	    mssError(1,"DAT","Bark!  Attempt to retrieve negative RowID!");
	    return NULL;
	    }
	scan_rowid = rowid;

    	/** Scan the node's rowid cache to find a close row or even a match **/
	for(i=0;i<DAT_NODE_ROWIDPTRS;i++)
	    {
	    if (!(node->RowIDPtrCache[i].Flags & DAT_RI_F_EMPTY))
	         {
		 dist = abs(scan_rowid - node->RowIDPtrCache[i].RowID);
		 if (closest == -1 || dist < closest_dist)
		     {
		     closest = i;
		     closest_dist = dist;
		     }
		 }
	    }

	/** Take into account that we can scan from the beginning of the file. **/
	if (closest == -1 || closest_dist > (scan_rowid - 0))
	    {
	    closest_rowid = 0;
	    closest_offset = 0;
	    }
	else
	    {
	    /** Ok, starting from somewhere in the middle. **/
	    closest_rowid = node->RowIDPtrCache[closest].RowID;
	    closest_offset = node->RowIDPtrCache[closest].Offset;
	    node->RowIDPtrCache[closest].LastAccess = node->RowAccessCnt++;
	    }

	/** Scan 'til we find the page with the row. **/
	cur_page = closest_offset / DAT_CACHE_PAGESIZE;
	cur_offset = closest_offset % DAT_CACHE_PAGESIZE;
	cur_row = closest_rowid;
	prevrow = cur_row;
	prevrow_page = cur_page;
	prevrow_offset = cur_offset;
	pg = NULL;

	/** Scan forwards or backwards?  If back, do it then move forward one char... **/
	if (cur_row > scan_rowid)
	    {
	    /** Scan backwards **/
	    /** This will NEVER happen when row 0 is requested. **/
	    while(1)
	        {
		/** Fetch the page **/
		if (!pg) pg = dat_internal_ReadPage(context, node, cur_page);
		if (!pg) return NULL;
		ptr = pg->Data + cur_offset;
		found = 0;

		/** Scan the page **/
		while(1)
		    {
		    /** Look at previous char. **/
		    ptr--;
	            cur_offset--;
		    if (cur_offset < 0) break;

		    /** Found end of a row? **/
		    if (*ptr == '\n' || *ptr == 0x01)
		        {
			cur_row--;
			}

		    /** Found end of the row before row in question? **/
		    if (cur_row == scan_rowid-1)
		        {
			found = 1;
			break;
			}
		    }
		if (found) break;

		/** Unlock the retrieved page and move to previous one. **/
		dat_internal_UnlockPage(pg);
		pg = NULL;
		cur_page--;
		cur_offset = DAT_CACHE_PAGESIZE;
		if (cur_page < 0) 
		    {
	    	    mssError(1,"DAT","Bark!  Arrived at negative page with rowid %d!",cur_row);
		    return NULL;
		    }
		}
	    }

	/** Scan forward to find the row **/
	found_nl = 0;
	while(1)
	    {
	    /** Grab the page. **/
	    if (!pg) pg = dat_internal_ReadPage(context, node, cur_page);
	    if (!pg) 
		{
		/** End of data.  Highest row is end of prev page **/
		if (cur_page == 0) return NULL;
		cur_page = prevrow_page;
		cur_row = prevrow;
		cur_offset = prevrow_offset;
		scan_rowid = cur_row;
		pg = dat_internal_ReadPage(context, node, cur_page);
		if (!pg) return NULL;
		}
	    ptr = pg->Data + cur_offset;

	    /** Scan the page. **/
	    found = 0;
	    while(cur_offset < pg->Length)
	        {
		/** Found the row if cur_row set. **/
	        if (cur_row == scan_rowid) 
		    {
		    found = 1;
		    break;
		    }

		/** Advance cur row if we found a line terminator **/
	        if (*ptr == '\n' || *ptr == 0x01)
		    {
		    cur_row++;
		    found_nl = 1;
		    }
		else
		    {
		    if (found_nl)
			{
			prevrow = cur_row;
			prevrow_page = cur_page;
			prevrow_offset = cur_offset;
			found_nl = 0;
			}
		    nlptr = memchr(ptr, '\n', pg->Length - cur_offset);
		    if (nlptr && !memchr(ptr, 0x01, nlptr - ptr))
		        {
		        cur_offset += (nlptr - ptr);
		        ptr = nlptr;
			cur_row++;
			found_nl = 1;
			}
		    }

		/** Advance to next character **/
		ptr++;
	        cur_offset++;
		}
	    if (found) break;

	    /** Unlock the retrieved page and move to next one. **/
	    dat_internal_UnlockPage(pg);
	    pg = NULL;
	    cur_page++;
	    cur_offset = 0;
	    }

	/** Ok, got the page and location. **/
	di = (pDatRowInfo)nmMalloc(sizeof(DatRowInfo));
	if (!di)
	    {
	    dat_internal_UnlockPage(pg);
	    return NULL;
	    }
	di->Pages[0] = pg;
	di->StartPageID = cur_page;
	di->Offset = cur_offset;
	di->nPages = 1;
	di->Flags = 0;
	if (*ptr == '#') di->Flags |= DAT_R_F_DELETED;

	/** Now see how long the row is, in case we need another page. **/
	len = 0;
	ptr = di->Pages[0]->Data + di->Offset;
	while(1)
	    {
	    if (*ptr == '\n' || *ptr == 1) break;
	    ptr++;
	    cur_offset++;
	    len++;
	    if (cur_offset >= di->Pages[di->nPages-1]->Length)
	        {
		if (di->nPages == DAT_ROW_MAXPAGESPAN)
		    {
		    mssError(1,"DAT","File '%s' row #%d exceeds %d pagespan limit",node->DataPath, scan_rowid, DAT_ROW_MAXPAGESPAN);
		    dat_internal_ReleaseRow(di);
		    return NULL;
		    }
		di->Pages[di->nPages] = dat_internal_ReadPage(context, node, di->StartPageID+di->nPages);
		if (!di->Pages[di->nPages]) break;
		cur_offset=0;
		ptr = di->Pages[di->nPages]->Data;
		di->nPages++;
		}
	    }
	di->Length = len;

	/** Row is deleted? **/
	if (*ptr == 1) di->Flags |= DAT_R_F_DELETED;

	/** Empty row? **/
	if (di->Length == 0)
	    {
	    dat_internal_ReleaseRow(di);
	    return NULL;
	    }

	/** Possibly update the rowid-ptr cache **/
	dat_internal_UpdateRowIDPtrCache(node,di,scan_rowid);

	/** Needed row past end of file? **/
	if (scan_rowid != rowid)
	    {
	    dat_internal_ReleaseRow(di);
	    return NULL;
	    }

    return di;
    }


/*** dat_csv_ParseRow - generate pointers to the various data items within a
 *** row of data, and parse out the integer/money/etc values.
 ***/
int
dat_csv_ParseRow(pDatData inf, pDatTableInf td)
    {
    int i,field,len,cur_page,cb_len;
    unsigned char* ptr;
    unsigned char quot;
    unsigned char is_start;
    unsigned char is_end;
    unsigned char conv_buf[64];
    unsigned char is_escaped;
    unsigned char* field_ptr;
    int itmp;
    double dtmp;
    DateTime dt;
    MoneyType m;

    	/** Set all columns to NULL first **/
	for(i=0;i<td->nCols;i++) inf->ColPtrs[i] = NULL;

    	/** Scan fields one at a time... **/
	inf->RowBufSize = 0;
	ptr = inf->Row->Pages[0]->Data + inf->Row->Offset - 1;
	len = -1;
	cur_page = 0;
	is_start = 1;
	is_end = 0;
	quot = 0;
	cb_len = 0;
	is_escaped = 0;
	field_ptr = inf->RowBuf;
	field = 0;
	while(1)
	    {
	    /** Advance to the next character **/
	    ptr++;
	    len++;
	    if (len > inf->Row->Length) break;
	    if ((ptr - inf->Row->Pages[cur_page]->Data) >= inf->Row->Pages[cur_page]->Length)
	        {
		cur_page++;
		if (cur_page >= inf->Row->nPages) break;
		ptr = inf->Row->Pages[cur_page]->Data;
		}

	    /** Process the current character: skip whitespace **/
	    if ((is_start || is_end) && (*ptr == ' ' || (*ptr == '\t' && inf->Node->FieldSep != '\t') || *ptr == 1 || *ptr == 0)) continue;

	    /** End of field, but field already handled? **/
	    if (is_end && (*ptr == inf->Node->FieldSep || *ptr == 1 || *ptr == '\n' || *ptr == '\r'))
	        {
		is_end=0;
		is_start=1;
		continue;
		}

	    /** Escape character? **/
	    if (*ptr == '\\') 
	        {
		is_escaped = 1;
		continue;
		}

	    /** Check for begin-quote mark. **/
	    if (is_start && (*ptr == '\'' || *ptr == '"') && !is_escaped)
	        {
		is_start = 0;
		quot = *ptr;
		continue;
		}

	    /** End-of-line inside quotes? **/
	    if (quot && (*ptr == '\r' || *ptr == '\n')) break;

	    /** Time to finish up a field? **/
	    if (!is_escaped && ((quot && *ptr == quot) || (!quot && *ptr == inf->Node->FieldSep) || *ptr == '\n' || *ptr == '\r'))
	        {
		/** Process based on the data type **/
		conv_buf[cb_len] = '\0';
		switch(td->ColTypes[field])
		    {
		    case DATA_T_STRING:
			/** Trim trailing spaces from the last field **/
			if (field == td->nCols-1)
			    {
			    while(inf->RowBuf + inf->RowBufSize > field_ptr && inf->RowBuf[inf->RowBufSize-1] == ' ')
				inf->RowBufSize--;
			    }
		        inf->RowBuf[inf->RowBufSize++] = '\0';
			break;

		    case DATA_T_INTEGER:
		        itmp = objDataToInteger(DATA_T_STRING, (void*)conv_buf, td->ColFmt[field]);
			memcpy(inf->RowBuf + inf->RowBufSize, &itmp, 4);
			inf->RowBufSize += 4;
			break;

		    case DATA_T_DATETIME:
		        objDataToDateTime(DATA_T_STRING, (void*)conv_buf, &dt, td->ColFmt[field]);
			memcpy(inf->RowBuf + inf->RowBufSize, &dt, sizeof(DateTime));
			inf->RowBufSize += sizeof(DateTime);
			break;

		    case DATA_T_DOUBLE:
		        dtmp = objDataToDouble(DATA_T_STRING, (void*)conv_buf);
			memcpy(inf->RowBuf + inf->RowBufSize, &dtmp, 8);
			inf->RowBufSize += 8;
			break;

		    case DATA_T_MONEY:
		        objDataToMoney(DATA_T_STRING, (void*)conv_buf, &m);
			memcpy(inf->RowBuf + inf->RowBufSize, &m, sizeof(MoneyType));
			inf->RowBufSize += sizeof(MoneyType);
			break;
		    }

		/** Set up the field ptr and advance to next one. **/
		if (td->ColTypes[field] != DATA_T_STRING && cb_len == 0)
		    inf->ColPtrs[field] = NULL;
		else
		    inf->ColPtrs[field] = field_ptr;
		field_ptr = inf->RowBuf + inf->RowBufSize;
		field++;
		if (quot && *ptr == quot) is_end = 1;
		quot = 0;
		if (!is_end) is_start = 1;
		cb_len = 0;
		is_escaped = 0;
		if (field >= td->nCols) break;
		continue;
		}

	    /** Ok, if in a string, copy directly to row buffer. **/
	    is_start=0;
	    if (td->ColTypes[field] == DATA_T_STRING)
	        {
		inf->RowBuf[inf->RowBufSize++] = *ptr;
		}
	    else
	        {
		/** Otherwise, to conversion buffer **/
		if (cb_len < 63) conv_buf[cb_len++] = *ptr;
		}
	    is_escaped = 0;
	    }

    return 0;
    }


/*** dat_internal_KeyToFilename - converts the primary key contents of the current
 *** query record back to the filename.  This filename consists of the primary key
 *** fields separated by | character(s).
 ***/
char*
dat_internal_KeyToFilename(pDatTableInf tdata, pDatData inf)
    {
    static char fbuf[80];
    char* keyptrs[8];
    int i,col=0;
    char* ptr;

    	/** Row id is filename? **/
	if (inf->Node->Flags & DAT_NODE_F_ROWIDKEY)
	    {
	    sprintf(fbuf,"%d",inf->RowID);
	    return fbuf;
	    }

    	/** Get pointers to the key data. **/
	ptr = fbuf;
	for(i=0;i<tdata->nKeys;i++)
	    {
	    if (i>0) *(ptr++)='|';
	    col = 0;
	    keyptrs[i] = NULL;
	    switch(tdata->ColTypes[tdata->KeyCols[i]])
	        {
		case DATA_T_INTEGER: /** INT **/
		    memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 4);
		    sprintf(ptr,"%d",col);
		    break;
		case DATA_T_STRING:
		    strcpy(ptr, (char*)inf->ColPtrs[tdata->KeyCols[i]]);
		    break;
		}
	    ptr += strlen(ptr);
	    }

    return fbuf;
    }


/*** dat_internal_FilenameToKey - converts a primary key filename to a where
 *** clause directing access for that key, for a given table within a given 
 *** database node.  The returned name is stored in a static storage area and
 *** must be copied from that place before allowing a context switch....
 ***/
char*
dat_internal_FilenameToKey(pDatNode node, char* table, char* filename)
    {
    static char wbuf[256];
    /* GRB uncomment these vars when we code the function */
    /*static char fbuf[80];
    char* sbuf;
    char* wptr;
    pDatTableInf key;
    char* ptr;
    int i;
    int is_new=0;*/

    return wbuf;
    }


/*** dat_internal_DetermineType - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
dat_internal_DetermineType(pObject obj, pDatData inf)
    {
    int i;

	/** Determine object type (depth) and get pointers set up **/
	obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
	for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = 0;
	inf->TablePtr = NULL;
	inf->TableSubPtr = NULL;
	inf->RowColPtr = NULL;

	/** Is this a spec file or data file? **/
	inf->TablePtr = inf->Pathname.Elements[obj->SubPtr-1];
	if (strlen(inf->TablePtr) <= 5 || strcmp(".spec",inf->TablePtr+strlen(inf->TablePtr)-5))
	    {
	    /** Set up pointers based on number of elements past the node object **/
	    inf->Type = DAT_T_TABLE;
	    obj->SubCnt = 1;
	    if (inf->Pathname.nElements - 1 >= obj->SubPtr)
	        {
		obj->SubCnt = 2;
	        inf->TableSubPtr = inf->Pathname.Elements[obj->SubPtr];
	        if (!strncmp(inf->TableSubPtr,"rows",4)) inf->Type = DAT_T_ROWSOBJ;
	        else if (!strncmp(inf->TableSubPtr,"columns",7)) inf->Type = DAT_T_COLSOBJ;
		else 
		    {
		    mssError(1,"DAT","Only two child objects of a table are 'rows' and 'columns'");
		    if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
		    inf->Pathname.OpenCtlBuf = NULL;
		    return -1;
		    }
	        }
	    if (inf->Pathname.nElements - 2 >= obj->SubPtr)
	        {
		obj->SubCnt = 3;
	    	inf->RowColPtr = inf->Pathname.Elements[obj->SubPtr+1];
	    	if (inf->Type == DAT_T_ROWSOBJ) inf->Type = DAT_T_ROW;
	    	else if (inf->Type == DAT_T_COLSOBJ) inf->Type = DAT_T_COLUMN;
		}
	    }
	else
	    {
	    /** Spec file -- two options, file itself or column spec **/
	    if (inf->Pathname.nElements == obj->SubPtr)
	        {
		inf->Type = DAT_T_FILESPEC;
		obj->SubCnt = 1;
		}
	    else if (inf->Pathname.nElements == obj->SubPtr+1)
	        {
		inf->Type = DAT_T_FILESPECCOL;
		inf->RowColPtr = inf->Pathname.Elements[obj->SubPtr];
		obj->SubCnt = 2;
		}
	    else
	        {
		mssError(1,"DAT",".spec file does not have that many levels of child objects");
		if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
		inf->Pathname.OpenCtlBuf = NULL;
		return -1;
		}
	    }

    return 0;
    }


/*** dat_internal_SortCols - sort the columns in a tableinf structure. 
 ***/
int
dat_internal_SortCols(pDatTableInf tdata)
    {
    int i,j;
    char* ctmp;
    int itmp;

        /** Ok, loaded the columns.  Now sort 'em **/
	for(i=0;i<tdata->nCols;i++)
	    {
	    for(j=i+1;j<tdata->nCols;j++)
		{
		if (tdata->ColIDs[i] > tdata->ColIDs[j])
		    {
		    /** Sort name **/
		    ctmp = tdata->Cols[i];
		    tdata->Cols[i] = tdata->Cols[j];
		    tdata->Cols[j] = ctmp;

		    /** Sort ID **/
		    itmp = tdata->ColIDs[i];
		    tdata->ColIDs[i] = tdata->ColIDs[j];
		    tdata->ColIDs[j] = itmp;

		    /** Sort Keys **/
		    itmp = tdata->ColKeys[i];
		    tdata->ColKeys[i] = tdata->ColKeys[j];
		    tdata->ColKeys[j] = itmp;

		    /** Sort Types **/
		    itmp = tdata->ColTypes[i];
		    tdata->ColTypes[i] = tdata->ColTypes[j];
		    tdata->ColTypes[j] = itmp;

		    /** Sort flags **/
		    itmp = tdata->ColFlags[i];
		    tdata->ColFlags[i] = tdata->ColFlags[j];
		    tdata->ColFlags[j] = itmp;

		    /** Sort formats **/
		    ctmp = tdata->ColFmt[i];
		    tdata->ColFmt[i] = tdata->ColFmt[j];
		    tdata->ColFmt[j] = ctmp;
		    }
		}
	    }

    return 0;
    }


/*** dat_fixed_OpenNode - handle fixed-field data files
 ***/
int
dat_fixed_OpenNode(pDatNode dn)
    {

	/** No field separator **/
	dn->FieldSep = '\0';

    return 0;
    }


/*** dat_csv_OpenNode - CSV file specific node open functionality.
 ***/
int
dat_csv_OpenNode(pDatNode dn)
    {

    	/** Set field separator **/
	dn->FieldSep = ',';

    return 0;
    }


/*** dat_bcp_OpenNode - BCP file specific node open operations.
 ***/
int
dat_bcp_OpenNode(pDatNode dn)
    {
    char* ptr = NULL;

	/** Determine field separator **/
	stAttrValue(stLookup(dn->Node->Data, "fieldsep"),NULL, &ptr, 0);
	if (ptr)
	    dn->FieldSep = *ptr;
	else
	    dn->FieldSep = '\t';

    return 0;
    }


/*** dat_internal_OpenNode - access the node object and parse the spec file,
 *** unless already cached.
 ***/
pDatNode
dat_internal_OpenNode(pDatData context, pObject obj, char* filename, int mode, int is_toplevel, int create_mask)
    {
    pDatNode dn;
    char nodefile[OBJSYS_MAX_PATH];
    char* dot_pos = NULL;
    char* slash_pos = NULL;
    int is_datafile = 0;
    char* ptr;
    int i,n;
    pStructInf col_inf;
    int use_mode;
    int new_node = 0;
    pDatTableInf tdata = NULL;

	/** Determine the datafile and specfile names **/
	strcpy(nodefile,filename);
	if (strlen(filename) <= 5 || strcmp(".spec",filename+strlen(filename)-5))
	    {
	    if (context->SpecName[0])
		{
		/** Specfile specified in open ctl **/
		strtcpy(nodefile, context->SpecName, sizeof(nodefile));
		}
	    else
		{
		/** Determine specfile name from object name **/
		slash_pos = strrchr(nodefile,'/');
		dot_pos = strrchr(nodefile,'.');
		if (!dot_pos || (slash_pos > dot_pos && slash_pos != NULL))
		    strcat(nodefile,".spec");
		else
		    strcpy(dot_pos,".spec");
		}
	    is_datafile = 1;
	    }

	/** Lookup the node in the hash table.  If not found, mk a new one **/
	dn = (pDatNode)xhLookup(&DAT_INF.DBNodes, (void*)nodefile);
	if (!dn)
	    {
	    /** Not found... Allocate a new one **/
	    dn = (pDatNode)nmMalloc(sizeof(DatNode));
	    new_node = 1;
	    if (!dn) return NULL;
	    strcpy(dn->SpecPath, nodefile);
	    dn->EndSpecPath = strchr(dn->SpecPath,'\0');
	    strcpy(dn->EndSpecPath, "?ls__type=application%2foctet-stream");
	    dn->FlushSem = syCreateSem(1,0);
	    dn->Flags = 0;
	    dn->NodeSerial = -1;
	    dn->TableInf = NULL;
	    dn->MaxRowID = DAT_NODE_ROWIDPTRS*2 - 1;
	    dn->RealMaxRowID = -1;
	    dn->RowAccessCnt = 0;
	    dn->HeaderCols.nCols = 0;
	    dn->HeaderCols.ColBuf = NULL;
	    dn->nRows = -1;
	    dn->InsertSem = syCreateSem(1,0);
	    for(i=0;i<DAT_NODE_ROWIDPTRS;i++) dn->RowIDPtrCache[i].Flags = DAT_RI_F_EMPTY;

	    /** Find the SnNode structure for the structure file data **/
	    if (is_datafile)
	        {
		/** We do 'nodefile+1' to eliminate the pathinfo standard './' **/
		context->SpecObj = objOpen(obj->Session, dn->SpecPath+1, O_RDWR, 0600, "system/filespec");
		if (!context->SpecObj)
		    {
		    mssError(0,"DAT","Could not access .spec file for datafile");
		    nmFree(dn,sizeof(DatNode));
		    return NULL;
		    }
		objUnmanageObject(obj->Session, context->SpecObj);
		context->DataObj = obj->Prev;
		objLinkTo(context->DataObj);
		}
	    else
	        {
		context->SpecObj = obj->Prev;
		objLinkTo(context->SpecObj);
		context->DataObj = NULL;
		}
	    dn->Node = snReadNode(context->SpecObj);
	    if (!dn->Node)
	        {
		mssError(0,"DAT","Could not process .spec file");
		nmFree(dn,sizeof(DatNode));
		return NULL;
		}

	    /** Determine type of the datafile **/
	    ptr = NULL;
	    stAttrValue(stLookup(dn->Node->Data,"filetype"),NULL,&ptr,0);
	    if (!ptr && (!is_datafile || !dot_pos || (slash_pos && slash_pos > dot_pos)))
	        {
		mssError(1,"DAT","Could not determine filetype for datafile");
		nmFree(dn,sizeof(DatNode));
		return NULL;
		}
	    else if (!ptr && is_datafile)
	        {
		ptr = dot_pos+1;
		}
	    if (strlen(ptr) > 7)
	        {
		mssError(1,"DAT","Extension type '%s' too long",ptr);
		nmFree(dn,sizeof(DatNode));
		return NULL;
		}
	    for(i=0;i<=strlen(ptr);i++) dn->Ext[i] = toupper(ptr[i]);
	    dn->Type = (intptr_t)xhLookup(&DAT_INF.TypesByExt, (void*)(dn->Ext));
	    if (!dn->Type)
	        {
		mssError(1,"DAT","Unknown datafile type '%s'",ptr);
		nmFree(dn,sizeof(DatNode));
		return NULL;
		}

	    /** Filetype-specific load stuff **/
	    switch(dn->Type)
	        {
		case DAT_NODE_T_CSV:	dat_csv_OpenNode(dn); break;
		case DAT_NODE_T_BCP:	dat_bcp_OpenNode(dn); break;
		case DAT_NODE_T_FIXED:	dat_fixed_OpenNode(dn); break;
		}

	    /** Need to set the datafile path as well as .spec file path **/
	    if (is_datafile)
	        {
		strcpy(dn->DataPath, filename);
		}
	    else
	        {
		strcpy(dn->DataPath, dn->SpecPath);
		sprintf(strstr(dn->DataPath,".spec"),".%s",ptr);
		}
	    dn->EndDataPath = strchr(dn->DataPath, '\0');
	    strcpy(dn->EndDataPath, "?ls__type=application%2foctet-stream");

	    /** Attempt to open the datafile **/
	    if (is_toplevel) use_mode = mode; else use_mode = O_RDWR;
	    /* dn->DataFD = fdOpen(dn->DataPath,use_mode,create_mask);
	    if (!dn->DataFD)
	        {
		mssErrorErrno(1,"DAT","Could not open datafile '%s'",dn->DataPath);
		nmFree(dn,sizeof(DatNode));
		return NULL;
		}*/
	    }
	else
	    {
	    /** If datafile, but was not opened last time, link to it now. **/
	    if (is_datafile && !context->DataObj)
	        {
		context->DataObj = obj->Prev;
		objLinkTo(context->DataObj);
		}
	    }

	/** Need to reload some of the changeable data information? **/
	if (snGetSerial(dn->Node) != dn->NodeSerial)
	    {
	    if (dn->NodeSerial != -1)
	        {
		strcpy(nodefile, obj_internal_PathPart(context->DataObj->Pathname, 0,0));
		/*objClose(dn->DataObj);*/
	        dn->NodeSerial = snGetSerial(dn->Node);
		if (is_datafile)
		    {
		    /*objLinkTo(obj);
		    dn->DataObj = obj;*/
		    }
		else
		    {
		    /** We can close the obj right away because the invocation of this routine **/
		    /** during the open() call will objLinkTo() the datafile object. **/
		    /*dn->DataObj = NULL;
		    dn->DataObj = objOpen(obj->Session, nodefile, O_RDWR, 0600, "system/datafile");
		    objClose(dn->DataObj);*/
		    }
	        if (!context->DataObj)
	            {
		    /*mssErrorErrno(1,"DAT","Could not re-open modified datafile '%s'",dn->DataPath);
		    nmFree(dn,sizeof(DatNode));
		    return NULL;*/
		    }
		}
	    dn->NodeSerial = snGetSerial(dn->Node);

	    /** Need to allocate the tableinf structure? **/
	    if (!dn->TableInf)
	        {
		dn->TableInf = (pDatTableInf)nmMalloc(sizeof(DatTableInf));
		memset(dn->TableInf, 0, sizeof(DatTableInf));
		dn->TableInf->ColBuf = NULL;
		dn->HeaderCols.ColBuf = NULL;
		}
	    if (dn->TableInf->ColBuf) nmSysFree(dn->TableInf->ColBuf);
	    dn->TableInf->ColBuf = (char*)nmSysMalloc(1024);
	    dn->TableInf->ColBufSize = 1024;
	    dn->TableInf->ColBufLen = 0;
	    if (dn->HeaderCols.ColBuf) nmSysFree(dn->HeaderCols.ColBuf);
	    dn->HeaderCols.ColBuf = (char*)nmSysMalloc(1024);
	    dn->HeaderCols.ColBufSize = 1024;
	    dn->HeaderCols.ColBufLen = 0;

	    /** Determine various flag information **/
	    ptr=NULL;
	    stAttrValue(stLookup(dn->Node->Data,"header_row"),NULL,&ptr,0);
	    if (ptr && !strcmp(ptr,"yes")) dn->Flags |= DAT_NODE_F_HDRROW;
	    ptr=NULL;
	    stAttrValue(stLookup(dn->Node->Data,"header_has_titles"),NULL,&ptr,0);
	    if (ptr && !strcmp(ptr,"yes")) dn->Flags |= DAT_NODE_F_HDRTITLE;
	    ptr=NULL;
	    stAttrValue(stLookup(dn->Node->Data,"key_is_rowid"),NULL,&ptr,0);
	    if (ptr && !strcmp(ptr,"yes")) dn->Flags |= DAT_NODE_F_ROWIDKEY;
	    n = -1;
	    stAttrValue(stLookup(dn->Node->Data,"new_row_padding"),&n,NULL,0);
	    if (n > DAT_CACHE_PAGESIZE) n = DAT_CACHE_PAGESIZE;
	    if (n >= 0)
		dn->NewRowPadding = n;
	    else
		dn->NewRowPadding = 0;

	    /** Load other information, such as annotation info **/
	    ptr=NULL;
	    stAttrValue(stLookup(dn->Node->Data,"annotation"),NULL,&ptr,0);
	    if (ptr)
	        {
	        strtcpy(dn->TableInf->Annotation, ptr, sizeof(dn->TableInf->Annotation));
		}
	    else
	        {
		dn->TableInf->Annotation[0] = 0;
		}
	    ptr=NULL;
	    stAttrValue(stLookup(dn->Node->Data,"row_annot_exp"),NULL,&ptr,0);
	    if (ptr)
	        {
		if (!dn->TableInf->ObjList)
		    {
		    dn->TableInf->ObjList = expCreateParamList();
		    expAddParamToList(dn->TableInf->ObjList, "this", NULL, 0);
		    }
		dn->TableInf->RowAnnotExpr = expCompileExpression(ptr, dn->TableInf->ObjList, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		}
	    else
	        {
		dn->TableInf->RowAnnotExpr = NULL;
		}


	    /** Load the column and key information, including header cols. **/
	    dn->HeaderCols.nCols = 0;
	    dn->TableInf->nCols = 0;
	    dn->TableInf->nKeys = 0;
	    for(i=0;i<dn->Node->Data->nSubInf;i++)
	        {
		col_inf = dn->Node->Data->SubInf[i];
		if (stStructType(col_inf) == ST_T_SUBGROUP)
		    {
		    /** Column or header column? **/
		    if (!strcmp(col_inf->UsrType, "filespec/column")) tdata = dn->TableInf;
		    else if (!strcmp(col_inf->UsrType, "filespec/hdrcolumn")) tdata = &(dn->HeaderCols);

		    /** Enter the name... **/
		    n = strlen(col_inf->Name)+1;
		    if (tdata->ColBufSize <= tdata->ColBufLen + n)
		        {
			/*ptr = (char*)nmSysRealloc(tdata->ColBuf, tdata->ColBufSize+1024);
			tdata->ColBuf = ptr;
			tdata->ColBufSize += 1024;*/
			break;
			}
		    memcpy(tdata->ColBuf + tdata->ColBufLen, col_inf->Name, n);
		    tdata->Cols[tdata->nCols] = tdata->ColBuf + tdata->ColBufLen;
		    tdata->ColBufLen += n;

		    /** Retrieve column type. **/
		    ptr=NULL;
		    stAttrValue(stLookup(col_inf,"type"),NULL,&ptr,0);
		    if (!ptr)
		        {
			mssError(1,"DAT","Type not specified for column '%s'",col_inf->Name);
			return NULL;
			}
		    if (!strcmp(ptr,"integer")) tdata->ColTypes[tdata->nCols] = DATA_T_INTEGER;
		    else if (!strcmp(ptr,"string")) tdata->ColTypes[tdata->nCols] = DATA_T_STRING;
		    else if (!strcmp(ptr,"datetime")) tdata->ColTypes[tdata->nCols] = DATA_T_DATETIME;
		    else if (!strcmp(ptr,"double")) tdata->ColTypes[tdata->nCols] = DATA_T_DOUBLE;
		    else if (!strcmp(ptr,"money")) tdata->ColTypes[tdata->nCols] = DATA_T_MONEY;
		    else
		        {
			mssError(1,"DAT","Invalid type specified for column '%s'",col_inf->Name);
			return NULL;
			}

		    /** Column flags **/
		    tdata->ColFlags[tdata->nCols] = DAT_CF_ALLOWNULL;
		    if (stAttrValue(stLookup(col_inf,"quoted"),NULL,&ptr,0) >= 0)
		        {
			if (!strcasecmp(ptr,"yes")) tdata->ColFlags[tdata->nCols] |= DAT_CF_QUOTED;
			if (!strcasecmp(ptr,"no")) tdata->ColFlags[tdata->nCols] |= DAT_CF_NONQUOTED;
			}

		    /** Column physical length **/
		    if (stAttrValue(stLookup(col_inf,"length"),&n,NULL,0) >= 0)
			tdata->ColLength[tdata->nCols] = n;
		    else
			tdata->ColLength[tdata->nCols] = 0;

		    /** Now for column id **/
		    if (stAttrValue(stLookup(col_inf,"id"),&n,NULL,0) >= 0)
		        tdata->ColIDs[tdata->nCols] = n;
		    else
		        tdata->ColIDs[tdata->nCols] = 0;

		    /** Is this a key? **/
		    if (stAttrValue(stLookup(col_inf,"key"),NULL,&ptr,0) >= 0 && !strcmp(ptr,"yes"))
		        {
			tdata->ColKeys[tdata->nCols] = 0xFF;
			}
		    else
		        {
			tdata->ColKeys[tdata->nCols] = 0x00;
			}

		    /** Check format. **/
		    tdata->ColFmt[tdata->nCols] = NULL;
		    if (stAttrValue(stLookup(col_inf,"format"),NULL,&ptr,0) >= 0)
			{
			n = strlen(ptr)+1;
			if (tdata->ColBufSize <= tdata->ColBufLen + n)
			    {
			    /*ptr = (char*)nmSysRealloc(tdata->ColBuf, tdata->ColBufSize+1024);
			    tdata->ColBuf = ptr;
			    tdata->ColBufSize += 1024;*/
			    break;
			    }
			memcpy(tdata->ColBuf + tdata->ColBufLen, ptr, n);
			tdata->ColFmt[tdata->nCols] = tdata->ColBuf + tdata->ColBufLen;
			tdata->ColBufLen += n;
			}

		    /** Next column... **/
		    tdata->nCols++;
		    }
		}

	    /** Ok, loaded the columns.  Now sort 'em **/
	    dat_internal_SortCols(dn->TableInf);
	    dat_internal_SortCols(&(dn->HeaderCols));

	    /** Renumber the column id's **/
	    for(i=0;i<dn->TableInf->nCols;i++) dn->TableInf->ColIDs[i] = i;

	    /** Got sorted columns.  Now process the keys **/
	    for(i=0;i<dn->TableInf->nCols;i++)
	        {
		if (dn->TableInf->ColKeys[i])
		    {
		    dn->TableInf->KeyCols[dn->TableInf->nKeys] = i;
		    dn->TableInf->Keys[dn->TableInf->nKeys] = dn->TableInf->Cols[i];
		    dn->TableInf->ColKeys[i] = dn->TableInf->nKeys + 1;
		    dn->TableInf->ColFlags[i] |= DAT_CF_PRIKEY;
		    dn->TableInf->nKeys++;
		    }
		}
	    }

	/** Add node to cache? **/
	if (new_node)
	    {
	    *(dn->EndSpecPath) = '\0';
	    xhAdd(&DAT_INF.DBNodes, (void*)(dn->SpecPath), (void*)dn);
	    }

    return dn;
    }


/*** dat_csv_GenerateText - build a textual data buffer suitable to be output into a
 *** CSV format or other ASCII variable-field formats.  Does NOT null-terminate the
 *** resulting ASCII field.  Does NOT add the field-separator.  DOES add quote marks
 *** around the field if needed.
 ***/
int
dat_csv_GenerateText(pDatNode node, int colid, pObjData val, unsigned char* buf, int maxlen)
    {
    unsigned char quot;
    int len;
    char* ptr;
    char* tmpptr;
    int type = node->TableInf->ColTypes[colid];
    int quote_cnt;
    char* savedfmt = NULL;

    	/** Do we need to quote the thing?  If so, which quote is best? **/
	if ((node->Flags & DAT_NODE_F_QUOTEALL) || (type == DATA_T_STRING && strchr(val->String, node->FieldSep)) ||
	    (type == DATA_T_STRING && strlen(val->String) == 0))
	    {
	    if (type == DATA_T_STRING && strchr(val->String,'"') && !(node->Flags & DAT_NODE_F_DBLQUOTEONLY))
	        quot = '\'';
	    else
	        quot = '"';
	    }
	else
	    {
	    quot = '\0';
	    }

	/** Forced non-quote or quote? **/
	if (node->TableInf->ColFlags[colid] & DAT_CF_QUOTED && !quot)
	    {
	    if (type == DATA_T_STRING && strchr(val->String,'"') && !(node->Flags & DAT_NODE_F_DBLQUOTEONLY))
	        quot = '\'';
	    else
	        quot = '"';
	    }
	else if (node->TableInf->ColFlags[colid] & DAT_CF_NONQUOTED && quot)
	    {
	    if (type == DATA_T_STRING && strchr(val->String, node->FieldSep))
	        {
		mssError(1,"DAT","Field <%s> forced non-quoted but contains field separator!",node->TableInf->Cols[colid]);
		return -1;
		}
	    quot = '\0';
	    }

	/** Need to set a date or money format? **/
	if (type == DATA_T_DATETIME && node->TableInf->ColFmt[colid])
	    {
	    savedfmt = mssGetParam("dfmt");
	    if (!savedfmt) savedfmt = obj_default_date_fmt;
	    mssSetParam("dfmt",node->TableInf->ColFmt[colid]);
	    }
	else if (type == DATA_T_MONEY && node->TableInf->ColFmt[colid])
	    {
	    savedfmt = mssGetParam("mfmt");
	    if (!savedfmt) savedfmt = obj_default_money_fmt;
	    mssSetParam("mfmt",node->TableInf->ColFmt[colid]);
	    }

	/** Get a string representation of the value. **/
	if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
	    ptr = objDataToStringTmp(type, (void*)val, 0);
	else
	    ptr = objDataToStringTmp(type, *(void**)val, 0);

	/** Need to put back a date or money format? **/
	if (savedfmt)
	    {
	    if (type == DATA_T_DATETIME)
	        mssSetParam("dfmt",savedfmt);
	    else
	        mssSetParam("mfmt",savedfmt);
	    }

	/** Count the number of quote marks in it that will be escaped. **/
	quote_cnt = 0;
	if (quot)
	    {
	    tmpptr = ptr;
	    while((tmpptr = strchr(tmpptr,quot)))
	        {
		tmpptr++;
		quote_cnt++;
		}
	    }

	/** Do we have enough room? **/
	len = strlen(ptr) + quote_cnt + (quot?2:0);
	if (len > maxlen) 
	    {
	    mssError(1,"DAT","Failed to write column <%s>, needed %d bytes, %d available", node->TableInf->Cols[colid], len, maxlen);
	    return -1;
	    }

	/** Copy the data. **/
	if (quot) *(buf++) = quot;
	while(*ptr)
	    {
	    if (*ptr == '\r' || *ptr == '\n')
	        {
		*(buf++) = ' ';
		}
	    else
	        {
	        if (*ptr == quot) *(buf++) = '\\';
	        *(buf++) = *ptr;
		}
	    ptr++;
	    }
	if (quot) *(buf++) = quot;

    return len;
    }


/*** dat_csv_UpdateRowBuf - update the RowBuf in the row's inf structure to reflect
 *** a modified property.
 ***/
int
dat_internal_UpdateRowBuf(pDatData inf, int update_colid, pObjData update_val)
    {
    return 0;
    }


/*** dat_csv_GenerateRow - generate an entire textual format row, including the 
 *** allocation of the buffer.  Optionally, look in the update ptr or in the
 *** OXT structures for the source data.  DOES add the field separators.  DOES
 *** NOT add the trailing newline or trailing spaces.  DOES null-terminate the
 *** buffer.
 ***/
unsigned char*
dat_csv_GenerateRow(pDatData inf, int update_colid, pObjData update_val, pObjTrxTree oxt)
    {
    pObjTrxTree sub_oxt;
    int i,len,j,d,s;
    unsigned char* buf;
    unsigned char* bufptr;
    int maxlen;
    pObjData val;
    pObjData oldval;

    	/** Allocate a buffer **/
	maxlen = DAT_CACHE_PAGESIZE*(DAT_ROW_MAXPAGESPAN-1);
	buf = (unsigned char*)nmMalloc(maxlen+1);

	/** Step through the fields, one at a time. **/
	bufptr = buf;
	for(i=0;i<inf->TData->nCols;i++)
	    {
	    val = NULL;
	    /** What's the source of this data? **/
	    if (update_colid == i)
	        {
		/** Source: from UPDATE (SetAttrValue) command. **/
		val = update_val;
		}
	    else if (oxt)
	        {
		/** Source: from INSERT (OXT - multiple SetAttrValue) **/
		for(j=0;j<oxt->Children.nItems;j++)
		    {
		    sub_oxt = (pObjTrxTree)(oxt->Children.Items[j]);
		    if (sub_oxt->OpType == OXT_OP_SETATTR && !strcmp(sub_oxt->AttrName,inf->TData->Cols[i]))
		        {
			if (sub_oxt->AttrType == DATA_T_INTEGER || sub_oxt->AttrType == DATA_T_DOUBLE)
			    val = POD(sub_oxt->AttrValue);
			else
			    val = POD(&(sub_oxt->AttrValue));
			break;
			}
		    }
		}
	    if (!val)
	        {
		/** Source: from EXISTING ROW DATA **/
		if (inf->ColPtrs[i] == NULL)
		    val = NULL;
		else if (inf->TData->ColTypes[i] == DATA_T_INTEGER || inf->TData->ColTypes[i] == DATA_T_DOUBLE)
		    val = POD(inf->ColPtrs[i]);
		else
		    val = POD(&(inf->ColPtrs[i]));
		}
	    else
		{
		/** Need to update rowbuf - find old value **/
		if (inf->ColPtrs[i] == NULL)
		    oldval = NULL;
		else if (inf->TData->ColTypes[i] == DATA_T_INTEGER || inf->TData->ColTypes[i] == DATA_T_DOUBLE)
		    oldval = POD(inf->ColPtrs[i]);
		else
		    oldval = POD(&(inf->ColPtrs[i]));

		/** Original was null? (as in the case of insert) **/
		if (val && !oldval)
		    {
		    switch(inf->TData->ColTypes[i])
			{
			case DATA_T_INTEGER:
			    s = sizeof(int);
			    break;
			case DATA_T_DOUBLE:
			    s = sizeof(double);
			    break;
			case DATA_T_MONEY:
			    s = sizeof(MoneyType);
			    break;
			case DATA_T_DATETIME:
			    s = sizeof(DateTime);
			    break;
			case DATA_T_STRING:
			    s = 1;
			    break;
			}
		    if (inf->RowBufSize + s <= maxlen)
			{
			inf->ColPtrs[i] = inf->RowBuf + inf->RowBufSize;
			inf->RowBufSize += s;
			if (inf->TData->ColTypes[i] == DATA_T_INTEGER || inf->TData->ColTypes[i] == DATA_T_DOUBLE)
			    oldval = POD(inf->ColPtrs[i]);
			else
			    oldval = POD(&(inf->ColPtrs[i]));
			if (inf->TData->ColTypes[i] == DATA_T_STRING)
			    inf->ColPtrs[i][0] = '\0';
			}
		    }

		/** And set it based on its type **/
		if (val && oldval)
		    {
		    switch(inf->TData->ColTypes[i])
			{
			case DATA_T_INTEGER:
			    memcpy(&(oldval->Integer), &(val->Integer), sizeof(int));
			    break;
			case DATA_T_DOUBLE:
			    memcpy(&(oldval->Double), &(val->Double), sizeof(double));
			    break;
			case DATA_T_DATETIME:
			    memcpy(oldval->DateTime, val->DateTime, sizeof(DateTime));
			    break;
			case DATA_T_MONEY:
			    memcpy(oldval->Money, val->Money, sizeof(MoneyType));
			    break;
			case DATA_T_STRING:
			    /** may have to move ptrs **/
			    if (val->String && oldval->String)
				{
				len = strlen(oldval->String);
				d = strlen(val->String) - len;
				if (inf->RowBufSize + d <= maxlen)
				    {
				    for(j=0;j<inf->TData->nCols;j++)
					{
					if (inf->ColPtrs[j] && inf->ColPtrs[j] > inf->ColPtrs[i]) inf->ColPtrs[j] += d;
					}
				    memmove(inf->ColPtrs[i]+d+len,inf->ColPtrs[i]+len, inf->RowBufSize - (inf->ColPtrs[i] - inf->RowBuf) - len);
				    inf->RowBufSize += d;
				    strcpy(oldval->String, val->String);
				    }
				}
			    break;
			}
		    }
		}

	    /** Generate the item. **/
	    if (val)
	        {
	        len = dat_csv_GenerateText(inf->Node, i, val, bufptr, maxlen - (bufptr - buf));
	        if (len < 0)
	            {
		    nmFree(buf,maxlen+1);
		    return NULL;
		    }
	        bufptr += len;
		}
	    else
	        {
		/** null? **/
		if (!(inf->TData->ColFlags[i] & DAT_CF_ALLOWNULL))
		    {
		    mssError(1,"DAT","Attribute <%s> does not allow NULL values", inf->TData->Cols[i]);
		    nmFree(buf,maxlen+1);
		    return NULL;
		    }
		}

	    /** Add field-sep? **/
	    if (i != (inf->TData->nCols - 1))
	        {
		if (maxlen - (bufptr - buf) <= 0)
		    {
		    mssError(1,"DAT","Could not generate row: row exceeded max length of %d bytes",maxlen);
		    nmFree(buf,maxlen+1);
		    return NULL;
		    }
		*(bufptr++) = inf->Node->FieldSep;
		}
	    }

	/** Add null-termination **/
	*bufptr = '\0';

    return buf;
    }


/*** dat_internal_FindLastRow() - locates the last row in the file and determines
 *** its row ID, setting that value in node->nRows.  Returns -1 on error, 0 on
 *** success.
 ***/
int
dat_internal_FindLastRow(pDatData context, pDatNode node)
    {
    pDatRowInfo ri;

	/** Already known? **/
	if (node->nRows >= 0) return 0;

	/** Try to do a GetRow on the highest possible row id.  This will
	 ** force a scan of the entire datafile.
	 **/
	ri = dat_internal_GetRow(context,node, DAT_MAXROWID);
	if (ri)
	    {
	    /** this is an unusual case **/
	    node->nRows = DAT_MAXROWID+1;
	    dat_internal_ReleaseRow(ri);
	    return 0;
	    }
	node->nRows = node->RealMaxRowID + 1;
	if (node->Flags & DAT_NODE_F_HDRROW) node->nRows--;

    return 0;
    }


/*** datOpen - open a table, row, or column.
 ***/
void*
datOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pDatData inf;
    int i;
    pStruct openctl_data;
    char *ptr;

	/** Allocate the structure **/
	inf = (pDatData)nmMalloc(sizeof(DatData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(DatData));
	inf->Obj = obj;
	inf->Mask = mask;
	inf->Pathname.OpenCtlBuf = NULL;

	/** Determine type and set pointers. **/
	if (dat_internal_DetermineType(obj,inf) < 0)
	    {
	    nmFree(inf,sizeof(DatData));
	    return NULL;
	    }

	/** Spec name passed in via openctl? **/
	if (obj->Pathname->OpenCtl[obj->SubPtr-1] && (openctl_data = stLookup_ne(obj->Pathname->OpenCtl[obj->SubPtr-1],"specobj")))
	    {
	    ptr = NULL;
	    stAttrValue_ne(openctl_data, &ptr);
	    if (ptr)
		{
		strtcpy(inf->SpecName, ptr, sizeof(inf->SpecName));
		}
	    }

	/** Access the DB node. **/
	inf->Node = dat_internal_OpenNode(inf, obj, obj_internal_PathPart(obj->Pathname,0,obj->SubPtr), obj->Mode,inf->Type == DAT_T_FILESPEC,mask);
	obj_internal_PathPart(obj->Pathname,0,0);
	if (!(inf->Node))
	    {
	    mssError(0,"DAT","Could not open database node!");
	    obj_internal_FreePathStruct(&inf->Pathname);
	    nmFree(inf,sizeof(DatData));
	    return NULL;
	    }
	inf->TData = inf->Node->TableInf;

	/** If opening the datafile itself, fetch header if necessary **/
	if (inf->Type == DAT_T_TABLE && (inf->Node->Flags & DAT_NODE_F_HDRROW))
	    {
	    inf->RowID = -1;
	    inf->Row = dat_internal_GetRow(inf,inf->Node, inf->RowID);
	    if (inf->Row)
	        {
		inf->Flags |= DAT_F_ROWPRESENT;
		dat_csv_ParseRow(inf, &(inf->Node->HeaderCols));
		inf->Flags |= DAT_F_ROWPARSED;
		dat_internal_ReleaseRow(inf->Row);
		inf->Row = NULL;
		inf->Flags &= DAT_F_ROWPRESENT;
		}
	    }

	/** If opening a row, fetch it. **/
	if (inf->Type == DAT_T_ROW)
	    {
	    if (inf->Node->Flags & DAT_NODE_F_ROWIDKEY)
	        {
		/** Auto name/key generation? **/
		if (inf->Obj->Mode & OBJ_O_AUTONAME)
		    {
		    dat_internal_FindLastRow(inf, inf->Node);
		    syGetSem(inf->Node->InsertSem, 1, 0);
		    inf->Flags |= DAT_F_HOLDINSERTSEM;
		    snprintf(inf->AutoName, sizeof(inf->AutoName), "%d", inf->Node->nRows);
		    inf->RowID = inf->Node->nRows;
		    }
		else
		    {
		    inf->RowID = strtoi(inf->RowColPtr,NULL,10);
		    }
		inf->Row = dat_internal_GetRow(inf,inf->Node, inf->RowID);
		if (inf->Row && inf->Row->Flags & DAT_R_F_DELETED)
		    {
		    dat_internal_ReleaseRow(inf->Row);
		    inf->Row = NULL;
		    }
		if (!inf->Row && !(obj->Mode & O_CREAT))
		    {
		    mssError(0,"DAT","Open: Row id '%s' not found.", inf->RowColPtr);
		    obj_internal_FreePathStruct(&inf->Pathname);
		    nmFree(inf,sizeof(DatData));
		    return NULL;
		    }
		else if (inf->Row && (obj->Mode & O_CREAT) && (obj->Mode & O_EXCL))
		    {
		    mssError(1,"DAT","Create: Row id '%s' already exists", inf->RowColPtr);
		    dat_internal_ReleaseRow(inf->Row);
		    obj_internal_FreePathStruct(&inf->Pathname);
		    nmFree(inf,sizeof(DatData));
		    return NULL;
		    }
		else if (!inf->Row)
		    {
		    for(i=0;i<inf->TData->nCols;i++) inf->ColPtrs[i] = NULL;
		    if (!*oxt) *oxt = obj_internal_AllocTree();
		    (*oxt)->OpType = OXT_OP_CREATE;
		    (*oxt)->LLParam = (void*)inf;
		    (*oxt)->Status = OXT_S_VISITED;
		    (*oxt)->Object = obj;
		    inf->Flags |= DAT_F_ROWPARSED;
		    return inf;
		    }
		inf->Flags |= DAT_F_ROWPRESENT;
		dat_csv_ParseRow(inf, inf->TData);
		inf->Flags |= DAT_F_ROWPARSED;
		dat_internal_ReleaseRow(inf->Row);
		inf->Row = NULL;
		inf->Flags &= DAT_F_ROWPRESENT;
		}
	    else
	        {
		}
	    }

    return (void*)inf;
    }


/*** dat_internal_InsertRow - inserts a new row into the datafile at the
 *** end of the file.  Does not search for deleted space to put the row;
 *** to remove deleted space the Compact method should be executed.
 ***/
int
dat_internal_InsertRow(pDatData context, pDatNode node, unsigned char* rowdata)
    {
    int curpg;
    int rowid;
    pDatRowInfo ri;
    pDatPage pg;
    unsigned char* ptr;
    int is_missing_nl = 0;
    int len,len2,i;
    int n_pages;
    unsigned char ch;
    unsigned char* endptr;
    int rval;

    	/** Hmm... is row too big? **/
	len = strlen((char*)rowdata) + 1 + node->NewRowPadding;
	endptr = rowdata + strlen((char*)rowdata);
	if (len > DAT_CACHE_PAGESIZE*(DAT_ROW_MAXPAGESPAN-1))
	    {
	    mssError(1,"DAT","Length %d for new row exceeds maximum of %d bytes",len,DAT_CACHE_PAGESIZE*(DAT_ROW_MAXPAGESPAN-1));
	    return -1;
	    }

	/** Get the insert semaphore? **/
	if (!(context->Flags & DAT_F_HOLDINSERTSEM)) syGetSem(node->InsertSem, 1, 0);

	/** Make sure we have scanned the entire file **/
	if (dat_internal_FindLastRow(context,node) < 0)
	    {
	    if (!(context->Flags & DAT_F_HOLDINSERTSEM)) syPostSem(node->InsertSem, 1, 0);
	    return -1;
	    }
	rowid = node->nRows;
	ri = NULL;
	if (node->nRows)
	    {
	    ri = dat_internal_GetRow(context,node, node->nRows - 1);
	    curpg = ri->StartPageID + ri->nPages - 1;
	    pg = ri->Pages[ri->nPages-1];
	    if (pg->Length == 0)
		{
		is_missing_nl = 1;
		}
	    else
		{
		ch = pg->Data[pg->Length-1];
		if (ch != '\n' && ch != 0x01)
		    is_missing_nl = 1;
		else
		    is_missing_nl = 0;
		}
	    if (pg->Length == DAT_CACHE_PAGESIZE)
		curpg++;
	    }
	if (!ri)
	    {
	    is_missing_nl = 0;
	    curpg = 0;
	    }
	else
	    {
	    dat_internal_ReleaseRow(ri);
	    }
	pg = dat_internal_ReadPage(context, node, curpg);
	if (!pg) pg = dat_internal_NewPage(context, node, curpg);
	if (!pg)
	    {
	    if (!(context->Flags & DAT_F_HOLDINSERTSEM)) syPostSem(node->InsertSem, 1, 0);
	    mssError(1,"DAT","Bark!  Internal error inserting row");
	    return -1;
	    }

	/** Ok, found last record in the file.  How many more pages do we need? **/
	len2 = len;
	len += (is_missing_nl?1:0);
	len -= (DAT_CACHE_PAGESIZE - pg->Length);
	n_pages = (len + DAT_CACHE_PAGESIZE - 1)/DAT_CACHE_PAGESIZE;
	ri = (pDatRowInfo)nmMalloc(sizeof(DatRowInfo));
	ri->nPages = n_pages + 1;
	ri->Pages[0] = pg;
	ri->StartPageID = curpg;
	ri->Offset = pg->Length + (is_missing_nl?1:0);
	for(i=1;i<=n_pages;i++) 
	    {
	    ri->Pages[i] = dat_internal_NewPage(context, node, curpg + i);
	    if (!ri->Pages[i])
	        {
		mssError(1,"DAT","Internal error - could not allocate pages for new row");
		ri->nPages = i;
		dat_internal_ReleaseRow(ri);
		if (!(context->Flags & DAT_F_HOLDINSERTSEM)) syPostSem(node->InsertSem, 1, 0);
		return -1;
		}
	    }

	/** Got the pages.  Now copy the row. **/
	ptr = ri->Pages[0]->Data + ri->Pages[0]->Length;
	ri->Pages[0]->Flags |= DAT_CACHE_F_DIRTY;
	curpg = 0;
	while(len2 || is_missing_nl)
	    {
	    if (is_missing_nl)
	        {
		is_missing_nl = 0;
		*(ptr++) = '\n';
		}
	    else if (rowdata < endptr)
	        {
		*(ptr++) = *(rowdata++);
		len2--;
		}
	    else 
		{
		*(ptr++) = ' ';
		len2--;
		}
	    if (ptr == ri->Pages[curpg]->Data + DAT_CACHE_PAGESIZE)
	        {
		ri->Pages[curpg]->Length = DAT_CACHE_PAGESIZE;
		curpg++;
		ptr = ri->Pages[curpg]->Data;
		ri->Pages[curpg]->Flags |= DAT_CACHE_F_DIRTY;
		}
	    }

	/** Add trailing newline, and adjust end page length. **/
	*(ptr++) = '\n';
	ri->Pages[curpg]->Length = (ptr - ri->Pages[curpg]->Data);
	ri->Pages[curpg]->Flags |= DAT_CACHE_F_DIRTY;

	/** Write the data back. **/
	for(i=1;i<=curpg;i++) dat_internal_UnlockPage(ri->Pages[i]);
	rval = dat_internal_FlushPages(context,ri->Pages[0]); /* unlocks it too */
	dat_internal_UpdateRowIDPtrCache(node,ri,rowid);
	nmFree(ri,sizeof(DatRowInfo));
	node->nRows++;
	syPostSem(node->InsertSem, 1, 0);
	context->Flags &= ~DAT_F_HOLDINSERTSEM;

    return rval;
    }


/*** datCommit - commit changes.
 ***/
int
datCommit(void* inf_v, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    unsigned char* insbuf;

    	/** Was this a create? **/
	if ((*oxt) && (*oxt)->OpType == OXT_OP_CREATE && (*oxt)->Status != OXT_S_COMPLETE)
	    {
	    switch (inf->Type)
	        {
		case DAT_T_TABLE:
		    /** We'll get to this a little later **/
		    break;

		case DAT_T_ROW:
		    /** Perform the insert. **/
		    insbuf = dat_csv_GenerateRow(inf, -1, NULL, *oxt);
		    if (!insbuf)
		        {
			/** FAIL the oxt. **/
			(*oxt)->Status = OXT_S_FAILED;
			return -1;
			}
		    if (dat_internal_InsertRow(inf, inf->Node, insbuf) < 0)
		        {
			/** FAIL the oxt. **/
			(*oxt)->Status = OXT_S_FAILED;
			nmFree(insbuf,DAT_CACHE_PAGESIZE*(DAT_ROW_MAXPAGESPAN-1));
			return -1;
			}

		    /** Complete the oxt. **/
		    (*oxt)->Status = OXT_S_COMPLETE;

		    break;

		case DAT_T_COLUMN:
		    /** We wait until table is done for this. **/
		    break;
		}
	    }

	/** Release the row descriptor, if one **/
	if (inf->Row) dat_internal_ReleaseRow(inf->Row);

    return 0;
    }


/*** datClose - close an open object
 ***/
int
datClose(void* inf_v, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    int rval;

	rval = datCommit(inf_v, oxt);

	/** Close out the 'other' object as well... **/
	if (inf->SpecObj != NULL) objClose(inf->SpecObj);
	if (inf->DataObj != NULL) objClose(inf->DataObj);

	/** Free the info structure **/
	obj_internal_FreePathStruct(&inf->Pathname);
	if (inf->Flags & DAT_F_HOLDINSERTSEM) syPostSem(inf->Node->InsertSem, 1, 0);
	nmFree(inf,sizeof(DatData));

    return rval;
    }


/*** datCreate - create a new object without actually opening that 
 *** object.
 ***/
int
datCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* objd;

    	/** Open and close the object **/
	obj->Mode |= O_CREAT;
	objd = datOpen(obj,mask,systype,usrtype,oxt);
	if (!objd) return -1;

    return datClose(objd,oxt);
    }


/*** datDeleteObj - delete an existing open object.
 ***/
int
datDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    unsigned char* dstptr;
    int curpg,i;
    pDatRowInfo ri;
    int rval;

	if (!inf->Row || !(inf->Flags & DAT_F_ROWPRESENT))
	    {
	    inf->Row = dat_internal_GetRow(inf,inf->Node, inf->RowID);
	    if (inf->Row && inf->Row->Flags & DAT_R_F_DELETED)
		{
		dat_internal_ReleaseRow(inf->Row);
		inf->Row = NULL;
		}
	    if (!inf->Row)
		{
		mssError(1,"DAT","DeleteObj: Could not access row data for row %d",inf->RowID);
		if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
		inf->Pathname.OpenCtlBuf = NULL;
		nmFree(inf,sizeof(DatData));
		return -1;
		}
	    }

	/** Now delete the thing. **/
	ri = inf->Row;
	ri->Pages[0]->Flags |= DAT_CACHE_F_DIRTY;
	dstptr = ri->Pages[0]->Data + ri->Offset;
	curpg = 0;
	while(*dstptr != '\n')
	    {
	    *(dstptr++) = ' ';
	    if (dstptr >= ri->Pages[curpg]->Data + ri->Pages[curpg]->Length)
		{
		curpg++;
		dstptr = ri->Pages[curpg]->Data;
		ri->Pages[curpg]->Flags |= DAT_CACHE_F_DIRTY;
		}
	    }
	*dstptr = 1;
	for(i=1;i<=curpg;i++) dat_internal_UnlockPage(ri->Pages[curpg]);
	rval = dat_internal_FlushPages(inf,ri->Pages[0]);
	nmFree(ri,sizeof(DatRowInfo));

	/** Free the structure **/
        if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
        inf->Pathname.OpenCtlBuf = NULL;
	nmFree(inf,sizeof(DatData));

    return rval;
    }


/*** datDelete - delete an existing object.
 ***/
int
datDelete(pObject obj, pObjTrxTree* oxt)
    {
    pDatData inf;

	/** Allocate the structure **/
	inf = (pDatData)nmMalloc(sizeof(DatData));
	if (!inf) return -1;
	memset(inf,0,sizeof(DatData));
	inf->Obj = obj;
	inf->Pathname.OpenCtlBuf = NULL;

	/** Determine type and set pointers. **/
	dat_internal_DetermineType(obj,inf);

	/** If a row, proceed else fail the delete. **/
	if (inf->Type != DAT_T_ROW)
	    {	
            if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
            inf->Pathname.OpenCtlBuf = NULL;
	    nmFree(inf,sizeof(DatData));
	    puts("Unimplemented delete operation in DAT.");
	    mssError(1,"DAT","Unimplemented delete operation in DAT");
	    return -1;
	    }

	/** Access the DB node. **/
	inf->Node = dat_internal_OpenNode(inf, obj, obj_internal_PathPart(obj->Pathname,0,obj->SubPtr),obj->Mode,inf->Type == DAT_T_FILESPEC,0600);
	obj_internal_PathPart(obj->Pathname,0,0);
	if (!(inf->Node))
	    {
	    mssError(0,"DAT","Could not open database node!");
            if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
            inf->Pathname.OpenCtlBuf = NULL;
	    nmFree(inf,sizeof(DatData));
	    return -1;
	    }
	
	inf->RowID = strtoi(inf->RowColPtr,NULL,10);

	/** Get the rowid and fetch the row **/
	inf->Row = dat_internal_GetRow(inf,inf->Node, inf->RowID);
	if (inf->Row && inf->Row->Flags & DAT_R_F_DELETED)
	    {
	    dat_internal_ReleaseRow(inf->Row);
	    inf->Row = NULL;
	    }
	if (!inf->Row)
	    {
	    mssError(0,"DAT","Delete: Row id '%s' not found.", inf->RowColPtr);
            if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
            inf->Pathname.OpenCtlBuf = NULL;
	    nmFree(inf,sizeof(DatData));
	    return -1;
	    }
	inf->Flags |= DAT_F_ROWPRESENT;

    return datDeleteObj(inf,oxt);
    }


/*** datRead - read from the object's content.  This only returns content
 *** when a row is opened within a table that has any attribute that
 *** is a 'text' or 'image' data type.  If more than one attribute has such
 *** a datatype, the first one is used.
 ***/
int
datRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /* GRB - uncomment these vars when the function is coded */
    /*pDatData inf = DAT(inf_v);
    int cnt;
    char* col = NULL;
    int i,rcnt,rval;*/

    return -1;
    }


/*** dat_internal_OpenTmpFile - name/create a new tmp file and return a file 
 *** descriptor for it.
 ***/
pFile
dat_internal_OpenTmpFile(char* name)
    {
    char ch;
    time_t t;

	/** Use 26 letters, with timestamp and random value **/
	t = time(NULL);
    	for(ch='a';ch<='z';ch++)
	    {
	    sprintf(name,"/tmp/LS-%8.8lX%4.4lX%c",t,lrand48()&0xFFFF,ch);
	    if (access(name,F_OK) < 0)
	        {
		return fdOpen(name, O_RDWR, 0600);
		}
	    }
	mssErrorErrno(1,"DAT","Could not open temp file");

    return NULL;
    }


/*** datWrite - write to an object's content.  If the user specified a 'size' value
 *** and O_TRUNC was specified, use that size and stream the writes to the database.
 *** Otherwise, if just 'size' specified, use the greater of 'size' and current size,
 *** and pad with NUL's if 'size' is smaller.  If 'size' not specified, we have to write
 *** the Write() calls to a temp file first, determine the size, and proceed as before,
 *** deleting the temp file when done.  Sigh.  Reason?  Sybase wants to know the exact
 *** size of a BLOB write operation before writing can start.
 ***/
int
datWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /* GRB - uncomment these vars when the function is coded */
    /*pDatData inf = DAT(inf_v);
    char* col = NULL;
    int i,rcnt,rval;
    char sbuf[160];*/

    return -1;
    }


/*** datOpenQuery - open a directory query. Until index file support is available,
 *** the where and orderby stuff has to be handled by the OSML.
 ***/
void*
datOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    pDatQuery qy;
    pDatRowInfo ri;

	/** Allocate the query structure **/
	qy = (pDatQuery)nmMalloc(sizeof(DatQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(DatQuery));
	qy->ObjInf = inf;
	qy->RowCnt = -1;
	qy->TableInf = inf->TData;

	/** State that we won't do full query (yet) **/
	query->Flags &= ~OBJ_QY_F_FULLQUERY;
	query->Flags &= ~OBJ_QY_F_FULLSORT;

	/** start the query based on object type. **/
	switch(inf->Type)
	    {
	    case DAT_T_TABLE:
	        /** No SQL needed -- always returns just 'columns' and 'rows' **/
		qy->RowCnt = 0;
	        break;

	    case DAT_T_COLSOBJ:
	        /** Get a columns list. **/
		qy->TableInf = qy->ObjInf->TData;
		qy->RowCnt = 0;
		break;

	    case DAT_T_ROWSOBJ:
	        /** Query the rows within a table -- scan the file **/
		qy->RowCnt = 0;
		qy->RowID = 0;

		/** Find some temporary row 0 start information. **/
		ri = dat_internal_GetRow(inf,inf->Node, 0);
		if (ri)
		    {
		    qy->Row.StartPageID = ri->StartPageID;
		    qy->Row.Offset = ri->Offset;
		    qy->Row.Length = ri->Length;
		    qy->Row.Flags = ri->Flags;
		    qy->Row.nPages = ri->nPages;
		    dat_internal_ReleaseRow(ri);
		    }
		else
		    {
		    qy->Row.StartPageID = 0xFFFFFFFF;
		    }
		break;

	    case DAT_T_COLUMN:
	    case DAT_T_ROW:
	        /** These don't support queries for sub-objects. **/
	        nmFree(qy,sizeof(DatQuery));
		qy = NULL;
		break;
	    }

    return (void*)qy;
    }


/*** datQueryFetch - get the next directory entry as an open object.
 ***/
void*
datQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pDatQuery qy = ((pDatQuery)(qy_v));
    pDatData inf;
    char filename[80];
    unsigned char* ptr;
    int new_type;
    int i;
    pDatTableInf tdata = qy->ObjInf->TData;
    int page, new_offset, len;

	qy->RowCnt++;
	qy->RowsSinceFetch++;

	/** Allocate the structure **/
	inf = (pDatData)nmMalloc(sizeof(DatData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(DatData));
	inf->TData = tdata;
	inf->Node = qy->ObjInf->Node;
	inf->Pathname.OpenCtlBuf = NULL;
	if (qy->ObjInf->DataObj)
	    {
	    inf->DataObj = qy->ObjInf->DataObj;
	    objLinkTo(inf->DataObj);
	    }
	if (qy->ObjInf->SpecObj)
	    {
	    inf->SpecObj = qy->ObjInf->SpecObj;
	    objLinkTo(inf->SpecObj);
	    }

    	/** Get the next name based on the query type. **/
	switch(qy->ObjInf->Type)
	    {
	    case DAT_T_TABLE:
	        /** Filename is either "rows" or "columns" **/
		if (qy->RowCnt == 1) 
		    {
		    strcpy(filename,"columns");
		    new_type = DAT_T_COLSOBJ;
		    }
		else if (qy->RowCnt == 2) 
		    {
		    strcpy(filename,"rows");
		    new_type = DAT_T_ROWSOBJ;
		    }
		else 
		    {
        	    if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
        	    inf->Pathname.OpenCtlBuf = NULL;
		    nmFree(inf,sizeof(DatData));
		    /*mssError(1,"DAT","Table object has only two subobjects: 'rows' and 'columns'");*/
		    return NULL;
		    }
	        break;

	    case DAT_T_ROWSOBJ:
	        /** No rows at all? **/
		if (qy->Row.StartPageID == 0xFFFFFFFF)
		    {
        	    if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
        	    inf->Pathname.OpenCtlBuf = NULL;
		    nmFree(inf,sizeof(DatData));
		    return NULL;
		    }

	        /** Scan for a row if need be. **/
		if (qy->RowCnt != 1 || (qy->Row.Flags & DAT_R_F_DELETED))
		    {
		    page = qy->Row.StartPageID;
		    new_offset = qy->Row.Offset + qy->Row.Length + 1;
		    while(new_offset >= DAT_CACHE_PAGESIZE)
		        {
			page++;
			new_offset -= DAT_CACHE_PAGESIZE;
			}
		    qy->Row.Pages[0] = dat_internal_ReadPage(qy->ObjInf, qy->ObjInf->Node, page);
		    if (!qy->Row.Pages[0]) 
		        {
        	        if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
        	        inf->Pathname.OpenCtlBuf = NULL;
			nmFree(inf,sizeof(DatData));
			return NULL;
			}
		    qy->Row.StartPageID = page;
		    page = 0;
		    qy->Row.Offset = new_offset;
		    ptr = qy->Row.Pages[page]->Data + qy->Row.Offset;
		    len = 0;
		    while(new_offset < qy->Row.Pages[page]->Length && *ptr != '\n')
		        {
			/** End of row, but row deleted? **/
			if (*ptr == 1)
			    {
			    qy->RowID++;
			    for(i=0;i<=page;i++) dat_internal_UnlockPage(qy->Row.Pages[i]);
			    qy->Row.Pages[0] = qy->Row.Pages[page];
			    len = -1;
			    qy->Row.Offset = new_offset+1;
			    qy->Row.StartPageID += page;
			    page = 0;
			    }

			/** Next character 'group'... **/
#if 00
			nlptr = memchr(ptr, '\n', qy->Row.Pages[page]->Length - new_offset);
			if (nlptr)
			    {
			    captr = memchr(ptr, 1, nlptr - ptr);
			    if (captr)
			        {
				len += (captr - ptr);
				new_offset += (captr - ptr);
				ptr = captr;
				}
			    else
			        {
				len += (nlptr - ptr);
				new_offset += (nlptr - ptr);
				ptr = nlptr;
				}
			    }
			else
			    {
			    captr = memchr(ptr, 1, qy->Row.Pages[page]->Length - new_offset);
			    if (captr)
			        {
				len += (captr - ptr);
				new_offset += (captr - ptr);
				ptr = captr;
				}
			    else
			        {
				len += (qy->Row.Pages[page]->Length - new_offset);
				ptr += (qy->Row.Pages[page]->Length - new_offset);
				new_offset = qy->Row.Pages[page]->Length;
				}
			    }
#else
			new_offset++;
			ptr++;
			len++;
#endif
			if (new_offset >= qy->Row.Pages[page]->Length)
			    {
			    if (qy->Row.Offset == new_offset && page == 0)
			        {
				qy->Row.Offset = 0;
				qy->Row.StartPageID++;
				}
			    else
			        {
			        page++;
				}
			    if (page >= DAT_ROW_MAXPAGESPAN)
			        {
				mssError(1,"DAT","Row length exceeds max page span");
        	        	if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
        	        	inf->Pathname.OpenCtlBuf = NULL;
				nmFree(inf,sizeof(DatData));
				for(i=0;i<page;i++) dat_internal_UnlockPage(qy->Row.Pages[i]);
				return NULL;
				}
			    new_offset = 0;
			    qy->Row.Pages[page] = dat_internal_ReadPage(qy->ObjInf, qy->ObjInf->Node, qy->Row.StartPageID + page);
			    if (!qy->Row.Pages[page]) 
			        {
			        len--;
				page--;
				break;
				}
			    ptr = qy->Row.Pages[page]->Data;
			    }
			}

		    /** Didn't find nothin' ? **/
		    if (len == 0 || page < 0)
		        {
       	        	if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
       	        	inf->Pathname.OpenCtlBuf = NULL;
			nmFree(inf,sizeof(DatData));
			for(i=0;i<=page;i++) dat_internal_UnlockPage(qy->Row.Pages[i]);
			return NULL;
			}

		    /** Parse the row and release the pages. **/
		    qy->Row.Length = len;
		    qy->Row.nPages = page+1;
		    }
		else
		    {
		    for(i=0;i<qy->Row.nPages;i++) 
		        qy->Row.Pages[i] = dat_internal_ReadPage(inf, inf->Node, qy->Row.StartPageID + i);
		    }

		/** Parse the row contents and release the pages. **/
		inf->RowID = qy->RowID;
		qy->RowID++;
		inf->Row = &(qy->Row);
		inf->Flags |= DAT_F_ROWPRESENT;
		dat_csv_ParseRow(inf, inf->TData);
		inf->Flags |= DAT_F_ROWPARSED;
		for(i=0;i<qy->Row.nPages;i++) dat_internal_UnlockPage(qy->Row.Pages[i]);
		inf->Row = NULL;
		inf->Flags &= DAT_F_ROWPRESENT;

	        /** Get the filename from the primary key of the row. **/
		new_type = DAT_T_ROW;
	        strcpy(filename,dat_internal_KeyToFilename(qy->TableInf,inf));
	        break;

	    case DAT_T_COLSOBJ:
	        /** Loop through the columns in the TableInf structure. **/
		new_type = DAT_T_COLUMN;
		if (qy->RowCnt <= qy->TableInf->nCols)
		    {
		    strcpy(filename,qy->TableInf->Cols[qy->RowCnt-1]);
		    }
		else
		    {
       	            if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
       	            inf->Pathname.OpenCtlBuf = NULL;
		    nmFree(inf,sizeof(DatData));
		    return NULL;
		    }
	        break;

	    default:
		if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
		inf->Pathname.OpenCtlBuf = NULL;
		nmFree(inf,sizeof(DatData));
		mssError(1,"DAT","Bark!  Internal object type not allowed (%d)", qy->ObjInf->Type);
		return NULL;
	    }

	/** Build the filename. **/
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	*(ptr++) = '/';
	strcpy((char*)ptr,filename);
	obj->Pathname->Elements[obj->Pathname->nElements++] = (char*)ptr;
	/*inf = datOpen(obj, mode & (~O_CREAT), NULL, "");*/

	/** Fill out the remainder of the structure. **/
	inf->Obj = obj;
	inf->Mask = 0600;
	inf->Type = new_type;
	inf->Node = qy->ObjInf->Node;
	obj->SubPtr = qy->ObjInf->Obj->SubPtr;
	dat_internal_DetermineType(obj,inf);

    return (void*)inf;
    }


/*** datQueryDelete - delete the contents of a query result set.  This is
 *** not yet supported.
 ***/
int
datQueryDelete(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** datQueryClose - close the query.
 ***/
int
datQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pDatQuery qy = ((pDatQuery)(qy_v));

	/** Free the structure **/
	nmFree(qy,sizeof(DatQuery));

    return 0;
    }


/*** datGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
datGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    int i;
    pDatTableInf tdata;

    	/** Name attribute?  String. **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

    	/** Content-type attribute?  String. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** Annotation?  String. **/
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Attr type depends on object type. **/
	if (inf->Type == DAT_T_ROW || inf->Type == DAT_T_TABLE)
	    {
	    if (inf->Type == DAT_T_ROW) tdata = inf->TData;
	    else tdata = &(inf->Node->HeaderCols);
	    for(i=0;i<tdata->nCols;i++)
	        {
		if (!strcmp(attrname,tdata->Cols[i]))
		    {
		    return tdata->ColTypes[i];
		    }
		}
	    }
	else if (inf->Type == DAT_T_COLUMN)
	    {
	    if (!strcmp(attrname,"datatype")) return DATA_T_STRING;
	    }

	mssError(1,"DAT","Invalid attribute '%s' for GetAttrType", attrname);

    return -1;
    }


/*** datGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
datGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    int i;
    pDatTableInf tdata;
    char* ptr;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DAT","Type mismatch accessing attribute 'name' (must be a string)");
		return -1;
		}
	    if ((inf->Obj->Mode & OBJ_O_AUTONAME) && (inf->Node->Flags & DAT_NODE_F_ROWIDKEY))
		{
		val->String = inf->AutoName;
		}
	    else
		{
		ptr = inf->Pathname.Elements[inf->Pathname.nElements-1];
		if (ptr[0] == '.' && ptr[1] == '\0')
		    {
		    val->String = "/";
		    }
		else
		    {
		    val->String = ptr;
		    }
		}
	    return 0;
	    }

	/** Is it an annotation? **/
	if (!strcmp(attrname, "annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DAT","Type mismatch accessing attribute 'annotation' (must be a string)");
		return -1;
		}
	    /** Different for various objects. **/
	    switch(inf->Type)
	        {
		case DAT_T_TABLE:
		    val->String = inf->TData->Annotation;
		    break;
		case DAT_T_ROWSOBJ:
		    val->String = "Contains rows for this table";
		    break;
		case DAT_T_COLSOBJ:
		    val->String = "Contains columns for this table";
		    break;
		case DAT_T_COLUMN:
		    val->String = "Column within this table";
		    break;
		case DAT_T_ROW:
		    if (!inf->TData->RowAnnotExpr)
		        {
			val->String = "";
			break;
			}
		    expModifyParam(inf->TData->ObjList, NULL, inf->Obj);
		    inf->TData->ObjList->Session = inf->Obj->Session;
		    expEvalTree(inf->TData->RowAnnotExpr, inf->TData->ObjList);
		    if (inf->TData->RowAnnotExpr->Flags & EXPR_F_NULL ||
		        inf->TData->RowAnnotExpr->String == NULL)
			{
			val->String = "";
			}
		    else
		        {
			val->String = inf->TData->RowAnnotExpr->String;
			}
		    break;
		}
	    return 0;
	    }

	/** If Attr is content-type, report the type. **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DAT","Type mismatch accessing attribute 'outer_type' (must be a string)");
		return -1;
		}
	    switch(inf->Type)
	        {
		case DAT_T_TABLE: val->String = "system/table"; break;
		case DAT_T_ROWSOBJ: val->String = "system/table-rows"; break;
		case DAT_T_COLSOBJ: val->String = "system/table-columns"; break;
		case DAT_T_ROW: val->String = "system/row"; break;
		case DAT_T_COLUMN: val->String = "system/column"; break;
		case DAT_T_FILESPEC: val->String = "application/filespec"; break;
		case DAT_T_FILESPECCOL: val->String = "filespec/column"; break;
		}
	    return 0;
	    }
	else if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DAT","Type mismatch accessing attribute '%s' (must be a string)",attrname);
		return -1;
		}
	    switch(inf->Type)
	        {
		case DAT_T_TABLE: val->String = "system/void"; break;
		case DAT_T_ROWSOBJ: val->String = "system/void"; break;
		case DAT_T_COLSOBJ: val->String = "system/void"; break;
		case DAT_T_ROW: val->String = "system/void"; break;
		case DAT_T_COLUMN: val->String = "system/void"; break;
		case DAT_T_FILESPEC: val->String = "system/void"; break;
		case DAT_T_FILESPECCOL: val->String = "system/void"; break;
		}
	    return 0;
	    }

	/** Column object?  Type is the only one. **/
	if (inf->Type == DAT_T_COLUMN)
	    {
	    if (strcmp(attrname,"datatype")) return -1;

	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DAT","Type mismatch accessing attribute 'datatype' (must be a string)");
		return -1;
		}

	    /** Get the table info. **/
	    tdata=inf->TData;
	    /*if (!(inf->Flags & DAT_F_ROWPARSED))
	        {
		inf->Flags |= DAT_F_ROWPARSED;
		dat_csv_ParseRow(inf, inf->TData);
		}*/

	    /** Search table info for this column. **/
	    for(i=0;i<tdata->nCols;i++) if (!strcmp(tdata->Cols[i],inf->RowColPtr))
	        {
		switch(tdata->ColTypes[i])
		    {
		    case DATA_T_INTEGER: val->String = "integer"; break;
		    case DATA_T_STRING: val->String = "string"; break;
		    case DATA_T_DOUBLE: val->String = "double"; break;
		    case DATA_T_DATETIME: val->String = "datetime"; break;
		    case DATA_T_MONEY: val->String = "money"; break;
		    default: val->String = "unknown"; break;
		    }
		return 0;
		}
	    }
	else if (inf->Type == DAT_T_ROW || inf->Type == DAT_T_TABLE)
	    {
	    /** Get the table info. **/
	    if (inf->Type == DAT_T_ROW) tdata = inf->TData;
	    else tdata = &(inf->Node->HeaderCols);

	    /** Search through the columns. **/
	    for(i=0;i<tdata->nCols;i++) if (!strcmp(tdata->Cols[i],attrname))
	        {
		if (inf->ColPtrs[i] == NULL) return 1;
		if (datatype != tdata->ColTypes[i])
		    {
		    mssError(1,"DAT","Type mismatch accessing attribute '%s' [requested=%s, actual=%s]",
			    attrname, obj_type_names[datatype], obj_type_names[tdata->ColTypes[i]]);
		    return -1;
		    }
		switch(tdata->ColTypes[i])
		    {
		    case DATA_T_INTEGER: memcpy(val, inf->ColPtrs[i], 4); break;
		    case DATA_T_DOUBLE: memcpy(val, inf->ColPtrs[i], 8); break;
		    case DATA_T_STRING: val->String = (char*)(inf->ColPtrs[i]); break;
		    case DATA_T_DATETIME: val->DateTime = (pDateTime)(inf->ColPtrs[i]); break;
		    case DATA_T_MONEY: val->Money = (pMoneyType)(inf->ColPtrs[i]); break;
		    }
		return 0;
		}
	    }

	mssError(1,"DAT","Invalid column '%s' for GetAttrValue", attrname);

    return -1;
    }


/*** datGetNextAttr - get the next attribute name for this object.
 ***/
char*
datGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    pDatTableInf tdata;

	/** Attribute listings depend on object type. **/
	switch(inf->Type)
	    {
	    case DAT_T_ROWSOBJ:
	        return NULL;

	    case DAT_T_COLSOBJ:
	        return NULL;

	    case DAT_T_COLUMN:
	        /** only attr is 'datatype' **/
		if (inf->CurAttr++ == 0) return "datatype";
	        break;

	    case DAT_T_TABLE:
	    case DAT_T_ROW:
	        /** Get the table info. **/
		if (inf->Type == DAT_T_TABLE) tdata = &(inf->Node->HeaderCols);
		else tdata = inf->TData;

	        /** Return attr in table inf **/
		if (inf->CurAttr < tdata->nCols) return tdata->Cols[inf->CurAttr++];
	        break;
	    }

    return NULL;
    }


/*** datGetFirstAttr - get the first attribute name for this object.
 ***/
char*
datGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = datGetNextAttr(inf_v,oxt);

    return ptr;
    }


/*** datSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
datSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    unsigned char* ptr;
    unsigned char* srcptr;
    unsigned char* dstptr;
    pStructInf node_inf;
    int type;
    int i,colid,len,curpg;
    pDatRowInfo ri;
    int rval = 0;

        /** Modifying name? **/
	if (!strcmp(attrname, "name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DAT","Type mismatch setting attribute 'name' (must be a string)");
		return -1;
		}
	    /** Kinda complex - not yet implemented. **/
	    return -1;
	    }

    	/** Modifying annotation? **/
	if (!strcmp(attrname, "annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DAT","Type mismatch setting attribute 'annotation' (must be a string)");
		return -1;
		}
	    /** How annotation is modified depends on object type **/
	    switch(inf->Type)
	        {
		case DAT_T_TABLE:
		    ptr = (unsigned char*)nmSysStrdup(val->String);
		    node_inf = stLookup(inf->Node->Node->Data,"annotation");
		    if (!node_inf) node_inf = stAddAttr(inf->Node->Node->Data,"annotation");
		    stSetAttrValue(node_inf, DATA_T_STRING, val, 0);
		    inf->Node->Node->Status = SN_NS_DIRTY;
		    snWriteNode(inf->SpecObj, inf->Node->Node);
		    return 0;

		case DAT_T_ROWSOBJ:
		case DAT_T_COLSOBJ:
		case DAT_T_COLUMN:
		    return -1;

		case DAT_T_ROW:
		    return -1;

		case DAT_T_FILESPEC:
		case DAT_T_FILESPECCOL:
		    return -1;
		}
	    }

	/** Other attr types -- if a ROW, check OXT. **/
	if (inf->Type == DAT_T_ROW)
	    {
	    /** In a transaction? (probably because we're doing INSERT, not UPDATE) **/
	    if (*oxt)
	        {
		/** We're within a transaction.  Fill in the oxt. **/
		type = datGetAttrType(inf_v, attrname, oxt);
		if (type < 0) return -1;
		if (datatype != type)
		    {
		    mssError(1,"DAT","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
			    attrname, obj_type_names[datatype], obj_type_names[type]);
		    return -1;
		    }
		(*oxt)->AllocObj = 0;
		(*oxt)->Object = NULL;
		(*oxt)->Status = OXT_S_VISITED;
		strcpy((*oxt)->AttrName, attrname);
		obj_internal_SetTreeAttr(*oxt, type, val);
		}
	    else
	        {
		/** Find the column **/
		colid = -1;
		for(i=0;i<inf->TData->nCols;i++) if (!strcmp(inf->TData->Cols[i], attrname))
		    {
		    colid = i;
		    break;
		    }
		if (colid == -1)
		    {
		    mssError(1,"DAT","Table column '%s' does not exist",attrname);
		    return -1;
		    }
		type = inf->TData->ColTypes[colid];

		/** Verify the caller is specifying the correct datatype **/
		if (datatype != type)
		    {
		    mssError(1,"DAT","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
			    attrname, obj_type_names[datatype], obj_type_names[type]);
		    return -1;
		    }

		/** Generate a new row. **/
		ptr = dat_csv_GenerateRow(inf, colid, val, NULL);
		if (!ptr) return -1;
		len = strlen((char*)ptr);

		/** Get the row pages from the page cache. **/
		ri = dat_internal_GetRow(inf,inf->Node, inf->RowID);
		if (!ri)
		    {
		    nmFree(ptr, DAT_CACHE_PAGESIZE*(DAT_ROW_MAXPAGESPAN-1));
		    return -1;
		    }

		/** Will the new row fit where the old one was? **/
		if (len <= ri->Length)
		    {
		    /** Yes.  Just overwrite the old row, adding trailing spaces if needed **/
		    srcptr = ptr;
		    dstptr = ri->Pages[0]->Data + ri->Offset;
		    ri->Pages[0]->Flags |= DAT_CACHE_F_DIRTY;
		    curpg=0;
		    while(*srcptr)
		        {
			*(dstptr++) = *(srcptr++);
			if (dstptr >= ri->Pages[curpg]->Data + ri->Pages[curpg]->Length)
			    {
			    curpg++;
			    dstptr = ri->Pages[curpg]->Data;
			    ri->Pages[curpg]->Flags |= DAT_CACHE_F_DIRTY;
			    }
			}
		    while(*dstptr != '\n')
			{
			*(dstptr++) = ' ';
			if (dstptr >= ri->Pages[curpg]->Data + ri->Pages[curpg]->Length)
			    {
			    curpg++;
			    dstptr = ri->Pages[curpg]->Data;
			    ri->Pages[curpg]->Flags |= DAT_CACHE_F_DIRTY;
			    }
			}
		    for(i=1;i<=curpg;i++) dat_internal_UnlockPage(ri->Pages[curpg]);
		    if (dat_internal_FlushPages(inf,ri->Pages[0]) < 0)
			rval = -1;
		    nmFree(ri,sizeof(DatRowInfo));
		    }
		else
		    {
		    /** No.  Delete the old row and add the newly generated one as a new row. **/
		    /** This could make the rowid change.  Bad, but no good way around it. **/
		    dstptr = ri->Pages[0]->Data + ri->Offset;
		    curpg = 0;
		    ri->Pages[0]->Flags |= DAT_CACHE_F_DIRTY;
		    while(*dstptr != '\n')
		        {
			*(dstptr++) = ' ';
			if (dstptr >= ri->Pages[curpg]->Data + ri->Pages[curpg]->Length)
			    {
			    curpg++;
			    dstptr = ri->Pages[curpg]->Data;
			    ri->Pages[curpg]->Flags |= DAT_CACHE_F_DIRTY;
			    }
			}
		    *dstptr = 1;
		    for(i=1;i<=curpg;i++) dat_internal_UnlockPage(ri->Pages[curpg]);
		    if (dat_internal_FlushPages(inf,ri->Pages[0]) < 0)
			rval = -1;
		    nmFree(ri,sizeof(DatRowInfo));

		    /** Insert new row. **/
		    inf->RowID = dat_internal_InsertRow(inf, inf->Node, ptr);
		    }

		/** Release the RAM used by the newly generated row data **/
		nmFree(ptr, DAT_CACHE_PAGESIZE*(DAT_ROW_MAXPAGESPAN-1));
		}
	    }
	else
	    {
	    /** Update attr on non-row?  Error. **/
	    return -1;
	    }

    return rval;
    }


/*** datAddAttr - add an attribute to an object.  Right now, just deny the
 *** request.
 ***/
int
datAddAttr(void* inf_v, char* attrname, int type, pObjData *val, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** datOpenAttr - open an attribute as if it were an object with 
 *** content.  The Sybase database objects don't yet have attributes that are
 *** suitable for this.
 ***/
void*
datOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** datGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
datGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** datGetNextMethod -- same as above.  Always fails. 
 ***/
char*
datGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }

/*** datExecuteMethod - No methods to execute, so this fails.
 ***/
int
datExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    return -1;
    }

/*** datPresentationHints - return a presentation-hints structure
 *** dcontaining information about a particular attribute.
 ***
 *** LME-NOTE:  There must be a better way to do this than two
 ***            nested loops, but this was the only way I could
 ***            find to make it work.
 ***/
pObjPresentationHints
datPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pDatData inf = DAT(inf_v);
    pStructInf stInf = NULL;
    pDatTableInf tdata = NULL;
    int i, j;

    tdata = inf->Node->TableInf;
    for (i=0; i < tdata->nCols; i++)
	{
	if (!strcmp(tdata->Cols[i], attrname))
	    {
	    stInf = stLookup(inf->Node->Node->Data, attrname);
	    for (j=0; j < stInf->nSubInf; j++)
		{
		if (stStructType(stInf->SubInf[j]) == ST_T_SUBGROUP)
		    {
		    if (!strcmp(stInf->SubInf[j]->UsrType, "column/hints"))
			{
			return objInfToHints(stInf->SubInf[j], tdata->ColTypes[i]);
			}
		    }
		}
	    }
	}

    return NULL;
    }

/*** datInfo - Return the capabilities of the object
 ***/
int
datInfo(void* inf_v, pObjectInfo info)
    {
	pDatData inf = DAT(inf_v);

	info->Flags |= (OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CANT_SEEK | 
	    OBJ_INFO_F_CANT_HAVE_CONTENT | OBJ_INFO_F_NO_CONTENT );
	switch (inf->Type)
	    {
	    case DAT_T_TABLE:
	    case DAT_T_COLSOBJ:
		info->Flags |= ( OBJ_INFO_F_HAS_SUBOBJ | OBJ_INFO_F_CAN_HAVE_SUBOBJ );
		break;
	    case DAT_T_ROWSOBJ:
		info->Flags |= ( OBJ_INFO_F_CAN_HAVE_SUBOBJ );
		break;
	    case DAT_T_COLUMN:
	    case DAT_T_ROW:
	    case DAT_T_FILESPEC:
	    case DAT_T_FILESPECCOL:
		info->Flags |= ( OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ );
		break;
	    }
	return 0;
    }

/*** datInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
datInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&DAT_INF,0,sizeof(DAT_INF));
	xhInit(&DAT_INF.DBNodes, 255, 0);
	xhInit(&DAT_INF.PagesByID, DAT_CACHE_MAXPAGES*2-1, DAT_CACHE_KEYSIZE);
	xhInit(&DAT_INF.TypesByExt, 63, 0);
	DAT_INF.AccessCnt = 1;
	DAT_INF.AllocPages = 0;
	DAT_INF.PageList.Next = &DAT_INF.PageList;
	DAT_INF.PageList.Prev = &DAT_INF.PageList;
	DAT_INF.PageList.Flags = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"DAT - Flat Datafile Driver");
	/*drv->Capabilities = 0;*/
	drv->Capabilities = OBJDRV_C_TRANS;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/datafile");
	xaAddItem(&(drv->RootContentTypes),"text/x-fixed-field");
	xaAddItem(&(drv->RootContentTypes),"application/filespec");

	/** Setup the function references. **/
	drv->Open = datOpen;
	drv->Close = datClose;
	drv->Create = datCreate;
	drv->Delete = datDelete;
	drv->DeleteObj = datDeleteObj;
	drv->OpenQuery = datOpenQuery;
	drv->QueryDelete = datQueryDelete;
	drv->QueryFetch = datQueryFetch;
	drv->QueryClose = datQueryClose;
	drv->Read = datRead;
	drv->Write = datWrite;
	drv->GetAttrType = datGetAttrType;
	drv->GetAttrValue = datGetAttrValue;
	drv->GetFirstAttr = datGetFirstAttr;
	drv->GetNextAttr = datGetNextAttr;
	drv->SetAttrValue = datSetAttrValue;
	drv->AddAttr = datAddAttr;
	drv->OpenAttr = datOpenAttr;
	drv->GetFirstMethod = datGetFirstMethod;
	drv->GetNextMethod = datGetNextMethod;
	drv->ExecuteMethod = datExecuteMethod;
	drv->PresentationHints = datPresentationHints;
	drv->Commit = datCommit;
	drv->Info = datInfo;

	nmRegister(sizeof(DatTableInf),"DatTableInf");
	nmRegister(sizeof(DatData),"DatData");
	nmRegister(sizeof(DatQuery),"DatQuery");
	nmRegister(sizeof(DatNode),"DatNode");
	nmRegister(sizeof(DatPage),"DatPage");

	/** Add file type extensions for flat datafiles **/
	xhAdd(&DAT_INF.TypesByExt, "CSV", (void*)DAT_NODE_T_CSV);
	xhAdd(&DAT_INF.TypesByExt, "BCP", (void*)DAT_NODE_T_BCP);
	xhAdd(&DAT_INF.TypesByExt, "SYB", (void*)DAT_NODE_T_BCP);
	xhAdd(&DAT_INF.TypesByExt, "FIXED", (void*)DAT_NODE_T_FIXED);

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }
