#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "xhashqueue.h"
#include "mtsession.h"
#include "stparse.h"
#include "expression.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_indexfile.c         				*/
/* Author:	Greg Beeley (GRB)      					*/
/* Creation:	January 25th, 2000     					*/
/* Description:	This driver provides for a highly speed-efficient	*/
/*		indexed file mechanism based on a B+-tree indexing	*/
/*		approach.  Some tradeoffs have been made that require	*/
/*		the utilization of additional space, that may be relaxed*/
/*		as this module becomes more mature.  			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_indexfile.c,v 1.1 2001/08/13 18:01:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_indexfile.c,v $

    $Log: objdrv_indexfile.c,v $
    Revision 1.1  2001/08/13 18:01:02  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:04  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Global controls... ***/
#define IDX_PAGESIZE		8192
#define IDX_MAXFILESIZE		(256*256*256*127)
#define IDX_MAXFILEPAGES	(IDX_MAXFILESIZE/IDX_PAGESIZE)	/* 260096 */
#define IDX_PAGESPERGROUP	255
#define IDX_PAGESPERGROUPHDR	1
#define IDX_MAXGROUPS		(IDX_MAXFILEPAGES/(IDX_PAGESPERGROUP+IDX_PAGESPERGROUPHDR)) /* 1016 max */
#define IDX_CACHE_MAXPAGES	64
#define IDX_PAGEHEADERSIZE	36
#define IDX_MAXROWLENGTH	(IDX_PAGESIZE - 48)
#define IDX_MAXHEADERPAGES	16

#define IDX_OFFSET_PAGEID	0
#define IDX_OFFSET_MODID	4
#define IDX_OFFSET_FREESPACE	12
#define IDX_OFFSET_ROWCNT	16
#define IDX_OFFSET_PTRCNT	20
#define IDX_OFFSET_PARENTPTR	24
#define IDX_OFFSET_NEXTPTR	28
#define IDX_OFFSET_PREVPTR	32
#define IDX_OFFSET_PTRS		36

#define IDX_ROWHEADERSIZE	8
#define IDX_COLHEADERSIZE	3
#define IDX_MAXSTRING		(IDX_MAXROWLENGTH - IDX_ROWHEADERSIZE - IDX_COLHEADERSIZE)
#define IDX_MAXCOLUMNS		256
#define IDX_MAXKEYS		8

#define IDX_PAGE_UNALLOCATED	0xFF
#define IDX_PAGE_FREE		0xFE
#define IDX_PAGE_ROOTNODE	0x00
#define IDX_PAGE_HEADER		0x01
#define IDX_PAGE_INDEX		0x10
#define IDX_PAGE_DATA		0x20
#define IDX_PAGE_BLOB		0x30
#define IDX_PAGE_TRANSLOG	0x40

#define IDX_PAGE_ROWOFFSET(p,x) (*(unsigned int*)((p)->Data.Raw + (p)->Data.Info.PtrCount*4 + IDX_OFFSET_PTRS + (x)*4))
#define IDX_PAGE_POINTER(p,x) (*(unsigned int*)((p)->Data.Raw + IDX_OFFSET_PTRS + (x)*4))


/*** Attribute listing for table itself ***/
static char* idx_tableattr[] =
    {
    "row_annot_expr",
    NULL,
    };
static int idx_tabletype[] =
    {
    DATA_T_STRING,
    0,
    };

/*** Attribute listing on a column ***/
static char* idx_colattr[] =
    {
    "datatype",
    "style",
    "length",
    "lengthdivisor",
    "key_id",
    "group_id",
    "groupname",
    "default",
    "constraint",
    "minimum",
    "maximum",
    "enumquery",
    "format",
    "description",
    NULL,
    };
static int idx_coltype[] =
    {
    DATA_T_STRING,
    DATA_T_INTEGER,
    DATA_T_INTEGER,
    DATA_T_INTEGER,
    DATA_T_INTEGER,
    DATA_T_INTEGER,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    0,
    };

/*** Structure describing allocation group summaries ***/
typedef struct
    {
    unsigned int	HdrPage;
    unsigned int	FreePages;
    }
    IdxAllocSummary, *pIdxAllocSummary;


/*** Structures for handling information within the headers ***/
typedef struct
    {
    unsigned int	DescriptorLength;
    unsigned int	Flags;
    unsigned int	Length;
    unsigned int	LengthDivisor;
    unsigned int	Style;
    unsigned char	Type;
    unsigned char	KeyID;
    unsigned char	GroupID;
    }
    ColInf, *pColInf;

typedef struct
    {
    unsigned int	HdrMagic;
    unsigned int	HdrByteCnt;
    unsigned int	HdrPageList[IDX_MAXHEADERPAGES];
    }
    HdrInf1, *pHdrInf1;

typedef struct
    {
    unsigned int	TableWideFlags;
    unsigned int	RootPageID;
    unsigned int	FirstPageID;
    unsigned int	LastPageID;
    unsigned int	GlobalModID[2];
    unsigned int	IdentityID[2];
    unsigned short	KeyLength;
    unsigned short	DataLength;
    IdxAllocSummary	AllocGroups[IDX_MAXGROUPS];
    unsigned char	nColumns;
    }
    hDRiNF2, *pHdrInf2;

/*** Structure containing data on a column. ***/
typedef struct
    {
    unsigned char*	RawData;
    pColInf		ColInf;
    unsigned char*	Name;
    unsigned char*	Constraint;
    unsigned char*	Default;
    unsigned char*	Minimum;
    unsigned char*	Maximum;
    unsigned char*	EnumQuery;
    unsigned char*	Format;
    unsigned char*	GroupName;
    unsigned char*	Description;
    pExpression		ConstraintExpr;
    pExpression		DefaultExpr;
    pExpression		MinimumExpr;
    pExpression		MaximumExpr;
    }
    IdxColData, *pIdxColData;


/*** Structure for dealing with allocation group headers ***/
typedef struct
    {
    unsigned int	TotalPages;
    unsigned int	FreePages;
    unsigned char	AllocationMap[IDX_PAGESPERGROUP];
    }
    IdxAllocGroup, *pIdxAllocGroup;


/*** Structure containing information on a given file. ***/
typedef struct
    {
    char		Pathname[256];
    unsigned char*	RawHeader;
    int			HdrAllocSize;
    int			OrigByteCnt;
    pHdrInf1		HdrInf1;
    pHdrInf2		HdrInf2;
    unsigned char*	Name;
    unsigned char*	Annotation;
    unsigned char*	RowAnnotString;
    pIdxColData		Columns[IDX_MAXCOLUMNS];
    int			nKeys;
    int			Keys[IDX_MAXKEYS];
    pExpression		RowAnnotExpr;
    pObject		NodeRef;
    pSemaphore		Lock;
    }
    IdxTableData, *pIdxTableData;


/*** These values are correct for i386 Linux gcc-2.7 ***/
typedef int		IdxFileInt;
typedef char*		IdxFilePtr;
typedef unsigned int 	IdxFileUInt;
typedef unsigned char 	IdxFileUChar;


/*** Structure representing a piece of metadata. ***/
typedef struct
    {
    union
        {
	struct
	    {
    	    IdxFileInt	DataID;
    	    IdxFileInt	Type;
            IdxFilePtr	UserValue;
	    }
	    Data;
	}
	Info;
    union
        {
        struct
            {
	    IdxFileInt	Length;
	    IdxFilePtr	Value;
	    }
	    String;
        struct
            {
	    IdxFileInt	Value;
	    }
	    Integer;
        struct
            {
	    IdxFileInt	MSW;
	    IdxFileUInt LSW;
	    }
	    Int64;
	}
	Data;
    }
    IdxMetaDataElement, *pIdxMetaDataElement;


/*** Codes for metadata attributes on the table and on columns ***/
typedef enum
    {
    IdxType_Name = 0,		/* Name of this table */
    IdxType_Annotation = 1,	/* Annotation of the table */
    IdxType_RowAnnotation = 2,	/* Annotation expression for rows */
    IdxType_ColID = 3,		/* ID of column (0...n) */
    IdxType_ColFlags = 4,	/* Column flags */
    IdxType_ColLength = 5,	/* Length (e.g. string maxlen) */
    IdxType_ColLength2 = 6,	/* e.g., # lines for multiline edit */
    IdxType_ColStyle = 7,	/* style bitmask for column */
    IdxType_ColType = 8,	/* DATA_T_xxx for column */
    IdxType_ColKeyID = 9,	/* 0=not key, 1-8 = primary key pos. */
    IdxType_ColGroupID = 10,	/* attribute group id */
    IdxType_ColName = 11,	/* column name */
    IdxType_ColConstraint = 12,	/* constraint expression */
    IdxType_ColDefault = 13,	/* default expression */
    IdxType_ColMinimum = 14,	/* minimum value expression */
    IdxType_ColMaximum = 15,	/* maximum value expression */
    IdxType_ColCodelist = 16,	/* codelist enumeration query */
    IdxType_ColFormat = 17,	/* presentation format */
    IdxType_ColGroupName = 18,	/* name of group of columns */
    IdxType_ColDescription = 19, /* description of column */
    }
    IdxMetaDataType;


/*** Names and types of metadata attributes.
 *** While it is not necessary to include the idxtype id in the data of this
 *** table, we do it just for clarity, so its obvious which row is which.  It
 *** may be advantageous in the Initialize() function to check the integrity
 *** of the ids vs. positions in the array for this table.
 ***/
struct { IdxMetaDataType id; int datatype; int is_table; char* name; } attrinfo[] =
    {
    /** Table metadata **/
    { IdxType_Name,		DATA_T_STRING,	1,	"name" 		},
    { IdxType_Annotation,	DATA_T_STRING,	1,	"annotation"	},
    { IdxType_RowAnnotation,	DATA_T_STRING,	1,	"rowannotexpr"	},

    /** Column metadata **/
    { IdxType_ColID,		DATA_T_INTEGER,	0,	"id"		},
    { IdxType_ColFlags,		DATA_T_INTEGER,	0,	"flags"		},
    { IdxType_ColLength,	DATA_T_INTEGER,	0,	"length"	},
    { IdxType_ColLength2,	DATA_T_INTEGER,	0,	"length2"	},
    { IdxType_ColStyle,		DATA_T_INTEGER,	0,	"style"		},
    { IdxType_ColType,		DATA_T_INTEGER,	0,	"datatype"	},
    { IdxType_ColKeyID,		DATA_T_INTEGER,	0,	"keyid"		},
    { IdxType_ColGroupID,	DATA_T_INTEGER,	0,	"groupid"	},
    { IdxType_ColName,		DATA_T_STRING,	0,	"name"		},
    { IdxType_ColConstraint,	DATA_T_STRING,	0,	"constraint"	},
    { IdxType_ColDefault,	DATA_T_STRING,	0,	"default"	},
    { IdxType_ColMinimum,	DATA_T_STRING,	0,	"minimum"	},
    { IdxType_ColMaximum,	DATA_T_STRING,	0,	"maximum"	},
    { IdxType_ColCodelist,	DATA_T_STRING,	0,	"enumquery"	},
    { IdxType_ColFormat,	DATA_T_STRING,	0,	"format"	},
    { IdxType_ColGroupName,	DATA_T_STRING,	0,	"groupname"	},
    { IdxType_ColDescription,	DATA_T_STRING,	0,	"description"	},
    };


/*** Structure for a page of data ***/
typedef struct
    {
    pIdxTableData	TData;
    unsigned int	PageID;
    unsigned int	Flags;
    unsigned int	Type;
    pSemaphore		Lock;
    pXHQElement		Xe;
    union
        {
        unsigned char	Raw[IDX_PAGESIZE];
	struct
	    {
	    unsigned int	PageID;
	    unsigned int	PageModID[2];
	    unsigned int	FreeSpace;
	    unsigned int	RowCount;
	    unsigned int	PtrCount;
	    unsigned int	Parent;
	    unsigned int	PrevPage;
	    unsigned int	NextPage;
	    unsigned char	DataArea[IDX_PAGESIZE - IDX_PAGEHEADERSIZE];
	    }
	    Info;
	}
	Data;
    }
    IdxPage, *pIdxPage;

#define IDX_PAGE_F_DIRTY	1


/*** Structure used to reference a row. ***/
typedef struct
    {
    pIdxPage		Page;
    unsigned int	RowID;
    }
    IdxRowRef, *pIdxRowRef;


/*** Structure used by this driver internally, per open... ***/
typedef struct 
    {
    char		Pathname[256];
    int			Flags;
    pObject		Obj;
    char*		TableNamePtr;	/* "/tablename" */
    char*		RowColObjPtr;	/* "/tablename/rows" */
    char*		RowColPtr;	/* "/tablename/rows/1002" */
    int			Mask;
    int			CurAttr;
    int			Type;
    pIdxTableData	TData;
    }
    IdxData, *pIdxData;

#define IDX_T_DATAFILE	0
#define IDX_T_ROWSOBJ	1
#define IDX_T_COLSOBJ	2
#define IDX_T_ROW	3
#define IDX_T_COLUMN	4


#define IDX(x) ((pIdxData)(x))


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pIdxData		Data;
    char		NameBuf[256];
    int			ItemCnt;
    }
    IdxQuery, *pIdxQuery;


/*** GLOBALS ***/
struct
    {
    pIdxPage		Pages[IDX_CACHE_MAXPAGES];
    int			nAlloc;
    XHashQueue		PageCache;
    XHashTable		TableData;
    }
    IDX_INF;


/*** A few internal functions used in this module ***/
/*** Table Header management ***/
pIdxTableData 	idx_internal_GetTData(pObject obj);
int 		idx_internal_WriteTData(pObject obj, pIdxTableData tdata);
int 		idx_internal_UpdateTDataPtrs(pIdxTableData tdata, unsigned char* newptr);
int 		idx_internal_SpaceTData(pIdxTableData tdata, int start_loc, int delta);
pIdxTableData 	idx_internal_NewTData(pIdxData inf);

/*** Page Cache management ***/
int 		idx_internal_DiscardPage(pXHashQueue xhq, pXHQElement xe, int locked);
int 		idx_internal_FlushCache(pIdxTableData tdata);
pIdxPage 	idx_internal_GetPage(pIdxTableData tdata, int type, int req_pageid);
int 		idx_internal_ReleasePage(pIdxPage page);

/*** Page Allocation management ***/
pIdxPage 	idx_internal_AllocPage(pIdxTableData tdata, int type, int target);
int 		idx_internal_FreePage(pIdxPage page);



/*** idx_internal_GetTData - read the node object to obtain the structural
 *** table data, including column information.  If table data has already 
 *** been read, it can be consulted via the TableData hashtable in the 
 *** globals structure.  last_modification should be consulted from the
 *** node object to determine whether to discard the info in the hash
 *** table.
 ***/
pIdxTableData
idx_internal_GetTData(pObject obj)
    {
    pIdxTableData tdata;
    pIdxColData col;
    pIdxAllocSummary grp;
    char* path = obj_internal_PathPart(obj->Prev->Pathname, 0, obj->Prev->SubPtr + obj->Prev->SubCnt - 1);
    int bytecnt,pagecnt,pagesread,offset,i;
    unsigned int *intptr;
    unsigned char* prevptr;

    	/** First check the cache hashtable. **/
	tdata = (pIdxTableData)xhLookup(&IDX_INF.TableData, path);
	if (tdata) return tdata;

	/** If not cached, try to read it. **/
	tdata = (pIdxTableData)nmMalloc(sizeof(IdxTableData));
	if (!tdata) return NULL;
	strcpy(tdata->Pathname, path);
	tdata->RawHeader = (unsigned char*)nmSysMalloc(IDX_PAGESIZE);
	if (objRead(obj->Prev, tdata->RawHeader, IDX_PAGESIZE, OBJ_U_PACKET | OBJ_U_SEEK, 0) != IDX_PAGESIZE)
	    {
	    mssError(0,"IDX","Could not read indexed file header");
	    nmSysFree(tdata->RawHeader);
	    nmFree(tdata,sizeof(IdxTableData));
	    return NULL;
	    }

	/** Verify the magic number **/
	if (0x7c543219 != ((int*)(tdata->RawHeader))[0])
	    {
	    mssError(1,"IDX","Not an indexed file or file is corrupt");
	    nmSysFree(tdata->RawHeader);
	    nmFree(tdata,sizeof(IdxTableData));
	    return NULL;
	    }

	/** Read the byte count and then read the _whole_ header **/
	bytecnt = ((int*)(tdata->RawHeader))[1];
	if (bytecnt > IDX_PAGESIZE*IDX_MAXHEADERPAGES)
	    {
	    mssError(1,"IDX","Header size appears to be > %dK; refusing to read it",IDX_PAGESIZE*IDX_MAXHEADERPAGES/1024);
	    nmSysFree(tdata->RawHeader);
	    nmFree(tdata,sizeof(IdxTableData));
	    return NULL;
	    }
	pagecnt = (bytecnt + IDX_PAGESIZE - 1)/IDX_PAGESIZE;
	pagesread = 1;
	tdata->RawHeader = (unsigned char*)nmSysRealloc(tdata->RawHeader, pagecnt*IDX_PAGESIZE);
	tdata->OrigByteCnt = bytecnt;
	while(pagesread < pagecnt)
	    {
	    offset = ((int*)(tdata->RawHeader))[2+pagesread]*IDX_PAGESIZE;
	    if (objRead(obj->Prev, tdata->RawHeader + pagesread*IDX_PAGESIZE, IDX_PAGESIZE, OBJ_U_PACKET | OBJ_U_SEEK, offset) != IDX_PAGESIZE)
	        {
	        mssError(0,"IDX","Could complete reading of indexed file header, offset %d",offset);
	        nmSysFree(tdata->RawHeader);
	        nmFree(tdata,sizeof(IdxTableData));
	        return NULL;
		}
	    pagesread++;
	    }

	/** Ok, good, got the whole file header.  Now fill in the TData structure. **/
	tdata->HdrAllocSize = pagecnt*IDX_PAGESIZE;
	tdata->HdrInf1 = (pHdrInf1)(tdata->RawHeader);
	tdata->HdrInf2 = (pHdrInf2)(tdata->RawHeader + sizeof(HdrInf1) + (IDX_MAXHEADERPAGES-1)*sizeof(unsigned int));
	tdata->Name = ((char*)(tdata->HdrInf2))+sizeof(HdrInf2);
	tdata->Annotation = strchr(tdata->Name,0)+1;
	tdata->RowAnnotString = strchr(tdata->Annotation,0)+1;

	/** Read the column-specific data **/
	prevptr = strchr(tdata->RowAnnotString,0)+1;
	for(i=0;i<tdata->HdrInf2->nColumns;i++)
	    {
	    col = tdata->Columns[i] = (pIdxColData)nmMalloc(sizeof(IdxColData));
	    col->RawData = (unsigned char*)(((unsigned int)(prevptr+3))&0xFFFFFFFC);
	    col->ColInf = (pColInf)(col->RawData);
	    col->Name = (char*)(col->RawData + sizeof(ColInf));
	    col->Constraint = strchr(col->Name,0)+1;
	    col->Default = strchr(col->Constraint,0)+1;
	    col->Minimum = strchr(col->Default,0)+1;
	    col->Maximum = strchr(col->Minimum,0)+1;
	    col->EnumQuery = strchr(col->Maximum,0)+1;
	    col->Format = strchr(col->EnumQuery,0)+1;
	    col->GroupName = strchr(col->Format,0)+1;
	    col->Description = strchr(col->GroupName,0)+1;
	    prevptr = col->RawData + col->ColInf->DescriptorLength;
	    }

	/** Ok, table descriptor loaded.  Now cache it. **/
	xhAdd(&IDX_INF.TableData, tdata->Pathname, (void*)tdata);

    return tdata;
    }


/*** idx_internal_WriteTData - write the table data header out to the
 *** datafile.
 ***/
int
idx_internal_WriteTData(pObject obj, pIdxTableData tdata)
    {
    int i,offset;
    int pagecnt;

    	/** Write each page. **/
	pagecnt = (tdata->HdrInf1->HdrByteCnt + IDX_PAGESIZE - 1)/IDX_PAGESIZE;
	for(i=0;i<pagecnt;i++)
	    {
	    offset = tdata->HdrInf1->HdrPageList[i] * IDX_PAGESIZE;
	    if (objWrite(obj->Prev, tdata->RawHeader + i*IDX_PAGESIZE, IDX_PAGESIZE, OBJ_U_PACKET | OBJ_U_SEEK, offset) != IDX_PAGESIZE)
	        {
		mssError(0,"IDX","Failed to save table metadata!");
		return -1;
		}
	    }

    return 0;
    }


/*** idx_internal_UpdateTDataPtrs - update the pointers in the TData
 *** structure if the memory allocation block gets altered because of a
 *** realloc() operation.
 ***/
int
idx_internal_UpdateTDataPtrs(pIdxTableData tdata, unsigned char* newptr)
    {
    int delta,i;
    pIdxColData col;

    	/** Compute delta between new and old memory regions. **/
	delta = newptr - tdata->RawHeader;
	if (delta == 0) return 0;

	/** Update base table data pointers **/
	tdata->RawHeader += delta;
	tdata->HdrInf1 = (pHdrInf1)(((unsigned char*)(tdata->HdrInf1))+delta);
	tdata->HdrInf2 = (pHdrInf2)(((unsigned char*)(tdata->HdrInf2))+delta);
	tdata->Name += delta;
	tdata->Annotation += delta;
	tdata->RowAnnotString += delta;

	/** Update column info.  Alignment won't be a problem. **/
	for(i=0;i<tdata->HdrInf2->nColumns;i++)
	    {
	    col = tdata->Columns[i];
	    col->ColInf = (pColInf)(((unsigned char*)(col->ColInf))+delta);
	    col->Name += delta;
	    col->Constraint += delta;
	    col->Default += delta;
	    col->Minimum += delta;
	    col->Maximum += delta;
	    col->EnumQuery += delta;
	    col->Format += delta;
	    col->GroupName += delta;
	    col->Description += delta;
	    }

    return 0;
    }


/*** idx_internal_SpaceTData - this routine re-spaces the TData structure
 *** information when size-changing modifications occur, such as changes
 *** to string values or changes in the number of column data structures
 *** present in the header.  A routine needing to modify the header data
 *** in such a way that spacing changes should call this function before 
 *** it modifies the data.  The calling routine should also lock the
 *** header before calling this function.
 ***/
int
idx_internal_SpaceTData(pIdxTableData tdata, int start_loc, int delta)
    {
    int newsize,newpagecnt,oldpagecnt,i,oldsize;
    unsigned char* newptr;
    pIdxPage pg;
    unsigned char* ckptr;
    unsigned char* ptr;
    int col_delta,n_cols;
    int col_len[IDX_MAXCOLUMNS];

	/** Whew, I wish all cases were as easy as delta==0 :) **/
	if (delta == 0) return 0;

    	/** First, determine if we have enough memory allocated **/
	if (tdata->HdrInf1->HdrByteCnt + delta > tdata->HdrAllocSize)
	    {
	    newpagecnt = (tdata->HdrInf1->HdrByteCnt + delta + IDX_PAGESIZE - 1)/IDX_PAGESIZE;
	    if (newpagecnt > IDX_MAXHEADERPAGES)
	        {
		mssError(1,"IDX","Internal error - max space for file header exceeded");
		return -1;
		}
	    oldpagecnt = tdata->HdrAllocSize / IDX_PAGESIZE;
	    newsize = newpagecnt * IDX_PAGESIZE;
	    newptr = (unsigned char*)nmSysRealloc(tdata->RawHeader, newsize);
	    idx_internal_UpdateTDataPtrs(tdata, newptr);
	    tdata->RawHeader = newptr;
	    tdata->HdrAllocSize = newsize;
	    for(i=oldpagecnt;i<newpagecnt;i++)
	        {
		/** Alloc as many new pages as we need, but the data won't be **/
		/** written via page I/O, but rather with direct writes, so just **/
		/** grab the new page id number. **/
	        pg = idx_internal_AllocPage(tdata,IDX_PAGE_HEADER,0);
		tdata->HdrInf1->HdrPageList[i] = pg->PageID;
		pg->Flags &= ~IDX_PAGE_F_DIRTY;
		idx_internal_ReleasePage(pg);
		}
	    }
	n_cols = tdata->HdrInf2->nColumns;

	for(i=0;i<n_cols;i++) col_len[i] = tdata->Columns[i]->DescriptorLength;

	/** Move the data, as needed, to make room. **/
	memmove(tdata->RawHeader+start_loc+delta, tdata->RawHeader+start_loc, tdata->HdrByteCnt - start_loc);

	/** Scan all ptrs for those that are past the start_loc offset **/
	ckptr = tdata->RawHeader + start_loc;
	if (tdata->Name > ckptr) tdata->Name += delta;
	if (tdata->Annotation > ckptr) tdata->Annotation += delta;
	ptr = strchr(tdata->RowAnnotString,0)+1;
	if (tdata->RowAnnotString > ckptr) tdata->RowAnnotString += delta;
	if ((unsigned char*)(tdata->HdrInf1) > ckptr) tdata->HdrInf1 = (pHdrInf1)(((unsigned char*)(tdata->HdrInf1))+delta);
	if ((unsigned char*)(tdata->HdrInf2) > ckptr) tdata->HdrInf2 = (pHdrInf2)(((unsigned char*)(tdata->HdrInf2))+delta);

	/** Special treatment for column data structures to 32bit-align them. **/
	if (n_cols > 0)
	    {
	    if (ckptr < (unsigned char*)(tdata->Columns[0]->RawData))
	        {
		ptr += delta;
		newptr = (unsigned char*)(((unsigned int)(ptr+3))&0xFFFFFFFC);
		col_delta = newptr - (unsigned char*)(tdata->Columns[0]->RawData);
		}
	    }
	for(i=0;i<n_cols;i++)
	    {
	    /** Expansion occurred within this col or before this col? **/
	    if (tdata->Columns[i]->RawData + col_len[i] > ckptr && tdata->Columns[i]->RawData <= ckptr)
	        {
		if (tdata->Columns[i]->Name > ckptr) tdata->Columns[i]->Name += delta;
		if (tdata->Columns[i]->Constraint > ckptr) tdata->Columns[i]->Constraint += delta;
		if (tdata->Columns[i]->Default > ckptr) tdata->Columns[i]->Default += delta;
		if (tdata->Columns[i]->Minimum > ckptr) tdata->Columns[i]->Minimum += delta;
		if (tdata->Columns[i]->Maximum > ckptr) tdata->Columns[i]->Maximum += delta;
		if (tdata->Columns[i]->EnumQuery > ckptr) tdata->Columns[i]->EnumQuery += delta;
		if (tdata->Columns[i]->Format > ckptr) tdata->Columns[i]->Format += delta;
		if (tdata->Columns[i]->GroupName > ckptr) tdata->Columns[i]->GroupName += delta;
		ptr = strchr(tdata->Columns[i]->Description,0)+1;
		if (tdata->Columns[i]->Description > ckptr) tdata->Columns[i]->Description += delta;
		if (i < n_cols-1)
		    {
		    ptr += delta;
		    newptr = (unsigned char*)(((unsigned int)(ptr+3))&0xFFFFFFFC);
		    col_delta = newptr - (unsigned char*)(tdata->Columns[0]->RawData);
		    }
		}
	    else if (tdata->Columns[i]->RawData + col_len[i] > ckptr)
	        {
		tdata->Columns[i]->RawData += col_delta;
		tdata->Columns[i]->ColInf = (pColInf)(((unsigned char*)(tdata->ColInf))+col_delta);
		tdata->Columns[i]->Name += col_delta;
		tdata->Columns[i]->Constraint += col_delta;
		tdata->Columns[i]->Default += col_delta;
		tdata->Columns[i]->Minimum += col_delta;
		tdata->Columns[i]->Maximum += col_delta;
		tdata->Columns[i]->EnumQuery += col_delta;
		tdata->Columns[i]->Format += col_delta;
		tdata->Columns[i]->GroupName += col_delta;
		tdata->Columns[i]->Description += col_delta;
		if (delta != col_delta)
		    {
		    memmove(tdata->Columns[i]->RawData, tdata->Columns[i]->RawData + (delta - col_delta), col_len[i]);
		    }
		}
	    }

    return 0;
    }


/*** idx_internal_NewTData - creates a new table data structure, and writes the
 *** table header, first alloc group, and rootnode, to the node object.
 ***/
pIdxTableData
idx_internal_NewTData(pIdxData inf)
    {
    pIdxTableData tdata;
    unsigned char* endptr;
    pIdxPage pg;

    	/** Allocate the tdata and an initial buffer **/
	tdata = (pIdxTableData)nmMalloc(sizeof(IdxTableData));
	tdata->RawHeader = (unsigned char*)nmSysMalloc(IDX_PAGESIZE);
	memset(tdata->RawHeader, 0, IDX_PAGESIZE);
	tdata->HdrAllocSize = IDX_PAGESIZE;

    	/** Set the pathname **/
	strcpy(tdata->Pathname, obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->SubPtr));
	
	/** Setup the basic parameter data. **/
	tdata->HdrInf1 = (pHdrInf1)(tdata->RawHeader);
	tdata->HdrInf2 = (pHdrInf2)(tdata->RawHeader + sizeof(HdrInf1));
	tdata->HdrInf1->HdrMagic = 0x7c543219;
	tdata->HdrInf1->HdrPageList[0] = 0;
	tdata->HdrInf2->TableWideFlags = 0;
	tdata->HdrInf2->GlobalModID[0] = 0;
	tdata->HdrInf2->GlobalModID[1] = 0;
	tdata->HdrInf2->IdentityID[0] = 0;
	tdata->HdrInf2->IdentityID[1] = 0;
	tdata->HdrInf2->KeyLength = 0;
	tdata->HdrInf2->DataLength = 0;
	tdata->HdrInf2->nColumns = 0;

	/** Set name, annotation, and row annotation string **/
	endptr = tdata->RawHeader + sizeof(HdrInf1) + sizeof(HdrInf2);
	tdata->Name = endptr;
	*(endptr++) = '\0';
	tdata->Annotation = endptr;
	*(endptr++) = '\0';
	tdata->RowAnnotString = endptr;
	*(endptr++) = '\0';

	/** Set sizing information, align padding 4*(1+ncols) **/
	tdata->OrigByteCnt = (endptr - tdata->RawHeader) + 4;
	tdata->HdrInf1->HdrByteCnt = tdata->OrigByteCnt;

	/** Write the header block. **/
	objWrite(inf->Obj->Prev, tdata->RawHeader, tdata->HdrAllocSize, OBJ_U_SEEK, 0);

	/** Ok, now allocate the root page.  This will also cause grp allocation **/
	pg = idx_internal_AllocPage(tdata, IDX_PAGE_ROOT, 0);
	if (!pg)
	    {
	    mssError(0,"IDX","Could not allocate root page");
	    nmSysFree(tdata->RawHeader);
	    nmFree(tdata,sizeof(IdxTableData));
	    return NULL;
	    }

	/** Got the root page -- point to it **/
	tdata->HdrInf2->RootPageID = pg->PageID;
	tdata->HdrInf2->LastPageID = pg->PageID;
	tdata->HdrInf2->FirstPageID = pg->PageID;

	/** Fill in the root page initial data. **/
	pg->Data.Info.PageID = pg->PageID;
	pg->Data.Info.PageModID[0] = 0;
	pg->Data.Info.PageModID[1] = 0;
	pg->Data.Info.RowCount = 0;
	pg->Data.Info.PtrCount = 0;
	pg->Data.Info.Parent = 0;
	pg->Data.Info.PrevPage = 0;
	pg->Data.Info.NextPage = 0;
	pg->Data.Info.FreeSpace = IDX_MAXROWLENGTH;

	/** Release the page in the cache **/
	idx_internal_ReleasePage(pg);

	/** Flush the cache for this tdata.  This will write the root tree node and **/
	/** the first allocation group page. **/
	idx_internal_FlushCache(tdata);

    return tdata;
    }


/*** idx_internal_DiscardPage - this is a callback from the XHQ module that
 *** is activated when a page is being LRU-discarded from the page cache.
 *** Here, we need to make sure the page is free, and then deallocate the
 *** thing.
 ***/
int
idx_internal_DiscardPage(pXHashQueue xhq, pXHQElement xe, int locked)
    {
    pIdxPage pg;
    int wcnt;
    
        if (locked) return -1;
        if (syGetSem(pg->Lock, 1, SEM_U_NOBLOCK) < 0) return -1;
	if (pg->Flags & IDX_PAGE_F_DIRTY)
	    {
	    wcnt = objWrite(pg->TData->NodeRef, pg->Data.Raw, IDX_PAGESIZE, OBJ_U_SEEK, pg->PageID*IDX_PAGESIZE);
	    if (wcnt < 0)
	        {
		mssError(0,"IDX","Could not write page %d", pg->PageID);
		syPostSem(pg->Lock, 1, 0);
		return -1;
		}
	    }
	syDestroySem(pg->Lock, 0);
	nmFree(pg, sizeof(IdxPage));

    return 0;
    }


/*** idx_internal_FlushCache - write any 'dirty' pages back to the node
 *** object's content, and mark the pages 'clean'.  If a TData is specified,
 *** then only object matching that tdata will be written.  Otherwise,
 *** TData should be null.
 ***/
int
idx_internal_FlushCache(pIdxTableData tdata)
    {
    pXHQElement xe;
    XArray sorted_pages;
    pIdxTableData cur_tdata = NULL;
    pIdxPage pg,search_pg;
    int i,err=0;

    	/** First, lock the page cache so we can safely scan it. **/
	xhqLock(&IDX_INF.PageCache);
	xaInit(&sorted_pages,16);

	/** Scan each element in the cache **/
	for(xe = xhqGetFirst(&IDX_INF.PageCache); xe; xe = xhqGetNext(&IDX_INF.PageCache))
	    {
	    /** Is this a page we want to look at? **/
	    pg = (pIdxPage)(xhqGetData(&IDX_INF.PageCache, xe, 0));
	    if ((tdata && tdata != pg->TData) || !(pg->Flags & IDX_PAGE_F_DIRTY)) continue;

	    /** Try to lock the page. **/
	    if (syGetSem(pg->Lock, 1, SEM_U_NOBLOCK) < 0) continue;

	    /** Need to write the current list of pages? **/
	    if (cur_tdata != pg->TData)
	        {
		for(i=0;i<sorted_pages.nItems;i++)
		    {
		    /** Try to write the page. **/
		    search_pg = (pIdxPage)(sorted_pages.Items[i]);
		    if (objWrite(pg->TData->NodeRef, pg->Data.Raw, IDX_PAGESIZE, OBJ_U_SEEK, pg->PageID*IDX_PAGESIZE) != IDX_PAGESIZE)
		        {
			mssError(0,"IDX","Could not flush page %d update to node object", pg->PageID);
			err = 1;
			}
		    syPostSem(search_pg->Lock, 1, 0);
		    }
		xaClear(&sorted_pages);
		}

	    /** Ok, add the current page to the list. **/
	    xaAddItemSorted(&sorted_pages, (void*)pg, 4, 4);
	    cur_tdata = pg->TData;
	    }

	/** Any last items to write? **/
	for(i=0;i<sorted_pages.nItems;i++)
	    {
	    /** Try to write the page. **/
	    search_pg = (pIdxPage)(sorted_pages.Items[i]);
	    if (objWrite(pg->TData->NodeRef, pg->Data.Raw, IDX_PAGESIZE, OBJ_U_SEEK, pg->PageID*IDX_PAGESIZE) != IDX_PAGESIZE)
	        {
		mssError(0,"IDX","Could not flush page %d update to node object", pg->PageID);
		err = 1;
		}
	    syPostSem(search_pg->Lock, 1, 0);
	    }

	/** Finally, release the lock on the page cache **/
	xaDeInit(&sorted_pages);
	xhqUnlock(&IDX_INF.PageCache);

    return err?-1:0;
    }


/*** idx_internal_AllocPage - allocate a new page in the given datafile.
 *** The allocated page is given a specified 'type', and is allocated as
 *** close to the 'target' page id as is reasonable, in accordance with
 *** the allocation algorithms in use.  Call this routine with the table-
 *** data semaphore locked.
 ***/
pIdxPage
idx_internal_AllocPage(pIdxTableData tdata, int type, int target)
    {
    int n_search, cur_grp, pg_offset,i,c;
    pIdxPage pg,a_pg;
    pIdxAllocGroup ag;

    	/** First scan the allocation groups to find a place to alloc. **/
	cur_grp = (target-tdata->HdrInf2->AllocGroups[0].HdrPage)/255;
	pg_offset = (target-tdata->HdrInf2->AllocGroups[0].HdrPage)%255;
	for(c=i=0;i<IDX_MAXGROUPS;i++)
	    {
	    if (tdata->HdrInf2->AllocGroups[cur_grp].FreePages > 0) break;
	    do { cur_grp += (c&1)?(-(c+1)):(c+1); c++; } while (cur_grp >= 0);
	    pg_offset = 0;
	    if (i == IDX_MAXGROUPS-1)
	        {
		mssError(1,"IDX","File storage exhausted");
		return NULL;
		}
	    }

	/** Allocation group not yet allocated? **/
	a_pg = NULL;
	if (tdata->HdrInf2->AllocGroups[cur_grp].HdrPage == 0)
	    {
	    tdata->HdrInf2->AllocGroups[cur_grp].HdrPage = tdata->HdrInf2->AllocGroups[cur_grp-1].HdrPage + (IDX_PAGESPERGROUP + IDX_PAGESPERGROUPHDR);
	    tdata->HdrInf2->AllocGroups[cur_grp].FreePages = IDX_PAGESPERGROUP;
	    a_pg = (pIdxPage)nmMalloc(sizeof(IdxPage));
	    a_pg->PageID = tdata->HdrInf2->AllocGroups[cur_grp-1].HdrPage + (IDX_PAGESPERGROUP + IDX_PAGESPERGROUPHDR);
	    a_pg->TData = tdata;
	    a_pg->Flags = IDX_PAGE_F_DIRTY;
	    a_pg->Type = IDX_PAGE_HEADER;
	    a_pg->Lock = syCreateSem(0,0);
	    a_pg->Xe = xhqAdd(&IDX_INF.PageCache, &(a_pg->TData), a_pg);
	    ag = (pIdxAllocGroup)(a_pg->Data.Raw);
	    ag->FreePages = IDX_PAGESPERGROUP;
	    ag->TotalPages = IDX_PAGESPERGROUP;
	    memset(ag->AllocationMap, PAGE_UNALLOCATED, IDX_PAGESPERGROUP);
	    }

	/** Ok, access the alloc group. **/
	if (!a_pg) a_pg = idx_internal_GetPage(tdata, IDX_PAGE_HEADER, tdata->HdrInf2->AllocGroups[cur_grp].HdrPage);
	if (!a_pg)
	    {
	    mssError(0,"IDX","Bark! Could not access allocation group %d header",cur_grp);
	    return NULL;
	    }
	ag = (pIdxAllocGroup)(a_pg->Data.Raw);
	if (ag->FreePages == 0)
	    {
	    mssError(1,"IDX","Bark! Alloc group summary free %d != actual free %d", tdata->HdrInf2->AllocGroups[cur_grp].FreePages, ag->FreePages);
	    idx_internal_ReleasePage(a_pg);
	    return NULL;
	    }
	for(c=i=0;i<IDX_PAGESPERGROUP;i++)
	    {
	    if (ag->AllocationMap[pg_offset] > 0x80) break;
	    do { pg_offset += (c&1)?(-(c+1)):(c+1); c++; } while (pg_offset >= 0);
	    if (i==IDX_PAGESPERGROUP-1)
	        {
		mssError(1,"IDX","Bark! Alloc group says free %d, no free page found", ag->FreePages);
		idx_internal_ReleasePage(a_pg);
		return NULL;
		}
	    }
	a_pg->FreePages--;
	tdata->HdrInf2->AllocGroups[cur_grp].FreePages--;
	a_pg->Flags |= IDX_PAGE_F_DIRTY;
	pg = (pIdxPage)nmMalloc(sizeof(IdxPage));
	pg->TData = tdata;
	pg->PageID = tdata->HdrInf2->AllocGroups[cur_grp].HdrPage + pg_offset + 1;
	pg->Type = type;
	pg->Flags = IDX_PAGE_F_DIRTY;
	pg->Lock = syCreateSem(0,0);
	pg->Xe = xhqAdd(&IDX_INF.PageCache, &(pg->TData), pg);
	idx_internal_ReleasePage(a_pg);

    return pg;
    }


/*** idx_internal_GetPage - access a page at a given page id.  The type
 *** of page must be specified, and it must match the internal alloc
 *** group bitmap page type.  If the page type is root, index, or data,
 *** the pageid in the datafile copy will be compared with the requested
 *** pageid when the page is read, to detect corruption early-on.
 ***/
pIdxPage
idx_internal_GetPage(pIdxTableData tdata, int type, int req_pageid)
    {
    pIdxPage pg;
    struct lookup_key { pIdxTableData td; int id; };
    int rcnt;

    	/** Try to fetch the page from the cache... **/
	lookup_key.td = tdata;
	lookup_key.id = req_pageid;
	pg = (pIdxPage)xhqLookup(&IDX_INF.PageCache, &lookup_key);
	if (pg)
	    {
	    syGetSem(pg->Lock, 1, 0);
	    return pg;
	    }

	/** Ok, not cached.  Read it and cache it. **/
	pg = (pIdxPage)nmMalloc(sizeof(IdxPage));
	rcnt = objRead(tdata->NodeRef, pg->Data.Raw, IDX_PAGESIZE, OBJ_U_SEEK, req_pageid*IDX_PAGESIZE);
	if (rcnt < 0)
	    {
	    nmFree(pg,sizeof(IdxPage));
	    mssError(0,"IDX","Bark! Could not read page %d", req_pageid);
	    return NULL;
	    }

	/** Verify the pageid. **/
	if (type == IDX_PAGE_INDEX || type == IDX_PAGE_DATA || type == IDX_PAGE_BLOB)
	    {
	    if (req_pageid != pg->Data.Info.PageID)
	        {
	        nmFree(pg,sizeof(IdxPage));
	        mssError(0,"IDX","Bark! Page %d requested, read page %d", req_pageid, pg->Data.Info.PageID);
	        return NULL;
		}
	    }

	/** Cache the page, possibly LRU-discarding another one in the process **/
	pg->TData = tdata;
	pg->PageID = req_pageid;
	pg->Flags = 0;
	pg->Type = type;
	pg->Lock = syCreateSem(0,0);
	pg->Xe = xhqAdd(&IDX_INF.PageCache, &(pg->TData), pg);

    return pg;
    }


/*** idx_internal_ReleasePage - call this routine when done working 
 *** with a given page; this must be called to unlock the page so other
 *** threads can access it.
 ***/
int
idx_internal_ReleasePage(pIdxPage page)
    {

    	/** Release the page semaphore **/
	syPostSem(page->Lock, 1, 0);
	xhqUnlink(&IDX_INF.PageCache, page->Xe, 0);

    return 0;
    }


/*** idx_internal_FreePage - releases a page back to freespace.  A 
 *** write operation will eventually be triggered which will zero the
 *** page in the database file, and will call Write with OBJ_U_DATAHOLE,
 *** which will, depending on the underlying storage facility, free the
 *** space used by the page in the underlying storage.
 ***/
int
idx_internal_FreePage(pIdxPage page)
    {
    int cur_grp, pg_offset;
    pIdxPage a_pg;
    pIdxAllocGroup ag;
    pIdxTableData tdata = page->TData;

    	/** Locate group and offset for the page. **/
	cur_grp = (page->PageID-tdata->HdrInf2->AllocGroups[0].HdrPage)/255;
	pg_offset = (page->PageID-tdata->HdrInf2->AllocGroups[0].HdrPage)%255;

	/** Get the alloc group hdr **/
	a_pg = idx_internal_GetPage(page->TData, IDX_PAGE_HEADER, tdata->HdrInf2->AllocGroups[cur_grp].HdrPage);
	if (!a_pg)
	    {
	    mssError(0,"IDX","Bark! Could not access group %d header", cur_grp);
	    return -1;
	    }
	ag = (pIdxAllocGroup)(a_pg->Data.Raw);
	if (ag->AllocationMap[pg_offset] >= 0x80)
	    {
	    mssError(0,"IDX","Bark! Attempt to free already-free page %d", page->PageID);
	    idx_internal_ReleasePage(a_pg);
	    return -1;
	    }
	ag->AllocationMap[pg_offset] = IDX_PAGE_FREE;
	ag->FreePages++;
	tdata->HdrInf2->AllocGroups[cur_grp].FreePages++;
	a_pg->Flags |= IDX_PAGE_F_DIRTY;
	idx_internal_ReleasePage(a_pg);
	xhqRemove(&(IDX_INF->PageCache), page->Xe, 0);
	syDestroySem(page->Lock, 0);
	nmFree(page, sizeof(IdxPage));

    return 0;
    }


/*** idx_internal_AttrGetType - this is a callback function for the below
 *** evalexpr function that is called by the Expression module to get the
 *** type of a field in a given table/row.
 ***/
int
idx_internal_AttrGetType(pIdxRowRef rowinf, char* attrname)
    {
    int i;

    	/** Scan the table data. **/
	for(i=0;i<rowinf->Page->TData->HdrInf2->nColumns;i++)
	    {
	    if (!strcmp(attrname, rowinf->Page->TData->Columns[i]->Name))
	        return rowinf->Page->TData->Columns[i]->Type;
	    }

    return -1;
    }


/*** idx_internal_AttrGetValue - this is a callback for getting the value
 *** of an element in a given row.
 ***/
int
idx_internal_AttrGetValue(pIdxRowRef rowinf, char* attrname, pObjData val)
    {
    int i,j,cnt;
    unsigned char* rowptr;

    	/** Scan the table data. **/
	for(i=0;i<rowinf->Page->TData->HdrInf2->nColumns;i++)
	    {
	    if (!strcmp(attrname, rowinf->Page->TData->Columns[i]->Name))
	        {
		cnt = 0;
		rowptr = rowinf->Page->Data.Raw + IDX_PAGE_ROWOFFSET(rowinf->Page, rowinf->RowID);
		rowptr += IDX_ROWHEADERSIZE;
		while(*rowptr != i && cnt < rowinf->Page->Data.Info.RowCount)
		    {
		    rowptr += (rowptr[1] + rowptr[2]*256);
		    cnt++;
		    }
		if (cnt == rowinf->Page->Data.Info.RowCount) return 1; /* null */
		switch(rowinf->Page->TData->Columns[i]->Type)
		    {
		    case DATA_T_INTEGER: memcpy(&(val->Integer), rowptr+3, 4); break;
		    case DATA_T_DOUBLE: memcpy(&(val->Double), rowptr+3, 8); break;
		    case DATA_T_STRING: val->String = rowptr+3; break;
		    case DATA_T_DATETIME: rowptr += (rowptr[1] + rowptr[2]*256 - sizeof(DateTime)); val->DateTime = (pDateTime)rowptr; break;
		    case DATA_T_MONEY: rowptr += (rowptr[1] + rowptr[2]*256 - sizeof(MoneyType)); val->Money = (pMoneyType)rowptr; break;
		    default: return -1;
		    }
	        return 0;
		}
	    }

    return -1;
    }


/*** idx_internal_EvalExpr - evaluate an expression against a given row.  Pass
 *** the page, rowid, and expression.  Result is expEvalTree return code, and
 *** the expression contains the resulting value.
 ***/
int
idx_internal_EvalExpr(pIdxPage pg, int rowid, pExpression expr)
    {
    int rval;
    pParamObjects objlist;
    IdxRowRef rowinf;

    	/** Create the paramobjs list **/
	objlist = expCreateParamList();
	rowinf.Page = pg;
	rowinf.RowID = rowid;
	expAddParamToList(objlist, NULL, &rowinf, 0);
	expSetParamFunctions(objlist, NULL, idx_internal_AttrGetType, idx_internal_AttrGetValue, NULL);

	/** Evaluate the expression **/
	rval = expEvalTree(expr, objlist);

	/** Free the object list **/
	expFreeParamList(objlist);

    return rval;
    }


/*** idx_internal_AddColumn - add a new column to the table header, obtaining
 *** column information from the passed "table/columns/colname" OXT node.  This 
 *** routine syncs the table header when it is finished.  Returns the new
 *** colid on success, -1 on failure.
 ***/
int
idx_internal_AddColumn(pIdxTableData tdata, char* colname, pObjTrxTree oxt)
    {
    pObjTrxTree attr_oxt;
    unsigned char* endptr;
    pIdxColData new_col;
    int len,offset;

    	/** First, find the end of current table header data **/
	if (tdata->HdrInf2->nColumns == 0)
	    endptr = strchr(tdata->RowAnnotString,0)+1;
	else
	    endptr = tdata->Columns[tdata->HdrInf2->nColumns-1]->RawData + tdata->Columns[tdata->HdrInf2->nColumns-1]->ColInf->DescriptorLength;

	/** Allocate the new column structure **/
	new_col = (pIdxColData)nmMalloc(sizeof(IdxColData));

	/** Determine length needed (start with hdr and terminators for each str). **/
	len = sizeof(ColInf) + 9;
	new_col->Name = colname;
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "constraint")) 
	    new_col->Constraint = attr_oxt->AttrValue;
	else
	    new_col->Constraint = "";
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "default")) 
	    new_col->Default = attr_oxt->AttrValue;
	else
	    new_col->Default = "";
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "minimum")) 
	    new_col->Minimum = attr_oxt->AttrValue;
	else
	    new_col->Minimum = "";
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "maximum")) 
	    new_col->Maximum = attr_oxt->AttrValue;
	else
	    new_col->Maximum = "";
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "enumquery")) 
	    new_col->EnumQuery = attr_oxt->AttrValue;
	else
	    new_col->EnumQuery = "";
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "format")) 
	    new_col->Format = attr_oxt->AttrValue;
	else
	    new_col->Format = "";
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "groupname")) 
	    new_col->GroupName = attr_oxt->AttrValue;
	else
	    new_col->GroupName = "";
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "description")) 
	    new_col->Description = attr_oxt->AttrValue;
	else
	    new_col->Description = "";

	/** Got the values, now adjust the lengths. **/
	len += strlen(new_col->Name) + strlen(new_col->Constraint) + strlen(new_col->Default);
	len += strlen(new_col->Minimum) + strlen(new_col->Maximum) + strlen(new_col->EnumQuery);
	len += strlen(new_col->Format) + strlen(new_col->GroupName) + strlen(new_col->Description);

	/** Add 4 to allow for realignment of the data structure. **/
	len += 4;

	/** Make room for the new column (at the end...) **/
	offset = endptr - tdata->RawHeader;
	idx_internal_SpaceTData(tdata, offset, len);

	/** Align the column structure to a 4-byte boundary. **/
	offset = (offset+3)&0xFFFFFFFC;
	new_col->RawData = tdata->RawHeader + offset;

	/** Setup the metadata for the column **/
	new_col->ColInf = (pColInf)(new_col->RawData);
	new_col->ColInf->DescriptorLength = len;
	new_col->ColInf->Flags = 0;
	endptr = new_col->RawData + sizeof(ColInf);
	strcpy(endptr,new_col->Name);
	new_col->Name = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->Constraint);
	new_col->Constraint = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->Default);
	new_col->Default = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->Minimum);
	new_col->Minimum = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->Maximum);
	new_col->Maximum = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->EnumQuery);
	new_col->EnumQuery = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->Format);
	new_col->Format = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->GroupName);
	new_col->GroupName = endptr;
	endptr = strchr(endptr,0)+1;
	strcpy(endptr,new_col->Description);

	/** Check some additional column properties **/
	new_col->ColInf->Flags = 0;
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "length")) 
	    new_col->ColInf->Length = *(int*)(attr_oxt->AttrValue);
	else
	    new_col->ColInf->Length = 0;
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "lengthdivisor")) 
	    new_col->ColInf->LengthDivisor = *(int*)(attr_oxt->AttrValue);
	else
	    new_col->ColInf->LengthDivisor = 0;
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "style")) 
	    new_col->ColInf->Style = *(int*)(attr_oxt->AttrValue);
	else
	    new_col->ColInf->Style = 0;
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "type")) 
	    {
	    if (!strcmp(attr_oxt->AttrValue,"string")) new_col->ColInf->Type = DATA_T_STRING;
	    else if (!strcmp(attr_oxt->AttrValue,"integer")) new_col->ColInf->Type = DATA_T_INTEGER;
	    else if (!strcmp(attr_oxt->AttrValue,"double")) new_col->ColInf->Type = DATA_T_DOUBLE;
	    else if (!strcmp(attr_oxt->AttrValue,"money")) new_col->ColInf->Type = DATA_T_MONEY;
	    else if (!strcmp(attr_oxt->AttrValue,"datetime")) new_col->ColInf->Type = DATA_T_DATETIME;
	    else 
	        {
		mssError(1,"IDX","Invalid data type '%s'",attr_oxt->AttrValue);
		new_col->Type = DATA_T_STRING;
		}
	    }
	else
	    {
	    new_col->Type = DATA_T_STRING;
	    }
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "keyid")) 
	    new_col->ColInf->KeyID = *(int*)(attr_oxt->AttrValue);
	else
	    new_col->ColInf->KeyID = 0;
	if (attr_oxt = obj_internal_FindAttrOxt(oxt, "groupid")) 
	    new_col->ColInf->GroupID = *(int*)(attr_oxt->AttrValue);
	else
	    new_col->ColInf->GroupID = 0;

    return 0;
    }


/*** idx_internal_DeleteColumn - remove an existing column from the table
 *** header, given the column id.  The id will not be re-used, but instead
 *** the column name in the descriptor will be zeroed, indicating that the
 *** column has been deleted.  Data for the column in various rows will not
 *** be deleted until the row is next updated (do an "update table set 
 *** field=field" to force that update).  Once 256 columns have been added
 *** (though possibly deleted as well), no more columns can be added; to
 *** rectify this do a "select into" to build a copy of the table.
 ***/
int
idx_internal_DeleteColumn(pIdxTableData tdata, int colid)
    {
    pIdxColData col;
    int oldsize;
    	
	/** Reset the column descriptor to zeroed values. **/
	col = tdata->Columns[colid];
	oldsize = col->DescriptorLength;
	memset(col->RawData, 0, sizeof(ColInf) + 9);
	col->ColInf->DescriptorLength = sizeof(ColInf) + 9;
	col->Name = col->RawData + sizeof(ColInf);
	col->Constraint = col->RawData + sizeof(ColInf) + 1;
	col->Default = col->RawData + sizeof(ColInf) + 2;
	col->Minimum = col->RawData + sizeof(ColInf) + 3;
	col->Maximum = col->RawData + sizeof(ColInf) + 4;
	col->EnumQuery = col->RawData + sizeof(ColInf) + 5;
	col->Format = col->RawData + sizeof(ColInf) + 6;
	col->GroupName = col->RawData + sizeof(ColInf) + 7;
	col->Description = col->RawData + sizeof(ColInf) + 8;

	/** Resize the table header **/
	idx_internal_SpaceTData(tdata, (col->RawData + sizeof(ColInf) + 8) - tdata->RawHeader, col->DescriptorLength - oldsize);

    return 0;
    }


/*** idx_internal_GetColAttr - get the attribute for column metadata (not
 *** column data within a row!).  Can be called from idxGetAttrValue.
 ***/
int
idx_internal_GetColAttr(pIdxTableData tdata, int colid, char* attrname, pObjData val)
    {
    pIdxColData col = tdata->Columns[colid];
    pColInf colinf = col->ColInf;

    	/** Check the attr, one string at a time. **/
	if (!strcmp(attrname,"length")) val->Integer = colinf->Length;
	else if (!strcmp(attrname,"lengthdivisor")) val->Integer = colinf->LengthDivisor;
	else if (!strcmp(attrname,"style")) val->Integer = colinf->Style;
	else if (!strcmp(attrname,"type"))
	    {
	    switch(colinf->Type)
	        {
		case DATA_T_INTEGER: val->String = "integer"; break;
		case DATA_T_STRING: val->String = "string"; break;
		case DATA_T_DOUBLE: val->String = "double"; break;
		case DATA_T_MONEY: val->String = "money"; break;
		case DATA_T_DATETIME: val->String = "datetime"; break;
		default: val->String = "unknown"; break;
		}
	    }
	else if (!strcmp(attrname,"keyid")) val->Integer = colinf->KeyID;
	else if (!strcmp(attrname,"groupid")) val->Integer = colinf->GroupID;
	else if (!strcmp(attrname,"name")) val->String = col->Name;
	else if (!strcmp(attrname,"constraint")) val->String = col->Constraint;
	else if (!strcmp(attrname,"default")) val->String = col->Default;
	else if (!strcmp(attrname,"minimum")) val->String = col->Minimum;
	else if (!strcmp(attrname,"maximum")) val->String = col->Maximum;
	else if (!strcmp(attrname,"enumquery")) val->String = col->EnumQuery;
	else if (!strcmp(attrname,"format")) val->String = col->Format;
	else if (!strcmp(attrname,"groupname")) val->String = col->GroupName;
	else if (!strcmp(attrname,"description")) val->String = col->Description;
	else return -1;

    return 0;
    }


/*** idx_internal_SetColAttr - sets an attribute in the column metadata
 *** (not in a row!).  Can be called from idxSetAttrValue.
 ***/
int
idx_internal_SetColAttr(pIdxTableData tdata, int colid, char* attrname, pObjData val)
    {
    unsigned char* ptr;
    }


/*** idx_internal_GetTableAttr - get an attribute of the general table 
 *** metadata.
 ***/
int
idx_internal_GetTableAttr(pIdxTableData tdata, char* attrname, pObjData val)
    {
    }


/*** idx_internal_SetTableAttr - set an attribute in the general table
 *** metadata.
 ***/
int
idx_internal_SetTableAttr(pIdxTableData tdata, char* attrname, pObjData val)
    {
    }


/*** idx_internal_CompareKey - takes two row pointers and compares the
 *** primary keys for less than, equality, or greater than, and returns
 *** -1, 0, or 1 respectively.
 ***/
int
idx_internal_CompareKey(pIdxTableData tdata, unsigned char* row1, unsigned char* row2)
    {
    }


/*** idx_internal_AddRowToPage - takes a pointer to raw row data, and inserts
 *** that row into a page.  If the row will not fit, a page split will occur.
 *** This routine should be called with the page locked.  If the page in 
 *** question has child pointers, then only the primary key part of the row
 *** will be inserted.  This allows this function to be used in a recursive
 *** manner.  Otherwise, the whole row in raw format will be inserted into
 *** the page.
 ***/
int
idx_internal_AddRowToPage(pIdxPage pg, unsigned char* newrow)
    {
    }


/*** idx_internal_DeleteRowFromPage - removes a row from a given page, and
 *** if the page becomes empty, deallocates the page and removes the key from
 *** the parent page as well.
 ***/
int
idx_internal_DeleteRowFromPage(pIdxPage pg, int rowid)
    {
    }


/*** idx_internal_NameToRowKey - converts the "name" of a row into the binary
 *** row data representation thereof, so that it can be built into raw row
 *** data or compared with row data via CompareKey.  RowBuf must be large enough
 *** to hold the resulting data; otherwise this fails.
 ***/
int
idx_internal_NameToRowKey(pIdxTableData tdata, char* name, unsigned char* rowbuf, int maxlen)
    {
    }


/*** idx_internal_RowKeyToName - convert the binary row primary key value into
 *** the "name" of the row, so that the "name" can be returned to the user in a
 *** more human-recognizable format :)
 ***/
int
idx_internal_RowKeyToName(pIdxTableData tdata, unsigned char* rowbuf, char* name, int maxlen)
    {
    }


/*** idxOpen - open an object.
 ***/
void*
idxOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pIdxData inf;
    int rval;
    char* node_path;
    char* ptr;
    char* name;

	/** Allocate the structure **/
	inf = (pIdxData)nmMalloc(sizeof(IdxData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(IdxData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

    return (void*)inf;
    }


/*** idxClose - close an open object.
 ***/
int
idxClose(void* inf_v, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);

	/** Release the memory **/
	nmFree(inf,sizeof(IdxData));

    return 0;
    }


/*** idxCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
idxCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = idxOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	idxClose(inf, oxt);

    return 0;
    }


/*** idxDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
idxDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pIdxData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pIdxData)idxOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(IdxData));

    return 0;
    }


/*** idxRead - Structure elements have no content.  Fails.
 ***/
int
idxRead(void* inf_v, char* buffer, int maxcnt, int flags, int offset, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    return -1;
    }


/*** idxWrite - Again, no content.  This fails.
 ***/
int
idxWrite(void* inf_v, char* buffer, int cnt, int flags, int offset, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    return -1;
    }


/*** idxOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
idxOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    pIdxQuery qy;

	/** Allocate the query structure **/
	qy = (pIdxQuery)nmMalloc(sizeof(IdxQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(IdxQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;

    return (void*)qy;
    }


/*** idxQueryFetch - get the next directory entry as an open object.
 ***/
void*
idxQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pIdxQuery qy = ((pIdxQuery)(qy_v));
    pIdxData inf;
    char* new_obj_name = "newobj";
    char* ptr;

	qy->ItemCnt++;

    return (void*)inf;
    }


/*** idxQueryClose - close the query.
 ***/
int
idxQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    int i;
    pIdxQuery qy = (pIdxQuery)qy_v;

	/** Free the structure **/
	nmFree(qy,sizeof(IdxQuery));

    return 0;
    }


/*** idxGetAttrType - get the type (DATA_T_idx) of an attribute by name.
 ***/
int
idxGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    return -1;
    }


/*** idxGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
idxGetAttrValue(void* inf_v, char* attrname, pObjData val, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	/** REPLACE MYOBJECT/TYPE WITH AN APPROPRIATE TYPE. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (inf->Type == IDX_T_USERLIST) *((char**)val) = "system/void";
	    else if (inf->Type == IDX_T_USER) *((char**)val) = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (inf->Type == IDX_T_USERLIST) *((char**)val) = "system/idxserlist";
	    else if (inf->Type == IDX_T_USER) *((char**)val) = "system/user";
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    return 0;
	    }

	mssError(1,"IDX","Could not locate requested attribute");

    return -1;
    }


/*** idxGetNextAttr - get the next attribute name for this object.
 ***/
char*
idxGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);

    return NULL;
    }


/*** idxGetFirstAttr - get the first attribute name for this object.
 ***/
char*
idxGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = idxGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** idxSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
idxSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    pStructInf find_inf;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }

	/** DO YOUR SEARCHING FOR ATTRIBUTES TO SET HERE **/
	return -1;

    return 0;
    }


/*** idxAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
idxAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree* oxt)
    {
    pIdxData inf = IDX(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** idxOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
idxOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** idxGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
idxGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** idxGetNextMethod -- same as above.  Always fails. 
 ***/
char*
idxGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** idxExecuteMethod - No methods to execute, so this fails.
 ***/
int
idxExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** idxPresentationHints - return a presentation-hints structure
 *** containing information about a particular attribute.
 ***/
pObjPresentationHints
idxPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** idxInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
idxInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&IDX_INF,0,sizeof(IDX_INF));
	xhqInit(&IDX_INF.PageCache,IDX_CACHE_MAXPAGES,sizeof(unsigned int) + sizeof(pIdxTableData), 255, idx_internal_DiscardPage, NULL);
	xhInit(&IDX_INF.TableData, 255, 0);
	IDX_INF.Pages = (pIdxPage)nmSysMalloc(sizeof(IdxPage)*IDX_CACHE_MAXPAGES);
	IDX_INF.nAlloc = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"IDX - Indexed Datafile Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/indexfile");

	/** Setup the function references. **/
	drv->Open = idxOpen;
	drv->Close = idxClose;
	drv->Create = idxCreate;
	drv->Delete = idxDelete;
	drv->OpenQuery = idxOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = idxQueryFetch;
	drv->QueryClose = idxQueryClose;
	drv->Read = idxRead;
	drv->Write = idxWrite;
	drv->GetAttrType = idxGetAttrType;
	drv->GetAttrValue = idxGetAttrValue;
	drv->GetFirstAttr = idxGetFirstAttr;
	drv->GetNextAttr = idxGetNextAttr;
	drv->SetAttrValue = idxSetAttrValue;
	drv->AddAttr = idxAddAttr;
	drv->OpenAttr = idxOpenAttr;
	drv->GetFirstMethod = idxGetFirstMethod;
	drv->GetNextMethod = idxGetNextMethod;
	drv->ExecuteMethod = idxExecuteMethod;
	drv->PresentationHints = idxPresentationHints;

	nmRegister(sizeof(IdxData),"IdxData");
	nmRegister(sizeof(IdxQuery),"IdxQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

