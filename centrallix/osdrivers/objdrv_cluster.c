
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
    check(fflush(stdout)); /* Failure ignored. */
    
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
	fprintf(stderr, "Warning: Error truncated (length %d > buffer size %d).\n", num_chars, BUFSIZ);
    
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
    void** result = check_ptr(nmSysMalloc(arr_size));
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
#define END_OF_ARRAY NULL
char* const ATTR_ROOT[] = {
    "source",
    "attr_name",
    END_OF_ARRAY,
};
char* const ATTR_CLUSTER[] =
    {
    "algorithm",
    "similarity_measure",
    "num_clusters",
    "min_improvement",
    "max_iterations",
    "date_created",
    "date_computed",
    END_OF_ARRAY,
    };
char* const ATTR_SEARCH[] =
    {
    "source",
    "threshold",
    "similarity_measure",
    "date_created",
    "date_computed",
    END_OF_ARRAY,
    };
char* const ATTR_CLUSTER_ENTRY[] =
    {
    "items",
    END_OF_ARRAY,
    };
char* const ATTR_SEARCH_ENTRY[] =
    {
    "val1",
    "val2",
    "sim",
    END_OF_ARRAY,
    };

/** Method name list. **/
char* const METHOD_NAME[] =
    {
    "cache",
    END_OF_ARRAY,
    };


/** ================ Struct Declarations ================ **/
/** ANCHOR[id=structs] **/

/*** Represents the data source which may have data already fetched.
 *** 
 *** Memory Stats:
 ***   - Padding: 4 bytes
 ***   - Total size: 72 bytes
 *** 
 *** @skip --> Attribute Data.
 *** @param Name The source name, specified in the .cluster file.
 *** @param Key  The key associated with this object in the SourceDataCache.
 *** @param SourcePath The path to the data source from which to retrieve data.
 *** @param AttrName The name of the attribute to get from the data source.
 *** 
 *** @skip --> Computed data.
 *** @param Strings The data strings to be clustered and searched, or NULL if
 *** 	they have not been fetched from the source.
 *** @param Vectors The cosine comparison vectors from the fetched data, or
 *** 	NULL if they haven't been computed. Note that vectors are no longer
 *** 	needed once all clusters and searches have been computed, so they are
 *** 	automatically freed in that case to save memory.
 *** @param nVectors The number of vectors and data strings.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created and initialized.
 *** @param DateComputed The date and time that the Labels field was computed.
 ***/
typedef struct _SOURCE
    {
    char*          Name;
    char*          Key;
    char*          SourcePath;
    char*          AttrName;
    char**         Strings;
    pVector*       Vectors;
    unsigned int   nVectors;
    DateTime       DateCreated;
    DateTime       DateComputed;
    }
    SourceData, *pSourceData;


/*** Computed data for a single cluster.
 *** 
 *** Memory Stats:
 ***   - Padding: 4 bytes
 ***   - Total size: 24 bytes
 *** 
 *** @param Size The number of items in the cluster.
 *** @param Strings The string values of each item.
 *** @param Vectors The cosine vectors for each item.
 ***/
typedef struct
    {
    unsigned int Size;
    char** Strings;
    pVector* Vectors;
    }
    Cluster, *pCluster;


/*** Data for each cluster. Only attribute data is checked for caching.
 *** 
 *** Memory Stats:
 ***   - Padding: 2 bytes
 ***   - Total size: 96 bytes
 *** 
 *** @skip --> Attribute Data.
 *** @param Name The cluster name, specified in the .cluster file.
 *** @param Key The key associated with this object in the ClusterDataCache.
 *** @param ClusterAlgorithm The clustering algorithm to be used.
 *** @param SimilarityMeasure The similarity measure used to compare items.
 *** @param nClusters The number of clusters. 1 if algorithm = none.
 *** @param MinImprovement The minimum amount of improvement that must be met
 *** 	each clustering iteration. If there is less improvement, the algorithm
 *** 	will stop. The "max" in a .cluster file is represented by -inf.
 *** @param MaxIterations The maximum number of iterations that a clustering
 *** 	algorithm can run for. Note: Sliding window uses this field to store
 *** 	the window_size.
 *** 
 *** @skip --> Relationship Data.
 *** @param nSubClusters The number of subclusters of this cluster.
 *** @param SubClusters A pClusterData array, NULL if nSubClusters == 0.
 *** @param Parent This cluster's parent. NULL if it is not a subcluster.
 *** @param SourceData Pointer to the source data that this cluster uses.
 *** 
 *** @skip --> Computed data.
 *** @param Clusters An array of length num_clusters, NULL if the clusters
 *** 	have not yet been computed.
 *** @param Sims An array of num_vectors elements, where index i stores the
 *** 	similarity of vector i to its assigned cluster. This field is NULL
 *** 	if the clusters have not yet been computed.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created and initialized.
 *** @param DateComputed The date and time that the Labels field was computed.
 ***/
typedef struct _CLUSTER
    {
    char*             Name;
    char*             Key;
    ClusterAlgorithm  ClusterAlgorithm;
    SimilarityMeasure SimilarityMeasure;
    unsigned int      nClusters;
    double            MinImprovement;
    unsigned int      MaxIterations;
    unsigned int      nSubClusters;
    struct _CLUSTER** SubClusters;
    struct _CLUSTER*  Parent;
    pSourceData       SourceData;
    Cluster*          Clusters;
    double*           Sims;
    DateTime          DateCreated;
    DateTime          DateComputed;
    }
    ClusterData, *pClusterData;
    

/*** Data for each search.
 ***
 *** Memory Stats:
 ***   - Padding: 3 bytes
 ***   - Total size: 64 bytes
 *** 
 *** @skip --> Attribute Data.
 *** @param Name The search name, specified in the .cluster file.
 *** @param Key The key associated with this object in the SearchDataCache.
 *** @param Source The cluster from which this search is to be derived.
 *** @param SimilarityMeasure The similarity measure used to compare items.
 *** @param Threshold The minimum similarity threshold for elements to be
 *** 	included in the results of the search.
 *** 
 *** @skip --> Computed data.
 *** @param Dups An array holding the dups found by the search, or NULL if the
 *** 	search has not been computed.
 *** @param nDups The number of dups found.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created and initialized.
 *** @param DateComputed The date and time that the Dups field was computed.
 ***/
typedef struct _SEARCH
    {
    char*             Name;
    char*             Key;
    pClusterData      Source;
    double            Threshold;
    pDup*             Dups;
    unsigned int      nDups;
    SimilarityMeasure SimilarityMeasure;
    DateTime          DateCreated;
    DateTime          DateComputed;
    }
    SearchData, *pSearchData;


/*** Node instance data.
 *** 
 *** Memory Stats:
 ***   - Padding: 4 bytes
 ***   - Total size: 64 bytes
 *** 
 *** @note When a .cluster file is openned, there will be only one node for that
 *** file. However, in the course of the query, many driver instance structs
 *** may be created by functions like clusterQueryFetch(), and closed by the
 *** object system using clusterClose().
 *** 
 *** @param SourceData Data from the provided source.
 *** @param Params A pParam array storing the params in the .cluster file.
 *** @param nParams The number of specified params.
 *** @param ParamList Functions as a "scope" for resolving values during parsing.
 *** @param ClusterDatas A pCluster array storing the clusters in the .cluster file.
 *** 	Will be NULL if nClusters = 0.
 *** @param nClusterDatas The number of specified clusters.
 *** @param SearchDatas A SearchData array storing the searches in the .cluster file.
 *** @param nSearches The number of specified searches.
 *** @param nSearchDatas The parent object used to open this NodeData instance.
 ***/
typedef struct _NODE
    {
    pObject        Parent;
    pParam*        Params;
    pParamObjects  ParamList;
    pSourceData    SourceData;
    pClusterData*  ClusterDatas;
    pSearchData*   SearchDatas;
    unsigned int   nParams;
    unsigned int   nClusterDatas;
    unsigned int   nSearchDatas;
    }
    NodeData, *pNodeData;

/*** Driver instance data.
 ***
 *** Memory Stats:
 ***   - Padding: 1 bytes
 ***   - Total size: 24 bytes
 ***  
 *** This struct can be thought of like a "pointer" to specific data accessible
 *** through the stored pNodeData struct. This struct also communicates whether
 *** that data is guaranteed to have been computed.
 *** 
 *** For example, if target type is the root, a cluster, or a search, no data
 *** is guaranteed to be computed. These three types can be returned from
 *** clusterOpen(), based on the provided path.
 *** 
 *** Alternatively, a cluster entry or search entry can be targetted by calling
 *** fetch on a query pointing to a driver instance that targets a cluster or
 *** search (respectively). These two entry target types ensure that the data
 *** they indicate has been computed, so the GetAttrType() and GetAttrValue()
 *** functions do not need to check this repeatedly each time they are called.
 *** 
 *** @param NodeData The associated node data struct. There can be many driver
 *** 	instances pointing to one NodeData at a time, but each driver instance
 *** 	always points to singular NodeData struct.
 *** @param TargetType The type of data targetted (see above).
 *** @param TargetData If target type is:
 *** ```csv 
 *** 	Root:                    A pointer to the SourceData struct.
 *** 	Cluster or ClusterEntry: A pointer to the targetted cluster.
 *** 	Search or SearchEntry:   A pointer to the targetted search.
 *** ```
 *** @param TargetAttrIndex An index into an attribute list (for GetNextAttr()).
 *** @param TargetMethodIndex An index into an method list (for GetNextMethod()).
 ***/
typedef struct _DRIVER
    {
    pNodeData      NodeData;
    void*          TargetData;
    unsigned int   TargetIndex;
    unsigned char  TargetAttrIndex;
    unsigned char  TargetMethodIndex;
    TargetType     TargetType;
    }
    DriverData, *pDriverData;

/*** Query instance data.
 ***
 *** Memory Stats:
 ***   - Padding: 4 bytes
 ***   - Total size: 16 bytes
 ***
 *** @param DriverData The associated driver instance being queried.
 *** @param RowIndex The selected row of the data targetted by the driver.
 ***/
typedef struct
    {
    pDriverData    DriverData;
    unsigned int   RowIndex;
    }
    ClusterQuery, *pClusterQuery;


/** Global storage for caches. **/
struct
    {
    XHashTable SourceDataCache;
    XHashTable ClusterDataCache;
    XHashTable SearchDataCache;
    }
    ClusterDriverCaches;


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
static void ci_ClearCaches(void);

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

/** Format a hint to give to the user. **/
static void ci_GiveHint(const char* hint)
    {
    fprintf(stderr, "  > Hint: Did you mean \"%s\"?\n", hint);
    }

/*** Given the user a hint when they specify an invalid string for a field
 *** where we know the list of valid strings. The hint is only displayed if
 *** their string is close enough to a valid string.
 *** 
 *** @param value The value the user gave.
 *** @param valid_values The valid values that could be what they meant.
 *** @param n_valid_values The number of valid values. Specify 0 to detect
 *** 	length on a null terminated array of values.
 *** @returns Whether a hint was given.
 ***/
static bool ci_TryHint(char* value, char** valid_values, const unsigned int n_valid_values)
    {
    char* guess = ca_most_similar(value, (void**)valid_values, n_valid_values, ca_lev_compare, 0.5);
    if (guess == NULL) return false; /* No hint. */
    
    /** Issue hint. **/
    ci_GiveHint(guess);
    return true;
    }


// LINK #functions
/*** Returns 0 for success and -1 on failure. Promises that mssError() will be
 *** invoked on failure, so the caller need not specify their own error message.
 *** Returns 1 if attribute is available, printing an error if the attribute was
 *** marked as required.
 *** 
 *** @attention - Promises that a failure invokes mssError() at least once.
 *** 
 *** TODO: Greg - Review Carefully.
 *** This function took a lot of debugging to get it to work. Please make sure
 *** it works correctly and properly requires runserver() for dynamic attributes.
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
    tprintf("Invoking ci_ParseAttribute('%s').\n", attr_name);
    
    /** Get attribute inf. **/
    pStructInf attr_info = stLookup(inf, attr_name);
    if (attr_info == NULL)
	{
	if (required) mssErrorf(1, "Cluster", "'%s' must be specified for clustering.", attr_name);
	return 1;
	}
    ASSERTMAGIC(attr_info, MGK_STRUCTINF);
    
    /** Allocate expression. **/
    pExpression exp = check_ptr(stGetExpression(attr_info, 0));
    if (exp == NULL) goto err;
    
    /** Bind parameters. **/
    /** TODO: Greg - What does this return? How do I know if it fails? **/
    expBindExpression(exp, param_list, EXPR_F_RUNSERVER);
    
    /** Evaluate expression. **/
    ret = expEvalTree(exp, param_list);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Expression evaluation failed (error code %d).", ret);
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
	    "Failed to get \"%s\" : %s using expression \"%s\" (error code %d).",
	    attr_name, ci_TypeToStr(datatype), exp->Name, ret
	);
	goto err;
	}
    
    /** Success. **/
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
 *** @attention - Promises that a failure invokes mssError() at least once.
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
    if (ci_ParseAttribute(inf, "algorithm", DATA_T_STRING, POD(&algorithm), param_list, true, true) != 0)
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
    
    /** Attempt to give a hint. **/
    char* all_names[nClusteringAlgorithms] = {NULL};
    for (unsigned int i = 0u; i < nClusteringAlgorithms; i++)
	all_names[i] = ci_ClusteringAlgorithmToString(ALL_CLUSTERING_ALGORITHMS[i]);
    if (ci_TryHint(algorithm, all_names, nClusteringAlgorithms));
    else if (strcasecmp(algorithm, "sliding") == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_SLIDING_WINDOW));
    else if (strcasecmp(algorithm, "window") == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_SLIDING_WINDOW));
    else if (strcasecmp(algorithm, "null") == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_NONE));
    else if (strcasecmp(algorithm, "nothing") == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_NONE));
    
    /** Fail. **/
    return ALGORITHM_NULL;
    }


// LINK #functions
/*** Parses a SimilarityMeasure from the similarity_measure field in the given
 *** pStructInf parameter, which represents some structure with that attribute
 *** in a parsed structure file.
 *** 
 *** @attention - Promises that a failure invokes mssError() at least once.
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
    
    /** Unknown similarity measure. **/
    mssErrorf(1, "Cluster", "Unknown \"similarity measure\": %s", measure);
    
    /** Attempt to give a hint. **/
    char* all_names[nSimilarityMeasures] = {NULL};
    for (unsigned int i = 0u; i < nSimilarityMeasures; i++)
	all_names[i] = ci_SimilarityMeasureToString(ALL_SIMILARITY_MEASURES[i]);
    if (ci_TryHint(measure, all_names, nSimilarityMeasures));
    else if (strcasecmp(measure, "cos") == 0) ci_GiveHint(ci_SimilarityMeasureToString(SIMILARITY_COSINE));
    else if (strcasecmp(measure, "lev") == 0) ci_GiveHint(ci_SimilarityMeasureToString(SIMILARITY_LEVENSHTEIN));
    else if (strcasecmp(measure, "edit-dist") == 0) ci_GiveHint(ci_SimilarityMeasureToString(SIMILARITY_LEVENSHTEIN));
    else if (strcasecmp(measure, "edit-distance") == 0) ci_GiveHint(ci_SimilarityMeasureToString(SIMILARITY_LEVENSHTEIN));
    
    /** Fail. **/
    return SIMILARITY_NULL;
    }


// LINK #functions
/*** Allocates a new pSourceData struct from a parsed pStructInf representing
 *** a .cluster structure file.
 *** 
 *** @attention - Warning: Caching in use.
 *** @attention - Promises that a failure invokes mssError() at least once.
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
    if (source_path == NULL) goto err;
    
    /** Get attribute name. **/
    if (ci_ParseAttribute(inf, "attr_name", DATA_T_STRING, POD(&buf), param_list, true, true) != 0) goto err;
    char* attr_name = check_ptr(nmSysStrdup(buf));
    if (attr_name == NULL) goto err_free_path;
    
    /** Create cache entry key. **/
    const size_t len = strlen(path) + strlen(source_path) + strlen(attr_name) + 3lu;
    char* key = check_ptr(nmSysMalloc(len * sizeof(char)));
    if (key == NULL) goto err_free_attr;
    snprintf(key, len, "%s?%s:%s", path, source_path, attr_name);
    
    /** Check for a cached version. **/
    pSourceData source_maybe = (pSourceData)xhLookup(&ClusterDriverCaches.SourceDataCache, key);
    if (source_maybe != NULL)
	{
	/** Cache hit. **/
	tprintf("# source: \"%s\"\n", key);
	
	/** Cause an imediate invalid read if cache was incorrectly freed. **/
	tprintf("--> Name: %s\n", source_maybe->Name);
	
	/** Free data we don't need. **/
	nmSysFree(source_path);
	nmSysFree(attr_name);
	nmSysFree(key);
	
	/** Return the cached source data. **/
	return source_maybe;
	}
    
    /** Cache miss: Create a new source data object. **/
    pSourceData source_data = check_ptr(nmMalloc(sizeof(SourceData)));
    if (source_data == NULL) goto err_free_key;
    memset(source_data, 0, sizeof(SourceData));
    source_data->Key = key;
    source_data->SourcePath = source_path;
    source_data->AttrName = attr_name;
    source_data->Name = check_ptr(nmSysStrdup(inf->Name));
    if (source_data->Name == NULL) goto err_free_source;
    if (!check(objCurrentDate(&source_data->DateCreated))) goto err_free_source;
    
    /** Add the new object to the cache for next time. **/
    tprintf("+ source: \"%s\"\n", key);
    if (!check(xhAdd(&ClusterDriverCaches.SourceDataCache, key, (void*)source_data)))
	goto err_free_source;
    
    /** Success. **/
    return source_data;
    
    /** Error handling. **/
    err_free_source:
    ci_FreeSourceData(source_data);
    nmSysFree(key);
    goto err;
    
    err_free_key:
    nmSysFree(key);
    
    err_free_attr:
    nmSysFree(attr_name);
    
    err_free_path:
    nmSysFree(source_path);
    
    err:
    mssErrorf(0, "Cluster",
	"Failed to parse source data from group \"%s\" in file: %s",
	inf->Name, path
    );
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
    
    /** Extract values. **/
    pParamObjects param_list = node_data->ParamList;
    pSourceData source_data = node_data->SourceData;
    
    /** Allocate space for data struct. **/
    pClusterData cluster_data = check_ptr(nmMalloc(sizeof(ClusterData)));
    if (cluster_data == NULL) goto err;
    memset(cluster_data, 0, sizeof(ClusterData));
    
    /** Basic Properties. **/
    cluster_data->Name = check_ptr(nmSysStrdup(inf->Name));
    if (cluster_data->Name == NULL) goto err_free_cluster;
    cluster_data->SourceData = check_ptr(source_data);
    if (cluster_data->SourceData == NULL) goto err_free_cluster;
    if (!check(objCurrentDate(&cluster_data->DateCreated))) goto err_free_cluster;
    
    /** Get algorithm. **/
    cluster_data->ClusterAlgorithm = ci_ParseClusteringAlgorithm(inf, param_list);
    if (cluster_data->ClusterAlgorithm == ALGORITHM_NULL) goto err_free_cluster;
    
    /** Handle no clustering case. **/
    if (cluster_data->ClusterAlgorithm == ALGORITHM_NONE)
	{
	cluster_data->nClusters = 1u;
	goto parsing_done;
	}
    
    /** Get similarity_measure. **/
    cluster_data->SimilarityMeasure = ci_ParseSimilarityMeasure(inf, param_list);
    if (cluster_data->SimilarityMeasure == SIMILARITY_NULL) goto err_free_cluster;
    
    /** Handle sliding window case. **/
    if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
	{
	/** Sliding window doesn't allocate any clusters. **/
	cluster_data->nClusters = 0u;
	
        /** Get window_size. **/
        int window_size;
        if (ci_ParseAttribute(inf, "window_size", DATA_T_INTEGER, POD(&window_size), param_list, true, true) != 0)
	    goto err_free_cluster;
        if (window_size < 1)
	    {
	    mssErrorf(1, "Cluster", "Invalid value for [window_size : uint > 0]: %d", window_size);
	    goto err_free_cluster;
	    }
	
	/** Store value. **/
	cluster_data->MaxIterations = (unsigned int)window_size;
	goto parsing_done;
	}
    
    /** Get num_clusters. **/
    int num_clusters;
    if (ci_ParseAttribute(inf, "num_clusters", DATA_T_INTEGER, POD(&num_clusters), param_list, true, true) != 0)
	goto err_free_cluster;
    if (num_clusters < 2)
	{
	mssErrorf(1, "Cluster", "Invalid value for [num_clusters : uint > 1]: %d", num_clusters);
	if (num_clusters == 1) fprintf(stderr, "HINT: Use algorithm=\"none\" to disable clustering.\n");
	goto err_free_cluster;
	}
    cluster_data->nClusters = (unsigned int)num_clusters;
    
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
	
	/** Successfully got value. **/
	cluster_data->MinImprovement = improvement;
	}
    else if (result == -1)
	{
	char* str;
	result = ci_ParseAttribute(inf, "min_improvement", DATA_T_STRING, POD(&str), param_list, false, true);
	if (result != 0) goto err_free_cluster;
	if (strcasecmp(str, "none") != 0)
	    {
	    mssErrorf(1, "Cluster", "Invalid value for [min_improvement : 0.0 < x < 1.0 | \"none\"]: %s", str);
	    goto err_free_cluster;
	    }
	
	/** Successfully got none. **/
	cluster_data->MinImprovement = -INFINITY;
	}
    
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
    if (!check(xaInit(&sub_clusters, 4u))) goto err_free_cluster;
    for (unsigned int i = 0u; i < inf->nSubInf; i++)
	{
	pStructInf sub_inf = check_ptr(inf->SubInf[i]);
	ASSERTMAGIC(sub_inf, MGK_STRUCTINF);
	char* name = sub_inf->Name;
	
	/** Handle various struct types. **/
	const int struct_type = stStructType(sub_inf);
	switch (struct_type)
	    {
	    case ST_T_ATTRIB:
		{
		/** Valid attribute names. **/
		char* attrs[] = {
		     "algorithm",
		     "similarity_measure",
		     "num_clusters",
		     "min_improvement",
		     "max_iterations",
		     "window_size",
		};
		const unsigned int nattrs = sizeof(attrs) / sizeof(char*);
		
		/** Ignore valid attribute names. **/
		bool is_valid = false;
		for (unsigned int i = 0u; i < nattrs; i++)
		    {
		    if (strcmp(name, attrs[i]) == 0)
			{
			is_valid = true;
			break;
			}
		    }
		if (is_valid) continue; /* Next inf. */
		
		/** Give the user a warning, and attempt to give them a hint. **/
		fprintf(stderr, "Warning: Unknown attribute '%s' in cluster \"%s\".\n", name, inf->Name);
		if (ci_TryHint(name, attrs, nattrs));
		else if (strcasecmp(name, "k") == 0) ci_GiveHint("num_clusters");
		else if (strcasecmp(name, "threshold") == 0) ci_GiveHint("min_improvement");
		
		break;
		}
	    
	    case ST_T_SUBGROUP:
		{
		/** Select array by group type. **/
		char* group_type = check_ptr(sub_inf->UsrType);
		if (group_type == NULL) goto err_free_subclusters;
		if (strcmp(group_type, "cluster/cluster") != 0)
		    {
		    fprintf(stderr,
			"Warning: Unknown group \"%s\" : \"%s\" in cluster \"%s\".\n",
			name, group_type, inf->Name
		    );
		    continue;
		    }
		
		/** Subcluster found. **/
		pClusterData sub_cluster = ci_ParseClusterData(sub_inf, node_data);
		if (sub_cluster == NULL) goto err_free_subclusters;
		sub_cluster->Parent = cluster_data;
		if (!check_neg(xaAddItem(&sub_clusters, sub_cluster))) goto err_free_subclusters;
		
		break;
		}
	    
	    default:
		{
		mssErrorf(1, "Cluster",
		    "Warning: Unknown struct type %d in cluster \"%s\".",
		    struct_type, inf->Name
		);
		goto err_free_subclusters;
		}
	    }
	}
    cluster_data->nSubClusters = sub_clusters.nItems;
    cluster_data->SubClusters = (cluster_data->nSubClusters > 0u) ? 
	(pClusterData*)ci_xaToTrimmedArray(&sub_clusters)
	: NULL; /* No sub-clusters. */
    check(xaDeInit(&sub_clusters)); /* Failure ignored. */
    
    /** Create the cache key. **/
    parsing_done:;
    char* key;
    switch (cluster_data->ClusterAlgorithm)
	{
	case ALGORITHM_NONE:
	    {
	    const size_t len = strlen(source_data->Key) + strlen(cluster_data->Name) + 8lu;
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
	    const size_t len = strlen(source_data->Key) + strlen(cluster_data->Name) + 16lu;
	    key = nmSysMalloc(len * sizeof(char));
	    snprintf(key, len, "%s/%s?%u&%u&%u",
		source_data->Key,
		cluster_data->Name,
		ALGORITHM_SLIDING_WINDOW,
		cluster_data->SimilarityMeasure,
		cluster_data->MaxIterations
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
		cluster_data->nClusters,
		cluster_data->MinImprovement,
		cluster_data->MaxIterations
	    );
	    break;
	    }
	}
    cluster_data->Key = key;
    
    /** Check for a cached version. **/
    pClusterData cluster_maybe = (pClusterData)xhLookup(&ClusterDriverCaches.ClusterDataCache, key);
    if (cluster_maybe != NULL)
	{
	/** Cache hit. **/
	tprintf("# cluster: \"%s\"\n", key);
	
	/** Cause invalid read if cache was incorrectly freed. **/
	tprintf("--> Name: %s\n", cluster_maybe->Name);
	
	/** Free the parsed cluster that we no longer need. */
	ci_FreeClusterData(cluster_data, false);
	nmSysFree(key);
	
	/** Return the cached cluster. **/
	return cluster_maybe;
	}
    
    /** Cache miss. **/
    tprintf("+ cluster: \"%s\"\n", key);
    if (!check(xhAdd(&ClusterDriverCaches.ClusterDataCache, key, (void*)cluster_data))) goto err_free_key;
    return cluster_data;
    
    /** Error cleanup. **/
    err_free_key:
    nmSysFree(key);
    
    err_free_subclusters:
    for (unsigned int i = 0u; i < sub_clusters.nItems; i++)
	ci_FreeClusterData(sub_clusters.Items[i], true);
    check(xaDeInit(&sub_clusters)); /* Failure ignored. */
    
    err_free_cluster:
    ci_FreeClusterData(cluster_data, false);
    
    err:
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
    pSearchData search_data = check_ptr(nmMalloc(sizeof(SearchData)));
    if (search_data == NULL) goto err;
    memset(search_data, 0, sizeof(SearchData));
    
    /** Get basic information. **/
    search_data->Name = check_ptr(nmSysStrdup(inf->Name));
    if (search_data->Name == NULL) goto err_free_search;
    if (!check(objCurrentDate(&search_data->DateCreated))) goto err_free_search;
    
    /** Get source. **/
    char* source_name;
    if (ci_ParseAttribute(inf, "source", DATA_T_STRING, POD(&source_name), node_data->ParamList, true, true) != 0) return NULL;
    for (unsigned int i = 0; i < node_data->nClusterDatas; i++)
	{
	pClusterData cluster_data = node_data->ClusterDatas[i];
	if (strcmp(source_name, cluster_data->Name) == 0)
	    {
	    /** Source found. **/
	    search_data->Source = cluster_data;
	    break;
	    }
	
	/** Note: Subclusters should probably be parsed here, if they were implemented. **/
	}
    
    /** Did we find the requested source? **/
    if (search_data->Source == NULL)
	{
	/** Print error. **/
	mssErrorf(1, "Cluster", "Could not find cluster \"%s\" for search \"%s\".", source_name, search_data->Name);
	
	/** Attempt to give a hint. **/
	char* cluster_names[node_data->nClusterDatas];
	for (unsigned int i = 0; i < node_data->nClusterDatas; i++)
	    cluster_names[i] = node_data->ClusterDatas[i]->Name;
	ci_TryHint(source_name, cluster_names, node_data->nClusterDatas);
	
	/** Fail. **/
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
    
    /** Check for additional data to warn the user about. **/
    for (unsigned int i = 0u; i < inf->nSubInf; i++)
	{
	pStructInf sub_inf = check_ptr(inf->SubInf[i]);
	ASSERTMAGIC(sub_inf, MGK_STRUCTINF);
	char* name = sub_inf->Name;
	
	/** Handle various struct types. **/
	const int struct_type = stStructType(sub_inf);
	switch (struct_type)
	    {
	    case ST_T_ATTRIB:
		{
		/** Valid attribute names. **/
		char* attrs[] = {
		     "source",
		     "threshold",
		     "similarity_measure",
		};
		const unsigned int nattrs = sizeof(attrs) / sizeof(char*);
		
		/** Ignore valid attribute names. **/
		bool is_valid = false;
		for (unsigned int i = 0u; i < nattrs; i++)
		    {
		    if (strcmp(name, attrs[i]) == 0)
			{
			is_valid = true;
			break;
			}
		    }
		if (is_valid) continue; /* Next inf. */
		
		/** Give the user a warning, and attempt to give them a hint. **/
		fprintf(stderr, "Warning: Unknown attribute '%s' in search \"%s\".\n", name, inf->Name);
		ci_TryHint(name, attrs, nattrs);
		
		break;
		}
	    
	    case ST_T_SUBGROUP:
		{
		/** The spec does not specify any valid sub-groups for searches. **/
		char* group_type = check_ptr(sub_inf->UsrType);
		if (group_type == NULL) goto err_free_search;
		fprintf(stderr,
		    "Warning: Unknown group \"%s\" : \"%s\" in search \"%s\".\n",
		    name, group_type, inf->Name
		);
		break;
		}
	    
	    default:
		{
		mssErrorf(1, "Cluster",
		    "Warning: Unknown struct type %d in search \"%s\".",
		    struct_type, inf->Name
		);
		goto err_free_search;
		}
	    }
	}
    
    /** Create cache entry key. **/
    char* source_key = search_data->Source->Key;
    const size_t len = strlen(source_key) + strlen(search_data->Name) + 16lu;
    char* key = check_ptr(nmSysMalloc(len * sizeof(char)));
    if (key == NULL) goto err_free_search;
    snprintf(key, len, "%s/%s?%g&%u",
	source_key,
	search_data->Name,
	search_data->Threshold,
	search_data->SimilarityMeasure
    );
    pXHashTable search_cache = &ClusterDriverCaches.SearchDataCache;
    
    /** Check for a cached version. **/
    pSearchData search_maybe = (pSearchData)xhLookup(search_cache, key);
    if (search_maybe != NULL)
	{
	/** Cache hit. **/
	tprintf("# search: \"%s\"\n", key);
	tprintf("--> Name: %s\n", search_maybe->Name); /* Cause invalid read if cache was incorrectly freed. */
	
	/** Free the parsed search that we no longer need. **/
	ci_FreeSearchData(search_data);
	nmSysFree(key);
	
	/** Return the cached search. **/
	return search_maybe;
	}
    
    /** Cache miss. **/
    tprintf("+ search: \"%s\"\n", key);
    check(xhAdd(search_cache, key, (void*)search_data));
    return search_data;
    
    /** Error cleanup. **/
    err_free_search:
    ci_FreeSearchData(search_data);
    
    err:
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
 *** @param parent The parent object struct.
 *** @returns A new pNodeData struct on success, or NULL on failure.
 ***/
static pNodeData ci_ParseNodeData(pStructInf inf, pObject parent)
    {
    int ret;
    char* path = check_ptr(ci_file_path(parent));
    if (path == NULL) goto err;
    
    /** Allocate node struct data. **/
    pNodeData node_data = check_ptr(nmMalloc(sizeof(NodeData)));
    if (node_data == NULL) goto err;
    memset(node_data, 0, sizeof(NodeData));
    node_data->Parent = parent;
    
    /** Set up param list. **/
    node_data->ParamList = check_ptr(expCreateParamList());
    if (node_data->ParamList == NULL) goto err;
    node_data->ParamList->Session = check_ptr(parent->Session);
    if (node_data->ParamList->Session == NULL) goto err;
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
    memset(&param_infs, 0, sizeof(XArray));
    memset(&cluster_infs, 0, sizeof(XArray));
    memset(&search_infs, 0, sizeof(XArray));
    if (!check(xaInit(&param_infs, 8))) goto err_free_arrs;
    if (!check(xaInit(&cluster_infs, 8))) goto err_free_arrs;
    if (!check(xaInit(&search_infs, 8))) goto err_free_arrs;
    for (unsigned int i = 0u; i < inf->nSubInf; i++)
	{
	pStructInf sub_inf = check_ptr(inf->SubInf[i]);
	ASSERTMAGIC(sub_inf, MGK_STRUCTINF);
	char* name = sub_inf->Name;
	
	/** Handle various struct types. **/
	const int struct_type = stStructType(sub_inf);
	switch (struct_type)
	    {
	    case ST_T_ATTRIB:
		{
		/** Valid attribute names. **/
		char* attrs[] = {
		     "source",
		     "attr_name",
		};
		const unsigned int nattrs = sizeof(attrs) / sizeof(char*);
		
		/** Ignore valid attribute names. **/
		bool is_valid = false;
		for (unsigned int i = 0u; i < nattrs; i++)
		    {
		    if (strcmp(name, attrs[i]) == 0)
			{
			is_valid = true;
			break;
			}
		    }
		if (is_valid) continue; /* Next inf. */
		
		/** Give the user a warning, and attempt to give them a hint. **/
		fprintf(stderr, "Warning: Unknown attribute '%s' in cluster node \"%s\".\n", name, inf->Name);
		ci_TryHint(name, attrs, nattrs);
		
		break;
		}
	    
	    case ST_T_SUBGROUP:
		{
		/** The spec does not specify any valid sub-groups for searches. **/
		char* group_type = check_ptr(sub_inf->UsrType);
		if (group_type == NULL) goto err_free_arrs;
		if (strcmp(group_type, "cluster/parameter") == 0)
		    {
		    if (!check_neg(xaAddItem(&param_infs, sub_inf)))
			goto err_free_arrs;
		    }
		else if (strcmp(group_type, "cluster/cluster") == 0)
		    {
		    if (!check_neg(xaAddItem(&cluster_infs, sub_inf)))
			goto err_free_arrs;
		    }
		else if (strcmp(group_type, "cluster/search") == 0)
		    {
		    if (!check_neg(xaAddItem(&search_infs, sub_inf)))
			goto err_free_arrs;
		    }
		else
		    {
		    /** Give the user a warning, and attempt to give them a hint. **/
		    fprintf(stderr,
			"Warning: Unknown group type \"%s\" on group \"%s\".\n",
			group_type, sub_inf->Name
		    );
		    ci_TryHint(group_type, (char*[]){
			"cluster/parameter",
			"cluster/cluster",
			"cluster/search",
			NULL,
		    }, 0u);
		    }
		break;
		}
	    
	    default:
		{
		mssErrorf(1, "Cluster",
		    "Warning: Unknown struct type %d in search \"%s\".",
		    struct_type, inf->Name
		);
		goto err_free_arrs;
		}
	    }
	}
    
    /** Extract OpenCtl for use below. **/
    bool has_provided_params = parent != NULL
	&& parent->Pathname != NULL
	&& parent->Pathname->OpenCtl != NULL
	&& parent->Pathname->OpenCtl[parent->SubPtr - 1] != NULL
	&& parent->Pathname->OpenCtl[parent->SubPtr - 1]->nSubInf > 0
	&& parent->Pathname->OpenCtl[parent->SubPtr - 1]->SubInf != NULL;
    int num_provided_params = (has_provided_params) ? parent->Pathname->OpenCtl[parent->SubPtr - 1]->nSubInf : 0;
    pStruct* provided_params = (has_provided_params) ? parent->Pathname->OpenCtl[parent->SubPtr - 1]->SubInf : NULL;
    
    /** Itterate over each param in the structure file. **/
    node_data->nParams = param_infs.nItems;
    const size_t params_size = node_data->nParams * sizeof(pParam);
    node_data->Params = check_ptr(nmSysMalloc(params_size));
    if (node_data->Params == NULL) goto err_free_arrs;
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
		    parent->SubPtr - 1, j
		);
		goto err_free_arrs;
		}
	    
	    /** If this provided param value isn't for the param, ignore it. **/
	    if (strcmp(provided_param->Name, param->Name) != 0) continue;
	    
	    /** Matched! The user is providing a value for this param. **/
	    ret = paramSetValueFromInfNe(param, provided_param, 0, node_data->ParamList, node_data->ParamList->Session);
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
	ret = paramEvalHints(param, node_data->ParamList, node_data->ParamList->Session);
	if (ret != 0)
	    {
	    mssErrorf(0, "Cluster",
		"Failed to evaluate parameter hints for parameter \"%s\" (error code %d).",
		param->Name, ret
	    );
	    goto err_free_arrs;
	    }
	}
    check(xaDeInit(&param_infs)); /* Failure ignored. */
    param_infs.nAlloc = 0;
    
    /** Parse source data. **/
    node_data->SourceData = ci_ParseSourceData(inf, node_data->ParamList, path);
    if (node_data->SourceData == NULL) goto err_free_arrs;
    
    /** Parse each cluster. **/
    node_data->nClusterDatas = cluster_infs.nItems;
    if (node_data->nClusterDatas > 0)
	{
	const size_t clusters_size = node_data->nClusterDatas * sizeof(pClusterData);
	node_data->ClusterDatas = check_ptr(nmSysMalloc(clusters_size));
	if (node_data->ClusterDatas == NULL) goto err_free_arrs;
	memset(node_data->ClusterDatas, 0, clusters_size);
	for (unsigned int i = 0u; i < node_data->nClusterDatas; i++)
	    {
	    node_data->ClusterDatas[i] = ci_ParseClusterData(cluster_infs.Items[i], node_data);
	    if (node_data->ClusterDatas[i] == NULL) goto err_free_arrs;
	    }
	}
    else node_data->ClusterDatas = NULL;
    check(xaDeInit(&cluster_infs)); /* Failure ignored. */
    cluster_infs.nAlloc = 0;
    
    /** Parse each search. **/
    node_data->nSearchDatas = search_infs.nItems;
    if (node_data->nSearchDatas > 0)
	{
	const size_t searches_size = node_data->nSearchDatas * sizeof(pSearchData);
	node_data->SearchDatas = check_ptr(nmSysMalloc(searches_size));
	if (node_data->SearchDatas == NULL) goto err_free_arrs;
	memset(node_data->SearchDatas, 0, searches_size);
	for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
	    {
	    node_data->SearchDatas[i] = ci_ParseSearchData(search_infs.Items[i], node_data);
	    if (node_data->SearchDatas[i] == NULL) goto err_free_arrs;
	    }
	}
    else node_data->SearchDatas = NULL;
    check(xaDeInit(&search_infs)); /* Failure ignored. */
    search_infs.nAlloc = 0;
    
    /** Success. **/
    return node_data;
    
    err_free_arrs:
    if (param_infs.nAlloc   != 0) check(xaDeInit(&param_infs));   /* Failure ignored. */
    if (cluster_infs.nAlloc != 0) check(xaDeInit(&cluster_infs)); /* Failure ignored. */
    if (search_infs.nAlloc  != 0) check(xaDeInit(&search_infs));  /* Failure ignored. */
    
    err_free_node:
    ci_FreeNodeData(node_data);
    
    err:
    mssErrorf(0, "Cluster", "Failed to parse node from group \"%s\" in file: %s", inf->Name, path);
    return NULL;
    }


/** ================ Freeing Functions ================ **/
/** ANCHOR[id=freeing] **/
// LINK #functions

/** @param source_data A pSourceData struct, freed by this function. **/
static void ci_FreeSourceData(pSourceData source_data)
    {
    /** Guard segfault. **/
    if (source_data == NULL)
	{
	fprintf(stderr, "Call to ci_FreeSourceData(NULL);\n");
	return;
	}
    
    /** Free top level attributes, if they exist. **/
    if (source_data->Name != NULL)
	{
	nmSysFree(source_data->Name);
	source_data->Name = NULL;
	}
    if (source_data->SourcePath != NULL)
	{
	nmSysFree(source_data->SourcePath);
	source_data->SourcePath = NULL;
	}
    if (source_data->AttrName != NULL)
	{
	nmSysFree(source_data->AttrName);
	source_data->AttrName = NULL;
	}
    
    /** Free fetched data, if it exists. **/
    if (source_data->Strings != NULL)
	{
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    {
	    nmSysFree(source_data->Strings[i]);
	    source_data->Strings[i] = NULL;
	    }
	nmSysFree(source_data->Strings);
	source_data->Strings = NULL;
	}
    
    /** Free computed vectors, if they exist. **/
    if (source_data->Vectors != NULL)
	{
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    {
	    ca_free_vector(source_data->Vectors[i]);
	    source_data->Vectors[i] = NULL;
	    }
	nmSysFree(source_data->Vectors);
	source_data->Vectors = NULL;
	}
    
    /** Free the source data struct. **/
    nmFree(source_data, sizeof(SourceData));
    source_data = NULL;
    }


// LINK #functions
/*** Free pClusterData struct with an option to recursively free subclusters.
 *** 
 *** @param cluster_data The cluster data struct to free.
 *** @param recrusive Whether to recursively free subclusters.
 ***/
static void ci_FreeClusterData(pClusterData cluster_data, bool recursive)
    {
    /** Guard segfault. **/
    if (cluster_data == NULL)
	{
	fprintf(stderr, "Call to ci_FreeClusterData(NULL, %s);\n", (recursive) ? "true" : "false");
	return;
	}
    
    /** Free attribute data. **/
    if (cluster_data->Name != NULL)
	{
	nmSysFree(cluster_data->Name);
	cluster_data->Name = NULL;
	}
    
    /** Free computed data, if it exists. **/
    if (cluster_data->Clusters != NULL)
	{
	for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
	    {
	    pCluster cluster = &cluster_data->Clusters[i];
	    nmSysFree(cluster->Strings);
	    nmSysFree(cluster->Vectors);
	    cluster->Strings = NULL;
	    cluster->Vectors = NULL;
	    }
	nmSysFree(cluster_data->Clusters);
	nmSysFree(cluster_data->Sims);
	cluster_data->Clusters = NULL;
	cluster_data->Sims = NULL;
	}
    
    /** Free subclusters recursively. **/
    if (cluster_data->SubClusters != NULL)
	{
	if (recursive)
	    {
	    for (unsigned int i = 0u; i < cluster_data->nSubClusters; i++)
		{
		ci_FreeClusterData(cluster_data->SubClusters[i], recursive);
		cluster_data->SubClusters[i] = NULL;
		}
	    }
	nmSysFree(cluster_data->SubClusters);
	cluster_data->SubClusters = NULL;
	}
    
    /** Free the cluster data struct. **/
    nmFree(cluster_data, sizeof(ClusterData));
    cluster_data = NULL;
    }


// LINK #functions
/** @param search_data A pSearchData struct, freed by this function. **/
static void ci_FreeSearchData(pSearchData search_data)
    {
    /** Guard segfault. **/
    if (search_data == NULL)
	{
	fprintf(stderr, "Call to ci_FreeSearchData(NULL);\n");
	return;
	}
    
    /** Free attribute data. **/
    if (search_data->Name != NULL)
	{
	nmSysFree(search_data->Name);
	search_data->Name = NULL;
	}
    
    /** Free computed data. **/
    if (search_data->Dups != NULL)
	{
	for (unsigned int i = 0; i < search_data->nDups; i++)
	    {
	    nmFree(search_data->Dups[i], sizeof(Dup));
	    search_data->Dups[i] = NULL;
	    }
	nmSysFree(search_data->Dups);
	search_data->Dups = NULL;
	}
    
    /** Free the search data struct. **/
    nmFree(search_data, sizeof(SearchData));
    search_data = NULL;
    }


// LINK #functions
/** @param node_data A pNodeData struct, freed by this function. **/
static void ci_FreeNodeData(pNodeData node_data)
    {
    /** Guard segfault. **/
    if (node_data == NULL)
	{
	fprintf(stderr, "Call to ci_FreeNodeData(NULL);\n");
	return;
	}
    
    /** Free parsed params, if they exist. **/
    if (node_data->Params != NULL)
	{
	for (unsigned int i = 0u; i < node_data->nParams; i++)
	    {
	    if (node_data->Params[i] == NULL) break;
	    paramFree(node_data->Params[i]);
	    node_data->Params[i] = NULL;
	    }
	nmSysFree(node_data->Params);
	node_data->Params = NULL;
        }
    if (node_data->ParamList != NULL)
	{
	expFreeParamList(node_data->ParamList);
	node_data->ParamList = NULL;
	}
    
    /** Free parsed clusters, if they exist. **/
    if (node_data->ClusterDatas != NULL)
	{
	/*** This data is cached, so we should NOT free it! The caching system
	 *** is responsible for the memory. We only need to free the array
	 *** holding our pointers to said cached memory.
	 ***/
	nmSysFree(node_data->ClusterDatas);
	node_data->ClusterDatas = NULL;
	}
    
    /** Free parsed searches, if they exist. **/
    if (node_data->SearchDatas != NULL)
	{
	/*** This data is cached, so we should NOT free it! The caching system
	 *** is responsible for the memory. We only need to free the array
	 *** holding our pointers to said cached memory.
	 ***/
	nmSysFree(node_data->SearchDatas);
	node_data->SearchDatas = NULL;
	}
	
    /** Free data source, if one exists. **/
    /*** Note: SourceData is freed last since other free functions may need to
     ***       access information from this structure when freeing data.
     ***       (For example, nVector which is used to determine the size of the
     ***        label struct in each cluster.)
     ***/
    if (node_data->SourceData != NULL)
	{
	/*** This data is cached, so we should NOT free it! The caching system
	 *** is responsible for the memory. We only need to free the array
	 *** holding our pointers to said cached memory.
	 ***/
	node_data->SourceData = NULL;
	}
    
    /** Free the node data. **/
    nmFree(node_data, sizeof(NodeData));
    node_data = NULL;
    }

/** Frees all data in caches for all cluster driver instances. **/
static void ci_ClearCaches(void)
    {
    /*** Free caches in reverse of the order they are created in case
     *** cached data relies on its source during the freeing process.
     ***/
    check(xhClearKeySafe(&ClusterDriverCaches.SearchDataCache, ci_CacheFreeSearch, NULL)); /* Failure ignored. */
    check(xhClearKeySafe(&ClusterDriverCaches.ClusterDataCache, ci_CacheFreeCluster, NULL)); /* Failure ignored. */
    check(xhClearKeySafe(&ClusterDriverCaches.SourceDataCache, ci_CacheFreeSourceData, NULL)); /* Failure ignored. */
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
    if (source_data->Strings != NULL)
	{
	for (unsigned int i = 0u; i < source_data->nVectors; i++)
	    size += strlen(source_data->Strings[i]) * sizeof(char);
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
    if (cluster_data->Clusters != NULL)
	{
	const unsigned int nVectors = cluster_data->SourceData->nVectors;
	for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
	    {
	    const unsigned int cluster_size = cluster_data->Clusters[i].Size;
	    size += cluster_size * sizeof(char*);
	    size += cluster_size * sizeof(pVector);
	    }
	size += nVectors * sizeof(Cluster);
	size += nVectors * sizeof(double);
	}
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
    
    /** Time to play shoots-and-ladders in an error-handling jungle of gotos. **/
    bool successful = false;
    int ret;
    
    /** Record the date and time. **/
    if (!check(objCurrentDate(&source_data->DateComputed))) goto end;
    
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
	goto end_close;
	}
    
    /** Initialize an xarray to store the retrieved data. **/
    XArray data_xarray, vector_xarray;
    memset(&data_xarray, 0, sizeof(XArray));
    memset(&vector_xarray, 0, sizeof(XArray));
    if (!check(xaInit(&data_xarray, 64))) goto end_close_query;
    if (!check(xaInit(&vector_xarray, 64))) goto end_free_data;
    
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
	    check(fflush(stdout)); /* Failure ignored. */
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
	if (ca_is_empty(vector))
	    {
	    mssErrorf(1, "Cluster", "Vector building for string \"%s\" produced no character pairs.", val);
	    successful = false;
	    goto end_free_data;
	    }
	if (ca_has_no_pairs(vector))
	    {
	    /** Skip pVector with no pairs. **/
	    tprintf(".");
	    check(fflush(stdout)); /* Failure ignored. */
	    ca_free_vector(vector);
	    continue;
	    }
	
	/** Store value. **/
	char* dup_val = check_ptr(nmSysStrdup(val));
	if (dup_val == NULL) goto end_free_data;
	if (!check_neg(xaAddItem(&data_xarray, (void*)dup_val))) goto end_free_data;
	if (!check_neg(xaAddItem(&vector_xarray, (void*)vector))) goto end_free_data;
	
	/** Clean up. **/
	ret = objClose(entry);
	if (ret != 0)
	    {
	    mssErrorf(0, "Cluster", "Failed to close object entry (error code %d).", ret);
	    // ret = ret; // Fall-through: Failure ignored.
	    }
	}
    tprintf("\nData aquired.\n");
    source_data->nVectors = vector_xarray.nItems;
    
    /** Trim data and store data. **/
    const size_t data_size = source_data->nVectors * sizeof(char*);
    source_data->Strings = check_ptr(nmSysMalloc(data_size));
    if (source_data->Strings == NULL) goto end_free_data;
    memcpy(source_data->Strings, data_xarray.Items, data_size);
    check(xaDeInit(&data_xarray)); /* Failure ignored. */
    data_xarray.nAlloc = 0;
    
    /** Trim data and store vectors. **/
    const size_t vectors_size = source_data->nVectors * sizeof(pVector);
    source_data->Vectors = check_ptr(nmSysMalloc(vectors_size));
    memcpy(source_data->Vectors, vector_xarray.Items, vectors_size);
    check(xaDeInit(&vector_xarray)); /* Failure ignored. */
    vector_xarray.nAlloc = 0;
    
    /** Success. **/
    successful = true;
    
    end_free_data:
    if (data_xarray.nAlloc != 0)
	{
	for (unsigned int i = 0u; i < data_xarray.nItems; i++)
	    nmSysFree(data_xarray.Items[i]);
	check(xaDeInit(&data_xarray)); /* Failure ignored. */
	}
    if (vector_xarray.nAlloc != 0)
	{
	for (unsigned int i = 0u; i < vector_xarray.nItems; i++)
	    ca_free_vector(vector_xarray.Items[i]);
	check(xaDeInit(&vector_xarray)); /* Failure ignored. */
	}
    
    end_close_query:
    ret = objQueryClose(query);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Failed to close query (error code %d).", ret);
	// ret = ret; // Fall-through: Failure ignored.
	}
    
    end_close:
    ret = objClose(obj);
    if (ret != 0)
	{
	mssErrorf(0, "Cluster", "Failed to close object driver (error code %d).", ret);
	// ret = ret; // Fall-through: Failure ignored.
	}
    
    end:
    if (!successful) mssErrorf(0, "Cluster", "SourceData computation failed.");
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
    if (cluster_data->Clusters != NULL) return 0;
    
    /** Make source data available. **/
    pSourceData source_data = node_data->SourceData;
    
    /** We need the SourceData vectors to compute clusters. **/
    if (ci_ComputeSourceData(source_data, node_data->ParamList->Session) != 0)
	{
	mssErrorf(0, "Cluster", "Failed to compute SourceData.");
	goto err;
	}
    
    /** Record the date and time. **/
    if (!check(objCurrentDate(&cluster_data->DateComputed))) goto err;
    
    /** Allocate static memory for finding clusters. **/
    const size_t clusters_size = cluster_data->nClusters * sizeof(Cluster);
    cluster_data->Clusters = check_ptr(nmSysMalloc(clusters_size));
    if (cluster_data->Clusters == NULL) goto err;
    memset(cluster_data->Clusters, 0, clusters_size);
    const size_t sims_size = source_data->nVectors * sizeof(double);
    cluster_data->Sims = check_ptr(nmSysMalloc(sims_size));
    if (cluster_data->Sims == NULL) goto err_free_clusters;
    memset(cluster_data->Sims, 0, sims_size);
    
    /** Execute clustering. **/
    switch (cluster_data->ClusterAlgorithm)
	{
	case ALGORITHM_NONE:
	    {
	    tprintf("Applying no clustering...\n");
	    /** Put all the data into one cluster. **/
	    pCluster first_cluster = &cluster_data->Clusters[0];
	    first_cluster->Size = source_data->nVectors;
	    first_cluster->Strings = check_ptr(nmSysMalloc(source_data->nVectors * sizeof(char*)));
	    if (first_cluster->Strings == NULL) goto err_free_sims;
	    first_cluster->Vectors = check_ptr(nmSysMalloc(source_data->nVectors * sizeof(pVector)));
	    if (first_cluster->Vectors == NULL) goto err_free_sims;
	    memcpy(first_cluster->Strings, source_data->Strings, source_data->nVectors * sizeof(char*));
	    memcpy(first_cluster->Vectors, source_data->Vectors, source_data->nVectors * sizeof(pVector));
	    break;
	    }
	
	case ALGORITHM_SLIDING_WINDOW:
	    /** Computed in each search for efficiency. **/
	    tprintf("Skipping sliding window clustering...\n");
	    memset(cluster_data->Clusters, 0, clusters_size);
	    break;
	
	case ALGORITHM_KMEANS:
	    {
	    tprintf("Applying kmeans clustering...\n");
	    /** Check for unimplemented similarity measures. **/
	    if (cluster_data->SimilarityMeasure != SIMILARITY_COSINE)
		{
		mssErrorf(1, "Cluster",
		    "The similarity meausre \"%s\" is not implemented.",
		    ci_SimilarityMeasureToString(cluster_data->SimilarityMeasure)
		);
		goto err_free_sims;
		}
	    
	    /** Allocate lables. Note: kmeans does not require us to initialize them. **/
	    const size_t lables_size = source_data->nVectors * sizeof(unsigned int);
	    unsigned int* labels = check_ptr(nmSysMalloc(lables_size));
	    if (labels == NULL) goto err_free_sims;
	    
	    /** Run kmeans. **/
	    tprintf("Running kmeans\n");
	    Timer timer_i, *timer = timer_start(timer_init(&timer_i));
	    const bool successful = check(ca_kmeans(
		source_data->Vectors,
		source_data->nVectors,
		cluster_data->nClusters,
		cluster_data->MaxIterations,
		cluster_data->MinImprovement,
		labels,
		cluster_data->Sims
	    ));
	    timer_stop(timer);
	    tprintf("Clustering done after %.4lf.\n", timer_get(timer));
	    if (!successful) goto err_free_sims;
	    
	    /** Convert the labels into clusters. **/
	    
	    /** Allocate space for clusters. **/
	    XArray indexes_in_cluster[cluster_data->nClusters];
	    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		if (!check(xaInit(&indexes_in_cluster[i], 8))) goto err_free_sims;
	    
	    /** Iterate through each label and add the index of the specified cluster to the xArray. **/
	    for (unsigned long long i = 0llu; i < source_data->nVectors; i++)
		if (!check_neg(xaAddItem(&indexes_in_cluster[labels[i]], (void*)i))) goto err_free_sims;
	    nmFree(labels, lables_size); /* Free unused data. */
	    
	    /** Iterate through each cluster, store it, and free the xArray. **/
	    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		{
		pXArray indexes_in_this_cluster = &indexes_in_cluster[i];
		pCluster cluster = &cluster_data->Clusters[i];
		cluster->Size = indexes_in_this_cluster->nItems;
		cluster->Strings = check_ptr(nmSysMalloc(cluster->Size * sizeof(char*)));
		if (cluster->Strings == NULL) goto err_free_sims;
		cluster->Vectors = check_ptr(nmSysMalloc(cluster->Size * sizeof(pVector)));
		if (cluster->Vectors == NULL) goto err_free_sims;
		for (unsigned int j = 0u; j < cluster->Size; j++)
		    {
		    const unsigned long long index = (unsigned long long)indexes_in_this_cluster->Items[j];
		    cluster->Strings[j] = source_data->Strings[index];
		    cluster->Vectors[j] = source_data->Vectors[index];
		    }
		check(xaDeInit(indexes_in_this_cluster)); /* Failure ignored. */
		}
	    
	    /** k-means is done. **/
	    break;
	    }
	
	default:
	    mssErrorf(1, "Cluster",
		"Clustering algorithm \"%s\" is not implemented.",
		ci_ClusteringAlgorithmToString(cluster_data->ClusterAlgorithm)
	    );
	    goto err;
	}
    
    /** Success. **/
    tprintf("Clustering done.\n");
    return 0;
    
    err_free_sims:
    nmFree(cluster_data->Sims, sims_size);
    cluster_data->Sims = NULL;
    
    err_free_clusters:
    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
	{
	pCluster cluster = &cluster_data->Clusters[i];
	if (cluster->Strings != NULL) nmFree(cluster->Strings, cluster->Size * sizeof(char*));
	else break;
	if (cluster->Vectors != NULL) nmFree(cluster->Vectors, cluster->Size * sizeof(pVector));
	else break;
	}
    nmFree(cluster_data->Clusters, clusters_size);
    cluster_data->Clusters = NULL;
    
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
    
    /** We need the cluster data to be computed before we search it. **/
    pClusterData cluster_data = search_data->Source;
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
    if (!check(objCurrentDate(&search_data->DateComputed))) goto err;
    
    tprintf("Invoking search.\n");
    Timer timer_i, *timer = timer_start(timer_init(&timer_i));
    /** Execute the search using the specified source and comparison function. **/
    pXArray dups = NULL, dups_temp = NULL;
    switch (search_data->SimilarityMeasure)
	{
	case SIMILARITY_COSINE:
	    {
	    if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
		{
		dups_temp = check_ptr(ca_sliding_search(
		    (void**)cluster_data->SourceData->Vectors,
		    cluster_data->SourceData->nVectors,
		    cluster_data->MaxIterations, /* Window size. */
		    ca_cos_compare,
		    search_data->Threshold,
		    dups
		));
		}
	    else
		{
		for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		    {
		    dups_temp = check_ptr(ca_complete_search(
			(void**)cluster_data->Clusters[i].Vectors,
			cluster_data->Clusters[i].Size,
			ca_cos_compare,
			search_data->Threshold,
			dups
		    ));
		    if (dups_temp == NULL) goto err;
		    else dups = dups_temp;
		    }
		}
	    break;
	    }
	
	case SIMILARITY_LEVENSHTEIN:
	    {
	    if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
		{
		dups_temp = check_ptr(ca_sliding_search(
		    (void**)cluster_data->SourceData->Vectors,
		    cluster_data->SourceData->nVectors,
		    cluster_data->MaxIterations, /* Window size. */
		    ca_lev_compare,
		    search_data->Threshold,
		    dups
		));
		}
	    else
		{
		for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		    {
		    dups_temp = check_ptr(ca_complete_search(
			(void**)cluster_data->Clusters[i].Strings,
			cluster_data->Clusters[i].Size,
			ca_lev_compare,
			search_data->Threshold,
			dups
		    ));
		    if (dups_temp == NULL) goto err;
		    else dups = dups_temp;
		    }
		}
	    break;
	    }
	
	default:
	     mssErrorf(1, "Cluster",
		"Unknown similarity meansure \"%s\".",
		ci_SimilarityMeasureToString(search_data->SimilarityMeasure)
	    );
	    goto err;
	}
    timer_stop(timer);
    if (dups_temp == NULL) goto err;
    else dups = dups_temp;
    tprintf("Search done after %.4lf.\n", timer_get(timer));
    
    /** Store dups. **/
    search_data->nDups = dups->nItems;
    search_data->Dups = (dups->nItems == 0)
	? check_ptr(nmSysMalloc(0))
	: ci_xaToTrimmedArray(dups);
    
    /** Free unused data. **/
    tprintf("Cleanup.\n");
    check(xaFree(dups)); /* Failure ignored. */
    
    /** Success. **/
    return 0;
    
    err:
    if (dups != NULL)
	{
	for (unsigned int i = 0u; i < dups->nItems; i++)
	    {
	    if (dups->Items[i] != NULL) nmFree(dups->Items[i], sizeof(Dup));
	    else break;
	    }
	check(xaFree(dups)); /* Failure ignored. */
	}
    
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
		
	/** Parameter found. **/
	if (param->Value == NULL) return 1;
	if (param->Value->Flags & DATA_TF_NULL) return 1;
	if (param->Value->DataType != datatype)
	    {
	    mssErrorf(1, "Cluster", "Type mismatch accessing parameter '%s'.", param->Name);
	    return -1;
	    }
	
	/** Return param value. **/
	if (!check(objCopyData(&(param->Value->Data), val, datatype))) goto err;
	return 0;
	}
    
    err:
    mssErrorf(1, "Cluster",
	"Failed to get parameter %s : %s",
	attr_name, ci_TypeToStr(datatype)
    );
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
 *** at the path provided in parent.
 *** 
 *** @param parent The parent of the object to be openned, including useful
 *** 	information such as the pathname, session, etc.
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
void* clusterOpen(pObject parent, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt)
    {
    tprintf("Warning: clusterOpen(\"%s\") is under active development.\n", ci_file_name(parent));
    
    /** If CREAT and EXCL are specified, exclusively create it, failing if the file already exists. **/
    pSnNode node_struct = NULL;
    bool can_create = (parent->Mode & O_CREAT) && (parent->SubPtr == parent->Pathname->nElements);
    if (can_create && (parent->Mode & O_EXCL))
	{
	node_struct = snNewNode(parent->Prev, usr_type);
	if (node_struct == NULL)
	    {
	    mssErrorf(0, "Cluster", "Failed to exclusively create new node struct.");
	    goto err;
	    }
	}
    
    /** Read the node if it exists. **/
    if (node_struct == NULL)
	node_struct = snReadNode(parent->Prev);
    
    /** If we can't read it, create it (if allowed). **/
    if (node_struct == NULL && can_create)
	node_struct = snNewNode(parent->Prev, usr_type);
    
    /** If there still isn't a node, fail early. **/
    if (node_struct == NULL)
	{
	mssErrorf(0, "Cluster", "Failed to create node struct.");
	goto err;
	}
    
    /** Magic. **/
    ASSERTMAGIC(node_struct, MGK_STNODE);
    ASSERTMAGIC(node_struct->Data, MGK_STRUCTINF);
    
    /** Parse node data from the node_struct. **/
    pNodeData node_data = ci_ParseNodeData(node_struct->Data, parent);
    if (node_data == NULL)
	{
	mssErrorf(0, "Cluster", "Failed to parse structure file \"%s\".", ci_file_name(parent));
	goto err;
	}
    
    /** Allocate driver instance data. **/
    pDriverData driver_data = check_ptr(nmMalloc(sizeof(DriverData)));
    if (driver_data == NULL) goto err_free_node;
    memset(driver_data, 0, sizeof(DriverData));
    driver_data->NodeData = node_data;
        
    /** Detect target from path. **/
    tprintf("Parsing node path: %d %d\n", parent->SubPtr, parent->SubCnt); parent->SubCnt = 0;
    char* target_name = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
    if (target_name == NULL)
	{
	/** Target found: Root **/
	tprintf("Found target: Root.\n");
	driver_data->TargetType = TARGET_ROOT;
	driver_data->TargetData = (void*)driver_data->NodeData->SourceData;
	return (void*)driver_data; /* Success. */
	}
    
    /** Search clusters. **/
    for (unsigned int i = 0u; i < node_data->nClusterDatas; i++)
	{
	pClusterData cluster = node_data->ClusterDatas[i];
	if (strcmp(cluster->Name, target_name) != 0) continue;
	
	/** Target found: Cluster **/
	driver_data->TargetType = TARGET_CLUSTER;
	tprintf("Found target cluster: %s\n", cluster->Name);
	
	/** Check for sub-clusters in the path. **/
	while (true)
	    {
	    /** Decend one path part deeper into the path. **/
	    const char* path_part = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
	    
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
	return (void*)driver_data; /* Success. */
	}
    
    /** Search searches. **/
    for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
	{
	pSearchData search = node_data->SearchDatas[i];
	if (strcmp(search->Name, target_name) != 0) continue;
	
	/** Target found: Search **/
	driver_data->TargetType = TARGET_SEARCH;
	driver_data->TargetData = (void*)search;
	
	/** Check for extra, invalid path parts. **/
	char* extra_data = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
	if (extra_data != NULL)
	    {
	    mssErrorf(1, "Cluster", "Unknown path part %s.", extra_data);
	    goto err_free_node;
	    }
	tprintf("Found target search: %s %d %d\n", search->Name, parent->SubPtr, parent->SubCnt);
	return (void*)driver_data; /* Success. */
	}
    
    /** We were unable to find the requested cluster or search. **/
    mssErrorf(1, "Cluster", "\"%s\" is not the name of a declared cluster or search.", target_name);
    
    /** Attempt to give a hint. **/
        {
        const unsigned int n_targets = node_data->nClusterDatas + node_data->nSearchDatas;
        char* target_names[n_targets];
        for (unsigned int i = 0u; i < node_data->nClusterDatas; i++)
	    target_names[i] = node_data->ClusterDatas[i]->Name;
	for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
	    target_names[i + node_data->nClusterDatas] = node_data->SearchDatas[i]->Name;
	ci_TryHint(target_name, target_names, n_targets);
	}
    
    /** Error cleanup. **/
    err_free_node:
    if (node_data != NULL) ci_FreeNodeData(node_data);
    if (driver_data != NULL) nmFree(driver_data, sizeof(DriverData));
    
    err:
    mssErrorf(0, "Cluster",
	"Failed to open cluster file \"%s\" at: %s",
	ci_file_name(parent), ci_file_path(parent)
    );
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
    if (cluster_query == NULL) return NULL;
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
	    data_amount = target->nClusters;
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
    pDriverData driver_data = check_ptr(nmMalloc(sizeof(DriverData)));
    if (driver_data == NULL) return NULL;
    memcpy(driver_data, cluster_query->DriverData, sizeof(DriverData));
    driver_data->TargetType = new_target_type;
    driver_data->TargetIndex = cluster_query->RowIndex++;
    
    /** Success. **/
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
	    if (strcmp(attr_name, "min_improvement") == 0)
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
	    if (strcmp(attr_name, "items") == 0)
		return DATA_T_STRINGVEC;
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
    
    /** Performance shortcut for frequently requested attributes: val1, val2, and sim. **/
    if (
	(attr_name[0] == 'v' && datatype == DATA_T_STRING) /* val1, val2 : String */
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
		if (target->nClusters > INT_MAX)
		    fprintf(stderr, "Warning: 'num_clusters' value of %u exceeds INT_MAX (%d).\n", target->nClusters, INT_MAX);
		val->Integer = (int)target->nClusters;
		return 0;
		}
	    if (strcmp(attr_name, "max_iterations") == 0)
		{
		if (target->MaxIterations > INT_MAX)
		    fprintf(stderr, "Warning: 'max_iterations' value of %u exceeds INT_MAX (%d).\n", target->MaxIterations, INT_MAX);
		val->Integer = (int)target->MaxIterations;
		return 0;
		}
	    if (strcmp(attr_name, "min_improvement") == 0)
		{
		val->Double = target->MinImprovement;
		return 0;
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
	    
	    if (strcmp(attr_name, "items") == 0)
		{
		/** Static variable to prevent leaking StringVec from previous calls. **/
		static StringVec* vec = NULL;
		if (vec != NULL) nmFree(vec, sizeof(StringVec));
		
		/** Allocate and initiallize the requested data. **/
		pCluster target_cluster = &target->Clusters[driver_data->TargetIndex];
		val->StringVec = vec = check_ptr(nmMalloc(sizeof(StringVec)));
		if (val->StringVec == NULL) return -1;
		val->StringVec->nStrings = target_cluster->Size;
		val->StringVec->Strings = target_cluster->Strings;
		
		/** Success. **/
		return 0;
		}
	    break;
	    }
	
	case TARGET_SEARCH_ENTRY:
	    {
	    pSearchData target = (pSearchData)driver_data->TargetData;
	    pDup target_dup = target->Dups[driver_data->TargetIndex];
	    
	    if (strcmp(attr_name, "id1") == 0)
		{
		unsigned int value = target_dup->id1;
		if (value > INT_MAX)
		    fprintf(stderr, "Warning: id1 value of %u exceeds INT_MAX (%d).\n", value, INT_MAX);
		val->Integer = (int)value;
		return 0;
		}
	    if (strcmp(attr_name, "id2") == 0)
		{
		unsigned int value = target_dup->id2;
		if (value > INT_MAX)
		    fprintf(stderr, "Warning: id2 value of %u exceeds INT_MAX (%d).\n", value, INT_MAX);
		val->Integer = (int)value;
		return 0;
		}
	    if (strcmp(attr_name, "val1") == 0)
		{
		val->String = driver_data->NodeData->SourceData->Strings[target_dup->id1];
		// val->Integer = (int)target_dup->id1;
		return 0;
		}
	    if (strcmp(attr_name, "val2") == 0)
		{
		val->String = driver_data->NodeData->SourceData->Strings[target_dup->id2];
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
    if (hints == NULL) goto err;
    memset(hints, 0, sizeof(ObjPresentationHints));
    
    /** Hints that are the same for all fields **/
    hints->GroupID = -1;
    hints->VisualLength2 = 1;
    hints->Style     |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
    hints->StyleMask |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
    
    /** Temporary param list for compiling expressions. **/
    pParamObjects tmp_list = check_ptr(expCreateParamList());
    if (hints == NULL) goto err;
    
    /** Search for the requested attribute through attributes common to all instances. **/
    if (strcmp(attr_name, "name") == 0)
        {
	hints->Length = 32;
	hints->VisualLength = 16;
	goto success;
	}
    if (strcmp(attr_name, "annotation") == 0)
        {
	hints->Length = 36;
	hints->VisualLength = 36;
	goto success;
	}
    if (strcmp(attr_name, "inner_type") == 0
	|| strcmp(attr_name, "inner_type") == 0
	|| strcmp(attr_name, "outer_type") == 0
	|| strcmp(attr_name, "content_type") == 0
	|| strcmp(attr_name, "last_modification") == 0)
        {
	hints->VisualLength = 30;
	goto success;
        }
    
    /** Handle date created and date computed. */
    if (strcmp(attr_name, "date_created") == 0
	|| strcmp(attr_name, "date_computed") == 0)
	{
	if (driver_data->TargetType == TARGET_CLUSTER || driver_data->TargetType == TARGET_SEARCH)
	    {
	    hints->Length = 24;
	    hints->VisualLength = 20;
	    hints->Format = nmSysStrdup("datetime");
	    goto success;
	    }
	else goto unknown_attribute;
	}
    
    /** Search by target type. **/
    switch (driver_data->TargetType)
	{
	case TARGET_ROOT:
	    if (strcmp(attr_name, "source") == 0)
		{
		hints->Length = _PC_PATH_MAX;
		hints->VisualLength = 64;
		hints->FriendlyName = "Source Path";
		goto success;
		}
	    if (strcmp(attr_name, "attr_name") == 0)
		{
		hints->Length = 255;
		hints->VisualLength = 32;
		hints->FriendlyName = "Attribute Name";
		goto success;
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
		goto success;
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
		goto success;
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
		goto success;
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
		goto success;
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
		goto success;
		}
	    
	    /** End of overlapping region. **/
	    if (driver_data->TargetType == TARGET_CLUSTER) break;
	    
	    if (strcmp(attr_name, "source") == 0)
		{
		hints->Length = 64;
		hints->VisualLength = 32;
		hints->FriendlyName = nmSysStrdup("Source Cluster Name");
		goto success;
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
		goto success;
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
		goto success;
		}
	    if (strcmp(attr_name, "val") == 0)
		{
		/** Other hints. **/
		hints->Length = 255;
		hints->VisualLength = 32;
		hints->FriendlyName = nmSysStrdup("Value");
		goto success;
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
		goto success;
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
		goto success;
		}
	    if (strcmp(attr_name, "val1") == 0 || strcmp(attr_name, "val2") == 0)
		{
		/** Other hints. **/
		hints->Length = 255;
		hints->VisualLength = 32;
		hints->FriendlyName = nmSysStrdup("Value");
		goto success;
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
		goto success;
		}
	    break;
	    }
	
	default:
	    mssErrorf(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
	    goto err;
	}
    
    /** Unknown attribute. **/
    unknown_attribute:;
    char* name;
    clusterGetAttrValue(inf_v, "name", DATA_T_STRING, POD(&name), NULL);
    mssErrorf(1, "Cluster",
	"Unknown attribute '%s' for cluster object %s (target type: %u, \"%s\").",
	attr_name, driver_data->NodeData->SourceData->Name, driver_data->TargetType, name
    );
    
    /** Error cleanup. **/
    err:
    if (tmp_list != NULL) check(expFreeParamList(tmp_list)); /* Failure ignored. */
    if (hints != NULL) nmFree(hints, sizeof(ObjPresentationHints));
    mssErrorf(0, "Cluster", "Failed execute generate presentation hints.");
    return NULL;
    
    /** Success. **/
    success:
    check(expFreeParamList(tmp_list)); /* Failure ignored. */
    return hints;
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
	case TARGET_ROOT:          return ATTR_ROOT[i];
	case TARGET_CLUSTER:       return ATTR_CLUSTER[i];
	case TARGET_SEARCH:        return ATTR_SEARCH[i];
	case TARGET_CLUSTER_ENTRY: return ATTR_CLUSTER_ENTRY[i];
	case TARGET_SEARCH_ENTRY:  return ATTR_SEARCH_ENTRY[i];
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
	    info->nSubobjects = node_data->nClusterDatas + node_data->nSearchDatas;
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
    tprintf("Warning: clusterGetNextMethod() is under active development.");
    pDriverData driver_data = (pDriverData)inf_v;
    return METHOD_NAME[driver_data->TargetMethodIndex++];
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
	default:
	    mssErrorf(0, "Cluster", "Unknown type_id %u.", *type_id_ptr);
	    return -1;
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
	    path = ci_file_path(driver_data->NodeData->Parent);
	    }
	if (strcmp(param->String, "show_all") == 0) show = true;
	
	if (show)
	    {
	    /** Print cache info table. **/
	    int ret = 0;
	    unsigned int i = 1u, source_bytes = 0u, cluster_bytes = 0u, search_bytes = 0u;
	    bool failed = false;
	    printf("\nShowing cache for ");
	    if (path != NULL) printf("\"%s\":\n", path);
	    else printf("all files:\n");
	    printf("%-8s %-16s %-12s %s\n", "Type", "Name", "Size", "Cache Entry Key");
	    failed |= !check(xhForEach(
		&ClusterDriverCaches.SourceDataCache,
		ci_PrintEntry,
		(void*[]){&i, &source_bytes, path}
	    ));
	    i++;
	    failed |= !check(xhForEach(
		&ClusterDriverCaches.ClusterDataCache,
		ci_PrintEntry,
		(void*[]){&i, &cluster_bytes, path}
	    ));
	    i++;
	    failed |= !check(xhForEach(
		&ClusterDriverCaches.SearchDataCache,
		ci_PrintEntry,
		(void*[]){&i, &search_bytes, path}
	    ));
	    if (failed)
		{
		mssErrorf(0, "Cluster", "Unexpected error occured while showhing caches.");
		ret = -1;
		}
	    
	    /** Print stats. **/
	    char buf[16];
	    printf("\nCache Stats:\n");
	    printf("%-8s %-4s %-12s\n", "", "#", "Total Size");
	    printf("%-8s %-4d %-12s\n", "Source", ClusterDriverCaches.SourceDataCache.nItems, snprint_bytes(buf, sizeof(buf), source_bytes));
	    printf("%-8s %-4d %-12s\n", "Cluster", ClusterDriverCaches.ClusterDataCache.nItems, snprint_bytes(buf, sizeof(buf), cluster_bytes));
	    printf("%-8s %-4d %-12s\n", "Search", ClusterDriverCaches.SearchDataCache.nItems, snprint_bytes(buf, sizeof(buf), search_bytes));
	    printf("%-8s %-4d %-12s\n\n", "Total",
		ClusterDriverCaches.SourceDataCache.nItems + ClusterDriverCaches.ClusterDataCache.nItems + ClusterDriverCaches.SearchDataCache.nItems,
		snprint_bytes(buf, sizeof(buf), source_bytes + cluster_bytes + search_bytes)
	    );
	    
	    return ret;
	    }
	
	/** drop_all. **/
	if (strcmp(param->String, "drop_all") == 0)
	    {
	    /** Print info. **/
	    printf("\nDropping cache for ");
	    if (path != NULL) printf("\"%s\":\n", path);
	    else printf("all files:\n");
	    
	    /** Free caches. **/
	    ci_ClearCaches();
	    
	    tprintf("Cache dropped.\n");
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
 ***         -1 if an error occured.
 ***/
int clusterInitialize(void)
    {
    /** Allocate the driver. **/
    pObjDriver drv = (pObjDriver)check_ptr(nmMalloc(sizeof(ObjDriver)));
    if (drv == NULL) goto err;
    memset(drv, 0, sizeof(ObjDriver));
    
    /** Initialize globals. **/
    memset(&ClusterDriverCaches, 0, sizeof(ClusterDriverCaches));
    if (!check(xhInit(&ClusterDriverCaches.SourceDataCache, 251, 0))) goto err;
    if (!check(xhInit(&ClusterDriverCaches.ClusterDataCache, 251, 0))) goto err;
    if (!check(xhInit(&ClusterDriverCaches.SearchDataCache, 251, 0))) goto err;
    
    /** Setup the structure. **/
    if (check_ptr(strcpy(drv->Name, "clu - Clustering Driver")) == NULL) goto err;
    if (!check(xaInit(&(drv->RootContentTypes), 1))) goto err;
    if (!check_neg(xaAddItem(&(drv->RootContentTypes), "system/cluster"))) goto err;
    drv->Capabilities = OBJDRV_C_TRANS | OBJDRV_C_FULLQUERY; /* TODO: Greg, double check these are correct. */
    
    /** Setup the function references. **/
    drv->Open = clusterOpen;
    drv->OpenChild = NULL;
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
    drv->PresentationHints = clusterPresentationHints;
    drv->Info = clusterInfo;
    drv->Commit = clusterCommit;
    drv->GetQueryCoverageMask = NULL;
    drv->GetQueryIdentityPath = NULL;

    /** Register some structures. **/
    nmRegister(sizeof(SourceData), "ClusterSourceData");
    nmRegister(sizeof(Cluster), "Cluster");
    nmRegister(sizeof(ClusterData), "ClusterData");
    nmRegister(sizeof(SearchData), "ClusterSearch");
    nmRegister(sizeof(NodeData), "ClusterNodeData");
    nmRegister(sizeof(DriverData), "ClusterDriverData");
    nmRegister(sizeof(ClusterQuery), "ClusterQuery");
    nmRegister(sizeof(ClusterDriverCaches), "ClusterDriverCaches");
    
    /** Print debug size info. **/
//     char buf1[16], buf2[16], buf3[16], buf4[16], buf5[16], buf6[16], buf7[16], buf8[16];
//     tprintf(
// 	"Cluster driver struct sizes:\n"
// 	"  > sizeof(SourceData):          %s\n"
// 	"  > sizeof(Cluster):             %s\n"
// 	"  > sizeof(ClusterData):         %s\n"
// 	"  > sizeof(SearchData):          %s\n"
// 	"  > sizeof(NodeData):            %s\n"
// 	"  > sizeof(DriverData):          %s\n"
// 	"  > sizeof(ClusterQuery):        %s\n"
// 	"  > sizeof(ClusterDriverCaches): %s\n",
// 	snprint_bytes(buf1, sizeof(buf1), sizeof(SourceData)),
// 	snprint_bytes(buf2, sizeof(buf2), sizeof(Cluster)),
// 	snprint_bytes(buf3, sizeof(buf3), sizeof(ClusterData)),
// 	snprint_bytes(buf4, sizeof(buf4), sizeof(SearchData)),
// 	snprint_bytes(buf5, sizeof(buf5), sizeof(NodeData)),
// 	snprint_bytes(buf6, sizeof(buf6), sizeof(DriverData)),
// 	snprint_bytes(buf7, sizeof(buf7), sizeof(ClusterQuery)),
// 	snprint_bytes(buf8, sizeof(buf8), sizeof(ClusterDriverCaches))
//     );
    
//     pVector v = ca_build_vector("");
//     const unsigned int len = ca_sparse_len(v);
//     fprintf(stderr, "Vector (x%d): [%d", len, v[0]);
//     for (unsigned int i = 1u; i < len; i++)
// 	{
// 	fprintf(stderr, ", %d", v[i]);
// 	}
//     fprintf(stderr, "]\n");
    
    /** Register the driver. **/
    if (!check(objRegisterDriver(drv))) goto err;
    
    /** Success. **/
    return 0;
    
    /** Error cleanup. **/
    err:
    if (drv != NULL) nmFree(drv, sizeof(ObjDriver));
    fprintf(stderr, "Error: Failed to initialize cluster driver.\n");
    return -1;
    }
