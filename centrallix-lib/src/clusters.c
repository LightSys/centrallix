
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

#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "clusters.h"
#include "newmalloc.h"
#include "util.h"
#include "xarray.h"

/*** Gets the hash, representing a pair of ASCII characters, represented by unsigned ints.
 *** Thank you to professor John Delano for this hashing algorithm.
 *** 
 *** @param num1 The first character in the pair.
 *** @param num1 The second character in the pair.
 *** @returns The resulting hash.
 ***/
static unsigned int hash_char_pair(const unsigned int num1, const unsigned int num2)
    {
    if (num1 == CA_BOUNDARY_CHAR && num2 == CA_BOUNDARY_CHAR)
	{
	// fprintf(stderr,
	//     "hash_char_pair(%u, %u) - Warning: Pair of boundary characters.\n",
	//     num1, num2
	// );
	}
    const double sum = (num1 * num1 * num1) + (num2 * num2 * num2);
    const double scale = ((double)num1 + 1.0) / ((double)num2 + 1.0);
    const unsigned int hash = (unsigned int)round(sum * scale) - 1u;
    return hash % CA_NUM_DIMS;
    }

/*** Builds a vector using a string.
 *** 
 *** Vectors are based on the frequencies of character pairs in the string.
 *** Space characters and punctuation characters (see code for list) are ignored,
 *** and all characters are converted to lowercase. Character 96, which is just
 *** before 'a' in the ASCII table (and maps to '`') is used to make pairs on the
 *** start and end of strings. The only supported characters for the passed char*
 *** are spaces, punctuation, uppercase and lowercase letters, and numbers.
 *** 
 *** This results in the following modified ASCII table:
 *** ```csv
 *** #,   char, #,   char, #,   char
 *** 97,  a,    109, m,    121, y
 *** 98,  b,    110, n,    122, z
 *** 99,  c,    111, o,    123, 0
 *** 100, d,    112, p,    124, 1
 *** 101, e,    113, q,    125, 2
 *** 102, f,    114, r,    126, 3
 *** 103, g,    115, s,    127, 4
 *** 104, h,    116, t,    128, 5
 *** 105, i,    117, u,    129, 6
 *** 106, j,    118, v,    130, 7
 *** 107, k,    119, w,    131, 8
 *** 108, l,    120, x,    132, 9
 *** ```
 *** Thus, any number from 96 (the start/end character) to 132 ('9') is a valid
 *** input to get_char_pair_hash().
 *** 
 *** After hashing each character pair, we add some number from 1 to 13 to the
 *** coresponding dimention. However, for most names, this results in a lot of
 *** zeros and a FEW positive numbers. Thus, after creating the dense vector,
 *** we convert it to a sparse vector in which a negative number replaces a run
 *** of that many zeros. Consider the following example:
 *** 
 *** Dense pVector: `[1,0,0,0,3,0]`
 *** 
 *** Sparse pVector: `[1,-3,3,-1]`
 *** 
 *** Using these sparse vectors greatly reduces the required memory and gives
 *** aproximately an x5 boost to performance when traversing vectors, at the
 *** cost of more algorithmically complex code.
 *** 
 *** @param str The string to be divided into pairs and hashed to make the vector.
 *** @returns The sparse vector built using the hashed character pairs.
 ***/
pVector ca_build_vector(const char* str)
    {
    /** Allocate space for a dense vector. **/
    unsigned int dense_vector[CA_NUM_DIMS] = {0u};
    
    /** j is the former character, i is the latter. **/
    const unsigned int num_chars = (unsigned int)strlen(str);
    for (unsigned int j = 65535u, i = 0u; i <= num_chars; i++)
	{
	/** isspace: space, \n, \v, \f, \r **/
	if (isspace(str[i])) continue;
	
	/** ispunct: !"#$%&'()*+,-./:;<=>?@[\]^_{|}~ **/
	if (ispunct(str[i]) && str[i] != CA_BOUNDARY_CHAR) continue;
	
	/*** iscntrl (0-8):   NULL, SOH, STX, ETX, EOT, ENQ, ACK, BEL, BS
	 ***         (14-31): SO, SI, DLE, DC1-4, NAK, SYN, ETB, CAN EM,
	 ***                  SUB, ESC, FS, GS, RS, US
	 ***/
	if (iscntrl(str[i]) && i != num_chars)
	    {
	    fprintf(stderr,
		"ca_build_vector(%s) - Warning: Skipping unknown character #%u.\n",
		str, (unsigned int)str[i]
	    );
	    continue;
	    }
	
	/** First and last character should fall one before 'a' in the ASCII table. **/
	unsigned int temp1 = (j == 65535u) ? CA_BOUNDARY_CHAR : (unsigned int)tolower(str[j]);
	unsigned int temp2 = (i == num_chars) ? CA_BOUNDARY_CHAR : (unsigned int)tolower(str[i]);
	
	/** Shift numbers to the end of the lowercase letters. **/
	if ('0' <= temp1 && temp1 <= '9') temp1 += 75u;
	if ('0' <= temp2 && temp2 <= '9') temp2 += 75u;
	
	/** Hash the character pair into an index (dimension).  **/
	/** Note that temp will be between 97 ('a') and 132 ('9'). **/
	unsigned int dim = hash_char_pair(temp1, temp2);
	
	/** Increment the dimension of the dense vector by a number from 1 to 13. **/
	dense_vector[dim] += (temp1 + temp2) % 13u + 1u;
	
	j = i;
	}
    
    /** Count how much space is needed for a sparse vector. **/
    bool zero_prev = false;
    size_t size = 0u;
    for (unsigned int dim = 0u; dim < CA_NUM_DIMS; dim++)
	{
	if (dense_vector[dim] == 0u)
	    {
	    size += (zero_prev) ? 0u : 1u;
	    zero_prev = true;
	    }
	else
	    {
	    size++;
	    zero_prev = false;
	    }
	}
    
    /*** Check compression size.
     *** If this check fails, I doubt anything will break. However, the longest
     *** word I know (supercalifragilisticexpialidocious) has only 35 character
     *** pairs, so it shouldn't reach half this size (and it'd be even shorter
     *** if the hash generates at least one collision).
     *** 
     *** Bad vector compression will result in degraded performace and increased
     *** memory usage. This indicates a likely bug in the code. Thus, if this
     *** warning is ever generated, it is definitely worth investigating.
     ***/
    const size_t expected_max_size = 64u;
    if (size > expected_max_size)
	{
	fprintf(stderr,
	    "cli_build_vector(%s) - Warning: Sparse vector larger than expected.\n"
	    "    > Size: %lu\n"
	    "    > #Dims: %u\n",
	    str,
	    size,
	    CA_NUM_DIMS
	);
	}
    
    /** Allocate space for sparse vector. **/
    const size_t sparse_vector_size = size * sizeof(int);
    pVector sparse_vector = (pVector)nmSysMalloc(sparse_vector_size);
    if (sparse_vector == NULL)
	{
	fprintf(stderr,
	    "cli_build_vector(%s) - nmSysMalloc(%lu) failed.\n",
	    str, sparse_vector_size
	);
	return NULL;
	}
    
    /** Convert the dense vector above to a sparse vector. **/
    unsigned int j = 0u, sparse_idx = 0u;
    while (j < CA_NUM_DIMS)
        {
	if (dense_vector[j] == 0u)
	    {
	    /*** Count and store consecutive zeros, except the first one,
	     *** which we already know is zero.
	     ***/
	    unsigned int zero_count = 1u;
	    j++;
	    while (j < CA_NUM_DIMS && dense_vector[j] == 0u)
	        {
		zero_count++;
		j++;
	        }
	    sparse_vector[sparse_idx++] = (int)-zero_count;
	    }
	else
	    {
	    /** Store the value. **/
	    sparse_vector[sparse_idx++] = (int)dense_vector[j++];
	    }
	}
    
    return sparse_vector;
    }

/*** Free memory allocated to store a sparse vector.
 *** 
 *** @param sparse_vector The sparse vector being freed.
 ***/
void ca_free_vector(pVector sparse_vector)
    {
    nmSysFree(sparse_vector);
    }

/*** Compute the magnitude of a sparsely allocated vector.
 *** 
 *** @param vector The vector.
 *** @returns The computed magnitude.
 ***/
static double magnitude_sparse(const pVector vector)
    {
    unsigned int magnitude = 0u;
    for (unsigned int i = 0u, dim = 0u; dim < CA_NUM_DIMS;)
	{
	const int val = vector[i++];
	
	/** Negative val represents -val 0s in the array, so skip that many values. **/
	if (val < 0) dim += (unsigned)(-val);
	
	/** We have a param_value, so square it and add it to the magnitude. **/
	else { magnitude += (unsigned)(val * val); dim++; }
	}
    return sqrt((double)magnitude);
    }

/*** Compute the length of a sparsely allocated vector.
 *** 
 *** @param vector The vector.
 *** @returns The computed length.
 ***/
unsigned int ca_sparse_len(const pVector vector)
    {
    unsigned int i = 0u;
    for (unsigned int dim = 0u; dim < CA_NUM_DIMS;)
	{
	const int val = vector[i++];
	
	/** Negative val represents -val 0s in the array, so skip that many values. **/
	if (val < 0) dim += (unsigned)(-val);
	
	/** We have a param_value, but we don't need to do anything with it. **/
	else dim++;
	}
    return i;
    }

/*** Compute the magnitude of a densely allocated centroid.
 *** 
 *** @param centroid The centroid.
 *** @returns The computed magnitude.
 ***/
static double magnitude_dense(const pCentroid centroid)
    {
    double magnitude = 0.0;
    for (int i = 0; i < CA_NUM_DIMS; i++)
	magnitude += centroid[i] * centroid[i];
    return sqrt(magnitude);
    }

/*** Parse a token from a sparsely allocated vector and write the param_value and
 *** number of remaining values to the passed locations.
 *** 
 *** @param token The sparse vector token being parsed.
 *** @param remaining The location to save the remaining number of characters.
 *** @param param_value The location to save the param_value of the token.
 ***/
static void parse_vector_token(const int token, unsigned int* remaining, unsigned int* param_value)
    {
    if (token < 0)
	{
	/** This run contains -token zeros. **/
	*remaining = (unsigned)(-token);
	*param_value = 0u;
	}
    else
	{
	/** This run contains one param_value. **/
	*remaining = 1u;
	*param_value = (unsigned)(token);
	}
    }

/*** Calculate the similarity on sparcely allocated vectors. Comparing
 *** any string to an empty string should always return 0.5 (untested).
 *** 
 *** @param v1 Sparse vector #1.
 *** @param v2 Sparse vector #2.
 *** @returns Similarity between 0 and 1 where
 ***     1 indicates identical and
 ***     0 indicates completely different.
 ***/
static double sparse_similarity(const pVector v1, const pVector v2)
    {
    /** Calculate dot product. **/
    unsigned int vec1_remaining = 0u, vec2_remaining = 0u;
    unsigned int dim = 0u, i1 = 0u, i2 = 0u, dot_product = 0u;
    while (dim < CA_NUM_DIMS)
	{
	unsigned int val1 = 0u, val2 = 0u;
	if (vec1_remaining == 0u) parse_vector_token(v1[i1++], &vec1_remaining, &val1);
	if (vec2_remaining == 0u) parse_vector_token(v2[i2++], &vec2_remaining, &val2);
	
	/*** Accumulate the dot_product. If either vector is 0 here,
	 *** the total is 0 and this statement does nothing.
	 ***/
	dot_product += val1 * val2;
	
	/** Consume overlap from both runs. **/
	unsigned int overlap = min(vec1_remaining, vec2_remaining);
	vec1_remaining -= overlap;
	vec2_remaining -= overlap;
	dim += overlap;
	}
    
    /** Optional optimization to speed up nonsimilar vectors. **/
    if (dot_product == 0u) return 0.0;
    
    /** Return the difference score. **/
    return (double)dot_product / (magnitude_sparse(v1) * magnitude_sparse(v2));
    }

/*** Calculate the difference on sparcely allocated vectors. Comparing
 *** any string to an empty string should always return 0.5 (untested).
 *** 
 *** @param v1 Sparse vector #1.
 *** @param v2 Sparse vector #2.
 *** @returns Similarity between 0 and 1 where
 ***     1 indicates completely different and
 ***     0 indicates identical.
 ***/
#define sparse_dif(v1, v2) (1.0 - sparse_similarity(v1, v2))

/*** Calculate the similarity between a sparsely allocated vector
 *** and a densely allocated centroid. Comparing any string to an
 *** empty string should always return 0.5 (untested).
 *** 
 *** @param v1 Sparse vector #1.
 *** @param c1 Dense centroid #2.
 *** @returns Similarity between 0 and 1 where
 ***     1 indicates identical and
 ***     0 indicates completely different.
 ***/
static double sparse_similarity_to_centroid(const pVector v1, const pCentroid c2)
    {
    /** Calculate dot product. **/
    double dot_product = 0.0;
    for (unsigned int i = 0u, dim = 0u; dim < CA_NUM_DIMS;)
	{
	const int val = v1[i++];
	
	/** Negative val represents -val 0s in the array, so skip that many values. **/
	if (val < 0) dim += (unsigned)(-val);
	
	/** We have a param_value, so square it and add it to the magnitude. **/
	else dot_product += (double)val * c2[dim++];
	}
    
    /** Return the difference score. **/
    return dot_product / (magnitude_sparse(v1) * magnitude_dense(c2));
    }

/*** Calculate the difference between a sparsely allocated vector
 *** and a densely allocated centroid. Comparing any string to an
 *** empty string should always return 0.5 (untested).
 *** 
 *** @param v1 Sparse vector #1.
 *** @param c1 Dense centroid #2.
 *** @returns Difference between 0 and 1 where
 ***     1 indicates completely different and
 ***     0 indicates identical.
 ***/
#define sparse_dif_to_centroid(v1, c2) (1.0 - sparse_similarity_to_centroid(v1, c2))

/*** Calculate the average size of all clusters in a set of vectors.
 *** 
 *** @param vectors The vectors of the dataset (allocated sparsely).
 *** @param num_vectors The number of vectors in the dataset.
 *** @param labels The clusters to which vectors are assigned.
 *** @param centroids The locations of the centroids (allocated densely).
 *** @param num_clusters The number of centroids (k).
 *** @returns The average cluster size.
 ***/
static double get_cluster_size(
    pVector* vectors,
    const unsigned int num_vectors,
    unsigned int* labels,
    pCentroid* centroids,
    const unsigned int num_clusters)
    {
    /** Could be up to around 1KB on the stack, but I think that's fine. **/
    double cluster_sums[num_clusters];
    unsigned int cluster_counts[num_clusters];
    for (unsigned int i = 0u; i < num_clusters; i++)
	cluster_sums[i] = 0.0;
    memset(cluster_counts, 0, sizeof(cluster_counts));
    
    /** Sum the difference from each vector to its cluster centroid. **/
    for (unsigned int i = 0u; i < num_vectors; i++)
	{
	const unsigned int label = labels[i];
	cluster_sums[label] += sparse_dif_to_centroid(vectors[i], centroids[label]);
	cluster_counts[label]++;
	}
    
    /** Add up the average cluster size. **/
    double cluster_total = 0.0;
    unsigned int num_valid_clusters = 0u;
    for (unsigned int label = 0u; label < num_clusters; label++)
	{
	const unsigned int cluster_count = cluster_counts[label];
	if (cluster_count == 0u) continue;
	
	cluster_total += cluster_sums[label] / cluster_count;
	num_valid_clusters++;
	}
    
    /** Return average sizes. **/
    return cluster_total / num_valid_clusters;
    }

/*** Compute the param_value for `k` (number of clusters), given a dataset of with
 *** a size of `n`.
 *** 
 *** The following table shows data sizes vs.selected cluster size. In testing,
 *** these numbers tended to givea good balance of accuracy and dulocates detected.
 *** 
 *** ```csv
 *** Data Size, Actual
 *** 10k,       12
 *** 100k,      33
 *** 1M,        67
 *** 4M,        93
 *** ```
 *** 
 *** This function is not intended for datasets smaller than (`n < ~2000`).
 *** These should be handled using complete search.
 *** 
 *** LaTeX Notation: \log_{36}\left(n\right)^{3.1}-8
 *** 
 *** @param n The size of the dataset.
 *** @returns k, the number of clusters to use.
 *** 
 *** Complexity: `O(1)`
 ***/
unsigned int compute_k(const unsigned int n)
    {
    return (unsigned)max(2, pow(log(n) / log(36), 3.2) - 8);
    }

/*** Executes the k-means clustering algorithm. Selects NUM_CLUSTERS random
 *** vectors as initial centroids. Then points are assigned to the nearest
 *** centroid, after which centroids are moved to the center of their points.
 *** 
 *** @param vectors The vectors to cluster.
 *** @param num_vectors The number of vectors to cluster.
 *** @param labels Stores the final cluster identities of the vectors after
 ***     clustering is completed.
 *** @param centroids Stores the locations of the centroids used for the clusters
 ***     of the data.
 *** @param iterations The number of iterations that actually executed is stored
 ***     here. Leave this NULL if you don't care.
 *** @param max_iter The max number of iterations.
 *** @param num_clusters The number of clusters to generate.
 ***
 *** @attention - Assumes: num_vectors is the length of vectors.
 *** @attention - Assumes: num_clusters is the length of labels.
 ***
 *** @attention - Issue: At larger numbers of clustering iterations, some
 ***     clusters have a size of    negative infinity. In this implementation,
 ***     the bug is mitigated by setting a small number of max iterations,
 ***     such as 16 instead of 100.
 *** @attention - Issue: Clusters do not apear to improve much after the first
 ***     iteration, which puts the efficacy of the algorithm into question. This
 ***     may be due to the uneven density of a typical dataset. However, the
 ***     clusters still offer useful information.
 *** 
 *** Complexity:
 *** 
 *** - `O(kd + k + i*(k + n*(k+d) + kd))`
 *** 
 *** - `O(kd + k + ik + ink + ind + ikd)`
 *** 
 *** - `O(nk + nd)`
 ***/
void ca_kmeans(
    pVector* vectors,
    const unsigned int num_vectors,
    unsigned int* labels,
    const unsigned int num_clusters,
    const unsigned int max_iter,
    const double improvement_threshold)
    {
    /** Ensure labels is clean. **/
    memset(labels, 0, num_clusters * sizeof(unsigned int));
    
    /** Allocate space to store centroids and new_centroids. **/
    /** Dynamic allocation is required because these densely allocated arrays might be up to 500KB! **/
    pCentroid* centroids = (pCentroid*)nmMalloc(num_clusters * sizeof(pCentroid));
    if (centroids == NULL)
	{
	fprintf(stderr, "ca_kmeans() - nmMalloc(%lu) failed.\n", num_clusters * sizeof(pCentroid));
	assert(false);
	}
    pCentroid* new_centroids = (pCentroid*)nmMalloc(num_clusters * sizeof(pCentroid));
    if (new_centroids == NULL)
	{
	fprintf(stderr, "ca_kmeans() - nmMalloc(%lu) failed.\n", num_clusters * sizeof(pCentroid));
	assert(false);
	}
    for (unsigned int i = 0u; i < num_clusters; i++)
	{
	/** Malloc each centroid. **/
	centroids[i] = (pCentroid)nmMalloc(pCentroidSize);
	if (centroids[i] == NULL)
	    {
	    fprintf(stderr, "ca_kmeans() - nmMalloc(%lu) failed.\n", pCentroidSize);
	    assert(false);
	    }
	memset(centroids[i], 0, pCentroidSize);
	
	/** Malloc each new centroid. **/
	new_centroids[i] = (pCentroid)nmMalloc(pCentroidSize);
	if (new_centroids[i] == NULL)
	    {
	    fprintf(stderr, "ca_kmeans() - nmMalloc(%lu) failed.\n", pCentroidSize);
	    assert(false);
	    }
	memset(new_centroids[i], 0, pCentroidSize);
	}
    
    /** Select random vectors to use as the initial centroids. **/
    srand(time(NULL));
    for (unsigned int i = 0u; i < num_clusters; i++)
	{
	// Pick a random vector.
	const pVector vector = vectors[rand() % num_vectors];
	
	// Sparse copy the vector to expand it into a densely allocated centroid.
	pCentroid centroid = centroids[i];
	for (unsigned int i = 0u, dim = 0u; dim < CA_NUM_DIMS;)
	    {
	    const int token = vector[i++];
	    if (token > 0) centroid[dim++] = (double)token;
	    else for (unsigned int j = 0u; j < -token; j++) centroid[dim++] = 0.0;
	    }
	}
    
    /** Main kmeans loop. **/
    double old_average_cluster_size = 1.0;
    unsigned int cluster_counts[num_clusters];
    for (unsigned int iter = 0u; iter < max_iter; iter++)
	{
	bool changed = false;
	
	/** Reset new centroids. **/
	for (unsigned int i = 0u; i < num_clusters; i++)
	    {
	    cluster_counts[i] = 0u;
	    for (unsigned int dim = 0; dim < CA_NUM_DIMS; dim++)
		new_centroids[i][dim] = 0.0;
	    }
	
	/** Assign each point to the nearest centroid. **/
	for (unsigned int i = 0u; i < num_vectors; i++)
	    {
	    const pVector vector = vectors[i];
	    double min_dist = DBL_MAX;
	    unsigned int best_centroid_label = 0u;
	    
	    // Find nearest centroid.
	    for (unsigned int j = 0u; j < num_clusters; j++)
		{
		const double dist = sparse_dif_to_centroid(vector, centroids[j]);
		if (dist < min_dist)
		    {
		    min_dist = dist;
		    best_centroid_label = j;
		    }
		}
		
	    /** Update label to new centroid, if necessary. **/
	    if (labels[i] != best_centroid_label)
		{
		labels[i] = best_centroid_label;
		changed = true;
		}
	    
	    /** Accumulate values for new centroid calculation. **/
	    pCentroid best_centroid = new_centroids[best_centroid_label];
	    for (unsigned int i = 0u, dim = 0u; dim < CA_NUM_DIMS;)
		{
		const int val = vector[i++];
		if (val < 0) dim += (unsigned)(-val);
		else best_centroid[dim++] += (double)val;
		}
	    cluster_counts[best_centroid_label]++;
	    }
	
	/** Stop if centroids didn't change. **/
	if (!changed) break;
	
	/** Update centroids. **/
	for (unsigned int i = 0u; i < num_clusters; i++)
	    {
	    if (cluster_counts[i] == 0u) continue;
	    pCentroid centroid = centroids[i];
	    const pCentroid new_centroid = new_centroids[i];
	    const unsigned int cluster_count = cluster_counts[i];
	    for (unsigned int dim = 0u; dim < CA_NUM_DIMS; dim++)
		centroid[dim] = new_centroid[dim] / cluster_count;
	    }
	
	/** Is there enough improvement? **/
	const double average_cluster_size = get_cluster_size(vectors, num_vectors, labels, centroids, num_clusters);
	const double improvement = old_average_cluster_size - average_cluster_size;
	if (improvement < improvement_threshold) break;
	old_average_cluster_size = average_cluster_size;
	}
    
    /** Clean up. **/
    for (unsigned int i = 0u; i < num_clusters; i++)
	{
	nmFree(centroids[i], pCentroidSize);
	nmFree(new_centroids[i], pCentroidSize);
	}
    nmFree(centroids, num_clusters * sizeof(pCentroid));
    nmFree(new_centroids, num_clusters * sizeof(pCentroid));
    }

pXArray ca_search(
    pVector* vectors,
    const unsigned int num_vectors,
    const unsigned int* labels,
    const double dupe_threshold)
    {
    /** Allocate space for dups. **/
    pXArray dups = xaNew(num_vectors);
    if (dups == NULL)
	{
	fprintf(stderr, "ca_search() - xaNew(%u) failed.\n", num_vectors);
	return NULL;
	}
    
    unsigned int a = 0, b = 0, c = 0, d = 0;
    for (unsigned int i = 0u; i < num_vectors; i++)
	{
	const pVector v1 = vectors[i];
	const unsigned int label = labels[i];
	for (unsigned int j = i + 1u; j < num_vectors; j++)
	    {
	    if (b++ % 100 == 0) printf(".");
	    if (labels[j] != label) continue;
	    if (c++ % 100 == 0) printf(":");
	    const pVector v2 = vectors[j];
	    const double similarity = sparse_similarity(v1, v2);
	    if (similarity > dupe_threshold) /* Dup found! */
		{
		Dup* dup = (Dup*)nmMalloc(sizeof(Dup));
		if (dup == NULL)
		    {
		    fprintf(stderr,
			"ca_search() - nmMalloc(%lu) failed.\n",
			sizeof(Dup)
		    );
		    goto err_free_dups;
		    }
		
		dup->id1 = i;
		dup->id2 = j;
		dup->similarity = similarity;
		xaAddItem(dups, (void*)dup);
		if (d++ % 4 == 0) printf("!");
		}
	    }
	if (a++ % 4 == 0) printf("\n");
	}
    
    return dups;
    
    /** Free dups. **/
    err_free_dups:;
    const size_t num_dups = dups->nItems;
    for (unsigned int i = 0u; i < num_dups; i++)
	{
	nmFree(dups->Items[i], sizeof(Dup));
	dups->Items[i] = NULL;
	}
    xaDeInit(dups);
    return NULL;
    }

/*** Runs complete search to find duplocates if `num_vectors <  MAX_COMPLETE_SEARCH`
 *** and runs a search using k-means clustering on larger amounts of data.
 *** 
 *** @param vectors Array of precomputed frequency vectors for all dataset strings.
 *** @param num_vectors The number of vectors to be scanned.
 *** @param dupe_threshold The similarity threshold, below which dups are ignored.
 *** @returns The duplicates in pDup structs.
 ***/
pXArray ca_lightning_search(pVector* vectors, const unsigned int num_vectors, const double dupe_threshold)
    {
    /** Allocate space for dups. **/
    const size_t guess_size = num_vectors * 2u;
    pXArray dups = xaNew(guess_size);
    if (dups == NULL)
	{
	fprintf(stderr, "ca_lightning_search() - xaNew(%lu) failed.\n", guess_size);
	return NULL;
	}
    
    /** Descide which algorithm to use. **/
    if (num_vectors <= 50 * 1000)
	{ /** Do a complete search. **/
	for (unsigned int i = 0u; i < num_vectors; i++)
	    {
	    const pVector v1 = vectors[i];
	    for (unsigned int j = i + 1u; j < num_vectors; j++)
		{
		const pVector v2 = vectors[j];
		const double similarity = sparse_similarity(v1, v2);
		if (similarity > dupe_threshold) // Dup found!
		    {
		    Dup* dup = (Dup*)nmMalloc(sizeof(Dup));
		    if (dup == NULL)
			{
			fprintf(stderr, "ca_lightning_search() - nmMalloc(%lu) failed.\n", sizeof(Dup));
			goto err_free_dups;
			}
		    
		    dup->id1 = i;
		    dup->id2 = j;
		    dup->similarity = similarity;
		    xaAddItem(dups, (void*)dup);
		    }
		}
	    }
	}
    else
	{ /** Do a k-means search. **/
	/** Define constants for the algorithm. **/
	const unsigned int max_iter = 64u; /** Hardcode value because idk. **/
	const unsigned int num_clusters = compute_k(num_vectors);
	
	/** Allocate static memory for finding clusters. **/
	unsigned int labels[num_vectors];
	memset(labels, 0u, sizeof(labels));
	
	/** Execute kmeans clustering. **/
	ca_kmeans(vectors, num_vectors, labels, num_clusters, max_iter, 0.0002);
	
	/** Find duplocates in clusters. **/
	for (unsigned int i = 0u; i < num_vectors; i++)
	    {
	    const pVector v1 = vectors[i];
	    const unsigned int label = labels[i];
	    for (unsigned int j = i + 1u; j < num_vectors; j++)
		{
		if (labels[j] != label) continue;
		const pVector v2 = vectors[j];
		const double similarity = sparse_similarity(v1, v2);
		if (similarity > dupe_threshold) /* Dup found! */
		    {
		    Dup* dup = (Dup*)nmMalloc(sizeof(Dup));
		    if (dup == NULL)
			{
			fprintf(stderr,
			    "ca_lightning_search() - nmMalloc(%lu) failed.\n",
			    sizeof(Dup)
			);
			goto err_free_dups;
			}
		    
		    dup->id1 = i;
		    dup->id2 = j;
		    dup->similarity = similarity;
		    xaAddItem(dups, (void*)dup);
		    }
		}
	    }
	}
    
    /** Done **/
    return dups;
    
    /** Free dups. **/
    err_free_dups:;
    const size_t num_dups = dups->nItems;
    for (unsigned int i = 0u; i < num_dups; i++)
	{
	nmFree(dups->Items[i], sizeof(Dup));
	dups->Items[i] = NULL;
	}
    xaDeInit(dups);
    return NULL;
    }

/*** Computes Levenshtein distance between two strings.
 *** 
 *** @param str1 The first string.
 *** @param str2 The second string.
 *** @param length1 The length of the first string.
 *** @param length1 The length of the first string.
 *** 
 *** @attention - Tip: Pass 0 for the length of either string to infer it
 *** 	using the null terminating character. Thus, strings with no null
 *** 	terminator are supported if you pass explicit lengths.
 ***  
 *** Complexity: O(length1 * length2).
 *** 
 *** @see centrallix-sysdoc/string_comparison.md
 ***/
unsigned int ca_edit_dist(const char* str1, const char* str2, const size_t str1_length, const size_t str2_length)
    {
    /*** lev_matrix:
     *** For all i and j, d[i][j] will hold the Levenshtein distance between
     *** the first i characters of s and the first j characters of t.
     *** 
     *** As they say, no dynamic programming algorithm is complete without a
     *** matrix that you fill out and it has the answer in the final location.
     ***/
    const size_t str1_len = (str1_length == 0u) ? strlen(str1) : str1_length;
    const size_t str2_len = (str2_length == 0u) ? strlen(str2) : str2_length;
    unsigned int lev_matrix[str1_len + 1][str2_len + 1];
    
    /*** Base case #0:
     *** Transforming an empty string into an empty string has 0 cost.
     ***/
    lev_matrix[0][0] = 0u;
    
    /*** Base case #1:
     *** Any source prefixe can be transformed into an empty string by
     *** dropping each character.
     ***/
    for (unsigned int i = 1u; i <= str1_len; i++)
	lev_matrix[i][0] = i;
    
    /*** Base case #2:
     *** Any target prefixes can be transformed into an empty string by
     *** inserting each character.
     ***/
    for (unsigned int j = 1u; j <= str2_len; j++)
	lev_matrix[0][j] = j;
    
    /** General Case **/
    for (unsigned int i = 1u; i <= str1_len; i++)
	{
	for (unsigned int j = 1u; j <= str2_len; j++)
	    {
	    /** Equal characters need no changes. **/
	    if (str1[i - 1] == str2[j - 1])
		lev_matrix[i][j] = lev_matrix[i - 1][j - 1];
	    
	    /*** We need to make a change, so use the opereration with the
	     *** lowest cost out of delete, insert, replace, or swap.
	     ***/
	    else 
		{
		unsigned int cost_delete  = lev_matrix[i - 1][j] + 1u;
		unsigned int cost_insert  = lev_matrix[i][j - 1] + 1u;
		unsigned int cost_replace = lev_matrix[i-1][j-1] + 1u;
		
		/** If a swap is possible, calculate the cost. **/
		bool can_swap = (
		    i > 1 && j > 1 &&
		    str1[i - 1] == str2[j - 2] &&
		    str1[i - 2] == str2[j - 1]
		);
		unsigned int cost_swap = (can_swap) ? lev_matrix[i - 2][j - 2] + 1 : UINT_MAX;
		
		// Find the best operation.
		lev_matrix[i][j] = min(min(min(cost_delete, cost_insert), cost_replace), cost_swap);
		}
	    }
	}
    
    return lev_matrix[str1_len][str2_len];
    }

/*** Runs complete search to find duplocates in phone numbers using the
 *** levenshtein min edit distance algorithm.
 ***
 *** @param dataset An array of characters for all dataset strings.
 *** @param dataset_size The number of phone numbers to be scanned.
 *** @param dupe_threshold The similarity threshold, below which dups are ignored.
 *** @returns The duplicates in pDup structs.
 ***/
pXArray ca_phone_search(char dataset[][10u], const unsigned int dataset_size, const double dupe_threshold)
    {
    /** Allocate space for dups. **/
    const size_t guess_size = dataset_size * 2u;
    pXArray dups = xaNew(guess_size);
    if (dups == NULL)
	{
	fprintf(stderr, "ca_phone_search() - xaNew(%lu) failed.\n", guess_size);
	return NULL;
	}
    
    /** Search for dups using edit distance. **/
    for (unsigned int i = 0u; i < dataset_size; i++)
	{
	const char* v1 = dataset[i];
	for (unsigned int j = i + 1u; j < dataset_size; j++)
	    {
	    const char* v2 = dataset[j];
	    const unsigned int dist = ca_edit_dist(v1, v2, 10u, 10u);
	    const double similarity = (double)dist / 10.0;
	    if (similarity > dupe_threshold) /* Dup found! */
		{
		Dup* dup = (Dup*)nmMalloc(sizeof(Dup));
		if (dup == NULL)
		    {
		    fprintf(stderr, "ca_phone_search() - nmMalloc(%lu) failed.\n", sizeof(Dup));
		    
		    /** Free data before returning. **/
		    const size_t num_dups = dups->nItems;
		    for (unsigned int i = 0u; i < num_dups; i++)
			{
			void* dup = dups->Items[i];
			nmFree(dup, sizeof(Dup));
			}
		    xaDeInit(dups);
		    return NULL;
		    }
		
		dup->id1 = i;
		dup->id2 = j;
		dup->similarity = similarity;
		xaAddItem(dups, (void*)dup);
		}
	    }
	}
    
    return dups;
    }

void ca_init()
    {
    nmRegister(sizeof(Dup), "Dup");
    }

/** Scope cleanup. **/
#undef sparse_dif
#undef sparse_dif_to_centroid
