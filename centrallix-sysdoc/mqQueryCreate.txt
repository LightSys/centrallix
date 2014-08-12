Document:   Implementation of mqQueryCreate for creation of objects via query
Author:	    Greg Beeley (GRB)
Date:	    23-July-2014
-------------------------------------------------------------------------------

OVERVIEW...

    The OSML provides an interface called objQueryCreate() which allow for the
    creation of a new object in the context of a running query.  This allows
    for the various query criteria to be applied (in reverse) to the new
    object when it is created.

    This document describes the implementation of this feature in the
    MultiQuery layer, which of course usually involves queries with multiple
    data sources.


PROCESS...

    1.	Identify the IDENTITY query source.
    
	If the query only has one source, then use that as the IDENTITY.  If
	the query has more than one source, then one source must be marked as
	the IDENTITY source in order for a create to be able to occur.

    2.	Create a New PseudoObject.

	We'll need to return an object reference, so create the pseudo object
	in the context of the query.  All objects comprising it should be NULL
	at this time.

    3.	Create the Underlying Object.

	With the IDENTITY source, create a new object there.

    4.	Run Static Criteria Items.

	Find any WHERE clause items that only reference the one data source,
	and run them each in reverse to set some values.

    5.	Allow the Caller to Set Properties.

	The mqQueryCreate should return to the caller and allow the caller to
	call SetAttrValue to set up the various properties.  Those will result
	in reverse expression evaluations occurring, causing properties to be
	set on the new underlying object.
