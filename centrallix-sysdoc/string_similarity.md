<!-------------------------------------------------------------------------->
<!-- Centrallix Application Server System                                 -->
<!-- Centrallix Core                                                      -->
<!--                                                                      -->
<!-- Copyright (C) 1998-2012 LightSys Technology Services, Inc.           -->
<!--                                                                      -->
<!-- This program is free software; you can redistribute it and/or modify -->
<!-- it under the terms of the GNU General Public License as published by -->
<!-- the Free Software Foundation; either version 2 of the License, or    -->
<!-- (at your option) any later version.                                  -->
<!--                                                                      -->
<!-- This program is distributed in the hope that it will be useful,      -->
<!-- but WITHOUT ANY WARRANTY; without even the implied warranty of       -->
<!-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        -->
<!-- GNU General Public License for more details.                         -->
<!--                                                                      -->
<!-- You should have received a copy of the GNU General Public License    -->
<!-- along with this program; if not, write to the Free Software          -->
<!-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             -->
<!-- 02111-1307  USA                                                      -->
<!--                                                                      -->
<!-- A copy of the GNU General Public License has been included in this   -->
<!-- distribution in the file "COPYING".                                  -->
<!--                                                                      -->
<!-- File:        string_similarity.md                                    -->
<!-- Author:      Israel Fuller                                           -->
<!-- Creation:    October 24th, 2025                                      -->
<!-- Description: Cluster object driver.                                  -->
<!-------------------------------------------------------------------------->

<!-- File notes:
 --- This file uses comment anchors, provided by the Comment Anchors VSCode
 --- extension from Starlane Studios. This allows developers with the extension
 --- to control click the "LINK <ID>" comments to navigate to the coresponding
 --- "ANCHOR[id=<ID>]" comment. (Note: Invalid or broken links will default to
 --- the first line of the file.)
 --- 
 --- For some reason, links expect to be proceeded by at least one * character,
 --- likely because they were not designed to be used in markdown files.
 --- 
 --- For example, this link should take you to the table of contents:
 --* LINK #contents
 --- 
 --- Any developers without this extension can safely ignore these comments,
 --- although please try not to break them. :)
 --- 
 --- Comment Anchors VSCode Extension:
 --- https://marketplace.visualstudio.com/items?itemName=ExodiusStudios.comment-anchors
 --->
 
# String Similarity
The following sections discuss the approaches to calculating similarity between two strings which are implemented in the `clusters.c` library.  This library can be included using `#include "clusters.h"` in centrallix-lib and `#include "cxlib/clusters.h"` in centrallix.


## Table of Contents <!-- ANCHOR[id=contents] -->
- [String Comparison](#string-comparison)
  - [Table of Contents](#table-of-contents)
  - [Cosine Similarity](#cosine-similarity)
    - [Character Sets](#character-sets)
    - [Character Pair Hashing](#character-pair-hashing)
    - [String Vectors](#string-vectors)
    - [Sparse Vectors](#sparse-vectors)
    - [Computing Similarity](#computing-similarity)
  - [Levenshtein Similarity](#levenshtein-similarity)
  - [Clustering](#clustering)
    - [K-means Clustering](#k-means-clustering)
    - [K-means++ Clustering](#k-means-clustering-1)
    - [K-medoids Clustering](#k-medoids-clustering)
    - [DBScan Clustering](#db-scan)
    - [Sliding Clusters](#sliding-clusters)
  - [Future Implementation](#future-implementation)
    - [K-means Fuzzy Clustering](#k-means-fuzzy-clusterings)
    - [Implement Missing Algorithms](#implement-missing-algorithms)


## Cosine Similarity <!-- ANCHOR[id=cosine] -->
The [Cosine Similarity](https://en.wikipedia.org/wiki/Cosine_similarity) function is defined as the dot product of two vectors divided by the product of the magnitude of the two vectors. Conceptually, it's like finding the _angle_ between two vectors.  To get these vectors, we use the relative frequency of character pairs within each string. To reduce memory cost and speed up computation, we store them in a special sparsely allocated form, described below.

### Character Sets <!-- ANCHOR[id=cosine_charsets] -->
Cosine compare currently uses the following character sets.  These can be extended or modified later, if necessary.
```c
const char ALLOW_SET[] = " \n\v\f\r!#$%&\"'()*+,-./:;<=>?@[]^_{|}~ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char CHAR_SET[] = "`abcdefghijklmnopqrstuvwxyz0123456789";
const char SIGNIFICANT_SET[] = "`ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char IGNORE_SET[] = " \n\v\f\r!#$%&\"'()*+,-./:;<=>?@[]^_{|}";
const char BOUNDARY_CHAR = ('a' - 1); // aka. '`'
```
- `ALLOW_SET` represents all characters which can be passed to a similarity detection algorithm.  Passing other characters may cause warnings and errors, undefined or unintended behavior, and even security concerns.
- `CHAR_SET` represents all of the characters that will be uniquely considered during the calculation of similarity.  Currently, this is all lowercase letters and numbers.
- `SIGNIFICANT_SET` represents all of the characters that are significant for the purposes of similarity.  For example, the uppercase letters are significant because they are considered identical to lowercase letters.  Thus, they are included in the `SIGNIFICANT_SET`, but not in the `CHAR_SET`.
- `IGNORE_SET` represents characters which, while allowed to be passed to a similarity algorithm, will be ignored.  For example, the strings "Ya!!" and "Ya..." will be considered identical.
- The `BOUNDARY_CHAR` is a special character which is conceptually added to the start and end of any string to be checked.
  - This allows for pairs that functionally include only the first and last character.
  - This character appears to have been selected to be one before the first character in `CHAR_SET` (thus convention dictates that it be written `'a' - 1` to indicate this), although it's unknown if that's the main or only reason.
  - If `clusters.h` is included, it can be accessed using the `CA_BOUNDARY_CHAR` macro.

### Character Pair Hashing <!-- ANCHOR[id=cosine_hashing] -->
Even with a small set of ASCII characters (say 36), there are still `36^2 = 1296` possible character pairs.  If the number of characters in the `CHAR_SET` ever needed to be expanded - for example, to include all UTF-8 characters - this number would quickly explode exponentially to utterly infeasible proportions.  Thus, a hashing algorithm is employed to hash each character pair down to a more reasonable number of dimensions (which can be accessed with the `CA_NUM_DIMS` macro).

### String Vectors <!-- ANCHOR[id=cosine_string_vectors] -->
Any string of characters in the `ALLOW_SET` can be represented by a vector.  For simplicity, imagine this vector has only `5` dimensions. To find this vector, we hash each character pair in the string.  As each character pair is hashed (for example, that the pair "ab" happens to hash to `3`), the corresponding dimension is increased by some amount.  This amount varies to based on the characters in the pair, helping to mitigate the impact of collisions where different character pairs hash to identical numbers (a larger number of dimensions also helps to mitigate this).

Remember that the first and last characters form a pair with the `BOUNDARY_CHAR`, so the string "ab" has three pairs: "a", "ab", and "b". If these each hash to `2`, `3`, and `0`. Thus, the vector generated by the string "ab" might be: `[7, 0, 4, 3, 0]`.  Notice that dimensions #1 and #4 are both `0` because no character pairs generated a hash of `1` or `4`.  In real usecases, the vast majority of elements are `0`s because the number of dimensions used is much larger than the number of character pairs in a typical string.

### Sparse Vectors <!-- ANCHOR[id=cosine_sparse_vectors] -->
As noted above, the vast majority of elements in a vector generated by a typical string are `0`s.  This would lead to a large waste of memory and computation if every `0` was stored separately, so instead, vectors are stored sparsely.  Because all hashes are positive integers, we represent `n` `0`s with a value of ` -n`.  Thus, the vector `[0, 1, 0, 0, 0]` (representing an empty string in `5` dimensions) would be represented sparsely as `[-1, 1, -3]`.

**Note**: A value of `0` in a sparse vector is undefined, so no element should be equal to `0`.

**Note**: Sparse arrays can vary greatly in length.  To find their size, one needs to traverse the array until the total number of values found adds up to `CA_NUM_DIMS`. The `ca_sparse_len()` function can be used to do this.  Also, the `ca_build_vector()` and `ca_free_vector()` use the `nmSys` functions from `newmalloc.h` to avoid conflicts over the size of the allocated data.

### Computing Similarity <!-- ANCHOR[id=cosine_similarity] -->
Finally, to find the cosine similarity between two strings, we can simply take the [dot product](https://en.wikipedia.org/wiki/Dot_product) of their coresponding vectors.  Then, we normalize the dot product by dividing by the magnitudes of both vectors multiplied together.  Two strings can be compared this way using the `ca_cos_compare()` function.


## Levenshtein Similarity <!-- ANCHOR[id=levenshtein] -->
The [Levenshtein](https://en.wikipedia.org/wiki/Levenshtein_distance) distance is defined as the number of insertions, deletions, or substitutions required to make one string exactly like another string.  The version implemented in `clusters.c` additionally allows a new operation called a "swap" in which two adjacent characters change places.  Transpositions of larger pieces of text are, unfortunately, not handled as well, which is a potential downfall of using levenshtein edit distance.

The levenshtein similarity of two strings can be compared using the `ca_lev_compare()` function.


## Clustering
When searching for similar strings in a large amount of data (for example, `1,000,000` strings), comparing every string to every other string can be very computationally expensive.  To speed up this process, it is helpful to _cluster_ similar strings together, then only compare strings within similar clusters. This sacrifices some accuracy to allow large amounts of data to be searched and compared in a feasible amount of time.

### K-means Clustering
When clustering data using the [k-means](https://en.wikipedia.org/wiki/K-means_clustering) algorithm, data is divided into a predefined number of clusters with the goal of maximizing the average similarity of data points within any given cluster.  To quickly summarize the algorithm:
1. Randomly select `k` data points to be the initial centroids of each cluster.
2. For each data point, find the centroid it is most similar to, and assign it to that cluster.
3. For each cluster, find the new centroid by averaging all data points in the cluster.
4. Repeat steps 2 and 3 until the clusters stabilize (i.e. no data point changes clusters).

The implementation used in `clusters.c` also allows the programmer to specify a maximum number of iterations (called `max_iter` in the code) to prevent this process from running forever.  Additionally, successive iterations can give diminishing results or even produce clusters that are slightly worse.  To improve performance, the programmer can also specify a minimum improvement threshold (called `min_improvement`).  Clusters must become more similar by at least this amount each iteration, otherwise the algorithm ends, even if the maximum number of iterations has not yet been reached.

The `ca_kmeans()` function can be invoked using [the cosine comparison string vectors](#string-vectors) (see above) to cluster them into similar clusters.

### K-means++ Clustering
**Not yet implemented**
This method is largely identical to k-means, except that [k-means++](https://en.wikipedia.org/wiki/K-means%2B%2B) assigns the initial centroids using an approximate algorithm designed to avoid some of the poor clustering possible with random assignment.

### K-medoids Clustering
**Not yet implemented**
This method is also very similar to k-means, except that [k-medoids](https://en.wikipedia.org/wiki/K-medoids) places an additional requirement that all centroids be points in the data.  This would theoretically allow for other similarity measures (such as Levenshtein edit distance) to be used for clustering instead of only cosine compare.

### DB-Scan
**Proposed, not yet implemented or documented**

### Sliding Clusters
A far more basic method of "clustering" is to simply sort all data alphabetically, then, instead of comparing each string to all other strings, it can be compared to only the next `n` strings.  Of course, differences near the start of a string (for example, "fox" vs. "box") will cause those strings to sort far away from each other, leading them to be completely missed.

Sorting using a similarity measure, such as `ca_cos_compare()` or `ca_lev_compare()` would resolve this issue.  However, these comparison functions do not meet the transitivity requirement for sorting, which is that `(A < B) & (B < C) -> (A < C)`.  For example, "car" is similar to "boxcar", which is also similar to "box".  However, "car" and "box" are not similar at all.

Additionally, sorting by the cosine vectors (similarly to how we cluster by them when using k-means) was proposed, but further investigation showed that this was also not possible.

For problems where a sorting algorithm exists which can mitigate the above issues, this solution may prove very promising.  However, so far we have not found such a problem, so the other clustering algorithms tend to outperform Sliding Clusters.


## Future Implementation

### K-means Fuzzy Clustering
One of the biggest downsides with k-means is that it creates very arbitrary boundaries between clusters.  Elements on either side of these boundaries may be highly similar, but if comparisons only occur within a cluster, these similar entries will be missed.  The problem becomes more extreme as a higher k value (more clusters) is used, creating more arbitrary boundaries.  This drawback is probably the main reason that clustering sacrifices some accuracy over searching every element.

Running the entire search multiple types may allow some of these to be found because the initial cluster locations are random.  This approach is partially implemented for duplicate searching because the algorithm runs nightly anyway, so a simple upsert (**UP**date existing entries; in**SERT** new entries) slightly reduces this problem.  However, this solution is obviously far from ideal.

If the clustering could be expanded with an additional step that makes clusters larger, adding elements from other clusters to them, this might effectively mitigate the issue.  It may also allow developers to use larger numbers of clusters, improving performance as well as accuracy. Further research is needed to verify the effectiveness of this approach before an implementation is written.

### Implement Missing Algorithms
Several algorithms (such as [k-means++](#k-means-clustering-1), [k-medoids](#k-medoids-clustering), and [DBScan](#db-scan)) above are proposed but lack an implementation.  They may be effective and useful, however, to reduce development time, they have not yet been implemented.

### Upgrade Other Duplicate Detection Systems
When a new record is entered, a quick scan is run to check if it might be a duplicate.  There is also a button in the UI for a record that lets you run a duplicate check.  These systems could also be upgraded using the new algorithms and strategies developed for general duplicate detection.

### Known Issues
- The cluster driver often fails to open the structure file if it was modifed since the last time the path was openned.  Opening a different path (including the root path, even though it does not support queries) fixes this issue.  This is either a bug in the st_node caching or in the cluster driver's usage of stparse.
- The cluster does not invalidate caches if the underlying data source changes.  This bug exists because I wasn't sure how to do this, but I'm pretty sure it's possible.  Workaround: Developers should use `exec <filename.cluster> "cache" "drop_all"` to invalidate caches when data is changed, or use a fresh object system instance.
