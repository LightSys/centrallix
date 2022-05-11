# Data Obfuscation

Author: Greg Beeley (GRB)

Date: 25-Nov-2013

## Table of Contents
- [Data Obfuscation](#data-obfuscation)
  - [Table of Contents](#table-of-contents)
  - [Purpose](#purpose)
  - [Approach](#approach)
    - [1.	Obfuscation Key (K)](#1obfuscation-key-k)
    - [2.	Attribute Name (A)](#2attribute-name-a)
    - [3.	Object Name (N)](#3object-name-n)
    - [4.	Object Type (T)](#4object-type-t)
    - [5.	Unobfuscated Value (V)](#5unobfuscated-value-v)
    - [6.	Data Type (D)](#6data-type-d)
  - [Obfuscation Rules](#obfuscation-rules)
  - [Algorithm](#algorithm)
  - [Random Value Generator](#random-value-generator)
    - [1.	For INTEGER values, or STRING values consisting entirely of digits:](#1for-integer-values-or-string-values-consisting-entirely-of-digits)
    - [2.	For MONEY values,](#2for-money-values)
    - [3.	For DOUBLE precision floating point values,](#3for-double-precision-floating-point-values)
    - [4.	For DATE/TIME values,](#4for-datetime-values)
    - [5.	For STRING values not handled as integers:](#5for-string-values-not-handled-as-integers)

## Purpose
The purpose of this data obfuscation module is to allow for sample and test data to be generated and exported without compromising live "real life" data.

The data could be obfuscated randomly every time it is generated.  However, the lack of repeatability means that the generated data loses its patterns and relationships, making it less useful for testing purposes.  Thus, this module will provide for some degree of repeatability to the obfuscation process.  This repeatability means that some information does leak through the obfuscation; this leakage can show patterns of data, but not the identities of the entities behind the data.  This compromise is intentional.

## Approach
The data obfuscation approach used here will include several inputs:

### 1.	Obfuscation Key (K)
The key will be a string of any kind, which is used in a cryptographic hash algorithm to provide repeatability to the generation of obfuscated data via this module.  Typically, it itself might be a hex string of some kind, but it need not be.  If the key is left empty, then a key is randomly generated.

### 2.	Attribute Name (A)
The name of the attribute being obfuscated is used as an input to the obfuscation logic.  This is so that obfuscated data is repeatable for a given attr name - for instance, $100 in a "Total Given" attr will be obfuscated the same way, but a $100 in a "Non-Deductible Gifts" attr would be obfuscated in a different way.

### 3.	Object Name (N)
When the name (primary key) of the object is available, it can also be used as an input to the obfuscation algorithm.  This will allow for the same data that occurs in two different objects to be obfuscated in different ways.

### 4.	Object Type (T)
The primary use of the object type as an input to the obfuscation mechanism is to guide how the obfuscation occurs.  For some objects and attributes, the obfuscation may need to be done differently than for others.  Currently the Type will be derived from the query name (for reports) or the parent object name (i.e., table name, for direct database obfuscation).

### 5.	Unobfuscated Value (V)
Finally, the unobfuscated value itself is used as an input to the data obfuscation.  In a simple scenario, it and the obfuscation key would be the only two inputs, but that may provide too much insight into what the underlying data is in many circumstances, which is why the attribute and object names can also be included.

### 6.	Data Type (D)
This is the data type of the attribute (integer, string, etc.).  It is not used when creating the random hash that forms the randomization information for the obfuscated data, but it can be used in rules to determine how to handle a piece of data.

## Obfuscation Rules
In the absence of any rules or key values, the obfuscation process will generate a random Key (K), and then use all of the other inputs (ANTV) that are possible.

However, a rule file can be supplied which will customize how the process is done.  Here are some examples:

```
A:p_partner_key			KV
A:p_donor_partner_id		KV
A:p_donor_partner_key		KV
D:datetime			KV	hms
D:money				KV	i
D:string			KANTV	/path/to/words.txt
*				KANTV
```

## Algorithm
Given:

K = Obfuscation Key

A = Attribute Name

N = Object Name

T = Object Type

V = Current Value

1.	If K is empty/null, generate a 64-bit random hex string for K.

2.	Lookup an obfuscation rule that matches the given A,N,T,V values.

3.	If no rule found, assume KANTV.

4.	Compute an SHA1 hash H over the inputs specified by the rule, factoring the set of inputs through SHA1 twice, as in KANTVKANTV for instance.

5.	Compute a second SHA1 hash G over all inputs except for V.

6.	Use the 160-bit hash value H as inputs into the random value generator to generate the replacement value.

## Random Value Generator
Given:

V = Current Value

H = SHA1 hash - 160 bits of pseudorandom data

G = SHA1 hash - 160 bits of pseudorandom data (not hashing V)

E = obfuscated value to replace V

### 1.	For INTEGER values, or STRING values consisting entirely of digits:
- a.  1 bit of H = whether to add 1 to V and then multiply by 10.
- b.  1 bit of H = whether to divide V by 10 and then add 1.
- c.  E := 0
- d.  while (V <> 0) { right shift V; left shift E; add 1 bit of H to E }
- e.  Compute the smallest power of 2, 2^X, that is larger than E.
- f.  While E is already in value hashing table identified by G and X, for a different value V, --> add 1 to E, mod 2^X
- g.  Add the E:V relationship to a hash table identified by G and X, if X is smaller than or equal to 17.

Integer values accept the third parameter in the rule file (this thus also applies to values that process using the integer logic, such as money types).  That third parameter can contain:

i = only allow multiples or factors of the original value V.  This algorithm works slightly differently:

- a.  1bit of H -> whether to increase or decrease V.
- b.  3bits of H -> which factors to use: 2,3,5 (111: use 000)
- c.  If decreasing and no factors divide V, then handle as if the 'i' parameter was not supplied.

### 2.	For MONEY values,
- a.  Convert to integer by multiplying by 100.
- b.  Use the integer approach steps a through d.
- c.  Divide the resulting integer by 100 and convert back to money.

Money values accept the rulefile third parameter, which can contain 'w' and/or 'f', for the whole and/or fraction parts of the amount.

w = obfuscate the whole part of the money amount.

f = obfuscate the fraction part of the money amount.

### 3.	For DOUBLE precision floating point values,
- a.  Compute the log10 (base 10 exponent) of the value
- b.  Multiply to convert to an integer
- c.  Apply steps c-d for integer values
- d.  Divide to convert back to floating point

### 4.	For DATE/TIME values,
- a.  3bit of H = how many years to add/subtract (-3 -2 -1 0 0 1 2 3)
- b.  3bit of H = how many months to add/subtract (-3 .. 3)
- c.  5bit of H = how many days to add/subtract (-15 .. 0 0 .. 15)
- d.  5bit of H = how many hours +/- (-12 .. 12 weighted to center)
- e.  6bit of H = how many min +/- (-30 .. 30 weighted to center)
- f.  6bit of H = how many sec +/- (-30 .. 30 weighted to center)

Date/Time values accept the third field, determining which of a-f above are used.  Values will not be allowed to wrap past or before the present time, and values will not be allowed to wrap such as to change date parts not allowed in the third field (such as adding 10 hours to move to the next day).

### 5.	For STRING values not handled as integers:
The third field present is allowed, and if present points to a file containing lists of sub-strings.  If a sub-string in the string matches a substring in the list, then an alternate substring will be chosen from the same category when obfuscating.

- a.  String is handled as a space/punctuation separated series of sub-strings.
- b.  Each substring is handled according to the below steps:
- c.  If a word list is supplied, the string is looked up in the list.
- d.  If the string matches, it is replaced with another entry from the same category in the word list, using N bits of H, according to the size of the word list.
- e.  If there is no match, the string is handled according to the below steps:
- f.  Sequences of numeric digits are handled like integers (steps c and d of the integer handling procedure).
- g.  Apparently phonetical sequences (alternating consonants and vowels/diphthongs) are replaced with a random phonetical sequence (2 bits - how many parts different from # of parts in the original sequence (-1 0 0 1), and N bits each part to select the consonant or vowel sequence).
- h.  Other sequences are replaced by randomly choosing characters from the same character classes (0-9, a-z, A-Z) and keeping punctuation/whitespace the same.  Sequences of two or more of the same class are subject to insertion/removal rules (see above).
