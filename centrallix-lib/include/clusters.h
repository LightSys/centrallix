#ifndef CLUSTERS_H
#define	CLUSTERS_H

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
/* Module:      lib_cluster.h                                           */
/* Author:      Israel Fuller                                           */
/* Creation:    September 29, 2025                                      */
/* Description: Internal algorithms for the cluster object driver.      */
/* See centrallix-sysdoc/EAV_Pivot.md for more information.             */
/************************************************************************/

#include <stdlib.h>

#ifdef CXLIB_INTERNAL
#include "xarray.h"
#else
#include "cxlib/xarray.h"
#endif

#define CA_NUM_DIMS 251 /* aka. The vector table size. */

/// LINK ../../centrallix-sysdoc/string_comparison.md#cosine_charsets
/** The character used to create a pair with the first and last characters of a string. **/
#define CA_BOUNDARY_CHAR ('a' - 1)

/** Types. **/
typedef int* pVector;      /* Sparse vector. */
typedef double* pCentroid; /* Dense centroid. */
#define pCentroidSize CA_NUM_DIMS * sizeof(double)

/** Duplocate information. **/
typedef struct
    {
    unsigned int id1;
    unsigned int id2;
    double similarity;
    }
    Dup, *pDup;

/** Registering all defined types for debugging. **/
#define ca_init() \
    nmRegister(sizeof(pVector), "pVector"); \
    nmRegister(sizeof(pCentroid), "pCentroid"); \
    nmRegister(pCentroidSize, "Centroid"); \
    nmRegister(sizeof(Dup), "Dup")

pVector ca_build_vector(const char* str);
unsigned int ca_sparse_len(const pVector vector);
void ca_free_vector(pVector sparse_vector);
int ca_kmeans(
    pVector* vectors,
    const unsigned int num_vectors,
    const unsigned int num_clusters,
    const unsigned int max_iter,
    const double min_improvement,
    unsigned int* labels,
    double* vector_sims);

/** Vector helper macros. **/
#define ca_is_empty(vector) (vector[0] == -CA_NUM_DIMS)
#define ca_has_no_pairs(vector) \
    ({ \
    __typeof__ (vector) _v = (vector); \
    _v[0] == -172 && _v[1] == 11 && _v[2] == -78; \
    })

/** Comparison functions, for ca_search(). **/
double ca_cos_compare(void* v1, void* v2);
double ca_lev_compare(void* str1, void* str2);

void* ca_most_similar(
    void* target,
    void** data,
    const unsigned int num_data,
    const double (*similarity)(void*, void*),
    const double threshold);
pXArray ca_sliding_search(
    void** data,
    const unsigned int num_data,
    const unsigned int window_size,
    const double (*similarity)(void*, void*),
    const double dupe_threshold,
    pXArray dups);
pXArray ca_complete_search(
    void** data,
    const unsigned int num_data,
    const double (*similarity)(void*, void*),
    const double dupe_threshold,
    pXArray dups);

#endif /* End of .h file. */
