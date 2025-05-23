# "Upsert" SQL operation (insert ... on duplicate ... update)

Author:	    Greg Beeley (GRB)

Date:	    October 16, 2013

## Overview
Often, new data needs to be merged into existing data, with some objects created and others updated.  Some SQL engines perform this operation with the ANSI "MERGE" construct, and others use an "ON DUPLICATE" clause.  This document describes the Centrallix approach, which uses an "ON DUPLICATE" clause but in a way that provides more flexibility and adaptability to data sources of varying types.

## Syntax
The Upsert operation syntax is as follows:

```
INSERT INTO /path/name
    SELECT {select-statement}
    ON DUPLICATE {expression list}
    UPDATE SET {update expression list}
```

The INSERT...SELECT is performed as usual, except that duplicate checking is also performed.  If there is already an object inside /path/name that has attributes that match the {expression list} that immediate follows "ON DUPLICATE", then the new object is not inserted, but instead the existing object is updated using the {update expression list} that follows the "UPDATE SET" statement.

The ON DUPLICATE expressions may take on the form of SELECT items; that is they may be renamed, as in:

    ON DUPLICATE key_id = :unique_code + :last_name

In the above instance, key_id refers to the attribute in the data source being inserted into.  The values unique_code and last_name refer to attributes from the SELECT statement.

## Algorithm
- A.	For each ON DUPLICATE expression,
    1.  Compile the expression into an expression tree.
    2.  Use the expression tree to create a {expression} = {value} tree
    3.  Save a reference to the expression tree and to the {value} node
- B.	Iterate over the Select Statement, retrieving matching objects.
    1.  Check Select statement HAVING clause against matching object.
    2.  For each ON DUPLICATE expression,
        - a.	Set the {value} node equal to the attribute from the SELECT.
    3.  Run a query on the insert path searching for objects that match the list of criteria created in step (2).
    4.  If a matching object is not obtained:
        - a.	Insert the row.
    5.  For each matching object that IS obtained:
        - a.	Save the matching object and the select object for step (C).
- C.	For each object to be updated (from B(5)(a)),
    1.  For each item in the UPDATE SET expression list:
        - a.  Set the attribute in the matching row equal to the expression given.

## Structure
```
+ INSERTSELECT node
+ UPSERT node
    + { select statement subtree including joins, projections, etc. }
```

In this construct, the UPSERT node would not perform the inserts, but would pass on the row needing to be inserted up to the INSERTSELECT node. As the INSERTSELECT node iterates over child->NextItem(), all it would see would be the rows which were NOT duplicates.  In this form, the UPSERT node would be a generator which would return at step B(4)(a) in the above algorithm.  The UPSERT node essentially acts as a filter for the INSERT.

## Other Comments
In the proposed mechanism here, the Upsert operation does not depend on inserts failing in order to determine when to update.  In that case, inserts would fail only on a duplicate primary key (on data sources that reject duplicate primary keys).  Since many data sources may not have duplicate key checking, or may auto-generate the primary key, causing the Upsert to trigger off of a failed insert would be short-sighted.

The proposed mechanism provides the developer great flexibility on when to do an update instead of an insert, while avoiding the complexity of the ANSI MERGE statement that many SQL implementations use.

The construct of performing all of the updates after all of the inserts have finished can have unexpected side effects if any of the updates would have changed whether future rows (in the same upsert operation) would have been inserted or updated, by changing the row attributes referenced in the ON DUPLICATE clause.  This is necessary, for the time being, due to the need to avoid deadlocking when upserting a relational database data source which may lock out the selected rows during the SELECT operation, thus preventing them from being properly updated by the Centrallix server.
