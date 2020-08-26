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
#include <assert.h>

#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
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
/* Copyright (C) 2002-2006 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_fp.c         					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 8th, 2006					*/
/* Description:	Objectsystem driver for FilePro files.  The node for	*/
/*		the group of filepro files points to a directory which	*/
/*		contains the files, and all such files are made		*/
/*		available.						*/
/************************************************************************/



/*** Module controls ***/
#define FP_FORCE_READONLY	1	/* not update safe yet!!! */
#define FP_INDEX_MAGIC		0xc139
#define FP_MAX_COLS		640
#define FP_SEGMENTSIZE		0x0400
#define FP_NAME_MAX		64	/* max len name for cols, tables, formats */
#define FP_MAX_INDICES		('Z' - '0' + 1) 
#define FP_MAX_INDEX_PARTS	16
#define FP_MAX_FILES		16
#define FP_KEY_KEYFILE		'*'
#define FP_KEY_MAPFILE		'?'
#define FP_KEY_UNSET		'\0'
#define FP_MAX_FORMATS		256

#define	FP_COL_F_FULLINDEX	1	/* an index exists on this col. */
#define	FP_COL_F_PARTINDEX	2	/* a partial index exists on this col */
#define FP_COL_F_ARRAY		4	/* this is an array column */

#define FP_DATA_COL_F_PARSED	1

#define FP_RECORD_S_VALID	1
#define FP_RECORD_S_DELETED	0


/*** FilePro Index Header format.
 ***/
typedef struct
    {
    CXUINT16		Magic;		/* index file magic # */
    CXUINT16		Levels;		/* number of levels in index tree */
    CXINT8		__a[4];
    CXUINT32		PageSize;	/* size of pages */
    CXUINT32		TopSegment;	/* topmost index segment */
    CXUINT32		IndexedValues;
    CXUINT16		ValueLength;
    CXUINT16		Flags;
    CXUINT16		IndexedField;	/* first field is #1, not #0 */
    CXINT8		__b[2];
    CXUINT16		FieldLength;
    CXINT8		__c[2];
    }
    FpIndexHdrSegment, *pFpIndexHdrSegment;


/*** FilePro Index Leaf format
 ***/
typedef struct
    {
    CXUINT32		Prev;		/* previous leaf page */
    CXUINT32		Next;		/* next leaf page */
    CXUINT16		__a;
    CXUINT16		nEntries;	/* number of ptrs in the leaf page */
    /* followed by N entries of text followed by row id */
    }
    FpIndexLeafSegment, *pFpIndexLeafSegment;


/*** FilePro Index Node format
 ***/
typedef struct
    {
    CXUINT16		nEntries;	/* number of entries in index page */
    /* followed by alternating page id and entry */
    }
    FpIndexNodeSegment, *pFpIndexNodeSegment;


/*** FilePro Key Header
 ***/
typedef struct
    {
    CXINT8		__a[6];
    CXINT8		nRecords[4];	/* an array due to alignment issues */
    CXINT8		__b[10];
    }
    FpKeyHdr, *pFpKeyHdr;


/*** FilePro Key Record Metadata
 ***/
typedef union
    {
    struct
	{
	CXINT8		RecordStatus;
	CXINT8		__a[1];
	CXINT16		CreateDate;
	CXINT16		CreateUID;
	CXINT16		UpdateDate;
	CXINT16		UpdateProcUID;
	CXINT16		ProcDate;
	CXINT8		__b[8];
	}	Valid;
    struct
	{
	CXINT8		RecordStatus;
	CXINT8		__a[1];
	CXINT8		PrevFree[4];
	CXINT8		NextFree[4];
	CXINT8		__b[10];
	}	Free;
    struct
	{
	CXINT8		RecordStatus;
	CXINT8		__a[19];
	}	Any;
    }
    FpRecData, *pFpRecData;


/*** FormatTree - required for defining a Format ***/
typedef struct _FT
    {
    struct _FT*		Parent;
    struct _FT*		Next;
    struct _FT*		ChildList;
    int			Type;			/* FP_FMTTREE_T_xxx */
    int			Flags;			/* FP_FMTTREE_F_xxx */
    char*		RefName;
    struct _FT*		RefTree;
    char*		Chomp;			/* match chars from input */
    char*		MoreChomp;		/* used for ranges of values */
    char*		Spew;			/* output chars */
    }
    FpFormatTree, *pFpFormatTree;

#define	FP_FMTTREE_T_REF	1
#define FP_FMTTREE_T_STRING	2
#define FP_FMTTREE_T_OR		3
#define FP_FMTTREE_T_CONCAT	4

#define FP_FMTTREE_F_NOOUTPUT	1		/* match only */
#define FP_FMTTREE_F_OPTIONAL	2
#define FP_FMTTREE_F_REPEAT	4		/* one or more of contents */
#define FP_FMTTREE_F_MATCHANY	8		/* wildcard match from src */
#define FP_FMTTREE_F_UPCASE	16		/* store only uppercase */
#define FP_FMTTREE_F_LOCASE	32		/* store only lowercase */
#define FP_FMTTREE_F_ICASE	64		/* case insensitive match */
#define FP_FMTTREE_F_BACKSLASH	128		/* function currently unknown */


/*** Format information ***/
typedef struct
    {
    char		Name[FP_NAME_MAX];
    char*		RawText;
    int			DataType;		/* DATA_T_xxx */
    pObjPresentationHints Hints;
    }
    FpFormat, *pFpFormat;


/*** Column information ***/
typedef struct
    {
    char		Name[FP_NAME_MAX];
    char		ArrayName[FP_NAME_MAX];	/* for array cols */
    int			Length;
    int			ElementNum;		/* for array columns */
    char		FormatText[FP_NAME_MAX];
    pFpFormat		Format;
    int			DecimalOffset;
    int			ParsedOffset;
    int			RecordOffset;
    int			Type;			/* centrallix data type */
    }
    FpColInf, *pFpColInf;


/*** Index information ***/
typedef struct
    {
    char		ID;
    int			nParts;
    int			ColIDs[FP_MAX_INDEX_PARTS];
    int			StartAt[FP_MAX_INDEX_PARTS];
    int			Lengths[FP_MAX_INDEX_PARTS];
    FpIndexHdrSegment	IndexHdr;
    }
    FpIndexInf, *pFpIndexInf;


/*** Node ***/
typedef struct
    {
    char		Path[OBJSYS_MAX_PATH];
    char		Description[256];
    char		DataPath[OBJSYS_MAX_PATH];
    char		EditsPath[OBJSYS_MAX_PATH];
    XArray		FormatsList;
    XHashTable		FormatsByName;
    pSnNode		Node;
    int			NodeSerial;
    XArray		TablesList;
    XHashTable		TablesByName;
    }
    FpNode, *pFpNode;


/*** Table information ***/
typedef struct
    {
    char		Name[FP_NAME_MAX];
    pFpNode		Node;
    int			nColumns;
    pFpColInf		Columns[FP_MAX_COLS];
    int			ColMemory;
    int			nPriKeyCols;
    int			PriKeyColID[FP_MAX_COLS];
    int			nIndices;
    pFpIndexInf		Indices[FP_MAX_INDICES];
    XHashTable		OpenFiles;
    char		RawPath[OBJSYS_MAX_PATH];
    char		MapPath[OBJSYS_MAX_PATH];
    char		KeyPath[OBJSYS_MAX_PATH];
    FpKeyHdr		FileHeader;
    int			nRecords;
    pParamObjects	ObjList;
    pExpression		RowAnnotExpr;
    char		Annotation[256];
    int			PhysLen;
    char		RawName[FP_NAME_MAX];
    char		Qualifier[10];
    }
    FpTableInf, *pFpTableInf;


/*** Open files ***/
typedef struct
    {
    pObjSession		OSMLSession;
    int			LinkCnt;
    pFpTableInf		TData;
    pObject		FileList[FP_MAX_FILES];
    unsigned char	FileKeys[FP_MAX_FILES];
    char		TmpPath[OBJSYS_MAX_PATH];
    }
    FpOpenFiles, *pFpOpenFiles;


/*** Structure used by this driver internally for open objects ***/
typedef struct 
    {
    pFpNode		Node;
    pFpTableInf		TData;
    Pathname		Pathname;
    int			Type;
    pObject		Obj;
    int			Mask;
    int			CurAttr;
    char*		TablePtr;
    char*		TableSubPtr;
    char*		RowColPtr;
    int			RowID;
    unsigned char*	ParsedData;
    unsigned char*	RowData;
    unsigned char	ColFlags[FP_MAX_COLS];
    unsigned int	Offset;
    pFpOpenFiles	OpenFiles;
    FpRecData		RowHeader;
    DateTime		CreateDate;
    DateTime		ModifyDate;
    DateTime		ProcessDate;
    }
    FpData, *pFpData;


#define FP_T_DATABASE		1
#define FP_T_TABLE		2
#define FP_T_ROWSOBJ		3
#define FP_T_COLSOBJ		4
#define FP_T_ROW		5
#define FP_T_COLUMN		6

#define FP(x) ((pFpData)(x))


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pFpData	ObjInf;
    pFpTableInf TableInf;
    pObjSession	ObjSession;
    int		RowCnt;
    int		LLRowCnt;
    pObject	LLObj;
    pObjQuery	LLQuery;
    int		LLRowCnt2;
    pObject	LLObj2;
    pObjQuery	LLQuery2;
    }
    FpQuery, *pFpQuery;


char* fp_special_attr[] =
    {
    "__fp_create_date",
    "__fp_create_uid",
    "__fp_process_date",
    "__fp_modify_date",
    "__fp_modify_uid",
    "last_modification",
    };

int fp_special_type[] =
    {
    DATA_T_DATETIME,
    DATA_T_INTEGER,
    DATA_T_DATETIME,
    DATA_T_DATETIME,
    DATA_T_INTEGER,
    DATA_T_DATETIME,
    };


/*** GLOBALS ***/
struct
    {
    XHashTable		DBNodes;
    XArray		DBNodeList;
    pStructInf		FpConfig;
    pObjDriver		ObjDriver;
    }
    FP_INF;


/*** fp_internal_Timestamp() - get a current timestamp, in the 
 *** resolution of the CLK_TCK times() counter.  This is used for
 *** finding out when we need to grab a last_modification on the
 *** underlying file to see if it has changed, and revalidate if
 *** needed.
 ***/
clock_t
fp_internal_Timestamp()
    {
    clock_t t;
    struct tms tms;

	t = times(&tms);

    return t;
    }


/*** fp_internal_StampToDate() - take a date stamp in a filepro
 *** record metadata block and convert it into a DateTime value.
 ***/
int
fp_internal_StampToDate(CXINT16 stamp, pDateTime dt)
    {
    int n_days;

	/** no negative values ... **/
	dt->Value = 0;
	if (stamp < 0) return -1;

	/** year **/
	dt->Part.Year = 83;
	while(1)
	    {
	    n_days = ((dt->Part.Year % 4) == 0 && (dt->Part.Year % 400) != 0)?366:365;
	    if (stamp >= n_days)
		{
		stamp -= n_days;
		dt->Part.Year++;
		}
	    else
		break;
	    }

	/** month **/
	while(1)
	    {
	    switch(dt->Part.Month)
		{
		case 3:		/* apr */
		case 5:		/* jun */
		case 8:		/* sep */
		case 10:	/* nov */
		    n_days = 30;
		    break;
		case 1:		/* feb */
		    n_days = ((dt->Part.Year % 4) == 0 && (dt->Part.Year % 400) != 0)?29:28;
		    break;
		default:
		    n_days = 31;
		    break;
		}
	    if (stamp >= n_days)
		{
		stamp -= n_days;
		dt->Part.Month++;
		}
	    else
		break;
	    }

	/** Day **/
	dt->Part.Day = stamp;

    return 0;
    }


/*** fp_internal_CloseFiles() - close up all files in the
 *** open files structure that are open, and free the open
 *** files structure.
 ***/
int
fp_internal_CloseFiles(pFpOpenFiles of)
    {
    int i;

	/** don't need to close 'em yet? **/
	if (--of->LinkCnt) return 0;

	/** Close the files & free **/
	for(i=0; i<FP_MAX_FILES; i++)
	    {
	    if (of->FileList[i])
		objClose(of->FileList[i]);
	    of->FileList[i] = NULL;
	    of->FileKeys[i] = 0;
	    }
	xhRemove(&of->TData->OpenFiles, (void*)(of->OSMLSession));
	nmFree(of, sizeof(FpOpenFiles));

    return 0;
    }


/*** fp_internal_FreeTData() - release table info structure.
 ***/
void
fp_internal_FreeTData(pFpTableInf tdata)
    {
    int i;

	/** free columns **/
	for(i=0;i<tdata->nColumns;i++)
	    {
	    nmFree(tdata->Columns[i], sizeof(FpColInf));
	    }

	/** free indices **/
	for(i=0;i<tdata->nIndices;i++)
	    {
	    nmFree(tdata->Indices[i], sizeof(FpIndexInf));
	    }

	/** free the tdata itself **/
	xhClear(&(tdata->OpenFiles), NULL, NULL);
	xhDeInit(&(tdata->OpenFiles));
	nmFree(tdata, sizeof(FpTableInf));
	
    return;
    }


/*** fp_internal_OpenFiles() - open the data and index files for a given table
 *** within a given OSML session.
 ***/
pFpOpenFiles
fp_internal_OpenFiles(pFpData inf)
    {
    pFpOpenFiles files;
    pFpTableInf tdata = inf->TData;
    pObjSession s = inf->Obj->Session;

	if (inf->OpenFiles) return inf->OpenFiles;

	/** already open?  cool... **/
	if ((files = (pFpOpenFiles)xhLookup(&(tdata->OpenFiles), (void*)s)) != NULL)
	    {
	    assert(files->LinkCnt > 0);
	    files->LinkCnt++;
	    inf->OpenFiles = files;
	    return files;
	    }

	/** Open the main data file.. **/
	files = (pFpOpenFiles)nmMalloc(sizeof(FpOpenFiles));
	if (!files) return NULL;
	memset(files, 0, sizeof(FpOpenFiles));
	files->OSMLSession = s;
	files->TData = tdata;
	files->FileKeys[0] = FP_KEY_KEYFILE;
	files->FileList[0] = objOpen(s, tdata->KeyPath, O_RDONLY, 0600, "application/octet-stream");
	if (!files->FileList[0])
	    {
	    mssError(0,"FP","Could not open 'key' file for table '%s'", tdata->Name);
	    nmFree(files, sizeof(FpOpenFiles));
	    return NULL;
	    }
	objUnmanageObject(s, files->FileList[0]);

	files->LinkCnt = 1;
	xhAdd(&(tdata->OpenFiles), (void*)(files->OSMLSession), (void*)files);
	inf->OpenFiles = files;

    return files;
    }


/*** fp_internal_GetOpenFile() - open a new file related to this object, and if
 *** it is already open, link to it.
 ***/
pObject
fp_internal_GetOpenFile(pFpData inf, char* filename, unsigned char key)
    {
    pFpOpenFiles files;
    pObject obj = NULL;
    int i, open_idx;

	/** Get list of open files for this object **/
	files = fp_internal_OpenFiles(inf);
	if (!files) return NULL;

	/** is file open yet? **/
	open_idx = -1;
	for(i=0;i<FP_MAX_FILES;i++) 
	    {
	    if (key == files->FileKeys[i])
		{
		obj = files->FileList[i];
		break;
		}
	    else if (!files->FileKeys[i] && open_idx < 0)
		{
		open_idx = i;
		}
	    }
	if (!obj)
	    {
	    if (open_idx < 0)
		{
		mssError(1, "FP", "Bark!  Internal resources exhausted handling open files for table '%s'", inf->TData->Name);
		return NULL;
		}
	    else
		{
		files->FileList[open_idx] = objOpen(inf->Obj->Session, filename, O_RDONLY, 0600, "application/octet-stream");
		objUnmanageObject(inf->Obj->Session, files->FileList[open_idx]);
		if (!files->FileList[open_idx])
		    return NULL;
		files->FileKeys[open_idx] = key;
		}
	    }

    return obj;
    }


/*** fp_internal_ReadHeader() - read the key (data) file header and parse out
 *** its data.
 ***/
int
fp_internal_ReadHeader(pFpData inf, char* key_path)
    {
    pObject keyfile;
    int rcnt;
    CXUINT32 n_records;

	/** Right now only thing being read in is rec count **/
	keyfile = fp_internal_GetOpenFile(inf, key_path, FP_KEY_KEYFILE);
	if (!keyfile) return -1;
	rcnt = objRead(keyfile, (char*)&inf->TData->FileHeader, sizeof(inf->TData->FileHeader), 0, OBJ_U_PACKET | OBJ_U_SEEK);
	if (rcnt != sizeof(inf->TData->FileHeader)) return -1;
	memcpy(&n_records, inf->TData->FileHeader.nRecords, 4);
	inf->TData->nRecords = n_records;

    return 0;
    }


/*** fp_internal_ParseDefinition() - parse the map file column information
 ***/
int
fp_internal_ParseDefinition(pFpTableInf tdata, pLxSession lxs)
    {
    int rval = -1;
    char* ptr;
    char* tptr;
    int n_bytes, n_columns;
    int i,j,k,found;
    int total_len;
    pFpColInf cdata;

	/** Get the 'map:' line **/
	if (mlxNextToken(lxs) != MLX_TOK_STRING) goto error;
	ptr = mlxStringVal(lxs, NULL);
	if (strncmp(ptr, "map:", 4)) goto error;
	tptr = strsep(&ptr, ":");
	tptr = strsep(&ptr, ":");
	if (!tptr) goto error;
	n_bytes = strtoi(tptr, NULL, 10);
	tptr = strsep(&ptr, ":");
	tptr = strsep(&ptr, ":");
	if (!tptr) goto error;
	n_columns = strtoi(tptr, NULL, 10);

	/** Too many columns? **/
	if (n_columns > FP_MAX_COLS)
	    goto error;

	/** Get a line for each column **/
	total_len = 0;
	for(i=0;i<n_columns;i++)
	    {
	    if (mlxNextToken(lxs) != MLX_TOK_STRING) goto error;
	    ptr = mlxStringVal(lxs, NULL);
	    tptr = strsep(&ptr, ":");
	    if (!tptr) goto error;
	    cdata = tdata->Columns[tdata->nColumns] = 
			nmMalloc(sizeof(FpColInf));
	    if (!cdata) goto error;
	    tdata->nColumns++;

	    /** Make sure column name is unique **/
	    strtcpy(cdata->Name, tptr, sizeof(cdata->Name));
	    for(j=0;j<tdata->nColumns-1;j++)
		{
		if (j) snprintf(cdata->Name, sizeof(cdata->Name), "%s.%d", tptr, j);
		found = 0;
		for(k=0;k<tdata->nColumns-1;k++)
		    {
		    if (!strcmp(tdata->Columns[k]->Name, cdata->Name))
			{
			found = 1;
			break;
			}
		    }
		if (!found) break;
		}

	    /** Length and 'edit format' **/
	    tptr = strsep(&ptr,":");
	    if (!tptr) goto error;
	    cdata->Length = strtoi(tptr, NULL, 10);
	    cdata->RecordOffset = total_len;
	    total_len += cdata->Length;
	    tptr = strsep(&ptr,":");
	    if (!tptr) goto error;
	    strtcpy(cdata->FormatText, tptr, sizeof(cdata->FormatText));
	    cdata->Format = NULL;

	    /** The following are temporary until we deduce the data type
	     ** from the format.
	     **/
	    cdata->Type = DATA_T_STRING;
	    cdata->Format = NULL;
	    }
	if (mlxNextToken(lxs) != MLX_TOK_EOF) goto error;

	/** Finish up **/
	tdata->PhysLen = n_bytes;
	rval = 0;

    error:
	if (rval < 0) 
	    mssError(1, "FP", 
		"Map file for table '%s': Invalid Format", tdata->Name);

    return rval;
    }


/*** fp_internal_ReadDefinition() - read the map (definition) file and parse
 *** out the column information.
 ***/
int
fp_internal_ReadDefinition(pFpData inf, char* map_path)
    {
    pObject mapfile = NULL;
    pLxSession lxs = NULL;
    int rval = -1;

	/** Access the map file.  No need to stick it in the open files
	 ** list - it is only needed once.
	 **/
	mapfile = objOpen(inf->Obj->Session, map_path, O_RDONLY, 0600, "application/octet-stream");
	if (!mapfile) goto error;
	objUnmanageObject(inf->Obj->Session, mapfile);

	/** Open it up in the lexer so we can parse it **/
	lxs = mlxGenericSession(mapfile, objRead, MLX_F_EOF | MLX_F_LINEONLY);
	if (!lxs) goto error;

	/** Parse the file **/
	if (fp_internal_ParseDefinition(inf->TData, lxs) < 0) goto error;

	/** Finish up. **/
	rval = 0;

    error:
	if (lxs) mlxCloseSession(lxs);
	if (mapfile) objClose(mapfile);

    return rval;
    }


/*** fp_internal_ResolveEditSyms() - for a given Node, resolve the symbols
 *** used in the edit formats (which often refer to each other).
 ***/
int
fp_internal_ResolveEditSyms(pFpNode node)
    {
    return 0;
    }


/*** fp_internal_ParseOneEdit() - take a given FpFormat structure and its
 *** associated RawText, and build a FpFormatTree representing the format.
 *** Resolution of symbols is left for later.
 ***/
int
fp_internal_ParseOneEdit(pFpFormat fdata)
    {
    return 0;
    }


/*** fp_internal_ReadEditsFile() - read the FilePro 'edits' file to get info
 *** on what data formats / data types are in use
 ***/
int
fp_internal_ReadEditsFile(pFpData inf, char* edits_path)
    {
    pObject editfile = NULL;
    pLxSession lxs = NULL;
    int rval = -1;
    int t;
    pFpFormat fdata;
    char* editstr;
    char* colonptr;

	/** Access the edits file.  No need to stick it in the open files
	 ** list - it is only needed once.
	 **/
	editfile = objOpen(inf->Obj->Session, edits_path, O_RDONLY, 0600, "application/octet-stream");
	if (!editfile) goto error;
	objUnmanageObject(inf->Obj->Session, editfile);
	lxs = mlxGenericSession(editfile, objRead, MLX_F_EOF | MLX_F_LINEONLY);

	while((t=mlxNextToken(lxs)) == MLX_TOK_STRING)
	    {
	    /** Allocate the format information structure **/
	    fdata = (pFpFormat)nmMalloc(sizeof(FpFormat));
	    if (!fdata) goto error;
	    memset(fdata, 0, sizeof(FpFormat));

	    /** Read in each edits line and build a preliminary structure **/
	    editstr = mlxStringVal(lxs, NULL);
	    if (!(colonptr = strchr(editstr, ':'))) 
		{
		mssError(1, "FP", "Edits file '%s' contains edit line without colon.");
		goto error;
		}
	    fdata->RawText = nmSysStrdup(colonptr + 1);
	    *colonptr = '\0';
	    strtcpy(fdata->Name, editstr, sizeof(fdata->Name));
	    if (fp_internal_ParseOneEdit(fdata) < 0) goto error;

	    /** Add it to the list of tables for this node **/
	    xhAdd(&(inf->Node->FormatsByName), fdata->Name, (void*)fdata);
	    xaAddItem(&(inf->Node->FormatsList), (void*)fdata);
	    fdata = NULL;
	    }

	/** Error reading file? **/
	if (t != MLX_TOK_EOF)
	    {
	    mssError(0, "FP", "Error in 'edits' file '%s'", edits_path);
	    goto error;
	    }

	/** Resolve symbols **/
	if (fp_internal_ResolveEditSyms(inf->Node) < 0) goto error;

	/** Ok, let's tail on outta here **/
	rval = 0;

    error:
	if (fdata) nmFree(fdata, sizeof(FpFormat));
	if (lxs) mlxCloseSession(lxs);
	if (editfile) objClose(editfile);

    return rval;
    }


/*** fp_internal_FindIndices() - find any index files and read in the info
 *** on what columns are indexed.
 ***/
int
fp_internal_FindIndices(pFpData inf, char* table_path)
    {
    unsigned char c;
    int i;
    int rcnt;
    int rval = -1;
    char index_path[OBJSYS_MAX_PATH];
    pObject index_obj;
    pFpIndexInf idata;
    pFpTableInf tdata = inf->TData;
    int remaining_length, cur_field;
    pObjQuery qy = NULL;
    pObject qy_obj = NULL;
    pObject fetched_obj = NULL;
    char* name;
    char index_query[128];

	/** Run a query to find the indexes **/
	qy_obj = objOpen(inf->Obj->Session, table_path, O_RDONLY, 0600, "system/directory");
	if (!qy_obj) goto error;
	objUnmanageObject(inf->Obj->Session, qy_obj);
	qpfPrintf(NULL, index_query, sizeof(index_query), 
		"substring(:name,1,%INT) == 'index%STR&ESCQ.' AND char_length(:name) == %INT", 
		6 + strlen(tdata->Qualifier), tdata->Qualifier, 7 + strlen(tdata->Qualifier));
	qy = objOpenQuery(qy_obj, index_query, NULL, NULL, NULL);
	if (!qy) goto error;
	objUnmanageQuery(inf->Obj->Session, qy);
	while((fetched_obj = objQueryFetch(qy, O_RDONLY)))
	    {
	    name = NULL;
	    objGetAttrValue(fetched_obj, "name", DATA_T_STRING, POD(&name));
	    if (!name || strlen(name) != 7 + strlen(tdata->Qualifier)) goto error;
	    c = name[6 + strlen(tdata->Qualifier)];
	    objClose(fetched_obj);
	    fetched_obj = NULL;
	    qpfPrintf(NULL, index_path, sizeof(index_path), "%STR/index%STR.%1STR",
		    table_path, tdata->Qualifier, &c);
	    index_obj = fp_internal_GetOpenFile(inf, index_path, c);
	    if (!index_obj) continue;
	    idata = tdata->Indices[tdata->nIndices++] = 
			nmMalloc(sizeof(FpIndexInf));
	    if (!idata) return -1;
	    idata->ID = c;
	    idata->nParts = 0;
	    rcnt = objRead(index_obj, (char*)&(idata->IndexHdr), sizeof(idata->IndexHdr), 0, OBJ_U_PACKET | OBJ_U_SEEK);
	    if (rcnt != sizeof(idata->IndexHdr)) return -1;
	    if (idata->IndexHdr.Magic != FP_INDEX_MAGIC) 
		{
		mssError(1, "FP", 
			"Invalid index header on index '%c' for table '%s'",
			idata->ID, tdata->Name);
		goto error;
		}

	    /** Figure out the segmentation of the indexed value **/
	    if (idata->IndexHdr.Flags != 0x0004)
	    remaining_length = idata->IndexHdr.ValueLength;
	    cur_field = idata->IndexHdr.IndexedField - 1;
	    while(remaining_length)
		{
		if (cur_field > tdata->nColumns)
		    {
		    idata->nParts = 0;
		    mssError(1, "FP",
			    "Invalid field structure on index '%c' for table '%s'",
			    idata->ID, tdata->Name);
		    goto error;
		    }
		if (remaining_length <= tdata->Columns[cur_field]->Length)
		    idata->Lengths[idata->nParts] = remaining_length;
		else
		    idata->Lengths[idata->nParts] = tdata->Columns[cur_field]->Length;
		idata->StartAt[idata->nParts] = 0;
		idata->ColIDs[idata->nParts] = cur_field;
		remaining_length -= idata->Lengths[idata->nParts];
		cur_field++;
		idata->nParts++;
		}
	    objClose(fetched_obj);
	    fetched_obj = NULL;
	    }

	/** Ok, finish up. **/
	rval = 0;

    error:
	if (fetched_obj) objClose(fetched_obj);
	if (qy) objQueryClose(qy);
	if (qy_obj) objClose(qy_obj);

    return rval;
    }


/*** fp_internal_Revalidate() - make sure schema hasn't changed
 ***/
int
fp_internal_Revalidate(pFpData inf)
    {
    return 0;
    }


/*** fp_internal_GetTData() - opens the filepro data file and map file,
 *** reading in the table metadata.
 ***/
pFpTableInf
fp_internal_GetTData(pFpData inf)
    {
    pFpTableInf tdata = NULL;
    int i;
    pFpColInf column;
    char* ptr;

	/** just aint gonna do it **/
	if (inf->Type == FP_T_DATABASE) return NULL;

	/** Try looking it up in the node. **/
	tdata = (pFpTableInf)xhLookup(&(inf->Node->TablesByName), inf->TablePtr);
	if (tdata) 
	    {
	    /** Did underlying files change?  Check header consistency if so. **/
	    if (fp_internal_Revalidate(inf) < 0)
		return NULL;
	    return tdata;
	    }

	/** Make a new tdata **/
	tdata = (pFpTableInf)nmMalloc(sizeof(FpTableInf));
	if (!tdata) return NULL;
	memset(tdata, 0, sizeof(FpTableInf));
	strtcpy(tdata->Name, inf->TablePtr, sizeof(tdata->Name));
	strtcpy(tdata->RawName, inf->TablePtr, sizeof(tdata->RawName));
	tdata->Node = inf->Node;
	inf->TData = tdata;
	xhInit(&(tdata->OpenFiles), 15, sizeof(pObjSession));

	/** Qualified table? **/
	if ((ptr = strrchr(tdata->RawName, '.')) != NULL)
	    {
	    *ptr = '\0';
	    strtcpy(tdata->Qualifier, ptr+1, sizeof(tdata->Qualifier));
	    }

	/** Build the path **/
	snprintf(tdata->RawPath, OBJSYS_MAX_PATH, "%s/%s", inf->Node->DataPath, tdata->RawName);
	snprintf(tdata->KeyPath, OBJSYS_MAX_PATH, "%s/%s/key%s", inf->Node->DataPath, tdata->RawName, tdata->Qualifier);
	snprintf(tdata->MapPath, OBJSYS_MAX_PATH, "%s/%s/map", inf->Node->DataPath, tdata->RawName);

	/** Easy part.  Try to open the data file and read its header **/
	if (fp_internal_ReadHeader(inf, tdata->KeyPath) < 0)
	    {
	    mssError(0, "FP", "Could not open/read underlying key file for table '%s'", tdata->Name);
	    goto error;
	    }

	/** Read in definition **/
	if (fp_internal_ReadDefinition(inf, tdata->MapPath) < 0)
	    {
	    mssError(0, "FP", "Could not open/read underlying map file for table '%s'", tdata->Name);
	    goto error;
	    }

	/** Find any indexes associated with the table **/
	if (fp_internal_FindIndices(inf, tdata->RawPath) < 0)
	    {
	    mssError(0, "FP", "Error finding indices for table '%s'", tdata->Name);
	    goto error;
	    }

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

    error:
	if (inf->OpenFiles) fp_internal_CloseFiles(inf->OpenFiles);
	inf->OpenFiles = NULL;
	if (tdata) fp_internal_FreeTData(tdata);
	if (inf->TData) inf->TData = NULL;

    return NULL;
    }


/*** fp_internal_DetermineType - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
fp_internal_DetermineType(pObject obj, pFpData inf)
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
	    inf->Type = FP_T_DATABASE;
	    obj->SubCnt = 1;
	    }
	if (inf->Pathname.nElements - 1 >= obj->SubPtr)
	    {
	    inf->Type = FP_T_TABLE;
	    inf->TablePtr = inf->Pathname.Elements[obj->SubPtr];
	    obj->SubCnt = 2;
	    }
	if (inf->Pathname.nElements - 2 >= obj->SubPtr)
	    {
	    inf->TableSubPtr = inf->Pathname.Elements[obj->SubPtr+1];
	    if (!strncmp(inf->TableSubPtr,"rows",4)) inf->Type = FP_T_ROWSOBJ;
	    else if (!strncmp(inf->TableSubPtr,"columns",7)) inf->Type = FP_T_COLSOBJ;
	    else
		{
		mssError(1, "FP", "Object '%s' does not exist", inf->TableSubPtr);
		return -1;
		}
	    obj->SubCnt = 3;
	    }
	if (inf->Pathname.nElements - 3 >= obj->SubPtr)
	    {
	    inf->RowColPtr = inf->Pathname.Elements[obj->SubPtr+2];
	    if (inf->Type == FP_T_ROWSOBJ) inf->Type = FP_T_ROW;
	    else if (inf->Type == FP_T_COLSOBJ) inf->Type = FP_T_COLUMN;
	    obj->SubCnt = 4;
	    }

    return 0;
    }


/*** fp_internal_OpenNode() - open up the structure file pointing to the
 *** directory containing all of the objects.
 ***/
pFpNode
fp_internal_OpenNode(pFpData inf, pContentType systype)
    {
    pFpNode node;
    char* ptr;
    int is_new_node;

	/** First, see if it is already loaded. **/
	ptr = obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->SubPtr);
	node = (pFpNode)xhLookup(&FP_INF.DBNodes, ptr);
	if (node)
	    {
	    if (node->NodeSerial == snGetSerial(node->Node))
		return node;
	    is_new_node = 0;
	    }
	else
	    {
	    /** Make a new node **/
	    node = (pFpNode)nmMalloc(sizeof(FpNode));
	    if (!node) return NULL;
	    memset(node, 0, sizeof(FpNode));
	    strcpy(node->Path, ptr);
	    is_new_node = 1;

	    /** Access the DB node. **/
	    node->Node = snReadNode(inf->Obj->Prev);
	    if (!node->Node)
		{
		mssError(0, "FP", "Could not read FilePro file group node!");
		nmFree(node, sizeof(FpNode));
		return NULL;
		}
	    }
	node->NodeSerial = snGetSerial(node->Node);

	/** Lookup definition file and isam file paths **/
	if (stGetAttrValue(stLookup(node->Node->Data, "data_directory"), DATA_T_STRING, POD(&ptr), 0) != 0)
	    ptr = obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->SubPtr-1)+1;
	strtcpy(node->DataPath, ptr, sizeof(node->DataPath));
	if (stGetAttrValue(stLookup(node->Node->Data, "edits_file"), DATA_T_STRING, POD(&ptr), 0) != 0)
	    ptr = "";
	strtcpy(node->EditsPath, ptr, sizeof(node->EditsPath));
	if (stGetAttrValue(stLookup(node->Node->Data, "description"), DATA_T_STRING, POD(&ptr), 0) != 0)
	    ptr = "";
	strtcpy(node->Description, ptr, sizeof(node->Description));

	/** Init the hashtable and xarray for the tables in this node **/
	xhInit(&(node->TablesByName), 255, 0);
	xaInit(&(node->TablesList), 127);
	xhInit(&(node->FormatsByName), 255, 0);
	xaInit(&(node->FormatsList), 127);

	/** Add the node to the table **/
	if (is_new_node)
	    xhAdd(&FP_INF.DBNodes, node->Path, (void*)node);

    return node;
    }


/*** fp_internal_MappedCopy() - copy data from a FP record 
 *** into the target area, and null-terminate it.
 ***/
int
fp_internal_MappedCopy(char* target, int target_size, pFpColInf column, char* row_data)
    {
    int i;

	/** enough room? **/
	if (target_size < column->Length + 1) return -1;

	/** copy **/
	memcpy(target, row_data+column->RecordOffset, column->Length);
	target[column->Length] = '\0';

    return 0;
    }


/*** fp_internal_ParseColumn() - parses one column from the FP record
 *** into the Centrallix data type.
 ***/
int
fp_internal_ParseColumn(pFpColInf column, pObjData pod, char* data, char* row_data)
    {
    char ibuf[32];
    char dtbuf[32];
    unsigned long long v;
    int i,f;

	switch(column->Type)
	    {
	    case DATA_T_INTEGER:
		if (fp_internal_MappedCopy(ibuf, sizeof(ibuf), column, row_data) < 0) return -1;
		pod->Integer = strtoi(ibuf, NULL, 10);
		break
	    case DATA_T_STRING:
		pod->String = data;
		if (fp_internal_MappedCopy(data, column->Length+1, column, row_data) < 0) return -1;
		break;
	    case DATA_T_DATETIME:
		pod->DateTime = (pDateTime)data;
		if (fp_internal_MappedCopy(dtbuf, sizeof(dtbuf), column, row_data) < 0) return -1;
		if (objDataToDateTime(DATA_T_STRING, dtbuf, pod->DateTime, NULL) < 0) return -1;
		break;
	    case DATA_T_DOUBLE:
		if (fp_internal_MappedCopy(ibuf, sizeof(ibuf), column, row_data) < 0) return -1;
		pod->Double = strtol(ibuf, NULL, 10);
		if (column->DecimalOffset) pod->Double /= pow(10, column->DecimalOffset);
		break;
	    case DATA_T_MONEY:
		if (fp_internal_MappedCopy(ibuf, sizeof(ibuf), column, row_data) < 0) return -1;
		v = strtoll(ibuf, NULL, 10);
		f = 1;
		for(i=0;i<column->DecimalOffset;i++) f *= 10;
		pod->Money = (pMoneyType)data;
		pod->Money->Value = (v/f) * 10000;
		//pod->Money->WholePart = v/f;
		v = (v/f)*f;
		if (column->DecimalOffset <= 4)
		    for(i=column->DecimalOffset;i<4;i++) v *= 10;
		else
		    for(i=4;i<column->DecimalOffset;i++) v /= 10;
		pod->Money->Value += v;
		//pod->Money->FractionPart = v;
		break;
	    default:
		mssError(1, "FP", "Bark!  Unhandled data type for column '%s'", column->Name);
		return -1;
	    }

    return 0;
    }


/*** fp_internal_ParseColN() - parse one column
 ***/
int
fp_internal_ParseColN(pFpData inf, int col_num)
    {
    pFpColInf column;
    pObjData pod;
    unsigned char* data;
    pFpTableInf tdata = inf->TData;
    int rval;

	column = tdata->Columns[col_num];
	pod = (pObjData)(inf->ParsedData + column->ParsedOffset);
	data = (inf->ParsedData + column->ParsedOffset + sizeof(ObjData));
	rval = fp_internal_ParseColumn(column, pod, (char*)data, (char*)inf->RowData);
	if (rval == 0) inf->ColFlags[col_num] |= FP_DATA_COL_F_PARSED;

    return rval;
    }


/*** fp_internal_TestParseColN() - parse one column unless already done.
 ***/
int
fp_internal_TestParseColN(pFpData inf, int col_num)
    {
    int rval;

	if (inf->ColFlags[col_num] & FP_DATA_COL_F_PARSED) return 0;
	rval = fp_internal_ParseColN(inf, col_num);
	if (rval == 0) inf->ColFlags[col_num] |= FP_DATA_COL_F_PARSED;

    return rval;
    }


/*** fp_internal_ParseRow() - parse the FP record into the Centrallix
 *** data types.
 ***/
int
fp_internal_ParseRow(pFpData inf)
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
	    if (fp_internal_TestParseColN(inf, i) < 0)
		return -1;
	    }*/

    return 0;
    }


/*** fp_internal_ReadRow() - read a single row of data from the data
 *** file at a given offset.  If the data file is not open, open it
 *** before reading and close it when we're done.
 ***/
int
fp_internal_ReadRow(pFpData inf, unsigned int offset)
    {
    pObject dataobj;
    int rval;

	/** Revalidate? **/
	if ((rval = fp_internal_Revalidate(inf)) < 0)
	    return -1;
	if (rval == 1)
	    {
	    /** modified, forget row **/
	    if (inf->RowData) inf->Offset = 0;
	    }

	/** Already there? **/
	if (inf->RowData && inf->Offset == offset) return 0;

	/** Gain access to underlying files **/
	dataobj = fp_internal_GetOpenFile(inf, inf->TData->KeyPath, FP_KEY_KEYFILE);
	if (!dataobj) return -1;

	/** Read the record in from the given offset **/
	if (!inf->RowData)
	    {
	    inf->RowData = nmSysMalloc(inf->TData->PhysLen);
	    if (!inf->RowData)
		{
		return -1;
		}
	    }
	rval = objRead(dataobj, (char*)&(inf->RowHeader), sizeof(FpRecData), offset, OBJ_U_SEEK | OBJ_U_PACKET);
	if (rval != sizeof(FpRecData))
	    return -1;
	rval = objRead(dataobj, (char*)inf->RowData, inf->TData->PhysLen, offset + sizeof(FpRecData), OBJ_U_SEEK | OBJ_U_PACKET);
	if (rval != inf->TData->PhysLen)
	    return -1;

	/** Parse the thing. **/
	if (fp_internal_ParseRow(inf) < 0) return -1;

	inf->Offset = offset;

    return 0;
    }


/*** fp_internal_RowToKey() - take a FpData structure, and return the 
 *** primary key, in string form, for the corresponding row.
 ***/
int
fp_internal_RowToKey(pFpData inf, char* keyname, int maxlen)
    {

	/** No declared primary key?  Use row id **/
	if (inf->TData->nPriKeyCols == 0)
	    {
	    snprintf(keyname, maxlen, "%d", inf->RowID);
	    }
	else
	    {
	    }

    return 0;
    }


/*** fp_internal_KeyToRow() - given an FpData structure and the primary
 *** key (string form) of the row, lookup and load the row.
 ***/
int
fp_internal_KeyToRow(pFpData inf, char* rowkey)
    {

	/** No declared primary key?  go with row id **/
	if (inf->TData->nPriKeyCols == 0)
	    {
	    inf->RowID = strtoi(rowkey, NULL, 10);
	    if (inf->RowID <= 0)
		{
		mssError(1,"FP","Row id '%d' in table '%s' is invalid",
			inf->RowID,
			inf->TData->Name);
		return -1;
		}
	    if (fp_internal_ReadRow(inf, sizeof(FpKeyHdr) + inf->TData->PhysLen + (inf->RowID-1) * (inf->TData->PhysLen + sizeof(FpRecData))) < 0)
		{
		return -1;
		}
	    if (inf->RowHeader.Any.RecordStatus != FP_RECORD_S_VALID)
		{
		mssError(1,"FP","Row id '%d' in table '%s' is deleted",
			inf->RowID,
			inf->TData->Name);
		return -1;
		}
	    }
	else
	    {
	    }

    return 0;
    }


/*** fpOpen - open a table, row, or column.
 ***/
void*
fpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pFpData inf;

	/** Allocate the structure **/
	inf = (pFpData)nmMalloc(sizeof(FpData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(FpData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine type and set pointers. **/
	if (fp_internal_DetermineType(obj,inf) < 0)
	    {
	    fp_internal_FreeInf(inf);
	    return NULL;
	    }

	/** Open the node object **/
	inf->Node = fp_internal_OpenNode(inf,systype);
	if (!inf->Node)
	    {
	    if (inf->OpenFiles) fp_internal_CloseFiles(inf->OpenFiles);
	    fp_internal_FreeInf(inf);
	    return NULL;
	    }

	/** Verify the table, if a table mentioned **/
	if (inf->TablePtr)
	    {
	    if (strpbrk(inf->TablePtr," \t\r\n"))
		{
		mssError(1,"FP","Requested table '%s' is invalid", inf->TablePtr);
		fp_internal_FreeInf(inf);
		return NULL;
		}
	    inf->TData = fp_internal_GetTData(inf);
	    if (!inf->TData)
		{
		mssError(1,"FP","Requested table '%s' could not be accessed", inf->TablePtr);
		fp_internal_FreeInf(inf);
		return NULL;
		}
	    }

	/** Lookup the row, if one was mentioned **/
	if (inf->Type == FP_T_ROW)
	    {
	    if (fp_internal_KeyToRow(inf, inf->RowColPtr) < 0)
		{
		fp_internal_FreeInf(inf);
		return NULL;
		}
	    }
	
    return (void*)inf;
    }

#if 0
/*** fp_internal_InsertRow - inserts a new row into the database, looking
 *** throught the OXT structures for the column values.  For text/image columns,
 *** it automatically inserts "" for an unspecified column value when the column
 *** does not allow nulls.
 ***/
int
fp_internal_InsertRow(pFpData inf, pObjTrxTree oxt)
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
	    if (inf->TData->ColFlags[j] & FP_CF_PRIKEY)
	        {
		/** Determine position,length within prikey-coded name **/
		kptr = inf->RowColPtr;
		for(i=0;i<inf->TData->ColKeys[j] && kptr != (char*)1;i++) kptr = strchr(kptr,'|')+1;
		if (kptr == (char*)1)
		    {
		    mssError(1,"FP","Not enough components in concat primary key (name)");
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
                    /*if (((pFpData)(attr_oxt->LLParam))->Type == FP_T_ATTR)*/
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


int
fp_internal_FreeInf(pFpData inf)
    {

	/** Free the info structure **/
	if (inf->OpenFiles) fp_internal_CloseFiles(inf->OpenFiles);
	obj_internal_FreePathStruct(&inf->Pathname);
	if (inf->RowData) nmSysFree(inf->RowData);
	if (inf->ParsedData) nmSysFree(inf->ParsedData);
	nmFree(inf,sizeof(FpData));

    return 0;
    }


/*** fpClose - close an open file or directory.
 ***/
int
fpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pFpData inf = FP(inf_v);

    	/** Was this a create? **/
	if ((*oxt) && (*oxt)->OpType == OXT_OP_CREATE && (*oxt)->Status != OXT_S_COMPLETE)
	    {
	    switch (inf->Type)
	        {
		case FP_T_TABLE:
		    /** We'll get to this a little later **/
		    break;

		case FP_T_ROW:
		    /** Complete the oxt. **/
		    (*oxt)->Status = OXT_S_COMPLETE;

		    break;

		case FP_T_COLUMN:
		    /** We wait until table is done for this. **/
		    break;
		}
	    }
	
	fp_internal_FreeInf(inf);

    return 0;
    }


/*** fpCreate - create a new object without actually opening that 
 *** object.
 ***/
int
fpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* objd;

    	/** Open and close the object **/
	obj->Mode |= O_CREAT;
	objd = fpOpen(obj,mask,systype,usrtype,oxt);
	if (!objd) return -1;

    return fpClose(objd,oxt);
    }


/*** fpDelete - delete an existing object.
 ***/
int
fpDelete(pObject obj, pObjTrxTree* oxt)
    {
    pFpData inf;

	/** Allocate the structure **/
	inf = (pFpData)nmMalloc(sizeof(FpData));
	if (!inf) return -1;
	memset(inf,0,sizeof(FpData));
	inf->Obj = obj;

	/** Determine type and set pointers. **/
	fp_internal_DetermineType(obj,inf);

	/** If a row, proceed else fail the delete. **/
	if (inf->Type != FP_T_ROW)
	    {
	    fp_internal_FreeInf(inf);
	    puts("Unimplemented delete operation in FP.");
	    mssError(1,"FP","Unimplemented delete operation in FP");
	    return -1;
	    }

	/** Access the DB node. **/

	/** Free the structure **/
	fp_internal_FreeInf(inf);

    return 0;
    }


/*** fpRead - read from the object's content.  Unsupported on these files.
 ***/
int
fpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pFpData inf = FP(inf_v);*/

    return -1;
    }


/*** fpWrite - write to an object's content.  Unsupported for these types of things.
 ***/
int
fpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pFpData inf = FP(inf_v);*/

    return -1;
    }


/*** fpOpenQuery - open a directory query.  We basically reformat the where clause
 *** and issue a query to the DB.
 ***/
void*
fpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pFpData inf = FP(inf_v);
    pFpQuery qy;

	/** Allocate the query structure **/
	qy = (pFpQuery)nmMalloc(sizeof(FpQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(FpQuery));
	qy->ObjInf = inf;
	qy->RowCnt = -1;
	qy->LLRowCnt = -1;
	qy->ObjSession = NULL;

	/** State that we won't do full query unless we get to FP_T_ROWSOBJ below **/
	query->Flags &= ~OBJ_QY_F_FULLQUERY;
	query->Flags &= ~OBJ_QY_F_FULLSORT;

	/** Build the query SQL based on object type. **/
	switch(inf->Type)
	    {
	    case FP_T_DATABASE:
	        /** Select the list of tables from the DB. **/
		qy->LLObj = objOpen(inf->Obj->Session, inf->Node->DataPath, O_RDONLY, 0600, "system/directory");
		if (!qy->LLObj) 
		    {
		    nmFree(qy,sizeof(FpQuery));
		    return NULL;
		    }
		objUnmanageObject(inf->Obj->Session, qy->LLObj);
		qy->LLQuery = objOpenQuery(qy->LLObj, NULL, ":name", NULL, NULL);
		if (!qy->LLQuery)
		    {
		    objClose(qy->LLObj);
		    nmFree(qy,sizeof(FpQuery));
		    return NULL;
		    }
		objUnmanageQuery(inf->Obj->Session, qy->LLQuery);
		qy->RowCnt = 0;
		qy->LLRowCnt = 0;
		qy->LLQuery2 = NULL;
		break;

	    case FP_T_TABLE:
	        /** No SQL needed -- always returns just 'columns' and 'rows' **/
		qy->RowCnt = 0;
	        break;

	    case FP_T_COLSOBJ:
	        /** Get a columns list. **/
		qy->TableInf = qy->ObjInf->TData;
		qy->RowCnt = 0;
		break;

	    case FP_T_ROWSOBJ:
	        /** Query the rows within a table -- iteration here. **/
		qy->LLRowCnt = 0;
		qy->RowCnt = 0;
		break;

	    case FP_T_COLUMN:
	    case FP_T_ROW:
	        /** These don't support queries for sub-objects. **/
	        nmFree(qy,sizeof(FpQuery));
		qy = NULL;
		break;
	    }

    return (void*)qy;
    }


/*** fpQueryFetch - get the next directory entry as an open object.
 ***/
void*
fpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pFpQuery qy = ((pFpQuery)(qy_v));
    pFpData inf = NULL;
    char filename[120];
    char* ptr;
    char* ptr2;
    int new_type;
    int i,cnt;
    pFpTableInf tdata = qy->ObjInf->TData;
    int restype;
    pObject ll_obj3;
    pFpColInf prikey;
    char* endptr = NULL;

	if (qy->RowCnt < 0) return NULL;

	qy->RowCnt++;

    	/** Get the next name based on the query type. **/
	while(!inf)
	    {
	    qy->LLRowCnt++;
	    /** Allocate the structure **/
	    inf = (pFpData)nmMalloc(sizeof(FpData));
	    if (!inf) return NULL;
	    memset(inf,0,sizeof(FpData));
	    inf->TData = tdata;
	    inf->Obj = obj;

	    if (endptr)
		{
		*endptr = '\0';
		obj->Pathname->nElements--;
		endptr = NULL;
		}

	    switch(qy->ObjInf->Type)
		{
		case FP_T_DATABASE:
		    while(1)
			{
			/** more qualified key files from the same table? **/
			if (qy->LLQuery2)
			    {
			    ll_obj3 = objQueryFetch(qy->LLQuery2, O_RDONLY);
			    if (ll_obj3)
				{
				objGetAttrValue(qy->LLObj2, "name", DATA_T_STRING, POD(&ptr));
				objGetAttrValue(ll_obj3, "name", DATA_T_STRING, POD(&ptr2));
				if (!strncmp(ptr2, "key", 3) && strlen(ptr2) - 3 < sizeof(tdata->Qualifier))
				    {
				    /** Got one **/
				    if (ptr2[3])
					snprintf(filename, sizeof(filename), "%s.%s", ptr, ptr2+3);
				    else
					strtcpy(filename, ptr, sizeof(filename));
				    objClose(ll_obj3);
				    break;
				    }

				/** Not valid, ignore this one and try for another **/
				objClose(ll_obj3);
				continue;
				}
			    else
				{
				objQueryClose(qy->LLQuery2);
				qy->LLQuery2 = NULL;
				}
			    }

			/** skip over any .fp files **/
			if (qy->LLObj2)
			    objClose(qy->LLObj2);
			qy->LLObj2 = objQueryFetch(qy->LLQuery, O_RDONLY);
			if (!qy->LLObj2)
			    {
			    fp_internal_FreeInf(inf);
			    return NULL;
			    }
			objUnmanageObject(inf->Obj->Session, qy->LLObj2);
			objGetAttrValue(qy->LLObj2, "name", DATA_T_STRING, POD(&ptr));
			strtcpy(filename, ptr, sizeof(filename));
			i = strlen(filename);
			if (i <= 3 || strncmp(filename+i-3, ".fp", 3))
			    {
			    qy->LLQuery2 = objOpenQuery(qy->LLObj2, "substring(:name,1,3) == 'key'", ":name", NULL, NULL);
			    if (qy->LLQuery2)
				objUnmanageQuery(inf->Obj->Session, qy->LLQuery2);
			    }
			}
		    break;

		case FP_T_TABLE:
		    /** Filename is either "rows" or "columns" **/
		    if (qy->RowCnt == 1) 
			{
			strcpy(filename,"columns");
			new_type = FP_T_COLSOBJ;
			}
		    else if (qy->RowCnt == 2) 
			{
			strcpy(filename,"rows");
			new_type = FP_T_ROWSOBJ;
			}
		    else 
			{
			fp_internal_FreeInf(inf);
			/*mssError(1,"FP","Table object has only two subobjects: 'rows' and 'columns'");*/
			return NULL;
			}
		    break;

		case FP_T_ROWSOBJ:
		    /** Get the filename from the primary key of the row. **/
		    new_type = FP_T_ROW;
		    inf->RowID = qy->LLRowCnt;
		    if (fp_internal_ReadRow(inf, sizeof(FpKeyHdr) + tdata->PhysLen + (qy->LLRowCnt-1) * (tdata->PhysLen + sizeof(FpRecData))) < 0)
			{
			fp_internal_FreeInf(inf);
			return NULL;
			}
		    if (inf->RowHeader.Any.RecordStatus != FP_RECORD_S_VALID)
			{
			fp_internal_FreeInf(inf);
			inf = NULL;
			}
		    else
			{
			if (fp_internal_RowToKey(inf, filename, sizeof(filename)) < 0)
			    {
			    mssError(1, "FP", "Could not build filename for primary key on table %s, row #%d",
				    tdata->Name, qy->LLRowCnt);
			    fp_internal_FreeInf(inf);
			    return NULL;
			    }
			}
		    /*prikey = tdata->Columns[tdata->PriKey];
		    prikey = tdata->Columns[0];
		    if (fp_internal_TestParseColN(inf, 0) < 0)
			{
			nmFree(inf,sizeof(FpData));
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
			mssError(1, "FP", "Could not build filename for primary key on table %s, row #%d",
				tdata->Name, qy->RowCnt);
			nmFree(inf,sizeof(FpData));
			return NULL;
			}
		    strcpy(filename, ptr);*/
		    break;

		case FP_T_COLSOBJ:
		    /** Loop through the columns in the TableInf structure. **/
		    new_type = FP_T_COLUMN;
		    if (qy->RowCnt <= qy->TableInf->nColumns)
			{
			memccpy(filename,qy->TableInf->Columns[qy->RowCnt-1]->Name, 0, sizeof(filename)-1);
			filename[sizeof(filename)-1] = '\0';
			}
		    else
			{
			fp_internal_FreeInf(inf);
			return NULL;
			}
		    break;
		}
	    if (!inf) continue;

	    /** Build the filename. **/
	    endptr = ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	    if ((ptr - obj->Pathname->Pathbuf) + 1 + strlen(filename) >= 255)
		{
		mssError(1,"FP","Pathname too long for internal representation");
		fp_internal_FreeInf(inf);
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
	    fp_internal_DetermineType(obj,inf);

	    if (!inf->TData)
		{
		inf->TData = fp_internal_GetTData(inf);
		if (!inf->TData)
		    {
		    //mssError(1,"FP","Requested table '%s' could not be accessed", inf->TablePtr);
		    fp_internal_FreeInf(inf);
		    inf = NULL;
		    }
		}
	    }

    return (void*)inf;
    }


/*** fpQueryDelete - delete the contents of a query result set.  This is
 *** not yet supported.
 ***/
int
fpQueryDelete(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** fpQueryClose - close the query.
 ***/
int
fpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pFpQuery qy = ((pFpQuery)(qy_v));

	/** Free the structure **/
	//if (qy->Files) fp_internal_CloseFiles(qy->Files);
	if (qy->LLObj)
	    objClose(qy->LLObj);
	if (qy->LLObj2)
	    objClose(qy->LLObj2);
	if (qy->LLQuery)
	    objQueryClose(qy->LLQuery);
	if (qy->LLQuery2)
	    objQueryClose(qy->LLQuery2);
	nmFree(qy,sizeof(FpQuery));

    return 0;
    }


/*** fpGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
fpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pFpData inf = FP(inf_v);
    int i;
    pFpTableInf tdata;

    	/** Attr type depends on object type. **/
	if (inf->Type == FP_T_ROW)
	    {
	    tdata = inf->TData;
	    for(i=0;i<tdata->nColumns;i++)
	        {
		if (!strcmp(attrname,tdata->Columns[i]->Name))
		    {
		    return tdata->Columns[i]->Type;
		    }
		}
	    for(i=0;i<sizeof(fp_special_attr)/sizeof(char*);i++) if (!strcmp(fp_special_attr[i], attrname))
		{
		return fp_special_type[i];
		}
	    }
	else if (inf->Type == FP_T_COLUMN)
	    {
	    if (!strcmp(attrname,"datatype")) return DATA_T_STRING;
	    if (!strcmp(attrname,"format")) return DATA_T_STRING;
	    }

	mssError(1,"FP","Invalid column for GetAttrType");

    return -1;
    }


/*** fpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
fpGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pFpData inf = FP(inf_v);
    int i,t,minus,n;
    unsigned int msl,lsl,divtmp;
    pFpTableInf tdata;
    char* ptr;
    int days,fsec;
    float f;
    pObjData srcpod;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"FP","Type mismatch accessing attribute '%s' [should be string]", attrname);
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
		mssError(1,"FP","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    /** Different for various objects. **/
	    switch(inf->Type)
	        {
		case FP_T_DATABASE:
		    val->String = inf->Node->Description;
		    break;
		case FP_T_TABLE:
		    val->String = inf->TData->Annotation;
		    break;
		case FP_T_ROWSOBJ:
		    val->String = "Contains rows for this table";
		    break;
		case FP_T_COLSOBJ:
		    val->String = "Contains columns for this table";
		    break;
		case FP_T_COLUMN:
		    val->String = "Column within this table";
		    break;
		case FP_T_ROW:
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
		mssError(1,"FP","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    switch(inf->Type)
	        {
		case FP_T_DATABASE: *((char**)val) = "system/void"; break;
		case FP_T_TABLE: *((char**)val) = "system/void"; break;
		case FP_T_ROWSOBJ: *((char**)val) = "system/void"; break;
		case FP_T_COLSOBJ: *((char**)val) = "system/void"; break;
		case FP_T_ROW: 
		    {
		    /*if (inf->TData->HasContent)
		        val->String = "application/octet-stream";
		    else*/
		        val->String = "system/void";
		    break;
		    }
		case FP_T_COLUMN: val->String = "system/void"; break;
		}
	    return 0;
	    }

	/** Outer type... **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"FP","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    switch(inf->Type)
	        {
		case FP_T_DATABASE: val->String = "application/filepro"; break;
		case FP_T_TABLE: val->String = "system/table"; break;
		case FP_T_ROWSOBJ: val->String = "system/table-rows"; break;
		case FP_T_COLSOBJ: val->String = "system/table-columns"; break;
		case FP_T_ROW: val->String = "system/row"; break;
		case FP_T_COLUMN: val->String = "system/column"; break;
		}
	    return 0;
	    }

	/** Column object?  Type is the only one. **/
	if (inf->Type == FP_T_COLUMN)
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"FP","Type mismatch accessing attribute '%s' [should be string]", attrname);
		return -1;
		}
	    /** Get the table info. **/
	    tdata=inf->TData;

	    if (!strcmp(attrname,"datatype"))
		{
		/** Search table info for this column. **/
		for(i=0;i<tdata->nColumns;i++) if (!strcmp(tdata->Columns[i]->Name,inf->RowColPtr))
		    {
		    val->String = obj_type_names[tdata->Columns[i]->Type];
		    return 0;
		    }
		}
	    else if (!strcmp(attrname,"format"))
		{
		/** Search table info for this column. **/
		for(i=0;i<tdata->nColumns;i++) if (!strcmp(tdata->Columns[i]->Name,inf->RowColPtr))
		    {
		    val->String = tdata->Columns[i]->FormatText;
		    return 0;
		    }
		}
	    else
		{
		return -1;
		}
	    }
	else if (inf->Type == FP_T_ROW)
	    {
	    /** Get the table info. **/
	    tdata = inf->TData;

	    /** Search through the columns. **/
	    for(i=0;i<tdata->nColumns;i++) if (!strcmp(tdata->Columns[i]->Name,attrname))
	        {
		t = tdata->Columns[i]->Type;
		if (datatype != t)
		    {
		    mssError(1,"FP","Type mismatch accessing attribute '%s' [requested=%s, actual=%s]", 
			    attrname, obj_type_names[datatype], obj_type_names[t]);
		    return -1;
		    }
		srcpod = (pObjData)(inf->ParsedData + tdata->Columns[i]->ParsedOffset);
		if (fp_internal_TestParseColN(inf, i) < 0) return -1;
		objCopyData(srcpod, val, t);
		return 0;
		}

	    /** And standard attrs on every fp table row **/
	    for(i=0;i<sizeof(fp_special_attr)/sizeof(char*);i++) if (!strcmp(fp_special_attr[i], attrname))
		{
		if (datatype != fp_special_type[i])
		    {
		    mssError(1,"FP","Type mismatch accessing attribute '%s' [requested=%s, actual=%s]", 
			    attrname, obj_type_names[datatype], obj_type_names[fp_special_type[i]]);
		    return -1;
		    }
		if (!strcmp(attrname, "__fp_create_date"))
		    {
		    if (fp_internal_StampToDate(inf->RowHeader.Valid.CreateDate, &inf->CreateDate) < 0) return -1;
		    val->DateTime = &inf->CreateDate;
		    return 0;
		    }
		else if (!strcmp(attrname, "__fp_create_uid"))
		    {
		    val->Integer = inf->RowHeader.Valid.CreateUID;
		    return 0;
		    }
		else if (!strcmp(attrname, "__fp_modify_date"))
		    {
		    if (fp_internal_StampToDate(inf->RowHeader.Valid.UpdateDate, &inf->ModifyDate) < 0) return -1;
		    val->DateTime = &inf->ModifyDate;
		    return 0;
		    }
		else if (!strcmp(attrname, "__fp_modify_uid"))
		    {
		    val->Integer = inf->RowHeader.Valid.UpdateProcUID;
		    return 0;
		    }
		else if (!strcmp(attrname, "__fp_process_date"))
		    {
		    if (fp_internal_StampToDate(inf->RowHeader.Valid.ProcDate, &inf->ProcessDate) < 0) return -1;
		    val->DateTime = &inf->ProcessDate;
		    return 0;
		    }
		else if (!strcmp(attrname, "last_modification"))
		    {
		    if (inf->RowHeader.Valid.ProcDate > inf->RowHeader.Valid.UpdateDate)
			{
			if (fp_internal_StampToDate(inf->RowHeader.Valid.ProcDate, &inf->ProcessDate) < 0) return -1;
			val->DateTime = &inf->ProcessDate;
			}
		    else
			{
			if (fp_internal_StampToDate(inf->RowHeader.Valid.UpdateDate, &inf->ModifyDate) < 0) return -1;
			val->DateTime = &inf->ModifyDate;
			}
		    return 0;
		    }
		}
	    }

	mssError(1,"FP","Invalid column for GetAttrValue");

    return -1;
    }


/*** fpGetNextAttr - get the next attribute name for this object.
 ***/
char*
fpGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pFpData inf = FP(inf_v);
    pFpTableInf tdata;

	/** Attribute listings depend on object type. **/
	switch(inf->Type)
	    {
	    case FP_T_DATABASE:
	        return NULL;
	
	    case FP_T_TABLE:
	        return NULL;

	    case FP_T_ROWSOBJ:
	        return NULL;

	    case FP_T_COLSOBJ:
	        return NULL;

	    case FP_T_COLUMN:
		if (inf->CurAttr++ == 0) return "datatype";
		if (inf->CurAttr++ == 1) return "format";
	        break;

	    case FP_T_ROW:
	        /** Get the table info. **/
		tdata = inf->TData;

	        /** Return attr in table inf **/
		if (inf->CurAttr < tdata->nColumns)
		    return tdata->Columns[inf->CurAttr++]->Name;
		else if (inf->CurAttr < tdata->nColumns + (sizeof(fp_special_attr)/sizeof(char*)))
		    return fp_special_attr[(inf->CurAttr++) - tdata->nColumns];
	        break;
	    }

    return NULL;
    }


/*** fpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
fpGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pFpData inf = FP(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = fpGetNextAttr(inf_v,oxt);

    return ptr;
    }


/*** fpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
fpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pFpData inf = FP(inf_v);
    int type,rval;
    char sbuf[160];
    char* ptr;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"FP","Type mismatch setting attribute '%s' [should be string]", attrname);
		return -1;
		}
	    if (inf->Type == FP_T_DATABASE) return -1;
	    }

	/** Changing the 'annotation'? **/
	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"FP","Type mismatch setting attribute '%s' [should be string]", attrname);
		return -1;
		}
	    /** Choose the appropriate action based on object type **/
	    switch(inf->Type)
	        {
		case FP_T_DATABASE:
		    strtcpy(inf->Node->Description, val?(val->String):"", 256);
		    break;
		    
		case FP_T_TABLE:
		    strtcpy(inf->TData->Annotation, val?(val->String):"", sizeof(inf->TData->Annotation));
		    while(strchr(inf->TData->Annotation,'"')) *(strchr(inf->TData->Annotation,'"')) = '\'';
		    break;

		case FP_T_ROWSOBJ:
		case FP_T_COLSOBJ:
		case FP_T_COLUMN:
		    /** Can't change any of these (yet) **/
		    return -1;

		case FP_T_ROW:
		    /** Not yet implemented :) **/
		    return -1;
		}
	    return 0;
	    }

	/** If this is a row, check the OXT. **/
	if (inf->Type == FP_T_ROW)
	    {
	    /** check Oxt. **/
	    if (*oxt)
		{
		/** We're within a transaction.  Fill in the oxt. **/
		type = fpGetAttrType(inf_v, attrname, oxt);
		if (type < 0) return -1;
		if (datatype != type)
		    {
		    mssError(1,"FP","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
			    attrname, obj_type_names[datatype], obj_type_names[type]);
		    return -1;
		    }
		(*oxt)->AllocObj = 0;
		(*oxt)->Object = NULL;
		(*oxt)->Status = OXT_S_VISITED;
		if (strlen(attrname) >= 64)
		    {
		    mssError(1,"FP","Attribute name '%s' too long",attrname);
		    return -1;
		    }
		strcpy((*oxt)->AttrName, attrname);
		obj_internal_SetTreeAttr(*oxt, type, val);
		}
	    else
		{
		/** No transaction.  Simply do an update. **/
		type = fpGetAttrType(inf_v, attrname, oxt);
		if (type < 0) return -1;
		if (datatype != type)
		    {
		    mssError(1,"FP","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
			    attrname, obj_type_names[datatype], obj_type_names[type]);
		    return -1;
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


/*** fpAddAttr - add an attribute to an object.  This doesn't work for
 *** unix filesystem objects, so we just deny the request.
 ***/
int
fpAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** fpOpenAttr - open an attribute as if it were an object with 
 *** content.  The Sybase database objects don't yet have attributes that are
 *** suitable for this.
 ***/
void*
fpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** fpGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
fpGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** fpGetNextMethod -- same as above.  Always fails. 
 ***/
char*
fpGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** fpExecuteMethod - No methods to execute, so this fails.
 ***/
int
fpExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree* oxt)
    {
    return -1;
    }



/*** fpInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
fpInitialize()
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
	memset(&FP_INF,0,sizeof(FP_INF));
	xhInit(&FP_INF.DBNodes,255,0);
	xaInit(&FP_INF.DBNodeList,127);
	FP_INF.FpConfig = stLookup(CxGlobals.ParsedConfig,"filepro");

	/** Setup the structure **/
	strcpy(drv->Name,"FP - FilePro Driver");
	drv->Capabilities = OBJDRV_C_FULLQUERY | OBJDRV_C_TRANS;
	/*drv->Capabilities = OBJDRV_C_TRANS;*/
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/filepro");

	/** Setup the function references. **/
	drv->Open = fpOpen;
	drv->Close = fpClose;
	drv->Create = fpCreate;
	drv->Delete = fpDelete;
	drv->OpenQuery = fpOpenQuery;
	drv->QueryDelete = fpQueryDelete;
	drv->QueryFetch = fpQueryFetch;
	drv->QueryClose = fpQueryClose;
	drv->Read = fpRead;
	drv->Write = fpWrite;
	drv->GetAttrType = fpGetAttrType;
	drv->GetAttrValue = fpGetAttrValue;
	drv->GetFirstAttr = fpGetFirstAttr;
	drv->GetNextAttr = fpGetNextAttr;
	drv->SetAttrValue = fpSetAttrValue;
	drv->AddAttr = fpAddAttr;
	drv->OpenAttr = fpOpenAttr;
	drv->GetFirstMethod = fpGetFirstMethod;
	drv->GetNextMethod = fpGetNextMethod;
	drv->ExecuteMethod = fpExecuteMethod;

	nmRegister(sizeof(FpColInf),"FpColInf");
	nmRegister(sizeof(FpTableInf),"FpTableInf");
	nmRegister(sizeof(FpData),"FpData");
	nmRegister(sizeof(FpQuery),"FpQuery");
	nmRegister(sizeof(FpNode),"FpNode");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;
	FP_INF.ObjDriver = drv;

    return 0;
    }


MODULE_INIT(fpInitialize);
MODULE_PREFIX("fp");
MODULE_DESC("FilePro ObjectSystem Driver");
MODULE_VERSION(0,9,0);
MODULE_IFACE(CX_CURRENT_IFACE);
