/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Core							*/
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
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA		*/
/* 02111-1307  USA							*/
/* 									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	objdrv_cluster.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 17, 2025					*/
/* Description:	Object driver for Centrallix cluster objects. This	*/
/* 		driver handles cluster configuration and access so	*/
/*		clustered resources can be opened, searched efficiently */
/* 		with caching to prevent unnecessary re-computation, and	*/
/* 		managed through the object system.			*/
/************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cxlib/clusters.h"
#include "cxlib/magic.h"
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "cxlib/util.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "expression.h"
#include "obj.h"
#include "param.h"
#include "st_node.h"
#include "stparse.h"

/*** This file uses the optional Comment Anchors VSCode extension, documented
 *** with CommentAnchorsExtension.md in centrallix-sysdoc.
 ***/

/** Defaults for unspecified optional attributes. **/
#define CI_DEFAULT_MIN_IMPROVEMENT 0.0001
#define CI_DEFAULT_MAX_ITERATIONS 64u
#define CI_NO_SEED 0u


/** ================ Enum Declarations ================ **/
/** ANCHOR[id=enums] **/

/** Enum type representing a clustering algorithm. **/
typedef unsigned char ClusterAlgorithm;
#define ALGORITHM_NULL             (ClusterAlgorithm)0u
#define ALGORITHM_NONE             (ClusterAlgorithm)1u
#define ALGORITHM_SLIDING_WINDOW   (ClusterAlgorithm)2u
#define ALGORITHM_KMEANS           (ClusterAlgorithm)3u
#define ALGORITHM_KMEANS_PLUS_PLUS (ClusterAlgorithm)4u
#define ALGORITHM_KMEDOIDS         (ClusterAlgorithm)5u
#define ALGORITHM_DB_SCAN          (ClusterAlgorithm)6u

ClusterAlgorithm ALL_CLUSTERING_ALGORITHMS[] =
    {
    ALGORITHM_NULL,
    ALGORITHM_NONE,
    ALGORITHM_SLIDING_WINDOW,
    ALGORITHM_KMEANS,
    ALGORITHM_KMEANS_PLUS_PLUS,
    ALGORITHM_KMEDOIDS,
    ALGORITHM_DB_SCAN,
    };
#define N_CLUSTERING_ALGORITHMS ((unsigned int)(sizeof(ALL_CLUSTERING_ALGORITHMS) / sizeof(ALL_CLUSTERING_ALGORITHMS[0])))

/** Converts a clustering algorithm to its string name. **/
char*
ci_ClusteringAlgorithmToString(ClusterAlgorithm clustering_algorithm)
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
    
    return NULL; /** Unreachable. **/
    }

/** Enum type representing a similarity measurement algorithm. **/
typedef unsigned char SimilarityMeasure;
#define SIMILARITY_NULL            (SimilarityMeasure)0u
#define SIMILARITY_COSINE          (SimilarityMeasure)1u
#define SIMILARITY_LEVENSHTEIN     (SimilarityMeasure)2u

SimilarityMeasure ALL_SIMILARITY_MEASURES[] =
    {
    SIMILARITY_NULL,
    SIMILARITY_COSINE,
    SIMILARITY_LEVENSHTEIN,
    };
#define N_SIMILARITY_MEASURES ((unsigned int)(sizeof(ALL_SIMILARITY_MEASURES) / sizeof(ALL_SIMILARITY_MEASURES[0])))

/*** Converts a similarity measure to its string name.
 *** 
 *** @param similarity_measure The similarity measure to convert.
 *** @returns The corresponding name.
 ***/
char*
ci_SimilarityMeasureToString(SimilarityMeasure similarity_measure)
    {
	switch (similarity_measure)
	    {
	    case SIMILARITY_NULL: return "NULL similarity measure";
	    case SIMILARITY_COSINE: return "cosine";
	    case SIMILARITY_LEVENSHTEIN: return "levenshtein";
	    default: return "Unknown similarity measure";
	    }
    
    return NULL; /** Unreachable. **/
    }

/*** Converts a similarity measure to a function pointer that points to the
 *** corresponding comparison function.  This function can be called directly
 *** or passed to functions from `clusters.c`.
 *** 
 *** @param similarity_measure The similarity measure to be converted.
 *** @returns A similarity computation function.  This function takes two void
 *** 	pointers, representing the data to be compared, and returns a double
 *** 	representing how similar the data is from 0.0 (no similarity) to 1.0
 *** 	(identical), or NAN if an error occurs.  This function may return NULL
 *** 	if an error occurs, in which case it always calls mssError() to give
 *** 	an error message.
 ***/
double (*ci_SimilarityMeasureToFunction(SimilarityMeasure similarity_measure))(void*, void*)
    {
    switch (similarity_measure)
	{
	case SIMILARITY_COSINE: return ca_cos_compare;
	case SIMILARITY_LEVENSHTEIN: return ca_lev_compare;
	default:
	    mssError(1, "Cluster",
		"Unknown similarity measure \"%s\" (%d).",
		ci_SimilarityMeasureToString(similarity_measure), similarity_measure
	    );
	    return NULL;
	}
    }

/*** Enum representing the type of data targeted by the driver, set based on
 *** the path given when the driver is used to open a cluster file.
 *** 
 *** `0u` is reserved for a possible NULL value in the future.  However, NULL
 *** values for TargetType are not currently allowed.
 ***/
typedef unsigned char TargetType;
#define TARGET_NODE          (TargetType)1u
#define TARGET_CLUSTER       (TargetType)2u
#define TARGET_SEARCH        (TargetType)3u
#define TARGET_CLUSTER_ENTRY (TargetType)4u
#define TARGET_SEARCH_ENTRY  (TargetType)5u

TargetType ALL_TARGET_TYPES[] =
    {
    TARGET_NODE,
    TARGET_CLUSTER,
    TARGET_SEARCH,
    TARGET_CLUSTER_ENTRY,
    TARGET_SEARCH_ENTRY,
    };
#define N_TARGET_TYPES ((unsigned int)(sizeof(ALL_TARGET_TYPES) / sizeof(ALL_TARGET_TYPES[0])))

/*** Attribute name lists by TargetType.
 *** 
 *** Promises that input attributes are listed first, before computed attributes.
 ***/
char* ROOT_ATTRS[] =
    {
    "source",
    "key_attr",
    "data_attr",
    END_OF_ARRAY,
    };
#define N_ROOT_ATTRS ((unsigned int)(sizeof(ROOT_ATTRS) / sizeof(ROOT_ATTRS[0])))
#define N_COMPUTED_ROOT_ATTRS 1u
#define N_INPUT_ROOT_ATTRS (N_ROOT_ATTRS - N_COMPUTED_ROOT_ATTRS)
char* CLUSTER_ATTRS[] =
    {
    "algorithm",
    "similarity_measure",
    "num_clusters",
    "min_improvement",
    "max_iterations",
    "seed",
    "date_created",
    "date_computed",
    };
#define N_CLUSTER_ATTRS ((unsigned int)(sizeof(CLUSTER_ATTRS) / sizeof(CLUSTER_ATTRS[0])))
#define N_COMPUTED_CLUSTER_ATTRS 2u
#define N_INPUT_CLUSTER_ATTRS (N_CLUSTER_ATTRS - N_COMPUTED_CLUSTER_ATTRS)
char* SEARCH_ATTRS[] =
    {
    "source",
    "threshold",
    "similarity_measure",
    "date_created",
    "date_computed",
    };
#define N_SEARCH_ATTRS ((unsigned int)(sizeof(SEARCH_ATTRS) / sizeof(SEARCH_ATTRS[0])))
#define N_COMPUTED_SEARCH_ATTRS 2u
#define N_INPUT_SEARCH_ATTRS (N_SEARCH_ATTRS - N_COMPUTED_SEARCH_ATTRS)
char* CLUSTER_ENTRY_ATTRS[] =
    {
    "items",
    END_OF_ARRAY,
    };
#define N_CLUSTER_ENTRY_ATTRS ((unsigned int)(sizeof(CLUSTER_ENTRY_ATTRS) / sizeof(CLUSTER_ENTRY_ATTRS[0])))
#define N_COMPUTED_CLUSTER_ENTRY_ATTRS 2u
#define N_INPUT_CLUSTER_ENTRY_ATTRS (N_CLUSTER_ENTRY_ATTRS - N_COMPUTED_CLUSTER_ENTRY_ATTRS)
char* SEARCH_ENTRY_ATTRS[] =
    {
    "key1",
    "key2",
    "sim",
    };
#define N_SEARCH_ENTRY_ATTRS ((unsigned int)(sizeof(SEARCH_ENTRY_ATTRS) / sizeof(SEARCH_ENTRY_ATTRS[0])))
#define N_COMPUTED_SEARCH_ENTRY_ATTRS 2u
#define N_INPUT_SEARCH_ENTRY_ATTRS (N_SEARCH_ENTRY_ATTRS - N_COMPUTED_SEARCH_ENTRY_ATTRS)

/** Method name list. **/
char* METHOD_NAMES[] =
    {
    "cache",
    "stat",
    };
#define METHOD_NAMES_COUNT ((unsigned int)(sizeof(METHOD_NAMES) / sizeof(METHOD_NAMES[0])))


/** ================ Struct Declarations ================ **/
/** ANCHOR[id=structs] **/

/*** Represents the data source which may have data already fetched.
 *** 
 *** Memory Stats:
 ***   - Padding: 0 bytes
 ***   - Total size: 80 bytes
 *** 
 *** @skip --> Attribute Data.
 *** @param Name The source name, specified in the .cluster file.
 *** @param Key The key associated with this object in the SourceDataCache.
 *** @param SourcePath The path to the data source from which to retrieve data.
 *** @param KeyAttr The name of the attribute to use when getting keys from
 *** 	the SourcePath.
 *** @param DataAttr The name of the attribute to use when getting data from
 *** 	the SourcePath.
 *** 
 *** @skip --> Fetched/Computed Data.
 *** @param Strings The keys for each data string strings received from the
 *** 	database, allowing them to be lined up again when queried.
 *** @param Strings The data strings to be clustered and searched, or NULL if
 *** 	they have not been fetched from the source.
 *** @param Vectors The cosine comparison vectors from the fetched data, or
 *** 	NULL if they haven't been computed.
 *** @param nVectors The number of vectors and data strings.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created/initialized.
 *** @param DateComputed The date and time that the computed attributes were computed.
 *** 
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct _SOURCE
    {
    Magic_t      Magic;
    unsigned int nVectors;
    char*        Name;
    char*        Key;
    char*        SourcePath;
    char*        KeyAttr;
    char*        DataAttr;
    char**       Keys;
    char**       Strings;
    pVector*     Vectors;
    DateTime     DateCreated;
    DateTime     DateComputed;
    }
    SourceData, *pSourceData;


/*** Computed data for a single cluster.
 *** 
 *** Memory Stats:
 ***   - Padding: 0 bytes
 ***   - Total size: 16 bytes
 *** 
 *** @param Size The number of items in the cluster.
 *** @param Indexes Represents the data points in the cluster, as indexes into
 *** 	the SourceData Keys, Strings, and Vectors fields. NULL if `Size == 0`.
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct
    {
    Magic_t       Magic;
    unsigned int  Size;
    unsigned int* Indexes;
    }
    Cluster, *pCluster;


/*** Data for each cluster. Only attribute data is checked for caching.
 *** 
 *** Memory Stats:
 ***   - Padding: 2 bytes
 ***   - Total size: 104 bytes
 *** 
 *** @skip --> Attribute Data.
 *** @param Name The cluster name, specified in the .cluster file.
 *** @param Key The key associated with this object in the ClusterDataCache.
 *** @param ClusterAlgorithm The clustering algorithm to be used.
 *** @param SimilarityMeasure The similarity measure used to compare items.
 *** @param nClusters The number of clusters. 1 if algorithm = none.
 *** @param MinImprovement The minimum amount of improvement that must be met
 *** 	each clustering iteration.  -inf represents "max" in the .cluster file.
 *** @param MaxIterations The maximum number of iterations to run clustering.
 *** 	Note: Sliding window uses this attribute to store the window_size.
 *** 
 *** @skip --> Relational Data.
 *** @param nSubClusters The number of subclusters of this cluster.
 *** @param SubClusters A pClusterData array, NULL if nSubClusters == 0.
 *** @param Parent This cluster's parent. NULL if it is not a subcluster.
 *** @param SourceData Pointer to the source data that this cluster uses.
 *** 
 *** @skip --> Computed Data.
 *** @param Clusters An array of length num_clusters, NULL if the clusters
 *** 	have not yet been computed.
 *** @param Sims An array of num_vectors elements, where index i stores the
 *** 	similarity of vector i to its assigned cluster. This attribute is NULL
 *** 	if the clusters have not yet been computed.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created/initialized.
 *** @param DateComputed The date and time that the computed attributes were computed.
 *** 
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct _CD
    {
    Magic_t           Magic;
    unsigned int      nClusters;
    char*             Name;
    char*             Key;
    ClusterAlgorithm  ClusterAlgorithm;
    SimilarityMeasure SimilarityMeasure;
    /** 2 bytes of auto-padding. **/
    unsigned int      Seed;
    double            MinImprovement;
    unsigned int      MaxIterations;
    unsigned int      nSubClusters;
    struct _CD**      SubClusters;
    struct _CD*       Parent;
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
 ***   - Padding: 7 bytes
 ***   - Total size: 72 bytes
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
 *** @param Pairs An array holding the pairs found by the search, or NULL if
 *** 	the search has not been computed.  The indexes stored in these pairs
 *** 	are indexes into the SourceData data arrays.
 *** @param nPairs The number of pairs found.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created/initialized.
 *** @param DateComputed The date and time that the computed attributes were computed.
 *** 
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct _SEARCH
    {
    Magic_t           Magic;
    /** 4 bytes of auto-padding. **/
    char*             Name;
    char*             Key;
    pClusterData      SourceCluster;
    double            Threshold;
    pPair*            Pairs;
    unsigned int      nPairs;
    SimilarityMeasure SimilarityMeasure;
    /** 3 bytes of auto-padding. **/
    DateTime          DateCreated;
    DateTime          DateComputed;
    }
    SearchData, *pSearchData;


/*** Node instance data.
 *** 
 *** Memory Stats:
 ***   - Padding: 4 bytes
 ***   - Total size: 72 bytes
 *** 
 *** @note When a .cluster file is opened, there will be only one node for
 *** that file.  However, in the course of the query, many driver instance
 *** structs using this one node may be created thanks to functions such as
 ***`clusterQueryFetch()`, and closed by with functions like `clusterClose()`.
 *** 
 *** @param SourceData Data from the provided source.
 *** @param Params A pParam array storing the params in the .cluster file.
 *** @param nParams The number of specified params.
 *** @param ParamList A "scope" for resolving parameter values during parsing.
 *** @param ClusterDatas A pCluster array for the clusters in the .cluster file.
 *** 	Will be NULL if `nClusters = 0`.
 *** @param nClusterDatas The number of specified clusters.
 *** @param SearchDatas A SearchData array for the searches in the .cluster file.
 *** @param nSearches The number of specified searches.
 *** @param nSearchDatas The parent object used to open this NodeData instance.
 *** @param OpenCount The number of open driver instances that are using the
 *** 	NodeData struct.  When this reaches 0, the struct should be freed.
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct _NODE
    {
    Magic_t        Magic;
    /** 4 bytes of auto-padding. **/
    pObject        Parent;
    pParam*        Params;
    pParamObjects  ParamList;
    pSourceData    SourceData;
    pClusterData*  ClusterDatas;
    pSearchData*   SearchDatas;
    unsigned int   OpenCount;
    unsigned int   nParams;
    unsigned int   nClusterDatas;
    unsigned int   nSearchDatas;
    }
    NodeData, *pNodeData;

/*** Driver instance data.
 ***
 *** Memory Stats:
 ***   - Padding: 5 bytes
 ***   - Total size: 32 bytes
 ***  
 *** This struct can be thought of like a "pointer" to specific data accessible
 *** through the stored pNodeData struct.  This struct also communicates whether
 *** that data is guaranteed to have been computed.
 *** 
 *** For example, if target type is the root, a cluster, or a search, no data
 *** is guaranteed to be computed.  These three types can be returned from
 *** clusterOpen(), based on the provided path.
 *** 
 *** Alternatively, a cluster entry or search entry can be targeted by calling
 *** fetch on a query pointing to a driver instance that targets a cluster or
 *** search (respectively).  These two entry target types ensure that the data
 *** they indicate has been computed.  This makes the `clusterGetAttrType()`
 *** and `clusterGetAttrValue()` functions faster and simpler because they do
 *** not need to check that the data is computed every time they are called.
 *** 
 *** @param NodeData The associated node data struct.  While many driver struct
 *** 	instances pointing to one NodeData at a time, but each driver instance
 *** 	always points to singular NodeData struct.
 *** @param TargetType The type of data targeted (see above).
 *** @param TargetData If target type is:
 *** ```txt
 *** 	Node:                    A pointer to the SourceData struct.
 *** 	Cluster or ClusterEntry: A pointer to the targeted cluster.
 *** 	Search or SearchEntry:   A pointer to the targeted search.
 *** ```
 *** @param TargetAttrIndex An index into an attribute list (for GetNextAttr()).
 *** @param TargetMethodIndex An index into an method list (for GetNextMethod()).
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct _DRIVER
    {
    Magic_t        Magic;
    /** 4 bytes of auto-padding. **/
    pNodeData      NodeData;
    void*          TargetData;
    unsigned int   TargetIndex;
    unsigned char  TargetAttrIndex;
    unsigned char  TargetMethodIndex;
    TargetType     TargetType;
    /** 1 bytes of auto-padding. **/
    }
    DriverData, *pDriverData;

/*** Query instance data.
 ***
 *** Memory Stats:
 ***   - Padding: 0 bytes
 ***   - Total size: 16 bytes
 ***
 *** @param DriverData The associated driver instance being queried.
 *** @param RowIndex The selected row of the data targeted by the driver.
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct
    {
    Magic_t        Magic;
    unsigned int   RowIndex;
    pDriverData    DriverData;
    }
    ClusterQuery, *pQueryData;


/** Global storage for caches. **/
struct
    {
    XHashTable SourceDataCache;
    XHashTable ClusterDataCache;
    XHashTable SearchDataCache;
    }
    ClusterDriverCaches = {0};

struct
    {
    unsigned long long OpenCalls;
    unsigned long long OpenQueryCalls;
    unsigned long long FetchCalls;
    unsigned long long CloseCalls;
    unsigned long long GetTypeCalls;
    unsigned long long GetValCalls;
    unsigned long long GetValCalls_name;
    unsigned long long GetValCalls_key1;
    unsigned long long GetValCalls_key2;
    unsigned long long GetValCalls_sim;
    } ClusterStatistics = {0};


/** ================ Function Declarations ================ **/
/** ANCHOR[id=functions] **/

/** Note: ci stands for "cluster_internal". **/

/** Parsing Functions. **/
// LINK #parsing
static void ci_GiveHint(const char* hint);
static bool ci_TryHint(char* value, char** valid_values, const unsigned int n_valid_values);
static void ci_UnknownAttribute(char* attr_name, int target_type);
static int ci_ParseAttribute(pStructInf inf, char* attr_name, int datatype, pObjData data, pParamObjects param_list, bool required, bool print_type_error);
static ClusterAlgorithm ci_ParseClusteringAlgorithm(pStructInf cluster_inf, pParamObjects param_list);
static SimilarityMeasure ci_ParseSimilarityMeasure(pStructInf cluster_inf, pParamObjects param_list);
static pSourceData ci_ParseSourceData(pStructInf inf, pParamObjects param_list, char* path);
static pClusterData ci_ParseClusterData(pStructInf inf, pParamObjects param_list, pSourceData source_data);
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
static size_t ci_SizeOfSourceData(pSourceData source_data);
static size_t ci_SizeOfClusterData(pClusterData cluster_data, bool recursive);
static size_t ci_SizeOfSearchData(pSearchData search_data);

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
void* clusterOpen(pObject parent, int mask, pContentType systype, char* usr_type, pObjTrxTree* oxt);
int clusterClose(void* inf_v, pObjTrxTree* oxt);
void* clusterOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt);
void* clusterQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt);
int clusterQueryClose(void* qy_v, pObjTrxTree* oxt);
int clusterGetAttrType(void* inf_v, char* attr_name, pObjTrxTree* oxt);
int clusterGetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt);
pObjPresentationHints clusterPresentationHints(void* inf_v, char* attr_name, pObjTrxTree* oxt);
char* clusterGetFirstAttr(void* inf_v, pObjTrxTree* oxt);
char* clusterGetNextAttr(void* inf_v, pObjTrxTree* oxt);
int clusterInfo(void* inf_v, pObjectInfo info);

/** Method Execution Functions. **/
// LINK #method
char* clusterGetFirstMethod(void* inf_v, pObjTrxTree* oxt);
char* clusterGetNextMethod(void* inf_v, pObjTrxTree* oxt);
static int ci_PrintEntry(pXHashEntry entry, void* arg);
static void ci_CacheFreeSourceData(pXHashEntry entry, void* path);
static void ci_CacheFreeCluster(pXHashEntry entry, void* path);
static void ci_CacheFreeSearch(pXHashEntry entry, void* path);
int clusterExecuteMethod(void* inf_v, char* method_name, pObjData param, pObjTrxTree* oxt);

/** Unimplemented DriverFunctions. **/
// LINK #unimplemented
int clusterCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
int clusterDelete(pObject obj, pObjTrxTree* oxt);
int clusterDeleteObj(void* inf_v, pObjTrxTree* oxt);
int clusterRead(void* inf_v, char* buffer, int max_cnt, int offset, int flags, pObjTrxTree* oxt);
int clusterWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
int clusterSetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt);
int clusterAddAttr(void* inf_v, char* attr_name, int type, pObjData val, pObjTrxTree* oxt);
void* clusterOpenAttr(void* inf_v, char* attr_name, int mode, pObjTrxTree* oxt);
int clusterCommit(void* inf_v, pObjTrxTree *oxt);

/** ================ Parsing Functions ================ **/
/** ANCHOR[id=parsing] **/
// LINK #functions

/** Format a hint to give to the user. **/
static void ci_GiveHint(const char* hint)
    {
	fprintf(stderr, "  > Hint: Did you mean \"%s\"?\n", hint);
    
    return;
    }


/*** Given the user a hint when they specify an invalid string for an attribute
 *** where we know the list of valid strings.  The hint is only displayed if
 *** their string is close enough to a valid string.
 *** 
 *** @param value The value the user gave.
 *** @param valid_values The valid values that could be what they meant.
 *** @param n_valid_values The number of valid values. Specify 0 to detect
 *** 	length on a null terminated array of values.
 *** @returns Whether a hint was given.
 ***/
static bool
ci_TryHint(char* value, char** valid_values, const unsigned int n_valid_values)
    {
	char* guess = ca_most_similar(value, (void**)valid_values, n_valid_values, ca_lev_compare, 0.25);
	if (guess == NULL) return false; /* No hint. */
	
	/** Issue hint. **/
	ci_GiveHint(guess);
    
    return true;
    }


/*** Display an error message when an unknown attribute is requested, including
 *** a hint about which attribute might be intended, if available.
 *** 
 *** @param attr_name The name of the missing attribute.
 *** @param target_type The target type, for determining the list of available
 *** 	attributes in this context.
 ***/
static void
ci_UnknownAttribute(char* attr_name, const int target_type)
    {
	/** Display the error message. **/
	mssError(1, "Cluster", "Unknown attribute '%s'.", attr_name);
	
	/** Collect specific attributes based on target type. **/
	char** my_attrs = NULL;
	unsigned int n_my_attrs = 0u;
	switch (target_type)
	    {
	    case TARGET_NODE:          my_attrs = ROOT_ATTRS;          n_my_attrs = N_INPUT_ROOT_ATTRS; break;
	    case TARGET_CLUSTER:       my_attrs = CLUSTER_ATTRS;       n_my_attrs = N_INPUT_CLUSTER_ATTRS; break;
	    case TARGET_SEARCH:        my_attrs = SEARCH_ATTRS;        n_my_attrs = N_INPUT_SEARCH_ATTRS; break;
	    case TARGET_CLUSTER_ENTRY: my_attrs = CLUSTER_ENTRY_ATTRS; n_my_attrs = N_INPUT_CLUSTER_ENTRY_ATTRS; break;
	    case TARGET_SEARCH_ENTRY:  my_attrs = SEARCH_ENTRY_ATTRS;  n_my_attrs = N_INPUT_SEARCH_ENTRY_ATTRS; break;
	    default:
		mssError(0, "Cluster",
		    "Unknown target type %u detected while attempting to generate hint.",
		    target_type
		);
		return;
	    }
	
	/** Attempt to give hints. **/
	if (ci_TryHint(attr_name, my_attrs, n_my_attrs));
	else if (ci_TryHint(attr_name, DRIVER_ATTRIBUTE_NAMES, N_DRIVER_ATTRIBUTE_NAMES));
    }


// LINK #functions
/*** @returns 0 if a value is found,
 ***         -1 if an error occurs, or
 ***          1 if attribute is null, calling mssError() for required attributes.
 *** 
 *** @attention - Promises that a failure invokes mssError() at least once.
 *** 
 *** TODO: Greg - Review carefully. I think this code is the reason that
 *** runserver() is sometimes not required for dynamic attributes in the
 *** cluster driver, which I'm pretty sure is incorrect.
 *** TODO: Israel - After Greg's review, update this doc comment to properly
 *** describe the function parameters.
 ***/
static int
ci_ParseAttribute(
    pStructInf inf,
    char* attr_name,
    int datatype,
    pObjData data,
    pParamObjects param_list,
    bool required,
    bool print_type_error)
    {
    int ret;
    
	/** Get attribute inf. **/
	pStructInf attr_info = stLookup(inf, attr_name);
	if (attr_info == NULL)
	     {
	     if (required) mssError(1, "Cluster", "'%s' must be specified for clustering.", attr_name);
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
	    mssError(0, "Cluster", "Expression evaluation failed (error code %d).", ret);
	    goto err;
	    }
	
	/** Check for data type mismatch. **/
	if (datatype != exp->DataType)
	    {
	    mssError(1, "Cluster",
		"Expected ['%s' : %s], but got type %s.",
		attr_name, objTypeToStr(datatype), objTypeToStr(exp->DataType)
	    );
	    goto err;
	    }
	
	/** Get the data out of the expression. **/
	ret = expExpressionToPod(exp, datatype, data);
	if (ret != 0)
	    {
	    mssError(1, "Cluster",
		"Failed to get ['%s' : %s] using expression \"%s\" (error code %d).",
		attr_name, objTypeToStr(datatype), exp->Name, ret
	    );
	    goto err;
	    }
	
	/** Success. **/
	return 0;
	
    err:
	mssError(0, "Cluster",
	    "Failed to parse attribute \"%s\" from group \"%s\"",
	    attr_name, inf->Name
	);
	
	/** Return error. **/
	return -1;
    }


// LINK #functions
/*** Parses a ClusteringAlgorithm from the algorithm attribute in the pStructInf.
 *** 
 *** @attention - Promises that a failure invokes mssError() at least once.
 *** 
 *** @param inf A parsed pStructInf.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @returns The data algorithm, or ALGORITHM_NULL on failure.
 ***/
static ClusterAlgorithm
ci_ParseClusteringAlgorithm(pStructInf inf, pParamObjects param_list)
    {
	/** Get the algorithm attribute. **/
	char* algorithm;
	if (ci_ParseAttribute(inf, "algorithm", DATA_T_STRING, POD(&algorithm), param_list, true, true) != 0)
	    {
	    mssError(0, "Cluster", "Failed to parse attribute 'algorithm' in group \"%s\".", inf->Name);
	    return ALGORITHM_NULL;
	    }
	
	/** Parse known clustering algorithms. **/
	if (strcasecmp(algorithm, "none") == 0)           return ALGORITHM_NONE;
	if (strcasecmp(algorithm, "sliding-window") == 0) return ALGORITHM_SLIDING_WINDOW;
	if (strcasecmp(algorithm, "k-means") == 0)        return ALGORITHM_KMEANS;
	if (strcasecmp(algorithm, "k-means++") == 0)      return ALGORITHM_KMEANS_PLUS_PLUS;
	if (strcasecmp(algorithm, "k-medoids") == 0)      return ALGORITHM_KMEDOIDS;
	if (strcasecmp(algorithm, "db-scan") == 0)        return ALGORITHM_DB_SCAN;
	
	/** Unknown value for clustering algorithm. **/
	mssError(1, "Cluster", "Unknown \"clustering algorithm\": %s", algorithm);
	
	/** Attempt to give a hint. **/
	char* all_names[N_CLUSTERING_ALGORITHMS] = {NULL};
	for (unsigned int i = 1u; i < N_CLUSTERING_ALGORITHMS; i++)
	    all_names[i] = ci_ClusteringAlgorithmToString(ALL_CLUSTERING_ALGORITHMS[i]);
	if (ci_TryHint(algorithm, all_names, N_CLUSTERING_ALGORITHMS));
	else if (strcasecmp(algorithm, "sliding") == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_SLIDING_WINDOW));
	else if (strcasecmp(algorithm, "window")  == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_SLIDING_WINDOW));
	else if (strcasecmp(algorithm, "null")    == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_NONE));
	else if (strcasecmp(algorithm, "nothing") == 0) ci_GiveHint(ci_ClusteringAlgorithmToString(ALGORITHM_NONE));
    
    /** Fail. **/
    return ALGORITHM_NULL;
    }


// LINK #functions
/*** Parses a SimilarityMeasure from the similarity_measure attribute in the given
 *** pStructInf parameter.
 *** 
 *** @attention - Promises that a failure invokes mssError() at least once.
 *** 
 *** @param inf A parsed pStructInf.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @returns The similarity measure, or SIMILARITY_NULL on failure.
 ***/
static SimilarityMeasure
ci_ParseSimilarityMeasure(pStructInf inf, pParamObjects param_list)
    {
	/** Get the similarity_measure attribute. **/
	char* measure;
	if (ci_ParseAttribute(inf, "similarity_measure", DATA_T_STRING, POD(&measure), param_list, true, true) != 0)
	    {
	    mssError(0, "Cluster", "Failed to parse attribute 'similarity_measure' in group \"%s\".", inf->Name);
	    return SIMILARITY_NULL;
	    }
	
	/** Parse known clustering algorithms. **/
	if (!strcasecmp(measure, "cosine"))      return SIMILARITY_COSINE;
	if (!strcasecmp(measure, "levenshtein")) return SIMILARITY_LEVENSHTEIN;
	
	/** Unknown similarity measure. **/
	mssError(1, "Cluster", "Unknown \"similarity measure\": %s", measure);
	
	/** Attempt to give a hint. **/
	char* all_names[N_SIMILARITY_MEASURES] = {NULL};
	for (unsigned int i = 1u; i < N_SIMILARITY_MEASURES; i++)
	    all_names[i] = ci_SimilarityMeasureToString(ALL_SIMILARITY_MEASURES[i]);
	if (ci_TryHint(measure, all_names, N_SIMILARITY_MEASURES));
	else if (strcasecmp(measure, "cos")           == 0) ci_GiveHint(ci_SimilarityMeasureToString(SIMILARITY_COSINE));
	else if (strcasecmp(measure, "lev")           == 0) ci_GiveHint(ci_SimilarityMeasureToString(SIMILARITY_LEVENSHTEIN));
	else if (strcasecmp(measure, "edit-dist")     == 0) ci_GiveHint(ci_SimilarityMeasureToString(SIMILARITY_LEVENSHTEIN));
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
static pSourceData
ci_ParseSourceData(pStructInf inf, pParamObjects param_list, char* path)
    {
    char* buf = NULL;
    pSourceData source_data = NULL;
    
	/** Magic checks. **/
	ASSERTMAGIC(inf, MGK_STRUCTINF);
	
	/** Allocate SourceData. **/
	source_data = check_ptr(nmMalloc(sizeof(SourceData)));
	if (source_data == NULL) goto err_free;
	memset(source_data, 0, sizeof(SourceData));
	SETMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Initialize obvious values for SourceData. **/
	source_data->Name = check_ptr(nmSysStrdup(inf->Name));
	if (source_data->Name == NULL) goto err_free;
	if (!check(objCurrentDate(&source_data->DateCreated))) goto err_free;
	
	/** Get source. **/
	if (ci_ParseAttribute(inf, "source", DATA_T_STRING, POD(&buf), param_list, true, true) != 0) goto err_free;
	source_data->SourcePath = check_ptr(nmSysStrdup(buf));
	if (source_data->SourcePath == NULL) goto err_free;
	
	/** Get the attribute name to use when querying keys from the source. **/
	if (ci_ParseAttribute(inf, "key_attr", DATA_T_STRING, POD(&buf), param_list, true, true) != 0) goto err_free;
	source_data->KeyAttr = check_ptr(nmSysStrdup(buf));
	if (source_data->KeyAttr == NULL) goto err_free;
	
	/** Get the attribute name to use for querying data from the source. **/
	if (ci_ParseAttribute(inf, "data_attr", DATA_T_STRING, POD(&buf), param_list, true, true) != 0) goto err_free;
	source_data->DataAttr = check_ptr(nmSysStrdup(buf));
	if (source_data->DataAttr == NULL) goto err_free;
	
	/** Create cache entry key. **/
	const size_t len = strlen(path)
	    + strlen(source_data->SourcePath)
	    + strlen(source_data->KeyAttr)
	    + strlen(source_data->DataAttr) + 5lu;
	source_data->Key = check_ptr(nmSysMalloc(len * sizeof(char)));
	if (source_data->Key == NULL) goto err_free;
	snprintf(source_data->Key, len,
	    "%s?%s->%s:%s",
	    path, source_data->SourcePath, source_data->KeyAttr, source_data->DataAttr
	);
	
	/** Check for a cached version. **/
	pSourceData source_maybe = (pSourceData)xhLookup(&ClusterDriverCaches.SourceDataCache, source_data->Key);
	if (source_maybe != NULL)
	    { /* Cache hit. */
	    ASSERTMAGIC(source_maybe, MGK_CL_SOURCE_DATA);
	    
	    /** Free data we don't need. **/
	    nmSysFree(source_data->Key);
	    ci_FreeSourceData(source_data);
	    
	    /** Return the cached source data. **/
	    return source_maybe;
	    }
	
	/** Cache miss: Add the new object to the cache for next time. **/
	if (!check(xhAdd(&ClusterDriverCaches.SourceDataCache, source_data->Key, (void*)source_data)))
	    goto err_free;
	
	/** Success. **/
	return source_data;
	
	/** Error handling. **/
    err_free:
	if (source_data != NULL)
	    {
	    if (source_data->Key != NULL) nmSysFree(source_data->Key);
	    ci_FreeSourceData(source_data);
	    }
	
	mssError(0, "Cluster",
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
static pClusterData
ci_ParseClusterData(pStructInf inf, pParamObjects param_list, pSourceData source_data)
    {
    int result;
    pClusterData cluster_data = NULL;
    XArray sub_clusters = {0};
    char* key = NULL;
    
	/** Verify source_data value. **/
	if (source_data == NULL) goto err_free;
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Allocate space for data struct. **/
	cluster_data = check_ptr(nmMalloc(sizeof(ClusterData)));
	if (cluster_data == NULL) goto err_free;
	memset(cluster_data, 0, sizeof(ClusterData));
	SETMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	
	/** Basic fields. **/
	cluster_data->Name = check_ptr(nmSysStrdup(inf->Name));
	if (cluster_data->Name == NULL) goto err_free;
	cluster_data->SourceData = source_data;
	if (!check(objCurrentDate(&cluster_data->DateCreated))) goto err_free;
	
	/** Get algorithm. **/
	cluster_data->ClusterAlgorithm = ci_ParseClusteringAlgorithm(inf, param_list);
	if (cluster_data->ClusterAlgorithm == ALGORITHM_NULL) goto err_free;
	
	/** Handle no clustering case. **/
	if (cluster_data->ClusterAlgorithm == ALGORITHM_NONE)
	    {
	    cluster_data->nClusters = 1u;
	    goto parsing_done;
	    }
	
	/** Get similarity_measure. **/
	cluster_data->SimilarityMeasure = ci_ParseSimilarityMeasure(inf, param_list);
	if (cluster_data->SimilarityMeasure == SIMILARITY_NULL) goto err_free;
	
	/** Handle sliding window case. **/
	if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
	    {
	    /** Sliding window doesn't allocate any clusters. **/
	    cluster_data->nClusters = 0u;
	    
	    /** Get window_size. **/
	    int window_size;
	    if (ci_ParseAttribute(inf, "window_size", DATA_T_INTEGER, POD(&window_size), param_list, true, true) != 0)
		goto err_free;
	    if (window_size < 1)
		{
		mssError(1, "Cluster", "Invalid value for [window_size : uint > 0]: %d", window_size);
		goto err_free;
		}
	    
	    /** Store value. **/
	    cluster_data->MaxIterations = (unsigned int)window_size;
	    goto parsing_done;
	    }
	
	/** Get num_clusters. **/
	int num_clusters;
	if (ci_ParseAttribute(inf, "num_clusters", DATA_T_INTEGER, POD(&num_clusters), param_list, true, true) != 0)
	    goto err_free;
	if (num_clusters < 2)
	    {
	    mssError(1, "Cluster", "Invalid value for [num_clusters : uint > 1]: %d", num_clusters);
	    if (num_clusters == 1) fprintf(stderr, "HINT: Use algorithm=\"none\" to disable clustering.\n");
	    goto err_free;
	    }
	cluster_data->nClusters = (unsigned int)num_clusters;
	
	/** Get min_improvement. **/
	double improvement;
	result = ci_ParseAttribute(inf, "min_improvement", DATA_T_DOUBLE, POD(&improvement), param_list, false, false);
	if (result == 1) cluster_data->MinImprovement = CI_DEFAULT_MIN_IMPROVEMENT;
	else if (result == 0)
	    {
	    if (improvement <= 0.0 || 1.0 <= improvement)
		{
		mssError(1, "Cluster", "Invalid value for [min_improvement : 0.0 < x < 1.0 | \"none\"]: %g", improvement);
		goto err_free;
		}
	    
	    /** Successfully got value. **/
	    cluster_data->MinImprovement = improvement;
	    }
	else if (result == -1)
	    {
	    char* str;
	    result = ci_ParseAttribute(inf, "min_improvement", DATA_T_STRING, POD(&str), param_list, false, true);
	    if (result != 0) goto err_free;
	    if (strcasecmp(str, "none") != 0)
		{
		mssError(1, "Cluster", "Invalid value for [min_improvement : 0.0 < x < 1.0 | \"none\"]: %s", str);
		goto err_free;
		}
	    
	    /** Successfully got none. **/
	    cluster_data->MinImprovement = -INFINITY;
	    }
	
	/** Get max_iterations. **/
	int max_iterations;
	result = ci_ParseAttribute(inf, "max_iterations", DATA_T_INTEGER, POD(&max_iterations), param_list, false, true);
	if (result == -1) goto err_free;
	if (result == 0)
	    {
	    if (max_iterations < 1)
		{
		mssError(1, "Cluster", "Invalid value for [max_iterations : uint > 0]: %d", max_iterations);
		goto err_free;
		}
	    cluster_data->MaxIterations = (unsigned int)max_iterations;
	    }
	else cluster_data->MaxIterations = CI_DEFAULT_MAX_ITERATIONS;
	
	/** Get seed. **/
	int seed;
	result = ci_ParseAttribute(inf, "seed", DATA_T_INTEGER, POD(&seed), param_list, false, true);
	if (result == -1) goto err_free;
	if (result == 0)
	    {
	    if (seed < 1)
		{
		mssError(1, "Cluster", "Invalid value for [seed : uint > 0]: %d", seed);
		goto err_free;
		}
	    cluster_data->Seed = (unsigned int)seed;
	    }
	else cluster_data->Seed = CI_NO_SEED;
	
	/** Search for sub-clusters. **/
	if (!check(xaInit(&sub_clusters, 4u))) goto err_free;
	for (unsigned int i = 0u; i < inf->nSubInf; i++)
	    {
	    pStructInf sub_inf = check_ptr(inf->SubInf[i]);
	    if (sub_inf == NULL)
		{
		mssError(1, "Cluster", "Failed to get %uth subinf.", i);
		goto err_free;
		}
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
			"seed",
		    };
		    const unsigned int num_attrs = sizeof(attrs) / sizeof(char*);
		    
		    /** Ignore valid attribute names. **/
		    bool is_valid = false;
		    for (unsigned int i = 0u; i < num_attrs; i++)
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
		    if (ci_TryHint(name, attrs, num_attrs));
		    else if (strcasecmp(name, "k") == 0) ci_GiveHint("num_clusters");
		    else if (strcasecmp(name, "threshold") == 0) ci_GiveHint("min_improvement");
		    
		    break;
		    }
		
		case ST_T_SUBGROUP:
		    {
		    /** Select array by group type. **/
		    char* group_type = check_ptr(sub_inf->UsrType);
		    if (group_type == NULL) goto err_free;
		    if (strcmp(group_type, "cluster/cluster") != 0)
			{
			mssError(1, "Cluster",
			    "Warning: Unknown group [\"%s\" : \"%s\"] in cluster \"%s\".\n",
			    name, group_type, inf->Name
			);
			ci_GiveHint("cluster/cluster");
			continue;
			}
		    
		    /** Subcluster found. **/
		    pClusterData sub_cluster = ci_ParseClusterData(sub_inf, param_list, source_data);
		    if (sub_cluster == NULL) goto err_free;
		    sub_cluster->Parent = cluster_data;
		    if (!check_neg(xaAddItem(&sub_clusters, sub_cluster))) goto err_free;
		    
		    break;
		    }
		
		default:
		    {
		    mssError(1, "Cluster",
			"Warning: Unknown struct type %d in cluster \"%s\".",
			struct_type, inf->Name
		    );
		    goto err_free;
		    }
		}
	    }
	cluster_data->nSubClusters = sub_clusters.nItems;
	cluster_data->SubClusters = (ClusterData**)check_ptr(xaToArray(&sub_clusters));
	if (cluster_data->SubClusters == NULL) goto err_free;
	check(xaDeInit(&sub_clusters)); /* Failure ignored. */
	sub_clusters.nAlloc = 0;
	
	/** Create the cache key. **/
    parsing_done:;
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
	    { /* Cache hit. */
	    ASSERTMAGIC(cluster_maybe, MGK_CL_CLUSTER_DATA);
	    
	    /** Free the parsed cluster that we no longer need. */
	    ci_FreeClusterData(cluster_data, false);
	    nmSysFree(key);
	    
	    /** Return the cached cluster. **/
	    return cluster_maybe;
	    }
	
	/** Cache miss. **/
	if (!check(xhAdd(&ClusterDriverCaches.ClusterDataCache, key, (void*)cluster_data))) goto err_free;
	return cluster_data;
	
	/** Error cleanup. **/
    err_free:
	if (key != NULL) nmSysFree(key);
	
	if (sub_clusters.nAlloc != 0)
	    {
	    for (unsigned int i = 0u; i < sub_clusters.nItems; i++)
		{
		pClusterData cur = sub_clusters.Items[i];
		if (cur == NULL) break;
		ci_FreeClusterData(cur, true);
		}
	    check(xaDeInit(&sub_clusters)); /* Failure ignored. */
	    }
	
	if (cluster_data != NULL) ci_FreeClusterData(cluster_data, false);
	
	mssError(0, "Cluster", "Failed to parse cluster from group \"%s\".", inf->Name);
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
static pSearchData
ci_ParseSearchData(pStructInf inf, pNodeData node_data)
    {
    pSearchData search_data = NULL;
    char* key = NULL;
    
	/** Extract values. **/
	pParamObjects param_list = check_ptr(node_data->ParamList);
	if (param_list == NULL) goto err_free;
	
	/** Allocate space for search struct. **/
	search_data = check_ptr(nmMalloc(sizeof(SearchData)));
	if (search_data == NULL) goto err_free;
	memset(search_data, 0, sizeof(SearchData));
	SETMAGIC(search_data, MGK_CL_SEARCH_DATA);
	
	/** Get basic information. **/
	search_data->Name = check_ptr(nmSysStrdup(inf->Name));
	if (search_data->Name == NULL) goto err_free;
	if (!check(objCurrentDate(&search_data->DateCreated))) goto err_free;
	
	/** Search for the source cluster. **/
	char* source_cluster_name;
	if (ci_ParseAttribute(inf, "source", DATA_T_STRING, POD(&source_cluster_name), param_list, true, true) != 0) return NULL;
	for (unsigned int i = 0; i < node_data->nClusterDatas; i++)
	    {
	    pClusterData cluster_data = node_data->ClusterDatas[i];
	    ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	    if (strcmp(source_cluster_name, cluster_data->Name) == 0)
		{
		/** SourceCluster found. **/
		search_data->SourceCluster = cluster_data;
		break;
		}
	    
	    /** Note: Subclusters should probably be parsed here, if they were implemented. **/
	    }
	
	/** Did we find the requested source? **/
	if (search_data->SourceCluster == NULL)
	    {
	    /** Print error. **/
	    mssError(1, "Cluster", "Could not find cluster \"%s\" for search \"%s\".", source_cluster_name, search_data->Name);
	    
	    /** Attempt to give a hint. **/
	    char* cluster_names[node_data->nClusterDatas];
	    for (unsigned int i = 0; i < node_data->nClusterDatas; i++)
		cluster_names[i] = node_data->ClusterDatas[i]->Name;
	    ci_TryHint(source_cluster_name, cluster_names, node_data->nClusterDatas);
	    
	    /** Fail. **/
	    goto err_free;
	    }
	
	/** Get threshold attribute. **/
	if (ci_ParseAttribute(inf, "threshold", DATA_T_DOUBLE, POD(&search_data->Threshold), param_list, true, true) != 0) goto err_free;
	if (search_data->Threshold <= 0.0 || 1.0 <= search_data->Threshold)
	    {
	    mssError(1, "Cluster",
		"Invalid value for [threshold : 0.0 < x < 1.0 | \"none\"]: %g",
		search_data->Threshold
	    );
	    goto err_free;
	    }
	
	/** Get similarity measure. **/
	search_data->SimilarityMeasure = ci_ParseSimilarityMeasure(inf, param_list);
	if (search_data->SimilarityMeasure == SIMILARITY_NULL) goto err_free;
	
	/** Check for additional data to warn the user about. **/
	for (unsigned int i = 0u; i < inf->nSubInf; i++)
	    {
	    pStructInf sub_inf = check_ptr(inf->SubInf[i]);
	    if (sub_inf == NULL)
		{
		mssError(1, "Cluster", "Failed to get %uth subinf.", i);
		goto err_free;
		}
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
		    const unsigned int num_attrs = sizeof(attrs) / sizeof(char*);
		    
		    /** Ignore valid attribute names. **/
		    bool is_valid = false;
		    for (unsigned int i = 0u; i < num_attrs; i++)
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
		    ci_TryHint(name, attrs, num_attrs);
		    
		    break;
		    }
		
		case ST_T_SUBGROUP:
		    {
		    /** The spec does not specify any valid sub-groups for searches. **/
		    char* group_type = check_ptr(sub_inf->UsrType);
		    if (group_type == NULL) goto err_free;
		    fprintf(stderr,
			"Warning: Unknown group [\"%s\" : \"%s\"] in search \"%s\".\n",
			name, group_type, inf->Name
		    );
		    break;
		    }
		
		default:
		    {
		    mssError(1, "Cluster",
			"Warning: Unknown struct type %d in search \"%s\".",
			struct_type, inf->Name
		    );
		    goto err_free;
		    }
		}
	    }
	
	/** Create cache entry key. **/
	char* source_key = search_data->SourceCluster->Key;
	const size_t len = strlen(source_key) + strlen(search_data->Name) + 16lu;
	key = check_ptr(nmSysMalloc(len * sizeof(char)));
	if (key == NULL) goto err_free;
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
	    { /* Cache hit. */
	    ASSERTMAGIC(search_maybe, MGK_CL_SEARCH_DATA);
	    
	    /** Free the parsed search that we no longer need. **/
	    if (search_data != NULL) ci_FreeSearchData(search_data);
	    if (key != NULL) nmSysFree(key);
	    
	    /** Return the cached search. **/
	    return search_maybe;
	    }
	
	/** Cache miss. **/
	check(xhAdd(search_cache, key, (void*)search_data));
	return search_data;
	
	/** Error cleanup. **/
    err_free:
	if (search_data != NULL) ci_FreeSearchData(search_data);
	
	mssError(0, "Cluster", "Failed to parse SearchData from group \"%s\".", inf->Name);
	
	return NULL;
    }


// LINK #functions
/*** Allocates a new pNodeData struct from a parsed pStructInf.
 *** 
 *** @attention - Does not use caching directly, but uses sub-functions to
 *** 	handle caching of substructures.
 *** @attention - Promises that mssError() will be invoked on failure, so the
 *** 	caller is not required to specify their own error message.
 *** 
 *** @param inf A parsed pStructInf for the top level group in a .cluster
 *** 	structure file.
 *** @param parent The parent object struct.
 *** @returns A new pNodeData struct on success, or NULL on failure.
 ***/
static pNodeData
ci_ParseNodeData(pStructInf inf, pObject parent)
    {
    int ret = -1;
    pNodeData node_data = NULL;
    XArray param_infs = {0};
    XArray cluster_infs = {0};
    XArray search_infs = {0};
    
	/** Magic. **/
	ASSERTMAGIC(inf, MGK_STRUCTINF);
	ASSERTMAGIC(parent, MGK_OBJECT);
	
	/** Get file path. **/
	char* path = check_ptr(objFilePath(parent));
	if (path == NULL) goto err_free;
	
	/** Allocate node struct data. **/
	node_data = check_ptr(nmMalloc(sizeof(NodeData)));
	if (node_data == NULL) goto err_free;
	memset(node_data, 0, sizeof(NodeData));
	SETMAGIC(node_data, MGK_CL_NODE_DATA);
	node_data->Parent = parent;
	
	/** Set up param list. **/
	node_data->ParamList = check_ptr(expCreateParamList());
	if (node_data->ParamList == NULL) goto err_free;
	node_data->ParamList->Session = check_ptr(parent->Session);
	if (node_data->ParamList->Session == NULL) goto err_free;
	ret = expAddParamToList(node_data->ParamList, "parameters", (void*)node_data, 0);
	if (ret != 0)
	    {
	    mssError(0, "Cluster", "Failed to add parameters to the param list scope (error code %d).", ret);
	    goto err_free;
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
	    mssError(0, "Cluster", "Failed to set param functions (error code %d).", ret);
	    goto err_free;
	    }
	
	/** Detect relevant groups. **/
	if (!check(xaInit(&param_infs, 8))) goto err_free;
	if (!check(xaInit(&cluster_infs, 8))) goto err_free;
	if (!check(xaInit(&search_infs, 8))) goto err_free;
	for (unsigned int i = 0u; i < inf->nSubInf; i++)
	    {
	    pStructInf sub_inf = check_ptr(inf->SubInf[i]);
	    if (sub_inf == NULL)
		{
		mssError(1, "Cluster", "Failed to get %uth subinf.", i);
		goto err_free;
		}
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
			"key_attr",
			"data_attr",
		    };
		    const unsigned int num_attrs = sizeof(attrs) / sizeof(char*);
		    
		    /** Ignore valid attribute names. **/
		    bool is_valid = false;
		    for (unsigned int i = 0u; i < num_attrs; i++)
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
		    ci_TryHint(name, attrs, num_attrs);
		    
		    break;
		    }
		
		case ST_T_SUBGROUP:
		    {
		    /** The spec does not specify any valid sub-groups for searches. **/
		    char* group_type = check_ptr(sub_inf->UsrType);
		    if (group_type == NULL) goto err_free;
		    if (strcmp(group_type, "cluster/parameter") == 0)
			{
			if (!check_neg(xaAddItem(&param_infs, sub_inf)))
			    goto err_free;
			}
		    else if (strcmp(group_type, "cluster/cluster") == 0)
			{
			if (!check_neg(xaAddItem(&cluster_infs, sub_inf)))
			    goto err_free;
			}
		    else if (strcmp(group_type, "cluster/search") == 0)
			{
			if (!check_neg(xaAddItem(&search_infs, sub_inf)))
			    goto err_free;
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
		    mssError(1, "Cluster",
			"Warning: Unknown struct type %d in search \"%s\".",
			struct_type, inf->Name
		    );
		    goto err_free;
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
	
	/** Allocate space to store params. **/
	node_data->nParams = param_infs.nItems;
	const size_t params_size = node_data->nParams * sizeof(pParam);
	node_data->Params = check_ptr(nmSysMalloc(params_size));
	if (node_data->Params == NULL) goto err_free;
	memset(node_data->Params, 0, params_size);
	
	/** Iterate over each param in the structure file. **/
	for (unsigned int i = 0u; i < node_data->nParams; i++)
	    {
	    pParam param = paramCreateFromInf(param_infs.Items[i]);
	    if (param == NULL)
		{
		mssError(0, "Cluster",
		    "Failed to create param from inf for param #%u: %s",
		    i, ((pStructInf)param_infs.Items[i])->Name
		);
		goto err_free;
		}
	    node_data->Params[i] = param;
	    
	    /** Check each provided param to see if the user provided value. **/
	    for (unsigned int j = 0u; j < num_provided_params; j++)
		{
		pStruct provided_param = check_ptr(provided_params[j]);
		if (provided_param == NULL) goto err_free;
		
		/** If this provided param value isn't for the param, ignore it. **/
		if (strcmp(provided_param->Name, param->Name) != 0) continue;
		
		/** Matched! The user is providing a value for this param. **/
		ret = paramSetValueFromInfNe(param, provided_param, 0, node_data->ParamList, node_data->ParamList->Session);
		if (ret != 0)
		    {
		    mssError(0, "Cluster",
			"Failed to set param value from struct info.\n"
			"  > Param #%u: %s\n"
			"  > Provided Param #%u: %n\n"
			"  > Error code: %d",
			i, param->Name,
			j, provided_param->Name,
			ret
		    );
		    goto err_free;
		    }
		
		/** Provided value successfully handled, we're done. **/
		break;
		}
	    
	    /** Invoke param hints parsing. **/
	    ret = paramEvalHints(param, node_data->ParamList, node_data->ParamList->Session);
	    if (ret != 0)
		{
		mssError(0, "Cluster",
		    "Failed to evaluate parameter hints for parameter \"%s\" (error code %d).",
		    param->Name, ret
		);
		goto err_free;
		}
	    }
	check(xaDeInit(&param_infs)); /* Failure ignored. */
	param_infs.nAlloc = 0;
	
	/** Iterate over provided parameters and warn the user if they specified a parameter that does not exist. **/
	for (unsigned int i = 0u; i < num_provided_params; i++)
	    {
	    pStruct provided_param = check_ptr(provided_params[i]);
	    if (provided_param == NULL) goto err_free;
	    char* provided_name = provided_param->Name;
	    
	    /** Look to see if this provided param actually exists for this driver instance. **/
	    for (unsigned int j = 0u; j < node_data->nParams; j++)
		if (strcmp(provided_name, node_data->Params[j]->Name) == 0)
		    goto next_provided_param;
	    
	    /** This param doesn't exist, warn the user and attempt to give them a hint. **/
	    fprintf(stderr, "Warning: Unknown provided parameter '%s' for cluster file: %s.\n", provided_name, objFileName(parent));
	    char** param_names = check_ptr(nmSysMalloc(node_data->nParams * sizeof(char*)));
	    for (unsigned int j = 0u; j < node_data->nParams; j++)
		param_names[j] = node_data->Params[j]->Name;
	    ci_TryHint(provided_name, param_names, node_data->nParams);
	    nmSysFree(param_names);
	    
	    next_provided_param:;
	    }
	
	/** Parse source data. **/
	node_data->SourceData = ci_ParseSourceData(inf, node_data->ParamList, path);
	if (node_data->SourceData == NULL) goto err_free;
	
	/** Parse each cluster. **/
	node_data->nClusterDatas = cluster_infs.nItems;
	if (node_data->nClusterDatas > 0)
	    {
	    const size_t clusters_size = node_data->nClusterDatas * sizeof(pClusterData);
	    node_data->ClusterDatas = check_ptr(nmSysMalloc(clusters_size));
	    if (node_data->ClusterDatas == NULL) goto err_free;
	    memset(node_data->ClusterDatas, 0, clusters_size);
	    for (unsigned int i = 0u; i < node_data->nClusterDatas; i++)
		{
		node_data->ClusterDatas[i] = ci_ParseClusterData(cluster_infs.Items[i], node_data->ParamList, node_data->SourceData);
		if (node_data->ClusterDatas[i] == NULL) goto err_free;
		ASSERTMAGIC(node_data->ClusterDatas[i], MGK_CL_CLUSTER_DATA);
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
	    if (node_data->SearchDatas == NULL) goto err_free;
	    memset(node_data->SearchDatas, 0, searches_size);
	    for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
		{
		node_data->SearchDatas[i] = ci_ParseSearchData(search_infs.Items[i], node_data);
		if (node_data->SearchDatas[i] == NULL) goto err_free;
		ASSERTMAGIC(node_data->SearchDatas[i], MGK_CL_SEARCH_DATA);
		}
	    }
	else node_data->SearchDatas = NULL;
	check(xaDeInit(&search_infs)); /* Failure ignored. */
	search_infs.nAlloc = 0;
	
	/** Success. **/
	return node_data;
	
    err_free:
	if (param_infs.nAlloc   != 0) check(xaDeInit(&param_infs));   /* Failure ignored. */
	if (cluster_infs.nAlloc != 0) check(xaDeInit(&cluster_infs)); /* Failure ignored. */
	if (search_infs.nAlloc  != 0) check(xaDeInit(&search_infs));  /* Failure ignored. */
	if (node_data != NULL) ci_FreeNodeData(node_data);
	mssError(0, "Cluster", "Failed to parse node from group \"%s\" in file: %s", inf->Name, path);
    
	return NULL;
    }


/** ================ Freeing Functions ================ **/
/** ANCHOR[id=freeing] **/
// LINK #functions

/** @param source_data A pSourceData struct, freed by this function. **/
static void
ci_FreeSourceData(pSourceData source_data)
    {
	/** Guard segfault. **/
	if (source_data == NULL)
	    {
	    fprintf(stderr, "Warning: Call to ci_FreeSourceData(NULL);\n");
	    return;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Free top level attributes, if they exist. **/
	/** Note: The key field is handled by the caching system, so we shouldn't free it here. **/
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
	if (source_data->KeyAttr != NULL)
	    {
	    nmSysFree(source_data->KeyAttr);
	    source_data->KeyAttr = NULL;
	    }
	if (source_data->DataAttr != NULL)
	    {
	    nmSysFree(source_data->DataAttr);
	    source_data->DataAttr = NULL;
	    }
	
	/** Free fetched keys, if they exist. **/
	if (source_data->Keys != NULL)
	    {
	    for (unsigned int i = 0u; i < source_data->nVectors; i++)
		{
		if (source_data->Keys[i] != NULL)
		    {
		    nmSysFree(source_data->Keys[i]);
		    source_data->Keys[i] = NULL;
		    }
		}
	    nmSysFree(source_data->Keys);
	    source_data->Keys = NULL;
	    }
	
	/** Free fetched data, if it exists. **/
	if (source_data->Strings != NULL)
	    {
	    for (unsigned int i = 0u; i < source_data->nVectors; i++)
		{
		if (source_data->Strings[i] != NULL)
		    {
		    nmSysFree(source_data->Strings[i]);
		    source_data->Strings[i] = NULL;
		    }
		}
	    nmSysFree(source_data->Strings);
	    source_data->Strings = NULL;
	    }
	
	/** Free computed vectors, if they exist. **/
	if (source_data->Vectors != NULL)
	    {
	    for (unsigned int i = 0u; i < source_data->nVectors; i++)
		{
		if (source_data->Vectors[i] != NULL)
		    {
		    ca_free_vector(source_data->Vectors[i]);
		    source_data->Vectors[i] = NULL;
		    }
		}
	    nmSysFree(source_data->Vectors);
	    source_data->Vectors = NULL;
	    }
	
	/** Free the source data struct. **/
	nmFree(source_data, sizeof(SourceData));
	source_data = NULL;
    
    return;
    }


// LINK #functions
/*** Free pClusterData struct with an option to recursively free subclusters.
 *** 
 *** @param cluster_data The cluster data struct to free.
 *** @param recursive Whether to recursively free subclusters.
 ***/
static void
ci_FreeClusterData(pClusterData cluster_data, bool recursive)
    {
	/** Guard segfault. **/
	if (cluster_data == NULL)
	    {
	    fprintf(stderr, "Warning: Call to ci_FreeClusterData(NULL, %s);\n", (recursive) ? "true" : "false");
	    return;
	    }
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	
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
		if (cluster == NULL) continue;
		if (cluster->Indexes != NULL) nmSysFree(cluster->Indexes);
		cluster->Indexes = NULL;
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
		    if (cluster_data->SubClusters[i] != NULL)
			ci_FreeClusterData(cluster_data->SubClusters[i], recursive);
		    else continue;
		    cluster_data->SubClusters[i] = NULL;
		    }
		}
	    nmSysFree(cluster_data->SubClusters);
	    cluster_data->SubClusters = NULL;
	    }
	
	/** Free the cluster data struct. **/
	nmFree(cluster_data, sizeof(ClusterData));
	cluster_data = NULL;
    
    return;
    }


// LINK #functions
/** @param search_data A pSearchData struct, freed by this function. **/
static void
ci_FreeSearchData(pSearchData search_data)
    {
	/** Guard segfault. **/
	if (search_data == NULL)
	    {
	    fprintf(stderr, "Warning: Call to ci_FreeSearchData(NULL);\n");
	    return;
	    }
	ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	
	/** Free attribute data. **/
	if (search_data->Name != NULL)
	    {
	    nmSysFree(search_data->Name);
	    search_data->Name = NULL;
	    }
	
	/** Free computed data. **/
	if (search_data->Pairs != NULL)
	    {
	    for (unsigned int i = 0; i < search_data->nPairs; i++)
		{
		nmFree(search_data->Pairs[i], sizeof(Pair));
		search_data->Pairs[i] = NULL;
		}
	    nmSysFree(search_data->Pairs);
	    search_data->Pairs = NULL;
	    }
	
	/** Free the search data struct. **/
	nmFree(search_data, sizeof(SearchData));
	search_data = NULL;
    
    return;
    }


// LINK #functions
/** @param node_data A pNodeData struct, freed by this function. **/
static void
ci_FreeNodeData(pNodeData node_data)
    {
	/** Guard segfault. **/
	if (node_data == NULL)
	    {
	    fprintf(stderr, "Warning: Call to ci_FreeNodeData(NULL);\n");
	    return;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
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
    
    return;
    }

/** Frees all data in caches for all cluster driver instances. **/
static void
ci_ClearCaches(void)
    {
	/*** Free caches in reverse of the order they are created in case
	 *** cached data relies on its source during the freeing process.
	 ***/
	check(xhClearKeySafe(&ClusterDriverCaches.SearchDataCache, ci_CacheFreeSearch, NULL)); /* Failure ignored. */
	check(xhClearKeySafe(&ClusterDriverCaches.ClusterDataCache, ci_CacheFreeCluster, NULL)); /* Failure ignored. */
	check(xhClearKeySafe(&ClusterDriverCaches.SourceDataCache, ci_CacheFreeSourceData, NULL)); /* Failure ignored. */
    
    return;
    }


/** ================ Deep Size Computation Functions ================ **/
/** ANCHOR[id=sizing] **/
// LINK #functions

/*** Returns the deep size of a SourceData struct, including the size of all
 *** allocated substructures.
 *** 
 *** Note: The Key field points to data managed by the caching systems, so it
 *** is ignored.
 *** 
 *** @param source_data The source data struct to be queried.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static size_t
ci_SizeOfSourceData(pSourceData source_data)
    {
	/** Guard segfaults. **/
	if (source_data == NULL)
	    {
	    fprintf(stderr, "Warning: Call to ci_SizeOfSourceData(NULL);\n");
	    return 0u;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	size_t size = 0u;
	if (source_data->Name != NULL) size += strlen(source_data->Name) * sizeof(char);
	if (source_data->SourcePath != NULL) size += strlen(source_data->SourcePath) * sizeof(char);
	if (source_data->KeyAttr != NULL) size += strlen(source_data->KeyAttr) * sizeof(char);
	if (source_data->DataAttr != NULL) size += strlen(source_data->DataAttr) * sizeof(char);
	if (source_data->Keys != NULL)
	    {
	    for (unsigned int i = 0u; i < source_data->nVectors; i++)
		size += strlen(source_data->Keys[i]) * sizeof(char);
	    size += source_data->nVectors * sizeof(char*);
	    }
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
 *** allocated substructures.
 *** 
 *** Note: The Key field points to data managed by the caching systems, so it
 *** is ignored.
 *** 
 *** @param cluster_data The cluster data struct to be queried.
 *** @param recursive Whether to recursively free subclusters.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static size_t
ci_SizeOfClusterData(pClusterData cluster_data, bool recursive)
    {
	/** Guard segfaults. **/
	if (cluster_data == NULL)
	    {
	    fprintf(stderr, "Warning: Call to ci_SizeOfClusterData(NULL, %s);\n", (recursive) ? "true" : "false");
	    return 0u;
	    }
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	
	size_t size = 0u;
	if (cluster_data->Name != NULL) size += strlen(cluster_data->Name) * sizeof(char);
	if (cluster_data->Clusters != NULL)
	    {
	    const unsigned int nVectors = cluster_data->SourceData->nVectors;
	    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		size += cluster_data->Clusters[i].Size * (sizeof(char*) + sizeof(pVector));
	    size += nVectors * (sizeof(Cluster) + sizeof(double));
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
 *** allocated substructures.
 *** 
 *** Note: The Key field points to data managed by the caching systems, so it
 *** is ignored.
 *** 
 *** @param search_data The search data struct to be queried.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static size_t
ci_SizeOfSearchData(pSearchData search_data)
    {
	/** Guard segfaults. **/
	if (search_data == NULL)
	    {
	    fprintf(stderr, "Warning: Call to ci_SizeOfSearchData(NULL);\n");
	    return 0u;
	    }
	ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	
	unsigned int size = 0u;
	if (search_data->Name != NULL) size += strlen(search_data->Name) * sizeof(char);
	if (search_data->Pairs != NULL) size += search_data->nPairs * (sizeof(void*) + sizeof(Pair));
	size += sizeof(SearchData);
    
    return size;
    }


/** ================ Computation Functions ================ **/
/** ANCHOR[id=computation] **/
// LINK #functions

/*** Ensures that the fetched/computed attributes for `source_data` have been
 *** computed, fetching from the data source and computing vectors if needed.
 ***
 *** @attention - Promises that mssError() will be invoked on failure.
 *** 
 *** @param source_data The pSourceData who's attributes should be computed.
 *** @param session The current session, used to open the data source.
 *** @returns 0 if successful, or
 ***         -1 other value on failure.
 ***/
static int
ci_ComputeSourceData(pSourceData source_data, pObjSession session)
    {
    bool successful = false;
    int ret;
    pObject obj = NULL;
    pObjQuery query = NULL;
    XArray key_xarray = {0};
    XArray data_xarray = {0};
    XArray vector_xarray = {0};
    
	/** Guard segfaults. **/
	if (source_data == NULL) return -1;
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** If the vectors are already computed, we're done. **/
	if (source_data->Vectors != NULL) return 0;
	
	/** Record the date and time. **/
	if (!check(objCurrentDate(&source_data->DateComputed))) goto end_free;
	
	/** Open the source path specified by the .cluster file. **/
	obj = objOpen(session, source_data->SourcePath, OBJ_O_RDONLY, 0600, "system/directory");
	if (obj == NULL)
	    {
	    mssError(0, "Cluster", "Failed to open object driver.");
	    goto end_free;
	    }
	
	/** Generate a "query" for retrieving data. **/
	query = objOpenQuery(obj, NULL, NULL, NULL, NULL, 0);
	if (query == NULL)
	    {
	    mssError(0, "Cluster", "Failed to open query.");
	    goto end_free;
	    }
	
	/** Initialize an xarray to store the retrieved data. **/
	if (!check(xaInit(&key_xarray, 64))) goto end_free;
	if (!check(xaInit(&data_xarray, 64))) goto end_free;
	if (!check(xaInit(&vector_xarray, 64))) goto end_free;
	
	/** Fetch data and build vectors. **/
	while (true)
	    {
	    pObject entry = objQueryFetch(query, O_RDONLY);
	    if (entry == NULL) break; /* Done. */
	    ASSERTMAGIC(entry, MGK_OBJECT);
	    
	    /** Data value: Type checking. **/
	    const int data_datatype = objGetAttrType(entry, source_data->DataAttr);
	    if (data_datatype == -1)
		{
		mssError(0, "Cluster",
		    "Failed to get type for data of %uth entry.",
		    vector_xarray.nItems
		);
		goto end_free;
		}
	    if (data_datatype != DATA_T_STRING)
		{
		mssError(1, "Cluster",
		    "Type for data of %uth entry was %s instead of String:\n",
		    vector_xarray.nItems, objTypeToStr(data_datatype)
		);
		goto end_free;
		}
	    
	    /** Data value: Get value from database. **/
	    char* data;
	    ret = objGetAttrValue(entry, source_data->DataAttr, DATA_T_STRING, POD(&data));
	    if (ret != 0)
		{
		mssError(0, "Cluster",
		    "Failed to get attribute value for %uth data entry (error code: %d).",
		    vector_xarray.nItems, ret
		);
		goto end_free;
		}
	    
	    /** Skip empty strings. **/
	    if (strlen(data) == 0) continue;
	    
	    /** Convert the string to a vector. **/
	    pVector vector = ca_build_vector(data);
	    if (vector == NULL)
		{
		mssError(1, "Cluster", "Failed to build vectors for string \"%s\".", data);
		goto end_free;
		}
	    if (ca_is_empty(vector))
		{
		mssError(1, "Cluster", "Vector building for string \"%s\" produced no character pairs.", data);
		goto end_free;
		}
	    if (ca_has_no_pairs(vector))
		{
		/** Skip pVector with only a single pair of boundary characters. **/
		ca_free_vector(vector);
		continue;
		}
	    
	    
	    /** Key value: Type checking. **/
	    const int key_datatype = objGetAttrType(entry, source_data->KeyAttr);
	    if (key_datatype == -1)
		{
		mssError(0, "Cluster",
		    "Failed to get type for key on %uth entry.",
		    vector_xarray.nItems
		);
		goto end_free;
		}
	    if (key_datatype != DATA_T_STRING)
		{
		mssError(1, "Cluster",
		    "Type for key on %uth entry was %s instead of String:",
		    vector_xarray.nItems, objTypeToStr(key_datatype)
		);
		goto end_free;
		}
	    
	    /** key value: Get value from database. **/
	    char* key;
	    ret = objGetAttrValue(entry, source_data->KeyAttr, DATA_T_STRING, POD(&key));
	    if (ret != 0)
		{
		mssError(0, "Cluster",
		    "Failed to value for key on %uth entry (error code: %d).",
		    vector_xarray.nItems, ret
		);
		goto end_free;
		}
	    
	    /** Store values. **/
	    char* key_dup = check_ptr(nmSysStrdup(key));
	    if (key_dup == NULL) goto end_free;
	    char* data_dup = check_ptr(nmSysStrdup(data));
	    if (data_dup == NULL) goto end_free;
	    if (!check_neg(xaAddItem(&key_xarray, (void*)key_dup))) goto end_free;
	    if (!check_neg(xaAddItem(&data_xarray, (void*)data_dup))) goto end_free;
	    if (!check_neg(xaAddItem(&vector_xarray, (void*)vector))) goto end_free;
	    
	    /** Clean up. **/
	    check(objClose(entry)); /* Failure ignored. */
	    }
	
	source_data->nVectors = vector_xarray.nItems;
	if (source_data->nVectors == 0)
	    {
	    mssError(0, "Cluster", "Data source path did not contain any valid data:\n");
	    goto end_free;
	    }
	
	/** Trim and store keys. **/
	source_data->Keys = (char**)check_ptr(xaToArray(&key_xarray));
	if (source_data->Keys == NULL) goto end_free;
	check(xaDeInit(&key_xarray)); /* Failure ignored. */
	key_xarray.nAlloc = 0;
	
	/** Trim and store data strings. **/
	source_data->Strings = (char**)check_ptr(xaToArray(&data_xarray));
	if (source_data->Strings == NULL) goto end_free;
	check(xaDeInit(&data_xarray)); /* Failure ignored. */
	data_xarray.nAlloc = 0;
	
	/** Trim and store vectors. **/
	source_data->Vectors = (int**)check_ptr(xaToArray(&vector_xarray));
	if (source_data->Vectors == NULL) goto end_free;
	check(xaDeInit(&vector_xarray)); /* Failure ignored. */
	vector_xarray.nAlloc = 0;
	
	/** Success. **/
	successful = true;

    end_free:
	/** Print an error if the function failed. **/
	if (!successful)
	    {
	    mssError(0, "Cluster",
		"SourceData computation failed:\n"
		"  > Key Attribute: ['%s' : String]\n"
		"  > Data Attribute: ['%s' : String]\n"
		"  > Source Path: \"%s\"\n"
		"  > Driver Used: %s",
		source_data->KeyAttr,
		source_data->DataAttr,
		source_data->SourcePath,
		(obj == NULL || obj->Driver == NULL) ? "Unavailable" : obj->Driver->Name
	    );

	    /** Free computed data. **/
	    if (source_data->Keys != NULL) nmSysFree(source_data->Keys);
	    if (source_data->Strings != NULL) nmSysFree(source_data->Strings);
	    if (source_data->Vectors != NULL) nmSysFree(source_data->Vectors);
	    }

	/** Clean up xarrays. **/
	if (key_xarray.nAlloc != 0)
	    {
	    for (unsigned int i = 0u; i < vector_xarray.nItems; i++)
		{
		char* key = key_xarray.Items[i];
		if (key != NULL) nmSysFree(key);
		else break;
		}
	    check(xaDeInit(&key_xarray)); /* Failure ignored. */
	    }
	if (data_xarray.nAlloc != 0)
	    {
	    for (unsigned int i = 0u; i < data_xarray.nItems; i++)
		{
		char* str = data_xarray.Items[i];
		if (str != NULL) nmSysFree(str);
		else break;
		}
	    check(xaDeInit(&data_xarray)); /* Failure ignored. */
	    }
	if (vector_xarray.nAlloc != 0)
	    {
	    for (unsigned int i = 0u; i < vector_xarray.nItems; i++)
		{
		pVector vec = vector_xarray.Items[i];
		if (vec != NULL) ca_free_vector(vec);
		else break;
		}
	    check(xaDeInit(&vector_xarray)); /* Failure ignored. */
	    }
	
	/** Clean up query & object structs. **/
	if (query != NULL) check(objQueryClose(query)); /* Failure ignored. */
	if (obj != NULL) check(objClose(obj)); /* Failure ignored. */

	/** Return the function status code. **/
	return (successful) ? 0 : -1;
    }


// LINK #functions
/*** Ensures that the computed attributes for `cluster_data` have been
 *** computed, running the specified clustering algorithm if necessary.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure.
 *** 
 *** @param cluster_data The pClusterData who's attributes should be computed.
 *** @param node_data The current pNodeData, used to get vectors to cluster.
 *** @returns 0 if successful, or
 ***         -1 other value on failure.
 ***/
static int
ci_ComputeClusterData(pClusterData cluster_data, pNodeData node_data)
    {
    size_t clusters_size = -1;
    size_t sims_size = -1;
    
	/** Guard segfaults. **/
	if (cluster_data == NULL || node_data == NULL) return -1;
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
	/** If the clusters are already computed, we're done. **/
	if (cluster_data->Clusters != NULL) return 0;
	
	/** Make source data available. **/
	pSourceData source_data = check_ptr(node_data->SourceData);
	if (source_data == NULL)
	    {
	    mssError(1, "Cluster", "Failed to get source data for cluster computation.");
	    goto err_free;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** We need the SourceData vectors to compute clusters. **/
	pObjSession session = check_ptr(node_data->ParamList->Session);
	if (session == NULL) goto err_free;
	ASSERTMAGIC(session, MGK_OBJSESSION);
	if (ci_ComputeSourceData(source_data, session) != 0)
	    {
	    mssError(0, "Cluster", "ClusterData computation failed due to missing SourceData.");
	    goto err_free;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Record the date and time. **/
	if (!check(objCurrentDate(&cluster_data->DateComputed))) goto err_free;
	
	/** Allocate static memory for finding clusters. **/
	clusters_size = cluster_data->nClusters * sizeof(Cluster);
	sims_size = source_data->nVectors * sizeof(double);
	cluster_data->Clusters = check_ptr(nmSysMalloc(clusters_size));
	cluster_data->Sims = check_ptr(nmSysMalloc(sims_size));
	if (cluster_data->Clusters == NULL) goto err_free;
	if (cluster_data->Sims == NULL) goto err_free;
	memset(cluster_data->Clusters, 0, clusters_size);
	memset(cluster_data->Sims, 0, sims_size);
	
	/** Execute clustering. **/
	switch (cluster_data->ClusterAlgorithm)
	    {
	    case ALGORITHM_NONE:
		{
		/** Use a single cluster. **/
		/*** Note: There will only be a single cluster because `cluster_data->nClusters`
		 *** is set to 1 during parsing when the clustering algorithm is NONE.
		 ***/
		pCluster only_cluster = &cluster_data->Clusters[0];
		SETMAGIC(only_cluster, MGK_CL_CLUSTER);
		
		/** Add all data points to that cluster. **/
		only_cluster->Size = source_data->nVectors;
		only_cluster->Indexes = check_ptr(nmSysMalloc(only_cluster->Size * sizeof(int)));
		if (only_cluster->Indexes == NULL) goto err_free;
		for (unsigned int i = 0u; i < only_cluster->Size; i++)
		    only_cluster->Indexes[i] = i;
		
		break;
		}
	    
	    case ALGORITHM_SLIDING_WINDOW:
		/** Computed in each search for efficiency. **/
		memset(cluster_data->Clusters, 0, clusters_size);
		break;
	    
	    case ALGORITHM_KMEANS:
		{
		/** Check for unimplemented similarity measures. **/
		if (cluster_data->SimilarityMeasure != SIMILARITY_COSINE)
		    {
		    mssError(1, "Cluster",
			"The similarity measure \"%s\" is not implemented for 'k-means' clusters.",
			ci_SimilarityMeasureToString(cluster_data->SimilarityMeasure)
		    );
		    goto err_free;
		    }
		
		/** Allocate labels. Note: ca_kmeans() initializes labels for us. **/
		const size_t labels_size = source_data->nVectors * sizeof(unsigned int);
		unsigned int* labels = check_ptr(nmSysMalloc(labels_size));
		if (labels == NULL) goto err_free;
		
		/** Handle seed for ca_kmeans(). **/
		const bool auto_seed = (cluster_data->Seed == CI_NO_SEED);
		if (!auto_seed) srand(cluster_data->Seed);
		
		/** Run ca_kmeans(). **/
		const bool successful = check(ca_kmeans(
		    source_data->Vectors,
		    source_data->nVectors,
		    cluster_data->nClusters,
		    cluster_data->MaxIterations,
		    cluster_data->MinImprovement,
		    labels,
		    cluster_data->Sims,
		    auto_seed
		));
		if (!successful) goto err_free;
		
		/** Convert the labels into clusters. **/
		
		/** Allocate temporary xArrays for tracking the indices stored in each cluster. **/
		XArray indexes_in_cluster[cluster_data->nClusters];
		for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		    if (!check(xaInit(&indexes_in_cluster[i], 8))) goto err_free;
		
		/** Iterate through each label and add the index of the specified cluster to the xArray. **/
		for (unsigned long long i = 0llu; i < source_data->nVectors; i++)
		    if (!check_neg(xaAddItem(&indexes_in_cluster[labels[i]], (void*)i))) goto err_free;
		nmSysFree(labels); /* Free unused data. */
		
		/** Store the indices for each cluster and free the temporary xArray. **/
		for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		    {
		    pXArray indexes_in_this_cluster = &indexes_in_cluster[i];
		    pCluster cluster = &cluster_data->Clusters[i];
		    SETMAGIC(cluster, MGK_CL_CLUSTER);
		    
		    /** Store the data in the cluster. **/
		    cluster->Size = indexes_in_this_cluster->nItems;
		    if (cluster->Size == 0)
			{
			cluster->Indexes = NULL;
			continue;
			}
		    cluster->Indexes = check_ptr(nmSysMalloc(cluster->Size * sizeof(unsigned int*)));
		    if (cluster->Indexes == NULL) goto err_free;
		    for (unsigned int i = 0u; i < indexes_in_this_cluster->nItems; i++)
			{
			const unsigned long long index = (unsigned long long)indexes_in_this_cluster->Items[i];
			if (index > __UINT32_MAX__)
			    {
			    mssError(1, "Cluster",
				"How did you try to cluster more than %u data points and ci_ComputeSearchData() "
				"was the first thing to break?! Well... looks like it's time to update %s:%s to "
				"handle a larger amount of data.",
				__UINT32_MAX__, __FILE__, __LINE__
			    );
			    goto err_free;
			    }
			cluster->Indexes[i] = (unsigned int)index;
			}
		    check(xaDeInit(indexes_in_this_cluster)); /* Failure ignored. */
		    }
		
		/** k-means done. **/
		break;
		}
	    
	    default:
		mssError(1, "Cluster",
		    "Clustering algorithm \"%s\" is not implemented.",
		    ci_ClusteringAlgorithmToString(cluster_data->ClusterAlgorithm)
		);
		goto err_free;
	    }
	
	/** Success. **/
	return 0;
	
    err_free:
	if (cluster_data->Sims != NULL) nmFree(cluster_data->Sims, sims_size);
	
	if (cluster_data->Clusters != NULL)
	    {
	    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		{
		/*** NOTE: The clusters here do not need to each be freed
		 *** individually because the structs themselves are stored
		 *** directly in the cluster_data->Clusters array.
		 *** Thus, this loop only frees each cluster's content.
		 ***/
		const pCluster cluster = &cluster_data->Clusters[i];
		
		/** Skip the cluster if its data hasn't been set. **/
		if (cluster_data->Clusters[i].Magic == 0) continue;
		
		/** Free the data for the cluster. **/
		ASSERTMAGIC(cluster, MGK_CL_CLUSTER);
		if (cluster->Indexes != NULL) nmSysFree(cluster->Indexes);
		else break;
		}
	    nmFree(cluster_data->Clusters, clusters_size);
	    }
	
	mssError(0, "Cluster", "ClusterData computation failed for \"%s\".", cluster_data->Name);
	
	return -1;
    }


// LINK #functions
/*** Ensures that the computed attributes for `search_data` are computed,
 *** running the a search with the specified similarity measure if necessary.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure.
 *** 
 *** @param cluster_data The pSearchData who's attributes should be computed.
 *** @param node_data The current pNodeData, used to get vectors to cluster.
 *** @returns 0 if successful, or
 ***         -1 other value on failure.
 ***/
static int
ci_ComputeSearchData(pSearchData search_data, pNodeData node_data)
    {
    pXArray pairs = NULL;
    
	/** Guard segfaults. **/
	if (search_data == NULL || node_data == NULL) return -1;
	ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
	/** If the clusters are already computed, we're done. **/
	if (search_data->Pairs != NULL) return 0;
	
	/** We need the cluster data to be computed before we search it. **/
	pClusterData cluster_data = check_ptr(search_data->SourceCluster);
	if (cluster_data == NULL)
	    {
	    mssError(1, "Cluster", "Failed to get cluster data for search computation.");
	    goto err_free;
	    }
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	if (ci_ComputeClusterData(cluster_data, node_data) != 0)
	    {
	    mssError(0, "Cluster", "SearchData computation failed due to missing clusters.");
	    goto err_free;
	    }
	    
	/** Extract source data. **/
	pSourceData source_data = cluster_data->SourceData;
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Record the date and time. **/
	if (!check(objCurrentDate(&search_data->DateComputed))) goto err_free;
	
	/** Get the comparison function based on the similarity measure. **/
	const double (*similarity_function)(void *, void *) = ci_SimilarityMeasureToFunction(search_data->SimilarityMeasure);
	
	/** Execute the search using the specified algorithm. **/
	if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
	    {
	    /*** Note: We don't need to examine the clusters because nothing
	     ***       was computed during the clustering phase.
	     ***/
	    
	    /** Get a pointer to the data that will be used for the search. **/
	    void** data = NULL;
	    switch (search_data->SimilarityMeasure)
		{
		case SIMILARITY_COSINE: data = (void**)source_data->Vectors; break;
		case SIMILARITY_LEVENSHTEIN: data = (void**)source_data->Strings; break;
		default:
		    mssError(1, "Cluster",
			"Unknown similarity measure \"%s\".",
			ci_SimilarityMeasureToString(search_data->SimilarityMeasure)
		    );
		    goto err_free;
		}
	    
	    /** Execute sliding search. **/
	    pairs = check_ptr(ca_sliding_search(
		data,
		source_data->nVectors,
		cluster_data->MaxIterations, /* Window size. */
		similarity_function,
		search_data->Threshold,
		NULL
	    ));
	    if (pairs == NULL)
		{
		mssError(1, "Cluster",
		    "Failed to compute sliding search with %s similarity measure.",
		    ci_SimilarityMeasureToString(search_data->SimilarityMeasure)
		);
		goto err_free;
		}
	    }
	else
	    {
	    /** Initialize the pairs array with a size of double the amount of data. **/
	    const int guess_size = search_data->SourceCluster->SourceData->nVectors * 2;
	    pairs = check_ptr(xaNew(guess_size));
	    if (pairs == NULL) goto err_free;
	    
	    /** Iterate over each cluster. **/
	    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		{
		/** Extract the struct for the cluster. **/
		pCluster cluster = &cluster_data->Clusters[i];
		ASSERTMAGIC(cluster, MGK_CL_CLUSTER);
		
		/** Get a pointer to the data of the type needed for the search. **/
		void** data = NULL;
		switch (search_data->SimilarityMeasure)
		    {
		    case SIMILARITY_COSINE: data = (void**)source_data->Vectors; break;
		    case SIMILARITY_LEVENSHTEIN: data = (void**)source_data->Strings; break;
		    default:
			mssError(1, "Cluster",
			    "Unknown similarity measure \"%s\".",
			    ci_SimilarityMeasureToString(search_data->SimilarityMeasure)
			);
			goto err_free;
		    }
		
		/** Filter the data to only include values in the current cluster. **/
		void** filtered_data = data;
		bool free_filtered_data = false;
		if (cluster_data->nClusters > 1)
		    {
		    /** Allocate space. **/
		    filtered_data = check_ptr(nmSysMalloc(cluster->Size * sizeof(void*)));
		    if (filtered_data == NULL) goto err_free;
		    free_filtered_data = true;
		    
		    /** Add filtered data. **/
		    for (unsigned int i = 0u; i < cluster->Size; i++)
			filtered_data[i] = data[cluster->Indexes[i]];
		    }
		
		/** Execute complete search. **/
		const pXArray cluster_pairs = check_ptr(ca_complete_search(
		    filtered_data,
		    cluster->Size,
		    similarity_function,
		    search_data->Threshold,
		    NULL
		));
		if (free_filtered_data) nmSysFree(filtered_data);
		if (cluster_pairs == NULL)
		    {
		    mssError(1, "Cluster",
			"Failed to compute ca_complete_search() with %s similarity measure.",
			ci_SimilarityMeasureToString(search_data->SimilarityMeasure)
		    );
		    goto err_free;
		    }
		
		/** Remap the pairs to point to index into the SourceData arrays instead of filtered_data. **/
		for (unsigned int i = 0u; i < cluster_pairs->nItems; i++)
		    {
		    const pPair pair = (pPair)cluster_pairs->Items[i];
		    pair->i = cluster->Indexes[pair->i];
		    pair->j = cluster->Indexes[pair->j];
		    if (!check_neg(xaAddItem(pairs, pair))) goto err_free;
		    }
		check(xaFree(cluster_pairs)); /* Failure ignored. */
		}
	    }
	
	/** Store pairs. **/
	search_data->nPairs = pairs->nItems;
	if (pairs->nItems == 0)
	    search_data->Pairs = check_ptr(nmSysMalloc(0));
	else
	    {
	    search_data->Pairs = (pPair*)check_ptr(xaToArray(pairs));
	    if (search_data->Pairs == NULL) goto err_free;
	    check(xaFree(pairs)); /* Failure ignored. */
	    pairs = NULL;
	    }
	if (search_data->Pairs == NULL)
	    {
	    mssError(1, "Cluster", "Failed to store pair after computing search data.");
	    goto err_free;
	    }
	
	/** Success. **/
	return 0;
	
    err_free:
	if (search_data->Pairs != NULL) nmSysFree(search_data->Pairs);
	if (pairs != NULL)
	    {
	    for (unsigned int i = 0u; i < pairs->nItems; i++)
		{
		if (pairs->Items[i] != NULL) nmFree(pairs->Items[i], sizeof(pairs));
		else break;
		}
	    check(xaFree(pairs)); /* Failure ignored. */
	    }
	
	mssError(0, "Cluster", "SearchData computation failed for \"%s\".", search_data->Name);
	
	return -1;
    }


/** ================ Parameter Functions ================ **/
/** ANCHOR[id=params] **/
// LINK #functions

/*** Get the type of a parameter. Intended for use in `expSetParamFunctions()`.
 *** 
 *** @param inf_v Node data containing the list of parameters.
 *** @param attr_name The name of the requested parameter.
 *** @returns The datatype, see datatypes.h for a list of valid datatypes.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
static int
ci_GetParamType(void* inf_v, const char* attr_name)
    {
	pNodeData node_data = (pNodeData)inf_v;
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
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
/*** Get the value of a parameter. Intended for use in `expSetParamFunctions()`.
 *** 
 *** @attention - Warning: If the retrieved value is `NULL`, the pObjectData
 *** 	val is not updated, and the function returns 1, indicating `NULL`,
 *** 	similar to other Centrallix functions.
 *** 
 *** @param inf_v Node data containing the list of parameters.
 *** @param attr_name The name of the requested parameter.
 *** @param datatype The expected datatype of the parameter value.
 *** 	See datatypes.h	for a list of valid datatypes.
 *** @param val A pointer to a location where a pointer to the requested
 *** 	data should be stored. Typically, the caller creates a local variable
 *** 	to store this pointer, then passes a pointer to that local variable
 ***    so that they will have a pointer to the data.
 *** 	This buffer will not be modified unless the data is successfully
 *** 	found. If a value other than 0 is returned, the buffer is not updated.
 *** @returns 0 if successful,
 ***          1 if the variable is null,
 ***         -1 if an error occurs.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
static int
ci_GetParamValue(void* inf_v, char* attr_name, int datatype, pObjData val)
    {
	pNodeData node_data = (pNodeData)inf_v;
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
    
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
		mssError(1, "Cluster", "Type mismatch accessing parameter '%s'.", param->Name);
		return -1;
		}
	    
	    /** Return param value. **/
	    if (!check(objCopyData(&(param->Value->Data), val, datatype))) goto err;
	    return 0;
	    }
	
    err:
	mssError(1, "Cluster",
	    "Failed to get parameter ['%s' : %s]",
	    attr_name, objTypeToStr(datatype)
	);
	
	return -1;
    }

// LINK #functions
/** Not implemented. **/
static int
ci_SetParamValue(void* inf_v, char* attr_name, int datatype, pObjData val)
    {
	mssError(1, "Cluster", "SetParamValue() is not implemented because clusters are immutable.");
    
    return -1;
    }


/** ================ Driver functions ================ **/
/** ANCHOR[id=driver] **/
// LINK #functions

/*** Opens a new cluster driver instance by parsing a `.cluster` file found
 *** at the path provided in parent.
 *** 
 *** @param parent The parent of the object to be opened, including useful
 *** 	information such as the pathname, session, etc.
 *** @param mask Driver permission mask (unused).
 *** @param sys_type ? (unused)
 *** @param usr_type The object system file type being opened. Should always
 *** 	be "system/cluster" because this driver is only registered for that
 *** 	type of file.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** 
 *** @returns A pDriverData struct representing a driver instance, or
 ***          NULL if an error occurs.
 ***/
void*
clusterOpen(pObject parent, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt)
    {
    pNodeData node_data = NULL;
    pDriverData driver_data = NULL;
    
	/** Update statistics. **/
	ClusterStatistics.OpenCalls++;
	
	/** Guard segfaults. **/
	if (parent == NULL)
	    {
	    fprintf(stderr, "Warning: Call to clusterOpen(NULL, ...);\n");
	    return NULL;
	    }
	ASSERTMAGIC(parent, MGK_OBJECT);
	
	/** If CREAT and EXCL are specified, exclusively create it, failing if the file already exists. **/
	pSnNode node_struct = NULL;
	bool can_create = (parent->Mode & O_CREAT) && (parent->SubPtr == parent->Pathname->nElements);
	if (can_create && (parent->Mode & O_EXCL))
	    {
	    node_struct = snNewNode(parent->Prev, usr_type);
	    if (node_struct == NULL)
		{
		mssError(0, "Cluster", "Failed to exclusively create new node struct.");
		goto err_free;
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
	    mssError(0, "Cluster", "Failed to create node struct from provided cluster file.");
	    goto err_free;
	    }
	
	/** Magic. **/
	ASSERTMAGIC(node_struct, MGK_STNODE);
	ASSERTMAGIC(node_struct->Data, MGK_STRUCTINF);
	
	/** Parse node data from the node_struct. **/
	node_data = ci_ParseNodeData(node_struct->Data, parent);
	if (node_data == NULL)
	    {
	    mssError(0, "Cluster", "Failed to parse structure file \"%s\".", objFileName(parent));
	    goto err_free;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
	/** Allocate driver instance data. **/
	driver_data = check_ptr(nmMalloc(sizeof(DriverData)));
	if (driver_data == NULL) goto err_free;
	memset(driver_data, 0, sizeof(DriverData));
	SETMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	driver_data->NodeData = node_data;
	driver_data->NodeData->OpenCount++;
	    
	/** Detect target from path. **/
	char* target_name = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
	if (target_name == NULL)
	    {
	    /** Target found: Root **/
	    driver_data->TargetType = TARGET_NODE;
	    driver_data->TargetData = (void*)driver_data->NodeData->SourceData;
	    goto success;
	    }
	
	/** Search clusters. **/
	for (unsigned int i = 0u; i < node_data->nClusterDatas; i++)
	    {
	    pClusterData cluster_data = node_data->ClusterDatas[i];
	    ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	    
	    /** Skip clusters with the wrong name. **/
	    if (strcmp(cluster_data->Name, target_name) != 0) continue;
	    
	    /** Target found: Cluster **/
	    driver_data->TargetType = TARGET_CLUSTER;
	    
	    /** Check for sub-clusters in the path. **/
	    while (true)
		{
		/** Decend one path part deeper into the path. **/
		const char* path_part = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
		
		/** If the path does not go any deeper, we're done. **/
		if (path_part == NULL)
		    {
		    driver_data->TargetData = (void*)cluster_data;
		    break;
		    }
		
		/** Need to go deeper: Search for the requested sub-cluster. **/
		for (unsigned int i = 0u; i < cluster_data->nSubClusters; i++)
		    {
		    pClusterData sub_cluster = cluster_data->SubClusters[i];
		    if (strcmp(sub_cluster->Name, path_part) != 0) continue;
		    
		    /** Target found: Sub-cluster_data **/
		    cluster_data = sub_cluster;
		    goto continue_descent;
		    }
		    
		/** Path names sub-cluster that does not exist. **/
		mssError(1, "Cluster", "Sub-cluster \"%s\" does not exist.", path_part);
		goto err_free;
		
		continue_descent:;
		}
	    goto success;
	    }
	
	/** Search searches. **/
	for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
	    {
	    pSearchData search_data = node_data->SearchDatas[i];
	    // ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	    
	    /** Skip clusters with the wrong name. **/
	    if (strcmp(search_data->Name, target_name) != 0) continue;
	    
	    /** Target found: Search **/
	    driver_data->TargetType = TARGET_SEARCH;
	    driver_data->TargetData = (void*)search_data;
	    
	    /** Check for extra, invalid path parts. **/
	    char* extra_data = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
	    if (extra_data != NULL)
		{
		mssError(1, "Cluster", "Unknown path part %s.", extra_data);
		goto err_free;
		}
	    return (void*)driver_data; /* Success. */
	    }
	
	/** We were unable to find the requested cluster or search. **/
	mssError(1, "Cluster", "\"%s\" is not the name of a declared cluster or search.", target_name);
	
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
    err_free:
	if (node_data != NULL) ci_FreeNodeData(node_data);
	if (driver_data != NULL) nmFree(driver_data, sizeof(DriverData));
	
	mssError(0, "Cluster",
	    "Failed to open cluster file \"%s\" at: %s",
	    objFileName(parent), objFilePath(parent)
	);
	
	return NULL;
	
    success:
	return driver_data;
    }


// LINK #functions
/*** Close a cluster driver instance object, releasing any necessary memory
 *** and closing any necessary underlying resources.  However, most of that
 *** data will be cached and won't be freed unless the cache is dropped.
 *** 
 *** @param inf_v The affected driver instance.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns 0, success.
 ***/
int
clusterClose(void* inf_v, pObjTrxTree* oxt)
    {
	pDriverData driver_data = (pDriverData)inf_v;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	/** Update statistics. **/
	ClusterStatistics.CloseCalls++;
	
	/** No work needed. **/
	if (driver_data == NULL) return 0;
	
	/** Unlink the driver's node data. **/
	pNodeData node_data = check_ptr(driver_data->NodeData); /** Failure ignored. **/
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	if (node_data != NULL && --node_data->OpenCount == 0)
	    ci_FreeNodeData(driver_data->NodeData);
	
	/** Free driver data. **/
	nmFree(driver_data, sizeof(DriverData));
    
    return 0;
    }


// LINK #functions
/*** Opens a new query pointing to the first row of the data targeted by
 *** the driver instance struct.  The query has an internal index counter
 *** that starts at the first row and increments as data is fetched.
 *** 
 *** @param inf_v The driver instance to be queried.
 *** @param query The query to use on this struct. This is assumed to be
 *** 	handled elsewhere, so we don't read it here (unused).
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns The cluster query, or
 ***          NULL if an error occurs.
 ***/
void*
clusterOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pQueryData query_data = NULL;
    
	/** Get driver data. **/
	pDriverData driver_data = check_ptr(inf_v);
	if (driver_data == NULL) goto err_free;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	if (driver_data->TargetType != TARGET_SEARCH
	    && driver_data->TargetType != TARGET_CLUSTER
	    && driver_data->TargetType != TARGET_NODE)
	    {
	    /** Queries are not supported for this target type. **/
	    goto err;
	    }
	
	/** Update statistics. **/
	ClusterStatistics.OpenQueryCalls++;
	
	/** Allocate memory for the query. **/
	query_data = check_ptr(nmMalloc(sizeof(ClusterQuery)));
	if (query_data == NULL) goto err_free;
	
	/** Initialize the query. **/
	SETMAGIC(query_data, MGK_CL_QUERY_DATA);
	query_data->DriverData = (pDriverData)inf_v;
	query_data->RowIndex = 0u;
	
	return query_data;
	
    err_free:
	/** Error cleanup. **/
	if (query_data != NULL) nmFree(query_data, sizeof(ClusterQuery));
	mssError(0, "Cluster", "Failed to open query.");
	
    err:
	return NULL;
    }


// LINK #functions
/*** Get the next entry of a query as an open driver instance object.
 *** 
 *** @param qy_v A query instance, storing an internal index which is
 *** 	incremented once that data has been fetched.
 *** @param obj Unused.
 *** @param mode Unused.
 *** @param oxt Unused.
 *** @returns pDriverData that is either a cluster entry or search entry,
 *** 	pointing to a specific target index into the relevant data,
 *** 	OR NULL if all data has been fetched or an error occurs.
 ***/
void*
clusterQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pDriverData result_data = NULL;
    
	/** Unpack data into local variables. **/
	pQueryData query_data = check_ptr((pQueryData)qy_v);
	if (query_data == NULL) goto err_free;
	ASSERTMAGIC(query_data, MGK_CL_QUERY_DATA);
	pDriverData driver_data = check_ptr(query_data->DriverData);
	if (driver_data == NULL) goto err_free;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	pNodeData node_data = check_ptr(driver_data->NodeData);
	if (node_data == NULL) goto err_free;
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
    
	/** Update statistics. **/
	ClusterStatistics.FetchCalls++;
	
	/** Allocate result struct. **/
	result_data = check_ptr(nmMalloc(sizeof(DriverData)));
	if (result_data == NULL) goto err_free;
	
	/** Default initialization. **/
	SETMAGIC(result_data, MGK_CL_DRIVER_DATA);
	result_data->NodeData = driver_data->NodeData;
	result_data->TargetData = driver_data->TargetData;
	result_data->TargetType = 0;        /* Unset. */
	result_data->TargetIndex = 0;       /* Reset. */
	result_data->TargetAttrIndex = 0;   /* Reset. */
	result_data->TargetMethodIndex = 0; /* Reset. */
	
	/** Ensure that the data being fetched exists and is computed. **/
	const TargetType target_type = driver_data->TargetType;
	switch (target_type)
	    {
	    case TARGET_NODE:
		{
		unsigned int index = query_data->RowIndex++;
		
		/** Iterate over clusters. **/
		const unsigned int n_cluster_datas = node_data->nClusterDatas;
		if (index < n_cluster_datas)
		    {
		    /** Fetch a cluster. **/
		    result_data->TargetType = TARGET_CLUSTER;
		    result_data->TargetData = node_data->ClusterDatas[index];
		    break;
		    }
		else index -= n_cluster_datas;
		
		/** Iterate over searches. **/
		const unsigned int n_search_datas = node_data->nSearchDatas;
		if (index < n_search_datas)
		    {
		    /** Fetch a search. **/
		    result_data->TargetType = TARGET_SEARCH;
		    result_data->TargetData = node_data->SearchDatas[index];
		    break;
		    }
		else index -= n_search_datas;
		
		/** Iteration complete. **/
		goto done_free;
		}
	    
	    case TARGET_CLUSTER:
		{
		/** Ensure the required data is computed. **/
		pClusterData target = (pClusterData)driver_data->TargetData;
		ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		if (ci_ComputeClusterData(target, node_data) != 0)
		    {
		    mssError(0, "Cluster", "Failed to compute ClusterData for query.");
		    goto err_free;
		    }
		
		/** Stop iteration if the requested data does not exist. **/
		if (query_data->RowIndex >= target->nClusters) goto done_free;
		
		/** Set the data being fetched. **/
		result_data->TargetType = TARGET_CLUSTER_ENTRY;
		result_data->TargetIndex = query_data->RowIndex++;
		
		break;
		}
	    
	    case TARGET_SEARCH:
		{
		/** Ensure the required data is computed. **/
		pSearchData target = (pSearchData)driver_data->TargetData;
		ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		if (ci_ComputeSearchData(target, node_data) != 0)
		    {
		    mssError(0, "Cluster", "Failed to compute SearchData for query.");
		    goto err_free;
		    }
		
		/** Stop iteration if the requested data does not exist. **/
		if (query_data->RowIndex >= target->nPairs) goto done_free;
		
		/** Set the data being fetched. **/
		result_data->TargetType = TARGET_SEARCH_ENTRY;
		result_data->TargetIndex = query_data->RowIndex++;
		
		break;
		}
	    
	    case TARGET_CLUSTER_ENTRY:
	    case TARGET_SEARCH_ENTRY:
		mssError(1, "Cluster", "Querying a query result is not allowed.");
		goto err_free;
	    
	    default:
		mssError(1, "Cluster", "Unknown target type %u.", target_type);
		goto err_free;
	    }
	
	/** Add a link to the NodeData so that it isn't freed while we're using it. **/
	node_data->OpenCount++;
	
	/** Success. **/
	return result_data;

    err_free:
	mssError(0, "Cluster", "Failed to fetch query result.");
	
    done_free:
	if (result_data != NULL) nmFree(result_data, sizeof(DriverData));
	return NULL;
    }


// LINK #functions
/*** Close a cluster query instance, releasing any necessary memory and
 *** closing any necessary underlying resources.  This does not close the
 *** underlying driver instance, which must be closed with clusterClose().
 *** 
 *** @param qy_v The affected query instance.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns 0, success.
 ***/
int
clusterQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
	/** No work needed to free NULL. **/
	if (qy_v == NULL) return 0;
	
	/** Cast the query data. **/
	pQueryData query_data = qy_v;
	ASSERTMAGIC(query_data, MGK_CL_QUERY_DATA);
	
	/** Free the query data. **/
	nmFree(query_data, sizeof(ClusterQuery));
    
    return 0;
    }


// LINK #functions
/*** Get the type of a cluster driver instance attribute.
 *** 
 *** @param inf_v The driver instance.
 *** @param attr_name The name of the requested attribute.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns The datatype, see datatypes.h for a list of valid datatypes, or
 ***          -1 if an error occurs.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
int
clusterGetAttrType(void* inf_v, char* attr_name, pObjTrxTree* oxt)
    {
	/** Extract target type from driver data. **/
	pDriverData driver_data = check_ptr(inf_v);
	if (driver_data == NULL) goto err;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	const TargetType target_type = driver_data->TargetType;
	
	/** Update statistics. **/
	ClusterStatistics.GetTypeCalls++;
	
	/** Guard possible segfault. **/
	if (attr_name == NULL)
	    {
	    fprintf(stderr, "Warning: Call to clusterGetAttrType() with NULL attribute name.\n");
	    return DATA_T_UNAVAILABLE;
	    }
	
	/** Performance shortcut for frequently requested attributes: key1, key2, and sim. **/
	if (attr_name[0] == 'k' || attr_name[0] == 's') goto handle_targets;
	
	/** Types for general attributes. **/
	if (strcmp(attr_name, "name") == 0
	    || strcmp(attr_name, "annotation") == 0
	    || strcmp(attr_name,"content_type") == 0
	    || strcmp(attr_name, "inner_type") == 0
	    || strcmp(attr_name,"outer_type") == 0)
	    return DATA_T_STRING;
	if (strcmp(attr_name, "last_modification") == 0)
	    return DATA_T_DATETIME;
	if (strcmp(attr_name, "date_created") == 0
	    || strcmp(attr_name, "date_computed") == 0)
	    {
	    return (target_type == TARGET_CLUSTER
		 || target_type == TARGET_CLUSTER_ENTRY
		 || target_type == TARGET_SEARCH
		 || target_type == TARGET_SEARCH_ENTRY)
		 ? DATA_T_DATETIME     /* Target has date attr. */
		 : DATA_T_UNAVAILABLE; /* Target does not have date attr. */
	    }
	
	/** Types for specific data targets. **/
    handle_targets:
	switch (target_type)
	    {
	    case TARGET_NODE:
		if (strcmp(attr_name, "source") == 0
		    || strcmp(attr_name, "key_attr") == 0
		    || strcmp(attr_name, "data_attr") == 0)
		    return DATA_T_STRING;
		break;
	    
	    case TARGET_CLUSTER:
		if (strcmp(attr_name, "algorithm") == 0
		    || strcmp(attr_name, "similarity_measure") == 0)
		    return DATA_T_STRING;
		if (strcmp(attr_name, "num_clusters") == 0
		    || strcmp(attr_name, "max_iterations") == 0
		    || strcmp(attr_name, "seed") == 0)
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
		if (strcmp(attr_name, "key1") == 0
		    || strcmp(attr_name, "key2") == 0)
		    return DATA_T_STRING;
		if (strcmp(attr_name, "sim") == 0)
		    return DATA_T_DOUBLE;
		break;
	    
	    default:
		mssError(1, "Cluster", "Unknown target type %u.", target_type);
		return DATA_T_UNAVAILABLE;
	    }
	
	return DATA_T_UNAVAILABLE;
	
    err:
    return -1;
    }


// LINK #functions
/*** Get the value of a cluster driver instance attribute.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param attr_name The name of the requested attribute.
 *** @param datatype The expected datatype of the attribute value.
 *** 	See `datatypes.h` for a list of valid datatypes.
 *** @param val A pointer to a location where a pointer to the requested
 *** 	data should be stored.  Typically, the caller creates a local variable
 *** 	to store this pointer, then passes a pointer to that local variable
 ***    so that they will have a pointer to the data.
 *** 	This buffer will not be modified unless the data is successfully
 *** 	found.  If a value other than 0 is returned, the buffer is not updated.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns 0 if successful,
 ***         -1 if an error occurs.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
int
clusterGetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    TargetType target_type = -1;
    
	/** Extract target type from driver data. **/
	pDriverData driver_data = check_ptr(inf_v);
	if (driver_data == NULL) goto err;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	target_type = driver_data->TargetType;
    
	/** Update statistics. **/
	ClusterStatistics.GetValCalls++;
	
	/** Guard possible segfault. **/
	if (attr_name == NULL)
	    {
	    fprintf(stderr, "Warning: Call to clusterGetAttrType() with NULL attribute name.\n");
	    goto err;
	    }
	
	/** Performance shortcut for frequently requested attributes: key1, key2, and sim. **/
	if ((attr_name[0] == 'k' && datatype == DATA_T_STRING) /* key1, key2 : string */
	 || (attr_name[0] == 's' && datatype == DATA_T_DOUBLE) /* sim : double */
	) goto handle_targets;
	
	/** Type check. **/
	const int expected_datatype = clusterGetAttrType(inf_v, attr_name, oxt);
	if (expected_datatype == DATA_T_UNAVAILABLE) goto unknown_attribute;
	if (datatype != expected_datatype)
	    {
	    mssError(1, "Cluster",
		"Type mismatch: Accessing attribute ['%s' : %s] as type %s.",
		attr_name, objTypeToStr(expected_datatype), objTypeToStr(datatype)
	    );
	    return -1;
	    }
	
	/** Handle name. **/
	if (strcmp(attr_name, "name") == 0)
	    {
	    ClusterStatistics.GetValCalls_name++;
	    switch (target_type)
		{
		case TARGET_NODE:
		    {
		    pSourceData source_data = check_ptr(driver_data->TargetData);
		    if (source_data == NULL) goto err;
		    ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
		    val->String = source_data->Name;
		    break;
		    }
		
		case TARGET_CLUSTER:
		case TARGET_CLUSTER_ENTRY:
		    {
		    pClusterData cluster_data = check_ptr(driver_data->TargetData);
		    if (cluster_data == NULL) goto err;
		    ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
		    val->String = cluster_data->Name;
		    break;
		    }
		
		case TARGET_SEARCH:
		case TARGET_SEARCH_ENTRY:
		    {
		    pSearchData search_data = check_ptr(driver_data->TargetData);
		    if (search_data == NULL) goto err;
		    ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
		    val->String = search_data->Name;
		    break;
		    }
		
		default:
		    mssError(1, "Cluster", "Unknown target type %u.", target_type);
		    return -1;
		}
	    
	    return 0;
	    }
	
	/** Handle annotation. **/
	if (strcmp(attr_name, "annotation") == 0)
	    {
	    switch (target_type)
		{
		case TARGET_NODE: val->String = "Clustering driver."; break;
		case TARGET_CLUSTER: val->String = "Clustering driver: Cluster."; break;
		case TARGET_CLUSTER_ENTRY: val->String = "Clustering driver: Cluster Entry."; break;
		case TARGET_SEARCH: val->String = "Clustering driver: Search."; break;
		case TARGET_SEARCH_ENTRY: val->String = "Clustering driver: Cluster Entry."; break;
		
		default:
		    mssError(1, "Cluster", "Unknown target type %u.", target_type);
		    return -1;
		}
	    return 0;
	    }
	
	/** Handle various types. **/
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
	if (strcmp(attr_name, "internal_type") == 0)
	    {
	    switch (target_type)
		{
		case TARGET_NODE:          val->String = "system/cluster"; break;
		case TARGET_CLUSTER:       val->String = "cluster/cluster"; break;
		case TARGET_CLUSTER_ENTRY: val->String = "cluster/entry"; break;
		case TARGET_SEARCH:        val->String = "cluster/search"; break;
		case TARGET_SEARCH_ENTRY:  val->String = "search/entry"; break;
		default:
		    mssError(1, "Cluster", "Unknown target type %u.", target_type);
		    return -1;
		}
	    
	    return 0;
	    }
	
	/** Last modification is not implemented. **/
	if (strcmp(attr_name, "last_modification") == 0) 
	    {
	    if (target_type == TARGET_CLUSTER
		|| target_type == TARGET_CLUSTER_ENTRY
		|| target_type == TARGET_SEARCH
		|| target_type == TARGET_SEARCH_ENTRY)
		goto date_computed;
	    else return 1; /* null */
	    }
	
	/** Handle date_created. **/
	if (strcmp(attr_name, "date_created") == 0)
	    {
	    switch (target_type)
		{
		case TARGET_NODE:
		    /** Attribute is not defined for this target type. **/
		    return -1;
		
		case TARGET_CLUSTER:
		case TARGET_CLUSTER_ENTRY:
		    {
		    pClusterData cluster_data = check_ptr(driver_data->TargetData);
		    if (cluster_data == NULL) goto err;
		    ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
		    if (cluster_data->DateCreated.Value == 0) return 1; /* DateCreated not set: return null - should never occur */
		    else val->DateTime = &cluster_data->DateCreated;
		    return 0;
		    }
		
		case TARGET_SEARCH:
		case TARGET_SEARCH_ENTRY:
		    {
		    pSearchData search_data = check_ptr(driver_data->TargetData);
		    if (search_data == NULL) goto err;
		    ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
		    if (search_data->DateCreated.Value == 0) return 1; /* DateCreated not set: return null - should never occur */
		    else val->DateTime = &search_data->DateCreated;
		    return 0;
		    }
		}
	    return -1;
	    }
	
	/** Handle date_computed. **/
	if (strcmp(attr_name, "date_computed") == 0)
	    {
    date_computed:
	    switch (target_type)
		{
		case TARGET_NODE:
		    /** Attribute is not defined for this target type. **/
		    return -1;
		
		case TARGET_CLUSTER:
		case TARGET_CLUSTER_ENTRY:
		    {
		    pClusterData target = check_ptr((pClusterData)driver_data->TargetData);
		    if (target == NULL) goto err;
		    ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		    if (target->DateComputed.Value == 0) return 1; /* DateComputed not set: return null */
		    else val->DateTime = &target->DateComputed;
		    return 0;
		    }
		
		case TARGET_SEARCH:
		case TARGET_SEARCH_ENTRY:
		    {
		    pSearchData target = check_ptr((pSearchData)driver_data->TargetData);
		    if (target == NULL) goto err;
		    ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		    if (target->DateComputed.Value == 0) return 1; /* DateComputed not set: return null */
		    else val->DateTime = &target->DateComputed;
		    return 0;
		    }
		}
	    
	    /** Default: Unknown type. **/
	    mssError(1, "Cluster", "Unknown target type %u.", target_type);
	    return -1;
	    }
	
	/** Handle attributes for specific data targets. **/
    handle_targets:
	switch (target_type)
	    {
	    case TARGET_NODE:
		{
		pSourceData source_data = check_ptr(driver_data->TargetData);
		if (source_data == NULL) goto err;
		ASSERTMAGIC(source_data, MGK_CL_SEARCH_DATA);
		
		if (strcmp(attr_name, "source") == 0)
		    {
		    val->String = source_data->SourcePath;
		    return 0;
		    }
		if (strcmp(attr_name, "key_attr") == 0)
		    {
		    val->String = source_data->KeyAttr;
		    return 0;
		    }
		if (strcmp(attr_name, "data_attr") == 0)
		    {
		    val->String = source_data->DataAttr;
		    return 0;
		    }
		break;
		}
	    
	    case TARGET_CLUSTER:
		{
		pClusterData target = check_ptr(driver_data->TargetData);
		if (target == NULL) goto err;
		ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		
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
		if (strcmp(attr_name, "seed") == 0)
		    {
		    val->Integer = target->Seed;
		    return 0;
		    }
		break;
		}
	    
	    case TARGET_SEARCH:
		{
		pSearchData target = check_ptr(driver_data->TargetData);
		if (target == NULL) goto err;
		ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		
		if (strcmp(attr_name, "source") == 0)
		    {
		    val->String = target->SourceCluster->Name;
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
		pClusterData target = check_ptr(driver_data->TargetData);
		if (target == NULL) goto err;
		ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		pCluster target_cluster = &target->Clusters[driver_data->TargetIndex];
		ASSERTMAGIC(target_cluster, MGK_CL_CLUSTER);
		
		if (strcmp(attr_name, "items") == 0)
		    {
		    /** Static variable to allow us to free the StringVecs from previous calls. **/
		    static StringVec* vec = NULL;
		    if (vec != NULL)
			{
			if (vec->Strings != NULL) nmSysFree(vec->Strings);
			nmFree(vec, sizeof(StringVec));
			}
		    
		    /** Allocate and initialize the requested data. **/
		    vec = val->StringVec = check_ptr(nmMalloc(sizeof(StringVec)));
		    if (vec == NULL) goto err;
		    memset(vec, 0, sizeof(StringVec));
		    vec->nStrings = target_cluster->Size;
		    vec->Strings = check_ptr(nmSysMalloc(target_cluster->Size * sizeof(char*)));
		    if (vec->Strings == NULL) goto err;
		    for (unsigned int i = 0u; i < target_cluster->Size; i++)
			vec->Strings[i] = target->SourceData->Strings[target_cluster->Indexes[i]];
		    
		    /** Success. **/
		    return 0;
		    }
		break;
		}
	    
	    case TARGET_SEARCH_ENTRY:
		{
		pSearchData target = check_ptr(driver_data->TargetData);
		if (target == NULL) goto err;
		ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		pPair target_dup = check_ptr(target->Pairs[driver_data->TargetIndex]);
		if (target_dup == NULL) goto err;
		
		if (strcmp(attr_name, "sim") == 0)
		    {
		    ClusterStatistics.GetValCalls_sim++;
		    val->Double = target_dup->similarity;
		    return 0;
		    }
		if (strcmp(attr_name, "key1") == 0)
		    {
		    ClusterStatistics.GetValCalls_key1++;
		    val->String = target->SourceCluster->SourceData->Keys[target_dup->i];
		    return 0;
		    }
		if (strcmp(attr_name, "key2") == 0)
		    {
		    ClusterStatistics.GetValCalls_key2++;
		    val->String = target->SourceCluster->SourceData->Keys[target_dup->j];
		    return 0;
		    }
		break;
		}
	    
	    default:
		mssError(1, "Cluster", "Unknown target type %u.", target_type);
		return -1;
	    }
	
    unknown_attribute:
	ci_UnknownAttribute(attr_name, driver_data->TargetType);
	
    err:;
	char* name;
	clusterGetAttrValue(inf_v, "name", DATA_T_STRING, POD(&name), NULL);
	mssError(1, "Cluster",
	    "Failed to get attribute for cluster object %s (target type: %u, \"%s\").",
	    driver_data->NodeData->SourceData->Name, target_type, name
	);
    
    return -1;
    }


// LINK #functions
/*** Create a new presentation hints object, describing this attribute on the
 *** provided cluster driver instance.
 *** 
 *** Note: Failures from nmSysStrdup() and several others are ignored because
 *** 	the worst case scenario is that the attributes are set to null, which
 *** 	will cause them to be ignored. I consider that to be better than than
 *** 	throwing an error that could unnecessarily disrupt normal usage.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param attr_name The name of the requested attribute.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns A presentation hints object, if successful,
 ***          NULL if an error occurs.
 ***/
pObjPresentationHints
clusterPresentationHints(void* inf_v, char* attr_name, pObjTrxTree* oxt)
    {
    pObjPresentationHints hints = NULL;
    pParamObjects tmp_list = NULL;
    
	/** Extract target type from driver data. **/
	pDriverData driver_data = check_ptr(inf_v);
	if (driver_data == NULL) goto err_free;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	const TargetType target_type = driver_data->TargetType;
    
	/** Malloc presentation hints struct. **/
	hints = check_ptr(nmMalloc(sizeof(ObjPresentationHints)));
	if (hints == NULL) goto err_free;
	memset(hints, 0, sizeof(ObjPresentationHints));
	
	/** Hints that are the same for all attributes. **/
	hints->GroupID = -1;
	hints->VisualLength2 = 1;
	hints->Style     |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
	hints->StyleMask |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
	
	/** Temporary param list for compiling expressions. **/
	tmp_list = check_ptr(expCreateParamList());
	if (hints == NULL) goto err_free;
	
	/** Search for the requested attribute through attributes common to all instances. **/
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
	
	/** Handle date created and date computed. */
	if (strcmp(attr_name, "date_created") == 0
	    || strcmp(attr_name, "date_computed") == 0)
	    {
	    if (target_type == TARGET_CLUSTER
		|| target_type == TARGET_CLUSTER_ENTRY
		|| target_type == TARGET_SEARCH
		|| target_type == TARGET_SEARCH_ENTRY)
		{
		hints->Length = 24;
		hints->VisualLength = 20;
		hints->Format = check_ptr(nmSysStrdup("datetime")); /* Failure ignored. */
		goto end;
		}
	    else goto unknown_attribute;
	    }
	
	/** Search by target type. **/
	switch (target_type)
	    {
	    case TARGET_NODE:
		if (strcmp(attr_name, "source") == 0)
		    {
		    hints->Length = _PC_PATH_MAX;
		    hints->VisualLength = 64;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Source Path")); /* Failure ignored. */
		    goto end;
		    }
		if (strcmp(attr_name, "key_attr") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Key Attribute Name")); /* Failure ignored. */
		    goto end;
		    }
		if (strcmp(attr_name, "data_attr") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Data Attribute Name")); /* Failure ignored. */
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
		    hints->FriendlyName = check_ptr(nmSysStrdup("Number of Clusters")); /* Failure ignored. */
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
		    hints->FriendlyName = check_ptr(nmSysStrdup("Minimum Improvement Threshold")); /* Failure ignored. */
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
		    hints->FriendlyName = check_ptr(nmSysStrdup("Maximum Iterations")); /* Failure ignored. */
		    goto end;
		    }
		if (strcmp(attr_name, "algorithm") == 0)
		    {
		    /** Enum values. **/
		    check(xaInit(&(hints->EnumList), N_CLUSTERING_ALGORITHMS)); /* Failure ignored. */
		    for (unsigned int i = 0u; i < N_CLUSTERING_ALGORITHMS; i++)
			check_neg(xaAddItem(&(hints->EnumList), &ALL_CLUSTERING_ALGORITHMS[i])); /* Failure ignored. */
		    
		    /** Min and max values. **/
		    hints->MinValue = expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    char buf[8];
		    snprintf(buf, sizeof(buf), "%d", N_CLUSTERING_ALGORITHMS);
		    hints->MaxValue = expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    
		    /** Display flags. **/
		    hints->Style     |= OBJ_PH_STYLE_BUTTONS;
		    hints->StyleMask |= OBJ_PH_STYLE_BUTTONS;
		    
		    /** Other hints. **/
		    hints->Length = 24;
		    hints->VisualLength = 20;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Clustering Algorithm")); /* Failure ignored. */
		    goto end;
		    }
		/** Fall-through: Start of overlapping region. **/
	    
	    case TARGET_SEARCH:
		if (strcmp(attr_name, "similarity_measure") == 0)
		    {
		    /** Enum values. **/
		    check(xaInit(&(hints->EnumList), N_SIMILARITY_MEASURES)); /* Failure ignored. */
		    for (unsigned int i = 0u; i < N_SIMILARITY_MEASURES; i++)
			check_neg(xaAddItem(&(hints->EnumList), &ALL_SIMILARITY_MEASURES[i])); /* Failure ignored. */
			
		    /** Display flags. **/
		    hints->Style     |= OBJ_PH_STYLE_BUTTONS;
		    hints->StyleMask |= OBJ_PH_STYLE_BUTTONS;
		    
		    /** Min and max values. **/
		    hints->MinValue = expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    char buf[8];
		    snprintf(buf, sizeof(buf), "%d", N_SIMILARITY_MEASURES);
		    hints->MaxValue = expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    
		    /** Other hints. **/
		    hints->Length = 32;
		    hints->VisualLength = 20;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Similarity Measure")); /* Failure ignored. */
		    goto end;
		    }
		
		/** End of overlapping region. **/
		if (target_type == TARGET_CLUSTER) break;
		
		if (strcmp(attr_name, "source") == 0)
		    {
		    hints->Length = 64;
		    hints->VisualLength = 32;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Source Cluster Name")); /* Failure ignored. */
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
		    hints->FriendlyName = check_ptr(nmSysStrdup("Similarity Threshold")); /* Failure ignored. */
		    goto end;
		    }
		break;
	    
	    case TARGET_CLUSTER_ENTRY:
		{
		/** Unused. **/
		// pClusterData target = check_ptr(driver_data->TargetData);
		// if (target == NULL) goto err_free;
		// ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		
		if (strcmp(attr_name, "items") == 0)
		    {
		    /** Other hints. **/
		    hints->Length = 65536;
		    hints->VisualLength = 256;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Cluster Data")); /* Failure ignored. */
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
		    hints->FriendlyName = check_ptr(nmSysStrdup("Similarity")); /* Failure ignored. */
		    goto end;
		    }
		break;
		}
	    
	    case TARGET_SEARCH_ENTRY:
		{
		/** Unused. **/
		// pSearchData target = check_ptr(driver_data->TargetData);
		// if (target == NULL) goto err_free;
		// ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		
		if (strcmp(attr_name, "key1") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Key 1")); /* Failure ignored. */
		    goto end;
		    }
		if (strcmp(attr_name, "key2") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = check_ptr(nmSysStrdup("Key 2")); /* Failure ignored. */
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
		    hints->FriendlyName = check_ptr(nmSysStrdup("Similarity")); /* Failure ignored. */
		    goto end;
		    }
		break;
		}
	    
	    default:
		mssError(1, "Cluster", "Unknown target type %u.", target_type);
		goto err_free;
	    }
	
    unknown_attribute:
	ci_UnknownAttribute(attr_name, driver_data->TargetType);
	
    err_free:
	/** Error cleanup. **/
	if (hints != NULL) nmFree(hints, sizeof(ObjPresentationHints));
	hints = NULL;
	
	/** Construct the clearest error message that we can. **/
	char* name = NULL;
	char* internal_type = NULL;
	check(clusterGetAttrValue(inf_v, "name", DATA_T_STRING, POD(&name), NULL)); /* Failure ignored. */
	check(clusterGetAttrValue(inf_v, "internal_type", DATA_T_STRING, POD(&internal_type), NULL)); /* Failure ignored. */
	mssError(0, "Cluster",
	    "Failed to get presentation hints for object '%s' : \"%s\".",
	    name, internal_type
	);
	
    end:
	if (tmp_list != NULL) check(expFreeParamList(tmp_list)); /* Failure ignored. */
    
    return hints;
    }


// LINK #functions
/*** Returns the name of the first attribute that one can get from
 *** this driver instance (using `GetAttrType()` and `GetAttrValue()`).
 *** Resets the internal variable (`TargetAttrIndex`) used to maintain
 *** iteration state for `clusterGetNextAttr()`.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns The name of the first attribute.
 ***/
char*
clusterGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
	pDriverData driver_data = (pDriverData)inf_v;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	driver_data->TargetAttrIndex = 0u;
    
    return clusterGetNextAttr(inf_v, oxt);
    }


// LINK #functions
/*** Returns the name of the next attribute that one can get from
 *** this driver instance (using GetAttrType() and GetAttrValue()).
 *** Uses an internal variable (TargetAttrIndex) used to maintain
 *** the state of this iteration over repeated calls.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns The name of the next attribute.
 ***/
char*
clusterGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
	pDriverData driver_data = (pDriverData)inf_v;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
    
	const unsigned int i = driver_data->TargetAttrIndex++;
	switch (driver_data->TargetType)
	    {
	    case TARGET_NODE:          return (i < N_ROOT_ATTRS) ? ROOT_ATTRS[i] : NULL;
	    case TARGET_CLUSTER:       return (i < N_CLUSTER_ATTRS) ? CLUSTER_ATTRS[i] : NULL;
	    case TARGET_SEARCH:        return (i < N_SEARCH_ATTRS) ? SEARCH_ATTRS[i] : NULL;
	    case TARGET_CLUSTER_ENTRY: return (i < N_CLUSTER_ENTRY_ATTRS) ? CLUSTER_ENTRY_ATTRS[i] : NULL;
	    case TARGET_SEARCH_ENTRY:  return (i < N_SEARCH_ENTRY_ATTRS) ? SEARCH_ENTRY_ATTRS[i] : NULL;
	    default:
		mssError(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
		return NULL;
	    }
    
    return NULL; /* Unreachable. */
    }


// LINK #functions
/*** Get the capabilities of the driver instance object.
 *** 
 *** @param inf_v The driver instance to be checked.
 *** @param info The struct to be populated with driver flags.
 *** @returns 0 if successful,
 ***         -1 if the driver is an unimplemented type (should never happen).
 ***/
int
clusterInfo(void* inf_v, pObjectInfo info)
    {
	pDriverData driver_data = (pDriverData)inf_v;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	pNodeData node_data = (pNodeData)driver_data->NodeData;
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
    
	/** Reset flags buffer. **/
	info->Flags = 0;
	
	/** Disallow unsupported functionality. **/
	info->Flags |= OBJ_INFO_F_CANT_ADD_ATTR;
	info->Flags |= OBJ_INFO_F_CANT_HAVE_CONTENT;
	info->Flags |= OBJ_INFO_F_NO_CONTENT;
	
	switch (driver_data->TargetType)
	    {
	    case TARGET_NODE:
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
		if (search_data->Pairs != NULL)
		    {
		    info->nSubobjects = search_data->nPairs;
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
		mssError(1, "Cluster", "Unknown target type %u.", driver_data->TargetType);
		goto err;
	    }
	
	return 0;
	
    err:
	mssError(0, "Cluster", "Failed execute get info.");
	return -1;
    }


/** ================ Method Execution Functions ================ **/
/** ANCHOR[id=method] **/
// LINK #functions

/*** Returns the name of the first method that one can execute from
 *** this driver instance (using `clusterExecuteMethod()`). Resets the
 *** internal variable (`TargetMethodIndex`) used to maintain iteration
 *** state for `clusterGetNextMethod()`.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns The name of the first method.
 ***/
char*
clusterGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
	pDriverData driver_data = (pDriverData)inf_v;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	driver_data->TargetMethodIndex = 0u;
    
    return clusterGetNextMethod(inf_v, oxt);
    }


// LINK #functions
/*** Returns the name of the next method that one can get from
 *** this driver instance (using `GetAttrType()` and `GetAttrValue()`).
 *** Uses an internal variable (`TargetMethodIndex`) used to maintain
 *** the state of this iteration over repeated calls.
 *** 
 *** @param inf_v The driver instance to be read.
 *** @param oxt Unused.
 *** @returns The name of the next method.
 ***/
char*
clusterGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
	pDriverData driver_data = (pDriverData)inf_v;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
    
    return METHOD_NAMES[driver_data->TargetMethodIndex++];
    }


// LINK #functions
/** Intended for use in `xhForEach()`. **/
static int
ci_PrintEntry(pXHashEntry entry, void* arg)
    {
	/** Extract entry. **/
	char* key = entry->Key;
	void* data = entry->Data;
	
	/** Extract args. **/
	void** args = (void**)arg;
	unsigned int* type_id_ptr     = (unsigned int*)args[0];
	unsigned int* total_bytes_ptr = (unsigned int*)args[1];
	unsigned long long* less_ptr  = (unsigned long long*)args[2];
	char* path = (char*)args[3];
	
	/** If a path is provided, check that it matches the start of the key. **/
	if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return 0;
	
	/** Handle type. **/
	char* type;
	char* name;
	size_t bytes;
	switch (*type_id_ptr)
	    {
	    case 1u:
		{
		pSourceData source_data = (pSourceData)data;
		
		/** Compute size. **/
		bytes = ci_SizeOfSourceData(source_data);
		
		/** If less is specified, skip uncomputed source. **/
		if (*less_ptr > 0llu && source_data->Vectors == NULL) goto no_print;
		
		/** Compute printing information. **/
		type = "Source";
		name = source_data->Name;
		break;
		}
	    case 2u:
		{
		pClusterData cluster_data = (pClusterData)data;
		
		/** Compute size. **/
		bytes = ci_SizeOfClusterData(cluster_data, false);
		
		/** If less is specified, skip uncomputed source. **/
		if (*less_ptr > 0llu && cluster_data->Clusters == NULL) goto no_print;
		
		/** Compute printing information. **/
		type = "Cluster";
		name = cluster_data->Name;
		break;
		}
	    case 3u:
		{
		pSearchData search_data = (pSearchData)data;
		
		/** Compute size. **/
		bytes = ci_SizeOfSearchData(search_data);
		
		/** If less is specified, skip uncomputed source. **/
		if (*less_ptr > 0llu && search_data->Pairs == NULL) goto no_print;
		
		/** Compute printing information. **/
		type = "Search";
		name = search_data->Name;
		break;
		}
	    default:
		mssError(0, "Cluster", "Unknown type_id %u.", *type_id_ptr);
		return -1;
	    }
	
	/** Print the cache entry data. **/
	char buf[12];
	snprint_bytes(buf, sizeof(buf), bytes);
	printf("%-8s %-16s %-12s \"%s\"\n", type, name, buf, key);
	goto increment_total;
	
    no_print:
	(*less_ptr)++;
	
    increment_total:
	*total_bytes_ptr += bytes;
    
    return 0;
    }


// LINK #functions
/** Intended for use in `xhClearKeySafe()`. **/
static void
ci_CacheFreeSourceData(pXHashEntry entry, void* path)
    {
	/** Extract hash entry. **/
	char* key = entry->Key;
	pSourceData source_data = (pSourceData)entry->Data;
	
	/** If a path is provided, check that it matches the start of the key. **/
	if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return;
	
	/** Free data. **/
	ci_FreeSourceData(source_data);
	nmSysFree(key);
    
    return;
    }


// LINK #functions
/** Intended for use in `xhClearKeySafe()`. **/
static void
ci_CacheFreeCluster(pXHashEntry entry, void* path)
    {
	/** Extract hash entry. **/
	char* key = entry->Key;
	pClusterData cluster_data = (pClusterData)entry->Data;
	
	/** If a path is provided, check that it matches the start of the key. **/
	if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return;
	
	/** Free data. **/
	ci_FreeClusterData(cluster_data, false);
	nmSysFree(key);
    
    return;
    }


// LINK #functions
/** Intended for use in `xhClearKeySafe()`. **/
static void
ci_CacheFreeSearch(pXHashEntry entry, void* path)
    {
	/** Extract hash entry. **/
	char* key = entry->Key;
	pSearchData search_data = (pSearchData)entry->Data;
	
	/** If a path is provided, check that it matches the start of the key. **/
	if (path != NULL && strncmp(key, (char*)path, strlen((char*)path)) != 0) return;
	
	/** Free data. **/
	ci_FreeSearchData(search_data);
	nmSysFree(key);
    
    return;
    }


// LINK #functions
/*** Executes a method with the given name.
 *** 
 *** @param inf_v The affected driver instance.
 *** @param method_name The name of the method.
 *** @param param A possibly optional param passed to the method.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 ***/
int
clusterExecuteMethod(void* inf_v, char* method_name, pObjData param, pObjTrxTree* oxt)
    {
	pDriverData driver_data = (pDriverData)inf_v;
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
    
	/** Cache management method. **/
	if (strcmp(method_name, "cache") == 0)
	    {
	    char* path = NULL;
	    
	    /** Second parameter is required. **/
	    if (param->String == NULL)
		{
		mssError(1, "Cluster",
		    "[param : \"show\" | \"show_less\" | \"show_all\" | \"drop_all\"] is required for the cache method."
		);
		goto err;
		}
	    
	    /** 'show' and 'show_all'. **/
	    bool show = false;
	    unsigned long long skip_uncomputed = 0llu;
	    if (strcmp(param->String, "show_less") == 0)
		/** Specify show_less to skip uncomputed caches. **/
		skip_uncomputed = 1ull;
	    if (skip_uncomputed == 1ull || strcmp(param->String, "show") == 0)
		{
		show = true;
		path = objFilePath(driver_data->NodeData->Parent);
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
		    (void*[]){&i, &source_bytes, (void*)&skip_uncomputed, path}
		));
		i++;
		failed |= !check(xhForEach(
		    &ClusterDriverCaches.ClusterDataCache,
		    ci_PrintEntry,
		    (void*[]){&i, &cluster_bytes, (void*)&skip_uncomputed, path}
		));
		i++;
		failed |= !check(xhForEach(
		    &ClusterDriverCaches.SearchDataCache,
		    ci_PrintEntry,
		    (void*[]){&i, &search_bytes, (void*)&skip_uncomputed, path}
		));
		if (failed)
		    {
		    mssError(0, "Cluster", "Unexpected error occurred while showing caches.");
		    ret = -1;
		    }
		    
		/** Precomputations. **/
		unsigned int total_caches = 0u
		    + (unsigned int)ClusterDriverCaches.SourceDataCache.nItems
		    + (unsigned int)ClusterDriverCaches.ClusterDataCache.nItems
		    + (unsigned int)ClusterDriverCaches.SearchDataCache.nItems;
		if (total_caches <= skip_uncomputed) printf("All caches skipped, nothing to show...\n");
		
		/** Print stats. **/
		char buf[16];
		printf("\nCache Stats:\n");
		printf("%-8s %-4s %-12s\n", "", "#", "Total Size");
		printf("%-8s %-4d %-12s\n", "Source", ClusterDriverCaches.SourceDataCache.nItems, snprint_bytes(buf, sizeof(buf), source_bytes));
		printf("%-8s %-4d %-12s\n", "Cluster", ClusterDriverCaches.ClusterDataCache.nItems, snprint_bytes(buf, sizeof(buf), cluster_bytes));
		printf("%-8s %-4d %-12s\n", "Search", ClusterDriverCaches.SearchDataCache.nItems, snprint_bytes(buf, sizeof(buf), search_bytes));
		printf("%-8s %-4d %-12s\n\n", "Total", total_caches, snprint_bytes(buf, sizeof(buf), source_bytes + cluster_bytes + search_bytes));
		
		/** Print skip stats (if anything was skipped.) **/
		if (skip_uncomputed > 0llu) printf("Skipped %llu uncomputed caches.\n\n", skip_uncomputed - 1llu);
		
		return ret;
		}
	    
	    /** 'drop_all'. **/
	    if (strcmp(param->String, "drop_all") == 0)
		{
		ci_ClearCaches();
		printf("Dropped cache for all cluster files.\n");
		return 0;
		}
	    
	    /** Unknown parameter. **/
	    mssError(1, "Cluster",
		"Expected [param : \"show\" | \"show_less\" | \"show_all\" | \"drop_all\"] for the cache method, but got: \"%s\"",
		param->String
	    );
	    goto err;
	    }
	
	if (strcmp(method_name, "stat") == 0)
	    {
	    char buf[12];
	    printf("Cluster Driver Statistics:\n");
	    printf("  Stat Name         %12s\n", "Value");
	    printf("  OpenCalls         %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.OpenCalls));
	    printf("  OpenQueryCalls    %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.OpenQueryCalls));
	    printf("  FetchCalls        %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.FetchCalls));
	    printf("  CloseCalls        %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.CloseCalls));
	    printf("  GetTypeCalls      %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.GetTypeCalls));
	    printf("  GetValCalls       %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.GetValCalls));
	    printf("  GetValCalls_name  %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.GetValCalls_name));
	    printf("  GetValCalls_key1  %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.GetValCalls_key1));
	    printf("  GetValCalls_key2  %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.GetValCalls_key2));
	    printf("  GetValCalls_sim   %12s\n", snprint_commas_llu(buf, sizeof(buf), ClusterStatistics.GetValCalls_sim));
	    printf("\n");
	    
	    nmStats();
	    
	    return 0;
	    }
	
	/** Unknown parameter. **/
	mssError(1, "Cluster", "Unknown command: \"%s\"", method_name);
	
	/** Attempt to give hint. **/
	unsigned int n_methods = 0;
	while (METHOD_NAMES[n_methods] != NULL) n_methods++;
	if (ci_TryHint(method_name, METHOD_NAMES, n_methods));
	
    err:
	mssError(0, "Cluster", "Failed execute command.");
	
	return -1;
    }


/** ================ Unimplemented Functions ================ **/
/** ANCHOR[id=unimplemented] **/
// LINK #functions

/** Not implemented. **/
int
clusterCreate(pObject obj, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterCreate() is not implemented.");
    
    return -ENOSYS;
    }

/** Not implemented. **/
int
clusterDelete(pObject obj, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterDelete() is not implemented.");
    
    return -1;
    }

/** Not implemented. **/
int
clusterDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterDeleteObj() is not implemented.");
    
    return -1;
    }

/** Not implemented. **/
int
clusterRead(void* inf_v, char* buffer, int max_cnt, int offset, int flags, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterRead() not implemented.");
	fprintf(stderr, "HINT: Use queries instead, (e.g. clusterOpenQuery()).\n");
    
    return -1;
    }

/** Not implemented. **/
int
clusterWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterWrite() not implemented because clusters are immutable.");
    
    return -1;
    }

/** Not implemented. **/
int
clusterSetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterSetAttrValue() not implemented because clusters are immutable.");
    
    return -1;
    }

/** Not implemented. **/
int
clusterAddAttr(void* inf_v, char* attr_name, int type, pObjData val, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterAddAttr() not implemented because clusters are immutable.");
    
    return -1;
    }

/** Not implemented. **/
void*
clusterOpenAttr(void* inf_v, char* attr_name, int mode, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterOpenAttr() not implemented.");
    
    return NULL;
    }

/** Not implemented. **/
int
clusterCommit(void* inf_v, pObjTrxTree* oxt)
    {
	mssError(1, "Cluster", "clusterCommit() not implemented because clusters are immutable.");
    
    return 0;
    }


// LINK #functions
/*** Initialize the driver, including:
 *** - Registering the driver with the object system.
 *** - Registering structs with newmalloc for debugging.
 *** - Initializing global data needed for the driver.
 *** 
 *** @returns 0 if successful, or
 ***         -1 if an error occurs.
 ***/
int
clusterInitialize(void)
    {
	/** Allocate the driver. **/
	pObjDriver drv = (pObjDriver)check_ptr(nmMalloc(sizeof(ObjDriver)));
	if (drv == NULL) goto err_free;
	memset(drv, 0, sizeof(ObjDriver));
	
	/** Initialize caches. **/
	// memset(&ClusterDriverCaches, 0, sizeof(ClusterDriverCaches));
	if (!check(xhInit(&ClusterDriverCaches.SourceDataCache, 251, 0))) goto err_free;
	if (!check(xhInit(&ClusterDriverCaches.ClusterDataCache, 251, 0))) goto err_free;
	if (!check(xhInit(&ClusterDriverCaches.SearchDataCache, 251, 0))) goto err_free;
	
	/** Setup the structure. **/
	if (check_ptr(strcpy(drv->Name, "cluster - Clustering Driver")) == NULL) goto err_free;
	if (!check(xaInit(&drv->RootContentTypes, 1))) goto err_free;
	if (!check_neg(xaAddItem(&drv->RootContentTypes, "system/cluster"))) goto err_free;
	drv->Capabilities = 0; /* TODO: Greg - Should I indicate any capabilities? */
	
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
	
	/** Register the driver. **/
	if (!check(objRegisterDriver(drv))) goto err_free;
	
	/** Register structs used in this project with the newmalloc memory management system. **/
	nmRegister(sizeof(SourceData), "ClusterSourceData");
	nmRegister(sizeof(Cluster), "Cluster");
	nmRegister(sizeof(ClusterData), "ClusterData");
	nmRegister(sizeof(SearchData), "ClusterSearch");
	nmRegister(sizeof(NodeData), "ClusterNodeData");
	nmRegister(sizeof(DriverData), "ClusterDriverData");
	nmRegister(sizeof(ClusterQuery), "ClusterQuery");
	nmRegister(sizeof(ClusterDriverCaches), "ClusterDriverCaches");
	
	/** Success. **/
	return 0;
	
    err_free:
	/** Error cleanup. **/
	if (ClusterDriverCaches.SourceDataCache.nRows != 0) check(xhDeInit(&ClusterDriverCaches.SourceDataCache)); /* Failure ignored. **/
	if (ClusterDriverCaches.ClusterDataCache.nRows != 0) check(xhDeInit(&ClusterDriverCaches.ClusterDataCache)); /* Failure ignored. **/
	if (ClusterDriverCaches.SearchDataCache.nRows != 0) check(xhDeInit(&ClusterDriverCaches.SearchDataCache)); /* Failure ignored. **/
	if (drv != NULL)
	    {
	    if (drv->RootContentTypes.nAlloc != 0) check(xaDeInit(&drv->RootContentTypes)); /* Failure ignored. */
	    nmFree(drv, sizeof(ObjDriver));
	    }
	
	mssError(1, "Cluster", "Failed to initialize cluster driver.\n");
	
	return -1;
    }
