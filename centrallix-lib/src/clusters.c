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
/* Module:      lib_cluster.c, lib_cluster.h                            */
/* Author:      Israel Fuller                                           */
/* Creation:    September 29, 2025                                      */
/* Description: Clustering library used to cluster and search data with */
/*              cosine similarity and Levenshtein similarity (aka. edit */
/*              distance). Used by the "clustering driver".             */
/*              For more information on how to use this library, see    */
/*              string-similarity.md in the centrallix-sysdoc folder.   */
/************************************************************************/

/** This file has additional documentation in string_similarity.md. **/

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
 *** @param c1 The first character in the pair.
 *** @param c2 The second character in the pair.
 *** @returns The resulting hash.
 ***/
static unsigned int hash_char_pair(const unsigned char c1, const unsigned char c2)
    {
    const double sum = (c1 * c1 * c1) + (c2 * c2 * c2);
    const double scale = ((double)c1 + 1.0) / ((double)c2 + 1.0);
    const unsigned int hash = (unsigned int)round(sum * scale) - 1u;
    return hash % CA_NUM_DIMS;
    }

/*** An internal struct for temporarily storing character pairs while building
 *** sparse vectors.
 *** 
 *** @param c1 The first character in the character pair.
 *** @param c2 The second character in the character pair.
 *** @param hash The hash for the two characters, calculated by calling the 
 *** 	hash_char_pair() function (above).
 **/
typedef struct
    {
    unsigned char c1, c2;
    unsigned int hash;
    }
    CharPair, *pCharPair;

/*** Internal function to compare two character pairs to allow us to sort them
 *** by hash (ascending).
 *** 
 *** @param p1 The first pCharPair.
 *** @param p2 The second pCharPair.
 *** @returns An int > 0 if p1's hash is larger.
 ***          An int < 0 if p2's hash is larger.
 ***          0 if p1 and p2 have identical hashes.
 ***/
static int charpair_cmp(const void *p1, const void *p2)
    {
    const CharPair *a = p1, *b = p2;
    return a->hash - b->hash;
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
 *** coresponding dimension. However, for most names, this results in a lot of
 *** zeros and a FEW positive numbers. Thus, after creating the dense vector,
 *** we convert it to a sparse vector in which a negative number replaces a run
 *** of that many zeros. Consider the following example:
 *** 
 *** Dense pVector: `[1,0,0,0,3,0]`
 *** 
 *** Sparse pVector: `[1,-3,3,-1]`
 *** 
 *** Using these sparse vectors greatly reduces the required memory and gives
 *** approximately an x5 boost to performance when traversing vectors, at the
 *** cost of more algorithmically complex code.
 *** 
 *** @param str The string to be divided into pairs and hashed to make the vector.
 *** @returns The sparse vector built using the hashed character pairs.
 ***/
pVector ca_build_vector(const char* str)
    {
    /** Guard against a segfault. **/
    if (str == NULL) return NULL;
    
    /** Allocate memory. **/
    unsigned int num_chars = 0u;
    unsigned char* chars = check_ptr(nmSysMalloc((strlen(str) + 2u) * sizeof(unsigned char)));
    if (chars == NULL) goto err;
    
    /** Begin adding char pairs (in order). **/
    chars[num_chars++] = CA_BOUNDARY_CHAR; /* Starting boundary character. */
    for (const char* char_ptr = str; *char_ptr != '\0'; char_ptr++)
	{
	char maybe_char = *char_ptr;
	if (maybe_char < 0) fprintf(stderr, "Warning: Unexpected negative char '%c' in string: \"%s\"\n", maybe_char, str);
	unsigned char c = (unsigned char)maybe_char;
	
	/** Always consider boundary character in string. **/
	if (c == CA_BOUNDARY_CHAR) goto skip_checks;
	
	/** Ignore insignificant characters: spaces and punctuation. **/
	if (isspace(c)) continue; /* space, \n, \v, \f, \r */
	if (ispunct(c)) continue; /* !"#$%&'()*+,-./:;<=>?@[\]^_{|}~ */
	
	skip_checks:
	/** Shift numbers to the end of the lowercase letters. **/
	if ('0' <= c && c <= '9') c += 75u;
	
	/** Store the character. **/
	chars[num_chars++] = tolower(c);
	}
    chars[num_chars++] = CA_BOUNDARY_CHAR; /* Ending boundary character. */
    
    /** Compute hash values for char pairs. **/
    CharPair* char_pairs = check_ptr(nmSysMalloc(num_chars * sizeof(CharPair)));
    if (char_pairs == NULL) goto err_free_chars;
    const unsigned int num_pairs = num_chars - 1u;
    for (unsigned int i = 0u; i < num_pairs; i++)
	{
	/** Store characters. **/
	char_pairs[i].c1 = chars[i];
	char_pairs[i].c2 = chars[i + 1];
	
	/** Hash the character pair into an index (dimension).  **/
	/** Note that the passed value should always be between 97 ('a') and 132 ('9'). **/
	char_pairs[i].hash = hash_char_pair(chars[i], chars[i + 1]);
	}
    
    /** Free unused memory. **/
    nmSysFree(chars);
    chars = NULL;
    
    /** Sort char_pairs by hash value. **/
    qsort(char_pairs, num_pairs, sizeof(CharPair), charpair_cmp);
    
    /** Allocate space for the sparse vector. **/
    pVector sparse_vector = check_ptr(nmSysMalloc((num_pairs * 2u + 1u) * sizeof(int)));
    if (sparse_vector == NULL) goto err_free_char_pairs;
    
    /** Build the sparse vector. **/
    unsigned int cur = 0u, dim = 0u;
    for (unsigned int i = 0u; i < num_pairs;)
	{
	unsigned int hash = char_pairs[i].hash;
	
	/** Proceed through the pairs until we find a unique hash. **/
	/** Dividing value by 2 each time reduces the impact of repeated pairs. **/
	int value = 0;
	for (; i < num_pairs && char_pairs[i].hash == hash; i++)
	    {
	    value /= 2; /* Reduce impact of repeated pairs. */
	    value += ((unsigned int)char_pairs[i].c1 + (unsigned int)char_pairs[i].c2) % 13u + 1u;
	    }
	
	/** Skip zeros to reach the dimension index specified by the hash. **/
	unsigned int num_zeros = hash - dim;
	if (num_zeros > 0u) 
	    {
	    sparse_vector[cur++] = (int)-num_zeros;
	    dim = hash;
	    }
	
	/** Add the value to the sparse vector. **/
	sparse_vector[cur++] = value;
	dim++;
	}
    if (dim != CA_NUM_DIMS) sparse_vector[cur++] = -(CA_NUM_DIMS - dim);
    
    /** Free unused memory. **/
    nmSysFree(char_pairs);
    char_pairs = NULL;
    
    /** Trim extra space wasted by identical hashes. **/
    pVector trimmed_sparse_vector = check_ptr(nmSysRealloc(sparse_vector, cur * sizeof(int)));
    if (trimmed_sparse_vector == NULL) goto err_free_sparse_vector;
    sparse_vector = NULL; /* Mark memory freed by nmSysRealloc() no longer valid. */
    
    /** Return the result. **/
    return trimmed_sparse_vector;
    
    err_free_sparse_vector:
    if (sparse_vector != NULL) nmSysFree(sparse_vector);
    
    err_free_char_pairs:
    if (char_pairs != NULL) nmSysFree(char_pairs);
    
    err_free_chars:
    if (chars != NULL) nmSysFree(chars);
    
    err:
    return NULL;
    }

/*** Free memory allocated to store a sparse vector.
 *** 
 *** @param sparse_vector The sparse vector being freed.
 ***/
void ca_free_vector(pVector sparse_vector)
    {
    nmSysFree(sparse_vector);
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

/*** Print the underlying implementation values sparsely allocated
 *** vector (for debugging).
 *** 
 *** @param vector The vector.
 ***/
void ca_print_vector(const pVector vector)
    {
    const unsigned int len = ca_sparse_len(vector);
    printf("Vector: [%d", vector[0]);
    for (unsigned int i = 1u; i < len; i++)
	printf(", %d", vector[i]);
    printf("]");
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

/*** Calculate the similarity on sparsely allocated vectors. Comparing
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

/*** Calculate the difference on sparsely allocated vectors. Comparing
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

/*** Computes Levenshtein distance between two strings.
 *** 
 *** @param str1 The first string.
 *** @param str2 The second string.
 *** @param length1 The length of the first string.
 *** @param length1 The length of the first string.
 *** @returns The edit distance between the two strings, or a negative value on error.
 *** 
 *** @attention - `Tip`: Pass 0 for the length of either string to infer it
 *** 	using the null terminating character. Conversely, character arrays
 *** 	with no null terminator are allowed if an explicit length is specified.
 *** 
 *** @attention - `Complexity`: O(nm), where n and m are the lengths of	str1
 *** 	and str2 (respectively).
 ***/
int ca_edit_dist(const char* str1, const char* str2, const size_t str1_length, const size_t str2_length)
    {
    int result = -1;
    
    /** Detect string lengths, if necessary. **/
    const size_t str1_len = (str1_length == 0u) ? strlen(str1) : str1_length;
    const size_t str2_len = (str2_length == 0u) ? strlen(str2) : str2_length;
    
    /** Optimization: Handle identical string pointers. **/
    if (str1 == str2)
	return (str1_len > str2_len)
	     ? (str1_len - str2_len)
	     : (str2_len - str1_len);
    
    /*** lev_matrix:
     *** For all i and j, d[i][j] will hold the Levenshtein distance between
     *** the first i characters of s and the first j characters of t.
     *** 
     *** As they say, no dynamic programming algorithm is complete without a
     *** matrix that you fill out and it has the answer in the final location.
     ***/
    unsigned int** lev_matrix = check_ptr(nmSysMalloc((str1_len + 1) * sizeof(unsigned int*)));
    if (lev_matrix == NULL) goto end;
    for (unsigned int i = 0u; i < str1_len + 1u; i++)
	{
	lev_matrix[i] = check_ptr(nmSysMalloc((str2_len + 1) * sizeof(unsigned int)));
	if (lev_matrix[i] == NULL) goto end;
	}
    
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
    
    /** General Case. **/
    for (unsigned int i = 1u; i <= str1_len; i++)
	{
	for (unsigned int j = 1u; j <= str2_len; j++)
	    {
	    /** If the characters are equal, no change is needed. **/
	    if (str1[i - 1] == str2[j - 1])
		lev_matrix[i][j] = lev_matrix[i - 1][j - 1];
	    
	    /*** We need to make a change, so use the oppereration with the
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
		
		/** Assign the best operation. **/
		lev_matrix[i][j] = min(min(min(cost_delete, cost_insert), cost_replace), cost_swap);
		}
	    }
	}
    
    /** Store result. **/
    unsigned int unsigned_result = lev_matrix[str1_len][str2_len];
    if (unsigned_result > INT_MAX)
	{
	fprintf(stderr,
	    "Warning: Integer overflow detected in ca_edit_dist(\"%s\", \"%s\", %lu, %lu) = %u > %d\n",
	    str1, str2, str1_length, str2_length, unsigned_result, INT_MAX
	);
	}
    result = (int)unsigned_result;
    
    /** Cleanup. **/
    end:
    if (lev_matrix != NULL)
	{
	for (unsigned int i = 0u; i < str1_len + 1u; i++)
	    {
	    if (lev_matrix[i] == NULL) break;
	    else nmSysFree(lev_matrix[i]);
	    }
	nmSysFree(lev_matrix);
	}
    
    /** Done. **/
    return result;
    }

/*** Compares two strings using their cosie similarity, returning a value
 *** between `0.0` (completely different) and `1.0` (identical). If either
 *** OR BOTH strings are NULL, this function returns `0.0`.
 *** 
 *** @attention - This function takes `void*` instead of `pVector` so that it
 *** 	can be used as the similarity function in the ca_search() function
 *** 	family without needing a messy typecast to avoid the compiler warning.
 *** 
 *** @param v1 A `pVector` to the first string to compare.
 *** @param v2 A `pVector` to the second string to compare.
 *** @returns The cosine similarity between the two strings.
 ***/
double ca_cos_compare(void* v1, void* v2)
    {
    if (v1 == v2) return 1.0;
    
    /** Input validation checks. **/
    const pVector vec1 = v1, vec2 = v2;
    const bool v1_empty = (vec1 == NULL || ca_is_empty(vec1) || ca_has_no_pairs(vec1));
    const bool v2_empty = (vec2 == NULL || ca_is_empty(vec2) || ca_has_no_pairs(vec2));
    if (v1_empty && v2_empty) return 1.0;
    if (v1_empty && !v2_empty) return 0.0;
    if (!v1_empty && v2_empty) return 0.0;
    
    /** Apply rounding to avoid annoying floating point issues before returning. **/
    return round(sparse_similarity(vec1, vec2) * 1000000) / 1000000;
    }

/*** Compares two strings using their Levenshtein edit distance to compute a
 *** similarity between `0.0` (completely different) and `1.0` (identical).
 *** If both strings are empty, this function returns `1.0` (identical). If
 *** either OR BOTH strings are NULL, this function returns `0.0`.
 *** 
 *** @attention - This function takes `void*` instead of `char*` so that it
 *** 	can be used as the similarity function in the ca_search() function
 *** 	family without needing a messy typecast to avoid the compiler warning.
 *** 
 *** @param str1 A `char*` to the first string to compare.
 *** @param str2 A `char*` to the second string to compare.
 *** @returns The levenshtein similarity between the two strings, or NAN on failure.
 ***/
double ca_lev_compare(void* str1, void* str2)
    {
    /** Input validation checks. **/
    if (str1 == NULL || str2 == NULL) return 0.0;
    if (str1 == str2) return 1.0;
    
    /** Handle string length. **/
    const size_t len1 = strlen(str1);
    const size_t len2 = strlen(str2);
    if (len1 == 0lu && len2 == 0lu) return 1.0;
    if (len1 != 0lu && len2 == 0lu) return 0.0;
    if (len1 == 0lu && len2 != 0lu) return 0.0;
    
    /** Compute levenshtein edit distance. **/
    const int edit_dist = ca_edit_dist((const char*)str1, (const char*)str2, len1, len2);
    if (!check_neg(edit_dist)) return NAN;
    
    /** Normalize edit distance into a similarity measure. **/
    const double normalized_similarity = 1.0 - (double)edit_dist / (double)max(len1, len2);
    
    /** Apply rounding to avoid annoying floating point issues before returning. **/
    return round(normalized_similarity * 1000000) / 1000000;
    }

/*** Check if two sparse vectors are identical.
 *** 
 *** @param v1 The first vector.
 *** @param v2 The second vector.
 *** @returns true if they are equal,
 ***          false if any element is different.
 ***/
bool ca_eql(pVector v1, pVector v2)
    {
    const unsigned int len = ca_sparse_len(v1);
    for (unsigned int i = 0u; i < len; i++)
	if (v1[i] != v2[i]) return false;
    return true;
    }

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
    double result = NAN;
    
    /** Allocate space to store clusters as averages are computed. **/
    /*** We use nmMalloc() here because this function is usually called
     *** repeatedly with the same number of clusters in the k-means loop.
     *** Also, it is likely that k-means may be invoked multiple times with
     *** the same k value, leading to additional caching benefits.
     ***/
    double* cluster_sums = check_ptr(nmMalloc(num_clusters * sizeof(double)));
    unsigned int* cluster_counts = check_ptr(nmMalloc(num_clusters * sizeof(unsigned int)));
    if (cluster_sums == NULL) goto end;
    if (cluster_counts == NULL) goto end;
    for (unsigned int i = 0u; i < num_clusters; i++)
	{
	cluster_sums[i] = 0.0;
	cluster_counts[i] = 0u;
	}
    
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
    
    /** Calculate average sizes. **/
    result = cluster_total / num_valid_clusters;
    
    end:
    /** Clean up. **/
    if (cluster_sums != NULL) nmFree(cluster_sums, num_clusters * sizeof(double));
    if (cluster_counts != NULL) nmFree(cluster_counts, num_clusters * sizeof(unsigned int));
    
    return result;
    }

/*** Executes the k-means clustering algorithm. Selects NUM_CLUSTERS random
 *** vectors as initial centroids. Then points are assigned to the nearest
 *** centroid, after which centroids are moved to the center of their points.
 *** 
 *** @param vectors The vectors to cluster.
 *** @param num_vectors The number of vectors to cluster.
 *** @param num_clusters The number of clusters to generate.
 *** @param max_iter The max number of iterations.
 *** @param min_improvement The minimum amount of improvement that must be met
 *** 	each clustering iteration. If there is less improvement, the algorithm
 *** 	will stop. Pass any value less than -1 to fully disable this feature.
 *** @param labels Stores the final cluster identities of the vectors after
 ***     clustering is completed. Each value will be `0 <= n < num_clusters`.
 *** @param vector_sims An array of num_vectors elements, allocated by the
 *** 	caller, where index i stores the similarity of vector i to its assigned
 *** 	cluster. Passing NULL skips evaluation of these values.
 ***
 *** @attention - Assumes: num_vectors is the length of vectors.
 *** @attention - Assumes: num_clusters is the length of labels.
 ***
 *** @attention - Issue: At larger numbers of clustering iterations, some
 ***     clusters have a size of    negative infinity. In this implementation,
 ***     the bug is mitigated by setting a small number of max iterations,
 ***     such as 16 instead of 100.
 *** @attention - Issue: Clusters do not appear to improve much after the first
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
int ca_kmeans(
    pVector* vectors,
    const unsigned int num_vectors,
    const unsigned int num_clusters,
    const unsigned int max_iter,
    const double min_improvement,
    unsigned int* labels,
    double* vector_sims)
    {
    /** Setup stuff. **/
    bool successful = false;
    unsigned int cluster_counts[num_clusters];
    memset(labels, 0u, num_vectors * sizeof(unsigned int));
    
    /** Allocate space to store centroids and new_centroids. **/
    /** Dynamic allocation is required because these densely allocated arrays might be up to 500KB! **/
    const size_t centroids_size = num_clusters * sizeof(pCentroid);
    pCentroid* centroids = check_ptr(nmMalloc(centroids_size));
    pCentroid* new_centroids = check_ptr(nmMalloc(centroids_size));
    if (centroids == NULL) goto end;
    if (new_centroids == NULL) goto end;
    memset(centroids, 0, centroids_size);
    memset(new_centroids, 0, centroids_size);
    for (unsigned int i = 0u; i < num_clusters; i++)
	{
	centroids[i] = check_ptr(nmMalloc(pCentroidSize));
	new_centroids[i] = check_ptr(nmMalloc(pCentroidSize));
	if (centroids[i] == NULL) goto end;
	if (new_centroids[i] == NULL) goto end;
	memset(centroids[i], 0, pCentroidSize);
	memset(new_centroids[i], 0, pCentroidSize);
	}
    
    /** Select random vectors to use as the initial centroids. **/
    srand(time(NULL));
    for (unsigned int i = 0u; i < num_clusters; i++)
	{
	/** Pick a random vector. **/
	const pVector vector = vectors[rand() % num_vectors];
	
	/** Sparse copy the vector to expand it into a densely allocated centroid. **/
	pCentroid centroid = centroids[i];
	for (unsigned int i = 0u, dim = 0u; dim < CA_NUM_DIMS;)
	    {
	    const int token = vector[i++];
	    if (token > 0) centroid[dim++] = (double)token;
	    else for (unsigned int j = 0u; j < (unsigned)-token; j++) centroid[dim++] = 0.0;
	    }
	}
    
    /** Main kmeans loop. **/
    double old_average_cluster_size = 1.0;
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
	if (min_improvement < -1) continue; /** Skip check if it will never end the loop. **/
	const double average_cluster_size = check_double(get_cluster_size(vectors, num_vectors, labels, centroids, num_clusters));
	if (isnan(average_cluster_size)) goto end;
	const double improvement = old_average_cluster_size - average_cluster_size;
	if (improvement < min_improvement) break;
	old_average_cluster_size = average_cluster_size;
	}
    
    /** Compute vector similarities, if requested. **/
    if (vector_sims != NULL)
	{
	for (unsigned int i = 0u; i < num_vectors; i++)
	    vector_sims[i] = sparse_similarity_to_centroid(vectors[i], centroids[labels[i]]);
	}
    
    /** Success. **/
    successful = true;
    
    /** Clean up. **/
    end:
    if (centroids != NULL)
	{
	for (unsigned int i = 0u; i < num_clusters; i++)
	    {
	    if (centroids[i] != NULL) nmFree(centroids[i], pCentroidSize);
	    else break;
	    }
	nmFree(centroids, num_clusters * sizeof(pCentroid));
	}
    if (new_centroids != NULL)
	{
	for (unsigned int i = 0u; i < num_clusters; i++)
	    {
	    if (new_centroids[i] != NULL) nmFree(new_centroids[i], pCentroidSize);
	    else break;
	    }
	nmFree(new_centroids, num_clusters * sizeof(pCentroid));
	}
    return (successful) ? 0 : -1;
    }

/*** Finds the data that is the most similar to the target and returns
 *** it if the similarity meets the threshold.
 *** 
 *** @param target The target data to compare to the rest of the data.
 *** @param data The rest of the data, compared against the target to
 *** 	find the data that is the most similar.
 *** @param num_data The number of elements in data. Specify 0 to detect
 *** 	length on a null terminated array of data.
 *** @param similarity A function which takes two data items of the type
 *** 	of the data param and returns their similarity.
 *** @param threshold The minimum similarity threshold. If the most similar
 *** 	data does not meet this threshold, the function returns NULL.
 *** @returns A pointer to the most similar piece of data found in the data
 *** 	array, or NULL if the most similar data did not meet the threshold.
 ***/
void* ca_most_similar(
    void* target,
    void** data,
    const unsigned int num_data,
    const double (*similarity)(void*, void*),
    const double threshold)
    {
    /** Error cases. **/
    if (target == NULL
     || data == NULL
     || similarity == NULL
     || num_data == 0u
     || (threshold < 0.0 || 1.0 < threshold)
     || isnan(threshold)
    ) return NULL;
    
    /** Search for the most similar string. **/
    void* most_similar = NULL;
    double best_sim = -INFINITY;
    for (unsigned int i = 0u; (num_data == 0u) ? (data[i] != NULL) : (i < num_data); i++)
	{
	const double sim = similarity(target, data[i]);
	if (isnan(sim)) continue; /* Skip this comparison. */
	if (sim > best_sim && sim >= threshold)
	    {
	    most_similar = data[i];
	    best_sim = sim;
	    }
	}
    return most_similar;
    }


/*** Runs a sliding search over the provided data, comparing each element to
 *** the following `window_size` elements, invoking the passed comparison
 *** function just under `window_size * num_data` times. If any comparison
 *** yields a similarity greater than the threshold, it is stored in the
 *** xArray returned by this function.
 *** 
 *** @param data The data to be searched.
 *** @param num_data The number of data items in data.
 *** @param window_size The size of the sliding window used for the search.
 *** @param similarity A function which takes two data items of the type of
 *** 	the data param and returns their similarity.
 *** @param threshold The minimum threshold required for a duplocate to be
 *** 	included in the returned xArray.
 *** @param maybe_keys A pointer to an array of keys, with one key per data.
 *** 	These will be used to fill in the key1 and key2 attributes for each
 *** 	struct. If this variable is null, these values are also left null.
 *** @param maybe_dups A pointer to an xArray in which dups should be found.
 *** 	Pass NULL to allocate a new one.
 *** @returns An xArray holding all of the duplocates found, or NULL if an
 *** 	error occurs.
 ***/
pXArray ca_sliding_search(
    void** data,
    const unsigned int num_data,
    const unsigned int window_size,
    const double (*similarity)(void*, void*),
    const double threshold,
    void** maybe_keys,
    pXArray maybe_dups)
    {
    /** Error cases. **/
    if (data == NULL
     || num_data == 0
     || window_size == 0
     || similarity == NULL 
     || (threshold < 0.0 || 1.0 < threshold)
     || isnan(threshold)
    ) return NULL;
    
    /** Allocate space for dups (if necessary). **/
    pXArray dups = maybe_dups;
    if (dups == NULL)
	{
	/** Guess that we will need space for num_data * 2 dups. **/
	const int guess_size = num_data * 2;
	dups = check_ptr(xaNew(guess_size));
	if (dups == NULL) goto err;
	}
    const int num_starting_dups = dups->nItems;
    
    /** Search for dups. **/
    for (unsigned int i = 0u; i < num_data; i++)
        {
	const unsigned int window_start = i + 1u;
	const unsigned int window_end = min(i + window_size, num_data);
	for (unsigned int j = window_start; j < window_end; j++)
	    {
	    const double sim = check_double(similarity(data[i], data[j]));
	    if (isnan(sim) || sim < 0.0 || 1.0 < sim)
		{
		fprintf(stderr, "Invalid similarity %g %lf.\n", sim, sim);
		goto err_free_dups;
		}
	    if (sim > threshold) /* Dup found! */
		{
		Dup* dup = (Dup*)check_ptr(nmMalloc(sizeof(Dup)));
		if (dup == NULL) goto err_free_dups;
		if (maybe_keys != NULL)
		    {
		    dup->key1 = maybe_keys[i];
		    dup->key2 = maybe_keys[j];
		    }
		dup->similarity = sim;
		if (!check_neg(xaAddItem(dups, (void*)dup))) goto err_free_dups;
		}
	    }
	}
    
    /** Success. **/
    return dups;
    
    /** Error cleanup. **/
    err_free_dups:
    /** Free the dups that we added to the XArray. **/
    while (dups->nItems > num_starting_dups)
	nmFree(dups->Items[--dups->nItems], sizeof(Dup));
    if (maybe_dups == NULL) check(xaDeInit(dups)); /* Failure ignored. */
    
    err:
    return NULL;
    }

/*** Runs a complete search over the provided data, comparing each element to
 *** each other element, invoking the passed comparison function `num_data^2`
 *** times. If any comparison yields a similarity greater than the threshold,
 *** it is stored in the xArray returned by this function.
 *** 
 *** @param data The data to be searched.
 *** @param num_data The number of data items in data.
 *** @param similarity A function which takes two data items of the type of
 *** 	the data param and returns their similarity.
 *** @param threshold The minimum threshold required for a duplocate to be
 *** 	included in the returned xArray.
 *** @param maybe_keys A pointer to an array of keys, with one key per data.
 *** 	These will be used to fill in the key1 and key2 attributes for each
 *** 	struct. If this variable is null, these values are also left null.
 *** @param maybe_dups A pointer to an xArray in which dups should be found.
 *** 	Pass NULL to allocate a new one.
 *** @returns An xArray holding all of the duplocates found. If maybe_dups is
 *** 	not NULL, this will be that xArray, to allow for chaining.
 ***/
pXArray ca_complete_search(
    void** data,
    const unsigned int num_data,
    const double (*similarity)(void*, void*),
    const double threshold,
    void** maybe_keys,
    pXArray maybe_dups)
    {
    return ca_sliding_search(data, num_data, num_data, similarity, threshold, maybe_keys, maybe_dups);
    }

/** Scope cleanup. **/
#undef sparse_dif
#undef sparse_dif_to_centroid
