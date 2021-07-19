#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <regex.h>
#include <ctype.h>
#include <sys/times.h>
#include <math.h>

#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/xhashqueue.h"
#include "multiquery.h"
#include "cxlib/magic.h"
#include "centrallix.h"
#include "cxlib/util.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2002 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_dbl.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	June 11th, 2002 					*/
/* Description:	Objectsystem driver for DBL ISAM files.  Requires there	*/
/*		to be DEF data associated with the IS1 file - see the	*/
/*		def_search_path setting in centrallix.conf for further	*/
/*		details.						*/
/************************************************************************/



/*** Module controls ***/
#define	DBL_DEFAULT_DEF_SEARCH_PATH "%.IS1 = %.DEF(^COMMON %):%.IS1 = ../src/%.DEF(^COMMON %)"
#define	DBL_MAX_PATH_ITEMS	16
#define DBL_FORCE_READONLY	1	/* not update safe yet!!! */
#define DBL_MAGIC		0x5cb5
#define DBL_INDEX_MAGIC		0x5ca5
#define DBL_MAX_COLS		256
#define DBL_SEGMENTSIZE		0x0400


/*** DBL .ISM leaf index page header structure.  12 bytes.
 ***
 *** followed by RecCnt records, each 4 byte offset + n byte key.
 ***/
typedef struct
    {
    CXUINT8		FF;		/* 0xff = leaf segment */
    CXUINT8		RecCnt;		/* records in this segment before next seg starts */
    CXINT8		__b[2];
    CXUINT32		PrevSegment;
    CXUINT32		NextSegment;
    }
    DblLeafPageHeader, *pDblLeafPageHeader;


/*** DBL .ISM index page header structure.  4 bytes.
 ***
 *** followed by RecCnt records and RecCnt+1 offsets, alternating,
 *** as in:  offset + rec + offset + rec + offset.  Records are counted
 *** strings with count byte including itself in the count.  Records 
 *** are not entire key - rather just enough to differentiate.
 ***/
typedef struct
    {
    CXUINT8		Zero;		/* 0x00 = index segment */
    CXUINT8		RecCnt;		/* records in this segment before next seg starts */
    CXUINT16		Length;		/* length of segment that is actually used */
    }
    DblIndexPageHeader, *pDblIndexPageHeader;


/*** DBL .IS1 / .ISM file header key data structure. 60 bytes. ***/
typedef struct
    {
    CXUINT8		Flags;		/* DBL_HDR_F_xxx */
    CXUINT8		RecsPerPage;	/* records per leaf page. */
    CXUINT8		TreeDepth;	/* depth of index tree, including leaf level */
    CXINT8		__a[1];
    CXUINT32		IndexStart;	/* first index page.  For IS1 = first leaf.  For ISM = root page */
    CXUINT16		SegmentOffset[8]; /* key segment offsets */
    CXUINT16		SegmentLength[8]; /* key segment lengths */
    CXUINT16		IndexRecLen;	/* length of index records for this key */
    CXINT8		__b[2];
    CXCHAR		Name[15];
    CXUINT8		Zero;
    }
    DblHeaderKey, *pDblHeaderKey;

#define DBL_HDR_F_FLG0		1	/* unknown */
#define DBL_HDR_F_FLG1		2	/* unknown */
#define DBL_HDR_F_FLG2		4	/* either Modify or Allow Dups */
#define DBL_HDR_F_FLG3		8	/* either Modify or Allow Dups */


/*** DBL .IS1 / .ISM file header structure.  LSB byte order.  1024 bytes.
 ***/
typedef struct
    {
    CXUINT16		Magic;		/* 0x5cb5 for .IS1, 0x5ca5 for .ISM */
    CXINT8		__a[2];
    CXINT8		NumKeys;	/* number of keys in file */
    CXINT8		__b[2];
    CXINT8		RecPrelen;	/* record pre-data length */
    CXINT8		__c[2];
    CXUINT16		KeyLen;		/* length of index key */
    CXUINT16		RecLen;		/* logical record length */
    CXINT8		__d[6];
    CXINT32		RecCount;	/* only valid in index file */
    CXINT8		__e[4];
    DblHeaderKey	Keys[8];
    CXUINT16		PhysLen;	/* physical record length */
    CXINT8		__f[508];
    CXCHAR		Version[6];	/* " 51000" Hmm... not sure this is a version... */
    }
    DblHeader, *pDblHeader;


/*** This is just to help match the datafile header against the index header. ***/
static DblHeader dataindex_hdr_mask;


/*** Column data within a DBL ISAM table ***/
typedef struct
    {
    char		Name[32];
    int			Flags;		/* DBL_COL_F_xxx */
    int			Offset;		/* for non-virtual, offset in record; for virtual, offset from column id. */
    int			StartCol;	/* starting col # (0 to n-1) for virtuals */
    int			Length;		/* length of field, for virtual or non virtual */
    char		Description[64];
    char		Format[32];
    int			KeyID;		/* which key, w.r.t. file header */
    unsigned short*	ByteMap;	/* computed mapping, for virtual fields */
    int			Type;		/* centrallix data type */
    int			DecimalOffset;	/* where is the decimal point in a numeric value */
    int			ParsedOffset;
    }
    DblColInf, *pDblColInf;

#define DBL_COL_F_NUMERIC	1	/* 'D' types, otherwise 'A' type */
#define DBL_COL_F_VIRTUAL	2	/* points inside other column(s) */
#define DBL_COL_F_KEY		4	/* indexing key */
#define DBL_COL_F_SEGMENTED	8	/* bytemap is not linear */


/*** Structure for directory entry nodes ***/
typedef struct
    {
    char	Path[OBJSYS_MAX_PATH];
    int		Type;
    pSnNode	SnNode;
    int		Serial;
    char	IsmPath[OBJSYS_MAX_PATH];
    char	DefPath[OBJSYS_MAX_PATH];
    char	Description[256];
    XHashTable	TablesByName;
    XArray	TablesList;
    int		Flags;			/* DBL_NODE_F_xxx */
    clock_t	LastTimestamp;		/* used for revalidation */
    }
    DblNode, *pDblNode;

#define DBL_NODE_T_NODE		0	/* datafile group */
#define DBL_NODE_T_DATA		1	/* one datafile */
#define DBL_NODE_T_INDEX	2	/* passthrough */
#define DBL_NODE_T_DEFINITION	3	/* passthrough */

#define DBL_NODE_F_IGNKEYNAMES	1	/* ignore names of keys in .ISM/.IS1 headers */


/*** Structure for storing table metadata information ***/
typedef struct
    {
    char		Name[32];
    char		Annotation[256];
    pExpression		RowAnnotExpr;
    pParamObjects	ObjList;	/* used for row annot expr evaluation */
    DblHeader		FileHeader;
    DblHeader		IndexHeader;
    DblHeader		TmpHeader;
    char		DataPath[OBJSYS_MAX_PATH];
    char		IndexPath[OBJSYS_MAX_PATH];
    char		DefPath[OBJSYS_MAX_PATH];
    int			nColumns;
    pDblColInf		Columns[DBL_MAX_COLS];
    int			HasContent;
    pDblNode		Node;
    int			PriKey;
    clock_t		LastTimestamp;	/* used for revalidation */
    DateTime		DataMtime;
    DateTime		IndexMtime;
    DateTime		DefMtime;
    XHashTable		Files;
    int			ColMemory;
    }
    DblTableInf, *pDblTableInf;


/*** Structure for maintaining information about open data and index files ***/
typedef struct
    {
    pObjSession		Session;
    pObject		DataObject;
    pObject		IndexObject;
    int			LinkCnt;
    pDblTableInf	TData;
    }
    DblOpenFiles, *pDblOpenFiles;


/*** Structure used by this driver internally for open objects ***/
typedef struct 
    {
    pDblNode	Node;
    pDblTableInf TData;
    Pathname	Pathname;
    int		Type;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    char*	TablePtr;
    char*	TableSubPtr;
    char*	RowColPtr;
    unsigned char* RowData;
    int		Size;
    unsigned int Offset;	/* offset into file of a given row */
    unsigned char* ParsedData;
    unsigned char ColFlags[DBL_MAX_COLS];   /* DBL_DATA_COL_F_xxx */
    }
    DblData, *pDblData;

#define DBL_T_DATABASE		1
#define DBL_T_TABLE		2
#define DBL_T_ROWSOBJ		3
#define DBL_T_COLSOBJ		4
#define DBL_T_ROW		5
#define DBL_T_COLUMN		6

#define DBL(x) ((pDblData)(x))

#define DBL_DATA_COL_F_PARSED	1

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pDblData	ObjInf;
    pDblTableInf TableInf;
    pObjSession	ObjSession;
    int		RowCnt;
    pObject	LLObj;
    pObjQuery	LLQuery;
    pDblOpenFiles Files;
    }
    DblQuery, *pDblQuery;


/*** structure for search path item ***/
typedef struct
    {
    char*		SrcPattern;
    char*		DefPattern;
    char*		SearchRegex;
    }
    DblSearchPathItem, *pDblSearchPathItem;



/*** GLOBALS ***/
struct
    {
    XHashTable		DBNodes;
    XArray		DBNodeList;
    pStructInf		DblConfig;
    char*		DefSearchPath;
    pDblSearchPathItem	SearchItems[DBL_MAX_PATH_ITEMS];
    int			nSearchItems;
    pObjDriver		ObjDriver;
    }
    DBL_INF;


/*** dbl_internal_Timestamp() - get a current timestamp, in the 
 *** resolution of the CLK_TCK times() counter.  This is used for
 *** finding out when we need to grab a last_modification on the
 *** underlying file to see if it has changed, and revalidate if
 *** needed.
 ***/
clock_t
dbl_internal_Timestamp()
    {
    clock_t t;
    struct tms tms;

	t = times(&tms);

    return t;
    }


/*** dbl_internal_FreeTData() - release table info structure.
 ***/
void
dbl_internal_FreeTData(pDblTableInf tdata)
    {
    int i;

	/** free columns **/
	for(i=0;i<tdata->nColumns;i++)
	    {
	    if (tdata->Columns[i]->ByteMap) nmSysFree(tdata->Columns[i]->ByteMap);
	    nmFree(tdata->Columns[i], sizeof(DblColInf));
	    }

	/** free the tdata itself **/
	nmFree(tdata, sizeof(DblTableInf));
	
    return;
    }


/*** dbl_internal_OpenFiles() - open the data and index files for a given table
 *** within a given OSML session.
 ***/
pDblOpenFiles
dbl_internal_OpenFiles(pObjSession s, pDblTableInf tdata)
    {
    pDblOpenFiles files;

	/** already open?  cool... **/
	if ((files = (pDblOpenFiles)xhLookup(&(tdata->Files), (void*)s)) != NULL)
	    {
	    files->LinkCnt++;
	    return files;
	    }

	/** Open em.. **/
	files = (pDblOpenFiles)nmMalloc(sizeof(DblOpenFiles));
	if (!files) return NULL;
	files->Session = s;
	files->TData = tdata;
	files->LinkCnt = 1;
	files->IndexObject = objOpen(s, tdata->IndexPath, O_RDONLY, 0600, "application/octet-stream");
	if (!files->IndexObject)
	    {
	    nmFree(files, sizeof(DblOpenFiles));
	    return NULL;
	    }
	files->DataObject = objOpen(s, tdata->DataPath, O_RDONLY, 0600, "application/octet-stream");
	if (!files->DataObject)
	    {
	    objClose(files->IndexObject);
	    nmFree(files, sizeof(DblOpenFiles));
	    return NULL;
	    }
	xhAdd(&(tdata->Files), (void*)(files->Session), (void*)files);

    return files;
    }


/*** dbl_internal_CloseFiles() - close up the files for a given table within
 *** a given OSML session.
 ***/
int
dbl_internal_CloseFiles(pDblOpenFiles files)
    {

	/** Last close? **/
	if ((--files->LinkCnt) == 0)
	    {
	    objClose(files->IndexObject);
	    objClose(files->DataObject);
	    xhRemove(&(files->TData->Files), (void*)(files->Session));
	    nmFree(files, sizeof(DblOpenFiles));
	    }

    return 0;
    }


/*** dbl_internal_ReadHeader() - read in the header from an ISM or IS1 file.
 ***
 *** Return: -1 on error, 0 on success, 1 on not-modified.
 ***/
int
dbl_internal_ReadHeader(pObjSession s, char* path, pDblHeader hdr, pDateTime last_mod)
    {
    pObject obj;
    pDateTime dt;

	/** Open the object **/
	obj = objOpen(s, path, O_RDONLY, 0600, "application/octet-stream");
	if (!obj)
	    {
	    return -1;
	    }

	/** Check date/time? **/
	if (last_mod)
	    {
	    if (objGetAttrValue(obj, "last_modification", DATA_T_DATETIME, POD(&dt)) == 0)
		{
		if (!memcmp(dt, last_mod, sizeof(DateTime))) 
		    {
		    objClose(obj);
		    return 1;
		    }

		/** If changed, update the last_mod passed by the caller. **/
		memcpy(last_mod, dt, sizeof(DateTime));
		}
	    }

	/** Read the header **/
	if (objRead(obj, (char*)hdr, sizeof(DblHeader), 0, 0) != sizeof(DblHeader))
	    {
	    objClose(obj);
	    return -1;
	    }
	objClose(obj);

    return 0;
    }


/*** dbl_internal_CompareHeader() - compare one ISM header against another to
 *** see if they are consistent, based on comparison only of structural /
 *** definition fields, not on data fields that may easily change, such as
 *** record count and pointers to root index pages.
 ***/
int
dbl_internal_CompareHeader(pDblHeader h1, pDblHeader h2)
    {
    int i;

	/** Compare index header against file header - some things need to match **/
	for(i=0;i<sizeof(DblHeader);i++)
	    {
	    if ((((char*)&dataindex_hdr_mask)[i] & ((char*)h1)[i]) != (((char*)&dataindex_hdr_mask)[i] & ((char*)h2)[i]))
		{
		mssError(1, "DBL", "Header mismatch at offset 0x%4.4x (0x%2.2x != 0x%2.2x)", 
			i, ((char*)h1)[i], ((char*)h2)[i]);
		return -1;
		}
	    }

    return 0;
    }


/*** dbl_internal_Revalidate() - check to see if anything underlying a given
 *** table has changed, and if so, refresh the data.
 ***
 *** TODO: revalidate .DEF file as well; if any caching is being done
 *** by the driver, flush the caches on any file modification and account
 *** for changed first-index-page pointers.
 ***/
int
dbl_internal_Revalidate(pObjSession s, pDblTableInf tdata)
    {
    clock_t current;
    int rval;
    int was_modified = 0;

	/** First, revalidate the node. **/
	current = dbl_internal_Timestamp();
	if (tdata->Node->LastTimestamp != current)
	    {
	    /** FIXME - check reloading the node here **/
	    tdata->Node->LastTimestamp = current;
	    }

	/** Then revalidate the underlying data, index, and definition files **/
	if (tdata->LastTimestamp != current)
	    {
	    /** FIXME - check reloading .DEF file as well as ISM/IS1 headers **/
	    rval = dbl_internal_ReadHeader(s, tdata->DataPath, &(tdata->TmpHeader), &(tdata->DataMtime));
	    if (rval < 0)
		{
		mssError(0, "DBL", "Unhandled exception - table '%s' data file '%s' became unreadable!", 
			tdata->Name, tdata->DataPath);
		return -1;
		}
	    if (rval == 0)
		{
		/** data header was modified **/
		if (dbl_internal_CompareHeader(&(tdata->FileHeader), &(tdata->TmpHeader)) < 0)
		    {
		    mssError(0, "DBL", "Unhandled exception - table '%s' data file '%s' changed unrecoverably!", 
			    tdata->Name, tdata->DataPath);
		    return -1;
		    }
		memcpy(&(tdata->FileHeader), &(tdata->TmpHeader), sizeof(DblHeader));
		was_modified = 1;
		}
	    rval = dbl_internal_ReadHeader(s, tdata->IndexPath, &(tdata->TmpHeader), &(tdata->IndexMtime));
	    if (rval < 0)
		{
		mssError(0, "DBL", "Unhandled exception - table '%s' index file '%s' became unreadable!", 
			tdata->Name, tdata->IndexPath);
		return -1;
		}
	    if (rval == 0)
		{
		/** index header was modified **/
		if (dbl_internal_CompareHeader(&(tdata->IndexHeader), &(tdata->TmpHeader)) < 0)
		    {
		    mssError(0, "DBL", "Unhandled exception - table '%s' index file '%s' changed unrecoverably!", 
			    tdata->Name, tdata->IndexPath);
		    return -1;
		    }
		memcpy(&(tdata->IndexHeader), &(tdata->TmpHeader), sizeof(DblHeader));
		was_modified = 1;
		}
	    tdata->LastTimestamp = current;
	    }

    return was_modified;
    }


/*** dbl_internal_VerifyByteMap() - check a key against a given column bytemap.
 ***/
int
dbl_internal_VerifyByteMap(pDblHeaderKey key, pDblColInf column, int report_errors)
    {
    int offset;
    int j,k;

	offset = 0;
	for(j=0;j<8;j++)
	    {
	    for(k=0; k<key->SegmentLength[j]; k++)
		{
		if (offset >= column->Length) 
		    {
		    if (report_errors) 
			mssError(1, "DBL", "Key '%s' segment %d extends past end of column (length %d)",
			    column->Name, j, column->Length);
		    return -1;
		    }
		if (column->ByteMap[offset] != key->SegmentOffset[j] + k) 
		    {
		    if (report_errors)
			mssError(1, "DBL", "Key '%s', position %d (%d:%d) IS1 segment bytemap %d does not match DEF-derived bytemap %d",
			    column->Name, offset, j, k, key->SegmentOffset[j]+k, column->ByteMap[offset]);
		    return -1;
		    }
		offset++;
		}
	    }
	if (offset != column->Length) 
	    {
	    if (report_errors)
		mssError(1, "DBL", "Key '%s' IS1 segmented length %d does not match DEF length %d",
		    column->Name, offset, column->Length);
	    return -1;
	    }

    return 0;
    }


/*** dbl_internal_VerifyDefinition() - compare the loaded definition of
 *** the table against what is actually in the ISAM file header, to get
 *** the key id's from the header as well as verify consistency and build
 *** the byte map for each virtual column.
 ***/
int
dbl_internal_VerifyDefinition(pDblTableInf tdata)
    {
    int i,j,found;
    pDblHeaderKey key;
    pDblColInf column;
    int colid,offset;
    int max_offset;

	/** Check header consistency **/
	if (dbl_internal_CompareHeader(&(tdata->FileHeader), &(tdata->IndexHeader)) < 0)
	    {
	    mssError(0, "DBL", "File header and index header for table '%s' are inconsistent", tdata->Name);
	    return -1;
	    }

	/** Check key count **/
	if (tdata->FileHeader.NumKeys > 8 || tdata->FileHeader.NumKeys < 1) 
	    {
	    mssError(1, "DBL", "Unexpected file header key count %d for table '%s'", tdata->FileHeader.NumKeys, tdata->Name);
	    return -1;
	    }

	/** First, get the key ids and verify key configuration **/
	for(j=0;j<tdata->FileHeader.NumKeys;j++)
	    {
	    key = &(tdata->FileHeader.Keys[j]);
	    if (key->Zero != 0) 
		{
		mssError(1, "DBL", "Unexpected file header key Zero field %d for table '%s', key id %d", key->Zero, tdata->Name, j);
		return -1;
		}
	    found = -1;
	    for(i=0;i<tdata->nColumns;i++)
		{
		column = tdata->Columns[i];
		if (strlen(column->Name) > 15) return -1;
		if (!strncmp(key->Name, column->Name, strlen(column->Name)) && (key->Name[strlen(column->Name)] == ' ' || key->Name[strlen(column->Name)] == '\0'))
		    {
		    found = i;
		    break;
		    }
		}
	    if (!(tdata->Node->Flags & DBL_NODE_F_IGNKEYNAMES))
		{
		if (found < 0) 
		    {
		    mssError(1, "DBL", "Key id %d not found in DEF file for table '%s'", j, tdata->Name);
		    return -1;
		    }
		column->Flags |= DBL_COL_F_KEY;
		column->KeyID = j;
		if (j == 0) tdata->PriKey = found;
		}
	    }

	/** Build the bytemap for all fields **/
	max_offset = 0;
	for(i=0;i<tdata->nColumns;i++)
	    {
	    column = tdata->Columns[i];
	    column->ByteMap = nmSysMalloc(column->Length*2);
	    if (!column->ByteMap) return -1;
	    if (column->Flags & DBL_COL_F_VIRTUAL)
		{
		/** map for virtual fields is tricky **/
		colid = column->StartCol;
		if (tdata->Columns[colid]->ByteMap == NULL) 
		    {
		    mssError(1, "DBL", "Bark!  Table/column '%s':'%s' referred to by column '%s' does not have bytemap", 
			    tdata->Name, tdata->Columns[colid]->Name, column->Name);
		    return -1;	/* has to be defined previously! */
		    }

		/** Skip to the real starting column **/
		while(column->Offset >= tdata->Columns[colid]->Length)
		    {
		    colid++;
		    column->StartCol++;
		    if (colid >= tdata->nColumns) 
			{
			mssError(1, "DBL", "Virtual column '%s' in table '%s' begins after end of physical record definition.",
				column->Name, tdata->Name);
			return -1;
			}
		    if (tdata->Columns[colid]->ByteMap == NULL) 
			{
			mssError(1, "DBL", "Bark!  Table/column '%s':'%s' needed by column '%s' does not have bytemap", 
				tdata->Name, tdata->Columns[colid]->Name, column->Name);
			return -1;
			}
		    column->Offset -= tdata->Columns[colid-1]->Length;
		    }

		/** Start the map **/
		offset = column->Offset;
		for(j=0;j<column->Length;j++)
		    {
		    column->ByteMap[j] = tdata->Columns[colid]->ByteMap[offset++];
		    if (j != 0 && column->ByteMap[j] != column->ByteMap[j-1]+1) 
			column->Flags |= DBL_COL_F_SEGMENTED;
		    if (offset >= tdata->Columns[colid]->Length)
			{
			colid++;
			if (colid >= tdata->nColumns) 
			    {
			    mssError(1, "DBL", "Virtual column '%s' in table '%s' overruns end of physical record definition.",
				    column->Name, tdata->Name);
			    return -1;
			    }
			if (tdata->Columns[colid]->ByteMap == NULL) 
			    {
			    mssError(1, "DBL", "Virtual column '%s' in table '%s' needed by column '%s' does not have bytemap",
				    tdata->Columns[colid]->ByteMap, tdata->Name, column->Name);
			    return -1;
			    }
			if (tdata->Columns[colid]->Length == 0) 
			    {
			    mssError(1, "DBL", "Virtual column '%s' in table '%s' needed by column '%s' is zero-length",
				    tdata->Columns[colid]->ByteMap, tdata->Name, column->Name);
			    return -1;
			    }
			offset = 0;
			}
		    }
		}
	    else
		{
		/** map for nonvirtual fields is easy **/
		for(j=0;j<column->Length;j++)
		    {
		    column->ByteMap[j] = column->Offset + j;
		    if (max_offset < column->ByteMap[j]) max_offset = column->ByteMap[j];
		    }
		}
	    }

	/** Check record length **/
	if (max_offset != tdata->FileHeader.RecLen - 1) 
	    {
	    mssError(1, "DBL", "Table '%s' record length %d from IS1 file does not match record definition length %d",
		    tdata->Name, tdata->FileHeader.RecLen - 1, max_offset);
	    return -1;
	    }
	if (tdata->FileHeader.PhysLen < tdata->FileHeader.RecLen) 
	    {
	    mssError(1, "DBL", "Table '%s' physical record length %d is not shorter than logical record length %d",
		    tdata->Name, tdata->FileHeader.PhysLen, tdata->FileHeader.RecLen);
	    return -1;
	    }
	if (tdata->FileHeader.RecLen + tdata->FileHeader.RecPrelen != tdata->FileHeader.PhysLen) 
	    {
	    mssError(1, "DBL", "Table '%s' physical record length %d does not equal logical record length %d plus record control bytes length %d",
		    tdata->Name, tdata->FileHeader.PhysLen, tdata->FileHeader.RecLen, tdata->FileHeader.RecPrelen);
	    return -1;
	    }

	/** Check key offsets and lengths in header **/
	for(i=0;i<tdata->FileHeader.NumKeys;i++)
	    {
	    key = &(tdata->FileHeader.Keys[i]);
	    for(j=0;j<8;j++) /* max. eight segments */
		{
		if (key->SegmentOffset[j] > max_offset) 
		    { 
		    mssError(1, "DBL", "Table '%s', key '%.15s' segment %d has bad offset %d",
			    tdata->Name, key->Name, j, key->SegmentOffset[j]);
		    return -1;
		    }
		if (key->SegmentLength[j] + key->SegmentOffset[j] > max_offset) 
		    {
		    mssError(1, "DBL", "Table '%s', key '%.15s' segment %d has bad length %d",
			    tdata->Name, key->Name, j, key->SegmentLength[j]);
		    return -1;
		    }
		}
	    }

	/** For keys, verify bytemap against file header **/
	if (!(tdata->Node->Flags & DBL_NODE_F_IGNKEYNAMES))
	    {
	    for(i=0;i<tdata->nColumns;i++)
		{
		column = tdata->Columns[i];
		if (column->Flags & DBL_COL_F_KEY)
		    {
		    key = &(tdata->FileHeader.Keys[column->KeyID]);
		    if (dbl_internal_VerifyByteMap(key, column, 1) < 0)
			{
			mssError(0, "DBL", "Failed to verify byte map for table '%s'", tdata->Name);
			return -1;
			}
		    }
		}
	    }
	else
	    {
	    /** Match keys and columns based on bytemap alone **/
	    for(i=0;i<tdata->FileHeader.NumKeys;i++)
		{
		key = &(tdata->FileHeader.Keys[i]);
		found = -1;
		for(j=0;j<tdata->nColumns;j++)
		    {
		    column = tdata->Columns[j];
		    if (column->Flags & DBL_COL_F_KEY) continue;
		    if (dbl_internal_VerifyByteMap(key, column, 0) == 0)
			{
			column->Flags |= DBL_COL_F_KEY;
			column->KeyID = i;
			found = j;
			break;
			}
		    }
		if (found < 0)
		    {
		    mssError(1, "DBL", "Table '%s': could not find a column in the DEF that matched key id %d (%.15s)",
			    tdata->Name, i, key->Name);
		    return -1;
		    }
		if (i==0) tdata->PriKey = found;
		}
	    }

	/** Make sure we have a primary key **/
	if (tdata->PriKey < 0)
	    {
	    mssError(1, "DBL", "Table '%s': could not find a primary key", tdata->Name);
	    return -1;
	    }

    return 0;
    }


/*** dbl_internal_LoadDef() - load in the definition of a table from the 
 *** current mtlexer session, positioned at the top of the definition.
 ***/
int
dbl_internal_LoadDef(pDblData inf, pDblTableInf tdata, pLxSession lxs)
    {
    int t;
    char* line;
    char* ptr;
    pDblColInf column;
    char* semi_ptr = NULL;
    char* end_ptr = NULL;
    char* comma_ptr = NULL;
    char* label = NULL;
    char* end_label = NULL;
    char* ad_ptr = NULL;
    char* len_ptr = NULL;
    char* at_ptr = NULL;
    char* at_field_ptr = NULL;
    char* at_offset_ptr = NULL;
    char* end_comment_ptr = NULL;
    char* format_ptr = NULL;
    char* format_end_ptr;
    char* x_ptr;
    char* decimal_ptr;
    int in_overlay = 0;
    int cur_offset = 0;
    int cur_col = 0;
    char atfield[32];
    int i;

	/** Loop through input file **/
	tdata->nColumns = 0;
	while((t = mlxNextToken(lxs)) != MLX_TOK_EOF && t != MLX_TOK_ERROR)
	    {
	    at_offset_ptr = x_ptr = semi_ptr = at_ptr = comma_ptr = label = at_field_ptr = end_comment_ptr = format_ptr = format_end_ptr = NULL;
	    line = mlxStringVal(lxs, NULL);
	    if (strchr(line,'\r')) *(strchr(line,'\r')) = '\0';
	    if (strchr(line,'\n')) *(strchr(line,'\n')) = '\0';
	    if (line[0] == ';' || line[0] == '\0' || line[0] == '.') continue;
	    end_ptr = line + strlen(line);
	    label = line;									/* beginning of line */
	    while(isspace(*label)) label++;							/* [whitespace] */
	    if (isalpha(*label))								/* label (alpha + alnum) */
		{
		end_label = label;
		while(isalnum(*end_label)) end_label++;
		}
	    else if (*label == ',')
		{
		/* no label */
		end_label = label;
		}
	    else
		label = NULL;
	    if (label)
		{
		ptr = end_label;
		while(isspace(*ptr)) ptr++;							/* [whitespace] */
		if (*ptr == ',')
		    {
		    comma_ptr = ptr;								/* , */
		    ptr++;
		    while(isspace(*ptr)) ptr++;							/* [whitespace] */
		    if (*ptr == 'A' || *ptr == 'a' || *ptr == 'D' || *ptr == 'd')		/* A, a, D, or d */
			{
			ad_ptr = ptr;
			ptr++;
			if (isdigit(*ptr))
			    {
			    len_ptr = ptr;							/* number (length) */
			    while(isdigit(*ptr)) ptr++;
			    while(isspace(*ptr)) ptr++;						/* [whitespace] */
			    at_ptr = NULL;
			    at_field_ptr = NULL;
			    if (*ptr == '@')							/* @ sign (optional) */
				{
				/* virtual */
				at_ptr = ptr;
				ptr++;
				while(isspace(*ptr)) ptr++;					/* [whitespace] */
				if (isalpha(*ptr))						/* @ followed by field name */
				    {
				    at_field_ptr = ptr;
				    while(isalnum(*ptr)) ptr++;
				    while(isspace(*ptr)) ptr++;					/* [whitespace] */
				    }
				}
			    if (*ptr == '+')							/* + */
				{
				/* offset */
				ptr++;
				while(isspace(*ptr)) ptr++;					/* [whitespace] */
				if (isdigit(*ptr))						/* number (offset) */
				    {
				    at_offset_ptr = ptr;
				    while(isdigit(*ptr)) ptr++;
				    while(isspace(*ptr)) ptr++;					/* [whitespace] */
				    }
				}
			    if (*ptr == ';')							/* ; */
				{
				semi_ptr = ptr;
				while(isalnum(*ptr) || ispunct(*ptr) || (isspace(*ptr) && !isspace(*(ptr+1))))
				    ptr++;
				end_comment_ptr = ptr;						/* textual comment string */
				while(isspace(*ptr)) ptr++;					/* [whitespace] */
				if (*ptr == 'X' || *ptr == 'Z')
				    {
				    format_ptr = ptr;
				    while(*ptr == 'X' || *ptr == 'Z' || ispunct(*ptr)) ptr++;	/* XXXZZZ,XXXZZZ etc. format */
				    format_end_ptr = ptr;
				    }
				}
			    }
			}
		    else if (*ptr == 'X' || *ptr == 'x')					/* X instead of A or D after , */
			{
			if (!isalnum(*(ptr+1))) x_ptr = ptr;
			ptr++;
			}

		    /** Check for common,x or record,x for beginning of keys **/
		    if ((!strncasecmp(label,"common",6) || !strncasecmp(label,"record",6)) && x_ptr)
			{
			in_overlay = 1;
			cur_offset = 0;
			cur_col = 0;
			continue;
			}

		    /** Ok, got at least a label and a comma, maybe more.  It's a field. **/
		    column = (pDblColInf)nmMalloc(sizeof(DblColInf));
		    if (!column) return -1;
		    memset(column, 0, sizeof(DblColInf));
		    if (end_label - label >= sizeof(column->Name)) end_label = label + sizeof(column->Name)-1;
		    memcpy(column->Name, label, end_label - label);
		    column->Name[end_label - label] = '\0';
		    if (ad_ptr && (*ad_ptr == 'D' || *ad_ptr == 'd')) 
			column->Flags |= DBL_COL_F_NUMERIC;
		    if (in_overlay || at_ptr)
			column->Flags |= DBL_COL_F_VIRTUAL;
		    if (len_ptr)
			column->Length = strtoul(len_ptr, NULL, 10);
		    if (semi_ptr && end_comment_ptr && end_comment_ptr > semi_ptr)
			{
			if (end_comment_ptr - (semi_ptr+1) >= sizeof(column->Description)) end_comment_ptr = (semi_ptr+1) + sizeof(column->Description)-1;
			memcpy(column->Description, semi_ptr+1, end_comment_ptr - (semi_ptr+1));
			column->Description[end_comment_ptr - (semi_ptr+1)] = '\0';
			}
		    if (format_ptr && format_end_ptr && format_end_ptr >= format_ptr)
			{
			if (format_end_ptr - format_ptr >= sizeof(column->Format)) format_end_ptr = format_ptr + sizeof(column->Format)-1;
			memcpy(column->Format, format_ptr+1, format_end_ptr - (format_ptr+1));
			column->Format[format_end_ptr - (format_ptr+1)] = '\0';

			/** Is there a decimal point? **/
			if ((decimal_ptr = strrchr(column->Format, '.')))
			    {
			    column->DecimalOffset = strlen(decimal_ptr+1);
			    }
			}
		    if (column->Flags & DBL_COL_F_VIRTUAL)
			{
			if (at_field_ptr)
			    {
			    /** find the field that's being talked about **/
			    ptr = atfield;
			    while(isalnum(*at_field_ptr) && ptr < atfield + sizeof(atfield) - 1) 
				*(ptr++) = *(at_field_ptr++);
			    *ptr = '\0';
			    for(i=0;i<tdata->nColumns;i++) if (!strcmp(atfield, tdata->Columns[i]->Name))
				{
				column->StartCol = i;
				cur_col = i;
				break;
				}
			    if (at_offset_ptr)
				column->Offset = strtoul(at_offset_ptr, NULL, 10);
			    cur_offset = column->Length + column->Offset;
			    }
			else
			    {
			    /** no field name.  Try and piggy back off of previous virtual field, if one. **/
			    if (tdata->nColumns == 0 || !(tdata->Columns[tdata->nColumns-1]->Flags & DBL_COL_F_VIRTUAL))
				{
				/** First key or virtual.  Start at beginning of record again. **/
				column->StartCol = 0;
				cur_col = 0;
				column->Offset = 0;
				cur_offset = column->Length;
				}
			    else
				{
				/** Previous virtual is there; tag onto the end of it. **/
				column->StartCol = cur_col;
				column->Offset = cur_offset;
				cur_offset += column->Length;
				}
			    }
			}
		    else
			{
			column->Offset = cur_offset;
			cur_offset += column->Length;
			cur_col++;
			}

		    /** Determine column data type **/
		    if (!(column->Flags & DBL_COL_F_NUMERIC))
			{
			column->Type = DATA_T_STRING;
			}
		    else if (!*(column->Format) && column->Length <= 9)
			{
			column->Type = DATA_T_INTEGER;
			}
		    else if (strchr(column->Format, '/') || strchr(column->Format, ':'))
			{
			column->Type = DATA_T_DATETIME;
			}
		    else if ((strstr(column->Format, ".XX") == column->Format+strlen(column->Format)-3 ||
		              strstr(column->Format, ".XX-") == column->Format+strlen(column->Format)-4
			     ))
			{
			column->Type = DATA_T_MONEY;
			}
		    else if (column->DecimalOffset > 0)
			{
			column->Type = DATA_T_DOUBLE;
			}
		    else 
			{
			column->Type = DATA_T_STRING;
			}

		    /** Add the column. **/
		    tdata->Columns[tdata->nColumns] = column;
		    tdata->nColumns++;
		    if (tdata->nColumns >= DBL_MAX_COLS) break;
		    }
		else
		    {
		    /* label but no comma - some other definition is here... */
		    break;
		    }
		}
	    }

    return 0;
    }


/*** dbl_internal_ScanDef() - search through one particular object's content
 *** for the definition of the given table.
 ***/
int
dbl_internal_ScanDef(pDblData inf, pDblTableInf tdata, pObject obj)
    {
    pLxSession lxs;
    char cpattern[80];
    regex_t pattern;
    regmatch_t matches[1];
    int t;
    char* ptr;
    int rval = -1;

	/** Open a lexer session on the .def file **/
	lxs = mlxGenericSession(obj, objRead, MLX_F_LINEONLY | MLX_F_EOF);
	if (!lxs) return -1;

	/** setup the match pattern. **/
	snprintf(cpattern, sizeof(cpattern), "^\\(common\\|record\\)[ \t]*\\(%s\\)", tdata->Name);
	if (regcomp(&pattern, cpattern, REG_ICASE) != 0)
	    {
	    mlxCloseSession(lxs);
	    return -1;
	    }

	/** Read a line at a time, looking for the definition. **/
	while((t = mlxNextToken(lxs)) != MLX_TOK_EOF && t != MLX_TOK_ERROR)
	    {
	    ptr = mlxStringVal(lxs, NULL);
	    if (regexec(&pattern, ptr, 1, matches, 0) == 0 && !isalnum(ptr[matches[0].rm_eo+1]))
		{
		/** found it! **/
		rval = dbl_internal_LoadDef(inf, tdata, lxs);
		break;
		}
	    }

	/** clean up **/
	regfree(&pattern);
	mlxCloseSession(lxs);

    return rval;
    }


/*** dbl_internal_FindDefinition() - tries to locate the definition of a given
 *** DBL table, and if it finds it, it loads it into the tdata.
 ***/
int
dbl_internal_FindDefinition(pDblData inf, pDblTableInf tdata)
    {
    pObject obj,subobj;
    pObjQuery qy;

	/** First, look for a DEF file with the same name. **/
	obj = objOpen(inf->Obj->Session, tdata->DefPath, O_RDONLY, 0600, "application/octet-stream");
	if (obj)
	    {
	    if (dbl_internal_ScanDef(inf, tdata, obj) == 0)
		{
		objClose(obj);
		return 0;
		}
	    objClose(obj);
	    }

	/** Not there.  Look in all DEF files in the DEF file directory (ack!) **/
	obj = objOpen(inf->Obj->Session, inf->Node->DefPath, O_RDONLY, 0600, "system/directory");
	if (!obj) return -1;
	qy = objOpenQuery(obj, "lower(right(:name, 4)) == '.def'", NULL, NULL, NULL);
	if (!qy)
	    {
	    objClose(obj);
	    return -1;
	    }
	while((subobj = objQueryFetch(qy, O_RDONLY)) != NULL)
	    {
	    if (dbl_internal_ScanDef(inf, tdata, subobj) == 0)
		{
		objClose(subobj);
		objQueryClose(qy);
		objClose(obj);
		return 0;
		}
	    objClose(subobj);
	    }
	objQueryClose(qy);
	objClose(obj);

    return -1;
    }


/*** dbl_internal_GetTData() - opens the DBL isam file and definition file,
 *** reading in the table metadata.
 ***/
pDblTableInf
dbl_internal_GetTData(pDblData inf)
    {
    pDblTableInf tdata;
    int i;
    pDblColInf column;

	/** just aint gonna do it **/
	if (inf->Type == DBL_T_DATABASE) return NULL;

	/** Try looking it up in the node. **/
	tdata = (pDblTableInf)xhLookup(&(inf->Node->TablesByName), inf->TablePtr);
	if (tdata) 
	    {
	    /** Did underlying files change?  Check header consistency if so. **/
	    if (dbl_internal_Revalidate(inf->Obj->Session, tdata) < 0)
		return NULL;
	    return tdata;
	    }

	/** Make a new tdata **/
	tdata = (pDblTableInf)nmMalloc(sizeof(DblTableInf));
	if (!tdata) return NULL;
	memset(tdata, 0, sizeof(tdata));
	memccpy(tdata->Name, inf->TablePtr, 0, sizeof(tdata->Name)-1);
	tdata->Name[sizeof(tdata->Name)-1] = '\0';
	tdata->Node = inf->Node;
	tdata->PriKey = -1;

	/** Build the data, index, and def paths **/
	if (tdata->Name[0] >= 'A' && tdata->Name[0] <= 'Z')
	    {
	    /** upper case conventions **/
	    snprintf(tdata->DataPath, OBJSYS_MAX_PATH, "%s/%s.IS1?ls__type=application%%2foctet-stream", inf->Node->IsmPath, tdata->Name);
	    snprintf(tdata->IndexPath, OBJSYS_MAX_PATH, "%s/%s.ISM?ls__type=application%%2foctet-stream", inf->Node->IsmPath, tdata->Name);
	    snprintf(tdata->DefPath, OBJSYS_MAX_PATH, "%s/%s.DEF?ls__type=application%%2foctet-stream", inf->Node->DefPath, tdata->Name);
	    }
	else
	    {
	    /** lower case conventions **/
	    snprintf(tdata->DataPath, OBJSYS_MAX_PATH, "%s/%s.is1?ls__type=application%%2foctet-stream", inf->Node->IsmPath, tdata->Name);
	    snprintf(tdata->IndexPath, OBJSYS_MAX_PATH, "%s/%s.ism?ls__type=application%%2foctet-stream", inf->Node->IsmPath, tdata->Name);
	    snprintf(tdata->DefPath, OBJSYS_MAX_PATH, "%s/%s.def?ls__type=application%%2foctet-stream", inf->Node->DefPath, tdata->Name);
	    }

	/** Easy part.  Try to open the data file and read its header **/
	if (dbl_internal_ReadHeader(inf->Obj->Session, tdata->DataPath, &(tdata->FileHeader), NULL) < 0)
	    {
	    mssError(0, "DBL", "Could not open/read underlying datafile for table '%s'", tdata->Name);
	    nmFree(tdata, sizeof(DblTableInf));
	    return NULL;
	    }
	if (dbl_internal_ReadHeader(inf->Obj->Session, tdata->IndexPath, &(tdata->IndexHeader), NULL) < 0)
	    {
	    mssError(0, "DBL", "Could not open/read underlying index file for table '%s'", tdata->Name);
	    nmFree(tdata, sizeof(DblTableInf));
	    return NULL;
	    }

	/** Verify file header magic number **/
	if (tdata->FileHeader.Magic != DBL_MAGIC)
	    {
	    mssError(1, "DBL", "File header magic number 0x%4.4x is unsupported, refusing to open table '%s'", 
		    tdata->FileHeader.Magic, tdata->Name);
	    nmFree(tdata, sizeof(DblTableInf));
	    return NULL;
	    }
	if (tdata->IndexHeader.Magic != DBL_INDEX_MAGIC)
	    {
	    mssError(1, "DBL", "Index header magic number 0x%4.4x is unsupported, refusing to open table '%s'", 
		    tdata->IndexHeader.Magic, tdata->Name);
	    nmFree(tdata, sizeof(DblTableInf));
	    return NULL;
	    }

	/** Hard part.  Go hunting for the file's definition. **/
	if (dbl_internal_FindDefinition(inf, tdata) < 0)
	    {
	    mssError(1, "DBL", "Could not locate definition for table '%s'", tdata->Name);
	    nmFree(tdata, sizeof(DblTableInf));
	    return NULL;
	    }

	/** Verify file's definition against the file header. **/
	if (dbl_internal_VerifyDefinition(tdata) < 0)
	    {
	    mssError(1, "DBL", "Definition and ISAM file header are inconsistent for table '%s'", tdata->Name);
	    dbl_internal_FreeTData(tdata);
	    return NULL;
	    }

	xhInit(&(tdata->Files), 15, sizeof(pObjSession));

	/** How much memory is required for parsed data for a given record? **/
	tdata->ColMemory = 0;
	for(i=0;i<tdata->nColumns;i++)
	    {
	    column = tdata->Columns[i];
	    column->ParsedOffset = tdata->ColMemory;

	    /** The ObjData part **/
	    tdata->ColMemory += sizeof(ObjData);

	    /** Additional storage needed for string, money, datetime **/
	    switch(column->Type)
		{
		case DATA_T_STRING:
		    tdata->ColMemory += (column->Length + 1);
		    break;
		case DATA_T_DATETIME:
		    tdata->ColMemory += sizeof(DateTime);
		    break;
		case DATA_T_MONEY:
		    tdata->ColMemory += sizeof(MoneyType);
		    break;
		default:
		    break;
		}

	    /** Round it off **/
	    tdata->ColMemory = ((tdata->ColMemory + 0x3) & (~0x3));
	    }

	/** Add it to the list of tables for this node **/
	xhAdd(&(inf->Node->TablesByName), tdata->Name, (void*)tdata);
	xaAddItem(&(inf->Node->TablesList), (void*)tdata);

    return tdata;
    }


/*** dbl_internal_DetermineType - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
dbl_internal_DetermineType(pObject obj, pDblData inf)
    {
    int i;

	/** Determine object type (depth) and get pointers set up **/
	obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
	for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = 0;
	inf->TablePtr = NULL;
	inf->TableSubPtr = NULL;
	inf->RowColPtr = NULL;

	/** Set up pointers based on number of elements past the node object **/
	if (inf->Pathname.nElements == obj->SubPtr)
	    {
	    inf->Type = DBL_T_DATABASE;
	    obj->SubCnt = 1;
	    }
	if (inf->Pathname.nElements - 1 >= obj->SubPtr)
	    {
	    inf->Type = DBL_T_TABLE;
	    inf->TablePtr = inf->Pathname.Elements[obj->SubPtr];
	    obj->SubCnt = 2;
	    }
	if (inf->Pathname.nElements - 2 >= obj->SubPtr)
	    {
	    inf->TableSubPtr = inf->Pathname.Elements[obj->SubPtr+1];
	    if (!strncmp(inf->TableSubPtr,"rows",4)) inf->Type = DBL_T_ROWSOBJ;
	    else if (!strncmp(inf->TableSubPtr,"columns",7)) inf->Type = DBL_T_COLSOBJ;
	    else
		{
		mssError(1, "DBL", "Object '%s' does not exist", inf->TableSubPtr);
		return -1;
		}
	    obj->SubCnt = 3;
	    }
	if (inf->Pathname.nElements - 3 >= obj->SubPtr)
	    {
	    inf->RowColPtr = inf->Pathname.Elements[obj->SubPtr+2];
	    if (inf->Type == DBL_T_ROWSOBJ) inf->Type = DBL_T_ROW;
	    else if (inf->Type == DBL_T_COLSOBJ) inf->Type = DBL_T_COLUMN;
	    obj->SubCnt = 4;
	    }

    return 0;
    }


/*** dbl_internal_DetermineNodeType() - determine the node type from the 
 *** provided systype.
 ***/
int
dbl_internal_DetermineNodeType(pDblData inf, pContentType systype)
    {
    if (!strcmp(systype->Name, "application/dbl")) return DBL_NODE_T_NODE;
    else if (!strcmp(systype->Name, "application/dbl-data")) return DBL_NODE_T_DATA;
    else if (!strcmp(systype->Name, "application/dbl-index")) return DBL_NODE_T_INDEX;
    else if (!strcmp(systype->Name, "application/dbl-definition")) return DBL_NODE_T_DEFINITION;
    return -1;
    }


/*** dbl_internal_OpenNode() - locate and load the values from the DEF file
 *** containing the definitions for this table.
 ***/
pDblNode
dbl_internal_OpenNode(pDblData inf, pContentType systype)
    {
    pDblNode node;
    /*int i;
    pDblSearchPathItem search_path;
    pObject def_file = NULL;
    pLxSession lxs;
    char dummybuf[1];
    char* linebuf = NULL;
    int t,n;*/
    char* ptr;
    int is_new_node;

	/** First, see if it is already loaded. **/
	ptr = obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->SubPtr);
	node = (pDblNode)xhLookup(&DBL_INF.DBNodes, ptr);
	if (node)
	    {
	    if (node->Serial == snGetSerial(node->SnNode))
		return node;
	    is_new_node = 0;
	    }
	else
	    {
	    /** Make a new node **/
	    node = (pDblNode)nmMalloc(sizeof(DblNode));
	    if (!node) return NULL;
	    memset(node, 0, sizeof(DblNode));
	    strcpy(node->Path, ptr);
	    is_new_node = 1;

	    /** Determine its type **/
	    node->Type = dbl_internal_DetermineNodeType(inf, systype);
	    if (node->Type != DBL_NODE_T_NODE)
		{
		mssError(0, "DBL", "Direct OSML access to DBL files not currently supported.");
		if (is_new_node) nmFree(node, sizeof(DblNode));
		return NULL;
		}

	    /** Access the DB node. **/
	    node->SnNode = snReadNode(inf->Obj->Prev);
	    if (!node->SnNode)
		{
		mssError(0, "DBL", "Could not read DBL file group node!");
		nmFree(node, sizeof(DblNode));
		return NULL;
		}
	    }
	node->Serial = snGetSerial(node->SnNode);

	/** Lookup definition file and isam file paths **/
	if (stGetAttrValue(stLookup(node->SnNode->Data, "isam_path"), DATA_T_STRING, POD(&ptr), 0) != 0)
	    ptr = obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->SubPtr-1)+1;
	snprintf(node->IsmPath, sizeof(node->IsmPath), "%s", ptr);
	if (stGetAttrValue(stLookup(node->SnNode->Data, "def_path"), DATA_T_STRING, POD(&ptr), 0) != 0)
	    ptr = obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->SubPtr-1)+1;
	snprintf(node->DefPath, sizeof(node->DefPath), "%s", ptr);
	if (stGetAttrValue(stLookup(node->SnNode->Data, "description"), DATA_T_STRING, POD(&ptr), 0) != 0)
	    ptr = "";
	snprintf(node->Description, sizeof(node->Description), "%s", ptr);
	if (stGetAttrValue(stLookup(node->SnNode->Data, "ignore_key_names"), DATA_T_STRING, POD(&ptr), 0) == 0 && !strcasecmp(ptr,"yes"))
	    node->Flags |= DBL_NODE_F_IGNKEYNAMES;

	/** Init the hashtable and xarray for the tables in this node **/
	xhInit(&(node->TablesByName), 255, 0);
	xaInit(&(node->TablesList), 127);

	/** Add the node to the table **/
	if (is_new_node)
	    xhAdd(&DBL_INF.DBNodes, node->Path, (void*)node);

    return node;

#if 0
	/** If not, we need to track it down.  Try each of the paths in the
	 ** search listing.
	 **/
	for(i=0;i<DBL_INF.nSearchItems;i++)
	    {
	    search_path = DBL_INF.SearchItems[i];
	    def_file = dbl_internal_FindDefFile(inf->Obj->Pathname, search_path);
	    if (def_file) break;
	    }
	if (!def_file)
	    {
	    mssError(1,"DBL","Could not locate definition file for object '%s'", tablename);
	    goto error;
	    }

	/** Ok, found a DEF file containing the given table definition.
	 ** Load in the definition.  Use a dummy objRead() to seek to the
	 ** beginning of the object content.
	 **/
	linebuf = nmMalloc(512);
	if (!linebuf) goto error;
	objRead(def_file, dummybuf, 0, 0, FD_U_SEEK);
	lxs = mlxGenericSession(def_file, objRead, MLX_F_LINEONLY | MLX_F_EOF);
	if (!lxs) goto error;
	while ((t = mlxNextToken(lxs)) != MLX_TOK_EOF && t != MLX_TOK_ERROR)
	    {
	    mlxCopyToken(lxs, linebuf, 512);
	    }

    error:
	if (linebuf) nmFree(linebuf, 512);
	if (lxs) mlxCloseSession(lxs);
	if (def_file) objClose(def_file);
	return NULL;
#endif
    }


/*** dbl_internal_ParseOneDefItem() - breaks down one DEF search path item
 *** and returns a structure describing it.  Modifies the item string in place
 *** to allow the building of the structure.  Structure becomes invalid if the
 *** item string is released or otherwise modified somehow.
 ***/
pDblSearchPathItem
dbl_internal_ParseOneDefItem(char* itemstring)
    {
    pDblSearchPathItem item = NULL;
    char* sepptr;
    char* whitespaceptr;

	/** Alloc the item **/
	item = (pDblSearchPathItem)nmMalloc(sizeof(DblSearchPathItem));
	if (!item) return NULL;
	item->SrcPattern = NULL;
	item->DefPattern = NULL;
	item->SearchRegex = NULL;

	/** Parse source (.IS1) pattern **/
	item->SrcPattern = itemstring;
	sepptr = strchr(item->SrcPattern,'=');
	if (!sepptr) goto error;
	*sepptr = '\0';
	whitespaceptr = sepptr-1;
	while (whitespaceptr > itemstring && *whitespaceptr == ' ') *(whitespaceptr--) = '\0';
	whitespaceptr = sepptr+1;
	while (*whitespaceptr == ' ') *(whitespaceptr++) = '\0';

	/** Parse destination (.DEF) pattrn **/
	item->DefPattern = whitespaceptr;
	sepptr = strchr(item->DefPattern,'(');
	if (!sepptr) goto error;
	*sepptr = '\0';
	whitespaceptr = sepptr-1;
	while (whitespaceptr > itemstring && *whitespaceptr == ' ') *(whitespaceptr--) = '\0';
	whitespaceptr = sepptr+1;
	while (*whitespaceptr == ' ') *(whitespaceptr++) = '\0';

	/** Parse search regex in parentheses **/
	item->SearchRegex = whitespaceptr;
	sepptr = strchr(item->SearchRegex,')');
	if (!sepptr) goto error;
	*sepptr = '\0';
	whitespaceptr = sepptr-1;
	while (whitespaceptr > itemstring && *whitespaceptr == ' ') *(whitespaceptr--) = '\0';

    return item;

    /** Error exit handler **/
    error:
	if (item) nmFree(item,sizeof(DblSearchPathItem));
	return NULL;
    }


/*** dbl_internal_ParseDefPathItems() - parses a definition search path into
 *** multiple definition search items.  Loads a copy of the defpath into the
 *** globals, and modifies that copy to break it up into components which can
 *** be linked to from search path item structures, loaded also into the
 *** globals for this module.
 ***/
int
dbl_internal_ParseDefPathItems(char* defpath)
    {
    char* itemptr;
    char* defcopy;
    char* semiptr;
    pDblSearchPathItem item;

	/** Make the copy of the string so we can work on it. **/
	defcopy = nmSysStrdup(defpath);
	if (!defcopy) return -1;
	DBL_INF.DefSearchPath = defcopy;

	/** Break it up into semicolon-separated items **/
	itemptr = defcopy;
	while(itemptr && *itemptr && DBL_INF.nSearchItems < DBL_MAX_PATH_ITEMS)
	    {
	    semiptr = strchr(itemptr,';');
	    if (semiptr) *(semiptr++) = '\0';
	    item = dbl_internal_ParseOneDefItem(itemptr);
	    if (item)
	        DBL_INF.SearchItems[DBL_INF.nSearchItems++] = item;
	    itemptr = semiptr;
	    }

    return 0;
    }


/*** dbl_internal_MappedCopy() - copy data from a DBL record through a
 *** column's byte map into the target area, and null-terminate it.
 ***/
int
dbl_internal_MappedCopy(char* target, int target_size, pDblColInf column, char* row_data)
    {
    int i;

	/** enough room? **/
	if (target_size < column->Length + 1) return -1;

	/** copy **/
	if (column->Flags & DBL_COL_F_SEGMENTED)
	    {
	    for(i=0;i<column->Length;i++)
		target[i] = row_data[column->ByteMap[i]];
	    target[i] = '\0';
	    }
	else
	    {
	    memcpy(target, row_data+column->ByteMap[0], column->Length);
	    target[column->Length] = '\0';
	    }

    return 0;
    }


/*** dbl_internal_ParseColumn() - parses one column from the DBL record
 *** into the Centrallix data type.
 ***/
int
dbl_internal_ParseColumn(pDblColInf column, pObjData pod, char* data, char* row_data)
    {
    char ibuf[32];
    char dtbuf[32];
    unsigned long long v;
    int i,f;
    double decimalOffsetValue = 10000;

	switch(column->Type)
	    {
	    case DATA_T_INTEGER:
		if (dbl_internal_MappedCopy(ibuf, sizeof(ibuf), column, row_data) < 0) return -1;
		pod->Integer = strtoi(ibuf, NULL, 10);
		break;
	    case DATA_T_STRING:
		pod->String = data;
		if (dbl_internal_MappedCopy(data, column->Length+1, column, row_data) < 0) return -1;
		break;
	    case DATA_T_DATETIME:
		pod->DateTime = (pDateTime)data;
		if (dbl_internal_MappedCopy(dtbuf, sizeof(dtbuf), column, row_data) < 0) return -1;
		if (objDataToDateTime(DATA_T_STRING, dtbuf, pod->DateTime, NULL) < 0) return -1;
		break;
	    case DATA_T_DOUBLE:
		if (dbl_internal_MappedCopy(ibuf, sizeof(ibuf), column, row_data) < 0) return -1;
		pod->Double = strtol(ibuf, NULL, 10);
		if (column->DecimalOffset) pod->Double /= pow(10, column->DecimalOffset);
		break;
	    case DATA_T_MONEY:
            //decimalOffsetValue is originally 10000 to convert v to 10000ths of a dollar
            //decimalOffsetValue is divided by 10, column->DecimalOffset times,
            //keeping the decimal as a double in case it drops below 0
            //Finally, I multiple v by decimalOffsetValue to get my Money->Value
            if (dbl_internal_MappedCopy(ibuf, sizeof(ibuf), column, row_data) < 0) return -1;
            v = strtoll(ibuf, NULL, 10);
            decimalOffsetValue /= pow(10, column->DecimalOffset);
            pod->Money = (pMoneyType)data;
            pod->Money->Value = (v*decimalOffsetValue)+ 0.1;
            break;
	    default:
		mssError(1, "DBL", "Bark!  Unhandled data type for column '%s'", column->Name);
		return -1;
	    }

    return 0;
    }



/*** dbl_internal_ParseColN() - parse one column
 ***/
int
dbl_internal_ParseColN(pDblData inf, int col_num)
    {
    pDblColInf column;
    pObjData pod;
    char* data;
    pDblTableInf tdata = inf->TData;
    int rval;

	column = tdata->Columns[col_num];
	pod = (pObjData)(inf->ParsedData + column->ParsedOffset);
	data = (inf->ParsedData + column->ParsedOffset + sizeof(ObjData));
	rval = dbl_internal_ParseColumn(column, pod, data, inf->RowData + tdata->FileHeader.RecPrelen);
	if (rval == 0) inf->ColFlags[col_num] |= DBL_DATA_COL_F_PARSED;

    return rval;
    }


/*** dbl_internal_TestParseColN() - parse one column unless already done.
 ***/
int
dbl_internal_TestParseColN(pDblData inf, int col_num)
    {
    int rval;

	if (inf->ColFlags[col_num] & DBL_DATA_COL_F_PARSED) return 0;
	rval = dbl_internal_ParseColN(inf, col_num);
	if (rval == 0) inf->ColFlags[col_num] |= DBL_DATA_COL_F_PARSED;

    return rval;
    }


/*** dbl_internal_ParseRow() - parse the DBL record into the Centrallix
 *** data types.
 ***/
int
dbl_internal_ParseRow(pDblData inf)
    {
    int i;

	/** Allocate the memory **/
	if (!inf->ParsedData)
	    {
	    inf->ParsedData = (void*)nmSysMalloc(inf->TData->ColMemory);
	    if (!inf->ParsedData) return -1;
	    }

	/** For each of the columns, build a POD and, optionally, data **/
	/*for(i=0;i<inf->TData->nColumns;i++)
	    {
	    if (dbl_internal_TestParseColN(inf, i) < 0)
		return -1;
	    }*/

    return 0;
    }


/*** dbl_internal_ReadRow() - read a single row of data from the data
 *** file at a given offset.  If the data file is not open, open it
 *** before reading and close it when we're done.
 ***/
int
dbl_internal_ReadRow(pDblData inf, unsigned int offset)
    {
    pDblOpenFiles files;
    int rval;

	/** Revalidate? **/
	if ((rval = dbl_internal_Revalidate(inf->Obj->Session, inf->TData)) < 0)
	    return -1;
	if (rval == 1)
	    {
	    /** modified, forget row **/
	    if (inf->RowData) inf->Offset = 0;
	    }

	/** Already there? **/
	if (inf->RowData && inf->Offset == offset) return 0;

	/** Gain access to underlying files **/
	files = dbl_internal_OpenFiles(inf->Obj->Session, inf->TData);
	if (!files) return -1;

	/** Read the record in from the given offset **/
	if (!inf->RowData)
	    {
	    inf->RowData = (char*)nmSysMalloc(inf->TData->FileHeader.PhysLen);
	    if (!inf->RowData)
		{
		dbl_internal_CloseFiles(files);
		return -1;
		}
	    }
	rval = objRead(files->DataObject, inf->RowData, inf->TData->FileHeader.PhysLen, offset, OBJ_U_SEEK | OBJ_U_PACKET);
	if (rval != inf->TData->FileHeader.PhysLen)
	    {
	    dbl_internal_CloseFiles(files);
	    return -1;
	    }

	/** Parse the thing. **/
	if (dbl_internal_ParseRow(inf) < 0) return -1;
	dbl_internal_CloseFiles(files);

	inf->Offset = offset;

    return 0;
    }


/*** dblOpen - open a table, row, or column.
 ***/
void*
dblOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pDblData inf;

	/** Allocate the structure **/
	inf = (pDblData)nmMalloc(sizeof(DblData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(DblData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine type and set pointers. **/
	if (dbl_internal_DetermineType(obj,inf) < 0)
	    {
	    nmFree(inf,sizeof(DblData));
	    return NULL;
	    }

	/** Open the node object **/
	inf->Node = dbl_internal_OpenNode(inf,systype);
	if (!inf->Node)
	    {
	    nmFree(inf,sizeof(DblData));
	    return NULL;
	    }

	/** Verify the table, if a table mentioned **/
	if (inf->TablePtr)
	    {
	    if (strpbrk(inf->TablePtr," \t\r\n"))
		{
		mssError(1,"DBL","Requested table '%s' is invalid", inf->TablePtr);
		nmFree(inf,sizeof(DblData));
		return NULL;
		}
	    inf->TData = dbl_internal_GetTData(inf);
	    if (!inf->TData)
		{
		mssError(1,"DBL","Requested table '%s' could not be accessed", inf->TablePtr);
		nmFree(inf,sizeof(DblData));
		return NULL;
		}
	    }

	/** Lookup the row, if one was mentioned **/
	if (inf->Type == DBL_T_ROW)
	    {
	    }
	
    return (void*)inf;
    }

#if 0
/*** dbl_internal_InsertRow - inserts a new row into the database, looking
 *** throught the OXT structures for the column values.  For text/image columns,
 *** it automatically inserts "" for an unspecified column value when the column
 *** does not allow nulls.
 ***/
int
dbl_internal_InsertRow(pDblData inf, pObjTrxTree oxt)
    {
    char* kptr;
    char* kendptr;
    int i,j,len,ctype,restype;
    pObjTrxTree attr_oxt, find_oxt;
    pXString insbuf;
    char* tmpptr;
    char tmpch;

        /** Ok, look for the attribute sub-OXT's **/
        for(j=0;j<inf->TData->nColumns;j++)
            {
	    /** If primary key, we have values in inf->RowColPtr. **/
	    if (inf->TData->ColFlags[j] & DBL_CF_PRIKEY)
	        {
		/** Determine position,length within prikey-coded name **/
		kptr = inf->RowColPtr;
		for(i=0;i<inf->TData->ColKeys[j] && kptr != (char*)1;i++) kptr = strchr(kptr,'|')+1;
		if (kptr == (char*)1)
		    {
		    mssError(1,"DBL","Not enough components in concat primary key (name)");
		    xsDeInit(insbuf);
		    nmFree(insbuf,sizeof(XString));
		    return -1;
		    }
		kendptr = strchr(kptr,'|');
		if (!kendptr) len = strlen(kptr); else len = kendptr-kptr;
		}
	    else
	        {
		/** Otherwise, we scan through the OXT's **/
                find_oxt=NULL;
                for(i=0;i<oxt->Children.nItems;i++)
                    {
                    attr_oxt = ((pObjTrxTree)(oxt->Children.Items[i]));
                    /*if (((pDblData)(attr_oxt->LLParam))->Type == DBL_T_ATTR)*/
		    if (attr_oxt->OpType == OXT_OP_SETATTR)
                        {
                        if (!strcmp(attr_oxt->AttrName,inf->TData->Cols[j]))
                            {
                            find_oxt = attr_oxt;
                            find_oxt->Status = OXT_S_COMPLETE;
                            break;
                            }
                        }
                    }
                }
	    }

    return 0;
    }
#endif


/*** dblClose - close an open file or directory.
 ***/
int
dblClose(void* inf_v, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);

    	/** Was this a create? **/
	if ((*oxt) && (*oxt)->OpType == OXT_OP_CREATE && (*oxt)->Status != OXT_S_COMPLETE)
	    {
	    switch (inf->Type)
	        {
		case DBL_T_TABLE:
		    /** We'll get to this a little later **/
		    break;

		case DBL_T_ROW:
		    /** Complete the oxt. **/
		    (*oxt)->Status = OXT_S_COMPLETE;

		    break;

		case DBL_T_COLUMN:
		    /** We wait until table is done for this. **/
		    break;
		}
	    }

	/** Free the info structure **/
	if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
	if (inf->RowData) nmSysFree(inf->RowData);
	if (inf->ParsedData) nmSysFree(inf->ParsedData);
	nmFree(inf,sizeof(DblData));

    return 0;
    }


/*** dblCreate - create a new object without actually opening that 
 *** object.
 ***/
int
dblCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* objd;

    	/** Open and close the object **/
	obj->Mode |= O_CREAT;
	objd = dblOpen(obj,mask,systype,usrtype,oxt);
	if (!objd) return -1;

    return dblClose(objd,oxt);
    }


/*** dblDelete - delete an existing object.
 ***/
int
dblDelete(pObject obj, pObjTrxTree* oxt)
    {
    pDblData inf;

	/** Allocate the structure **/
	inf = (pDblData)nmMalloc(sizeof(DblData));
	if (!inf) return -1;
	memset(inf,0,sizeof(DblData));
	inf->Obj = obj;

	/** Determine type and set pointers. **/
	dbl_internal_DetermineType(obj,inf);

	/** If a row, proceed else fail the delete. **/
	if (inf->Type != DBL_T_ROW)
	    {
	    nmFree(inf,sizeof(DblData));
	    puts("Unimplemented delete operation in DBL.");
	    mssError(1,"DBL","Unimplemented delete operation in DBL");
	    return -1;
	    }

	/** Access the DB node. **/

	/** Free the structure **/
	nmFree(inf,sizeof(DblData));

    return 0;
    }


/*** dblRead - read from the object's content.  Unsupported on these files.
 ***/
int
dblRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pDblData inf = DBL(inf_v);*/

    return -1;
    }


/*** dblWrite - write to an object's content.  Unsupported for these types of things.
 ***/
int
dblWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pDblData inf = DBL(inf_v);*/

    return -1;
    }


/*** dblOpenQuery - open a directory query.  We basically reformat the where clause
 *** and issue a query to the DB.
 ***/
void*
dblOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    pDblQuery qy;

	/** Allocate the query structure **/
	qy = (pDblQuery)nmMalloc(sizeof(DblQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(DblQuery));
	qy->ObjInf = inf;
	qy->RowCnt = -1;
	qy->ObjSession = NULL;

	/** State that we won't do full query unless we get to DBL_T_ROWSOBJ below **/
	query->Flags &= ~OBJ_QY_F_FULLQUERY;
	query->Flags &= ~OBJ_QY_F_FULLSORT;

	/** Build the query SQL based on object type. **/
	switch(inf->Type)
	    {
	    case DBL_T_DATABASE:
	        /** Select the list of tables from the DB. **/
		qy->LLObj = objOpen(inf->Obj->Session, inf->Node->IsmPath, O_RDONLY, 0600, "system/directory");
		if (!qy->LLObj) 
		    {
		    nmFree(qy,sizeof(DblQuery));
		    return NULL;
		    }
		qy->LLQuery = objOpenQuery(qy->LLObj, "lower(right(:name, 4)) == '.ism'", ":name", NULL, NULL);
		if (!qy->LLQuery)
		    {
		    objClose(qy->LLObj);
		    nmFree(qy,sizeof(DblQuery));
		    return NULL;
		    }
		qy->RowCnt = 0;
		break;

	    case DBL_T_TABLE:
	        /** No SQL needed -- always returns just 'columns' and 'rows' **/
		qy->RowCnt = 0;
	        break;

	    case DBL_T_COLSOBJ:
	        /** Get a columns list. **/
		qy->TableInf = qy->ObjInf->TData;
		qy->RowCnt = 0;
		break;

	    case DBL_T_ROWSOBJ:
	        /** Query the rows within a table -- iteration here. **/
		qy->RowCnt = 0;
		qy->Files = dbl_internal_OpenFiles(inf->Obj->Session, inf->TData);
		break;

	    case DBL_T_COLUMN:
	    case DBL_T_ROW:
	        /** These don't support queries for sub-objects. **/
	        nmFree(qy,sizeof(DblQuery));
		qy = NULL;
		break;
	    }

    return (void*)qy;
    }


/*** dblQueryFetch - get the next directory entry as an open object.
 ***/
void*
dblQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pDblQuery qy = ((pDblQuery)(qy_v));
    pDblData inf;
    char filename[120];
    char* ptr;
    int new_type;
    int i,cnt;
    pDblTableInf tdata = qy->ObjInf->TData;
    int restype;
    pObject ll_obj;
    pDblColInf prikey;

	if (qy->RowCnt < 0) return NULL;

	/** Allocate the structure **/
	inf = (pDblData)nmMalloc(sizeof(DblData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(DblData));
	inf->TData = tdata;
	inf->Obj = obj;

	qy->RowCnt++;

    	/** Get the next name based on the query type. **/
	switch(qy->ObjInf->Type)
	    {
	    case DBL_T_DATABASE:
		ll_obj = objQueryFetch(qy->LLQuery, O_RDONLY);
		if (!ll_obj)
		    {
		    nmFree(inf,sizeof(DblData));
		    return NULL;
		    }
		objGetAttrValue(ll_obj, "name", DATA_T_STRING, POD(&ptr));
		snprintf(filename, sizeof(filename), "%s", ptr);
		i = strlen(filename);
		if (i > 4 && !strncasecmp(filename+i-4,".ism",4)) *(filename+i-4) = '\0';
	        break;

	    case DBL_T_TABLE:
	        /** Filename is either "rows" or "columns" **/
		if (qy->RowCnt == 1) 
		    {
		    strcpy(filename,"columns");
		    new_type = DBL_T_COLSOBJ;
		    }
		else if (qy->RowCnt == 2) 
		    {
		    strcpy(filename,"rows");
		    new_type = DBL_T_ROWSOBJ;
		    }
		else 
		    {
		    nmFree(inf,sizeof(DblData));
		    /*mssError(1,"DBL","Table object has only two subobjects: 'rows' and 'columns'");*/
		    return NULL;
		    }
	        break;

	    case DBL_T_ROWSOBJ:
	        /** Get the filename from the primary key of the row. **/
		new_type = DBL_T_ROW;
		if (dbl_internal_ReadRow(inf, DBL_SEGMENTSIZE + (qy->RowCnt-1) * tdata->FileHeader.PhysLen) < 0)
		    {
		    nmFree(inf,sizeof(DblData));
		    return NULL;
		    }
		prikey = tdata->Columns[tdata->PriKey];
		if (dbl_internal_TestParseColN(inf, tdata->PriKey) < 0)
		    {
		    nmFree(inf,sizeof(DblData));
		    return NULL;
		    }
		switch(prikey->Type)
		    {
		    case DATA_T_INTEGER:
			ptr = objDataToStringTmp(prikey->Type, POD(inf->ParsedData+prikey->ParsedOffset), 0);
			break;
		    case DATA_T_STRING:
			ptr = objDataToStringTmp(prikey->Type, POD(inf->ParsedData+prikey->ParsedOffset)->String, 0);
			break;
		    default:
			ptr = NULL;
			break;
		    }
		if (!ptr || strlen(ptr)+1 > sizeof(filename) || strchr(ptr, '/'))
		    {
		    mssError(1, "DBL", "Could not build filename for primary key on table %s, row #%d",
			    tdata->Name, qy->RowCnt);
		    nmFree(inf,sizeof(DblData));
		    return NULL;
		    }
		strcpy(filename, ptr);
	        break;

	    case DBL_T_COLSOBJ:
	        /** Loop through the columns in the TableInf structure. **/
		new_type = DBL_T_COLUMN;
		if (qy->RowCnt <= qy->TableInf->nColumns)
		    {
		    memccpy(filename,qy->TableInf->Columns[qy->RowCnt-1]->Name, 0, sizeof(filename)-1);
		    filename[sizeof(filename)-1] = '\0';
		    }
		else
		    {
		    nmFree(inf,sizeof(DblData));
		    return NULL;
		    }
	        break;
	    }

	/** Build the filename. **/
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	if ((ptr - obj->Pathname->Pathbuf) + 1 + strlen(filename) >= 255)
	    {
	    mssError(1,"DBL","Pathname too long for internal representation");
	    nmFree(inf,sizeof(DblData));
	    return NULL;
	    }
	*(ptr++) = '/';
	strcpy(ptr,filename);
	obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;

	/** Fill out the remainder of the structure. **/
	inf->Mask = 0600;
	inf->Type = new_type;
	inf->Node = qy->ObjInf->Node;
	obj->SubPtr = qy->ObjInf->Obj->SubPtr;
	dbl_internal_DetermineType(obj,inf);

	if (!inf->TData)
	    {
	    inf->TData = dbl_internal_GetTData(inf);
	    if (!inf->TData)
		{
		mssError(1,"DBL","Requested table '%s' could not be accessed", inf->TablePtr);
		nmFree(inf,sizeof(DblData));
		return NULL;
		}
	    }

    return (void*)inf;
    }


/*** dblQueryDelete - delete the contents of a query result set.  This is
 *** not yet supported.
 ***/
int
dblQueryDelete(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** dblQueryClose - close the query.
 ***/
int
dblQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pDblQuery qy = ((pDblQuery)(qy_v));

	/** Free the structure **/
	if (qy->Files) dbl_internal_CloseFiles(qy->Files);
	nmFree(qy,sizeof(DblQuery));

    return 0;
    }


/*** dblGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
dblGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    int i;
    pDblTableInf tdata;

    	/** Name attribute?  String. **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

    	/** Content-type attribute?  String. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** Annotation?  String. **/
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Attr type depends on object type. **/
	if (inf->Type == DBL_T_ROW)
	    {
	    tdata = inf->TData;
	    for(i=0;i<tdata->nColumns;i++)
	        {
		if (!strcmp(attrname,tdata->Columns[i]->Name))
		    {
		    return tdata->Columns[i]->Type;
		    }
		}
	    }
	else if (inf->Type == DBL_T_COLUMN)
	    {
	    if (!strcmp(attrname,"datatype")) return DATA_T_STRING;
	    }

	mssError(1,"DBL","Invalid column for GetAttrType");

    return -1;
    }


/*** dblGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
dblGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    int i,t,minus,n;
    unsigned int msl,lsl,divtmp;
    pDblTableInf tdata;
    char* ptr;
    int days,fsec;
    float f;
    pObjData srcpod;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DBL","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    ptr = inf->Pathname.Elements[inf->Pathname.nElements-1];
	    if (ptr[0] == '.' && ptr[1] == '\0')
	        {
	        val->String = "/";
		}
	    else
	        {
	        val->String = ptr;
		}
	    return 0;
	    }

	/** Is it an annotation? **/
	if (!strcmp(attrname, "annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DBL","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    /** Different for various objects. **/
	    switch(inf->Type)
	        {
		case DBL_T_DATABASE:
		    val->String = inf->Node->Description;
		    break;
		case DBL_T_TABLE:
		    val->String = inf->TData->Annotation;
		    break;
		case DBL_T_ROWSOBJ:
		    val->String = "Contains rows for this table";
		    break;
		case DBL_T_COLSOBJ:
		    val->String = "Contains columns for this table";
		    break;
		case DBL_T_COLUMN:
		    val->String = "Column within this table";
		    break;
		case DBL_T_ROW:
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
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DBL","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    switch(inf->Type)
	        {
		case DBL_T_DATABASE: *((char**)val) = "system/void"; break;
		case DBL_T_TABLE: *((char**)val) = "system/void"; break;
		case DBL_T_ROWSOBJ: *((char**)val) = "system/void"; break;
		case DBL_T_COLSOBJ: *((char**)val) = "system/void"; break;
		case DBL_T_ROW: 
		    {
		    if (inf->TData->HasContent)
		        val->String = "application/octet-stream";
		    else
		        val->String = "system/void";
		    break;
		    }
		case DBL_T_COLUMN: val->String = "system/void"; break;
		}
	    return 0;
	    }

	/** Outer type... **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DBL","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    switch(inf->Type)
	        {
		case DBL_T_DATABASE: val->String = "application/dbl"; break;
		case DBL_T_TABLE: val->String = "system/table"; break;
		case DBL_T_ROWSOBJ: val->String = "system/table-rows"; break;
		case DBL_T_COLSOBJ: val->String = "system/table-columns"; break;
		case DBL_T_ROW: val->String = "system/row"; break;
		case DBL_T_COLUMN: val->String = "system/column"; break;
		}
	    return 0;
	    }

	/** Column object?  Type is the only one. **/
	if (inf->Type == DBL_T_COLUMN)
	    {
	    if (strcmp(attrname,"datatype")) return -1;

	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DBL","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    /** Get the table info. **/
	    tdata=inf->TData;

	    /** Search table info for this column. **/
	    for(i=0;i<tdata->nColumns;i++) if (!strcmp(tdata->Columns[i]->Name,inf->RowColPtr))
	        {
		val->String = obj_type_names[tdata->Columns[i]->Type];
		return 0;
		}
	    }
	else if (inf->Type == DBL_T_ROW)
	    {
	    /** Get the table info. **/
	    tdata = inf->TData;

	    /** Search through the columns. **/
	    for(i=0;i<tdata->nColumns;i++) if (!strcmp(tdata->Columns[i]->Name,attrname))
	        {
		t = tdata->Columns[i]->Type;
		if (datatype != t)
		    {
		    mssError(1,"DBL","Type mismatch accessing attribute '%s' [requested=%s, actual=%s]", 
			    attrname, obj_type_names[datatype], obj_type_names[t]);
		    return -1;
		    }
		srcpod = (pObjData)(inf->ParsedData + tdata->Columns[i]->ParsedOffset);
		if (dbl_internal_TestParseColN(inf, i) < 0) return -1;
		objCopyData(srcpod, val, t);
		return 0;
		}
	    }

	mssError(1,"DBL","Invalid column for GetAttrValue");

    return -1;
    }


/*** dblGetNextAttr - get the next attribute name for this object.
 ***/
char*
dblGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    pDblTableInf tdata;

	/** Attribute listings depend on object type. **/
	switch(inf->Type)
	    {
	    case DBL_T_DATABASE:
	        return NULL;
	
	    case DBL_T_TABLE:
	        return NULL;

	    case DBL_T_ROWSOBJ:
	        return NULL;

	    case DBL_T_COLSOBJ:
	        return NULL;

	    case DBL_T_COLUMN:
	        /** only attr is 'datatype' **/
		if (inf->CurAttr++ == 0) return "datatype";
	        break;

	    case DBL_T_ROW:
	        /** Get the table info. **/
		tdata = inf->TData;

	        /** Return attr in table inf **/
		if (inf->CurAttr < tdata->nColumns) return tdata->Columns[inf->CurAttr++]->Name;
	        break;
	    }

    return NULL;
    }


/*** dblGetFirstAttr - get the first attribute name for this object.
 ***/
char*
dblGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = dblGetNextAttr(inf_v,oxt);

    return ptr;
    }


/*** dblSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
dblSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    int type,rval;
    char sbuf[160];
    char* ptr;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DBL","Type mismatch setting attribute '%s' [should be string]", attrname);
		return -1;
		}
	    if (inf->Type == DBL_T_DATABASE) return -1;
	    }

	/** Changing the 'annotation'? **/
	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"DBL","Type mismatch setting attribute '%s' [should be string]", attrname);
		return -1;
		}
	    /** Choose the appropriate action based on object type **/
	    switch(inf->Type)
	        {
		case DBL_T_DATABASE:
		    memccpy(inf->Node->Description, val?(val->String):"", '\0', 255);
		    inf->Node->Description[255] = 0;
		    break;
		    
		case DBL_T_TABLE:
		    memccpy(inf->TData->Annotation, val?(val->String):"", '\0', 255);
		    inf->TData->Annotation[255] = 0;
		    while(strchr(inf->TData->Annotation,'"')) *(strchr(inf->TData->Annotation,'"')) = '\'';
		    break;

		case DBL_T_ROWSOBJ:
		case DBL_T_COLSOBJ:
		case DBL_T_COLUMN:
		    /** Can't change any of these (yet) **/
		    return -1;

		case DBL_T_ROW:
		    /** Not yet implemented :) **/
		    return -1;
		}
	    return 0;
	    }

	/** If this is a row, check the OXT. **/
	if (inf->Type == DBL_T_ROW)
	    {
	    /** Attempting to set 'suggested' size for content write? **/
	    if (!strcmp(attrname,"size")) 
	        {
		if (datatype != DATA_T_INTEGER)
		    {
		    mssError(1,"DBL","Type mismatch setting attribute '%s' [should be integer]", attrname);
		    return -1;
		    }
		if (!val)
		    {
		    mssError(1,"DBL","'size' property cannot be NULL");
		    return -1;
		    }
		inf->Size = val->Integer;
		}
	    else
	        {
	        /** Otherwise, check Oxt. **/
	        if (*oxt)
	            {
		    /** We're within a transaction.  Fill in the oxt. **/
		    type = dblGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
		    if (datatype != type)
			{
			mssError(1,"DBL","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
				attrname, obj_type_names[datatype], obj_type_names[type]);
			return -1;
			}
		    (*oxt)->AllocObj = 0;
		    (*oxt)->Object = NULL;
		    (*oxt)->Status = OXT_S_VISITED;
		    if (strlen(attrname) >= 64)
			{
			mssError(1,"DBL","Attribute name '%s' too long",attrname);
			return -1;
			}
		    strcpy((*oxt)->AttrName, attrname);
		    obj_internal_SetTreeAttr(*oxt, type, val);
		    }
	        else
	            {
		    /** No transaction.  Simply do an update. **/
		    type = dblGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
		    if (datatype != type)
			{
			mssError(1,"DBL","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
				attrname, obj_type_names[datatype], obj_type_names[type]);
			return -1;
			}
		    }
		}
	    }
	else
	    {
	    /** Don't support setattr on anything else yet. **/
	    return -1;
	    }

    return 0;
    }


/*** dblAddAttr - add an attribute to an object.  This doesn't work for
 *** unix filesystem objects, so we just deny the request.
 ***/
int
dblAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** dblOpenAttr - open an attribute as if it were an object with 
 *** content.  The Sybase database objects don't yet have attributes that are
 *** suitable for this.
 ***/
void*
dblOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** dblGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
dblGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** dblGetNextMethod -- same as above.  Always fails. 
 ***/
char*
dblGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** dblExecuteMethod - No methods to execute, so this fails.
 ***/
int
dblExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree* oxt)
    {
    return -1;
    }



/*** dblInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
dblInitialize()
    {
    pObjDriver drv;
    int i;
#if 00
    pQueryDriver qdrv;
#endif

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&DBL_INF,0,sizeof(DBL_INF));
	xhInit(&DBL_INF.DBNodes,255,0);
	xaInit(&DBL_INF.DBNodeList,127);
	DBL_INF.DblConfig = stLookup(CxGlobals.ParsedConfig,"dbl");
	DBL_INF.nSearchItems = 0;
	memset(&dataindex_hdr_mask, 0, sizeof(DblHeader));
	dataindex_hdr_mask.NumKeys	= 0xFF;
	dataindex_hdr_mask.RecPrelen	= 0xFF;
	dataindex_hdr_mask.KeyLen	= 0xFFFF;
	dataindex_hdr_mask.RecLen	= 0xFFFF;
	dataindex_hdr_mask.PhysLen	= 0xFFFF;
	memcpy(dataindex_hdr_mask.Version, "\xff\xff\xff\xff\xff\xff", 6);
	for(i=0;i<8;i++)
	    {
	    dataindex_hdr_mask.Keys[i].RecsPerPage = 0xFF;
	    memset(dataindex_hdr_mask.Keys[i].SegmentOffset, 0xFF, sizeof(dataindex_hdr_mask.Keys[i].SegmentOffset));
	    memset(dataindex_hdr_mask.Keys[i].SegmentLength, 0xFF, sizeof(dataindex_hdr_mask.Keys[i].SegmentLength));
	    dataindex_hdr_mask.Keys[i].IndexRecLen = 0xFFFF;
	    memset(dataindex_hdr_mask.Keys[i].Name, 0xFF, sizeof(dataindex_hdr_mask.Keys[i].Name));
	    dataindex_hdr_mask.Keys[i].Zero = 0xFF;
	    }

	/** Setup the structure **/
	strcpy(drv->Name,"DBL - DBL ISAM File Driver");
	drv->Capabilities = OBJDRV_C_FULLQUERY | OBJDRV_C_TRANS;
	/*drv->Capabilities = OBJDRV_C_TRANS;*/
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/dbl");
	xaAddItem(&(drv->RootContentTypes),"application/dbl-data");
	xaAddItem(&(drv->RootContentTypes),"application/dbl-index");
	xaAddItem(&(drv->RootContentTypes),"application/dbl-definition");

	/** Setup the function references. **/
	drv->Open = dblOpen;
	drv->Close = dblClose;
	drv->Create = dblCreate;
	drv->Delete = dblDelete;
	drv->OpenQuery = dblOpenQuery;
	drv->QueryDelete = dblQueryDelete;
	drv->QueryFetch = dblQueryFetch;
	drv->QueryClose = dblQueryClose;
	drv->Read = dblRead;
	drv->Write = dblWrite;
	drv->GetAttrType = dblGetAttrType;
	drv->GetAttrValue = dblGetAttrValue;
	drv->GetFirstAttr = dblGetFirstAttr;
	drv->GetNextAttr = dblGetNextAttr;
	drv->SetAttrValue = dblSetAttrValue;
	drv->AddAttr = dblAddAttr;
	drv->OpenAttr = dblOpenAttr;
	drv->GetFirstMethod = dblGetFirstMethod;
	drv->GetNextMethod = dblGetNextMethod;
	drv->ExecuteMethod = dblExecuteMethod;

	nmRegister(sizeof(DblTableInf),"DblTableInf");
	nmRegister(sizeof(DblData),"DblData");
	nmRegister(sizeof(DblQuery),"DblQuery");
	nmRegister(sizeof(DblNode),"DblNode");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;
	DBL_INF.ObjDriver = drv;

    return 0;
    }


MODULE_INIT(dblInitialize);
MODULE_PREFIX("dbl");
MODULE_DESC("DBL ISAM File ObjectSystem Driver");
MODULE_VERSION(0,9,0);
MODULE_IFACE(CX_CURRENT_IFACE);
