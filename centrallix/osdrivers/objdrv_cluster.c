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
#include "cxlib/expect.h"
#include "cxlib/magic.h"
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "cxlib/strtcpy.h"
#include "cxlib/util.h"
#include "cxlib/warn.h"
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
#define CI_DEFAULT_MAX_ITERATIONS 64
#define CI_NO_SEED 0u

/** Stringify a constant value. **/
#define CI_STRINGIFY_CONSTANT_(value) #value
#define CI_STRINGIFY_CONSTANT(value) CI_STRINGIFY_CONSTANT_(value)

/** Initial sizes. **/
#define CI_INITIAL_SOURCE_DATAS        64
#define CI_INITIAL_INFS                8
#define CI_INITIAL_SUBCLUSTERS         4
#define CI_INITIAL_POINTS_PER_CLUSTER  8
#define CI_CACHE_HASHTABLE_ROWS        251

/** Developer config values. **/
#define CI_HINT_SIMILARITY_THRESHOLD   0.25

/** ================ Enum Declarations ================ **/
/** ANCHOR[id=enums] **/

/** Enum type representing a clustering algorithm. **/
typedef unsigned char ClusterAlgorithm;
#define ALGORITHM_NULL                 ((ClusterAlgorithm)0u)
#define ALGORITHM_NONE                 ((ClusterAlgorithm)1u)
#define ALGORITHM_SLIDING_WINDOW       ((ClusterAlgorithm)2u)
#define ALGORITHM_KMEANS               ((ClusterAlgorithm)3u)
#define ALGORITHM_KMEANS_PLUS_PLUS     ((ClusterAlgorithm)4u)
#define ALGORITHM_KMEDOIDS             ((ClusterAlgorithm)5u)
#define ALGORITHM_DB_SCAN              ((ClusterAlgorithm)6u)

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
cluster_i_clusteringAlgorithmToString(ClusterAlgorithm clustering_algorithm)
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
	    default: return "unknown algorithm";
	    }
    
    return NULL; /** Unreachable. **/
    }


/** Enum type representing a similarity measurement algorithm. **/
typedef unsigned char SimilarityMeasure;
#define SIMILARITY_NULL                ((SimilarityMeasure)0u)
#define SIMILARITY_COSINE              ((SimilarityMeasure)1u)
#define SIMILARITY_LEVENSHTEIN         ((SimilarityMeasure)2u)

SimilarityMeasure ALL_SIMILARITY_MEASURES[] =
    {
    SIMILARITY_NULL,
    SIMILARITY_COSINE,
    SIMILARITY_LEVENSHTEIN,
    };
#define N_SIMILARITY_MEASURES ((unsigned int)(sizeof(ALL_SIMILARITY_MEASURES) / sizeof(ALL_SIMILARITY_MEASURES[0])))

/*** Converts a similarity measure to its string name.
 *** 
 *** @param similarity_measure The similarity measure value to convert.
 *** @returns The corresponding name.
 ***/
char*
cluster_i_similarityMeasureToString(SimilarityMeasure similarity_measure)
    {
	switch (similarity_measure)
	    {
	    case SIMILARITY_NULL: return "NULL similarity measure";
	    case SIMILARITY_COSINE: return "cosine";
	    case SIMILARITY_LEVENSHTEIN: return "levenshtein";
	    default: return "unknown similarity measure";
	    }
    
    return NULL; /** Unreachable. **/
    }

/*** Converts a similarity measure to a function pointer that points to the
 *** corresponding comparison function.  This function can be called directly
 *** or passed to functions from `clusters.c`.
 *** 
 *** @param similarity_measure The similarity measure value to be converted.
 *** @returns A similarity computation function.  This function takes two void
 *** 	pointers, representing the data to be compared, and returns a double
 *** 	representing how similar the data is from 0.0 (no similarity) to 1.0
 *** 	(identical), or NAN if an error occurs.  This function may return NULL
 *** 	if an error occurs, in which case it always calls mssError() to give
 *** 	an error message.
 ***/
pSimilarityFn
cluster_i_similarityMeasureToFunction(SimilarityMeasure similarity_measure)
    {
    switch (similarity_measure)
	{
	case SIMILARITY_COSINE: return caCosCompare;
	case SIMILARITY_LEVENSHTEIN: return caLevCompare;
	default:
	    mssError(1, "Cluster",
		"Unknown similarity measure \"%s\" (%d).",
		cluster_i_similarityMeasureToString(similarity_measure), similarity_measure
	    );
	    return NULL;
	}
    }


/*** Enum representing the type of an unknown data struct.
 *** 
 *** `0u` is reserved for a possible NULL value in the future.  However, NULL
 *** values for CIDataType are not currently allowed.
 ***/
typedef unsigned char CIDataType;
#define CI_SOURCE_DATA                 ((CIDataType)1u)
#define CI_CLUSTER_DATA                ((CIDataType)2u)
#define CI_SEARCH_DATA                 ((CIDataType)3u)

CIDataType ALL_CI_DATA_TYPES[] =
    {
    CI_SOURCE_DATA,
    CI_CLUSTER_DATA,
    CI_SEARCH_DATA,
    };
#define N_CI_DATA_TYPES ((unsigned int)(sizeof(ALL_CI_DATA_TYPES) / sizeof(ALL_CI_DATA_TYPES[0])))


/*** Enum representing the type of data targeted by the driver, set based on
 *** the path given when the driver is used to open a cluster file.
 *** 
 *** `0u` is reserved for a possible NULL value in the future.  However, NULL
 *** values for TargetType are not currently allowed.
 ***/
typedef unsigned char TargetType;
#define TARGET_NODE                    ((TargetType)1u)
#define TARGET_CLUSTER                 ((TargetType)2u)
#define TARGET_SEARCH                  ((TargetType)3u)
#define TARGET_CLUSTER_ENTRY           ((TargetType)4u)
#define TARGET_SEARCH_ENTRY            ((TargetType)5u)

TargetType ALL_TARGET_TYPES[] =
    {
    TARGET_NODE,
    TARGET_CLUSTER,
    TARGET_SEARCH,
    TARGET_CLUSTER_ENTRY,
    TARGET_SEARCH_ENTRY,
    };
#define N_TARGET_TYPES ((unsigned int)(sizeof(ALL_TARGET_TYPES) / sizeof(ALL_TARGET_TYPES[0])))

/*** Converts a target type to its string name.
 *** 
 *** @param target_type The target type value to convert.
 *** @returns The corresponding name.
 ***/
char*
cluster_i_targetTypeToString(TargetType target_type)
    {
	switch (target_type)
	    {
	    case TARGET_NODE: return "node";
	    case TARGET_CLUSTER: return "cluster";
	    case TARGET_SEARCH: return "search";
	    case TARGET_CLUSTER_ENTRY: return "cluster entry";
	    case TARGET_SEARCH_ENTRY: return "search entry";
	    default: return "unknown target type";
	    }
    
    return NULL; /** Unreachable. **/
    }


/*** Attribute name lists by TargetType.
 *** 
 *** Promises that input attributes are listed first, before computed attributes.
 ***/
char* ROOT_ATTRS[] =
    {
    "source",
    "key_attr",
    "data_attr",
    "internal_type",
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
    "window_size",
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
    "date_computed",
    "date_created",
    };
#define N_CLUSTER_ENTRY_ATTRS ((unsigned int)(sizeof(CLUSTER_ENTRY_ATTRS) / sizeof(CLUSTER_ENTRY_ATTRS[0])))
#define N_COMPUTED_CLUSTER_ENTRY_ATTRS 2u
#define N_INPUT_CLUSTER_ENTRY_ATTRS (N_CLUSTER_ENTRY_ATTRS - N_COMPUTED_CLUSTER_ENTRY_ATTRS)
char* SEARCH_ENTRY_ATTRS[] =
    {
    "key1",
    "key2",
    "sim",
    "date_computed",
    "date_created",
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

/*** Represents the data source which may have data already fetched.  Only
 *** attribute data is checked for caching.
 *** 
 *** Memory Stats:
 ***   - Padding: 0 bytes
 ***   - Total size: 88 bytes
 *** 
 *** @skip --> Attribute Data.
 *** @param Name The source name, specified in the .cluster file.
 *** @param CacheKey The key associated with this object in the SourceDataCache.
 *** @param SourcePath The path to the data source from which to retrieve data.
 *** @param KeyAttr The name of the attribute to use when getting keys from
 *** 	the driver represented by SourcePath.
 *** @param DataAttr The name of the attribute to use when getting data from
 *** 	the driver represented by SourcePath.
 *** 
 *** @skip --> Fetched/Computed Data.
 *** @param Keys The keys for each data string received from the data source,
 *** 	used when the results are queried, or NULL if the data has not been
 *** 	fetched.
 *** @param Strings The data strings to be clustered and searched, or NULL if
 *** 	they have not been fetched from the source.
 *** @param Vectors The cosine comparison vectors from the fetched data, or
 *** 	NULL if they haven't been computed yet.
 *** @param nDatas The number of keys, data strings, and vectors that have been
 *** 	fetched/computed, or 0 if no data has been fetched/computed yet.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created/initialized.
 *** @param DateComputed The date and time that the fetch/computed attributes
 *** 	were fetched/computed.
 *** 
 *** @param Magic A magic value for detecting memory corruption.
 ***/
typedef struct _SOURCE
    {
    Magic_t      Magic;
    unsigned int nDatas;
    char*        Name;
    char*        CacheKey;
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
 *** @param Indexes The data points in the cluster, as indexes into SourceData.
 *** 	Use these to access the Keys, Strings, or Vectors array fields. This
 *** 	value is NULL if `Size == 0`.
 *** @param Magic A magic value for detecting memory corruption.
 ***/
typedef struct
    {
    Magic_t       Magic;
    unsigned int  Size;
    unsigned int* Indexes;
    }
    Cluster, *pCluster;


/*** Data for each cluster object defined in the .cluster file.  Only attribute
 *** data is checked for caching.
 *** 
 *** Memory Stats:
 ***   - Padding: 2 bytes
 ***   - Total size: 104 bytes
 *** 
 *** @skip --> Attribute Data.
 *** @param Name The cluster name, specified in the .cluster file.
 *** @param CacheKey The key associated with this object in the ClusterDataCache.
 *** @param ClusterAlgorithm The clustering algorithm to be used.
 *** @param SimilarityMeasure The similarity measure used to compare items.
 *** @param nClusters The number of clusters.  1 if `algorithm == none`.
 *** @param MinImprovement The minimum amount of improvement that must be met
 *** 	each clustering iteration.  -inf represents the "max" value in the
 *** 	.cluster file.
 *** @param MaxIterations The maximum number of iterations to run clustering.
 *** @param WindowSize The size of the sliding window for sliding window
 *** 	searches.  Shares memory with MaxIterations because the sliding window
 *** 	clustering algorithm does not have any concept of "iterations".
 *** 
 *** @skip --> Relational Data. (Note: sub-clusters are not implemented.)
 *** @param nSubClusters The number of sub-clusters for this cluster.
 *** @param SubClusters A pClusterData array, NULL if `nSubClusters == 0`.
 *** @param Parent This cluster's parent.  NULL if it is not a sub-cluster.
 *** @param SourceData Pointer to the source data that this cluster uses.
 *** 
 *** @skip --> Computed Data.
 *** @param Clusters An array of length nClusters, NULL if the clusters have
 *** 	not been computed yet.
 *** @param Sims An array of nDatas elements, where index i stores the
 *** 	similarity of vector i to its assigned cluster, NULL if the clusters
 *** 	have not been computed yet.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created/initialized.
 *** @param DateComputed The date and time that the computed attributes were computed.
 *** 
 *** @param Magic A magic value for detecting memory corruption.
 ***/
typedef struct _CD
    {
    Magic_t           Magic;
    unsigned int      nClusters;
    char*             Name;
    char*             CacheKey;
    ClusterAlgorithm  ClusterAlgorithm;
    SimilarityMeasure SimilarityMeasure;
    /** 2 bytes of auto-padding. **/
    unsigned int      Seed;
    double            MinImprovement;
    union {
	unsigned int  MaxIterations;
	unsigned int  WindowSize;
    };
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
 *** @param CacheKey The key associated with this object in the SearchDataCache.
 *** @param SourceCluster The cluster from which this search is to be derived.
 *** @param SimilarityMeasure The similarity measure used to compare items.
 *** @param Threshold The minimum similarity threshold for elements to be
 *** 	included in the results of the search.
 *** 
 *** @skip --> Computed data.
 *** @param Pairs An array holding the pairs found by the search, or NULL if
 *** 	the search has not been computed yet.  The indexes stored in these
 *** 	pairs are indexes into the SourceData data arrays (access with
 *** 	`SourceCluster->SourceData->[DATA_ARRAY]`).
 *** @param nPairs The number of pairs found, or 0 if the search has not been
 *** 	computed yet.
 *** 
 *** @skip --> Time.
 *** @param DateCreated The date and time that this object was created/initialized.
 *** @param DateComputed The date and time that the computed attributes were computed.
 *** 
 *** @param Magic A magic value for detecting memory corruption.
 ***/
typedef struct _SEARCH
    {
    Magic_t           Magic;
    /** 4 bytes of auto-padding. **/
    char*             Name;
    char*             CacheKey;
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
 *** `clusterQueryFetch()`, and closed with functions like `clusterClose()`.
 *** 
 *** @param SourceData Data from the provided source.
 *** @param Params A pParam array storing the params in the .cluster file.
 *** @param nParams The number of specified params.
 *** @param ParamList A "scope" for resolving parameter values during parsing.
 *** @param ClusterDatas A pClusterData array for the clusters in the .cluster file,
 *** 	NULL if `nClusterDatas == 0`.
 *** @param nClusterDatas The number of specified clusters.
 *** @param SearchDatas A pSearchData array for the searches in the .cluster file.
 *** @param nSearchDatas The number of specified searches.
 *** @param Parent The parent object used to open this NodeData instance.
 *** @param OpenCount The number of open driver instances that are using the
 *** 	NodeData struct.  When this reaches 0, the struct should be freed.
 *** @param Magic A magic value for detecting memory corruption.
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
 *** Think of this struct like a "pointer" to specific data accessible through
 *** the pNodeData field.  This struct also tells us whether that data is
 *** guaranteed to be computed already.
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
 *** @attention - When allocating this struct, remember to increment the
 *** 	`NodeData->OpenCount` value. When freeing this struct, remember to
 *** 	decrement `NodeData->OpenCount`, freeing the NodeData struct as well
 *** 	if it reaches 0, to prevent memory leaks.
 *** 
 *** @param NodeData The associated node data struct.  Many driver struct
 *** 	instances may point to one NodeData at a time, but each driver
 *** 	instance always points to a singular NodeData struct.
 *** @param TargetType The type of data targeted (see above).
 *** @param TargetData If target type is:
 *** ```txt
 *** 	Node:                    A pointer to the SourceData struct.
 *** 	Cluster or ClusterEntry: A pointer to the targeted cluster.
 *** 	Search or SearchEntry:   A pointer to the targeted search.
 *** ```
 *** @param TargetAttrIndex An index into an attribute list (for GetNextAttr()).
 *** @param TargetMethodIndex An index into a method list (for GetNextMethod()).
 *** @param Magic A magic value for detecting memory corruption.
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
    /** 1 byte of auto-padding. **/
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
 *** @param Magic A magic value for detecting memory corruption.
 ***/
typedef struct
    {
    Magic_t        Magic;
    unsigned int   RowIndex;
    pDriverData    DriverData;
    }
    ClusterQuery, *pQueryData;


/** Global storage for driver caches. **/
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

/** Parsing Functions. **/
// LINK #parsing
static void cluster_i_giveHint(const char* hint);
static bool cluster_i_tryHint(char* value, char** valid_values, const unsigned int n_valid_values);
static void cluster_i_unknownAttribute(char* attr_name, TargetType target_type);
static ClusterAlgorithm cluster_i_parseClusteringAlgorithm(pStructInf cluster_inf, pParamObjects param_list);
static SimilarityMeasure cluster_i_parseSimilarityMeasure(pStructInf cluster_inf, pParamObjects param_list);
static pSourceData cluster_i_parseSourceData(pStructInf inf, pParamObjects param_list, char* path);
static pClusterData cluster_i_parseClusterData(pStructInf inf, pParamObjects param_list, pSourceData source_data);
static pSearchData cluster_i_parseSearchData(pStructInf inf, pNodeData node_data);
static pNodeData cluster_i_parseNodeData(pStructInf inf, pObject obj);

/** Freeing Functions. **/
// LINK #freeing
static void cluster_i_freeSourceData(pSourceData source_data);
static void cluster_i_freeClusterData(pClusterData cluster_data, bool recursive);
static void cluster_i_freeSearchData(pSearchData search_data);
static void cluster_i_freeNodeData(pNodeData node_data);
static void cluster_i_clearCaches(void);

/** Deep Size Computation Functions. **/
// LINK #sizing
static size_t cluster_i_sizeOfSourceData(pSourceData source_data);
static size_t cluster_i_sizeOfClusterData(pClusterData cluster_data, bool recursive);
static size_t cluster_i_sizeOfSearchData(pSearchData search_data);

/** Computation Functions. (Ensure data is computed.) **/
// LINK #computation
static int cluster_i_computeSourceData(pSourceData source_data, pObjSession session);
static int cluster_i_computeClusterData(pClusterData cluster_data, pNodeData node_data);
static int cluster_i_computeSearchData(pSearchData search_data, pNodeData node_data);

/** Parameter Functions. **/
// LINK #params
static int cluster_i_getParamType(void* inf_v, const char* attr_name);
static int cluster_i_getParamValue(void* inf_v, char* attr_name, int datatype, pObjData val);
static int cluster_i_setParamValue(void* inf_v, char* attr_name, int datatype, pObjData val);

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
static int cluster_i_printEntry(pXHashEntry entry, va_list args);
static void cluster_i_cacheFreeSourceData(pXHashEntry entry, void* path);
static void cluster_i_cacheFreeCluster(pXHashEntry entry, void* path);
static void cluster_i_cacheFreeSearch(pXHashEntry entry, void* path);
int clusterExecuteMethod(void* inf_v, char* method_name, pObjData param, pObjTrxTree* oxt);

/** Unimplemented Driver Functions. **/
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

/*** Format a hint to print to the user.
 *** 
 *** @param hint The text of the guess to print as a hint.
 ***/
static void cluster_i_giveHint(const char* hint)
    {
	fprintf(stderr, "  > Hint: Did you mean \"%s\"?\n", hint);
    
    return;
    }


/*** Give the user a hint when they specify an invalid string for an attribute
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
cluster_i_tryHint(char* value, char** valid_values, const unsigned int n_valid_values)
    {
	char* guess = caMostSimilar(
	    value, (void**)valid_values, n_valid_values,
	    caLevCompare, CI_HINT_SIMILARITY_THRESHOLD
	);
	if (guess == NULL) return false; /* No hint. */
	
	/** Issue hint. **/
	cluster_i_giveHint(guess);
    
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
cluster_i_unknownAttribute(char* attr_name, const TargetType target_type)
    {
	/** Display the error message. **/
	mssError(1, "Cluster", "Unknown attribute '%s'.", attr_name);
	
	/** Collect specific attributes based on target type. **/
	char** my_attrs = NULL;
	unsigned int n_my_attrs = 0u;
	switch (target_type)
	    {
	    case TARGET_NODE:          my_attrs = ROOT_ATTRS;          n_my_attrs = N_ROOT_ATTRS; break;
	    case TARGET_CLUSTER:       my_attrs = CLUSTER_ATTRS;       n_my_attrs = N_CLUSTER_ATTRS; break;
	    case TARGET_SEARCH:        my_attrs = SEARCH_ATTRS;        n_my_attrs = N_SEARCH_ATTRS; break;
	    case TARGET_CLUSTER_ENTRY: my_attrs = CLUSTER_ENTRY_ATTRS; n_my_attrs = N_CLUSTER_ENTRY_ATTRS; break;
	    case TARGET_SEARCH_ENTRY:  my_attrs = SEARCH_ENTRY_ATTRS;  n_my_attrs = N_SEARCH_ENTRY_ATTRS; break;
	    default:
		mssError(0, "Cluster",
		    "Unknown target type %u detected while attempting to generate hint.",
		    target_type
		);
		return;
	    }
	
	/** Attempt to give hints. **/
	if (cluster_i_tryHint(attr_name, my_attrs, n_my_attrs)) {}
	else if (cluster_i_tryHint(attr_name, DRIVER_ATTRIBUTE_NAMES, N_DRIVER_ATTRIBUTE_NAMES)) {}
    
    return;
    }


// LINK #functions
/*** Parses a ClusterAlgorithm from the algorithm attribute in the pStructInf.
 *** 
 *** @attention - Promises that a failure invokes mssError() at least once.
 *** 
 *** @param inf A parsed pStructInf.
 *** @param param_list The param objects that function as a kind of "scope" for
 *** 	evaluating parameter variables in the structure file.
 *** @returns The clustering algorithm, or ALGORITHM_NULL on failure.
 ***/
static ClusterAlgorithm
cluster_i_parseClusteringAlgorithm(pStructInf inf, pParamObjects param_list)
    {
	/** Get the algorithm attribute. **/
	char* algorithm;
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "algorithm",
	    DATA_T_STRING,
	    POD(&algorithm),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to parse attribute 'algorithm' in group \"%s\".", inf->Name);
	    return ALGORITHM_NULL;
	    }
	
	/** Check for any known clustering algorithms. **/
	if (strcasecmp(algorithm, "none")           == 0) return ALGORITHM_NONE;
	if (strcasecmp(algorithm, "sliding-window") == 0) return ALGORITHM_SLIDING_WINDOW;
	if (strcasecmp(algorithm, "k-means")        == 0) return ALGORITHM_KMEANS;
	if (strcasecmp(algorithm, "k-means++")      == 0) return ALGORITHM_KMEANS_PLUS_PLUS;
	if (strcasecmp(algorithm, "k-medoids")      == 0) return ALGORITHM_KMEDOIDS;
	if (strcasecmp(algorithm, "db-scan")        == 0) return ALGORITHM_DB_SCAN;
	
	/** Unknown value for clustering algorithm. **/
	mssError(1, "Cluster", "Unknown \"clustering algorithm\": %s", algorithm);
	
	/** Attempt to give a hint. **/
	char* all_names[N_CLUSTERING_ALGORITHMS] = {NULL};
	for (unsigned int i = 1u; i < N_CLUSTERING_ALGORITHMS; i++)
	    all_names[i] = cluster_i_clusteringAlgorithmToString(ALL_CLUSTERING_ALGORITHMS[i]);
	if (cluster_i_tryHint(algorithm, all_names, N_CLUSTERING_ALGORITHMS));
	else if (strcasecmp(algorithm, "sliding") == 0) cluster_i_giveHint(cluster_i_clusteringAlgorithmToString(ALGORITHM_SLIDING_WINDOW));
	else if (strcasecmp(algorithm, "window")  == 0) cluster_i_giveHint(cluster_i_clusteringAlgorithmToString(ALGORITHM_SLIDING_WINDOW));
	else if (strcasecmp(algorithm, "null")    == 0) cluster_i_giveHint(cluster_i_clusteringAlgorithmToString(ALGORITHM_NONE));
	else if (strcasecmp(algorithm, "nothing") == 0) cluster_i_giveHint(cluster_i_clusteringAlgorithmToString(ALGORITHM_NONE));
    
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
cluster_i_parseSimilarityMeasure(pStructInf inf, pParamObjects param_list)
    {
	/** Get the similarity_measure attribute. **/
	char* measure;
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "similarity_measure",
	    DATA_T_STRING,
	    POD(&measure),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to parse attribute 'similarity_measure' in group \"%s\".", inf->Name);
	    return SIMILARITY_NULL;
	    }
	
	/** Check for any known similarity measure. **/
	if (strcasecmp(measure, "cosine") == 0)      return SIMILARITY_COSINE;
	if (strcasecmp(measure, "levenshtein") == 0) return SIMILARITY_LEVENSHTEIN;
	
	/** Unknown similarity measure. **/
	mssError(1, "Cluster", "Unknown \"similarity measure\": %s", measure);
	
	/** Attempt to give a hint. **/
	char* all_names[N_SIMILARITY_MEASURES] = {NULL};
	for (unsigned int i = 1u; i < N_SIMILARITY_MEASURES; i++)
	    all_names[i] = cluster_i_similarityMeasureToString(ALL_SIMILARITY_MEASURES[i]);
	if (cluster_i_tryHint(measure, all_names, N_SIMILARITY_MEASURES));
	else if (strcasecmp(measure, "cos")           == 0) cluster_i_giveHint(cluster_i_similarityMeasureToString(SIMILARITY_COSINE));
	else if (strcasecmp(measure, "lev")           == 0) cluster_i_giveHint(cluster_i_similarityMeasureToString(SIMILARITY_LEVENSHTEIN));
	else if (strcasecmp(measure, "edit-dist")     == 0) cluster_i_giveHint(cluster_i_similarityMeasureToString(SIMILARITY_LEVENSHTEIN));
	else if (strcasecmp(measure, "edit-distance") == 0) cluster_i_giveHint(cluster_i_similarityMeasureToString(SIMILARITY_LEVENSHTEIN));
    
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
cluster_i_parseSourceData(pStructInf inf, pParamObjects param_list, char* path)
    {
    char* buf = NULL;
    pSourceData source_data = NULL;
    
	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse source data from NULL struct inf.");
	    return NULL; /* Skip error handler, which expects a valid struct inf. */
	    }
	ASSERTMAGIC(inf, MGK_STRUCTINF);
	if (UNLIKELY(param_list == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse source data with NULL param_list.");
	    goto err_free;
	    }
	if (UNLIKELY(path == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse source data with NULL path.");
	    goto err_free;
	    }
	
	/*** Knowing that an error has started and the error stack must be
	 *** cleared is sometimes not possible in this function, so clear it
	 *** while we know no errors are happening. **/
	mssClearError();
	
	/** Allocate SourceData. **/
	source_data = nmMalloc(sizeof(SourceData));
	if (UNLIKELY(source_data == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(SourceData));
	    goto err_free;
	    }
	memset(source_data, 0, sizeof(SourceData));
	SETMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Initialize obvious values for SourceData. **/
	source_data->Name = nmSysStrdup(inf->Name);
	if (UNLIKELY(source_data->Name == NULL))
	    {
	    mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", inf->Name);
	    goto err_free;
	    }
	if (UNLIKELY(objCurrentDate(&source_data->DateCreated) != 0))
	    {
	    mssError(1, "Cluster", "objCurrentDate() failed.");
	    goto err_free;
	    }
	
	/** Get source. **/
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "source",
	    DATA_T_STRING,
	    POD(&buf),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to get required 'source' attribute.");
	    goto err_free;
	    }
	source_data->SourcePath = nmSysStrdup(buf);
	if (UNLIKELY(source_data->SourcePath == NULL))
	    {
	    mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", buf);
	    goto err_free;
	    }
	
	/** Get the attribute name to use when querying keys from the source. **/
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "key_attr",
	    DATA_T_STRING,
	    POD(&buf),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to get required 'key_attr' attribute.");
	    goto err_free;
	    }
	source_data->KeyAttr = nmSysStrdup(buf);
	if (UNLIKELY(source_data->KeyAttr == NULL))
	    {
	    mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", buf);
	    goto err_free;
	    }
	
	/** Get the attribute name to use for querying data from the source. **/
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "data_attr",
	    DATA_T_STRING,
	    POD(&buf),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to get required 'data_attr' attribute.");
	    goto err_free;
	    }
	source_data->DataAttr = nmSysStrdup(buf);
	if (UNLIKELY(source_data->DataAttr == NULL))
	    {
	    mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", buf);
	    goto err_free;
	    }
	
	/** Create cache entry key. **/
	const size_t len = strlen(path)
	    + strlen(source_data->SourcePath)
	    + strlen(source_data->KeyAttr)
	    + strlen(source_data->DataAttr) + 5lu;
	const size_t cache_key_size = len * sizeof(char);
	source_data->CacheKey = nmSysMalloc(cache_key_size);
	if (UNLIKELY(source_data->CacheKey == NULL))
	    {
	    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", cache_key_size);
	    goto err_free;
	    }
	snprintf(source_data->CacheKey, len,
	    "%s?%s->%s:%s",
	    path, source_data->SourcePath, source_data->KeyAttr, source_data->DataAttr
	);
	
	/** Check for a cached version. **/
	pSourceData source_maybe = (pSourceData)xhLookup(&ClusterDriverCaches.SourceDataCache, source_data->CacheKey);
	if (source_maybe != NULL)
	    { /* Cache hit. */
	    ASSERTMAGIC(source_maybe, MGK_CL_SOURCE_DATA);
	    
	    /** Free data we don't need. **/
	    nmSysFree(source_data->CacheKey);
	    cluster_i_freeSourceData(source_data);
	    
	    /** Return the cached source data. **/
	    return source_maybe;
	    }
	
	/** Cache miss: Add the new object to the cache for next time. **/
	if (xhAdd(&ClusterDriverCaches.SourceDataCache, source_data->CacheKey, (void*)source_data) != 0)
	    {
	    mssError(1, "Cluster",
		"Failed to add source data to cache hash table with cache_key: \"%s\".",
		source_data->CacheKey
	    );
	    goto err_free;
	    }
	
	/** Success. **/
	return source_data;
	
    err_free:
	/** Error handling. **/
	if (source_data != NULL)
	    {
	    if (source_data->CacheKey != NULL) nmSysFree(source_data->CacheKey);
	    cluster_i_freeSourceData(source_data);
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
cluster_i_parseClusterData(pStructInf inf, pParamObjects param_list, pSourceData source_data)
    {
    int result;
    pClusterData cluster_data = NULL;
    XArray sub_clusters = {0};
    char* cache_key = NULL;
    
	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse cluster data from NULL struct inf.");
	    return NULL; /* Skip error handler, which expects a valid struct inf. */
	    }
	ASSERTMAGIC(inf, MGK_STRUCTINF);
	if (UNLIKELY(param_list == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse cluster data from NULL param_list.");
	    goto err_free;
	    }
	
	/** Recursion check. **/
	if (UNLIKELY(thExcessiveRecursion()))
	    {
	    mssError(1, "Cluster", "Resource exhaustion occurred while parsing cluster data.");
	    goto err_free;
	    }
	
	/** Verify source_data value. **/
	if (UNLIKELY(source_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse cluster data from NULL source data.");
	    goto err_free;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Allocate space for data struct. **/
	cluster_data = nmMalloc(sizeof(ClusterData));
	if (UNLIKELY(cluster_data == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(ClusterData));
	    goto err_free;
	    }
	memset(cluster_data, 0, sizeof(ClusterData));
	SETMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	
	/** Basic fields. **/
	cluster_data->Name = nmSysStrdup(inf->Name);
	if (UNLIKELY(cluster_data->Name == NULL))
	    {
	    mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", inf->Name);
	    goto err_free;
	    }
	cluster_data->SourceData = source_data;
	if (UNLIKELY(objCurrentDate(&cluster_data->DateCreated) != 0))
	    {
	    mssError(1, "Cluster", "objCurrentDate() failed.");
	    goto err_free;
	    }
	
	/** Get algorithm. **/
	cluster_data->ClusterAlgorithm = cluster_i_parseClusteringAlgorithm(inf, param_list);
	if (UNLIKELY(cluster_data->ClusterAlgorithm == ALGORITHM_NULL)) goto err_free;
	
	/** Handle no clustering case. **/
	if (cluster_data->ClusterAlgorithm == ALGORITHM_NONE)
	    {
	    cluster_data->nClusters = 1u;
	    goto parsing_done;
	    }
	
	/** Get similarity_measure. **/
	cluster_data->SimilarityMeasure = cluster_i_parseSimilarityMeasure(inf, param_list);
	if (UNLIKELY(cluster_data->SimilarityMeasure == SIMILARITY_NULL))
	    {
	    mssError(0, "Cluster", "Failed to parse similarity measure.");
	    goto err_free;
	    }
	
	/** Handle sliding window case. **/
	if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
	    {
	    /** Sliding window doesn't allocate any clusters. **/
	    cluster_data->nClusters = 0u;
	    
	    /** Get window_size. **/
	    int window_size;
	    if (UNLIKELY(stGetObjAttrValueOSML(
		inf,
		"window_size",
		DATA_T_INTEGER,
		POD(&window_size),
		0,
		param_list->Session,
		param_list,
		EXPR_F_RUNSERVER
	    ) != 0))
		{
		mssError(0, "Cluster", "Failed to get required 'window_size' attribute.");
		goto err_free;
		}
	    if (window_size < 1)
		{
		mssError(1, "Cluster", "Invalid value for [window_size : uint > 0]: %d", window_size);
		goto err_free;
		}
	    
	    /** Store value. **/
	    cluster_data->WindowSize = (unsigned int)window_size;
	    goto parsing_done;
	    }
	
	/** Get num_clusters. **/
	int num_clusters;
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "num_clusters",
	    DATA_T_INTEGER,
	    POD(&num_clusters),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to get required 'num_clusters' attribute.");
	    goto err_free;
	    }
	if (num_clusters < 2)
	    {
	    mssError(1, "Cluster", "Invalid value for [num_clusters : uint > 1]: %d", num_clusters);
	    if (num_clusters == 1) fprintf(stderr, "HINT: Use algorithm=\"none\" to disable clustering.\n");
	    goto err_free;
	    }
	cluster_data->nClusters = (unsigned int)num_clusters;
	
	/** Get min_improvement. **/
	double improvement;
	result = stGetObjAttrValueOSML(
	    inf,
	    "min_improvement",
	    DATA_T_DOUBLE,
	    POD(&improvement),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	);
	if (result == 1) cluster_data->MinImprovement = CI_DEFAULT_MIN_IMPROVEMENT;
	else if (result == 0)
	    {
	    if (-1.0 <= improvement && improvement <= 1.0)
		cluster_data->MinImprovement = improvement;
	    else
		{
		mssError(1, "Cluster", "Invalid value for [min_improvement : -1.0 <= x <= 1.0]: %g", improvement);
		goto err_free;
		}
	    }
	else
	    {
	    mssError(0, "Cluster", "Failed to check for optional 'min_improvement' attribute.");
	    goto err_free;
	    }
	
	/** Get max_iterations. **/
	int max_iterations;
	result = stGetObjAttrValueOSML(
	    inf,
	    "max_iterations",
	    DATA_T_INTEGER,
	    POD(&max_iterations),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	);
	if (result == 1) cluster_data->MaxIterations = CI_DEFAULT_MAX_ITERATIONS;
	else if (result == 0)
	    {
	    if (max_iterations < 1)
		{
		mssError(1, "Cluster", "Invalid value for [max_iterations : uint > 0]: %d", max_iterations);
		goto err_free;
		}
	    cluster_data->MaxIterations = (unsigned int)max_iterations;
	    }
	else
	    {
	    mssError(0, "Cluster", "Failed to check for optional 'max_iterations' attribute.");
	    goto err_free;
	    }
	
	/** Get seed. **/
	int seed;
	result = stGetObjAttrValueOSML(
	    inf,
	    "seed",
	    DATA_T_INTEGER,
	    POD(&seed),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	);
	if (result == 1) cluster_data->Seed = CI_NO_SEED;
	else if (result == 0)
	    {
	    if (UNLIKELY(seed < 1))
		{
		mssError(1, "Cluster", "Invalid value for [seed : uint > 0]: %d", seed);
		goto err_free;
		}
	    cluster_data->Seed = (unsigned int)seed;
	    }
	else
	    {
	    mssError(0, "Cluster", "Failed to check for optional 'seed' attribute.");
	    goto err_free;
	    }
	
	/** Search for sub-clusters. **/
	if (xaInit(&sub_clusters, CI_INITIAL_SUBCLUSTERS))
	    {
	    mssError(1, "Cluster",
		"Failed to allocate an XArray of %d subclusters.",
		CI_INITIAL_SUBCLUSTERS
	    );
	    goto err_free;
	    }
	for (unsigned int i = 0u; i < inf->nSubInf; i++)
	    {
	    pStructInf sub_inf = inf->SubInf[i];
	    if (UNLIKELY(sub_inf == NULL))
		{
		mssError(1, "Cluster", "Failed to get subinf #%u/%u.", i + 1, inf->nSubInf);
		goto err_free; /* Skip in-loop error handler, which expects a valid sub_inf. */
		}
	    ASSERTMAGIC(sub_inf, MGK_STRUCTINF);
	    char* name = sub_inf->Name;
	    if (UNLIKELY(name == NULL))
		goto err_free; /* Skip in-loop error handler, which expects a valid sub_inf->Name. */
	    
	    /** Handle various struct types. **/
	    const int struct_type = stStructType(sub_inf);
	    switch (struct_type)
		{
		case ST_T_ATTRIB:
		    {
		    /** Ignore valid attribute names. **/
		    bool is_valid = false;
		    for (unsigned int i = 0u; i < N_INPUT_CLUSTER_ATTRS; i++)
			{
			if (strcmp(name, CLUSTER_ATTRS[i]) == 0)
			    {
			    is_valid = true;
			    break;
			    }
			}
		    if (is_valid) continue; /* Next inf. */
		    
		    /** Give the user a warning, and attempt to give a hint. **/
		    fprintf(stderr, "Warning: Unknown attribute '%s' in cluster \"%s\".\n", name, inf->Name);
		    if (cluster_i_tryHint(name, CLUSTER_ATTRS, N_INPUT_CLUSTER_ATTRS));
		    else if (strcasecmp(name, "k") == 0) cluster_i_giveHint("num_clusters");
		    else if (strcasecmp(name, "threshold") == 0) cluster_i_giveHint("min_improvement");
		    
		    break;
		    }
		
		case ST_T_SUBGROUP:
		    {
		    /** Select array by group type. **/
		    char* group_type = sub_inf->UsrType;
		    if (UNLIKELY(group_type == NULL))
			{
			mssError(1, "Cluster", "Failed to get group type.");
			goto err_sub_inf;
			}
		    if (strcmp(group_type, "cluster/cluster") != 0)
			{
			fprintf(stderr,
			    "Warning: Unknown group [\"%s\" : \"%s\"] in cluster \"%s\".\n",
			    name, group_type, inf->Name
			);
			cluster_i_giveHint("cluster/cluster");
			continue;
			}
		    
		    /** Subcluster found. **/
		    pClusterData sub_cluster = cluster_i_parseClusterData(sub_inf, param_list, source_data);
		    if (UNLIKELY(sub_cluster == NULL)) goto err_sub_inf;
		    sub_cluster->Parent = cluster_data;
		    if (UNLIKELY(xaAddItem(&sub_clusters, sub_cluster) < 0))
			{
			mssError(1, "Cluster", "Failed to add parsed subcluster to XArray.");
			goto err_sub_inf;
			}
		    
		    break;
		    }
		
		default:
		    {
		    fprintf(stderr,
			"Warning: Unknown struct type %d in cluster data.\n",
			struct_type
		    );
		    continue; /* Skip it. */
		    }
		}
	    
	    /** Success. **/
	    continue;
	    
    err_sub_inf:
	    mssError(0, "Cluster",
		"Failed to parse \"%s\", the #%u/%u subinf of %s.",
		sub_inf->Name, i + 1, inf->nSubInf, inf->Name
	    );
	    goto err_free;
	    }
	
	/** Post sub-inf parsing cleanup. **/
	cluster_data->nSubClusters = sub_clusters.nItems;
	cluster_data->SubClusters = (ClusterData**)xaToArray(&sub_clusters);
	if (UNLIKELY(cluster_data->SubClusters == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get cluster data array.");
	    goto err_free;
	    }
	warnFail(xaDeInit(&sub_clusters));
	sub_clusters.nAlloc = 0;
	
    parsing_done:;
	/** Create the cache_key. **/
	switch (cluster_data->ClusterAlgorithm)
	    {
	    case ALGORITHM_NONE:
		{
		const size_t len = strlen(source_data->CacheKey) + strlen(cluster_data->Name) + 8lu;
		const size_t cache_key_size = len * sizeof(char);
		cache_key = nmSysMalloc(cache_key_size);
		if (UNLIKELY(cache_key == NULL))
		    {
		    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", cache_key_size);
		    goto err_free;
		    }
		snprintf(cache_key, len, "%s/%s?%u",
		    source_data->CacheKey,
		    cluster_data->Name,
		    ALGORITHM_NONE
		);
		break;
		}
	    
	    case ALGORITHM_SLIDING_WINDOW:
		{
		const size_t len = strlen(source_data->CacheKey) + strlen(cluster_data->Name) + 16lu;
		const size_t cache_key_size = len * sizeof(char);
		cache_key = nmSysMalloc(cache_key_size);
		if (UNLIKELY(cache_key == NULL))
		    {
		    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", cache_key_size);
		    goto err_free;
		    }
		snprintf(cache_key, len, "%s/%s?%u&%u&%u",
		    source_data->CacheKey,
		    cluster_data->Name,
		    ALGORITHM_SLIDING_WINDOW,
		    cluster_data->SimilarityMeasure,
		    cluster_data->WindowSize
		);
		break;
		}
	    
	    default:
		{
		const size_t len = strlen(source_data->CacheKey) + strlen(cluster_data->Name) + 32lu;
		const size_t cache_key_size = len * sizeof(char);
		cache_key = nmSysMalloc(cache_key_size);
		if (UNLIKELY(cache_key == NULL))
		    {
		    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", cache_key_size);
		    goto err_free;
		    }
		snprintf(cache_key, len, "%s/%s?%u&%u&%u&%g&%u",
		    source_data->CacheKey,
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
	cluster_data->CacheKey = cache_key;
	
	/** Check for a cached version. **/
	pClusterData cluster_maybe = (pClusterData)xhLookup(&ClusterDriverCaches.ClusterDataCache, cache_key);
	if (cluster_maybe != NULL)
	    { /* Cache hit. */
	    ASSERTMAGIC(cluster_maybe, MGK_CL_CLUSTER_DATA);
	    
	    /** Free the parsed cluster that we no longer need. **/
	    if (LIKELY(cluster_data != NULL)) cluster_i_freeClusterData(cluster_data, false);
	    if (LIKELY(cache_key != NULL)) nmSysFree(cache_key);
	    
	    /** Return the cached cluster. **/
	    return cluster_maybe;
	    }
	
	/** Cache miss. **/
	if (xhAdd(&ClusterDriverCaches.ClusterDataCache, cache_key, (void*)cluster_data) != 0)
	    {
	    mssError(1, "Cluster",
		"Failed to add cluster data to cache hash table with cache_key: \"%s\".",
		cache_key
	    );
	    goto err_free;
	    }
	
	/** Success. **/
	return cluster_data;
	
    err_free:
	/** Error cleanup. **/
	if (cache_key != NULL) nmSysFree(cache_key);
	
	/** The cluster cache owns the subclusters so we only free the XArray. **/
	if (sub_clusters.nAlloc != 0) warnFail(xaDeInit(&sub_clusters));
	
	if (cluster_data != NULL) cluster_i_freeClusterData(cluster_data, false);
	
	mssError(0, "Cluster", "Failed to parse cluster from group \"%s\":\"%s\".", inf->Name, inf->UsrType);
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
 *** @param node_data The pNodeData, used to get the param list and to look up
 *** 	the cluster pointed to by the source attribute.
 *** @returns A new pSearchData struct on success, or NULL on failure.
 ***/
static pSearchData
cluster_i_parseSearchData(pStructInf inf, pNodeData node_data)
    {
    pSearchData search_data = NULL;
    char* cache_key = NULL;
    
	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse search data from NULL struct inf.");
	    return NULL; /* Skip error handler, which expects a valid struct inf. */
	    }
	ASSERTMAGIC(inf, MGK_STRUCTINF);
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse search data from NULL node_data.");
	    goto err_free;
	    }
	
	/** Extract values. **/
	pParamObjects param_list = node_data->ParamList;
	if (UNLIKELY(param_list == NULL))
	    {
	    mssError(1, "Cluster", "Node data has NULL ParamList!");
	    goto err_free;
	    }
	
	/** Allocate space for search struct. **/
	search_data = nmMalloc(sizeof(SearchData));
	if (UNLIKELY(search_data == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(SearchData));
	    goto err_free;
	    }
	memset(search_data, 0, sizeof(SearchData));
	SETMAGIC(search_data, MGK_CL_SEARCH_DATA);
	
	/** Get basic information. **/
	search_data->Name = nmSysStrdup(inf->Name);
	if (UNLIKELY(search_data->Name == NULL))
	    {
	    mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", inf->Name);
	    goto err_free;
	    }
	if (UNLIKELY(objCurrentDate(&search_data->DateCreated) != 0))
	    {
	    mssError(1, "Cluster", "objCurrentDate() failed.");
	    goto err_free;
	    }
	
	/** Search for the source cluster. **/
	char* source_cluster_name;
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "source",
	    DATA_T_STRING,
	    POD(&source_cluster_name),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to get required 'source' attribute for search.");
	    goto err_free;
	    }
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
	    
	    /** Note: Subclusters should probably be investigated here, if they were implemented. **/
	    }
	
	/** Did we find the requested source? **/
	if (UNLIKELY(search_data->SourceCluster == NULL))
	    {
	    /** Print error. **/
	    mssError(1, "Cluster",
		"Could not find cluster \"%s\" for search \"%s\".",
		source_cluster_name, search_data->Name
	    );
	    
	    /** Attempt to give a hint. **/
	    char* cluster_names[node_data->nClusterDatas];
	    for (unsigned int i = 0; i < node_data->nClusterDatas; i++)
		cluster_names[i] = node_data->ClusterDatas[i]->Name;
	    cluster_i_tryHint(source_cluster_name, cluster_names, node_data->nClusterDatas);
	    
	    /** Fail. **/
	    goto err_free;
	    }
	
	/** Get threshold attribute. **/
	if (UNLIKELY(stGetObjAttrValueOSML(
	    inf,
	    "threshold",
	    DATA_T_DOUBLE,
	    POD(&search_data->Threshold),
	    0,
	    param_list->Session,
	    param_list,
	    EXPR_F_RUNSERVER
	) != 0))
	    {
	    mssError(0, "Cluster", "Failed to get required 'threshold' attribute for search.");
	    goto err_free;
	    }
	if (UNLIKELY(search_data->Threshold <= 0.0 || 1.0 <= search_data->Threshold))
	    {
	    mssError(1, "Cluster",
		"Invalid value for [threshold : 0.0 < x < 1.0]: %g",
		search_data->Threshold
	    );
	    goto err_free;
	    }
	
	/** Get similarity measure. **/
	search_data->SimilarityMeasure = cluster_i_parseSimilarityMeasure(inf, param_list);
	if (UNLIKELY(search_data->SimilarityMeasure == SIMILARITY_NULL))
	    {
	    mssError(0, "Cluster", "Failed to parse similarity measure.");
	    goto err_free;
	    }
	
	/** Check for additional data to warn the user about. **/
	for (unsigned int i = 0u; i < inf->nSubInf; i++)
	    {
	    pStructInf sub_inf = inf->SubInf[i];
	    if (UNLIKELY(sub_inf == NULL))
		{
		mssError(1, "Cluster", "Failed to get subinf #%u/%u.", i + 1, inf->nSubInf);
		goto err_free; /* Skip in-loop error handler, which expects a valid sub_inf. */
		}
	    ASSERTMAGIC(sub_inf, MGK_STRUCTINF);
	    char* name = sub_inf->Name;
	    if (UNLIKELY(name == NULL))
		goto err_free; /* Skip in-loop error handler, which expects a valid sub_inf->Name. */
	    
	    /** Handle various struct types. **/
	    const int struct_type = stStructType(sub_inf);
	    switch (struct_type)
		{
		case ST_T_ATTRIB:
		    {
		    /** Ignore valid attribute names. **/
		    bool is_valid = false;
		    for (unsigned int i = 0u; i < N_INPUT_SEARCH_ATTRS; i++)
			{
			if (strcmp(name, SEARCH_ATTRS[i]) == 0)
			    {
			    is_valid = true;
			    break;
			    }
			}
		    if (is_valid) continue; /* Next inf. */
		    
		    /** Give the user a warning, and attempt to give a hint. **/
		    fprintf(stderr, "Warning: Unknown attribute '%s' in search \"%s\".\n", name, inf->Name);
		    cluster_i_tryHint(name, SEARCH_ATTRS, N_INPUT_SEARCH_ATTRS);
		    
		    break;
		    }
		
		case ST_T_SUBGROUP:
		    {
		    /** The spec does not specify any valid sub-groups for searches. **/
		    char* group_type = sub_inf->UsrType;
		    if (UNLIKELY(group_type == NULL))
			{
			mssError(1, "Cluster", "Failed to get group type.");
			goto err_sub_inf;
			}
		    fprintf(stderr,
			"Warning: Unknown group [\"%s\" : \"%s\"] in search \"%s\".\n",
			name, group_type, inf->Name
		    );
		    break;
		    }
		
		default:
		    {
		    fprintf(stderr,
			"Warning: Unknown struct type %d in search data.\n",
			struct_type
		    );
		    continue; /* Skip it. */
		    }
		}
	    
	    /** Success. **/
	    continue;
	    
    err_sub_inf:
	    mssError(0, "Cluster",
		"Failed to parse \"%s\", the #%u/%u subinf of %s.",
		sub_inf->Name, i + 1, inf->nSubInf, inf->Name
	    );
	    goto err_free;
	    }
	
	/** Create cache entry key. **/
	char* source_cache_key = search_data->SourceCluster->CacheKey;
	const size_t cache_key_len = strlen(source_cache_key) + strlen(search_data->Name) + 16lu;
	const size_t cache_key_size = cache_key_len * sizeof(char);
	cache_key = nmSysMalloc(cache_key_size);
	if (UNLIKELY(cache_key == NULL))
	    {
	    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", cache_key_size);
	    goto err_free;
	    }
	snprintf(cache_key, cache_key_len, "%s/%s?%g&%u",
	    source_cache_key,
	    search_data->Name,
	    search_data->Threshold,
	    search_data->SimilarityMeasure
	);
	search_data->CacheKey = cache_key;
	pXHashTable search_cache = &ClusterDriverCaches.SearchDataCache;
	
	/** Check for a cached version. **/
	pSearchData search_maybe = (pSearchData)xhLookup(search_cache, cache_key);
	if (search_maybe != NULL)
	    { /* Cache hit. */
	    ASSERTMAGIC(search_maybe, MGK_CL_SEARCH_DATA);
	    
	    /** Free the parsed search that we no longer need. **/
	    if (LIKELY(search_data != NULL)) cluster_i_freeSearchData(search_data);
	    if (LIKELY(cache_key != NULL)) nmSysFree(cache_key);
	    
	    /** Return the cached search. **/
	    return search_maybe;
	    }
	
	/** Cache miss. **/
	if (xhAdd(search_cache, cache_key, (void*)search_data) != 0)
	    {
	    mssError(1, "Cluster",
		"Failed to add search data to cache hash table with cache_key: \"%s\".",
		cache_key
	    );
	    goto err_free;
	    }
	
	/** Done. **/
	return search_data;
	
    err_free:
	mssError(0, "Cluster", "Failed to parse SearchData from group \"%s\".", inf->Name);
	
	/** Error cleanup. **/
	if (search_data != NULL) cluster_i_freeSearchData(search_data);
	if (cache_key != NULL) nmSysFree(cache_key);
	
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
cluster_i_parseNodeData(pStructInf inf, pObject parent)
    {
    int ret = -1;
    pNodeData node_data = NULL;
    XArray param_infs = {0};
    XArray cluster_infs = {0};
    XArray search_infs = {0};
    
	/** Edge cases. **/
	if (UNLIKELY(inf == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse node data from NULL struct inf.");
	    return NULL; /* Skip error handler, which expects a valid struct inf. */
	    }
	ASSERTMAGIC(inf, MGK_STRUCTINF);
	if (UNLIKELY(parent == NULL))
	    {
	    mssError(1, "Cluster", "Failed to parse node data from NULL parent object.");
	    return NULL; /* Skip error handler, which expects a valid path. */
	    }
	ASSERTMAGIC(parent, MGK_OBJECT);
	
	/** Get file path. **/
	char* path = objFilePath(parent);
	if (UNLIKELY(path == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get parent file path.");
	    goto err_free;
	    }
	
	/** Allocate node struct data. **/
	node_data = nmMalloc(sizeof(NodeData));
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(NodeData));
	    goto err_free;
	    }
	memset(node_data, 0, sizeof(NodeData));
	SETMAGIC(node_data, MGK_CL_NODE_DATA);
	node_data->Parent = parent;
	
	/** Set up param list. **/
	node_data->ParamList = expCreateParamList();
	if (UNLIKELY(node_data->ParamList == NULL))
	    {
	    mssError(1, "Cluster", "expCreateParamList() failed.");
	    goto err_free;
	    }
	node_data->ParamList->Session = parent->Session;
	if (UNLIKELY(node_data->ParamList->Session == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get session because parent->Session is NULL.");
	    goto err_free;
	    }
	ret = expAddParamToList(node_data->ParamList, "parameters", (void*)node_data, 0);
	if (UNLIKELY(ret != 0))
	    {
	    mssError(0, "Cluster", "Failed to add parameters to the param list scope (error code %d).", ret);
	    goto err_free;
	    }
	
	/** Set the param functions, defined later in the file. **/
	ret = expSetParamFunctions(
	    node_data->ParamList,
	    "parameters",
	    cluster_i_getParamType,
	    cluster_i_getParamValue,
	    cluster_i_setParamValue
	);
	if (UNLIKELY(ret != 0))
	    {
	    mssError(0, "Cluster", "Failed to set param functions (error code %d).", ret);
	    goto err_free;
	    }
	
	/** Detect relevant groups. **/
	if (xaInit(&param_infs, CI_INITIAL_INFS) != 0
	    || xaInit(&cluster_infs, CI_INITIAL_INFS) != 0
	    || xaInit(&search_infs, CI_INITIAL_INFS) != 0
	)   {
	    mssError(1, "Cluster", "Failed to initialize an XArray of size %d.", CI_INITIAL_INFS);
	    goto err_free;
	    }
	for (unsigned int i = 0u; i < inf->nSubInf; i++)
	    {
	    pStructInf sub_inf = inf->SubInf[i];
	    if (UNLIKELY(sub_inf == NULL))
		{
		mssError(1, "Cluster", "Failed to get subinf #%u/%u.", i + 1, inf->nSubInf);
		goto err_free; /* Skip in-loop error handler, which expects a valid sub_inf. */
		}
	    ASSERTMAGIC(sub_inf, MGK_STRUCTINF);
	    char* name = sub_inf->Name;
	    if (UNLIKELY(name == NULL))
		goto err_free; /* Skip in-loop error handler, which expects a valid sub_inf->Name. */
	    
	    /** Handle various struct types. **/
	    const int struct_type = stStructType(sub_inf);
	    switch (struct_type)
		{
		case ST_T_ATTRIB:
		    {
		    /** Ignore valid attribute names. **/
		    bool is_valid = false;
		    for (unsigned int i = 0u; i < N_INPUT_ROOT_ATTRS; i++)
			{
			if (strcmp(name, ROOT_ATTRS[i]) == 0)
			    {
			    is_valid = true;
			    break;
			    }
			}
		    if (is_valid) continue; /* Next inf. */
		    
		    /** Give the user a warning, and attempt to give a hint. **/
		    fprintf(stderr, "Warning: Unknown attribute '%s' in cluster driver root node \"%s\".\n", name, inf->Name);
		    cluster_i_tryHint(name, ROOT_ATTRS, N_INPUT_ROOT_ATTRS);
		    
		    break;
		    }
		
		case ST_T_SUBGROUP:
		    {
		    char* group_type = sub_inf->UsrType;
		    if (UNLIKELY(group_type == NULL))
			{
			mssError(1, "Cluster", "Failed to get group type.");
			goto err_sub_inf;
			}
		    if (strcmp(group_type, "cluster/parameter") == 0)
			{
			if (UNLIKELY(xaAddItem(&param_infs, sub_inf) < 0))
			    {
			    mssError(1, "Cluster", "Failed to add detected param inf to XArray.");
			    goto err_sub_inf;
			    }
			}
		    else if (strcmp(group_type, "cluster/cluster") == 0)
			{
			if (UNLIKELY(xaAddItem(&cluster_infs, sub_inf) < 0))
			    {
			    mssError(1, "Cluster", "Failed to add detected cluster inf to XArray.");
			    goto err_sub_inf;
			    }
			}
		    else if (strcmp(group_type, "cluster/search") == 0)
			{
			if (UNLIKELY(xaAddItem(&search_infs, sub_inf) < 0))
			    {
			    mssError(1, "Cluster", "Failed to add detected search inf to XArray.");
			    goto err_sub_inf;
			    }
			}
		    else
			{
			/** Give the user a warning, and attempt to give a hint. **/
			fprintf(stderr,
			    "Warning: Unknown group type \"%s\" on group \"%s\".\n",
			    group_type, sub_inf->Name
			);
			cluster_i_tryHint(group_type, (char*[]){
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
		    fprintf(stderr,
			"Warning: Unknown struct type %d in node data.\n",
			struct_type
		    );
		    continue; /* Skip it. */
		    }
		}
	    
	    /** Success. **/
	    continue;
	    
    err_sub_inf:
	    mssError(0, "Cluster",
		"Failed to parse \"%s\", the #%u/%u subinf of %s.",
		sub_inf->Name, i + 1, inf->nSubInf, inf->Name
	    );
	    goto err_free;
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
	node_data->Params = nmSysMalloc(params_size);
	if (UNLIKELY(node_data->Params == NULL))
	    {
	    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", params_size);
	    goto err_free;
	    }
	memset(node_data->Params, 0, params_size);
	
	/** Iterate over each param in the structure file. **/
	for (unsigned int i = 0u; i < node_data->nParams; i++)
	    {
	    pParam param = paramCreateFromInf(param_infs.Items[i]);
	    if (UNLIKELY(param == NULL))
		{
		mssError(0, "Cluster",
		    "Failed to create param from inf for param #%u/%u: %s",
		    i + 1, node_data->nParams, ((pStructInf)param_infs.Items[i])->Name
		);
		goto err_free;
		}
	    node_data->Params[i] = param;
	    
	    /** Check each provided param to see if the user provided value. **/
	    for (unsigned int j = 0u; j < num_provided_params; j++)
		{
		pStruct provided_param = provided_params[j];
		if (UNLIKELY(provided_param == NULL))
		    {
		    mssError(1, "Cluster",
			"Failed to get param #%u/%d.",
			j + 1, num_provided_params
		    );
		    goto err_free;
		    }
		
		/** If this provided param value isn't for the param, ignore it. **/
		if (UNLIKELY(strcmp(provided_param->Name, param->Name) != 0)) continue;
		
		/** Matched! The user is providing a value for this param. **/
		ret = paramSetValueFromInfNe(param, provided_param, 0, node_data->ParamList, node_data->ParamList->Session);
		if (UNLIKELY(ret != 0))
		    {
		    mssError(0, "Cluster",
			"Failed to set param value from struct info.\n"
			"  > Param #%u: %s\n"
			"  > Provided Param #%u: %s\n"
			"  > Error code: %d",
			i + 1, param->Name,
			j + 1, provided_param->Name,
			ret
		    );
		    goto err_free;
		    }
		
		/** Provided value successfully handled, we're done. **/
		break;
		}
	    
	    /** Invoke param hints parsing. **/
	    ret = paramEvalHints(param, node_data->ParamList, node_data->ParamList->Session);
	    if (UNLIKELY(ret != 0))
		{
		mssError(0, "Cluster",
		    "Failed to evaluate parameter hints for parameter \"%s\" (error code %d).",
		    param->Name, ret
		);
		goto err_free;
		}
	    }
	warnFail(xaDeInit(&param_infs));
	param_infs.nAlloc = 0;
	
	/*** Iterate over provided parameters to warn the user if they
	 *** specified a parameter that does not exist.
	 ***/
	for (unsigned int i = 0u; i < num_provided_params; i++)
	    {
	    pStruct provided_param = provided_params[i];
	    if (UNLIKELY(provided_param == NULL))
		{
		mssError(1, "Cluster",
		    "Failed to get provided param #%u/%d.",
		    i + 1, num_provided_params
		);
		goto err_free;
		}
	    char* provided_name = provided_param->Name;
	    if (UNLIKELY(provided_name == NULL))
		{
		mssError(1, "Cluster",
		    "Failed to get provided param name from param #%u/%d.",
		    i + 1, num_provided_params
		);
		goto err_free;
		}
	    
	    /** Look to see if this provided param actually exists for this driver instance. **/
	    bool param_exists = false;
	    for (unsigned int j = 0u; j < node_data->nParams; j++)
		if (strcmp(provided_name, node_data->Params[j]->Name) == 0)
		    {
		    param_exists = true;
		    break;
		    }
	    
	    /** If this param doesn't exist, warn the user and attempt to give a hint. **/
	    if (!param_exists)
		{
		fprintf(stderr,
		    "Warning: Unknown provided parameter '%s' for cluster file: %s.\n",
		    provided_name, objFileName(parent)
		);
		
		/** Attempt hint. **/
		const size_t param_name_size = node_data->nParams * sizeof(char*);
		char** param_names = nmSysMalloc(param_name_size);
		if (UNLIKELY(param_names == NULL))
		    {
		    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", param_name_size);
		    goto err_free;
		    }
		for (unsigned int j = 0u; j < node_data->nParams; j++)
		    param_names[j] = node_data->Params[j]->Name;
		cluster_i_tryHint(provided_name, param_names, node_data->nParams);
		nmSysFree(param_names);
		}
	    }
	
	/** Parse source data. **/
	node_data->SourceData = cluster_i_parseSourceData(inf, node_data->ParamList, path);
	if (UNLIKELY(node_data->SourceData == NULL))
	    {
	    mssError(0, "Cluster", "Failed to parse source data.");
	    goto err_free;
	    }
	
	/** Parse each cluster. **/
	node_data->nClusterDatas = cluster_infs.nItems;
	if (LIKELY(node_data->nClusterDatas > 0))
	    {
	    const size_t clusters_size = node_data->nClusterDatas * sizeof(pClusterData);
	    node_data->ClusterDatas = nmSysMalloc(clusters_size);
	    if (UNLIKELY(node_data->ClusterDatas == NULL))
		{
		mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", clusters_size);
		goto err_free;
		}
	    memset(node_data->ClusterDatas, 0, clusters_size);
	    for (unsigned int i = 0u; i < node_data->nClusterDatas; i++)
		{
		node_data->ClusterDatas[i] = cluster_i_parseClusterData(cluster_infs.Items[i], node_data->ParamList, node_data->SourceData);
		if (UNLIKELY(node_data->ClusterDatas[i] == NULL))
		    {
		    mssError(0, "Cluster", "Failed to parse cluster data.");
		    goto err_free;
		    }
		}
	    }
	else node_data->ClusterDatas = NULL;
	warnFail(xaDeInit(&cluster_infs));
	cluster_infs.nAlloc = 0;
	
	/** Parse each search. **/
	node_data->nSearchDatas = search_infs.nItems;
	if (LIKELY(node_data->nSearchDatas > 0))
	    {
	    const size_t searches_size = node_data->nSearchDatas * sizeof(pSearchData);
	    node_data->SearchDatas = nmSysMalloc(searches_size);
	    if (UNLIKELY(node_data->SearchDatas == NULL))
		{
		mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", searches_size);
		goto err_free;
		}
	    memset(node_data->SearchDatas, 0, searches_size);
	    for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
		{
		node_data->SearchDatas[i] = cluster_i_parseSearchData(search_infs.Items[i], node_data);
		if (UNLIKELY(node_data->SearchDatas[i] == NULL))
		    {
		    mssError(0, "Cluster", "Failed to parse search data.");
		    goto err_free;
		    }
		}
	    }
	else node_data->SearchDatas = NULL;
	warnFail(xaDeInit(&search_infs));
	search_infs.nAlloc = 0;
	
	/** Success. **/
	return node_data;
	
    err_free:
	mssError(0, "Cluster", "Failed to parse node from group \"%s\" in file: %s", inf->Name, path);
	
	/** Clean up. **/
	if (param_infs.nAlloc   != 0) warnFail(xaDeInit(&param_infs));
	if (cluster_infs.nAlloc != 0) warnFail(xaDeInit(&cluster_infs));
	if (search_infs.nAlloc  != 0) warnFail(xaDeInit(&search_infs));
	if (node_data != NULL) cluster_i_freeNodeData(node_data);
    
	return NULL;
    }


/** ================ Freeing Functions ================ **/
/** ANCHOR[id=freeing] **/
// LINK #functions

/** @param source_data A pSourceData struct, freed by this function. **/
static void
cluster_i_freeSourceData(pSourceData source_data)
    {
	/** Guard segfault. **/
	if (UNLIKELY(source_data == NULL))
	    {
	    fprintf(stderr, "Warning: Call to cluster_i_freeSourceData(NULL);\n");
	    return;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Free top level attributes, if they exist. **/
	/** Note: The CacheKey field is handled by the caching system, so we shouldn't free it here. **/
	if (LIKELY(source_data->Name != NULL))
	    {
	    nmSysFree(source_data->Name);
	    source_data->Name = NULL;
	    }
	if (LIKELY(source_data->SourcePath != NULL))
	    {
	    nmSysFree(source_data->SourcePath);
	    source_data->SourcePath = NULL;
	    }
	if (LIKELY(source_data->KeyAttr != NULL))
	    {
	    nmSysFree(source_data->KeyAttr);
	    source_data->KeyAttr = NULL;
	    }
	if (LIKELY(source_data->DataAttr != NULL))
	    {
	    nmSysFree(source_data->DataAttr);
	    source_data->DataAttr = NULL;
	    }
	
	/** Free fetched keys, if they exist. **/
	if (source_data->Keys != NULL)
	    {
	    for (unsigned int i = 0u; i < source_data->nDatas; i++)
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
	    for (unsigned int i = 0u; i < source_data->nDatas; i++)
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
	    for (unsigned int i = 0u; i < source_data->nDatas; i++)
		{
		if (source_data->Vectors[i] != NULL)
		    {
		    caFreeVector(source_data->Vectors[i]);
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
 *** The recursive version may leak some memory if the stack recursion limit
 *** is reached.
 *** 
 *** @param cluster_data The cluster data struct to free.
 *** @param recursive Whether to recursively free subclusters.
 ***/
static void
cluster_i_freeClusterData(pClusterData cluster_data, bool recursive)
    {
	if (UNLIKELY(thExcessiveRecursion()))
	    {
	    mssError(1, "Cluster", "Resource exhaustion occurred while freeing cluster datas.");
	    return;
	    }
	
	/** Guard segfault. **/
	if (UNLIKELY(cluster_data == NULL))
	    {
	    fprintf(stderr, "Warning: Call to cluster_i_freeClusterData(NULL, %s);\n", (recursive) ? "true" : "false");
	    return;
	    }
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	
	/** Free attribute data. **/
	if (LIKELY(cluster_data->Name != NULL))
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
		if (cluster->Indexes != NULL)
		    {
		    nmSysFree(cluster->Indexes);
		    cluster->Indexes = NULL;
		    }
		}
	    nmSysFree(cluster_data->Clusters);
	    cluster_data->Clusters = NULL;
	    }
	if (cluster_data->Sims != NULL)
	    {
	    nmSysFree(cluster_data->Sims);
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
			{
			cluster_i_freeClusterData(cluster_data->SubClusters[i], recursive);
			cluster_data->SubClusters[i] = NULL;
			}
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
cluster_i_freeSearchData(pSearchData search_data)
    {
	/** Guard segfault. **/
	if (UNLIKELY(search_data == NULL))
	    {
	    fprintf(stderr, "Warning: Call to cluster_i_freeSearchData(NULL);\n");
	    return;
	    }
	ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	
	/** Free attribute data. **/
	if (LIKELY(search_data->Name != NULL))
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
cluster_i_freeNodeData(pNodeData node_data)
    {
	/** Guard segfault. **/
	if (UNLIKELY(node_data == NULL))
	    {
	    fprintf(stderr, "Warning: Call to cluster_i_freeNodeData(NULL);\n");
	    return;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
	/** Free parsed params, if they exist. **/
	if (LIKELY(node_data->Params != NULL))
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
	if (LIKELY(node_data->ParamList != NULL))
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
	    /*** This data is cached, so we should NOT free it! The cache owns
	     *** this memory so we only need to drop our pointer.
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
cluster_i_clearCaches(void)
    {
	/*** Free caches in reverse of the order they are created in case
	 *** cached data relies on its source during the freeing process.
	 ***/
	warnFail(xhClearKeySafe(&ClusterDriverCaches.SearchDataCache, cluster_i_cacheFreeSearch, NULL));
	warnFail(xhClearKeySafe(&ClusterDriverCaches.ClusterDataCache, cluster_i_cacheFreeCluster, NULL));
	warnFail(xhClearKeySafe(&ClusterDriverCaches.SourceDataCache, cluster_i_cacheFreeSourceData, NULL));
    
    return;
    }


/** ================ Deep Size Computation Functions ================ **/
/** ANCHOR[id=sizing] **/
// LINK #functions

/*** Returns the deep size of a SourceData struct, including the size of all
 *** allocated substructures.
 *** 
 *** Note: The CacheKey field points to data managed by the caching systems, so it
 *** is ignored.
 *** 
 *** @param source_data The source data struct to be queried.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static size_t
cluster_i_sizeOfSourceData(pSourceData source_data)
    {
	/** Guard segfaults. **/
	if (UNLIKELY(source_data == NULL))
	    {
	    fprintf(stderr, "Warning: Call to cluster_i_sizeOfSourceData(NULL);\n");
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
	    for (unsigned int i = 0u; i < source_data->nDatas; i++)
		size += strlen(source_data->Keys[i]) * sizeof(char);
	    size += source_data->nDatas * sizeof(char*);
	    }
	if (source_data->Strings != NULL)
	    {
	    for (unsigned int i = 0u; i < source_data->nDatas; i++)
		size += strlen(source_data->Strings[i]) * sizeof(char);
	    size += source_data->nDatas * sizeof(char*);
	    }
	if (source_data->Vectors != NULL)
	    {
	    for (unsigned int i = 0u; i < source_data->nDatas; i++)
		size += caSparseLen(source_data->Vectors[i]) * sizeof(int);
	    size += source_data->nDatas * sizeof(pVector);
	    }
	size += sizeof(SourceData);
    
    return size;
    }


// LINK #functions
/*** Returns the deep size of a ClusterData struct, including the size of all
 *** allocated substructures.
 *** 
 *** Note: The CacheKey field points to data managed by the caching systems, so it
 *** is ignored.
 *** 
 *** @param cluster_data The cluster data struct to be queried.
 *** @param recursive Whether to recursively include subcluster sizes.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static size_t
cluster_i_sizeOfClusterData(pClusterData cluster_data, bool recursive)
    {
	if (UNLIKELY(thExcessiveRecursion()))
	    {
	    mssError(1, "Cluster", "Resource exhaustion occurred while counting size of cluster data structs.");
	    return 0u;
	    }
	
	/** Guard segfaults. **/
	if (UNLIKELY(cluster_data == NULL))
	    {
	    fprintf(stderr, "Warning: Call to cluster_i_sizeOfClusterData(NULL, %s);\n", (recursive) ? "true" : "false");
	    return 0u;
	    }
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	
	size_t size = 0u;
	if (cluster_data->Name != NULL) size += strlen(cluster_data->Name) * sizeof(char);
	if (cluster_data->Clusters != NULL)
	    {
	    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		size += cluster_data->Clusters[i].Size * (sizeof(int));
	    size += cluster_data->nClusters * sizeof(Cluster);
	    }
	if (cluster_data->Sims != NULL) size += cluster_data->SourceData->nDatas * sizeof(double);
	if (cluster_data->SubClusters != NULL)
	    {
	    if (recursive)
		{
		for (unsigned int i = 0u; i < cluster_data->nSubClusters; i++)
		    size += cluster_i_sizeOfClusterData(cluster_data->SubClusters[i], recursive);
		}
	    size += cluster_data->nSubClusters * sizeof(pClusterData);
	    }
	size += sizeof(ClusterData);
    
    return size;
    }


// LINK #functions
/*** Returns the deep size of a SearchData struct, including the size of all
 *** allocated substructures.
 *** 
 *** Note: The CacheKey field points to data managed by the caching systems, so it
 *** is ignored.
 *** 
 *** @param search_data The search data struct to be queried.
 *** @returns The size in bytes of the struct and all internal allocated data.
 ***/
static size_t
cluster_i_sizeOfSearchData(pSearchData search_data)
    {
	/** Guard segfaults. **/
	if (UNLIKELY(search_data == NULL))
	    {
	    fprintf(stderr, "Warning: Call to cluster_i_sizeOfSearchData(NULL);\n");
	    return 0u;
	    }
	ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	
	size_t size = 0u;
	if (search_data->Name != NULL) size += strlen(search_data->Name) * sizeof(char);
	if (search_data->Pairs != NULL) size += search_data->nPairs * (sizeof(pPair) + sizeof(Pair));
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
 *** @param source_data The pSourceData whose attributes should be computed.
 *** @param session The current session, used to open the data source.
 *** @returns 0 if successful, or
 ***         -1 on failure.
 ***/
static int
cluster_i_computeSourceData(pSourceData source_data, pObjSession session)
    {
    bool successful = false;
    int ret;
    pObject obj = NULL;
    pObjQuery query = NULL;
    XArray key_xarray = {0};
    XArray data_xarray = {0};
    XArray vector_xarray = {0};
    
	/** Edge cases. **/
	if (UNLIKELY(source_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to compute source data on NULL source data struct.");
	    return -1; /* Skip error handler, which expects a valid source data struct. */
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** If the vectors are already computed, we're done. **/
	if (source_data->Vectors != NULL)
	    {
	    successful = true;
	    goto end_free;
	    }
	
	/** Record the date and time. **/
	if (UNLIKELY(objCurrentDate(&source_data->DateComputed) != 0))
	    {
	    mssError(1, "Cluster", "objCurrentDate() failed.");
	    goto end_free;
	    }
	
	/** Open the source path specified by the .cluster file. **/
	obj = objOpen(session, source_data->SourcePath, OBJ_O_RDONLY, 0600, "system/directory");
	if (UNLIKELY(obj == NULL))
	    {
	    mssError(0, "Cluster", "Failed to open object driver.");
	    goto end_free;
	    }
	
	/** Generate a "query" for retrieving data. **/
	query = objOpenQuery(obj, NULL, NULL, NULL, NULL, 0);
	if (UNLIKELY(query == NULL))
	    {
	    mssError(0, "Cluster", "Failed to open query.");
	    goto end_free;
	    }
	
	/** Initialize an xarray to store the retrieved data. **/
	if (xaInit(&key_xarray, CI_INITIAL_SOURCE_DATAS) != 0
	    || xaInit(&data_xarray, CI_INITIAL_SOURCE_DATAS) != 0
	    || xaInit(&vector_xarray, CI_INITIAL_SOURCE_DATAS) != 0
	)   {
	    mssError(1, "Cluster", "Failed to initialize an XArray of size %d.", CI_INITIAL_SOURCE_DATAS);
	    goto end_free;
	    }
	
	/** Fetch data and build vectors. **/
	pObject entry;
	while ((entry = objQueryFetch(query, O_RDONLY)) != NULL)
	    {
	    ASSERTMAGIC(entry, MGK_OBJECT);
	    pVector vector = NULL;
	    char* key_dup = NULL;
	    char* data_dup = NULL;
	    bool entry_ok = false;
	    
	    /** Data value: Type checking. **/
	    const int data_datatype = objGetAttrType(entry, source_data->DataAttr);
	    if (UNLIKELY(data_datatype == -1))
		{
		mssError(0, "Cluster",
		    "Failed to get type for data of entry #%d.",
		    vector_xarray.nItems
		);
		goto entry_free;
		}
	    if (UNLIKELY(data_datatype != DATA_T_STRING))
		{
		mssError(1, "Cluster",
		    "Type for data of entry #%d was %s instead of String.",
		    vector_xarray.nItems, objTypeToStr(data_datatype)
		);
		goto entry_free;
		}
	    
	    /** Data value: Get value from provided data source. **/
	    char* data;
	    ret = objGetAttrValue(entry, source_data->DataAttr, DATA_T_STRING, POD(&data));
	    if (UNLIKELY(ret != 0))
		{
		mssError(0, "Cluster",
		    "Failed to get attribute value for data entry #%d (error code: %d).",
		    vector_xarray.nItems, ret
		);
		goto entry_free;
		}
	
	    if (UNLIKELY(data == NULL))
		{
		mssError(1, "Cluster",
		    "Got null string value from successful call to objGetAttrValue() for data entry #%d.",
		    vector_xarray.nItems
		);
		goto entry_free;
		}
	    
	    /** Skip empty strings. **/
	    if (strlen(data) == 0)
		{
		entry_ok = true;
		goto entry_free;
		}
	    
	    /** Convert the string to a vector. **/
	    vector = caBuildVector(data);
	    if (UNLIKELY(vector == NULL))
		{
		mssError(1, "Cluster", "Failed to build vectors for string \"%s\".", data);
		goto entry_free;
		}
	    if (UNLIKELY(caIsEmpty(vector)))
		{
		mssError(1, "Cluster", "Vector building for string \"%s\" produced no character pairs.", data);
		goto entry_free;
		}
	    if (caHasNoPairs(vector))
		{
		/** Skip pVector with only a single pair of boundary characters. **/
		entry_ok = true;
		goto entry_free;
		}
	    
	    
	    /** Key value: Type checking. **/
	    const int key_datatype = objGetAttrType(entry, source_data->KeyAttr);
	    if (UNLIKELY(key_datatype == -1))
		{
		mssError(0, "Cluster",
		    "Failed to get type of key on entry #%d.",
		    vector_xarray.nItems
		);
		goto entry_free;
		}
	    if (UNLIKELY(key_datatype != DATA_T_STRING))
		{
		mssError(1, "Cluster",
		    "Type of key on entry #%d was %s instead of String.",
		    vector_xarray.nItems, objTypeToStr(key_datatype)
		);
		goto entry_free;
		}
	    
	    /** Key value: Get value from provided data source. **/
	    char* key;
	    ret = objGetAttrValue(entry, source_data->KeyAttr, DATA_T_STRING, POD(&key));
	    if (UNLIKELY(ret != 0))
		{
		mssError(0, "Cluster",
		    "Failed to get value for key on entry #%d (error code: %d).",
		    vector_xarray.nItems, ret
		);
		goto entry_free;
		}
	    
	    if (UNLIKELY(key == NULL))
		{
		mssError(1, "Cluster",
		    "Got null string value from successful call to objGetAttrValue() for key on entry #%d.",
		    vector_xarray.nItems
		);
		goto entry_free;
		}
	    
	    /** Store values. **/
	    key_dup = nmSysStrdup(key);
	    if (key_dup == NULL)
		{
		mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", key);
		goto entry_free;
		}
	    data_dup = nmSysStrdup(data);
	    if (data_dup == NULL)
		{
		mssError(1, "Cluster", "nmSysStrdup(\"%s\") failed.", data);
		goto entry_free;
		}
	    
	    /** Hand each value to its xarray, which owns it from then on. **/
	    if (xaAddItem(&key_xarray, (void*)key_dup) < 0)
		{
		mssError(1, "Cluster", "Failed to add key_dup to XArray.");
		goto entry_free;
		}
	    key_dup = NULL;
	    if (xaAddItem(&data_xarray, (void*)data_dup) < 0)
		{
		mssError(1, "Cluster", "Failed to add data_dup to XArray.");
		goto entry_free;
		}
	    data_dup = NULL;
	    if (xaAddItem(&vector_xarray, (void*)vector) < 0)
		{
		mssError(1, "Cluster", "Failed to add vector to XArray.");
		goto entry_free;
		}
	    vector = NULL;
	    entry_ok = true;
	    
    entry_free:
	    /** Clean up owned memory, then fail if an error occurred. **/
	    if (UNLIKELY(vector != NULL)) caFreeVector(vector);
	    if (UNLIKELY(key_dup != NULL)) nmSysFree(key_dup);
	    if (UNLIKELY(data_dup != NULL)) nmSysFree(data_dup);
	    warnFail(objClose(entry));
	    
	    if (UNLIKELY(!entry_ok)) goto end_free;
	    }
	
	source_data->nDatas = vector_xarray.nItems;
	if (UNLIKELY(source_data->nDatas == 0))
	    {
	    mssError(0, "Cluster", "Data source path did not contain any valid data.");
	    goto end_free;
	    }
	
	/** Trim and store the keys, data strings, and vectors. **/
	source_data->Keys = (char**)xaToArray(&key_xarray);
	if (UNLIKELY(source_data->Keys == NULL))
	    {
	    mssError(1, "Cluster", "xaToArray(&key_xarray) failed.");
	    goto end_free;
	    }
	source_data->Strings = (char**)xaToArray(&data_xarray);
	if (UNLIKELY(source_data->Strings == NULL))
	    {
	    mssError(1, "Cluster", "xaToArray(&data_xarray) failed.");
	    goto end_free;
	    }
	source_data->Vectors = (int**)xaToArray(&vector_xarray);
	if (UNLIKELY(source_data->Vectors == NULL))
	    {
	    mssError(1, "Cluster", "xaToArray(&vector_xarray) failed.");
	    goto end_free;
	    }
	
	/** The stored arrays own the values now, so only free the xarrays. **/
	warnFail(xaDeInit(&key_xarray));
	key_xarray.nAlloc = 0;
	warnFail(xaDeInit(&data_xarray));
	data_xarray.nAlloc = 0;
	warnFail(xaDeInit(&vector_xarray));
	vector_xarray.nAlloc = 0;
	
	/** Success. **/
	successful = true;

    end_free:
	/** Print an error if the function failed. **/
	if (UNLIKELY(!successful))
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
	    if (source_data->Keys != NULL)
		{
		nmSysFree(source_data->Keys);
		source_data->Keys = NULL;
		}
	    if (source_data->Strings != NULL)
		{
		nmSysFree(source_data->Strings);
		source_data->Strings = NULL;
		}
	    if (source_data->Vectors != NULL)
		{
		nmSysFree(source_data->Vectors);
		source_data->Vectors = NULL;
		}
	    }

	/** Clean up xarrays. **/
	/** Unlikely because (assuming no errors) these should already be freed. **/
	if (UNLIKELY(key_xarray.nAlloc != 0))
	    {
	    for (unsigned int i = 0u; i < key_xarray.nItems; i++)
		{
		char* key = key_xarray.Items[i];
		if (key != NULL) nmSysFree(key);
		}
	    warnFail(xaDeInit(&key_xarray));
	    }
	if (UNLIKELY(data_xarray.nAlloc != 0))
	    {
	    for (unsigned int i = 0u; i < data_xarray.nItems; i++)
		{
		char* str = data_xarray.Items[i];
		if (str != NULL) nmSysFree(str);
		}
	    warnFail(xaDeInit(&data_xarray));
	    }
	if (UNLIKELY(vector_xarray.nAlloc != 0))
	    {
	    for (unsigned int i = 0u; i < vector_xarray.nItems; i++)
		{
		pVector vec = vector_xarray.Items[i];
		if (vec != NULL) caFreeVector(vec);
		}
	    warnFail(xaDeInit(&vector_xarray));
	    }
	
	/** Clean up query & object structs. **/
	if (LIKELY(query != NULL)) warnFail(objQueryClose(query));
	if (LIKELY(obj != NULL)) warnFail(objClose(obj));

	/** Return the function status code. **/
	return (successful) ? 0 : -1;
    }


// LINK #functions
/*** Ensures that the computed attributes for `cluster_data` have been
 *** computed, running the specified clustering algorithm if necessary.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure.
 *** 
 *** @param cluster_data The pClusterData whose attributes should be computed.
 *** @param node_data The current pNodeData, used to get vectors to cluster.
 *** @returns 0 if successful, or
 ***         -1 on failure.
 ***/
static int
cluster_i_computeClusterData(pClusterData cluster_data, pNodeData node_data)
    {
    size_t clusters_size = -1;
    size_t sims_size = -1;
    
	/** Edge cases. **/
	if (UNLIKELY(cluster_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to compute cluster data on NULL cluster data struct.");
	    return -1; /* Skip error handler, which expects a valid cluster data struct. */
	    }
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to compute cluster data with NULL node data struct.");
	    goto err_free;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
	/** If the clusters are already computed, we're done. **/
	if (LIKELY(cluster_data->Clusters != NULL)) return 0;
	
	/** Make source data available. **/
	pSourceData source_data = node_data->SourceData;
	if (UNLIKELY(source_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get source data for cluster computation.");
	    goto err_free;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Ensure that we have computed SourceData vectors for computing clusters. **/
	pParamObjects param_list = node_data->ParamList;
	if (UNLIKELY(param_list == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get param list from driver node data.");
	    goto err_free;
	    }
	pObjSession session = param_list->Session;
	if (UNLIKELY(session == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get session from param list.");
	    goto err_free;
	    }
	ASSERTMAGIC(session, MGK_OBJSESSION);
	if (UNLIKELY(cluster_i_computeSourceData(source_data, session) != 0))
	    {
	    mssError(0, "Cluster", "ClusterData computation failed due to missing SourceData.");
	    goto err_free;
	    }
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Record the date and time. **/
	if (UNLIKELY(objCurrentDate(&cluster_data->DateComputed) != 0))
	    {
	    mssError(1, "Cluster", "objCurrentDate() failed.");
	    goto err_free;
	    }
	
	/** Allocate static memory for finding clusters. **/
	clusters_size = cluster_data->nClusters * sizeof(Cluster);
	sims_size = source_data->nDatas * sizeof(double);
	cluster_data->Clusters = nmSysMalloc(clusters_size);
	cluster_data->Sims = nmSysMalloc(sims_size);
	if (UNLIKELY(cluster_data->Clusters == NULL))
	    {
	    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", clusters_size);
	    goto err_free;
	    }
	if (UNLIKELY(cluster_data->Sims == NULL))
	    {
	    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", sims_size);
	    goto err_free;
	    }
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
		const size_t indexes_size = source_data->nDatas * sizeof(int);
		only_cluster->Size        = source_data->nDatas;
		only_cluster->Indexes     = nmSysMalloc(indexes_size);
		if (UNLIKELY(only_cluster->Indexes == NULL))
		    {
		    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", indexes_size);
		    goto err_free;
		    }
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
		unsigned int* labels = NULL;
		XArray indexes_in_cluster[cluster_data->nClusters];
		memset(indexes_in_cluster, 0, sizeof(indexes_in_cluster));
		
		/** Check for unimplemented similarity measures. **/
		if (UNLIKELY(cluster_data->SimilarityMeasure != SIMILARITY_COSINE))
		    {
		    mssError(1, "Cluster",
			"The similarity measure \"%s\" is not implemented for 'k-means' clusters.",
			cluster_i_similarityMeasureToString(cluster_data->SimilarityMeasure)
		    );
		    goto err_cleanup;
		    }
		
		/** Allocate labels. Note: caKmeans() initializes labels for us. **/
		const size_t labels_size = source_data->nDatas * sizeof(unsigned int);
		labels = nmSysMalloc(labels_size);
		if (UNLIKELY(labels == NULL))
		    {
		    mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", labels_size);
		    goto err_cleanup;
		    }
		
		/** Handle seed for caKmeans(). **/
		const bool auto_seed = (cluster_data->Seed == CI_NO_SEED);
		if (!auto_seed) srand(cluster_data->Seed);
		
		/** Run caKmeans(). **/
		const int kmeans_result = caKmeans(
		    source_data->Vectors,
		    source_data->nDatas,
		    cluster_data->nClusters,
		    cluster_data->MaxIterations,
		    cluster_data->MinImprovement,
		    labels,
		    cluster_data->Sims,
		    auto_seed
		);
		if (UNLIKELY(kmeans_result != 0))
		    {
		    mssError(1, "Cluster",
			"caKmeans(pVector[], %u, %u, %u, %lf, labels[], similarities[], %s) failed (error code: %d).",
			source_data->nDatas,
			cluster_data->nClusters,
			cluster_data->MaxIterations,
			cluster_data->MinImprovement,
			(auto_seed) ? "true" : "false",
			kmeans_result
		    );
		    goto err_cleanup;
		    }
		
		/** Convert the labels into clusters. **/
		
		/** Allocate temporary xArrays for tracking the indices stored in each cluster. **/
		for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		    {
		    if (UNLIKELY(xaInit(&indexes_in_cluster[i], CI_INITIAL_POINTS_PER_CLUSTER) != 0))
			{
			mssError(1, "Cluster",
			    "Failed to initialize XArray for cluster #%u/%u.",
			    i + 1, cluster_data->nClusters
			);
			memset(&indexes_in_cluster[i], 0, sizeof(XArray));
			goto err_cleanup;
			}
		    }
		
		/** Iterate through each label and add the index of the data to the specified cluster. **/
		for (unsigned long i = 0lu; i < source_data->nDatas; i++)
		    {
		    if (xaAddItem(&indexes_in_cluster[labels[i]], (void*)i) < 0)
			{
			mssError(1, "Cluster",
			    "Failed to add index #%lu/%u to XArray for cluster #%u/%u.",
			    i + 1lu, source_data->nDatas, labels[i] + 1, cluster_data->nClusters
			);
			goto err_cleanup;
			}
		    }
		
		/** Free unused data. **/
		nmSysFree(labels);
		labels = NULL;
		
		/** Store the indices for each cluster and free the temporary xArray. **/
		for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		    {
		    pXArray indexes_in_this_cluster = &indexes_in_cluster[i];
		    pCluster cluster = &cluster_data->Clusters[i];
		    SETMAGIC(cluster, MGK_CL_CLUSTER);
		    
		    /** Store the data in the cluster. **/
		    cluster->Size = indexes_in_this_cluster->nItems;
		    if (cluster->Size == 0) goto cluster_cleanup; /* Not a failure, but the array still needs to be freed. */
		    const size_t indexes_size = cluster->Size * sizeof(unsigned int);
		    cluster->Indexes = nmSysMalloc(indexes_size);
		    if (UNLIKELY(cluster->Indexes == NULL))
			{
			mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", indexes_size);
			goto err_cleanup;
			}
		    for (unsigned int i = 0u; i < indexes_in_this_cluster->nItems; i++)
			{
			const unsigned long index = (unsigned long)indexes_in_this_cluster->Items[i];
			if (UNLIKELY(index > __UINT32_MAX__))
			    {
			    mssError(1, "Cluster",
				"How did you try to cluster more than %u data points and cluster_i_computeClusterData() "
				"was the first thing to break?! Well... looks like it's time to update %s:%d to "
				"handle a larger amount of data.",
				__UINT32_MAX__, __FILE__, __LINE__
			    );
			    goto err_cleanup;
			    }
			cluster->Indexes[i] = (unsigned int)index;
			}
		    
    cluster_cleanup:
		    warnFail(xaDeInit(indexes_in_this_cluster));
		    indexes_in_this_cluster->Items = NULL;
		    }
		
		/** k-means done. **/
		break;
		
    err_cleanup:
		/** Error clean up. **/
		if (labels != NULL) nmSysFree(labels);
		for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		    {
		    if (indexes_in_cluster[i].Items == NULL) continue;
		    warnFail(xaDeInit(&indexes_in_cluster[i]));
		    }
		goto err_free;
		}
	    
	    default:
		mssError(1, "Cluster",
		    "Clustering algorithm \"%s\" is not implemented.",
		    cluster_i_clusteringAlgorithmToString(cluster_data->ClusterAlgorithm)
		);
		goto err_free;
	    }
	
	/** Success. **/
	return 0;
	
    err_free:
	if (cluster_data->Sims != NULL)
	    {
	    nmSysFree(cluster_data->Sims);
	    cluster_data->Sims = NULL;
	    }
	
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
		if (cluster->Indexes != NULL)
		    {
		    nmSysFree(cluster->Indexes);
		    cluster->Indexes = NULL;
		    }
		}
	    nmSysFree(cluster_data->Clusters);
	    cluster_data->Clusters = NULL;
	    }
	
	mssError(0, "Cluster", "ClusterData computation failed for \"%s\".", cluster_data->Name);
	
	return -1;
    }


// LINK #functions
/*** Ensures that the computed attributes for `search_data` are computed,
 *** running a search with the specified similarity measure if necessary.
 *** 
 *** @attention - Promises that mssError() will be invoked on failure.
 *** 
 *** @param search_data The pSearchData whose attributes should be computed.
 *** @param node_data The current pNodeData, used to get vectors to cluster.
 *** @returns 0 if successful, or
 ***         -1 on failure.
 ***/
static int
cluster_i_computeSearchData(pSearchData search_data, pNodeData node_data)
    {
    pXArray pairs = NULL;
    
	/** Edge cases. **/
	if (UNLIKELY(search_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to compute search data on NULL search data struct.");
	    return -1; /* Skip error handler, which expects a valid search data struct. */
	    }
	ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to compute search data with NULL node data struct.");
	    goto err_free;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
	/** If the pairs are already computed, we're done. **/
	if (LIKELY(search_data->Pairs != NULL)) return 0;
	
	/** We need the cluster data to be computed before we search it. **/
	pClusterData cluster_data = search_data->SourceCluster;
	if (UNLIKELY(cluster_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get cluster data for search computation.");
	    goto err_free;
	    }
	ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
	if (UNLIKELY(cluster_i_computeClusterData(cluster_data, node_data) != 0))
	    {
	    mssError(0, "Cluster", "SearchData computation failed due to missing clusters.");
	    goto err_free;
	    }
	    
	/** Extract source data. **/
	pSourceData source_data = cluster_data->SourceData;
	ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
	
	/** Record the date and time. **/
	if (UNLIKELY(objCurrentDate(&search_data->DateComputed) != 0))
	    {
	    mssError(1, "Cluster", "objCurrentDate() failed.");
	    goto err_free;
	    }
	
	/** Get the comparison function based on the similarity measure. **/
	pSimilarityFn similarity_function = cluster_i_similarityMeasureToFunction(search_data->SimilarityMeasure);
	if (UNLIKELY(similarity_function == NULL))
	    {
	    mssError(0, "Cluster", "Failed to get similarity measure function.");
	    goto err_free;
	    }
	
	/** Get a pointer to the data that will be used for the search. **/
	void** data = NULL;
	switch (search_data->SimilarityMeasure)
	    {
	    case SIMILARITY_COSINE: data = (void**)source_data->Vectors; break;
	    case SIMILARITY_LEVENSHTEIN: data = (void**)source_data->Strings; break;
	    default:
		mssError(1, "Cluster",
		    "Unknown similarity measure \"%s\".",
		    cluster_i_similarityMeasureToString(search_data->SimilarityMeasure)
		);
		goto err_free;
	    }
	
	/** Execute the search using the specified algorithm. **/
	if (cluster_data->ClusterAlgorithm == ALGORITHM_SLIDING_WINDOW)
	    {
	    /*** Note: We don't need to examine the clusters because nothing
	     ***       was computed during the clustering phase.
	     ***/
	    
	    /** Execute sliding search. **/
	    pairs = caSlidingSearch(
		data,
		source_data->nDatas,
		cluster_data->WindowSize,
		similarity_function,
		search_data->Threshold,
		NULL
	    );
	    if (UNLIKELY(pairs == NULL))
		{
		mssError(1, "Cluster",
		    "Failed to compute sliding search with %s similarity measure.",
		    cluster_i_similarityMeasureToString(search_data->SimilarityMeasure)
		);
		goto err_free;
		}
	    }
	else
	    {
	    /** Initialize the pairs array with a size of double the amount of data. **/
	    const int guess_size = search_data->SourceCluster->SourceData->nDatas * 2;
	    pairs = xaNew(guess_size);
	    if (UNLIKELY(pairs == NULL))
		{
		mssError(1, "Cluster", "xaNew(%d) failed.", guess_size);
		goto err_free;
		}
	    
	    /** Iterate over each cluster. **/
	    for (unsigned int i = 0u; i < cluster_data->nClusters; i++)
		{
		/** Extract the struct for the cluster. **/
		pCluster cluster = &cluster_data->Clusters[i];
		ASSERTMAGIC(cluster, MGK_CL_CLUSTER);
		
		/** Filter the data to only include values in the current cluster. **/
		void** filtered_data = data;
		bool free_filtered_data = false;
		if (cluster_data->nClusters > 1)
		    {
		    /** Allocate space. **/
		    const size_t filtered_data_size = cluster->Size * sizeof(void*);
		    filtered_data = nmSysMalloc(filtered_data_size);
		    if (UNLIKELY(filtered_data == NULL))
			{
			mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", filtered_data_size);
			goto err_free;
			}
		    free_filtered_data = true;
		    
		    /** Add filtered data. **/
		    for (unsigned int i = 0u; i < cluster->Size; i++)
			filtered_data[i] = data[cluster->Indexes[i]];
		    }
		
		/** Execute complete search. **/
		const pXArray cluster_pairs = caCompleteSearch(
		    filtered_data,
		    cluster->Size,
		    similarity_function,
		    search_data->Threshold,
		    NULL
		);
		if (free_filtered_data) nmSysFree(filtered_data);
		if (UNLIKELY(cluster_pairs == NULL))
		    {
		    mssError(1, "Cluster",
			"Failed to compute caCompleteSearch() with %s similarity measure.",
			cluster_i_similarityMeasureToString(search_data->SimilarityMeasure)
		    );
		    goto err_free;
		    }
		
		/** Remap the pairs to point to index into the SourceData arrays instead of filtered_data. **/
		for (unsigned int i = 0u; i < cluster_pairs->nItems; i++)
		    {
		    const pPair pair = (pPair)cluster_pairs->Items[i];
		    pair->i = cluster->Indexes[pair->i];
		    pair->j = cluster->Indexes[pair->j];
		    if (UNLIKELY(xaAddItem(pairs, pair) < 0))
			{
			mssError(1, "Cluster", "Failed to add new pair to pairs XArray.");
			
			/** Free remaining items. **/
			for (unsigned int j = i; j < cluster_pairs->nItems; j++)
			    nmFree(cluster_pairs->Items[j], sizeof(Pair));
			warnFail(xaFree(cluster_pairs));
			goto err_free;
			}
		    }
		warnFail(xaFree(cluster_pairs));
		}
	    }
	
	/** Store pairs. **/
	search_data->nPairs = pairs->nItems;
	if (pairs->nItems == 0)
	    {
	    /*** We need to set a valid pointer to indicate that the pairs
	     *** were computed, but nmSysMalloc(0) may return NULL, so we
	     *** allocate a 1 byte memory section to be a marker.
	     ***/
	    search_data->Pairs = nmSysMalloc(1);
	    if (UNLIKELY(search_data->Pairs == NULL))
		{
		mssError(1, "Cluster", "nmSysMalloc(1) failed.");
		goto err_free;
		}
	    warnFail(xaFree(pairs));
	    pairs = NULL;
	    }
	else
	    {
	    search_data->Pairs = (pPair*)xaToArray(pairs);
	    if (UNLIKELY(search_data->Pairs == NULL))
		{
		mssError(1, "Cluster", "xaToArray(pairs) failed.");
		goto err_free;
		}
	    warnFail(xaFree(pairs));
	    pairs = NULL;
	    }
	
	/** Success. **/
	return 0;
	
    err_free:
	if (search_data->Pairs != NULL)
	    {
	    nmSysFree(search_data->Pairs);
	    search_data->Pairs = NULL;
	    }
	if (pairs != NULL)
	    {
	    for (unsigned int i = 0u; i < pairs->nItems; i++)
		{
		if (pairs->Items[i] != NULL) nmFree(pairs->Items[i], sizeof(Pair));
		else break;
		}
	    warnFail(xaFree(pairs));
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
cluster_i_getParamType(void* inf_v, const char* attr_name)
    {
	/** Edge cases. **/
	pNodeData node_data = inf_v;
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get param type from NULL struct inf.");
	    return -1;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	if (UNLIKELY(attr_name == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get param type for null attr name.");
	    return -1;
	    }
	
	/** Find the parameter. **/
	for (unsigned int i = 0; i < node_data->nParams; i++)
	    {
	    const pParam param = warnNull(node_data->Params[i]);
	    if (UNLIKELY(param == NULL)) continue; /* Skip it. */
	    if (strcmp(param->Name, attr_name) != 0) continue;
	    
	    /** Parameter found. **/
	    return (param->Value == NULL) ? -1 : param->Value->DataType;
	    }
    
    /** Parameter not found. **/
    return -1;
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
 *** 	See datatypes.h for a list of valid datatypes.
 *** @param val A pointer to a location where a pointer to the requested
 *** 	data should be stored. Typically, the caller creates a local variable
 *** 	to store this pointer, then passes a pointer to that local variable
 *** 	so that they will have a pointer to the data.
 *** 	This buffer will not be modified unless the data is successfully
 *** 	found. If a value other than 0 is returned, the buffer is not updated.
 *** @returns 0 if successful,
 ***          1 if the variable is null,
 ***         -1 if an error occurs.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
static int
cluster_i_getParamValue(void* inf_v, char* attr_name, int datatype, pObjData val)
    {
	/** Edge cases. **/
	pNodeData node_data = inf_v;
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get param value from NULL struct inf.");
	    return -1;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	if (UNLIKELY(attr_name == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get param value for null attr name.");
	    return -1;
	    }
    
	/** Find the parameter. **/
	for (unsigned int i = 0; i < node_data->nParams; i++)
	    {
	    pParam param = (pParam)node_data->Params[i];
	    if (UNLIKELY(param == NULL)) continue;
	    if (strcmp(param->Name, attr_name) != 0) continue;
	    
	    /** Parameter found. **/
	    if (param->Value == NULL) return 1;
	    if (param->Value->Flags & DATA_TF_NULL) return 1;
	    if (UNLIKELY(param->Value->DataType != datatype))
		{
		mssError(1, "Cluster", "Type mismatch accessing parameter '%s'.", param->Name);
		return -1;
		}
	    
	    /** Return param value. **/
	    if (UNLIKELY(objCopyData(&(param->Value->Data), val, datatype) != 0))
		{
		mssError(1, "Cluster",
		    "Failed to copy param data of type %s (%d).",
		    objTypeToStr(datatype), datatype
		);
		goto err;
		}
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
cluster_i_setParamValue(void* inf_v, char* attr_name, int datatype, pObjData val)
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
 *** @param sys_type The content type registered by this driver that caused it
 *** 	to be picked to open this content.  This driver only registers itself
 *** 	for "system/cluster" currently.
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
	
	/** Edge cases. **/
	if (UNLIKELY(parent == NULL))
	    {
	    mssError(0, "Cluster", "Call to clusterOpen(NULL, ...);");
	    return NULL; /* Skip error handler, which expects a valid parent. */
	    }
	ASSERTMAGIC(parent, MGK_OBJECT);
	
	/** If CREAT and EXCL are specified, exclusively create it, failing if the file already exists. **/
	pSnNode node_struct = NULL;
	bool can_create = (parent->Mode & O_CREAT) && (parent->SubPtr == parent->Pathname->nElements);
	if (can_create && (parent->Mode & O_EXCL))
	    {
	    node_struct = snNewNode(parent->Prev, usr_type);
	    if (UNLIKELY(node_struct == NULL))
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
	if (UNLIKELY(node_struct == NULL))
	    {
	    mssError(0, "Cluster", "Failed to create node struct from provided cluster file.");
	    goto err_free;
	    }
	
	/** Magic. **/
	ASSERTMAGIC(node_struct, MGK_STNODE);
	ASSERTMAGIC(node_struct->Data, MGK_STRUCTINF);
	
	/** Parse node data from the node_struct. **/
	node_data = cluster_i_parseNodeData(node_struct->Data, parent);
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(0, "Cluster", "Failed to parse structure file \"%s\".", objFileName(parent));
	    goto err_free;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	
	/** Allocate driver instance data. **/
	driver_data = nmMalloc(sizeof(DriverData));
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(DriverData));
	    goto err_free;
	    }
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
	    
	    /** Success. **/
	    return driver_data;
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
	    
	    /** Sub-clusters are not fully implemented yet, skip the cluster logic below. **/
	    parent->SubCnt++;
	    driver_data->TargetData = (void*)cluster_data;
	    return driver_data; /* Success. */
	    
	    /** Check for sub-clusters in the path. **/
	    while (true)
		{
		/** Descend one path part deeper into the path. **/
		const char* path_part = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
		
		/** If the path does not go any deeper, we're done. **/
		if (path_part == NULL)
		    {
		    driver_data->TargetData = (void*)cluster_data;
		    break;
		    }
		
		/** Need to go deeper: Search for the requested sub-cluster. **/
		bool found = false;
		for (unsigned int i = 0u; i < cluster_data->nSubClusters; i++)
		    {
		    pClusterData sub_cluster = cluster_data->SubClusters[i];
		    if (strcmp(sub_cluster->Name, path_part) != 0) continue;
		    
		    /** Target found: Sub-cluster_data **/
		    cluster_data = sub_cluster;
		    found = true;
		    break;
		    }
		
		/** Error if path names sub-cluster that does not exist. **/
		if (!found)
		    {
		    mssError(1, "Cluster", "Sub-cluster \"%s\" does not exist.", path_part);
		    goto err_free;
		    }
		}
		
	    /** Success. **/
	    return driver_data;
	    }
	
	/** Search searches. **/
	for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
	    {
	    pSearchData search_data = node_data->SearchDatas[i];
	    ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
	    
	    /** Skip searches with the wrong name. **/
	    if (strcmp(search_data->Name, target_name) != 0) continue;
	    
	    /** Target found: Search **/
	    driver_data->TargetType = TARGET_SEARCH;
	    driver_data->TargetData = (void*)search_data;
	    
	    /** Check for extra, invalid path parts. **/
	    char* extra_data = obj_internal_PathPart(parent->Pathname, parent->SubPtr + parent->SubCnt++, 1);
	    if (UNLIKELY(extra_data != NULL))
		{
		mssError(1, "Cluster", "Unknown path part %s.", extra_data);
		goto err_free;
		}
		
	    /** Success. **/
	    return driver_data;
	    }
	
	/** We were unable to find the requested cluster or search. **/
	mssError(1, "Cluster", "\"%s\" is not the name of a declared cluster or search.", target_name);
	
	/** A separate scope is needed because target_names is a dynamically sized array. **/
	    {
	    /** Attempt to give a hint. **/
	    const unsigned int n_targets = node_data->nClusterDatas + node_data->nSearchDatas;
	    char* target_names[n_targets];
	    for (unsigned int i = 0u; i < node_data->nClusterDatas; i++)
		target_names[i] = node_data->ClusterDatas[i]->Name;
	    for (unsigned int i = 0u; i < node_data->nSearchDatas; i++)
		target_names[i + node_data->nClusterDatas] = node_data->SearchDatas[i]->Name;
	    cluster_i_tryHint(target_name, target_names, n_targets);
	    }
	
	/** Error cleanup. **/
    err_free:
	if (node_data != NULL) cluster_i_freeNodeData(node_data);
	if (driver_data != NULL) nmFree(driver_data, sizeof(DriverData));
	
	mssError(0, "Cluster",
	    "Failed to open cluster file \"%s\" at: %s",
	    objFileName(parent), objFilePath(parent)
	);
	
	return NULL;
    }


// LINK #functions
/*** Close a cluster driver instance object, releasing any necessary memory
 *** and closing any necessary underlying resources.  However, most of that
 *** data will be cached and won't be freed unless the cache is dropped.
 *** 
 *** @param inf_v The affected driver instance.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns 0 for success, or -1 if an error occurs.
 ***/
int
clusterClose(void* inf_v, pObjTrxTree* oxt)
    {
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to close NULL driver object.");
	    return -1;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	/** Update statistics. **/
	ClusterStatistics.CloseCalls++;
	
	/** Unlink the driver's node data. **/
	pNodeData node_data = warnNull(driver_data->NodeData);
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
	if (UNLIKELY(node_data != NULL && --node_data->OpenCount == 0))
	    cluster_i_freeNodeData(driver_data->NodeData);
	
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
    
	/*** When an error occurs in this function, it's hard to detect if the
	 *** error stack should be cleared so we clear it preemptively while
	 *** we know no error is occurring.
	 ***/
	mssClearError();
    
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to open a query on a NULL driver object.");
	    return NULL;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	/** Fail on target types that don't support queries. **/
	if (driver_data->TargetType != TARGET_SEARCH
	    && driver_data->TargetType != TARGET_CLUSTER
	    && driver_data->TargetType != TARGET_NODE)
	    {
	    /** Queries are not supported for this target type. **/
	    mssError(1, "Cluster",
		"The cluster driver object targeting a %s does not support queries.",
		cluster_i_targetTypeToString(driver_data->TargetType)
	    );
	    goto err_free;
	    }
	
	/** Update statistics. **/
	ClusterStatistics.OpenQueryCalls++;
	
	/** Allocate memory for the query. **/
	query_data = nmMalloc(sizeof(ClusterQuery));
	if (UNLIKELY(query_data == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(ClusterQuery));
	    goto err_free;
	    }
	memset(query_data, 0, sizeof(ClusterQuery));
	
	/** Initialize the query. **/
	SETMAGIC(query_data, MGK_CL_QUERY_DATA);
	query_data->DriverData = (pDriverData)inf_v;
	query_data->RowIndex = 0u;
	
	/** Success. **/
	return query_data;
	
    err_free:
	/** Error cleanup. **/
	if (query_data != NULL) nmFree(query_data, sizeof(ClusterQuery));
	mssError(0, "Cluster", "Failed to open query.");
	
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
	pQueryData query_data = qy_v;
	if (UNLIKELY(query_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to fetch from NULL query object.");
	    goto err_free;
	    }
	ASSERTMAGIC(query_data, MGK_CL_QUERY_DATA);
	pDriverData driver_data = query_data->DriverData;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to fetch from query object with NULL driver data.");
	    goto err_free;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	pNodeData node_data = driver_data->NodeData;
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to fetch from query object with NULL node data.");
	    goto err_free;
	    }
	ASSERTMAGIC(node_data, MGK_CL_NODE_DATA);
    
	/** Update statistics. **/
	ClusterStatistics.FetchCalls++;
	
	/** Allocate result struct. **/
	result_data = nmMalloc(sizeof(DriverData));
	if (UNLIKELY(result_data == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(DriverData));
	    goto err_free;
	    }
	
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
		unsigned int index = query_data->RowIndex;
		
		/** Fetch a cluster at the current index. **/
		const unsigned int n_cluster_datas = node_data->nClusterDatas;
		if (index < n_cluster_datas)
		    {
		    /** Fetch a cluster. **/
		    result_data->TargetType = TARGET_CLUSTER;
		    result_data->TargetData = node_data->ClusterDatas[index];
		    query_data->RowIndex++; /* Consume fetched entry. */
		    break;
		    }
		else index -= n_cluster_datas;
		
		/** Fetch a search at the current index. **/
		const unsigned int n_search_datas = node_data->nSearchDatas;
		if (index < n_search_datas)
		    {
		    /** Fetch a search. **/
		    result_data->TargetType = TARGET_SEARCH;
		    result_data->TargetData = node_data->SearchDatas[index];
		    query_data->RowIndex++; /* Consume fetched entry. */
		    break;
		    }
		else index -= n_search_datas;
		
		goto done_free;
		}
	    
	    case TARGET_CLUSTER:
		{
		/** Ensure the required data is computed. **/
		pClusterData target = (pClusterData)driver_data->TargetData;
		if (UNLIKELY(target == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get cluster target for fetch.");
		    goto err_free;
		    }
		ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		if (UNLIKELY(cluster_i_computeClusterData(target, node_data) != 0))
		    {
		    mssError(0, "Cluster", "Failed to compute ClusterData for query.");
		    goto err_free;
		    }
		
		/** Stop fetching if the requested data does not exist. **/
		if (UNLIKELY(query_data->RowIndex >= target->nClusters)) goto done_free;
		
		/** Set the data being fetched. **/
		result_data->TargetType = TARGET_CLUSTER_ENTRY;
		result_data->TargetIndex = query_data->RowIndex++;
		
		break;
		}
	    
	    case TARGET_SEARCH:
		{
		/** Ensure the required data is computed. **/
		pSearchData target = (pSearchData)driver_data->TargetData;
		if (UNLIKELY(target == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get search target for fetch.");
		    goto err_free;
		    }
		ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		if (UNLIKELY(cluster_i_computeSearchData(target, node_data) != 0))
		    {
		    mssError(0, "Cluster", "Failed to compute SearchData for query.");
		    goto err_free;
		    }
		
		/** Stop fetching if the requested data does not exist. **/
		if (UNLIKELY(query_data->RowIndex >= target->nPairs)) goto done_free;
		
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
	
	/*** Add a link to node_data, which is referenced by the result_data
	 *** struct we are about to return.
	 ***/
	node_data->OpenCount++;
	
	/** Success. **/
	return result_data;

    err_free:
	mssError(0, "Cluster", "Failed to fetch query result.");
	
    done_free:
	if (LIKELY(result_data != NULL)) nmFree(result_data, sizeof(DriverData));
	return NULL;
    }


// LINK #functions
/*** Close a cluster query instance, releasing any necessary memory and
 *** closing any necessary underlying resources.  This does not close the
 *** underlying driver instance, which must be closed with clusterClose().
 *** 
 *** @param qy_v The affected query instance.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns 0 for success, or -1 if an error occurs.
 ***/
int
clusterQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
	/** Cast the query data. **/
	pQueryData query_data = qy_v;
	if (UNLIKELY(query_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to close NULL query object.");
	    return -1;
	    }
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
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get an attribute type from a NULL driver object.");
	    return -1;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	/** Extract target type from driver data. **/
	const TargetType target_type = driver_data->TargetType;
	
	/** Update statistics. **/
	ClusterStatistics.GetTypeCalls++;
	
	/** Guard possible segfault. **/
	if (UNLIKELY(attr_name == NULL))
	    {
	    fprintf(stderr, "Warning: Call to clusterGetAttrType() with NULL attribute name.\n");
	    return DATA_T_UNAVAILABLE;
	    }
	
	/** Types for general attributes. **/
	if (LIKELY(strcmp(attr_name, "name") == 0)
	    || strcmp(attr_name, "annotation") == 0
	    || strcmp(attr_name, "content_type") == 0
	    || strcmp(attr_name, "inner_type") == 0
	    || strcmp(attr_name, "outer_type") == 0
	    || strcmp(attr_name, "internal_type") == 0)
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
 *** 	so that they will have a pointer to the data.
 *** 	This buffer will not be modified unless the data is successfully
 *** 	found.  If a value other than 0 is returned, the buffer is not updated.
 *** @param oxt The transaction tree (for the incomplete transaction system).
 *** @returns 0 if successful,
 ***          1 if the attribute's value is NULL,
 ***         -1 if an error occurs.
 *** 
 *** LINK ../../centrallix-lib/include/datatypes.h:72
 ***/
int
clusterGetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    TargetType target_type = -1;
    
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get an attribute value from a NULL driver object.");
	    return -1; /* Skip the error handler which expects a valid inf_v struct. */
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	/** Extract target type from driver data. **/
	target_type = driver_data->TargetType;
    
	/** Update statistics. **/
	ClusterStatistics.GetValCalls++;
	
	/** Guard possible segfault. **/
	if (UNLIKELY(attr_name == NULL))
	    {
	    fprintf(stderr, "Warning: Call to clusterGetAttrValue() with NULL attribute name.\n");
	    goto err;
	    }
	
	/** Type check. **/
	const int expected_datatype = clusterGetAttrType(inf_v, attr_name, oxt);
	if (UNLIKELY(expected_datatype == DATA_T_UNAVAILABLE || expected_datatype < 0))
	    {
	    cluster_i_unknownAttribute(attr_name, driver_data->TargetType);
	    goto err;
	    }
	if (UNLIKELY(datatype != expected_datatype))
	    {
	    mssError(1, "Cluster",
		"Type mismatch: Accessing attribute ['%s' : %s] as type %s.",
		attr_name, objTypeToStr(expected_datatype), objTypeToStr(datatype)
	    );
	    goto err;
	    }
	
	/** Handle name. **/
	if (LIKELY(strcmp(attr_name, "name") == 0))
	    {
	    ClusterStatistics.GetValCalls_name++;
	    switch (target_type)
		{
		case TARGET_NODE:
		    {
		    pSourceData source_data = driver_data->TargetData;
		    if (UNLIKELY(source_data == NULL))
			{
			mssError(1, "Cluster", "Failed to get source data.");
			goto err;
			}
		    ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
		    val->String = source_data->Name;
		    break;
		    }
		
		case TARGET_CLUSTER:
		case TARGET_CLUSTER_ENTRY:
		    {
		    pClusterData cluster_data = driver_data->TargetData;
		    if (UNLIKELY(cluster_data == NULL))
			{
			mssError(1, "Cluster", "Failed to get cluster data.");
			goto err;
			}
		    ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
		    val->String = cluster_data->Name;
		    break;
		    }
		
		case TARGET_SEARCH:
		case TARGET_SEARCH_ENTRY:
		    {
		    pSearchData search_data = driver_data->TargetData;
		    if (UNLIKELY(search_data == NULL))
			{
			mssError(1, "Cluster", "Failed to get search data.");
			goto err;
			}
		    ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
		    val->String = search_data->Name;
		    break;
		    }
		
		default:
		    mssError(1, "Cluster", "Unknown target type %u.", target_type);
		    goto err;
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
		case TARGET_SEARCH_ENTRY: val->String = "Clustering driver: Search Entry."; break;
		
		default:
		    mssError(1, "Cluster", "Unknown target type %u.", target_type);
		    goto err;
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
		    goto err;
		}
	    
	    return 0;
	    }
	
	/** Handle date_created. **/
	if (strcmp(attr_name, "date_created") == 0)
	    {
	    switch (target_type)
		{
		case TARGET_NODE:
		    /** Attribute is not defined for this target type. **/
		    goto err;
		
		case TARGET_CLUSTER:
		case TARGET_CLUSTER_ENTRY:
		    {
		    pClusterData cluster_data = driver_data->TargetData;
		    if (UNLIKELY(cluster_data == NULL))
			{
			mssError(1, "Cluster", "Failed to get cluster data.");
			goto err;
			}
		    ASSERTMAGIC(cluster_data, MGK_CL_CLUSTER_DATA);
		    if (cluster_data->DateCreated.Value == 0) return 1; /* DateCreated not set: return null - should never occur */
		    else val->DateTime = &cluster_data->DateCreated;
		    return 0;
		    }
		
		case TARGET_SEARCH:
		case TARGET_SEARCH_ENTRY:
		    {
		    pSearchData search_data = driver_data->TargetData;
		    if (UNLIKELY(search_data == NULL))
			{
			mssError(1, "Cluster", "Failed to get search data.");
			goto err;
			}
		    ASSERTMAGIC(search_data, MGK_CL_SEARCH_DATA);
		    if (search_data->DateCreated.Value == 0) return 1; /* DateCreated not set: return null - should never occur */
		    else val->DateTime = &search_data->DateCreated;
		    return 0;
		    }
		}
	    goto err;
	    }
	
	/** Handle last_modification and date_computed. **/
	if (strcmp(attr_name, "last_modification") == 0
	    || strcmp(attr_name, "date_computed") == 0)
	    {
	    switch (target_type)
		{
		case TARGET_NODE:
		    /** Attribute is not defined for this target type. **/
		    return 1;
		
		case TARGET_CLUSTER:
		case TARGET_CLUSTER_ENTRY:
		    {
		    pClusterData target = driver_data->TargetData;
		    if (UNLIKELY(target == NULL))
			{
			mssError(1, "Cluster", "Failed to get cluster data target.");
			goto err;
			}
		    ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		    
		    if (UNLIKELY(target->DateComputed.Value == 0))
			return 1; /* DateComputed not set: return null */
		    else val->DateTime = &target->DateComputed;
		    
		    return 0;
		    }
		
		case TARGET_SEARCH:
		case TARGET_SEARCH_ENTRY:
		    {
		    pSearchData target = driver_data->TargetData;
		    if (UNLIKELY(target == NULL))
			{
			mssError(1, "Cluster", "Failed to get search data target.");
			goto err;
			}
		    ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		    
		    if (UNLIKELY(target->DateComputed.Value == 0))
			return 1; /* DateComputed not set: return null */
		    else val->DateTime = &target->DateComputed;
		    
		    return 0;
		    }
		}
	    
	    /** Default: Unknown type. **/
	    mssError(1, "Cluster", "Unknown target type %u.", target_type);
	    goto err;
	    }
	
	/** Handle attributes for specific data targets. **/
	switch (target_type)
	    {
	    case TARGET_NODE:
		{
		pSourceData source_data = driver_data->TargetData;
		if (UNLIKELY(source_data == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get source data.");
		    goto err;
		    }
		ASSERTMAGIC(source_data, MGK_CL_SOURCE_DATA);
		
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
		pClusterData target = driver_data->TargetData;
		if (UNLIKELY(target == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get cluster data target.");
		    goto err;
		    }
		ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		
		if (strcmp(attr_name, "algorithm") == 0)
		    {
		    val->String = cluster_i_clusteringAlgorithmToString(target->ClusterAlgorithm);
		    return 0;
		    }
		if (strcmp(attr_name, "similarity_measure") == 0)
		    {
		    val->String = cluster_i_similarityMeasureToString(target->SimilarityMeasure);
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
		pSearchData target = driver_data->TargetData;
		if (UNLIKELY(target == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get search data target.");
		    goto err;
		    }
		ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		
		if (strcmp(attr_name, "source") == 0)
		    {
		    val->String = target->SourceCluster->Name;
		    return 0;
		    }
		if (strcmp(attr_name, "similarity_measure") == 0)
		    {
		    val->String = cluster_i_similarityMeasureToString(target->SimilarityMeasure);
		    return 0;
		    }
		if (strcmp(attr_name, "threshold") == 0)
		    {
		    val->Double = target->Threshold;
		    return 0;
		    }
		break;
		}
	    
	    case TARGET_CLUSTER_ENTRY:
		{
		pClusterData target = driver_data->TargetData;
		if (UNLIKELY(target == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get cluster data target.");
		    goto err;
		    }
		ASSERTMAGIC(target, MGK_CL_CLUSTER_DATA);
		pCluster target_cluster = &target->Clusters[driver_data->TargetIndex];
		ASSERTMAGIC(target_cluster, MGK_CL_CLUSTER);
		
		if (strcmp(attr_name, "items") == 0)
		    {
		    /** Static variable to allow us to free the StringVecs from previous calls. **/
		    static pStringVec vec = NULL;
		    if (vec != NULL)
			{
			if (vec->Strings != NULL) nmSysFree(vec->Strings);
			nmFree(vec, sizeof(StringVec));
			}
		    
		    /** Allocate a string vec for the requested data. **/
		    vec = val->StringVec = nmMalloc(sizeof(StringVec));
		    if (UNLIKELY(vec == NULL))
			{
			mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(StringVec));
			goto err;
			}
		    memset(vec, 0, sizeof(StringVec));
		    
		    /** Initialize the string vec. **/
		    const size_t strings_size = target_cluster->Size * sizeof(char*);
		    vec->Strings = nmSysMalloc(strings_size);
		    if (UNLIKELY(vec->Strings == NULL))
			{
			mssError(1, "Cluster", "nmSysMalloc(%zu) failed.", strings_size);
			goto err;
			}
		    vec->nStrings = target_cluster->Size;
		    for (unsigned int i = 0u; i < target_cluster->Size; i++)
			vec->Strings[i] = target->SourceData->Strings[target_cluster->Indexes[i]];
		    
		    /** Success. **/
		    return 0;
		    }
		break;
		}
	    
	    case TARGET_SEARCH_ENTRY:
		{
		pSearchData target = driver_data->TargetData;
		if (UNLIKELY(target == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get search data target.");
		    goto err;
		    }
		ASSERTMAGIC(target, MGK_CL_SEARCH_DATA);
		pPair target_dup = target->Pairs[driver_data->TargetIndex];
		if (UNLIKELY(target_dup == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get target duplicate.");
		    goto err;
		    }
		
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
		goto err;
	    }
	
	/** No checks matched the requested attribute. **/
	cluster_i_unknownAttribute(attr_name, driver_data->TargetType);
	
    err:;
	char* name;
	clusterGetAttrValue(inf_v, "name", DATA_T_STRING, POD(&name), NULL);
	mssError(0, "Cluster",
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
 *** 	will cause them to be ignored. This prevents throwing an error that
 *** 	could unnecessarily disrupt normal usage.
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
    
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get presentation hints from a NULL driver object.");
	    return NULL; /* Skip the error handler, which expects a valid inf_v struct. */
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	/** Extract target type from driver data. **/
	const TargetType target_type = driver_data->TargetType;
	
	/** Malloc presentation hints struct. **/
	hints = nmMalloc(sizeof(ObjPresentationHints));
	if (UNLIKELY(hints == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(ObjPresentationHints));
	    goto err_free;
	    }
	memset(hints, 0, sizeof(ObjPresentationHints));
	
	/** Hints that are the same for all attributes. **/
	hints->GroupID = -1;
	hints->VisualLength2 = 1;
	hints->Style     |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
	hints->StyleMask |= OBJ_PH_STYLE_READONLY | OBJ_PH_STYLE_CREATEONLY | OBJ_PH_STYLE_NOTNULL;
	
	/** Temporary param list for compiling expressions. **/
	tmp_list = expCreateParamList();
	if (UNLIKELY(tmp_list == NULL))
	    {
	    mssError(1, "Cluster", "expCreateParamList() failed.");
	    goto err_free;
	    }
	
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
	    || strcmp(attr_name, "outer_type") == 0
	    || strcmp(attr_name, "content_type") == 0
	    || strcmp(attr_name, "last_modification") == 0)
	    {
	    hints->VisualLength = 30;
	    goto end;
	    }
	if (strcmp(attr_name, "internal_type") == 0)
	    {
	    if (warnFail(xaInit(&(hints->EnumList), 5)) == 0)
		{
		warnNeg(xaAddItem(&(hints->EnumList), warnNull(nmSysStrdup("system/cluster"))));
		warnNeg(xaAddItem(&(hints->EnumList), warnNull(nmSysStrdup("cluster/cluster"))));
		warnNeg(xaAddItem(&(hints->EnumList), warnNull(nmSysStrdup("cluster/entry"))));
		warnNeg(xaAddItem(&(hints->EnumList), warnNull(nmSysStrdup("cluster/search"))));
		warnNeg(xaAddItem(&(hints->EnumList), warnNull(nmSysStrdup("search/entry"))));
		}
	    hints->Length = 16;
	    hints->VisualLength = 16;
	    hints->FriendlyName = warnNull(nmSysStrdup("Internal Type"));
	    hints->Style     |= OBJ_PH_STYLE_HIDDEN | OBJ_PH_STYLE_LOWERCASE;
	    hints->StyleMask |= OBJ_PH_STYLE_HIDDEN | OBJ_PH_STYLE_LOWERCASE;
	    goto end;
	    }
	
	/** Handle date created and date computed. **/
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
		hints->Format = warnNull(nmSysStrdup("datetime"));
		goto end;
		}
	    else
		{
		cluster_i_unknownAttribute(attr_name, driver_data->TargetType);
		goto err_free;
		}
	    }
	
	/** Search by target type. **/
	switch (target_type)
	    {
	    case TARGET_NODE:
		if (strcmp(attr_name, "source") == 0)
		    {
		    hints->Length = OBJSYS_MAX_PATH;
		    hints->VisualLength = 64;
		    hints->FriendlyName = warnNull(nmSysStrdup("Source Path"));
		    goto end;
		    }
		if (strcmp(attr_name, "key_attr") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = warnNull(nmSysStrdup("Key Attribute Name"));
		    goto end;
		    }
		if (strcmp(attr_name, "data_attr") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = warnNull(nmSysStrdup("Data Attribute Name"));
		    goto end;
		    }
		break;
	    
	    case TARGET_CLUSTER:
		if (strcmp(attr_name, "num_clusters") == 0)
		    {
		    /** Min and max values. **/
		    hints->MinValue = warnNull(expCompileExpression("1", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    hints->MaxValue = warnNull(expCompileExpression("2147483647", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    
		    /** Other hints. **/
		    hints->Length = 8;
		    hints->VisualLength = 4;
		    hints->FriendlyName = warnNull(nmSysStrdup("Number of Clusters"));
		    goto end;
		    }
		if (strcmp(attr_name, "min_improvement") == 0)
		    {
		    /** Min and max values. **/
		    hints->DefaultExpr = warnNull(expCompileExpression(CI_STRINGIFY_CONSTANT(CI_DEFAULT_MIN_IMPROVEMENT), tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    hints->MinValue = warnNull(expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    hints->MaxValue = warnNull(expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    
		    /** Other hints. **/
		    hints->Length = 16;
		    hints->VisualLength = 8;
		    hints->FriendlyName = warnNull(nmSysStrdup("Minimum Improvement Threshold"));
		    goto end;
		    }
		if (strcmp(attr_name, "max_iterations") == 0)
		    {
		    /** Min and max values. **/
		    hints->DefaultExpr = warnNull(expCompileExpression(CI_STRINGIFY_CONSTANT(CI_DEFAULT_MAX_ITERATIONS), tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    hints->MinValue = warnNull(expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    hints->MaxValue = warnNull(expCompileExpression("2147483647", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    
		    /** Other hints. **/
		    hints->Length = 8;
		    hints->VisualLength = 4;
		    hints->FriendlyName = warnNull(nmSysStrdup("Maximum Iterations"));
		    goto end;
		    }
		if (strcmp(attr_name, "algorithm") == 0)
		    {
		    /** Enum values. **/
		    if (warnFail(xaInit(&(hints->EnumList), N_CLUSTERING_ALGORITHMS)) == 0)
			{
			for (unsigned int i = 0u; i < N_CLUSTERING_ALGORITHMS; i++)
			    {
			    char* cluster_string = warnNull(nmSysStrdup(cluster_i_clusteringAlgorithmToString(ALL_CLUSTERING_ALGORITHMS[i])));
			    if (cluster_string == NULL) continue; /* Skip this. */
			    warnNeg(xaAddItem(&(hints->EnumList), cluster_string));
			    }
			}
		    
		    /** Min and max values. **/
		    hints->MinValue = warnNull(expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    char buf[8];
		    snprintf(buf, sizeof(buf), "%u", N_CLUSTERING_ALGORITHMS - 1u);
		    hints->MaxValue = warnNull(expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    
		    /** Display flags. **/
		    hints->Style     |= OBJ_PH_STYLE_BUTTONS;
		    hints->StyleMask |= OBJ_PH_STYLE_BUTTONS;
		    
		    /** Other hints. **/
		    hints->Length = 24;
		    hints->VisualLength = 20;
		    hints->FriendlyName = warnNull(nmSysStrdup("Clustering Algorithm"));
		    goto end;
		    }
		/** Fall-through: Start of overlapping region. **/
	    
	    case TARGET_SEARCH:
		if (strcmp(attr_name, "similarity_measure") == 0)
		    {
		    /** Enum values. **/
		    if (warnFail(xaInit(&(hints->EnumList), N_SIMILARITY_MEASURES)) == 0)
			{
			for (unsigned int i = 0u; i < N_SIMILARITY_MEASURES; i++)
			    {
			    char* similarity_string = warnNull(nmSysStrdup(cluster_i_similarityMeasureToString(ALL_SIMILARITY_MEASURES[i])));
			    if (similarity_string == NULL) continue; /* Skip this. */
			    warnNeg(xaAddItem(&(hints->EnumList), similarity_string));
			    }
			}
			
		    /** Display flags. **/
		    hints->Style     |= OBJ_PH_STYLE_BUTTONS;
		    hints->StyleMask |= OBJ_PH_STYLE_BUTTONS;
		    
		    /** Min and max values. **/
		    hints->MinValue = warnNull(expCompileExpression("0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    char buf[8];
		    snprintf(buf, sizeof(buf), "%u", N_SIMILARITY_MEASURES - 1u);
		    hints->MaxValue = warnNull(expCompileExpression(buf, tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    
		    /** Other hints. **/
		    hints->Length = 32;
		    hints->VisualLength = 20;
		    hints->FriendlyName = warnNull(nmSysStrdup("Similarity Measure"));
		    goto end;
		    }
		
		/** End of overlapping region. **/
		if (target_type == TARGET_CLUSTER) break;
		
		if (strcmp(attr_name, "source") == 0)
		    {
		    hints->Length = 64;
		    hints->VisualLength = 32;
		    hints->FriendlyName = warnNull(nmSysStrdup("Source Cluster Name"));
		    goto end;
		    }
		if (strcmp(attr_name, "threshold") == 0)
		    {
		    /** Min and max values. **/
		    hints->MinValue = warnNull(expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    hints->MaxValue = warnNull(expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    
		    /** Other hints. **/
		    hints->Length = 16;
		    hints->VisualLength = 8;
		    hints->FriendlyName = warnNull(nmSysStrdup("Similarity Threshold"));
		    goto end;
		    }
		break;
	    
	    case TARGET_CLUSTER_ENTRY:
		{
		if (strcmp(attr_name, "items") == 0)
		    {
		    /** Other hints. **/
		    hints->Length = 65536;
		    hints->VisualLength = 256;
		    hints->FriendlyName = warnNull(nmSysStrdup("Cluster Data"));
		    goto end;
		    }
		break;
		}
	    
	    case TARGET_SEARCH_ENTRY:
		{
		if (strcmp(attr_name, "key1") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = warnNull(nmSysStrdup("Key 1"));
		    goto end;
		    }
		if (strcmp(attr_name, "key2") == 0)
		    {
		    hints->Length = 255;
		    hints->VisualLength = 32;
		    hints->FriendlyName = warnNull(nmSysStrdup("Key 2"));
		    goto end;
		    }
		if (strcmp(attr_name, "sim") == 0)
		    {
		    /** Min and max values. **/
		    hints->MinValue = warnNull(expCompileExpression("0.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    hints->MaxValue = warnNull(expCompileExpression("1.0", tmp_list, MLX_F_ICASE | MLX_F_FILENAMES, 0));
		    
		    /** Other hints. **/
		    hints->Length = 16;
		    hints->VisualLength = 8;
		    hints->FriendlyName = warnNull(nmSysStrdup("Similarity"));
		    goto end;
		    }
		break;
		}
	    
	    default:
		mssError(1, "Cluster", "Unknown target type %u.", target_type);
		goto err_free;
	    }
	
	/** No checks matched the requested attribute. **/
	cluster_i_unknownAttribute(attr_name, driver_data->TargetType);
	
    err_free:;
	/** Construct the clearest error message that we can. **/
	char* name = NULL;
	char* internal_type = NULL;
	warnFail(clusterGetAttrValue(inf_v, "name", DATA_T_STRING, POD(&name), NULL));
	warnFail(clusterGetAttrValue(inf_v, "internal_type", DATA_T_STRING, POD(&internal_type), NULL));
	mssError(0, "Cluster",
	    "Failed to get presentation hints for '%s' on object '%s' : \"%s\".",
	    attr_name, name, internal_type
	);
	
	/** Error cleanup. **/
	if (hints != NULL) nmFree(hints, sizeof(ObjPresentationHints));
	hints = NULL;
	
    end:
	if (tmp_list != NULL) warnFail(expFreeParamList(tmp_list));
    
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
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get first attribute from NULL driver object.");
	    return NULL;
	    }
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
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get next attribute from NULL driver object.");
	    return NULL;
	    }
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
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get cluster info for NULL cluster object.");
	    goto err;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	/** Get node data. **/
	pNodeData node_data = driver_data->NodeData;
	if (UNLIKELY(node_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get node data from driver data.");
	    goto err;
	    }
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
		    info->nSubobjects = node_data->SourceData->nDatas;
		    }
		break;
	    
	    case TARGET_SEARCH:
		{
		pSearchData search_data = driver_data->TargetData;
		if (UNLIKELY(search_data == NULL))
		    {
		    mssError(1, "Cluster", "Failed to get search data.");
		    goto err;
		    }
		
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
	mssError(0, "Cluster", "Failed to execute get info.");
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
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get first method from NULL driver object.");
	    return NULL;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
	
	driver_data->TargetMethodIndex = 0u;
    
    return clusterGetNextMethod(inf_v, oxt);
    }


// LINK #functions
/*** Returns the name of the next method that one can execute from
 *** this driver instance (using `clusterExecuteMethod()`).
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
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to get next method from NULL driver object.");
	    return NULL;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
    
	if (driver_data->TargetMethodIndex >= METHOD_NAMES_COUNT) return NULL;
    
    return METHOD_NAMES[driver_data->TargetMethodIndex++];
    }


// LINK #functions
/*** Prints a hash table entry that is assumed to be from one of the caches.
 *** 
 *** @attention - Intended for use in `xhForEach()`.
 *** 
 *** @param entry The hash table entry to print.
 *** @param args Several arguments that control the printing:
 *** 	- The type of driver struct data being printed: either source data,
 *** 	  cluster data, or search data.
 *** 	- A pointer to an unsigned int storing the total number of bytes
 *** 	  used by the entry printed.
 *** 	- A pointer to an unsigned long long that counts how many uncomputed
 *** 	  caches have been skipped. Specify NULL to include uncomputed caches.
 *** 	- A key prefix to filter entries by, or NULL to print all entries.
 *** @returns 0 if successful,
 ***         -1 if an error occurs.
 ***/
static int
cluster_i_printEntry(pXHashEntry entry, va_list args)
    {
	/** Extract entry. **/
	char* key = entry->Key;
	void* data = entry->Data;
	
	/** Extract args. **/
	CIDataType data_type = (CIDataType)va_arg(args, int);
	unsigned int* total_bytes_ptr = va_arg(args, unsigned int*);
	unsigned long long* num_uncomputed_skipped_ptr = va_arg(args, unsigned long long*);
	char* path = va_arg(args, char*);
	
	/** If a path is provided, check that it matches the start of the key. **/
	if (path != NULL && strncmp(key, path, strlen(path)) != 0) return 0;
	
	/** Handle type. **/
	char* type;
	char* name;
	size_t bytes;
	switch (data_type)
	    {
	    case CI_SOURCE_DATA:
		{
		pSourceData source_data = (pSourceData)data;
		
		/** Compute size. **/
		bytes = cluster_i_sizeOfSourceData(source_data);
		
		/** If num_uncomputed_skipped_ptr is specified, skip uncomputed source. **/
		if (num_uncomputed_skipped_ptr != NULL
		    && source_data->Vectors == NULL
		) goto no_print;
		
		/** Compute printing information. **/
		type = "Source";
		name = source_data->Name;
		break;
		}
	    case CI_CLUSTER_DATA:
		{
		pClusterData cluster_data = (pClusterData)data;
		
		/** Compute size. **/
		bytes = cluster_i_sizeOfClusterData(cluster_data, false);
		
		/** If num_uncomputed_skipped_ptr is specified, skip uncomputed cluster. **/
		if (num_uncomputed_skipped_ptr != NULL
		    && cluster_data->Clusters == NULL
		) goto no_print;
		
		/** Compute printing information. **/
		type = "Cluster";
		name = cluster_data->Name;
		break;
		}
	    case CI_SEARCH_DATA:
		{
		pSearchData search_data = (pSearchData)data;
		
		/** Compute size. **/
		bytes = cluster_i_sizeOfSearchData(search_data);
		
		/** If num_uncomputed_skipped_ptr is specified, skip uncomputed search. **/
		if (num_uncomputed_skipped_ptr != NULL
		    && search_data->Pairs == NULL
		) goto no_print;
		
		/** Compute printing information. **/
		type = "Search";
		name = search_data->Name;
		break;
		}
	    default:
		mssError(0, "Cluster", "Unknown type_id %u.", data_type);
		return -1;
	    }
	
	/** Print the cache entry data. **/
	char buf[SNPRINT_BYTES_BUF_SIZE];
	snprintBytes(buf, sizeof(buf), bytes);
	printf("%-8s %-16s %-12s \"%s\"\n", type, name, buf, key);
	goto increment_total;
	
    no_print:
	if (num_uncomputed_skipped_ptr != NULL)
	    (*num_uncomputed_skipped_ptr)++;
	
    increment_total:
	if (total_bytes_ptr != NULL)
	    *total_bytes_ptr += bytes;
    
    return 0;
    }


// LINK #functions
/*** Free the source data stored in a hash table entry that is assumed to be
 *** from the source data cache.
 *** 
 *** @attention - Intended for use in `xhClearKeySafe()`.
 *** 
 *** @param entry The hash table entry to use for freeing.
 *** @param unused An unused pointer.
 ***/
static void
cluster_i_cacheFreeSourceData(pXHashEntry entry, void* unused)
    {
	/** Free the data and the key. **/
	cluster_i_freeSourceData((pSourceData)entry->Data);
	nmSysFree(entry->Key);
    
    return;
    }


// LINK #functions
/*** Free the cluster data stored in a hash table entry that is assumed to be
 *** from the cluster data cache.
 *** 
 *** @attention - Intended for use in `xhClearKeySafe()`.
 *** 
 *** @param entry The hash table entry to use for freeing.
 *** @param unused An unused pointer.
 ***/
static void
cluster_i_cacheFreeCluster(pXHashEntry entry, void* unused)
    {
	/** Free the data and the key. **/
	cluster_i_freeClusterData((pClusterData)entry->Data, false);
	nmSysFree(entry->Key);
    
    return;
    }


// LINK #functions
/*** Free the search data stored in a hash table entry that is assumed to be
 *** from the search data cache.
 *** 
 *** @attention - Intended for use in `xhClearKeySafe()`.
 *** 
 *** @param entry The hash table entry to use for freeing.
 *** @param unused An unused pointer.
 ***/
static void
cluster_i_cacheFreeSearch(pXHashEntry entry, void* unused)
    {
	/** Free the data and the key. **/
	cluster_i_freeSearchData((pSearchData)entry->Data);
	nmSysFree(entry->Key);
    
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
	/** Get driver data. **/
	pDriverData driver_data = inf_v;
	if (UNLIKELY(driver_data == NULL))
	    {
	    mssError(1, "Cluster", "Failed to execute method on NULL driver object.");
	    return -1;
	    }
	ASSERTMAGIC(driver_data, MGK_CL_DRIVER_DATA);
    
	/** Cache management method. **/
	if (strcmp(method_name, "cache") == 0)
	    {
	    char* path = NULL;
	    
	    /** Second parameter is required. **/
	    if (UNLIKELY(param->String == NULL))
		{
		mssError(1, "Cluster",
		    "[param : \"show\" | \"show_less\" | \"show_all\" | \"drop_all\"] is required for the cache method."
		);
		goto err;
		}
	    
	    /** 'show', 'show_less', and 'show_all'. **/
	    bool show = false, skip_uncomputed = false;
	    if (strcmp(param->String, "show_less") == 0)
		{
		/** Specifying show_less skips uncomputed caches. **/
		skip_uncomputed = true;
		}
	    if (skip_uncomputed || strcmp(param->String, "show") == 0)
		{
		show = true;
		path = objFilePath(driver_data->NodeData->Parent);
		}
	    if (strcmp(param->String, "show_all") == 0)
		show = true;
	    
	    /** Declare local variables to store skip counts. **/
	    unsigned long long num_uncomputed_skipped = 0;
	    unsigned long long* num_uncomputed_skipped_ptr = (skip_uncomputed)
		? &num_uncomputed_skipped
		: NULL;
	    
	    if (show)
		{
		/** Print cache info table headers. **/
		int ret = 0;
		unsigned int source_bytes = 0u, cluster_bytes = 0u, search_bytes = 0u;
		printf("\nShowing cache for ");
		if (path != NULL) printf("\"%s\":\n", path);
		else printf("all files:\n");
		printf("%-8s %-16s %-12s %s\n", "Type", "Name", "Size", "Entry CacheKey");
		
		/** Print source data cache. **/
		if (UNLIKELY(xhForEach(
		    &ClusterDriverCaches.SourceDataCache,
		    cluster_i_printEntry,
		    (int)CI_SOURCE_DATA, &source_bytes, num_uncomputed_skipped_ptr, path
		) != 0))
		    {
		    mssError(0, "Cluster", "Failed to print source data cache.");
		    ret = -1;
		    }
		
		/** Print cluster data cache. **/
		if (UNLIKELY(xhForEach(
		    &ClusterDriverCaches.ClusterDataCache,
		    cluster_i_printEntry,
		    (int)CI_CLUSTER_DATA, &cluster_bytes, num_uncomputed_skipped_ptr, path
		) != 0))
		    {
		    mssError(0, "Cluster", "Failed to print cluster data cache.");
		    ret = -1;
		    }
		
		/** Print search data cache. **/
		if (UNLIKELY(xhForEach(
		    &ClusterDriverCaches.SearchDataCache,
		    cluster_i_printEntry,
		    (int)CI_SEARCH_DATA, &search_bytes, num_uncomputed_skipped_ptr, path
		) != 0))
		    {
		    mssError(0, "Cluster", "Failed to print search data cache.");
		    ret = -1;
		    }
		    
		/** Precomputations. **/
		unsigned int total_caches = 0u
		    + (unsigned int)ClusterDriverCaches.SourceDataCache.nItems
		    + (unsigned int)ClusterDriverCaches.ClusterDataCache.nItems
		    + (unsigned int)ClusterDriverCaches.SearchDataCache.nItems;
		if (skip_uncomputed && total_caches <= num_uncomputed_skipped)
		    printf("All caches skipped, nothing to show...\n");
		
		/** Print stats. **/
		char buf[SNPRINT_BYTES_BUF_SIZE];
		printf("\nCache Stats:\n");
		printf("%-8s %-4s %-12s\n", "", "#", "Total Size");
		snprintBytes(buf, sizeof(buf), source_bytes);
		printf("%-8s %-4d %-12s\n", "Source", ClusterDriverCaches.SourceDataCache.nItems, buf);
		snprintBytes(buf, sizeof(buf), cluster_bytes);
		printf("%-8s %-4d %-12s\n", "Cluster", ClusterDriverCaches.ClusterDataCache.nItems, buf);
		snprintBytes(buf, sizeof(buf), search_bytes);
		printf("%-8s %-4d %-12s\n", "Search", ClusterDriverCaches.SearchDataCache.nItems, buf);
		snprintBytes(buf, sizeof(buf), source_bytes + cluster_bytes + search_bytes);
		printf("%-8s %-4u %-12s\n\n", "Total", total_caches, buf);
		
		/** Print skip stats (if anything was skipped). **/
		if (num_uncomputed_skipped > 0)
		    printf("Skipped %llu uncomputed caches.\n\n", num_uncomputed_skipped);
		
		return ret;
		}
	    
	    /** 'drop_all'. **/
	    if (strcmp(param->String, "drop_all") == 0)
		{
		cluster_i_clearCaches();
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
	    char buf[SNPRINT_COMMAS_LLU_BUF_SIZE];
	    printf("Cluster Driver Statistics:\n");
	    printf("  Stat Name         %12s\n", "Value");
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.OpenCalls);
	    printf("  OpenCalls         %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.OpenQueryCalls);
	    printf("  OpenQueryCalls    %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.FetchCalls);
	    printf("  FetchCalls        %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.CloseCalls);
	    printf("  CloseCalls        %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.GetTypeCalls);
	    printf("  GetTypeCalls      %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.GetValCalls);
	    printf("  GetValCalls       %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.GetValCalls_name);
	    printf("  GetValCalls_name  %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.GetValCalls_key1);
	    printf("  GetValCalls_key1  %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.GetValCalls_key2);
	    printf("  GetValCalls_key2  %12s\n", buf);
	    snprintCommasLlu(buf, sizeof(buf), ClusterStatistics.GetValCalls_sim);
	    printf("  GetValCalls_sim   %12s\n", buf);
	    printf("\n");
	    
	    nmStats();
	    
	    return 0;
	    }
	
	/** Unknown method. **/
	mssError(1, "Cluster", "Unknown command: \"%s\"", method_name);
	
	/** Attempt to give hint. **/
	cluster_i_tryHint(method_name, METHOD_NAMES, METHOD_NAMES_COUNT);
	
    err:
	mssError(0, "Cluster", "Failed to execute command.");
	
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
	fprintf(stderr, "HINT: Use queries instead (e.g. clusterOpenQuery()).\n");
    
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
    
    return -1;
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
	pObjDriver drv = nmMalloc(sizeof(ObjDriver));
	if (UNLIKELY(drv == NULL))
	    {
	    mssError(1, "Cluster", "nmMalloc(%zu) failed.", sizeof(ObjDriver));
	    goto err_free;
	    }
	memset(drv, 0, sizeof(ObjDriver));
	
	/** Initialize caches. **/
	// memset(&ClusterDriverCaches, 0, sizeof(ClusterDriverCaches));
	if (UNLIKELY(xhInit(&ClusterDriverCaches.SourceDataCache, CI_CACHE_HASHTABLE_ROWS, 0) != 0))
	    {
	    mssError(1, "Cluster", "Failed to allocate the hash table for the source data cache.");
	    goto err_free;
	    }
	if (UNLIKELY(xhInit(&ClusterDriverCaches.ClusterDataCache, CI_CACHE_HASHTABLE_ROWS, 0) != 0))
	    {
	    mssError(1, "Cluster", "Failed to allocate the hash table for the cluster data cache.");
	    goto err_free;
	    }
	if (UNLIKELY(xhInit(&ClusterDriverCaches.SearchDataCache, CI_CACHE_HASHTABLE_ROWS, 0) != 0))
	    {
	    mssError(1, "Cluster", "Failed to allocate the hash table for the search data cache.");
	    goto err_free;
	    }
	
	/** Set up the structure. **/
	if (strtcpy(drv->Name, "cluster - Clustering Driver", sizeof(drv->Name)) < 0)
	    {
	    mssError(1, "Cluster", "Failed to write driver name.");
	    goto err_free;
	    }
	if (UNLIKELY(xaInit(&drv->RootContentTypes, 1) != 0))
	    {
	    mssError(1, "Cluster", "Failed to allocate the XArray table for the driver's root content types.");
	    goto err_free;
	    }
	if (UNLIKELY(xaAddItem(&drv->RootContentTypes, "system/cluster") < 0))
	    {
	    mssError(1, "Cluster", "Failed to add \"system/cluster\" to the drv->RootContentTypes XArray.");
	    goto err_free;
	    }
	
	drv->Capabilities = 0; /* TODO: Greg - Should I indicate any capabilities? */
	
	/** Set up the function references. **/
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
	if (objRegisterDriver(drv) != 0)
	    {
	    mssError(1, "Cluster", "Failed to register driver.");
	    goto err_free;
	    }
	
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
	if (ClusterDriverCaches.SourceDataCache.nRows != 0) warnFail(xhDeInit(&ClusterDriverCaches.SourceDataCache));
	if (ClusterDriverCaches.ClusterDataCache.nRows != 0) warnFail(xhDeInit(&ClusterDriverCaches.ClusterDataCache));
	if (ClusterDriverCaches.SearchDataCache.nRows != 0) warnFail(xhDeInit(&ClusterDriverCaches.SearchDataCache));
	if (drv != NULL)
	    {
	    if (drv->RootContentTypes.nAlloc != 0) warnFail(xaDeInit(&drv->RootContentTypes));
	    nmFree(drv, sizeof(ObjDriver));
	    }
	
	mssError(0, "Cluster", "Failed to initialize cluster driver.");
	
	return -1;
    }
