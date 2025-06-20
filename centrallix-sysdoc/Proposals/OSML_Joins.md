# Mechanism for passing joins through to the back-end RDBMS
Author:	Greg Beeley (GRB)

Date:	25-Oct-2012, updated 11-Jun-2025

## Overview
Presently, in Centrallix, a wide variety of data sources can be joined together using the Centrallix SQL engine (multiquery module).  However, all of the joins are processed inside Centrallix, resulting in a fairly inefficient situation for large queries.

The goal of this document is to describe a mechanism for passing joins through to a backend data source, such as a RDBMS, which supports joins in itself.

## Requirements
1.	A method to determine whether a backend datasource supports joining two query sources together.

2.	A method to take a pair of open queries (that have not yet returned any results), and combine them into a single query returning the joined objects.

3.	A method to continue to be able to individually access the returned objects relative to the original two data sources.  In other words, the join mechanism cannot solely return a synthetic joined object; it must return objects that individually belong to a single data source. This way, updates and deletes can properly be done on individual objects rather than on synthetic ones.

4.	The capability to cascade joins together - so that a joined query may be further joined with other queries.

5.	A way of specifying the join criteria when joining two queries.

## Structural Changes
1.	Add a new OSML API call, objQueryJoin(), which takes two open queries and join criteria, and returns a single "synthetic" query handle that is used to retrieve the results.  The original two query handles are still valid but objQueryFetch on the synthetic query handle is what drives the result set.  (optional: objQueryFetch on an original handle advances the result set until that handle's object changes, or objQueryFetch just returns the current object for that handle, instead of objComprisedObject below)

2.	Add a new OSML driver capability flag, OBJDRV_C_JOIN, which indicates that a driver is capable of joining two of its sources together.

3.	Add a new OSML API call, objSourceHash(), which returns a 128-bit string of characters that is unique for each data source that the underlying driver handles, such that two objects with the same source hash are joinable provided that the driver has the OBJDRV_C_JOIN capability.

4.	Add a new OSML API call, objComprisedObject(), which takes a synthetic object returned by objQueryFetch for a joined query, along with the original query handle for the original data source, and returns the current object for that original data source.  This approach may be clearer in the code than overloading objQueryFetch for this purpose.

5.	objQueryDelete() on one of the original query handles will delete objects from that data source that are a part of the overall result set.

6.	Modify the MQ equijoin module to be able to process more than one type of join method.

7.	Create a new join method for using objQueryJoin().

8.	Modify multiq_upsert.c and maybe multiq_equijoin.c to use a join for processing an upsert.

9.	Modify the sql engine to be able to process correlated subqueries as joins.

10.	Add a keyword setting to the sql engine to force the use of Centrallix joins rather than pass-through joins, to facilitate testing.

## Driver Support
1.	Add support to the MySQL driver

2.	Consider adding support to the Sybase driver
