Title:	Mechanism for passing joins through to the back-end RDBMS
Author:	Greg Beeley (GRB)
Date:	25-Oct-2012
-------------------------------------------------------------------------------

OVERVIEW...

    Presently, in Centrallix, a wide variety of data sources can be joined
    together using the Centrallix SQL engine (multiquery module).  However,
    all of the joins are processed inside Centrallix, resulting in a fairly
    inefficient situation for large queries.

    The goal of this document is to describe a mechanism for passing joins
    through to a backend data source, such as a RDBMS, which supports joins
    in itself.


REQUIREMENTS...

    1.	A method to determine whether a backend datasource supports joining
	two query sources together.

    2.	A method to take a pair of open queries (that have not yet returned
	any results), and combine them into a single query returning the
	joined objects.

    3.	A method to continue to be able to individually access the returned
	objects relative to the original two data sources.  In other words,
	the join mechanism cannot just return a synthetic joined object; it
	must return objects that individually belong to a single data source.
	This way, updates and deletes can properly be done on individual
	objects rather than on synthetic ones.

    4.	The capability to cascade joins together - so that a joined query may
	be further joined with other queries.

    5.	A way of specifying the join criteria when joining two queries.


OSML CHANGES...

    1.	Add a new OSML API call, objQueryJoin(), which takes two open queries
	and join criteria, and returns a single "synthetic" query handle that
	is used to retrieve the results.  The original two query handles are
	no longer valid after passing them to objQueryJoin().

    2.	Add a new OSML driver capability flag, OBJDRV_C_JOIN, which indicates
	that a driver is capable of joining two of its sources together.

    3.	Add a new OSML API call, objSourceHash(), which returns a string of
	characters that is unique for each data source that the underlying
	driver handles, such that two objects with the same source hash are
	joinable provided that the driver has the OBJDRV_C_JOIN capability.
