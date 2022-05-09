Title:	EAV Pivot OSD Optimization Strategy
Author:	Greg Beeley
Date:	24-Oct-2012
-------------------------------------------------------------------------------

OVERVIEW...

    An Entity-Attribute-Value schema allows for a lot of flexibility in an
    application, but it also poses some special challenges when it comes to
    properly optimized queries.

    The purpose of this document is to describe the optimization strategy and
    options for the "querypivot" driver which provides relational access to
    an underlying EAV schema or schema element.


STRATEGY #1:  LOOKUP BY KEY...

    The easiest situation to handle is when the user simply needs to lookup
    data by its key.  This is easily handled by simply passing on the key
    value lookup to the underlying data source, retrieving data by that same
    key.


STRATEGY #2:  TABLE SCAN...

    In the absence of other more optimized strategies, the only real option
    available is to do a complete table scan of the underlying EAV data,
    applying whatever criteria is needed to the compiled relational data.


STRATEGY #3:  LOOKUP BY SINGLE ATTRIBUTE...

    If the user is simply looking for an object by single attribute value,
    rather than via a complex WHERE clause, the general strategy is:

	1.  Find an underlying EAV row with that attribute name and value.

	2.  Take the entity key in that EAV row and lookup the associated
	    rows containing the other attributes for that entity.

	3.  Return the compiled relational row to the user.

    This can be done via nested iteration inside the querypivot driver itself,
    or by building a self-join query and passing it to the Centrallix SQL
    engine.


STRATEGY #4:  MULTIPLE SELF-JOIN AND WHERE CLAUSE TRANSFORM

    If the WHERE clause contains several criteria, potentially combined
    together in a complex manner, the querypivot driver could handle this
    using a multiply self-joined query with a transformed WHERE clause:

	1.  Scan the WHERE clause to determine which attributes are being
	    referenced.
	2.  Each attribute becomes its own distinct FROM source.
	3.  The FROM sources are self-joined on the entity key field(s).
	4.  Each instance of ":object:attrname" is then replaced with
	    ":attrsource:attr_value_field".
	5.  The WHERE clause is augmented such that each FROM source is
	    restricted to only pertain to EAV rows with the respective
	    values in the 'attribute' column.

    Unfortunately, this strategy doesn't work as well with WHERE clauses that
    are predominantly joined by OR operators, mainly due to the fact that
    Centrallix SQL lacks a merge join operator and currently cannot pass
    joins through to a remote RDBMS.


STRATEGY #5:  SINGLE SOURCE CRITERIA WITH GROUP BY AGGREGATION

    For WHERE clauses that are primarily joined using OR operators, a
    different strategy can be used.  In this case, no self-join is done,
    but a GROUP BY query is run.

	1.  Transform the WHERE clause by replacing instances of
	    ":object:attrname = VALUE" with ":attr_value_field = VALUE
	    AND :attr_name_field = 'attrname'".
	2.  GROUP BY the entity key field(s) so that multiple hits on the
	    same entity are grouped together.
	3.  The querypivot driver would implement nested iteration to
	    obtain the remainder of the attributes for each entity found.
	4.  Alternately, a self join could be done to obtain the above
	    attribute/value list, if the GROUP BY clause is properly
	    constructed.

    The downside to this strategy is that, as is, it doesn't work well
    with WHERE clauses that are primarily joined by AND operators, so much
    of the criteria in that case would be evaluated after the pivot
    operation is applied.
