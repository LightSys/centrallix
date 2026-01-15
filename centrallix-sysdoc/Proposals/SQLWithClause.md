# WITH Clause for Centrallix SQL

Author:	Greg Beeley

Date:	September 18, 2025

## Overview

One challenge with SQL (in any SQL language) is that expressions can become unwieldy and difficult to read when there is any level of complexity involved.  The reasons are that common sub-expressions end up repeated and that sub-expressions are difficult to identify by the conceptual step being performed.  This is in contrast to procedural languages where an expression or value can be built up over multiple lines of code, making it much clearer what is being done.

This document describes a SQL extension to provide this functionality.

## The WITH Clause in SELECT statements

One possibility for dealing with common or precedent subexpressions is to allow one SELECT item to reference another SELECT item.  This could be a partial solution, but it also requires that the common or precedent expressions be added to the SQL result set, which isn't always desired and sometimes isn't even possible.

This proposal not only suggests allowing SELECT items to reference each other, but also describes the addition of a WITH clause to SELECT statements to identify precedent or common subexpressions that can then be used in other parts of the query, including SELECT statements and even the WHERE clause.

## Existing uses of WITH in other SQLs

Some SQLs provide a WITH clause with a conceptually similar, but logically different, use: defining a tabular data source, almost like a lightweight VIEW, that can be referenced in the FROM clause of a SELECT statement.  It takes the form

```
	WITH viewname (outparams) AS (SELECT statement) SELECT ...
```

We don't want to implement our use of the WITH keyword in a way that would prevent us from implementing this usage in the future; that said, I believe our usage is both distinct and can co-exist appropriately.

## WITH Clause structure and format

The WITH clause will follow the SELECT item list, but precede the FROM clause.  This allows it to co-exist with other uses if that need arises someday.

The general format would be:

```
	SELECT
		values ...
	WITH
		values ...
	FROM
		sources ...
	WHERE
		expression
```

The SELECT items and WHERE clause can both reference common and precedent values in the WITH clause, and the WITH clause values can reference any data source that a FROM clause item can reference.

The WITH items will overall be similar to SELECT items, but will not be per se included in the query's result set.

## Simple Example

```
	SELECT
		largest = condition(:this:a > :this:b, :this:a, :this:b)
	WITH
		a = 5,
		b = 6
	;
```

## Implementation considerations

1.	Parser.  The SQL engine data structures and parser would be straightforward to update for this feature.

2.	ObjectList Context.  I recommend using the "this" object name for referencing WITH items from WHERE or SELECT items.  However, research may need to be done to verify that this does not collide with other uses of "this".

3.	Self-References.  WITH items can reference each other.  However, this means that the expression evaluation will likely need to 1) evaluate WITH items on demand if context has changed, and 2) detect reference loops and return an error.

4.	WHERE Clause Delegation.  The CXSQL engine will delegate parts of the WHERE clause to underlying data sources.  If such a WHERE clause item references a WITH item, the WITH item needs to be copied into the WHERE clause to support such delegation.

5.	Coverage Mask.  WHERE and SELECT items referencing a WITH item will need coverage masks updated to include the coverage mask of the WITH items that are referenced (and WITH items that the WITH items reference and so forth).
