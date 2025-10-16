
/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Core                                                      */
/*                                                                      */
/* Copyright (C) 1998-2012 LightSys Technology Services, Inc.           */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             */
/* 02111-1307  USA                                                      */
/*                                                                      */
/* A copy of the GNU General Public License has been included in this   */
/* distribution in the file "COPYING".                                  */
/*                                                                      */
/* Module:      objdrv_cluster.c                                        */
/* Author:      Israel Fuller                                           */
/* Creation:    September 17, 2025                                      */
/* Description: Cluster object driver.                                  */
/************************************************************************/

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cxlib/clusters.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "cxlib/util.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "expression.h"
#include "hints.h"
#include "obj.h"
#include "param.h"
#include "st_node.h"
#include "stparse.h"

/*** File notes:
 *** This file uses comment anchors, provided by the Comment Anchors VSCode
 *** extension from Starlane Studios. This allows developers with the extension
 *** to control click the "LINK <ID>" comments to navigate to the coresponding
 *** "ANCHOR[id=<ID>]" comment. (Note: Invalid or broken links will default to
 *** the first line of the file.)
 *** 
 *** For example, this link should take you to the function signatures:
 *** LINK #functions
 *** 
 *** Any developers without this extension can safely ignore these comments,
 *** although please try not to break them. :)
 *** 
 *** Comment Anchors VSCode Extension:
 *** https://marketplace.visualstudio.com/items?itemName=ExodiusStudios.comment-anchors
 ***/

/** Pure Laziness **/
#define ENABLE_TPRINTF

/** Debugging **/
#ifndef ENABLE_TPRINTF
void void_func() {}
#define tprintf void_func
#endif
#ifdef ENABLE_TPRINTF
#define tprintf printf
#endif

/** Defaults for unspecified optional attributes. **/
#define DEFAULT_MIN_IMPROVEMENT 0.0001
#define DEFAULT_MAX_ITERATIONS 64u

/** ================ Stuff That Should Be Somewhere Else ================ **/
/** ANCHOR[id=temp] **/

/** TODO: I think this should be moved to mtsession. **/
/*** I caused at least 10 bugs so far trying to pass format specifiers to
 *** mssError without realizing that it didn't support them. Eventually, I
 *** got fed up enough with the whole thing to write the following function.
 ***/
/*** Displays error text to the user. Does not print a stack trace. Does not
 *** exit the program, allowing for the calling function to fail, generating
 *** an error cascade which may be useful to the user since a stack trace is
 *** not readily available.
 *** 
 *** @todo I think this should be moved to somewhere else.
 *** 
 *** @param clr Whether to clear the current error stack. As a rule of thumb,
 *** 	if you are the first one to detec the error, clear the stack so that
 *** 	other unrelated messages are not shown. If you are detecting an error
 *** 	from another function that may also call an mssError() function, do
 *** 	not clear the stack.
 *** @param module The name or abbreviation of the module in which this 
 *** 	function is being called, to help developers narrow down the location
 *** 	of the error.
 *** @param format The format text for the error, which accepts any format
 *** 	specifier that would be accepted by printf().
 *** @param ... Variables matching format specifiers in the format.
 *** @returns Nothing, always succeeds.
 ***/
void mssErrorf(int clr, char* module, const char* format, ...)
    {
    /** Prevent interlacing with stdout flushing at a weird time. **/
    check(fflush(stdout));
    
    /** Insert convenient newline before error stack begins. **/
    if (clr == 1) fprintf(stderr, "\n");
    
    /** Process the format with all the same rules as printf(). **/
    char buf[BUFSIZ];
    va_list args;
    va_start(args, format);
    const int num_chars = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    /** Error check vsnprintf, just to be safe. **/
    if (num_chars < 0)
	{
	perror("vsnprintf() failed");
	fprintf(stderr, "FAIL: mssErrorf(%d, \"%s\", \"%s\", ...)\n", clr, module, format);
	return;
	}
    if (num_chars > BUFSIZ)
	fprintf(stderr, "WARNING: Error truncated (length %d > buffer size %d).\n", num_chars, BUFSIZ);
    
    /** Print the error. **/
    const int ret = mssError(clr, module, "%s", buf);

    /** Not sure why you have to error check the error function... **/
    if (ret != 0) fprintf(stderr, "FAIL %d: mssError(%d, \"%s\", \"%%s\", \"%s\")\n", ret, clr, module, buf);
    }


/** TODO: I think this should be moved to datatypes. **/
/** Should maybe replace current type parsing in the presentation hints. **/
/*** Parse the given string into a datatype. The case of the first character
 *** is ignored, but all other characters must be capitalized correctly.
 *** 
 *** @attention - This function is optimized to prevent performance hits
 *** 	situations where it may need to be called many thousands of times.
 *** 
 *** @param str The string to be parsed to a datatype.
 *** @returns The datatype.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
int ci_TypeFromStr(const char* str)
    {
    /** All valid types are non-null strings, at least 2 characters long. **/
    if (str == NULL || str[0] == '\0' || str[1] == '\0') return -1;
    
    /** Check type. **/
    switch (str[0])
	{
	case 'A': case 'a':
	    if (strcmp(str+1, "Array"+1) == 0) return DATA_T_ARRAY;
	    if (strcmp(str+1, "Any"+1) == 0) return DATA_T_ANY;
	    break;
	
	case 'B': case 'b':
	    if (strcmp(str+1, "Binary"+1) == 0) return DATA_T_BINARY;
	    break;
	
	case 'C': case 'c':
	    if (strcmp(str+1, "Code"+1) == 0) return DATA_T_CODE;
	    break;
	
	case 'D': case 'd':
	    if (strcmp(str+1, "Double"+1) == 0) return DATA_T_DOUBLE;
	    if (strcmp(str+1, "DateTime"+1) == 0) return DATA_T_DATETIME;
	    break;
	
	case 'I': case 'i':
	    if (strcmp(str+1, "Integer"+1) == 0) return DATA_T_INTEGER;
	    if (strcmp(str+1, "IntVecor"+1) == 0) return DATA_T_INTVEC;
	    break;
	
	case 'M': case 'm':
	    if (strcmp(str+1, "Money"+1) == 0) return DATA_T_MONEY;
	    break;
	
	case 'S': case 's':
	    if (strcmp(str+1, "String"+1) == 0) return DATA_T_STRING;
	    if (strcmp(str+1, "StringVector"+1) == 0) return DATA_T_STRINGVEC;
	    break;
	    
	case 'U': case 'u':
	    if (strcmp(str+1, "Unknown"+1) == 0) return DATA_T_UNAVAILABLE;
	    if (strcmp(str+1, "Unavailable"+1) == 0)  return DATA_T_UNAVAILABLE;
	    break;
	}
    
    /** Invalid type. **/
    return -1;
    }

/** TODO: I think this should be moved to datatypes. **/
/** Should maybe replace duplocate functionality elsewhere. **/
char* ci_TypeToStr(const int type)
    {
    switch (type)
	{
	case DATA_T_UNAVAILABLE: return "Unknown";
	case DATA_T_INTEGER:     return "Integer";
	case DATA_T_STRING:      return "String";
	case DATA_T_DOUBLE:      return "Double";
	case DATA_T_DATETIME:    return "DateTime";
	case DATA_T_INTVEC:      return "IntVecor";
	case DATA_T_STRINGVEC:   return "StringVector";
	case DATA_T_MONEY:       return "Money";
	case DATA_T_ARRAY:       return "Array";
	case DATA_T_CODE:        return "Code";
	case DATA_T_BINARY:      return "Binary";
	}
    
    /** Invalid type. **/
    mssErrorf(1, "Cluster", "Invalid type %d.\n", type);
    return "Invalid"; /* Shall not parse to a valid type in ci_TypeFromStr(). */
    }

/** TODO: I think this should be moved to xarray. **/
/** Contract: Return value is null iff pXArray has 0 items. **/
void** ci_xaToTrimmedArray(pXArray arr)
    {
    if (arr->nItems == 0) {
	mssErrorf(1, "Cluster", "Failed to trim XArray of length 0.");
	return NULL;
    }
    
    const size_t arr_size = arr->nItems * sizeof(void*);
    void** result = check_ptr(nmMalloc(arr_size));
    memcpy(result, arr->Items, arr_size);
    return result;
    }

/** I got tired of forgetting how to do these. **/
#define ci_file_name(obj) \
    ({ \
    __typeof__ (obj) _obj = (obj); \
    obj_internal_PathPart(_obj->Pathname, _obj->SubPtr - 1, 1); \
    })
#define ci_file_path(obj) \
    ({ \
    __typeof__ (obj) _obj = (obj); \
    obj_internal_PathPart(_obj->Pathname, 0, _obj->SubPtr); \
    })


/** ================ Enum Declairations ================ **/
/** ANCHOR[id=enums] **/

/** Enum representing a clustering algorithm. **/
typedef unsigned char ClusterAlgorithm;
#define ALGORITHM_NULL             (ClusterAlgorithm)0u
#define ALGORITHM_NONE             (ClusterAlgorithm)1u
#define ALGORITHM_SLIDING_WINDOW   (ClusterAlgorithm)2u
#define ALGORITHM_KMEANS           (ClusterAlgorithm)3u
#define ALGORITHM_KMEANS_PLUS_PLUS (ClusterAlgorithm)4u
#define ALGORITHM_KMEDOIDS         (ClusterAlgorithm)5u
#define ALGORITHM_DB_SCAN          (ClusterAlgorithm)6u

#define nClusteringAlgorithms 7u
ClusterAlgorithm ALL_CLUSTERING_ALGORITHMS[nClusteringAlgorithms] =
    {
    ALGORITHM_NULL,
    ALGORITHM_NONE,
    ALGORITHM_SLIDING_WINDOW,
    ALGORITHM_KMEANS,
    ALGORITHM_KMEANS_PLUS_PLUS,
    ALGORITHM_KMEDOIDS,
    ALGORITHM_DB_SCAN,
    };

/** Converts a clustering algorithm to its string name. **/
char* ci_ClusteringAlgorithmToString(ClusterAlgorithm clustering_algorithm)
    {
    switch (clustering_algorithm)
	{
	case ALGORITHM_NULL: return "NULL algorithm";
	case ALGORITHM_NONE: return "none";
	case ALGORITHM_SLIDING_WINDOW: return "sliding-window";
	case ALGORITHM_KMEANS: return "k-means";
	case ALGORITHM_KMEANS_PLUS_PLUS: return "k-means++";
	case ALGORITHM_KMEDOIDS: return "k-medoids";
	case ALGORITHM_DB_SCAN: return "db-scan";
	default: return "Unknown algorithm";
	}
    }

/** Enum representing a similarity measurement algorithm. **/
typedef unsigned char SimilarityMeasure;
#define SIMILARITY_NULL            (SimilarityMeasure)0u
#define SIMILARITY_COSINE          (SimilarityMeasure)1u
#define SIMILARITY_LEVENSHTEIN     (SimilarityMeasure)2u

#define nSimilarityMeasures 3u
SimilarityMeasure ALL_SIMILARITY_MEASURES[nSimilarityMeasures] =
    {
    SIMILARITY_NULL,
    SIMILARITY_COSINE,
    SIMILARITY_LEVENSHTEIN,
    };

/** Converts a similarity measure to its string name. **/
char* ci_SimilarityMeasureToString(SimilarityMeasure similarity_measure)
    {
    switch (similarity_measure)
	{
	case SIMILARITY_NULL: return "NULL similarity measure";
	case SIMILARITY_COSINE: return "cosine";
	case SIMILARITY_LEVENSHTEIN: return "levenshtein";
	default: return "Unknown similarity measure";
	}
    }

/*** Enum representing the type of data targetted by the driver,
 *** set based on the path given when the driver is used to open
 *** a cluster file.
 *** 
 *** `0u` is reserved for a possible `NULL` value in the future.
 *** However, there is currently no allowed `NULL` TargetType.
 ***/
typedef unsigned char TargetType;
#define TARGET_ROOT          (TargetType)1u
#define TARGET_CLUSTER       (TargetType)2u
#define TARGET_SEARCH        (TargetType)3u
#define TARGET_CLUSTER_ENTRY (TargetType)4u
#define TARGET_SEARCH_ENTRY  (TargetType)5u

/** Attribute name lists by TargetType. **/
#define nATTR_ROOT 2u
char* const ATTR_ROOT[nATTR_ROOT] = {
    "source",
    "attr_name",
};
#define nATTR_CLUSTER 7u
char* const ATTR_CLUSTER[nATTR_CLUSTER] =
    {
    "algorithm",
    "similarity_measure",
    "num_clusters",
    "min_improvement",
    "max_iterations",
    "date_created",
    "date_computed",
    };
#define nATTR_SEARCH 5u
char* const ATTR_SEARCH[nATTR_SEARCH] =
    {
    "source",
    "threshold",
    "similarity_measure",
    "date_created",
    "date_computed",
    };
#define nATTR_CLUSTER_ENTRY 2u
char* const ATTR_CLUSTER_ENTRY[nATTR_CLUSTER_ENTRY] =
    {
    "val",
    "sim",
    };
#define nATTR_SEARCH_ENTRY 3u
char* const ATTR_SEARCH_ENTRY[nATTR_SEARCH_ENTRY] =
    {
    "val1",
    "val2",
    "sim",
    };
#define END_OF_ATTRIBUTES NULL


/** Method name list. **/
#define nMETHOD_NAME 2u
char* const METHOD_NAME[nMETHOD_NAME] =
    {
    "cache",
    };
#define END_OF_METHODS END_OF_ATTRIBUTES


/** ================ Struct Declarations ================ **/
/** ANCHOR[id=structs] **/

/** Represents the data source which may have data already fetched. **/
typedef struct _SOURCE
    {
    /** Top level attributes (specified in the .cluster file). **/
    char*          Name;           /* The node name, specified in the .cluster file.
                                    * Warning: Some code makes the assumption that this
                                    * is the first field in the struct.
                                    */
    char*          Key;            /* The key associated with this object in the global SourceCache. */
    char*          SourcePath;     /* The path to the data source from which to retrieve data. */
    char*          AttrName;       /* The name of the attribute to get from the data source. */
    
    /** Computed data. **/
    char**         Data;           /* The data strings to be clustered and searched, or NULL if they
                                    * have not been fetched from the source.
                                    */
    pVector*       Vectors;        /* The cosine comparison vectors from the fetched data, or NULL if
                                    * they haven't been computed. Note that vectors are no longer
                                    * needed once all clusters and searches have been computed, so
                                    * they are automatically freed in that case to save memory.
                                    */
    unsigned int   nVectors;       /* The number of vectors and data strings. Note: This is not
                                    * set to 0 if the vector array is freed, this case should be
                                    * checked separately.
                                    */
    
    /** Time. **/
    DateTime       DateCreated;    /* The date and time that this object was created and initialized. */
    DateTime       DateComputed;   /* The date and time that the Data and Vectors fields were computed. */
    } SourceData, *pSourceData;

/** Data for each cluster. **/
typedef struct _CLUSTER
    {
    /** Attribute Data. **/
    char*             Name;              /* The cluster name, specified in the .cluster file.
                                          * Warning: Some code makes the assumption that this
                                          * is the first field in the struct.
                                          */
    char*             Key;               /* The key associated with this object in the global ClusterCache. */
    ClusterAlgorithm  ClusterAlgorithm;  /* The clustering algorithm to be used. */
    SimilarityMeasure SimilarityMeasure; /* The similarity measurse to be used when clustering. */
    unsigned int      NumClusters;       /* The number of clusters. 1 if algorithm = none. */
    double            MinImprovement;    /* The minimum amount of improvement that must be met each
                                          * clustering iteration. If there is less improvement, the
                                          * algorithm will stop. Specifying "max" in the .cluster
                                          * file should be represented by a value of -inf.
                                          */
    unsigned int      MaxIterations;     /* The maximum number of iterations to run clustering. */
    
    /** Other data (ignored by caching). **/
    unsigned int      nSubClusters;      /* The number of subclusters of this cluster. */
    struct _CLUSTER** SubClusters;       /* A pClusterData array, NULL if nSubClusters == 0. */
    struct _CLUSTER*  Parent;            /* This cluster's parent. NULL if it is not a subcluster. */
    pSourceData       SourceData;        /* Pointer to the source data that this cluster uses. */
    
    /** Computed data. **/
    unsigned int*     Labels;            /* An array with one element for each vector in the data
                                          * (aka. DriverData->nVectors). For vector i, Labels[i] is
                                          * the ID of the cluster to which that data is assigned.
                                          * NULL if the cluster has not been computed. */
    
    /** Time. **/
    DateTime       DateCreated;          /* The date and time that this object was created and initialized. */
    DateTime       DateComputed;         /* The date and time that the Labels field was computed. */
    }
    ClusterData, *pClusterData;
    
/** Data for each search. **/
typedef struct _SEARCH
    {
    char*             Name;              /* The search name, specified in the .cluster file.
                                          * Warning: Some code makes the assumption that this
                                          * is the first field in the struct.
                                          */
    char*             Key;               /* The key associated with this object in the global SearchCache. */
    pClusterData      Source;            /* The cluster from which this search is to be derived. */
    double            Threshold;         /* The minimum similarity threshold for elements to be
                                          * included in the results of the search.
                                          */
    SimilarityMeasure SimilarityMeasure; /* The similarity measure used to compare items. */
    
    /** Computed data. **/
    pDup*             Dups;              /* An array holding the dups found by the search, or NULL
                                          * if the search has not been computed.
                                          */
    unsigned int      nDups;             /* The number of dups found. */
    
    /** Time. **/
    DateTime       DateCreated;          /* The date and time that this object was created and initialized. */
    DateTime       DateComputed;             /* The date and time that the Dups field was computed. */
    }
    SearchData, *pSearchData;

/*** Node instance data.
 *** When a .cluster file is openned, there will be only one node for that
 *** file. However, in the course of the query, many driver instance structs
 *** may be created by functions like clusterQueryFetch(), and closed by the
 *** object system using clusterClose().
 ***/
typedef struct _NODE
    {
    /** Substructures. **/
    pSourceData    SourceData;  /* Data from the provided source. */
    pParam*        Params;      /* A pParam array storing the params in the .cluster file. */
    unsigned int   nParams;     /* The number of specified params. */
    pParamObjects  ParamList;   /* Functions as a "scope" for resolving values during parsing. */
    pClusterData*  Clusters;    /* A pCluster array storing the clusters in the .cluster file.
                                 * Will be NULL if nClusters = 0.
                                 */
    unsigned int   nClusters;   /* The number of specified clusters. */
    pSearchData*   Searches;    /* A SearchData array storing the searches in the .cluster file. */
    unsigned int   nSearches;   /* The number of specified searches. */
    
    /** Other stuff, idk why it's here. **/
    pSnNode        Node;
    pObject        Obj;
    }
    NodeData, *pNodeData;

/** Driver instance data. **/
/*** Similar to a pointer to specific, computed data in the pNodeData struct.
 *** If target type is the root, a cluster, or a search, no data is guarnteed
 *** to be computed yet. These three types can be returned from clusterOpen().
 *** To target a cluster entry or search entry, fetch a driver targetting a
 *** cluster or search (respectively). These target types ensure that the data
 *** has been computed, so the GetAttr functions do not need to ensure this.
 ***/
typedef struct _DRIVER
    {
    pNodeData      NodeData;          /* The associated node data. */
    TargetType     TargetType;        /* The type of data targetted by this driver instance. */
    void*          TargetData;        /* A pointer to the specific targetted cluster or search. */
    unsigned int   TargetIndex;       /* An index into the cluster or search (entries only). */
    unsigned char  TargetAttrIndex;   /* An index into an attribute list (for GetNextAttr()). */
    unsigned char  TargetMethodIndex; /* An index into an method list (for GetNextMethod()). */
    }
    DriverData, *pDriverData;

/** Query instance data. **/
typedef struct
    {
    pDriverData    DriverData;      /* The associated driver instance being queried. */
    unsigned int   RowIndex;        /* The selected row of the data targetted by the driver. */
    }
    ClusterQuery, *pClusterQuery;

/** Global storage for caches. **/
struct
    {
    XHashTable SourceCache;
    XHashTable ClusterCache;
    XHashTable SearchCache;
    }
    ClusterCaches;


/** ================ Function Declarations ================ **/
/** ANCHOR[id=functions] **/

/** Note: ci stands for "cluster_internal". **/

/** Parsing Functions. **/
// LINK #parsing
static int ci_ParseAttribute(pStructInf inf, char* attr_name, int datatype, pObjData data, pParamObjects param_list, bool required, bool print_type_error);
static ClusterAlgorithm ci_ParseClusteringAlgorithm(pStructInf cluster_inf, pParamObjects param_list);
static SimilarityMeasure ci_ParseSimilarityMeasure(pStructInf cluster_inf, pParamObjects param_list);
static pSourceData ci_ParseSourceData(pStructInf inf, pParamObjects param_list, char* path);
static pClusterData ci_ParseClusterData(pStructInf inf, pNodeData node_data);
static pSearchData ci_ParseSearchData(pStructInf inf, pNodeData node_data);
static pNodeData ci_ParseNodeData(pStructInf inf, pObject obj);

/** Freeing Functions. **/
// LINK #freeing
static void ci_FreeSourceData(pSourceData source_data);
static void ci_FreeClusterData(pClusterData cluster_data, bool recursive);
static void ci_FreeSearchData(pSearchData search_data);
static void ci_FreeNodeData(pNodeData node_data);

/** Deep Size Computation Functions. **/
// LINK #sizing
static unsigned int ci_SizeOfSourceData(pSourceData source_data);
static unsigned int ci_SizeOfClusterData(pClusterData cluster_data, bool recursive);
static unsigned int ci_SizeOfSearchData(pSearchData search_data);

/** Computation Functions. (Ensure data is computed.) **/
// LINK #computation
static int ci_ComputeSourceData(pSourceData source_data, pObjSession session);
static int ci_ComputeClusterData(pClusterData cluster_data, pNodeData node_data);
static int ci_ComputeSearchData(pSearchData search_data, pNodeData node_data);

/** Parameter Functions. **/
// LINK #params
static int ci_GetParamType(void* inf_v, const char* attr_name);
static int ci_GetParamValue(void* inf_v, char* attr_name, int datatype, pObjData val);
static int ci_SetParamValue(void* inf_v, char* attr_name, int datatype, pObjData val);

/** Driver Functions. **/
// LINK #driver
void* clusterOpen(pObject obj, int mask, pContentType systype, char* usr_type, pObjTrxTree* oxt);
int clusterClose(void* inf_v, pObjTrxTree* oxt);
void* clusterOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt);
void* clusterQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt);
int clusterQueryClose(void* qy_v, pObjTrxTree* oxt);
int clusterGetAttrType(void* inf_v, char* attr_name, pObjTrxTree* oxt);
int clusterGetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt);
pObjPresentationHints clusterPresentationHints(void* inf_v, char* attr_name, pObjTrxTree* oxt);
char* clusterGetFirstAttr(void* inf_v, pObjTrxTree oxt);
char* clusterGetNextAttr(void* inf_v, pObjTrxTree oxt);
int clusterInfo(void* inf_v, pObjectInfo info);

/** Method Execution Functions. **/
// LINK #method
char* clusterGetFirstMethod(void* inf_v, pObjTrxTree oxt);
char* clusterGetNextMethod(void* inf_v, pObjTrxTree oxt);
static int ci_PrintEntry(pXHashEntry entry, void* arg);
static void ci_CacheFreeSourceData(pXHashEntry entry, void* path);
static void ci_CacheFreeCluster(pXHashEntry entry, void* path);
static void ci_CacheFreeSearch(pXHashEntry entry, void* path);
int clusterExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt);

/** Unimplemented DriverFunctions. **/
// LINK #unimplemented
int clusterCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
int clusterDeleteObj(void* inf_v, pObjTrxTree* oxt);
int clusterDelete(pObject obj, pObjTrxTree* oxt);
int clusterRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt);
int clusterWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
int clusterSetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree oxt);
int clusterAddAttr(void* inf_v, char* attr_name, int type, pObjData val, pObjTrxTree oxt);
void* clusterOpenAttr(void* inf_v, char* attr_name, int mode, pObjTrxTree oxt);
int clusterCommit(void* inf_v, pObjTrxTree *oxt);

/** ================ Parsing Functions ================ **/
/** ANCHOR[id=parsing] **/
// LINK #functions

/*** Returns 0 for success and -1 on failure. Promises that mssError() will be
 *** invoked on failure, so the caller need not specify their own error message.
 *** Returns 1 if attribute is available, printing an error if the attribute was
 *** marked as required.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** TODO: Greg
 *** This function took several hours of debugging before it worked at all, and I
 *** still don't know if it works correctly... or really how it works. Please
 *** review this code carefully!
 ***/
static int ci_ParseAttribute(
    pStructInf inf,
    char* attr_name,
    int datatype,
    pObjData data,
    pParamObjects param_list,
    bool required,
    bool print_type_error)
    {
    int ret;
    
    /** Get attribute name. **/
    pStructInf attr_info = stLookup(inf, attr_name);
    if (attr_info == NULL)
	{
	if (required) mssErrorf(1, "Cluster", "'%s' must be specified for clustering.", attr_name);
	return 1;
	}
    ASSERTMAGIC(attr_info, MGK_STRUCTINF);
    
    /** Get the attribute. **/
    tprintf("Invoking ci_ParseAttribute('%s')...\n", attr_name);
    pExpression exp = check_ptr(stGetExpression(attr_info, 0));
    expBindExpression(exp, param_list, EXPR_F_RUNSERVER);
    ret = expEvalTree(exp, param_list);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Expression evaluation failed.");
	goto err;
	}
    
    /** Check for data type mismatch. **/
    if (datatype != exp->DataType)
	{
	mssErrorf(1, "Cluster",
	    "Expected \"%s\" : %s, but got type %s.",
	    attr_name, ci_TypeToStr(datatype), ci_TypeToStr(exp->DataType)
	);
	goto err;
	}
    
    /** Get the data out of the expression. **/
    ret = expExpressionToPod(exp, datatype, data);
    if (ret != 0)
	{
	mssErrorf(1, "Cluster",
	    "Failed to get data of type \"%s\" from exp \"%s\" (error code %d).",
	    ci_TypeToStr(datatype), exp->Name, ret
	);
	goto err;
	}
    
//     const int ret = stGetAttrValueOSML(
// 	attr_info,
// 	datatype,
// 	data,
// 	0,
// 	param_list->Session,
// 	param_list
//     );
//     if (ret == 1)
// 	{
// 	mssErrorf(1, "Cluster",
// 	    "stGetAttrValueOSML('%s') because %s cannot be null.\n"
// 	    "  > Hint: You might have used an undefined variable or forgot to add runserver().",
// 	    attr_name, attr_name
// 	    );
// 	return 1;
// 	}
//     if (ret != 0)
// 	{
// 	if (print_type_error) 
// 	    {
// 	    mssErrorf(1, "Cluster",
// 		"stGetAttrValueOSML('%s') failed (error code %d).\n"
// 		"  > Hint: It might be a type mismatch, or you used an undefined variable.",
// 		attr_name, ret
// 	    );
// 	    }
// 	return ret;
// 	}
    
    return 0;
    
    err:
    mssErrorf(0, "Cluster",
	"Failed to parse attribute \"%s\" from group \"%s\"",
	attr_name, inf->Name
    );
    return -1;
    }


// LINK #functions
/*** Parses a ClusteringAlgorithm from the algorithm field in the pStructInf
 *** representing some structure with that attribute in a parsed structure file.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param inf A parsed pStructInf.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @returns The data algorithm, or ALGORITHM_NULL on failure.
 ***/
static ClusterAlgorithm ci_ParseClusteringAlgorithm(pStructInf inf, pParamObjects param_list)
    {
    /** Get the algorithm attribute. **/
    char* algorithm;
    if (ci_ParseAttribute(inf, "algorithm", DATA_T_STRING, POD(&algorithm),	param_list, true, true) != 0)
	{
	mssErrorf(0, "Cluster", "Failed to parse attribute 'algorithm' in group \"%s\".", inf->Name);
	return ALGORITHM_NULL;
	}
    
    /** Parse known clustering algorithms. **/
    if (!strcasecmp(algorithm, "none"))           return ALGORITHM_NONE;
    if (!strcasecmp(algorithm, "sliding-window")) return ALGORITHM_SLIDING_WINDOW;
    if (!strcasecmp(algorithm, "k-means"))        return ALGORITHM_KMEANS;
    if (!strcasecmp(algorithm, "k-means++"))      return ALGORITHM_KMEANS_PLUS_PLUS;
    if (!strcasecmp(algorithm, "k-medoids"))      return ALGORITHM_KMEDOIDS;
    if (!strcasecmp(algorithm, "db-scan"))        return ALGORITHM_DB_SCAN;
    
    /** Unknown value for clustering algorithm. **/
    mssErrorf(1, "Cluster", "Unknown \"clustering algorithm\": %s", algorithm);
    return ALGORITHM_NULL;
    }


// LINK #functions
/*** Parses a SimilarityMeasure from the similarity_measure field in the given
 *** pStructInf parameter, which represents some structure with that attribute
 *** in a parsed structure file.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param inf A parsed pStructInf.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @returns The similarity measure, or SIMILARITY_NULL on failure.
 ***/
static SimilarityMeasure ci_ParseSimilarityMeasure(pStructInf inf, pParamObjects param_list)
    {
    /** Get the similarity_measure attribute. **/
    char* measure;
    if (ci_ParseAttribute(inf, "similarity_measure", DATA_T_STRING, POD(&measure), param_list, true, true) != 0)
	{
	mssErrorf(0, "Cluster", "Failed to parse attribute 'similarity_measure' in group \"%s\".", inf->Name);
	return SIMILARITY_NULL;
	}
    
    /** Parse known clustering algorithms. **/
    if (!strcasecmp(measure, "cosine"))      return SIMILARITY_COSINE;
    if (!strcasecmp(measure, "levenshtein")) return SIMILARITY_LEVENSHTEIN;
    
    mssErrorf(1, "Cluster", "Unknown \"similarity measure\": %s", measure);
    return SIMILARITY_NULL;
    }


// LINK #functions
/*** Allocates a new pSourceData struct from a parsed pStructInf representing
 *** a .cluster structure file.
 *** 
 *** @attention - Warning: Caching in use.
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param inf A parsed pStructInf for a .cluster structure file.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @param path The file path to the parsed structure file, used to generate
 *** 	cache entry keys.
 *** @returns A new pSourceData struct on success, or NULL on failure.
 ***/
static pSourceData ci_ParseSourceData(pStructInf inf, pParamObjects param_list, char* path)
    {
    char* buf;
    
    /** Get source. **/
    if (ci_ParseAttribute(inf, "source", DATA_T_STRING, POD(&buf), param_list, true, true) != 0) goto err;
    char* source_path = check_ptr(nmSysStrdup(buf));
    
    /** Get attribute name. **/
    if (ci_ParseAttribute(inf, "attr_name", DATA_T_STRING, POD(&buf), param_list, true, true) != 0) goto err;
    char* attr_name = check_ptr(nmSysStrdup(buf));
    
    /** Create cache entry key. **/
    const size_t len = strlen(path) + strlen(source_path) + strlen(attr_name) + 3lu;
    char* key = check_ptr(nmSysMalloc(len * sizeof(char)));
    snprintf(key, len, "%s?%s:%s", path, source_path, attr_name);
    pXHashTable source_cache = &ClusterCaches.SourceCache;
    
    /** Check for a cached version. **/
    pSourceData source_maybe = (pSourceData)xhLookup(source_cache, key);
    if (source_maybe != NULL)
	{
	/** Cache hit. **/
	tprintf("# source: \"%s\"\n", key);
	tprintf("--> Name: %s\n", source_maybe->Name); /* Cause invalid read if cache was incorrectly freed. */
	
	/** Free data we don't need. */
	nmSysFree(source_path);
	nmSysFree(attr_name);
	nmSysFree(key);
	
	/** Return the cached source data. **/
	return source_maybe;
	}
    
    /** Cache miss: Create a new source data object. **/
    pSourceData source_data = check_ptr(nmMalloc(sizeof(SourceData)));
    memset(source_data, 0, sizeof(SourceData));
    source_data->Name = check_ptr(nmSysStrdup(inf->Name));
    source_data->Key = key;
    source_data->SourcePath = source_path;
    source_data->AttrName = attr_name;
    check(objCurrentDate(&source_data->DateCreated));
    
    /** Add the new object to the cache for next time. **/
    tprintf("+ source: \"%s\"\n", key);
    check(xhAdd(source_cache, key, (void*)source_data));
    
    return source_data;
    
    err:
    mssErrorf(0, "Cluster", "Failed to parse source data from group \"%s\" in file: %s", inf->Name, path);
    return NULL;
    }


// LINK #functions
/*** Allocates a new pClusterData struct from a parsed pStructInf.
 *** 
 *** @attention - Warning: Caching in use.
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param inf A parsed pStructInf for a cluster group in a structure file.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @param source_data The pSourceData that clusters are to be built from, also
 *** 	used to generate cache entry keys.
 *** @returns A new pClusterData struct on success, or NULL on failure.
 ***/
static pClusterData ci_ParseClusterData(pStructInf inf, pNodeData node_data)
    {
    int result;
    
    tprintf("Parsing cluster: %s\n", inf->Name);
    
    pParamObjects param_list = node_data->ParamList;
    pSourceData source_data = node_data->SourceData;
    
    /** Allocate space for data struct. **/
    pClusterData cluster_data = check_ptr(nmMalloc(sizeof(ClusterData)));
    memset(cluster_data, 0, sizeof(ClusterData));
    
    /** Basic Properties. **/
    cluster_data->Name = check_ptr(nmSysStrdup(inf->Name));
    cluster_data->SourceData = source_data;
    check(objCurrentDate(&cluster_data->DateCreated));
    
    /** Get algorithm. **/
    cluster_data->ClusterAlgorithm = ci_ParseClusteringAlgorithm(inf, param_list);
    if (cluster_data->ClusterAlgorithm == ALGORITHM_NULL) goto err_free_cluster;
    
    /** Handle no clustering case. **/
    if (cluster_data->ClusterAlgorithm == ALGORITHM_NONE)
	{
	cluster_data->NumClusters = 1u;
	goto parsing_done;
	}
    
    /** Get similarity_measure. **/
    cluster_data->SimilarityMeasure = ci_ParseSimilarityMeasure(inf, param_list);
    if (cluster_data->SimilarityMeasure == SIMILARITY_NULL) goto err_free_cluster;
    
    /** Handle sliding window case. **/
    if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
	goto parsing_done;
    
    /** Get num_clusters. **/
    int num_clusters;
    if (ci_ParseAttribute(inf, "num_clusters", DATA_T_INTEGER, POD(&num_clusters), param_list, true, true) != 0) goto err_free_cluster;
    if (num_clusters < 2)
	{
	mssErrorf(1, "Cluster", "Invalid value for [num_clusters : uint > 1]: %d", num_clusters);
	if (num_clusters == 1) fprintf(stderr, "HINT: Use algorithm=\"none\" to disable clustering.\n");
	goto err_free_cluster;
	}
    cluster_data->NumClusters = (unsigned int)num_clusters;
    tprintf("Got value for num_clusters: %d\n", num_clusters);
    
    /** Get min_improvement. **/
    double improvement;
    result = ci_ParseAttribute(inf, "min_improvement", DATA_T_DOUBLE, POD(&improvement), param_list, false, false);
    if (result == 1) cluster_data->MinImprovement = DEFAULT_MIN_IMPROVEMENT;
    else if (result == 0)
	{
	if (improvement <= 0.0 || 1.0 <= improvement)
	    {
	    mssErrorf(1, "Cluster", "Invalid value for [min_improvement : 0.0 < x < 1.0 | \"none\"]: %g", improvement);
	    goto err_free_cluster;
	    }
	cluster_data->MinImprovement = improvement;
	}
    else if (result == -1)
	{
	char* str;
	result = ci_ParseAttribute(inf, "min_improvement", DATA_T_STRING, POD(&str), param_list, false, true);
	if (result == 0 && !strcasecmp(str, "none"))
	    {
	    /** Specify no min improvement. **/
	    cluster_data->MinImprovement = -INFINITY;
	    }
	}
    if (result == -1) goto err_free_cluster;
    
    /** Get max_iterations. **/
    int max_iterations;
    result = ci_ParseAttribute(inf, "max_iterations", DATA_T_INTEGER, POD(&max_iterations), param_list, false, true);
    if (result == -1) goto err_free_cluster;
    if (result == 0)
	{
	if (max_iterations < 1)
	    {
	    mssErrorf(1, "Cluster", "Invalid value for [max_iterations : uint]: %d", max_iterations);
	    goto err_free_cluster;
	    }
	cluster_data->MaxIterations = (unsigned int)max_iterations;
	}
    else cluster_data->MaxIterations = DEFAULT_MAX_ITERATIONS;
    
    /** Search for sub-clusters. **/
    XArray sub_clusters;
    const int ret = xaInit(&sub_clusters, 4u);
    if (ret != 0)
	{
	mssErrorf(1, "Cluster", "FAIL - xaInit(&sub_clusters, %u): %d", 4u, ret);
	goto err_free_cluster;
	}
    for (unsigned int i = 0u; i < inf->nSubInf; i++)
	{
	/** Check that this is a group (not an attribute). **/
	pStructInf group_inf = inf->SubInf[i];
	ASSERTMAGIC(group_inf, MGK_STRUCTINF);
	if (stStructType(group_inf) != ST_T_SUBGROUP) continue;
	
	/** Select array by group type. **/
	if (strcmp(check_ptr(group_inf->UsrType), "cluster/cluster") != 0) continue;
	
	/** Subcluster found. **/
	pClusterData sub_cluster = ci_ParseClusterData(group_inf, node_data);
	if (sub_cluster == NULL) goto err_free_sub_clusters;
	sub_cluster->Parent = cluster_data;
	xaAddItem(&sub_clusters, sub_cluster);
	}
    cluster_data->nSubClusters = sub_clusters.nItems;
    cluster_data->SubClusters = (cluster_data->nSubClusters > 0u) ? 
	(pClusterData*)ci_xaToTrimmedArray(&sub_clusters)
	: NULL; /* No sub-clusters. */
    xaDeInit(&sub_clusters);
    
    /** Create the cache key. **/
    parsing_done:;
    char* key;
    switch (cluster_data->ClusterAlgorithm)
	{
	case ALGORITHM_NONE:
	    {
	    const size_t len = strlen(source_data->Key) + strlen(cluster_data->Name) + 5lu;
	    key = nmSysMalloc(len * sizeof(char));
	    snprintf(key, len, "%s/%s?%u",
		source_data->Key,
		cluster_data->Name,
		ALGORITHM_NONE
	    );
	    break;
	    }
	
	case ALGORITHM_SLIDING_WINDOW:
	    {
	    const size_t len = strlen(source_data->Key) + strlen(cluster_data->Name) + 8lu;
	    key = nmSysMalloc(len * sizeof(char));
	    snprintf(key, len, "%s/%s?%u&%u",
		source_data->Key,
		cluster_data->Name,
		ALGORITHM_SLIDING_WINDOW,
		cluster_data->SimilarityMeasure
	    );
	    break;
	    }
	
	default:
	    {
	    const size_t len = strlen(source_data->Key) + strlen(cluster_data->Name) + 32lu;
	    key = nmSysMalloc(len * sizeof(char));
	    snprintf(key, len, "%s/%s?%u&%u&%u&%g&%u",
		source_data->Key,
		cluster_data->Name,
		cluster_data->ClusterAlgorithm,
		cluster_data->SimilarityMeasure,
		cluster_data->NumClusters,
		cluster_data->MinImprovement,
		cluster_data->MaxIterations
	    );
	    break;
	    }
	}
    pXHashTable cluster_cache = &ClusterCaches.ClusterCache;
    cluster_data->Key = key;
    
    /** Check for a cached version. **/
    pClusterData cluster_maybe = (pClusterData)xhLookup(cluster_cache, key);
    if (cluster_maybe != NULL)
	{
	/** Cache hit. **/
	tprintf("# cluster: \"%s\"\n", key);
	tprintf("--> Name: %s\n", cluster_maybe->Name); /* Cause invalid read if cache was incorrectly freed. */
	
	/** Free the parsed cluster that we no longer need. */
	ci_FreeClusterData(cluster_data, false);
	nmSysFree(key);
	
	/** Return the cached cluster. **/
	return cluster_maybe;
	}
    
    /** Cache miss. **/
    tprintf("+ cluster: \"%s\"\n", key);
    check(xhAdd(cluster_cache, key, (void*)cluster_data));
    return cluster_data;
    
    /** Error cleanup. **/
    err_free_sub_clusters:
    for (unsigned int i = 0u; i < sub_clusters.nItems; i++)
	ci_FreeClusterData(sub_clusters.Items[i], true);
    xaDeInit(&sub_clusters);
    
    err_free_cluster:
    ci_FreeClusterData(cluster_data, false);
    
    // err:
    mssErrorf(0, "Cluster", "Failed to parse cluster from group \"%s\".", inf->Name);
    return NULL;
    }


// LINK #functions
/*** Allocates a new pSearchData struct from a parsed pStructInf.
 *** 
 *** @attention - Warning: Caching in use.
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param inf A parsed pStructInf for a search group in a structure file.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @param node_data The pNodeData, used to get the param list and to look up
 *** 	the cluster pointed to by the source attribute.
 *** @returns A new pSearchData struct on success, or NULL on failure.
 ***/
static pSearchData ci_ParseSearchData(pStructInf inf, pNodeData node_data)
    {
    tprintf("Parsing search: %s\n", inf->Name);
    
    /** Allocate space for search struct. **/
    pSearchData search_data = nmMalloc(sizeof(SearchData));
    assert(search_data != NULL);
    memset(search_data, 0, sizeof(SearchData));

    /** Get basic information. **/
    search_data->Name = check_ptr(nmSysStrdup(inf->Name));
    check(objCurrentDate(&search_data->DateCreated));
    
    /** Get source. **/
    char* source_name;
    if (ci_ParseAttribute(inf, "source", DATA_T_STRING, POD(&source_name), node_data->ParamList, true, true) != 0) return NULL;
    for (unsigned int i = 0; i < node_data->nClusters; i++)
	{
	pClusterData cluster_data = node_data->Clusters[i];
	if (strcmp(source_name, cluster_data->Name) == 0)
	    {
	    /** Source found. **/
	    search_data->Source = cluster_data;
	    break;
	    }
	
	/** Note: Subclusters not implemented here. **/
	}
    if (search_data->Source == NULL)
	{
	mssErrorf(1, "Cluster", "Could not find cluster %s for search %s.", source_name, search_data->Name);
	goto err_free_search;
	}
    
    /** Get threshold attribute. **/
    if (ci_ParseAttribute(inf, "threshold", DATA_T_DOUBLE, POD(&search_data->Threshold), node_data->ParamList, true, true) != 0) goto err_free_search;
    if (search_data->Threshold <= 0.0 || 1.0 <= search_data->Threshold)
	{
	mssErrorf(1, "Cluster",
	    "Invalid value for [threshold : 0.0 < x < 1.0 | \"none\"]: %g",
	    search_data->Threshold
	);
	goto err_free_search;
	}
    
    /** Get similarity measure. **/
    search_data->SimilarityMeasure = ci_ParseSimilarityMeasure(inf, node_data->ParamList);
    if (search_data->SimilarityMeasure == SIMILARITY_NULL) goto err_free_search;
    
    /** Create cache entry key. **/
    char* source_key = search_data->Source->Key;
    const size_t len = strlen(source_key) + strlen(search_data->Name) + 16lu;
    char* key = nmSysMalloc(len * sizeof(char));
    snprintf(key, len, "%s/%s?%g&%u",
	source_key,
	search_data->Name,
	search_data->Threshold,
	search_data->SimilarityMeasure
    );
    pXHashTable search_cache = &ClusterCaches.SearchCache;
    
    /** Check for a cached version. **/
    pSearchData search_maybe = (pSearchData)xhLookup(search_cache, key);
    if (search_maybe != NULL)
	{
	/** Cache hit. **/
	tprintf("# search: \"%s\"\n", key);
	tprintf("--> Name: %s\n", search_maybe->Name); /* Cause invalid read if cache was incorrectly freed. */
	
	/** Free the parsed search that we no longer need. */
	ci_FreeSearchData(search_data);
	nmSysFree(key);
	
	/** Return the cached search. **/
	return search_maybe;
	}
    
    /** Cache miss. **/
    tprintf("+ search: \"%s\"\n", key);
    check(xhAdd(search_cache, key, (void*)search_data));
    return search_data;
    
    err_free_search:
    ci_FreeSearchData(search_data);
    mssErrorf(0, "Cluster", "Failed to parse search from group \"%s\".", inf->Name);
    return NULL;
    }


// LINK #functions
/*** Allocates a new pNodeData struct from a parsed pStructInf.
 *** 
 *** @attention - Does not use caching directly, but uses subfunctions to
 *** 	handle caching of substructures.
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param inf A parsed pStructInf for the top level group in a .cluster
 *** 	structure file.
 *** @param obj The parent object struct.
 *** @returns A new pNodeData struct on success, or NULL on failure.
 ***/
static pNodeData ci_ParseNodeData(pStructInf inf, pObject obj)
    {
    int ret;
    
    /** Retrieve path so we'll know we have it later. **/
    char* path = ci_file_path(obj);
    
    /** Allocate node struct data. **/
    // pNodeData node_data = NodeData |> sizeof() |> nmMalloc() |> check_ptr();
    pNodeData node_data = check_ptr(nmMalloc(sizeof(NodeData)));
    memset(node_data, 0, sizeof(NodeData));
    node_data->Obj = obj;
    
    /** Set up param list. **/
    node_data->ParamList = check_ptr(expCreateParamList());
    node_data->ParamList->Session = obj->Session;
    ret = expAddParamToList(node_data->ParamList, "parameters", (void*)node_data, 0);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Failed to add parameters to the param list scope (error code %d).", ret);
	goto err_free_node;
	}
    
    /** Set the param functions, defined later in the file. **/
    ret = expSetParamFunctions(
	node_data->ParamList,
	"parameters",
	ci_GetParamType,
	ci_GetParamValue,
	ci_SetParamValue
    );
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Failed to set param functions (error code %d).", ret);
	goto err_free_node;
	}
    
    /** Detect relevant groups. **/
    XArray param_infs, cluster_infs, search_infs;
    check(xaInit(&param_infs, 8));
    check(xaInit(&cluster_infs, 8));
    check(xaInit(&search_infs, 8));
    for (unsigned int i = 0u; i < inf->nSubInf; i++)
	{
	/** Check that this is a group (not an attribute). **/
	pStructInf group_inf = inf->SubInf[i];
	ASSERTMAGIC(group_inf, MGK_STRUCTINF);
	if (stStructType(group_inf) != ST_T_SUBGROUP) continue;
	
	/** Select array by group type. **/
	const char* group_type = group_inf->UsrType;
	if (strcmp(group_type, "cluster/parameter") == 0) check_strict(xaAddItem(&param_infs, group_inf));
	else if (strcmp(group_type, "cluster/cluster") == 0) check_strict(xaAddItem(&cluster_infs, group_inf));
	else if (strcmp(group_type, "cluster/search") == 0) check_strict(xaAddItem(&search_infs, group_inf));
	else
	    {
	    mssErrorf(1, "Cluster",
		"Unkown group type \"%s\" on group \"%s\".",
		group_type, group_inf->Name
	    );
	    goto err_free_arrs;
	    }
	}
    
    /** Extract OpenCtl for use below. **/
    bool has_provided_params = obj != NULL
	&& obj->Pathname != NULL
	&& obj->Pathname->OpenCtl != NULL
	&& obj->Pathname->OpenCtl[obj->SubPtr - 1] != NULL
	&& obj->Pathname->OpenCtl[obj->SubPtr - 1]->nSubInf > 0
	&& obj->Pathname->OpenCtl[obj->SubPtr - 1]->SubInf != NULL;
    int num_provided_params = (has_provided_params) ? obj->Pathname->OpenCtl[obj->SubPtr - 1]->nSubInf : 0;
    pStruct* provided_params = (has_provided_params) ? obj->Pathname->OpenCtl[obj->SubPtr - 1]->SubInf : NULL;
    
    /** Itterate over each param in the structure file. **/
    node_data->nParams = param_infs.nItems;
    const size_t params_size = node_data->nParams * sizeof(pParam);
    node_data->Params = check_ptr(nmMalloc(params_size));
    memset(node_data->Params, 0, params_size);
    for (unsigned int i = 0u; i < node_data->nParams; i++)
	{
	pParam param = paramCreateFromInf(param_infs.Items[i]);
	if (param == NULL)
	    {
	    mssErrorf(0, "Cluster",
		"Failed to create param from inf for param #%u: %s",
		i, ((pStructInf)param_infs.Items[i])->Name
	    );
	    goto err_free_arrs;
	    }
	node_data->Params[i] = param;
	
	/** Check each provided param to see if the user provided value. **/
        for (unsigned int j = 0u; j < num_provided_params; j++)
	    {
	    pStruct provided_param = provided_params[j];
	    if (provided_param == NULL)
		{
		mssErrorf(1, "Cluster", "Provided param struct cannot be NULL.");
		fprintf(stderr,
		    "Debug info: obj->Pathname->OpenCtl[%d]->SubInf[%u] is NULL", 
		    obj->SubPtr - 1, j
		);
		goto err_free_arrs;
		}
	    
	    /** If this provided param value isn't for the param, ignore it. **/
	    if (strcmp(provided_param->Name, param->Name) != 0) continue;
	    
	    /** Matched! The user is providing a value for this param. **/
	    ret = paramSetValueFromInfNe(param, provided_param, 0, node_data->ParamList, obj->Session);
	    if (ret != 0)
		{
		mssErrorf(0, "Cluster",
		    "Failed to set param value from struct info.\n"
		    "  > Param #%u: %s\n"
		    "  > Provided Param #%u: %n\n"
		    "  > Error code: %d",
		    i, param->Name,
		    j, provided_param->Name,
		    ret
		);
		goto err_free_arrs;
		}
	    tprintf("Found provided value for %s, which is now %d\n", param->Name, param->Value->Data.Integer);
	    
	    /** Provided value successfully handled, we're done. **/
	    break;
	    }
	
	/** Invoke param hints parsing. **/
	ret = paramEvalHints(param, node_data->ParamList, obj->Session);
	if (ret != 0)
	    {
	    mssErrorf(0, "Cluster",
		"Failed to evaluate parameter hints for parameter \"%s\" (error code %d).",
		param->Name, ret
	    );
	    goto err_free_arrs;
	    }
	if (strcmp("k", param->Name) == 0) tprintf("Param k is now %d\n", param->Value->Data.Integer);
	}
    check(xaDeInit(&param_infs));
    param_infs.nAlloc = 0;
    
    /** Parse source data. **/
    node_data->SourceData = ci_ParseSourceData(inf, node_data->ParamList, path);
    if (node_data->SourceData == NULL) goto err_free_node;
    
    /** Parse each cluster. **/
    node_data->nClusters = cluster_infs.nItems;
    if (node_data->nClusters > 0)
	{
	const size_t clusters_size = node_data->nClusters * sizeof(pClusterData);
	node_data->Clusters = check_ptr(nmMalloc(clusters_size));
	memset(node_data->Clusters, 0, clusters_size);
	for (unsigned int i = 0u; i < node_data->nClusters; i++)
	    {
	    node_data->Clusters[i] = ci_ParseClusterData(cluster_infs.Items[i], node_data);
	    if (node_data->Clusters[i] == NULL) goto err_free_arrs;
	    }
	}
    else node_data->Clusters = NULL;
    check(xaDeInit(&cluster_infs));
    cluster_infs.nAlloc = 0;
    
    /** Parse each search. **/
    node_data->nSearches = search_infs.nItems;
    if (node_data->nSearches > 0)
	{
	const size_t searches_size = node_data->nSearches * sizeof(pSearchData);
	node_data->Searches = check_ptr(nmMalloc(searches_size));
	memset(node_data->Searches, 0, searches_size);
	for (unsigned int i = 0u; i < node_data->nSearches; i++)
	    {
	    node_data->Searches[i] = ci_ParseSearchData(search_infs.Items[i], node_data);
	    if (node_data->Searches[i] == NULL) goto err_free_node; /* The XArrays are already freed. */
	    }
	}
    else node_data->Searches = NULL;
    check(xaDeInit(&search_infs));
    search_infs.nAlloc = 0;
    
    /** Success. **/
    return node_data;
    
    err_free_arrs:
    if (param_infs.nAlloc != 0) check(xaDeInit(&param_infs));
    if (cluster_infs.nAlloc != 0) check(xaDeInit(&cluster_infs));
    if (search_infs.nAlloc != 0) check(xaDeInit(&search_infs));
    
    err_free_node:
    ci_FreeNodeData(node_data);
    mssErrorf(0, "Cluster", "Failed to parse node from group \"%s\" in file: %s", inf->Name, path);
    return NULL;
    }


/** ================ Freeing Functions ================ **/
/** ANCHOR[id=freeing] **/
// LINK #functions

/** @param source_data A pSourceData struct, freed by this function. **/
static void ci_FreeSourceData(pSourceData source_data)
    {
    /** Free top level attributes, if they exist. **/
    if (source_data->Name != NULL)       nmSysFree(source_data->Name);
    if (source_data->SourcePath != NULL) nmSysFree(source_data->SourcePath);
    if (source_data->AttrName != NULL)   nmSysFree(source_data->AttrName);
    
    /** Free fetched data, if it exists. **/
    if (source_data->Data != NULL)
	{
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    nmSysFree(source_data->Data[i]);
	nmFree(source_data->Data, source_data->nVectors * sizeof(char*));
	source_data->Data = NULL;
	}
    
    /** Free computed vectors, if they exist. **/
    if (source_data->Vectors != NULL)
	{
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    ca_free_vector(source_data->Vectors[i]);
	nmFree(source_data->Vectors, source_data->nVectors * sizeof(pVector));
	source_data->Vectors = NULL;
	}
    
    /** Free the source_data struct. **/
    nmFree(source_data, sizeof(SourceData));
    }


// LINK #functions
/*** Free pClusterData struct with an option to recursively free subclusters.
 *** 
 *** @param cluster_data The cluster data struct to free.
 *** @param recrusive Whether to recursively free subclusters.
 ***/
static void ci_FreeClusterData(pClusterData cluster_data, bool recursive)
    {
    /** Free top level cluster data. **/
    if (cluster_data->Name != NULL) nmSysFree(cluster_data->Name);
    
    /** Free computed data, if it exists. **/
    if (cluster_data->Labels != NULL)
	{
	const unsigned int nVectors = cluster_data->SourceData->nVectors;
	nmFree(cluster_data->Labels, nVectors * sizeof(unsigned int));
	cluster_data->Labels = NULL;
	}
    
    /** Free subclusters recursively. **/
    if (cluster_data->SubClusters != NULL)
	{
	if (recursive)
	    {
	    for (unsigned int i = 0u; i < cluster_data->nSubClusters; i++)
		ci_FreeClusterData(cluster_data->SubClusters[i], recursive);
	    }
	nmFree(cluster_data->SubClusters, cluster_data->nSubClusters * sizeof(void*));
	cluster_data->SubClusters = NULL;
	}
    
    /** Free the cluster struct. **/
    nmFree(cluster_data, sizeof(ClusterData));
    }


// LINK #functions
/** @param search_data A pSearchData struct, freed by this function. **/
static void ci_FreeSearchData(pSearchData search_data)
    {
    if (search_data->Name != NULL) nmSysFree(search_data->Name);
    if (search_data->Dups != NULL)
	{
	for (unsigned int i = 0; i < search_data->nDups; i++)
	    nmFree(search_data->Dups[i], sizeof(Dup));
	nmFree(search_data->Dups, search_data->nDups * sizeof(void*));
	search_data->Dups = NULL;
	}
    nmFree(search_data, sizeof(SearchData));
    }


// LINK #functions
/** @param node_data A pNodeData struct, freed by this function. **/
static void ci_FreeNodeData(pNodeData node_data)
    {
    /** Free parsed params, if they exist. **/
    if (node_data->Params != NULL)
	{
	for (unsigned int i = 0u; i < node_data->nParams; i++)
	    {
	    if (node_data->Params[i] == NULL) break;
	    paramFree(node_data->Params[i]);
	    }
	nmFree(node_data->Params, node_data->nParams * sizeof(pParam));
        }
    if (node_data->ParamList != NULL) expFreeParamList(node_data->ParamList);
    
    /** Free parsed clusters, if they exist. **/
    if (node_data->Clusters != NULL)
	{
	/*** This data is cached, so we should NOT free it!
	 *** The caching system is responsible for the memory.
	 ***/
	nmFree(node_data->Clusters, node_data->nClusters * sizeof(pClusterData));
	node_data->Clusters = NULL;
	}
    
    /** Free parsed searches, if they exist. **/
    if (node_data->Searches != NULL)
	{
	/*** This data is cached, so we should NOT free it!
	 *** The caching system is responsible for the memory.
	 ***/
	nmFree(node_data->Searches, node_data->nSearches * sizeof(pSearchData));
	node_data->Searches = NULL;
	}
	
    /** Free data source, if one exists. **/
    /*** Note: SourceData is freed last since other free functions may need to
     ***       access information from this structure when freeing data.
     ***       (For example, nVector which is used to determine the size of the
     ***        label struct in each cluster.)
     ***/
    if (node_data->SourceData != NULL)
	{
	/*** This data is cached, so we should NOT free it!
	 *** The caching system is responsible for the memory.
	 ***/
	node_data->SourceData = NULL;
	}
    
    /** Free the node data. **/
    nmFree(node_data, sizeof(NodeData));
    }

/** ================ Deep Size Computation Functions ================ **/
/** ANCHOR[id=sizing] **/
// LINK #functions

/*** Returns the deep size of a SourceData struct, including the size of all
 *** allocated substructures. As far as I can tell, this is probably only
 *** useful for cache management and debugging.
 *** 
 *** Note that Key is ignored because it is a pointer to data managed by the
 *** caching systems, so it is not technically part of the struct.
 *** 
 *** @param source_data The source data struct to be queried.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static unsigned int ci_SizeOfSourceData(pSourceData source_data)
    {
    unsigned int size = 0u;
    if (source_data->Name != NULL) size += strlen(source_data->Name) * sizeof(char);
    if (source_data->SourcePath != NULL) size += strlen(source_data->SourcePath) * sizeof(char);
    if (source_data->AttrName != NULL) size += strlen(source_data->AttrName) * sizeof(char);
    if (source_data->Data != NULL)
	{
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    size += strlen(source_data->Data[i]) * sizeof(char);
	size += source_data->nVectors * sizeof(char*);
	}
    if (source_data->Vectors != NULL)
	{
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    size += ca_sparse_len(source_data->Vectors[i]) * sizeof(int);
	size += source_data->nVectors * sizeof(pVector);
	}
    size += sizeof(SourceData);
    return size;
    }


// LINK #functions
/*** Returns the deep size of a ClusterData struct, including the size of all
 *** allocated substructures. As far as I can tell, this is probably only
 *** useful for cache management and debugging.
 *** 
 *** Note that Key is ignored because it is a pointer to data managed by the
 *** caching systems, so it is not technically part of the struct.
 *** 
 *** @param cluster_data The cluster data struct to be queried.
 *** @param recrusive Whether to recursively free subclusters.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static unsigned int ci_SizeOfClusterData(pClusterData cluster_data, bool recursive)
    {
    unsigned int size = 0u;
    if (cluster_data->Name != NULL) size += strlen(cluster_data->Name) * sizeof(char);
    if (cluster_data->Labels != NULL) size += cluster_data->SourceData->nVectors * sizeof(unsigned int);
    if (cluster_data->SubClusters != NULL)
	{
	if (recursive)
	    {
	    for (unsigned int i = 0u; i < cluster_data->nSubClusters; i++)
		size += ci_SizeOfClusterData(cluster_data->SubClusters[i], recursive);
	    }
	size += cluster_data->nSubClusters * sizeof(void*);
	}
    size += sizeof(ClusterData);
    return size;
    }


// LINK #functions
/*** Returns the deep size of a SearchData struct, including the size of all
 *** allocated substructures. As far as I can tell, this is probably only
 *** useful for cache management and debugging.
 *** 
 *** Note that Key is ignored because it is a pointer to data managed by the
 *** caching systems, so it is not technically part of the struct.
 *** 
 *** @param search_data The search data struct to be queried.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static unsigned int ci_SizeOfSearchData(pSearchData search_data)
    {
    unsigned int size = 0u;
    if (search_data->Name != NULL) size += strlen(search_data->Name) * sizeof(char);
    if (search_data->Dups != NULL) size += search_data->nDups * (sizeof(void*) + sizeof(Dup));
    size += sizeof(SearchData);
    return size;
    }


/** ================ Computation Functions ================ **/
/** ANCHOR[id=computation] **/
// LINK #functions

/*** Ensures that the source_data->Data has been fetched from the data source
 *** and that source_data->nVectors has been computed from the fetched data.
 ***
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param source_data The pSourceData affected by the computation.
 *** @param session The current session, used to open the data source.
 *** @returns 0 if successful, or
 ***         -1 other value on failure.
 ***/
static int ci_ComputeSourceData(pSourceData source_data, pObjSession session)
    {
    /** If the vectors are already computed, we're done. **/
    if (source_data->Vectors != NULL) return 0;
    
    /** Handle error case that happens if memory optimizations break. **/
    if (source_data->Data != NULL)
	{
	/*** We have data, but not vectors, which means that this function ran
	 *** before, but the vectors were cleared by ci_GCSourceData(). This
	 *** should only happen if the vectors will not be needed again. Thus,
	 *** clearly something has gone wrong.
	 ***/
	fprintf(stderr, "ERROR:"
	    "\tci_computeSourceData() invoked on source data \"%s\" where\n"
	    "\tvectors were previously freed. There is likely a bug in\n"
	    "\tci_GCSourceData() which caused it to free vectors when we\n"
	    "\tstill needed them.\n",
	    source_data->Name
	);
	fprintf(stderr, "Resolution:\n"
	    "\tThe original data will be dropped and refetched, and the\n"
	    "\tthe vectors will be recomputed, avoiding possible issues\n"
	    "\tfrom stale data.\n"
	);
	
	/** Drop source_data->Data. **/
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    nmSysFree(source_data->Data[i]);
	nmFree(source_data->Data, source_data->nVectors * sizeof(char*));
	source_data->Data = NULL;
	source_data->nVectors = 0;
	}
    
    /** Record the date and time. **/
    /** Even if this computation fails, we may want this information. **/
    check(objCurrentDate(&source_data->DateComputed));
    
    /** Time to play shoots-and-ladders in an error-handling jungle of gotos. **/
    bool successful = false;
    int ret;
    
    /** Open the source path specified by the .cluster file. **/
    tprintf("Openning...\n");
    pObject obj = objOpen(session, source_data->SourcePath, OBJ_O_RDONLY, 0600, "system/directory");
    if (obj == NULL)
	{
	mssErrorf(0, "Cluster",
	    "Failed to open object driver:"
	    "  > Attribute: \"%s\" : String\n"
	    "  > Source Path: %s",
	    source_data->AttrName,
	    source_data->SourcePath
	);
	successful = false;
	goto end;
	}
    
    /** Generate a "query" for retrieving data. **/
    tprintf("Openning query...\n");
    pObjQuery query = objOpenQuery(obj, NULL, NULL, NULL, NULL, 0);
    if (query == NULL)
	{
	mssErrorf(0, "Cluster",
	    "Failed to open query:\n"
	    "  > Attribute: \"%s\" : String\n"
	    "  > Driver Used: %s\n"
	    "  > Source Path: %s",
	    source_data->AttrName,
	    obj->Driver->Name,
	    source_data->SourcePath
	);
	successful = false;
	goto end_close;
	}
    
    /** Initialize an xarray to store the retrieved data. **/
    XArray data_xarray, vector_xarray;
    check(xaInit(&data_xarray, 64));
    check(xaInit(&vector_xarray, 64));
    
    /** Fetch data and build vectors. **/
    tprintf("Skips: ");
    unsigned int i = 0u;
    while (true)
	{
	pObject entry = objQueryFetch(query, O_RDONLY);
	if (entry == NULL) break; /* Done. */
	
	/** Type checking. **/
	const int datatype = objGetAttrType(entry, source_data->AttrName);
	if (datatype == -1)
	    {
	    mssErrorf(0, "Cluster",
		"Failed to get type for %uth entry:\n"
		"  > Attribute: \"%s\" : String\n"
		"  > Driver Used: %s\n"
		"  > Source Path: %s",
		i,
		source_data->AttrName,
		obj->Driver->Name,
		source_data->SourcePath
	    );
	    goto end_free_data;
	    }
	if (datatype != DATA_T_STRING)
	    {
	    mssErrorf(1, "Cluster",
		"Type for %uth entry was not a string:\n"
		"  > Attribute: \"%s\" : %s!!\n"
		"  > Driver Used: %s\n"
		"  > Source Path: %s",
		i,
		source_data->AttrName, ci_TypeToStr(datatype),
		obj->Driver->Name,
		source_data->SourcePath
	    );
	    goto end_free_data;
	    }
	
	/** Get value from database. **/
	char* val;
	ret = objGetAttrValue(entry, source_data->AttrName, DATA_T_STRING, POD(&val));
	if (ret != 0)
	    {
	    tprintf("\n");
	    mssErrorf(0, "Cluster",
		"Failed to value for %uth entry:\n"
		"  > Attribute: \"%s\" : String\n"
		"  > Driver Used: %s\n"
		"  > Source Path: %s\n"
		"  > Error code: %d",
		i,
		source_data->AttrName,
		obj->Driver->Name,
		source_data->SourcePath,
		ret
	    );
	    successful = false;
	    goto end_free_data;
	    }
	
	/** Skip empty strings. **/
	if (strlen(val) == 0)
	    {
	    tprintf("_");
	    check(fflush(stdout));
	    continue;
	    }
	
	/** Convert the string to a vector. **/
	pVector vector = ca_build_vector(val);
	if (vector == NULL)
	    {
	    mssErrorf(1, "Cluster", "Failed to build vectors for string \"%s\".", val);
	    successful = false;
	    goto end_free_data;
	    }
	if (vector[0] == -CA_NUM_DIMS)
	    {
	    mssErrorf(1, "Cluster", "Vector building for string \"%s\" produced no character pairs.", val);
	    successful = false;
	    goto end_free_data;
	    }
	if (vector[0] == -172 && vector[1] == 11 && vector[2] == -78)
	    {
	    /** Skip pVector with no pairs. **/
	    tprintf(".");
	    check(fflush(stdout));
	    ca_free_vector(vector);
	    continue;
	    }
	
	/** Store value. **/
	char* dup_val = check_ptr(nmSysStrdup(val));
	check_strict(xaAddItem(&data_xarray, (void*)dup_val));
	check_strict(xaAddItem(&vector_xarray, (void*)vector));
	
	/** Clean up. **/
	check(objClose(entry));
	}
    tprintf("\nData aquired.\n");
    source_data->nVectors = vector_xarray.nItems;
    
    /** Trim data and store data. **/
    const size_t data_size = source_data->nVectors * sizeof(char*);
    source_data->Data = check_ptr(nmMalloc(data_size));
    memcpy(source_data->Data, data_xarray.Items, data_size);
    check(xaDeInit(&data_xarray));
    data_xarray.nAlloc = 0;
    
    /** Trim data and store vectors. **/
    const size_t vectors_size = source_data->nVectors * sizeof(pVector);
    source_data->Vectors = check_ptr(nmMalloc(vectors_size));
    memcpy(source_data->Vectors, vector_xarray.Items, vectors_size);
    check(xaDeInit(&vector_xarray));
    vector_xarray.nAlloc = 0;
    
    /** Success. **/
    successful = true;
    
    end_free_data:
    if (data_xarray.nAlloc != 0)
	{
	for (unsigned int i = 0u; i < data_xarray.nItems; i++)
	    nmSysFree(data_xarray.Items[i]);
	check(xaDeInit(&data_xarray));
	}
    if (vector_xarray.nAlloc != 0)
	{
	for (unsigned int i = 0u; i < vector_xarray.nItems; i++)
	    ca_free_vector(vector_xarray.Items[i]);
	check(xaDeInit(&vector_xarray));
	}
    
    // end_close_query:
    ret = objQueryClose(query);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Failed to close query (error code %d).", ret);
	// ret = ret; // Fall-through: Continue through failure.
	}
    
    end_close:
    ret = objClose(obj);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Failed to close object driver (error code %d).", ret);
	// ret = ret; // Fall-through: Continue through failure.
	}
    
    end:
    if (!successful) mssErrorf(0, "Cluster", "Vector computation failed.");
    return (successful) ? 0 : -1;
    }


// LINK #functions
/*** Ensures that the cluster_data->Labels has been computed, running the
 *** specified clustering algorithm if necessary.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param cluster_data The pClusterData affected by the computation.
 *** @param node_data The current pNodeData, used to get vectors to cluster.
 *** @returns 0 if successful, or
 ***         -1 other value on failure.
 ***/
static int ci_ComputeClusterData(pClusterData cluster_data, pNodeData node_data)
    {
    /** If the clusters are alreadyd computed, we're done. **/
    if (cluster_data->Labels != NULL) return 0;
    
    /** Make source data available. **/
    pSourceData source_data = node_data->SourceData;
    
    /** We need the vectors to compute clusters. **/
    if (ci_ComputeSourceData(source_data, node_data->ParamList->Session) != 0)
	{
	mssErrorf(0, "Cluster", "Vectors not found.");
	goto err;
	}
    
    /** Record the date and time. **/
    /** Even if this computation fails, we may want this information. **/
    check(objCurrentDate(&cluster_data->DateComputed));
    
    /** Allocate static memory for finding clusters. **/
    const size_t labels_size = source_data->nVectors * sizeof(unsigned int);
    cluster_data->Labels = check_ptr(nmMalloc(labels_size));
    
    /** Execute clustering. **/
    switch (cluster_data->ClusterAlgorithm)
	{
	case ALGORITHM_NONE:
	case ALGORITHM_SLIDING_WINDOW: /* Clusters are not computed separately for performance reasons. */
	    tprintf("Applying no clustering...\n");
	    memset(cluster_data->Labels, 0u, labels_size);
	    break;
	
	case ALGORITHM_KMEANS:
	    /** Check for unimplemented similarity measures. **/
	    if (cluster_data->SimilarityMeasure != SIMILARITY_COSINE)
		{
		mssErrorf(1, "Cluster",
		    "The similarity meausre \"%s\" is not implemented.",
		    ci_SimilarityMeasureToString(cluster_data->SimilarityMeasure)
		);
		goto err;
		}
	    
	    /** kmeans expects clusters to be initialized. **/
	    memset(cluster_data->Labels, 0u, labels_size);
	    
	    tprintf("Running kmeans\n");
	    Timer timer_i, *timer = timer_start(timer_init(&timer_i));
	    ca_kmeans(
		source_data->Vectors,
		source_data->nVectors,
		cluster_data->Labels,
		cluster_data->NumClusters,
		cluster_data->MaxIterations,
		cluster_data->MinImprovement
	    );
	    timer_stop(timer);
	    tprintf("Done after %.4lf.\n", timer_get(timer));
	    break;
	
	default:
	    mssErrorf(1, "Cluster",
		"Clustering algorithm \"%s\" is not implemented.",
		ci_ClusteringAlgorithmToString(cluster_data->ClusterAlgorithm)
	    );
	    goto err;
	}
    
    tprintf("Clustering done.\n");
    return 0;
    
    err:
    mssErrorf(0, "Cluster", "Cluster computation failed for \"%s\".", cluster_data->Name);
    return -1;
    }


// LINK #functions
/*** Ensures that the search_data->Dups has been computed, running the a
 *** search with the specified similarity measure if necessary.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param cluster_data The pClusterData affected by the computation.
 *** @param node_data The current pNodeData, used to get vectors to cluster.
 *** @returns 0 if successful, or
 ***         -1 other value on failure.
 ***/
static int ci_ComputeSearchData(pSearchData search_data, pNodeData node_data)
    {
    int ret;
    
    /** If the clusters are already computed, we're done. **/
    if (search_data->Dups != NULL) return 0;
    
    /** Extract structs. **/
    pClusterData cluster_data = search_data->Source;
    pSourceData source_data = node_data->SourceData;
    
    /** We need the clusters to be able to search them. **/
    ret = ci_ComputeClusterData(cluster_data, node_data);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Search computation failed due to missing clusters.");
	goto err;
	}
    
    /** Check for unimplemented similarity measures. **/
    if (search_data->SimilarityMeasure != SIMILARITY_COSINE)
	{
	mssErrorf(1, "Cluster",
	    "The similarity meausre \"%s\" is not implemented.",
	    ci_SimilarityMeasureToString(search_data->SimilarityMeasure)
	);
	goto err;
	}
    
    /** Record the date and time. **/
    /** Even if this computation fails, we may want this information. **/
    check(objCurrentDate(&search_data->DateComputed));
    
    /** Execute the search. **/
    tprintf("Invoking ca_search.\n");
    Timer timer_i, *timer = timer_start(timer_init(&timer_i));
    pXArray dups_temp = ca_search(
	source_data->Vectors,
	source_data->nVectors,
	cluster_data->Labels,
	search_data->Threshold
    );
    timer_stop(timer);
    if (dups_temp == NULL) goto err;
    tprintf("ca_search done after %.4lf.\n", timer_get(timer));
    
    /** Store dups. **/
    search_data->nDups = dups_temp->nItems;
    search_data->Dups = (dups_temp->nItems == 0)
	? check_ptr(nmMalloc(0))
	: ci_xaToTrimmedArray(dups_temp);
    
    /** Free unused data. **/
    tprintf("Cleanup.\n");
    check(xaFree(dups_temp));
    
    return 0;
    
    err:
    mssErrorf(0, "Cluster", "Search computation failed for \"%s\".", search_data->Name);
    return -1;
    }


/** ================ Parameter Functions ================ **/
/** ANCHOR[id=params] **/
// LINK #functions

/*** Get the type of a parameter. Intended for expSetParamFunctions().
 *** 
 *** @param inf_v Node data containing the list of paramenters.
 *** @param attr_name The name of the requested paramenter.
 *** @returns The datatype, see datatypes.h for a list of valid datatypes.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
static int ci_GetParamType(void* inf_v, const char* attr_name)
    {
    tprintf("Call to ci_GetParamType(\"%s\")\n", attr_name);
    pNodeData node_data = (pNodeData)inf_v;
    
    /** Find the parameter. **/
    for (unsigned int i = 0; i < node_data->nParams; i++)
	{
	pParam param = node_data->Params[i];
	if (strcmp(param->Name, attr_name) != 0) continue;
	
	/** Parameter found. **/
	return (param->Value == NULL) ? DATA_T_UNAVAILABLE : param->Value->DataType;
	}
    
    /** Parameter not found. **/
    return DATA_T_UNAVAILABLE;
    }


// LINK #functions
/*** Get the value of a parameter. Intended for `expSetParamFunctions()`.
 *** 
 *** @attention - Warning: If the retrieved value is `NULL`, the pObjectData
 *** 	val is not updated, and the function returns 1, indicating `NULL`.
 *** 	This is intended behavior, for consistancy with other Centrallix
 *** 	functions, so keep it in mind so you're not surpised.
 *** 
 *** @param inf_v Node data containing the list of paramenters.
 *** @param attr_name The name of the requested paramenter.
 *** @param datatype The expected datatype of the parameter value.
 *** 	See datatypes.h	for a list of valid datatypes.
 *** @param val A pointer to a location where a pointer to the requested
 *** 	data should be stored. Typically, the caller creates a local variable
 *** 	to store this pointer, then passes a pointer to that local variable
 ***    so that they will have a pointer to the data.
 *** 	This buffer will not be modified unless the data is successfully
 *** 	found. If a value other than 0 is returned, the buffer is not updated.
 *** @returns 0 if successsful,
 ***          1 if the variable is null,
 ***         -1 if an error occures.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
static int ci_GetParamValue(void* inf_v, char* attr_name, int datatype, pObjData val)
    {
    tprintf("Call to ci_GetParamValue(\"%s\", %s)\n", attr_name, ci_TypeToStr(datatype));
    pNodeData node_data = (pNodeData)inf_v;
    
    /** Find the parameter. **/
    for (unsigned int i = 0; i < node_data->nParams; i++)
	{
	pParam param = (pParam)node_data->Params[i];
	if (strcmp(param->Name, attr_name) != 0) continue;
	
	tprintf("Param found: Parsing...\n");
	
	/** Parameter found. **/
	if (param->Value == NULL) return 1;
	if (param->Value->Flags & DATA_TF_NULL) return 1;
	if (param->Value->DataType != datatype)
	    {
	    mssErrorf(1, "Cluster", "Type mismatch accessing parameter '%s'.", param->Name);
	    return -1;
	    }
	
	tprintf("Param found: Copying...\n");
	/** Return param value. **/
	objCopyData(&(param->Value->Data), val, datatype);
	return 0;
	}
    
    /** Param not found. **/
    tprintf("Param not found.\n");
    return -1;
    }

// LINK #functions
/** Not implemented. **/
static int ci_SetParamValue(void* inf_v, char* attr_name, int datatype, pObjData val)
    {
    tprintf("Call to ci_SetParamValue(%s, %s)\n", attr_name, ci_TypeToStr(datatype));
    mssErrorf(1, "Cluster", "SetParamValue() is not implemented because clusters are imutable.");
    return -1;
    }


/** ================ Driver functions ================ **/
/** ANCHOR[id=driver] **/
// LINK #functions

/*** Opens a new cluster driver instance by parsing a `.cluster` file found
 *** at the path provided in obj.
 *** 
 *** @param obj The object being opened, including the path, session, and
 *** 	other necessary information.
 *** @param mask Driver permission mask (unused).
 *** @param sys_type ? (unused)
 *** @param usr_type The object system file type being openned. Should always
 *** 	be "system/cluster" because this driver is only registered for that
 *** 	type of file.
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 *** 
 *** @returns A pDriverData struct representing a driver instance, or
 ***          NULL if an error occures.
 ***/
void* clusterOpen(pObject obj, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt)
    {
    tprintf("Warning: clusterOpen(\"%s\") is under active development.\n", ci_file_name(obj));
    
    /** If CREAT and EXCL are specified, create it and fail if it already exists. **/
    pSnNode node_struct = NULL;
    bool can_create = (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements);
    if (can_create && (obj->Mode & O_EXCL))
	{
	node_struct = snNewNode(obj->Prev, usr_type);
	if (node_struct == NULL)
	    {
	    mssErrorf(0, "Cluster", "Failed to EXCL create new node struct.");
	    goto err;
	    }
	}
    
    /** Read the node if it exists. **/
    if (node_struct == NULL)
	node_struct = snReadNode(obj->Prev);
    
    /** If we can't read, create it (if allowed). **/
    if (node_struct == NULL && can_create)
	node_struct = snNewNode(obj->Prev, usr_type);
    
    /** If there still isn't a node, fail early. **/
    if (node_struct == NULL)
	{
	mssErrorf(0, "Cluster", "Failed to create node struct.");
	goto err;
	}
    
    /** Parse node data. **/
    pNodeData node_data = ci_ParseNodeData(node_struct->Data, obj);
    if (node_data == NULL)
	{
	mssErrorf(0, "Cluster", "Failed to parse structure file \"%s\".", ci_file_name(obj));
	goto err;
	}
    node_data->Node = node_struct;
    node_data->Node->OpenCnt++;
    
    /** Allocate driver instance data. **/
    pDriverData driver_data = check_ptr(nmMalloc(sizeof(DriverData)));
    memset(driver_data, 0, sizeof(DriverData));
    driver_data->NodeData = node_data;
        
    /** Detect target from path. **/
    tprintf("Parsing node path: %d %d\n", obj->SubPtr, obj->SubCnt); obj->SubCnt = 0;
    char* target_name = obj_internal_PathPart(obj->Pathname, obj->SubPtr + obj->SubCnt++, 1);
    if (target_name == NULL)
	{
	/** Target found: Root **/
	tprintf("Found target: Root.\n");
	driver_data->TargetType = TARGET_ROOT;
	driver_data->TargetData = (void*)driver_data->NodeData->SourceData;
	return (void*)driver_data; /* Sucess. */
	}
    
    /** Search clusters. **/
    for (unsigned int i = 0u; i < node_data->nClusters; i++)
	{
	pClusterData cluster = node_data->Clusters[i];
	if (strcmp(cluster->Name, target_name) != 0) continue;
	
	/** Target found: Cluster **/
	driver_data->TargetType = TARGET_CLUSTER;
	tprintf("Found target cluster: %s\n", cluster->Name);
	
	/** Check for sub-clusters in the path. **/
	while (true)
	    {
	    /** Decend one path part deeper into the path. **/
	    const char* path_part = obj_internal_PathPart(obj->Pathname, obj->SubPtr + obj->SubCnt++, 1);
	    
	    /** If the path does not go any deeper, we're done. **/
	    if (path_part == NULL)
		{
		driver_data->TargetData = (void*)cluster;
		break;
		}
	    
	    /** Need to go deeper: Search for the requested sub-cluster. **/
	    for (unsigned int i = 0u; i < cluster->nSubClusters; i++)
		{
		pClusterData sub_cluster = cluster->SubClusters[i];
		if (strcmp(sub_cluster->Name, path_part) != 0) continue;
		
		/** Target found: Sub-cluster **/
		tprintf("Found target sub-cluster: %s\n", sub_cluster->Name);
		cluster = sub_cluster;
		goto continue_descent;
		}
		
	    /** Path names sub-cluster that does not exist. **/
	    mssErrorf(1, "Cluster", "Sub-cluster \"%s\" does not exist.", path_part);
	    goto err_free_node;
	    
	    continue_descent:;
	    }
	return (void*)driver_data; /* Sucess. */
	}
    
    /** Search searches. **/
    for (unsigned int i = 0u; i < node_data->nSearches; i++)
	{
	pSearchData search = node_data->Searches[i];
	if (strcmp(search->Name, target_name) != 0) continue;
	
	/** Target found: Search **/
	driver_data->TargetType = TARGET_SEARCH;
	driver_data->TargetData = (void*)search;
	
	/** Check for extra, invalid path parts. **/
	char* extra_data = obj_internal_PathPart(obj->Pathname, obj->SubPtr + obj->SubCnt++, 1);
	if (extra_data != NULL)
	    {
	    mssErrorf(1, "Cluster", "Unknown path part %s.", extra_data);
	    goto err_free_node;
	    }
	tprintf("Found target search: %s %d %d\n", search->Name, obj->SubPtr, obj->SubCnt);
	return (void*)driver_data; /* Sucess. */
	}
    
    /** We were unable to find the requested cluster or search. **/
    mssErrorf(1, "Cluster", "\"%s\" is not the name of a declaired cluster or search.", target_name);
    
    /** Error cleanup. **/
    err_free_node:
    ci_FreeNodeData(node_data);
    nmFree(driver_data, sizeof(DriverData));
    
    err:
    return NULL;
    }


// LINK #functions
/*** Close a cluster driver instance object, releasing any necessary memory
 *** and closing any necessary underlying resources. However, most of that
 *** data will be cached and won't be freed unless the cache is dropped.
 *** 
 *** @param inf_v The affected driver instance.
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 *** @returns 0, success.
 ***/
int clusterClose(void* inf_v, pObjTrxTree* oxt)
    {
    tprintf("Warning: clusterClose() is under active development.\n");
    pDriverData driver_data = (pDriverData)inf_v;
    
    /** Entries are shallow copies so we shouldn't do a deep free. **/
    if (driver_data->TargetType == TARGET_CLUSTER_ENTRY
	|| driver_data->TargetType == TARGET_SEARCH_ENTRY)
	{
	nmFree(driver_data, sizeof(DriverData));
	return 0;
	}
    
    /** Free the node data (which is held in cache). **/
    ci_FreeNodeData(driver_data->NodeData);
    
    /** Free driver data. **/
    nmFree(driver_data, sizeof(DriverData));
    
    return 0;
    }


// LINK #functions
/*** Opens a new query pointing to the first row of the data targetted by
 *** the driver instance struct. The query has an internal index counter
 *** that starts at the first row and increments as data is fetched.
 *** 
 *** @param inf_v The driver instance to be queried.
 *** @param query The query to use on this struct. This is assumed to be
 *** 	handled elsewhere, so we don't read it here (unused).
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 *** @returns The cluster query.
 ***/
void* clusterOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    tprintf("Warning: clusterOpenQuery() is under active development.\n");
    pClusterQuery cluster_query = check_ptr(nmMalloc(sizeof(ClusterQuery)));
    cluster_query->DriverData = (pDriverData)inf_v;
    cluster_query->RowIndex = 0u;
    return cluster_query;
    }


// LINK #functions
/*** Get the next entry as an open driver instance object.
 *** 
 *** @param qy_v A query instance, storing an internal index which is
 *** 	incremented once that data has been fetched.
 *** @param obj Unused.
 *** @param mode Unused.
 *** @param oxt Unused.
 *** @returns pDriverData that is either a cluster entry or search entry,
 *** 	pointing to a specific target index into the relevant data.
 *** 	OR NULL, indicating that all data has been fetched.
 ***/
void* clusterQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    int ret;
    tprintf("Warning: clusterQueryFetch() is under active development.\n");
    pClusterQuery cluster_query = (pClusterQuery)qy_v;
    
    /** Ensure that the data being fetched exists and is computed. **/
    TargetType target_type = cluster_query->DriverData->TargetType, new_target_type;
    unsigned int data_amount = 0u;
    switch (target_type)
	{
	case TARGET_ROOT:
	    mssErrorf(1, "Cluster", "Querying the root node of a cluster file is not allowed.");
	    fprintf(stderr, "  > Hint: Try /<cluster_name> or /<search_name>\n");
	    return NULL;
	
	case TARGET_CLUSTER:
	    {
	    new_target_type = TARGET_CLUSTER_ENTRY;
	    pClusterData target = (pClusterData)cluster_query->DriverData->TargetData;
	    ret = ci_ComputeClusterData(target, cluster_query->DriverData->NodeData);
	    if (ret != 0)
		{
		mssErrorf(0, "Cluster", "Internal cluster computation failed.");
		return NULL;		
		}
	    data_amount = cluster_query->DriverData->NodeData->SourceData->nVectors;
	    break;
	    }
	
	case TARGET_SEARCH:
	    {
	    new_target_type = TARGET_SEARCH_ENTRY;
	    pSearchData target = (pSearchData)cluster_query->DriverData->TargetData;
	    ret = ci_ComputeSearchData(target, cluster_query->DriverData->NodeData);
	    if (ret != 0)
		{
		mssErrorf(0, "Cluster", "Internal search computation failed.");
		return NULL;		
		}
	    data_amount = target->nDups;
	    break;
	    }
	
	case TARGET_CLUSTER_ENTRY:
	case TARGET_SEARCH_ENTRY:
	    mssErrorf(1, "Cluster", "Querying a query result is not allowed.");
	    return NULL;
	
	default:
	    mssErrorf(1, "Cluster", "Unknown target type %u.", target_type);
	    return NULL;
	}
    tprintf("Fetch Index: %u/16 (total: %u)\n", cluster_query->RowIndex, data_amount);
    
    /** Cap results to 16 for faster debugging. TODO: Remove. **/
    data_amount = min(data_amount, 16);
    
    /** Check that the requested data exists, returning null if we've reached the end of the data. **/
    if (cluster_query->RowIndex >= data_amount) return NULL;
    
    /** Create the result struct. **/
    pDriverData driver_data = nmMalloc(sizeof(DriverData));
    assert(driver_data != NULL);
    memcpy(driver_data, cluster_query->DriverData, sizeof(DriverData));
    driver_data->TargetType = new_target_type;
    driver_data->TargetIndex = cluster_query->RowIndex++;
    
    return driver_data;
    }


// LINK #functions
/*** Close a cluster query instance, releasing any necessary memory and
 *** closing any necessary underlying resources. This does not close the
 *** underlying driver instance, which must be closed with clusterClose().
 *** 
 *** @param qy_v The affected query instance.
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 *** @returns 0, success.
 ***/
int clusterQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    tprintf("Warning: clusterQueryClose() is under active development.\n");
    
    nmFree(qy_v, sizeof(ClusterQuery));
    return 0;
    }


// LINK #functions
/*** Get the type of a cluster driver instance attribute.
 *** 
 *** @param inf_v The driver instance.
 *** @param attr_name The name of the requested attribute.
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 *** @returns The datatype, see datatypes.h for a list of valid datatypes.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
int clusterGetAttrType(void* inf_v, char* attr_name, pObjTrxTree* oxt)
    {
    pDriverData driver_data = (pDriverData)inf_v;
    
    /** Guard possible segfault. **/
    if (attr_name == NULL)
	{
	fprintf(stderr, "Warning: Call to clusterGetAttrType() with NULL attribute name.\n");
	return DATA_T_UNAVAILABLE;
	}
    
    /** Performance shortcut for frequently requested attributes: val, val1, val2, and sim. **/
    if (attr_name[0] == 'v' || attr_name[0] == 's') goto handle_targets;
    
    /** Debug info. **/
    if (oxt == NULL) tprintf("  > ");
    tprintf("Call to clusterGetAttrType(%s)\n", attr_name);
    
    /** Types for general attributes. **/
    if (strcmp(attr_name, "name") == 0
	|| strcmp(attr_name, "annotation") == 0
	|| strcmp(attr_name,"content_type") == 0
	|| strcmp(attr_name, "inner_type") == 0
	|| strcmp(attr_name,"outer_type") == 0)
	return DATA_T_STRING;
    if (strcmp(attr_name, "last_modification") == 0)
	return DATA_T_DATETIME;
    if ((strcmp(attr_name, "date_created") == 0
	 || strcmp(attr_name, "date_computed") == 0)
	 &&
	(driver_data->TargetType == TARGET_CLUSTER
	 || driver_data->TargetType == TARGET_SEARCH))
	return DATA_T_DATETIME;
    
    /** Types for specific data targets. **/
    handle_targets:
    switch (driver_data->TargetType)
	{
	case TARGET_ROOT:
	    if (strcmp(attr_name, "source") == 0
		|| strcmp(attr_name, "attr_name") == 0)
		return DATA_T_STRING;
	    break;
	
	case TARGET_CLUSTER:
	    if (strcmp(attr_name, "algorithm") == 0
		|| strcmp(attr_name, "similarity_measure") == 0)
		return DATA_T_STRING;
	    if (strcmp(attr_name, "num_clusters") == 0
		|| strcmp(attr_name, "max_iterations") == 0)
		return DATA_T_INTEGER;
	    if (strcmp(attr_name, "min_improvement") == 0
		|| strcmp(attr_name, "average_similarity") == 0
		|| strcmp(attr_name, "size") == 0)
		return DATA_T_DOUBLE;
	    break;
	
	case TARGET_SEARCH:
	    if (strcmp(attr_name, "source") == 0
		|| strcmp(attr_name, "similarity_measure") == 0)
		return DATA_T_STRING;
	    if (strcmp(attr_name, "threshold") == 0)
		return DATA_T_DOUBLE;
	    break;
		
	case TARGET_CLUSTER_ENTRY:
	    if (strcmp(attr_name, "id") == 0)
		return DATA_T_INTEGER;
	    if (strcmp(attr_name, "val") == 0)
		return DATA_T_STRING;
	    if (strcmp(attr_name, "sim") == 0)
		return DATA_T_DOUBLE;
	    break;
	
	case TARGET_SEARCH_ENTRY:
	    if (strcmp(attr_name, "id1") == 0
		|| strcmp(attr_name, "id2") == 0)
		return DATA_T_INTEGER;
	    if (strcmp(attr_name, "val1") == 0
		|| strcmp(attr_name, "val2") == 0)
		return DATA_T_STRING;
	    if (strcmp(attr_name, "sim") == 0)
		return DATA_T_DOUBLE;
	    break;
	
	default:
	    mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
	    return DATA_T_UNAVAILABLE;
	}
    
    return DATA_T_UNAVAILABLE;
    }


// LINK #functions
/*** Get the value of a cluster driver instance attribute.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param attr_name The name of the requested attribute.
 *** @param datatype The expected datatype of the attribute value.
 *** 	See datatypes.h	for a list of valid datatypes.
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 *** @param val A pointer to a location where a pointer to the requested
 *** 	data should be stored. Typically, the caller creates a local variable
 *** 	to store this pointer, then passes a pointer to that local variable
 ***    so that they will have a pointer to the data.
 *** 	This buffer will not be modified unless the data is successfully
 *** 	found. If a value other than 0 is returned, the buffer is not updated.
 *** @returns 0 if successsful,
 ***         -1 if an error occures.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
int clusterGetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pDriverData driver_data = (pDriverData)inf_v;
    
    /** Guard possible segfault. **/
    if (attr_name == NULL)
	{
	fprintf(stderr, "Warning: Call to clusterGetAttrType() with NULL attribute name.\n");
	return DATA_T_UNAVAILABLE;
	}
    
    /** Performance shortcut for frequently requested attributes: val, val1, val2, and sim. **/
    if (
	(attr_name[0] == 'v' && datatype == DATA_T_STRING) /* val, val1, val2 : String */
     || (attr_name[0] == 's' && datatype == DATA_T_DOUBLE) /* sim : double */
    ) goto handle_targets;
    
    /** Debug info. **/
    tprintf("Call to clusterGetAttrValue(%s)\n", attr_name);
    
    /** Type check. **/
    const int expected_datatype = clusterGetAttrType(inf_v, attr_name, NULL);
    if (datatype != expected_datatype)
	{
	mssErrorf(1, "Cluster",
	    "Type mismatch: Accessing attribute '%s' : %s as type %s.",
	    attr_name, ci_TypeToStr(expected_datatype), ci_TypeToStr(datatype)
	);
	return -1;
	}
    
    /** Handle name and annotation. **/
    if (strcmp(attr_name, "name") == 0)
	{
    	switch (driver_data->TargetType)
	    {
	    case TARGET_ROOT:
		val->String = ((pSourceData)driver_data->TargetData)->Name;
		break;
	    
	    case TARGET_CLUSTER:
	    case TARGET_CLUSTER_ENTRY:
		val->String = ((pClusterData)driver_data->TargetData)->Name;
		break;
	    
	    case TARGET_SEARCH:
	    case TARGET_SEARCH_ENTRY:
		val->String = ((pSearchData)driver_data->TargetData)->Name;
		break;
	    
	    default:
		mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
		return -1;
	    }
	
	return 0;
	}
    if (strcmp(attr_name, "annotation") == 0)
	{
    	switch (driver_data->TargetType)
	    {
	    case TARGET_ROOT: val->String = "Clustering driver."; break;
	    case TARGET_CLUSTER: val->String = "Clustering driver: Cluster."; break;
	    case TARGET_CLUSTER_ENTRY: val->String = "Clustering driver: Cluster Entry."; break;
	    case TARGET_SEARCH: val->String = "Clustering driver: Search."; break;
	    case TARGET_SEARCH_ENTRY: val->String = "Clustering driver: Cluster Entry."; break;
	    
	    default:
		mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
		return -1;
	    }
	return 0;
	}
    
    /** Return the appropriate types. **/
    if (strcmp(attr_name, "outer_type") == 0)
	{
	val->String = "system/row";
	return 0;
	}
    if (strcmp(attr_name, "content_type") == 0
        || strcmp(attr_name, "inner_type") == 0)
	{
	val->String = "system/void";
	return 0;
	}
    
    /** Last modification is not implemented yet. **/
    if (strcmp(attr_name, "last_modification") == 0) return 1; /* null */
    
    /** Handle creation and computation dates. **/
    if (strcmp(attr_name, "date_created") == 0)
	{
	switch (driver_data->TargetType)
	    {
	    case TARGET_ROOT:
	    case TARGET_CLUSTER_ENTRY:
	    case TARGET_SEARCH_ENTRY:
		/** Field is not defined for this target type. **/
		return -1;
	    
	    case TARGET_CLUSTER:
		val->DateTime = &((pClusterData)driver_data->TargetData)->DateCreated;
		return 0;
	    
	    case TARGET_SEARCH:
		val->DateTime = &((pSearchData)driver_data->TargetData)->DateCreated;
		return 0;
	    }
	return -1;
	}
    if (strcmp(attr_name, "date_computed") == 0)
	{
	switch (driver_data->TargetType)
	    {
	    case TARGET_ROOT:
	    case TARGET_CLUSTER_ENTRY:
	    case TARGET_SEARCH_ENTRY:
		/** Field is not defined for this target type. **/
		return -1;
	    
	    case TARGET_CLUSTER:
		{
		pClusterData target = (pClusterData)driver_data->TargetData;
		pDateTime date_time = &target->DateComputed;
		if (date_time->Value == 0) return 1; /* null */
		else val->DateTime = date_time;
		return 0;
		}
	    
	    case TARGET_SEARCH:
		{
		pSearchData target = (pSearchData)driver_data->TargetData;
		pDateTime date_time = &target->DateComputed;
		if (date_time->Value == 0) return 1; /* null */
		else val->DateTime = date_time;
		return 0;
		}
	    }
	
	/** Default: Unknown type. **/
	mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
	return -1;
	}
    
    /** Handle attributes for specific data targets. **/
    handle_targets:
    switch (driver_data->TargetType)
	{
	case TARGET_ROOT:
	    if (strcmp(attr_name, "source") == 0)
		{
		val->String = ((pSourceData)driver_data->TargetData)->SourcePath;
		return 0;
		}
	    if (strcmp(attr_name, "attr_name") == 0)
		{
		val->String = ((pSourceData)driver_data->TargetData)->AttrName;
		return 0;
		}
	    break;
	
	case TARGET_CLUSTER:
	    {
	    pClusterData target = (pClusterData)driver_data->TargetData;
	    
	    if (strcmp(attr_name, "algorithm") == 0)
		{
		val->String = ci_ClusteringAlgorithmToString(target->ClusterAlgorithm);
		return 0;
		}
	    if (strcmp(attr_name, "similarity_measure") == 0)
		{
		val->String = ci_SimilarityMeasureToString(target->SimilarityMeasure);
		return 0;
		}
	    if (strcmp(attr_name, "num_clusters") == 0)
		{
		if (target->NumClusters > INT_MAX)
		    fprintf(stderr, "Warning: num_clusters value of %u exceeds INT_MAX (%d).\n", target->NumClusters, INT_MAX);
		val->Integer = (int)target->NumClusters;
		return 0;
		}
	    if (strcmp(attr_name, "max_iterations") == 0)
		{
		if (target->MaxIterations > INT_MAX)
		    fprintf(stderr, "Warning: max_iterations value of %u exceeds INT_MAX (%d).\n", target->MaxIterations, INT_MAX);
		val->Integer = (int)target->MaxIterations;
		return 0;
		}
	    if (strcmp(attr_name, "min_improvement") == 0)
		{
		val->Double = target->MinImprovement;
		return 0;
		}
	    if (strcmp(attr_name, "average_similarity") == 0
		|| strcmp(attr_name, "size") == 0)
		{
		mssErrorf(1, "Cluster", "average_similarity is not implemented.");
		return -1;
		}
	    break;
	    }
	
	case TARGET_SEARCH:
	    {
	    pSearchData target = (pSearchData)driver_data->TargetData;
	    
	    if (strcmp(attr_name, "source") == 0)
		{
		val->String = target->Source->Name;
		return 0;
		}
	    if (strcmp(attr_name, "similarity_measure") == 0)
		{
		val->String = ci_SimilarityMeasureToString(target->SimilarityMeasure);
		return 0;
		}
	    if (strcmp(attr_name, "threshold") == 0)
		{
		val->Double = target->Threshold;
		return 0;
		}
	    }
	
	case TARGET_CLUSTER_ENTRY:
	    {
	    pClusterData target = (pClusterData)driver_data->TargetData;
	    
	    if (strcmp(attr_name, "id") == 0)
		{
		val->Integer = (int)target->Labels[driver_data->TargetIndex];
		return 0;
		}
	    if (strcmp(attr_name, "val") == 0)
		{
		val->String = driver_data->NodeData->SourceData->Data[driver_data->TargetIndex];
		return 0;
		}
	    if (strcmp(attr_name, "sim") == 0)
		{
		mssErrorf(1, "Cluster", "Cluster entry similarity is not supported.");
		return -1;
		}
	    break;
	    }
	
	case TARGET_SEARCH_ENTRY:
	    {
	    pSearchData target = (pSearchData)driver_data->TargetData;
	    pDup target_dup = target->Dups[driver_data->TargetIndex];
	    
	    if (strcmp(attr_name, "id1") == 0)
		{
		val->Integer = (int)target_dup->id1;
		return 0;
		}
	    if (strcmp(attr_name, "id2") == 0)
		{
		val->Integer = (int)target_dup->id2;
		return 0;
		}
	    if (strcmp(attr_name, "val1") == 0)
		{
		val->String = driver_data->NodeData->SourceData->Data[target_dup->id1];
		// val->Integer = (int)target_dup->id1;
		return 0;
		}
	    if (strcmp(attr_name, "val2") == 0)
		{
		val->String = driver_data->NodeData->SourceData->Data[target_dup->id2];
		// val->Integer = (int)target_dup->id2;
		return 0;
		}
	    if (strcmp(attr_name, "sim") == 0)
		{
		val->Double = target_dup->similarity;
		return 0;
		}
	    break;
	    }
	
	default:
	    mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
	    return -1;
	}
    
    /** Unknown attribute. **/
    char* name;
    clusterGetAttrValue(inf_v, "name", DATA_T_STRING, POD(&name), NULL);
    mssErrorf(1, "Cluster",
	"Unknown attribute '%s' for cluster object %s (target type: %u, \"%s\").",
	attr_name, driver_data->NodeData->SourceData->Name, driver_data->TargetType, name
    );
    
    return -1;
    }


// LINK #functions
/*** Create a new presentation hints object, describing this attribute on the
 *** provided cluster driver instance.
 *** 
 *** Note: expCompileExpression() and nmSysStrdup() are run unchecked because
 *** 	the worst case senario is that the fields are set to null and ignored,
 *** 	which I consider to be better than ending the script because one of
 *** 	them failed.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param attr_name The name of the requested attribute.
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 *** @returns A presentation hints object, if successsful,
 ***          NULL if an error occures.
 ***/
pObjPresentationHints clusterPresentationHints(void* inf_v, char* attr_name, pObjTrxTree* oxt)
    {
    tprintf("Warning: clusterPresentationHints(\"%s\") is under active development.", attr_name);
    pDriverData driver_data = (pDriverData)inf_v;
    
    /** Malloc presentation hints struct. **/
    pObjPresentationHints hints = check_ptr(nmMalloc(sizeof(ObjPresentationHints)));
    memset(hints, 0, sizeof(ObjPresentationHints));
    
    /** Hints that are the same for all fields */
    hints->GroupID = -1;
    hints->VisualLength2 = 1;
    hints->Style     |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
    hints->StyleMask |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
    
    /** Temporary param list for compiling expressions. **/
    pParamObjects tmp_list = check_ptr(expCreateParamList());
    
    if (strcmp(attr_name, "name") == 0)
        {
	hints->Length = 32;
	hints->VisualLength = 16;
	goto end;
	}
    if (strcmp(attr_name, "annotation") == 0)
        {
	hints->Length = 36;
	hints->VisualLength = 36;
	goto end;
	}
    if (strcmp(attr_name, "inner_type") == 0
	|| strcmp(attr_name, "inner_type") == 0
	|| strcmp(attr_name, "outer_type") == 0
	|| strcmp(attr_name, "content_type") == 0
	|| strcmp(attr_name, "last_modification") == 0)
        {
	hints->VisualLength = 30;
	goto end;
        }
    
    if (strcmp(attr_name, "date_created") == 0
	|| strcmp(attr_name, "date_computed") == 0)
	{
	hints->Length = 24;
	hints->VisualLength = 20;
	hints->Format = nmSysStrdup("datetime");
	goto end;
	}
    
    switch (driver_data->TargetType)
	{
	case TARGET_ROOT:
	    if (strcmp(attr_name, "source") == 0)
		{
		hints->Length = _PC_PATH_MAX;
		hints->VisualLength = 64;
		hints->FriendlyName = "Source Path";
		goto end;
		}
	    if (strcmp(attr_name, "attr_name") == 0)
		{
		hints->Length = 255;
		hints->VisualLength = 32;
		hints->FriendlyName = "Attribute Name";
		goto end;
		}
	    break;
	
	case TARGET_CLUSTER:
	    if (strcmp(attr_name, "num_clusters") == 0)
		{
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("2", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("2147483647", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 8;
		hints->VisualLength = 4;
		hints->FriendlyName = nmSysStrdup("Number of Clusters");
		goto end;
		}
	    if (strcmp(attr_name, "min_improvement") == 0)
		{
		/** Min and max values. **/
		hints->DefaultExpr = expCompileExpression("0.0001", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MinValue = expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 16;
		hints->VisualLength = 8;
		hints->FriendlyName = nmSysStrdup("Minimum Improvement Threshold");
		goto end;
		}
	    if (strcmp(attr_name, "max_iterations") == 0)
		{
		/** Min and max values. **/
		hints->DefaultExpr = expCompileExpression("64", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MinValue = expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("2147483647", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 8;
		hints->VisualLength = 4;
		hints->FriendlyName = nmSysStrdup("Maximum Number of Clustering Iterations");
		goto end;
		}
	    if (strcmp(attr_name, "average_similarity") == 0)
		{
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 16;
		hints->VisualLength = 8;
		hints->FriendlyName = nmSysStrdup("Average Similarity");
		goto end;
		}
	    if (strcmp(attr_name, "size") == 0)
		{
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 16;
		hints->VisualLength = 8;
		hints->FriendlyName = nmSysStrdup("Average Cluster Size");
		goto end;
		}
	    if (strcmp(attr_name, "algorithm") == 0)
		{
		/** Enum values. **/
		check(xaInit(&(hints->EnumList), nClusteringAlgorithms));
		for (unsigned int i = 0u; i < nClusteringAlgorithms; i++)
		    check_neg(xaAddItem(&(hints->EnumList), &ALL_CLUSTERING_ALGORITHMS[i]));
		
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		char buf[4u];
		snprintf(buf, sizeof(buf), "%d", nClusteringAlgorithms);
		hints->MaxValue = expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Display flags. **/
		hints->Style     |= OBJ_PH_STYLE_BUTTONS;
		hints->StyleMask |= OBJ_PH_STYLE_BUTTONS;
		
		/** Other hints. **/
		hints->Length = 24;
		hints->VisualLength = 20;
		hints->FriendlyName = nmSysStrdup("Clustering Algorithm");
		goto end;
		}
	    /** Fall-through: Start of overlapping region. **/
	
	case TARGET_SEARCH:
	    if (strcmp(attr_name, "similarity_measure") == 0)
		{
		/** Enum values. **/
		check(xaInit(&(hints->EnumList), nSimilarityMeasures));
		for (unsigned int i = 0u; i < nSimilarityMeasures; i++)
		    check_neg(xaAddItem(&(hints->EnumList), &ALL_SIMILARITY_MEASURES[i]));
		    
		/** Display flags. **/
		hints->Style     |= OBJ_PH_STYLE_BUTTONS;
		hints->StyleMask |= OBJ_PH_STYLE_BUTTONS;
		
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		char buf[4u];
		snprintf(buf, sizeof(buf), "%d", nSimilarityMeasures);
		hints->MaxValue = expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 32;
		hints->VisualLength = 20;
		hints->FriendlyName = nmSysStrdup("Similarity Measure");
		goto end;
		}
	    
	    /** End of overlapping region. **/
	    if (driver_data->TargetType == TARGET_CLUSTER) break;
	    
	    if (strcmp(attr_name, "source") == 0)
		{
		hints->Length = 64;
		hints->VisualLength = 32;
		hints->FriendlyName = nmSysStrdup("Source Cluster Name");
		goto end;
		}
	    if (strcmp(attr_name, "threshold") == 0)
		{
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 16;
		hints->VisualLength = 8;
		hints->FriendlyName = nmSysStrdup("Similarity Threshold");
		goto end;
		}
	    break;
	
	case TARGET_CLUSTER_ENTRY:
	    {
	    pClusterData target = (pClusterData)driver_data->TargetData;
	    
	    if (strcmp(attr_name, "id") == 0)
		{
		pSourceData source_data = (pSourceData)target->SourceData;
		
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (source_data->Vectors != NULL)
		    {
		    char buf[16u];
		    snprintf(buf, sizeof(buf), "%u", source_data->nVectors);
		    hints->MaxValue = expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    }
		
		/** Other hints. **/
		hints->Length = 8;
		hints->VisualLength = 4;
		goto end;
		}
	    if (strcmp(attr_name, "val") == 0)
		{
		/** Other hints. **/
		hints->Length = 255;
		hints->VisualLength = 32;
		hints->FriendlyName = nmSysStrdup("Value");
		goto end;
		}
	    if (strcmp(attr_name, "sim") == 0)
		{
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 16;
		hints->VisualLength = 8;
		hints->FriendlyName = nmSysStrdup("Similarity");
		goto end;
		}
	    break;
	    }
	
	case TARGET_SEARCH_ENTRY:
	    {
	    pSearchData target = (pSearchData)driver_data->TargetData;
	    
	    if (strcmp(attr_name, "id1") == 0 || strcmp(attr_name, "id2") == 0)
		{
		pSourceData source_data = (pSourceData)target->Source->SourceData;
		
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (source_data->Vectors != NULL)
		    {
		    char buf[16u];
		    snprintf(buf, sizeof(buf), "%u", source_data->nVectors);
		    hints->MaxValue = expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    }
		
		/** Other hints. **/
		hints->Length = 8;
		hints->VisualLength = 4;
		goto end;
		}
	    if (strcmp(attr_name, "val1") == 0 || strcmp(attr_name, "val2") == 0)
		{
		/** Other hints. **/
		hints->Length = 255;
		hints->VisualLength = 32;
		hints->FriendlyName = nmSysStrdup("Value");
		goto end;
		}
	    if (strcmp(attr_name, "sim") == 0)
		{
		/** Min and max values. **/
		hints->MinValue = expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		
		/** Other hints. **/
		hints->Length = 16;
		hints->VisualLength = 8;
		hints->FriendlyName = nmSysStrdup("Similarity");
		goto end;
		}
	    break;
	    }
	
	default:
	    mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
	    goto err;
	}
    
    
    end:
    check(expFreeParamList(tmp_list));
    return hints;
    
    err:
    mssErrorf(0, "Cluster", "Failed execute generate presentation hints.");
    return NULL;
    }


// LINK #functions
/*** Returns the name of the first attribute that one can get from
 *** this driver instance (using GetAttrType() and GetAttrValue()).
 *** Resets the internal variable (TargetAttrIndex) used to maintain
 *** itteration state for clusterGetNextAttr().
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt Unused.
 *** @returns The name of the first attribute.
 ***/
char* clusterGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    tprintf("Warning: clusterGetFirstAttr() is under active development.\n");
    pDriverData driver_data = (pDriverData)inf_v;
    driver_data->TargetAttrIndex = 0u;
    return clusterGetNextAttr(inf_v, oxt);
    }


// LINK #functions
/*** Returns the name of the next attribute that one can get from
 *** this driver instance (using GetAttrType() and GetAttrValue()).
 *** Uses an internal variable (TargetAttrIndex) used to maintain
 *** the state of this itteration over repeated calls.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt Unused.
 *** @returns The name of the next attribute.
 ***/
char* clusterGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    tprintf("Warning: clusterGetNextAttr(");
    pDriverData driver_data = (pDriverData)inf_v;
    const unsigned int i = driver_data->TargetAttrIndex++;
    tprintf("%u) is under active development.\n", i);
    switch (driver_data->TargetType)
	{
	case TARGET_ROOT:          return (i < nATTR_ROOT)          ? ATTR_ROOT[i]          : END_OF_ATTRIBUTES;
	case TARGET_CLUSTER:       return (i < nATTR_CLUSTER)       ? ATTR_CLUSTER[i]       : END_OF_ATTRIBUTES;
	case TARGET_SEARCH:        return (i < nATTR_SEARCH)        ? ATTR_SEARCH[i]        : END_OF_ATTRIBUTES;
	case TARGET_CLUSTER_ENTRY: return (i < nATTR_CLUSTER_ENTRY) ? ATTR_CLUSTER_ENTRY[i] : END_OF_ATTRIBUTES;
	case TARGET_SEARCH_ENTRY:  return (i < nATTR_SEARCH_ENTRY)  ? ATTR_SEARCH_ENTRY[i]  : END_OF_ATTRIBUTES;
	default:
	    mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
	    return NULL;
	}
    }


// LINK #functions
/*** Get the capabilities of the driver instance object.
 *** 
 *** @param inf_v The driver instance to be checked.
 *** @param info The struct to be populated with driver flags.
 *** @returns 0 if succesful,
 ***         -1 if the driver is an unimplemented type (should never happen).
 ***/
int clusterInfo(void* inf_v, pObjectInfo info)
    {
    tprintf("Warning: clusterInfo() is under active development.\n");
    pDriverData driver_data = (pDriverData)inf_v;
    pNodeData node_data = (pNodeData)driver_data->NodeData;
    
    /** Reset flags buffer. **/
    info->Flags = 0;
    
    /** Disallow unsupported functionality. **/
    info->Flags |= OBJ_INFO_F_CANT_ADD_ATTR;
    info->Flags |= OBJ_INFO_F_CANT_HAVE_CONTENT;
    info->Flags |= OBJ_INFO_F_NO_CONTENT;
    
    switch (driver_data->TargetType)
	{
	case TARGET_ROOT:
	    info->nSubobjects = node_data->nClusters + node_data->nSearches;
    	    info->Flags |= OBJ_INFO_F_CAN_HAVE_SUBOBJ;
	    info->Flags |= OBJ_INFO_F_SUBOBJ_CNT_KNOWN;
	    info->Flags |= (info->nSubobjects > 0) ? OBJ_INFO_F_HAS_SUBOBJ : OBJ_INFO_F_NO_SUBOBJ;
	    break;
	
	case TARGET_CLUSTER:
    	    info->Flags |= OBJ_INFO_F_CAN_HAVE_SUBOBJ;
	    info->Flags |= OBJ_INFO_F_HAS_SUBOBJ; /* Data must not be empty. */
	    
	    /*** Clusters always have one label per vector.
	     *** If we know how many vectors are in the dataset,
	     *** we know how many labels this cluster will have,
	     *** even if it hasn't been computed yet.
	     ***/
	    if (node_data->SourceData->Vectors != NULL)
		{
		info->Flags |= OBJ_INFO_F_SUBOBJ_CNT_KNOWN;
		info->nSubobjects = node_data->SourceData->nVectors;
		}
	    break;
	
	case TARGET_SEARCH:
	    {
	    pSearchData search_data = (pSearchData)driver_data->TargetData;
    	    info->Flags |= OBJ_INFO_F_CAN_HAVE_SUBOBJ;
	    if (search_data->Dups != NULL)
		{
		info->nSubobjects = search_data->nDups;
		info->Flags |= OBJ_INFO_F_SUBOBJ_CNT_KNOWN;
		info->Flags |= (info->nSubobjects > 0) ? OBJ_INFO_F_HAS_SUBOBJ : OBJ_INFO_F_NO_SUBOBJ;
		}
	    break;
	    }
	
	case TARGET_CLUSTER_ENTRY:
	case TARGET_SEARCH_ENTRY:
	    /** No Subobjects. **/
	    info->Flags |= OBJ_INFO_F_CANT_HAVE_SUBOBJ;
	    info->Flags |= OBJ_INFO_F_NO_SUBOBJ;
	    info->Flags |= OBJ_INFO_F_SUBOBJ_CNT_KNOWN;
	    info->nSubobjects = 0;
	    break;
	
	default:
	    mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
	    goto err;
	}
    
    tprintf("Info result: "INT_TO_BINARY_PATTERN"\n", INT_TO_BINARY(info->Flags));
    return 0;
    
    err:
    mssErrorf(0, "Cluster", "Failed execute get info.");
    return -1;
    }


/** ================ Method Execution Functions ================ **/
/** ANCHOR[id=method] **/
// LINK #functions

/*** Returns the name of the first method that one can execute from
 *** this driver instance (using clusterExecuteMethod()). Resets the
 *** internal variable (TargetMethodIndex) used to maintain itteration
 *** state for clusterGetNextMethod().
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt Unused.
 *** @returns The name of the first methd.
 ***/
char* clusterGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    tprintf("Warning: clusterGetFirstMethod() is under active development.\n");
    pDriverData driver_data = (pDriverData)inf_v;
    driver_data->TargetMethodIndex = 0u;
    return clusterGetNextMethod(inf_v, oxt);
    }


// LINK #functions
/*** Returns the name of the next method that one can get from
 *** this driver instance (using GetAttrType() and GetAttrValue()).
 *** Uses an internal variable (TargetMethodIndex) used to maintain
 *** the state of this itteration over repeated calls.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt Unused.
 *** @returns The name of the next method.
 ***/
char* clusterGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    tprintf("Warning: clusterGetNextMethod(");
    pDriverData driver_data = (pDriverData)inf_v;
    const unsigned int i = driver_data->TargetMethodIndex++;
    tprintf("%u) is under active development.\n", i);
    return (i < nMETHOD_NAME) ? METHOD_NAME[i] : END_OF_METHODS;
    }


// LINK #functions
/** Intended for use in xhForEach(). **/
static int ci_PrintEntry(pXHashEntry entry, void* arg)
    {
    /** Extract entry. **/
    char* key = entry->Key;
    void* data = entry->Data;
    
    /** Extract args. **/
    void** args = (void**)arg;
    unsigned int* type_id_ptr     = (unsigned int*)args[0];
    unsigned int* total_bytes_ptr = (unsigned int*)args[1];
    char* path = (char*)args[2];
    
    /** If a path is provided, check that it matches the start of the key. **/
    if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return 0;
    
    /** Handle type. **/
    char* type;
    char* name;
    unsigned int bytes;
    switch (*type_id_ptr)
	{
	case 1u:
	    {
	    pSourceData source_data = (pSourceData)data;
	    type = "Source";
	    name = source_data->Name;
	    bytes = ci_SizeOfSourceData(source_data);
	    break;
	    }
	case 2u:
	    {
	    pClusterData cluster_data = (pClusterData)data;
	    type = "Cluster";
	    name = cluster_data->Name;
	    bytes = ci_SizeOfClusterData(cluster_data, false);
	    break;
	    }
	case 3u:
	    {
	    pSearchData search_data = (pSearchData)data;
	    type = "Search";
	    name = search_data->Name;
	    bytes = ci_SizeOfSearchData(search_data);
	    break;
	    }
	default: assert(false);
	}
    
    /** Increment total bytes. **/
    *total_bytes_ptr += bytes;
    
    char buf[12];
    snprint_bytes(buf, sizeof(buf), bytes);
    printf("%-8s %-16s %-12s \"%s\"\n", type, name, buf, key);
    
    return 0;
    }


// LINK #functions
/** Intended for use in xhClearKeySafe(). **/
static void ci_CacheFreeSourceData(pXHashEntry entry, void* path)
    {
    /** Extract hash entry. **/
    char* key = entry->Key;
    pSourceData source_data = (pSourceData)entry->Data;
    
    /** If a path is provided, check that it matches the start of the key. **/
    if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return;
    
    /** Free data. **/
    tprintf("- source: \"%s\"\n", key);
    ci_FreeSourceData(source_data);
    nmSysFree(key);
    }


// LINK #functions
/** Intended for use in xhClearKeySafe(). **/
static void ci_CacheFreeCluster(pXHashEntry entry, void* path)
    {
    /** Extract hash entry. **/
    char* key = entry->Key;
    pClusterData cluster_data = (pClusterData)entry->Data;
    
    /** If a path is provided, check that it matches the start of the key. **/
    if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return;
    
    /** Free data. **/
    tprintf("- cluster: \"%s\"\n", key);
    ci_FreeClusterData(cluster_data, false);
    nmSysFree(key);
    }


// LINK #functions
/** Intended for use in xhClearKeySafe(). **/
static void ci_CacheFreeSearch(pXHashEntry entry, void* path)
    {
    /** Extract hash entry. **/
    char* key = entry->Key;
    pSearchData search_data = (pSearchData)entry->Data;
    
    /** If a path is provided, check that it matches the start of the key. **/
    if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return;
    
    /** Free data. **/
    tprintf("- search: \"%s\"\n", key);
    ci_FreeSearchData(search_data);
    nmSysFree(key);
    }


// LINK #functions
/*** Executes a method with the given name.
 *** 
 *** @param inf_v The affected driver instance.
 *** @param method_name The name of the method.
 *** @param param A possibly optional param passed to the method.
 *** @param oxt The object system tree, similar to a kind of "scope" (unused).
 ***/
int clusterExecuteMethod(void* inf_v, char* method_name, pObjData param, pObjTrxTree oxt)
    {
    tprintf("Warning: clusterExecuteMethod(\"%s\") is under active development.\n", method_name);
    pDriverData driver_data = (pDriverData)inf_v;
    
    /** Cache management method. **/
    if (strcmp(method_name, "cache") == 0)
	{
	char* path = NULL;
	
	/** Second parameter is required. **/
	if (param->String == NULL)
	    {
	    mssErrorf(1, "Cluster",
		"param : \"show\" | \"show_all\" | \"drop_all\" is required for the cache method."
	    );
	goto err;
	    }
	
	/** show and show_all. **/
	bool show = false;
	if (strcmp(param->String, "show") == 0)
	    {
	    show = true;
	    path = ci_file_path(driver_data->NodeData->Obj);
	    }
	if (strcmp(param->String, "show_all") == 0) show = true;
	
	if (show)
	    {
	    /** Print cache info table. **/
	    unsigned int i = 1u, source_bytes = 0u, cluster_bytes = 0u, search_bytes = 0u;
	    printf("\nShowing cache for ");
	    if (path != NULL) printf("\"%s\":\n", path);
	    else printf("all files:\n");
	    printf("%-8s %-16s %-12s %s\n", "Type", "Name", "Size", "Cache Entry Key");
	    xhForEach(&ClusterCaches.SourceCache,  ci_PrintEntry, (void*[]){&i, &source_bytes, path}); i++;
	    xhForEach(&ClusterCaches.ClusterCache, ci_PrintEntry, (void*[]){&i, &cluster_bytes, path}); i++;
	    xhForEach(&ClusterCaches.SearchCache,  ci_PrintEntry, (void*[]){&i, &search_bytes, path}); i++;
	    
	    /** Print stats. **/
	    char buf[16];
	    printf("\nCache Stats:\n");
	    printf("%-8s %-4s %-12s\n", "", "#", "Total Size");
	    const int n_sources = ClusterCaches.SourceCache.nItems;
	    snprint_bytes(buf, sizeof(buf), source_bytes);
	    printf("%-8s %-4d %-12s\n", "Source", n_sources, buf);
	    const int n_clusters = ClusterCaches.ClusterCache.nItems;
	    snprint_bytes(buf, sizeof(buf), cluster_bytes);
	    printf("%-8s %-4d %-12s\n", "Cluster", n_clusters, buf);
	    const int n_searches = ClusterCaches.SearchCache.nItems;
	    snprint_bytes(buf, sizeof(buf), search_bytes);
	    printf("%-8s %-4d %-12s\n", "Search", n_searches, buf);
	    snprint_bytes(buf, sizeof(buf), source_bytes + cluster_bytes + search_bytes);
	    printf("%-8s %-4d %-12s\n\n", "Total", n_sources + n_clusters + n_searches, buf);
	    return 0;
	    }
	
	/** drop and drop_all. **/
	bool drop = false;
	if (strcmp(param->String, "drop") == 0)
	    {
	    show = true;
	    path = ci_file_path(driver_data->NodeData->Obj);
	    }
	if (strcmp(param->String, "drop_all") == 0) drop = true;
	
	if (drop)
	    {
	    printf("\nDropping cache for ");
	    if (path != NULL) printf("\"%s\":\n", path);
	    else printf("all files:\n");
	    
	    /*** Free caches in reverse of the order they are created in case
	     *** cached data relies on its source during the freeing process.
	     ***/
	    xhClearKeySafe(&ClusterCaches.SearchCache, ci_CacheFreeSearch, path);
	    xhClearKeySafe(&ClusterCaches.ClusterCache, ci_CacheFreeCluster, path);
	    xhClearKeySafe(&ClusterCaches.SourceCache, ci_CacheFreeSourceData, path);
	    printf("Cache dropped.\n");
	    return 0;
	    }
	
	/** Unknown parameter. **/
	mssErrorf(1, "Cluster",
	    "Expected param : \"show\" | \"show_all\" | \"drop_all\" the cache method, but got: \"%s\"",
	    param->String
	);
	goto err;
	}

    /** Unknown parameter. **/
    mssErrorf(1, "Cluster", "Unknown command: \"%s\"", method_name);
    
    err:
    mssErrorf(0, "Cluster", "Failed execute command.");
    return -1;
    }
    
/** ================ Unimplemented Functions ================ **/
/** ANCHOR[id=unimplemented] **/
// LINK #functions

/** Not implemented. **/
int clusterCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    mssErrorf(1, "Cluster", "clusterCreate() is not implemented.");
    return -ENOSYS;
    }
/** Not implemented. **/
int clusterDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    mssErrorf(1, "Cluster", "clusterDeleteObj() is not implemented.");
    return -1;
    }
/** Not implemented. **/
int clusterDelete(pObject obj, pObjTrxTree* oxt)
    {
    mssErrorf(1, "Cluster", "clusterDelete() is not implemented.");
    return -1;
    }
/** Not implemented. **/
int clusterRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    mssErrorf(1, "Cluster", "clusterRead() not implemented.");
    fprintf(stderr, "HINT: Use queries instead, (e.g. clusterOpenQuery()).\n");
    return -1;
    }
/** Not implemented. **/
int clusterWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    mssErrorf(1, "Cluster", "clusterWrite() not implemented because clusters are imutable.");
    return -1;
    }
/** Not implemented. **/
int clusterSetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree oxt)
    {
    mssErrorf(1, "Cluster", "clusterSetAttrValue() not implemented because clusters are imutable.");
    return -1;
    }
/** Not implemented. **/
int clusterAddAttr(void* inf_v, char* attr_name, int type, pObjData val, pObjTrxTree oxt)
    {
    mssErrorf(1, "Cluster", "clusterAddAttr() not implemented because clusters are imutable.");
    return -1;
    }
/** Not implemented. **/
void* clusterOpenAttr(void* inf_v, char* attr_name, int mode, pObjTrxTree oxt)
    {
    mssErrorf(1, "Cluster", "clusterOpenAttr() not implemented.");
    return NULL;
    }
/** Not implemented. **/
int clusterCommit(void* inf_v, pObjTrxTree *oxt)
    {
    mssErrorf(1, "Cluster", "clusterCommit() not implemented because clusters are imutable.");
    return 0;
    }


// LINK #functions
/*** Initialize the driver. This includes:
 *** - Registering the driver with the objectsystem.
 *** - Registering structs with newmalloc for debugging.
 *** - Initializing global data needed for the driver.
 *** 
 *** @returns 0 if successful, or
 ***          a negative value if an error occured.
 ***/
int clusterInitialize(void)
    {
    int ret;
    /** Initialize library. **/
    ca_init();
    
    /** Allocate the driver. **/
    pObjDriver drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
    if (drv == NULL) return -1;
    memset(drv, 0, sizeof(ObjDriver));
    
    /** Initialize globals. **/
    memset(&ClusterCaches, 0, sizeof(ClusterCaches));
    ret = xhInit(&ClusterCaches.SourceCache, 251, 0);
    if (ret < 0) return ret;
    ret = xhInit(&ClusterCaches.ClusterCache, 251, 0);
    if (ret < 0) return ret;
    ret = xhInit(&ClusterCaches.SearchCache, 251, 0);
    if (ret < 0) return ret;
    
    /** Setup the structure. **/
    strcpy(drv->Name, "clu - Clustering Driver");
    drv->Capabilities = OBJDRV_C_TRANS | OBJDRV_C_FULLQUERY; // OBJDRV_C_TRANS | OBJDRV_C_FULLQUERY;
    ret = xaInit(&(drv->RootContentTypes), 1);
    if (ret < 0) return ret;
    ret = xaAddItem(&(drv->RootContentTypes), "system/cluster");
    if (ret < 0) return ret;
    
    /** Setup the function references. **/
    drv->Open = clusterOpen;
    drv->Close = clusterClose;
    drv->Create = clusterCreate;
    drv->Delete = clusterDelete;
    drv->DeleteObj = clusterDeleteObj;
    drv->OpenQuery = clusterOpenQuery;
    drv->QueryDelete = NULL;
    drv->QueryFetch = clusterQueryFetch;
    drv->QueryClose = clusterQueryClose;
    drv->Read = clusterRead;
    drv->Write = clusterWrite;
    drv->GetAttrType = clusterGetAttrType;
    drv->GetAttrValue = clusterGetAttrValue;
    drv->GetFirstAttr = clusterGetFirstAttr;
    drv->GetNextAttr = clusterGetNextAttr;
    drv->SetAttrValue = clusterSetAttrValue;
    drv->AddAttr = clusterAddAttr;
    drv->OpenAttr = clusterOpenAttr;
    drv->GetFirstMethod = clusterGetFirstMethod;
    drv->GetNextMethod = clusterGetNextMethod;
    drv->ExecuteMethod = clusterExecuteMethod;
    drv->Commit = clusterCommit;
    drv->Info = clusterInfo;
    drv->PresentationHints = clusterPresentationHints;

    /** Register some structures. **/
    nmRegister(sizeof(ClusterData), "ClusterData");
    nmRegister(sizeof(SearchData), "ClusterSearch");
    nmRegister(sizeof(SourceData), "ClusterSourceData");
    nmRegister(sizeof(NodeData), "ClusterNodeData");
    nmRegister(sizeof(DriverData), "ClusterDriverData");
    nmRegister(sizeof(ClusterQuery), "ClusterQuery");
    nmRegister(sizeof(ClusterCaches), "ClusterCaches");
    
    /** Print debug size info. **/
    char cluster_size_buf[16];
    char search_size_buf[16];
    char source_size_buf[16];
    char node_size_buf[16];
    char driver_size_buf[16];
    char query_size_buf[16];
    char caches_size_buf[16];
    tprintf(
	"Cluster driver struct sizes:\n"
	"  > sizeof(ClusterData):   %s\n"
	"  > sizeof(SearchData):    %s\n"
	"  > sizeof(SourceData):    %s\n"
	"  > sizeof(NodeData):      %s\n"
	"  > sizeof(DriverData):    %s\n"
	"  > sizeof(ClusterQuery):  %s\n"
	"  > sizeof(ClusterCaches): %s\n",
	snprint_bytes(cluster_size_buf, sizeof(cluster_size_buf), sizeof(ClusterData)),
	snprint_bytes(search_size_buf,  sizeof(search_size_buf),  sizeof(SearchData)),
	snprint_bytes(source_size_buf,  sizeof(source_size_buf),  sizeof(SourceData)),
	snprint_bytes(node_size_buf,    sizeof(node_size_buf),    sizeof(NodeData)),
	snprint_bytes(driver_size_buf,  sizeof(driver_size_buf),  sizeof(DriverData)),
	snprint_bytes(query_size_buf,   sizeof(query_size_buf),   sizeof(ClusterQuery)),
	snprint_bytes(caches_size_buf,  sizeof(caches_size_buf),  sizeof(ClusterCaches))
    );
    
    /** Register the driver. **/
    ret = objRegisterDriver(drv);
    if (ret < 0) return ret;
    
    return 0;
    }
