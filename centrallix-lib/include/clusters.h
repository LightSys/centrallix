
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
/* Module:      lib_cluster.c                                           */
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

pVector ca_build_vector(const char* str);
unsigned int ca_sparse_len(const pVector vector);
void ca_free_vector(pVector sparse_vector);
void ca_kmeans(
    pVector* vectors,
    const unsigned int num_vectors,
    unsigned int* labels,
    const unsigned int num_clusters,
    const unsigned int max_iter,
    const double improvement_threshold
);
pXArray ca_search(
    pVector* vectors,
    const unsigned int num_vectors,
    const unsigned int* labels,
    const double dupe_threshold
);
pXArray ca_lightning_search(
    pVector* vectors,
    const unsigned int num_vectors,
    const double dupe_threshold
);
unsigned int ca_edit_dist(
    const char* str1,
    const char* str2,
    const size_t str1_length,
    const size_t str2_length
);
pXArray ca_phone_search(
    char dataset[][10u],
    const unsigned int dataset_size,
    const double dupe_threshold
);
void ca_init();
