# String Comparison
The following sections discuss the two approaches to calculating similarity between two strings. Both approaches use a SQL function to calculate a similarity metric (on a scale of 0 to 1) for two string parameters.

## Table of Contents
- [String Comparison](#string-comparison)
  - [Table of Contents](#table-of-contents)
  - [Levenshtein Similarity](#levenshtein-similarity)
    - [Levenshtein](#levenshtein)
  - [Cosine Similarity](#cosine-similarity)
    - [CHAR_SET](#char_set)
    - [Frequency Table](#frequency-table)
    - [Relative Frequency Table](#relative-frequency-table)
    - [TF-IDF](#tf-idf)
    - [Dot Product](#dot-product)
    - [Magnitude](#magnitude)
    - [Similarity](#similarity)
  - [Future Implementation](#future-implementation)
    - [Inverse Document Frequency (IDF)](#inverse-document-frequency-idf)

## Levenshtein Similarity
The [Levenshtein](https://en.wikipedia.org/wiki/Levenshtein_distance) distance is defined as the number of insertions, deletions, or substitutions required to make one string exactly like another string.

### Levenshtein
```c
int exp_fn_levenshtein(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
```
Returns the levenshtein edit distance between two strings.

```c
int exp_fn_fuzzy_compare(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
```
Returns a value between 0.0 (complete match) and 1.0 (complete difference) between strings a and b, based on the (levenshtein distance) / (max len of input strings). 
Some alterations to the calculation are as follows: 
- matching an empty string against anything returns 0.5. 
- a string that only required insertions to become the other string has its (lev_dist)/(strlen) value halved before returning
The parameter max_field_width is required, but not used.

## Cosine Similarity  

The [Cosine Similarity](https://en.wikipedia.org/wiki/Cosine_similarity) function is defined as the dot product of two vectors divided by the product of the magnitude of the two vectors. We use the relative frequency of the individual characters within each term as the vectors in the calculation. The following functions are used to calculate cosine similarity.

### CHAR_SET
```c
const char *CHAR_SET ...
```
`CHAR_SET` represents all of the characters that should be considered during the calculation of similarity. `CHAR_SET` can be extended to include additional characters, as necessary.

### Frequency Table
```c
int exp_fn_i_frequency_table(double *table, char *term)
```
Helper function for similarity(). Creates a frequency table containing indices corresponding to all characters in `CHAR_SET` (all other characters are ignored). The values in the frequency table will contain the number of times each character appers in `term`.

The `table` parameter must be allocated prior to calling the function with `nmMalloc()` using `sizeof(x * sizeof(double))`, where `x` is the length of `CHAR_SET`. The function will initialize all `table` values to 0, before calculating the frequency values.

### Relative Frequency Table
```c
int exp_fn_i_relative_frequency_table(double *frequency_table)
```
Helper function for similarity(). Converts a frequency table into a relative frequency table, where each value in the `frequency_table` is converted to the percent of occurrence (i.e., frequency divided by the sum of total occurrences).

The `frequency_table` parameter must have been created using the `exp_fn_i_frequency_table` function above.

### TF-IDF
```c
int exp_fn_i_tf_idf_table(double *frequency_table)
```
Helper function for similarity(). Creates a TF x IDF vector from a frequency table, where each value in the resulting table is created by multiplying the relative frequency of each letter by the corresponding coefficient in the IDF array.

The `frequency_table` parameter must have been created using the `exp_fn_i_frequency_table` function above.

### Dot Product
```c
int exp_fn_i_dot_product(double *dot_product, double *r_freq_table1, double *r_freq_table2)
```
Helper function for similarity(). Calculates the dot product of two relative frequency tables (sum of the squared values from each relative frequency table). 

The `dot_product` parameter should be initialized to 0 before calling the function. The table parameters must contain relative frequency tables that are generated from the `exp_fn_i_relative_frequency_table` function. The lengths of both tables must equal the length of `CHAR_SET`.

### Magnitude
```c
int exp_fn_i_magnitude(double *magnitude, double *r_freq_table)
```
Helper function for similarity(). Calculates the magnitude of a relative frequency table (square root of the sum of the squared relative frequencies).

The `magnitude` parameter should be initialized to 0 before calling the function. The table parameter must contain a relative frequency table that was generated from the `exp_fn_i_relative_frequency_table` function. The length of the frequency table must equal the length of `CHAR_SET`.

### Similarity
```c
int exp_fn_similarity(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
```
Returns a value between 0.0 (completely different) and 1.0 (complete match) reflecting the similarity between the value passed in to i0 and the value passed in to i1. The first two parameters should contain strings that need to be compared. If the value 1 is passed in the third parameter, then the similarity function will rely on TF x IDF scores to determine similarity. If no third parameter is passed, then the function will rely only on relative frequency scores.

## Future Implementation

### Inverse Document Frequency (IDF)
In text mining, the most common metric to use in the cosine similarity function is the [TF x IDF](https://en.wikipedia.org/wiki/Tf%E2%80%93idf) metric. Our approach uses only TF (term frequency). Inverse document frequency calculates a weighting factor for each character. This could increase precision a small amount by weighting characters that appear on many records as less important in distinguishing matches, and weighting characters that appear on only certain records as more important. IDF could be calculated by iterating through the entire partner dataset each time. The current approach uses the relative frequency of each letter used in the English language on [Wikipedia](https://en.wikipedia.org/wiki/Letter_frequency), which may not be consistent with the data in the partner database.


