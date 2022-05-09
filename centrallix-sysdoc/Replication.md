Document:   Replication support logic
Date:	    12-Jun-2015
Author:	    Greg Beeley (GRB)
-------------------------------------------------------------------------------

OVERVIEW...

    An important aspect of Centrallix going forward will be its ability to
    replicate data (the observer/publisher pattern) from server to client and
    vice versa.  This document explores some of the mechanisms needed to
    complete the replication mechanism across all data that the Centrallix
    server manages.


GENERAL MECHANISM...

    The general mechanism for replication will have several key elements:

    A.	A series of modules, drivers, or other architectural layers that
	participate together in handling data within the Centrallix server.  We
	will call these "Units" for the purpose of this document.

    B.	A way for each Unit to determine when the data provided to it has been
	changed, added to, or deleted from.

    C.	A way for each Unit to notify observers of when the data of interest to
	an observer has been changed, added to, or deleted from.

    D.	A transactional mechanism so that data changes, creations, and
	deletions can be grouped into atomic operations, to prevent observers
	from working with an inconsistent data set.

    E.	Ways for originators of data (i.e., ObjectSystem Drivers) to determine
	when the data they manage has changed, so they can issue notifications
	to observers.  This will of course vary based on the data source.

    F.	An OSML-level subscriber/publisher mechanism that retains a database of
	subscribed object information, so that when a change is made the
	appropriate observers can be notified of the change.

    G.	The OSML-level mechanism should support the creation of new objects --
	for example, if a new object is created that matches an existing query
	that is subscribed to, the new object's existence should be published
	to the subscriber.  Thus, the mechanism will need to support matching
	criteria of some kind rather than just a database of object ID's.


DATA SOURCES...

    For data sources, the determination that the underlying data has changed is
    fundamentally different because of the variety of data sources that are
    supported.  Therefore, every data source will need to implement this in
    different ways.  Here are some examples of how this could be done for
    common types of data sources:

    A.	File-based data.  For a simple file-based data source, such as XML
	data, JSON data, CSV data, structure files, etc., the driver will need
	to rely on change notifications from the underlying driver, such as
	the local filesystem driver (UXD).  However, care will need to be taken
	to avoid handling data that has incompletely changed.  This can be done
	by the underlying driver not delivering a change notification until the
	source of the change issues an objCommitObject() or objClose().  If
	however the change source is external, this may be more difficult.  See
	the discussion for the UXD driver below.

    B.	Local files.  The UXD (local filesystem) driver provides access to
	local files on the server.  These files can be changed from within the
	Centrallix server or the changes could come from outside (such as from
	a developer editing a file).  The critical issue is determining when a
	change has fully been made vs. just part of a change, since a partially
	changed file could be internally inconsistent or simply unparseable.
	For changes occurring from within, the Centrallix server should have
	information about whether the operation is complete -- for example the
	use of transactions, object closure, or other similar operations.
	However, from outside the server, a different tactic will need to be
	used.  A suggested approach would be to use an adaptive timer -- to
	only send a notification to subscribers once a delay has passed since
	the last change to the file.  The driver could track what the typical
	range of intervals is between changes to a file, and determine what
	changes look like they are "grouped together" in time.  As some
	history is built on these intervals, the server will be able to reduce
	the delay that it waits after the last change.

    C.	Remote data.  Some remote data sources may implement an observer
	mechanism of some kind, but most will not.  For those data sources,
	some kind of polling may be necessary.  This polling may often search
	at regular intervals for data that has been changed since a particular
	point in time, but that does not cover deleted data.  For databases, if
	a search is done on objects with a modification timestamp since some
	point in time, an index may be required on that timestamp.  To handle
	deleted data, for importnat replicated data it may be preferable to
	simply mark data as deleted/obsolete rather than actually removing the
	data.  The polling interval can be chosen based on practical matters
	(not overloading the remote resource), and/or dynamically based on the
	roughly maximum rate that changes appear to be happening.


OSML SUBSCRIBER/PUBLISHER MECHANISM...

    The OSML itself will play an important role here.  Data may be accessed and
    subscribed to based on individual objects, or based on a collection of
    objects matching a given criteria, and the OSML mechanism must encompass
    both of these situations.

    The purpose of this mechanism is to provide the correct updates to a given
    subscriber -- the truth, the full truth, and nothing but the truth.

    One possibility is to have an expression table that is used to match
    objects of interest.  For individual objects, the expression could just
    match against the unique object name.  For collections, the expression may
    match against the parent "directory" and against selected matching criteria.

    This could be done by providing the equivalent data to a FROM data source in
    a SQL query, as well as any accompanying WHERE criteria.  The FROM data
    source may take the form of "FROM OBJECT ..." or "FROM ...".  A wildcard
    type data source might need to be implemented as multiple levels of
    subscription by the subscriber, or could be implemented by storing the
    wildcard path directly in the subscription list.  A SUBTREE data source may
    be able to be handled directly here.

    Subscriptions will, then, need to carry the following information:

	A.  Identity of subscribing user
	B.  Security context (endorsements, etc.) of subscription context
	C.  Pathname of object or directory being subscribed to
	D.  Object/subtree control flags, allowing:
	    1.	FROM /path
	    2.	FROM OBJECT /path
	    3.	FROM SUBTREE /path
	    4.	FROM INCLUSIVE SUBTREE /path
	E.  An expression identifying matching objects for the subscription


WEB APP DATA TRANSFER MECHANISM...

    There are several subparts to making the data transfer / replication update
    mechanism work for web applications (htmlgen):

    A.	Server-side OSRC list.  The app information structure on the server will
	need to be updated to contain a list of objectsources in the app.

    B.	Server-side OSRC result sets.  The server will need to cache the OSRC
	results that are sent to the client.  This is necessary so that the
	server can determine if an object has truly changed.  In many cases a
	replication update may result in an open object from a SQL result set
	being updated, but the update may not necessarily change the object's
	attributes.  The server will compare the old and updated data to see if
	the updated/patched object needs to be sent to the client or not.

    C.	Conversion of the HTTP interface to use JSON as the data transfer
	format instead of a list of encoded web links.

    D.	Server-side caching of active application structure (wgtr tree).  This
	will be used for managing and pushing modifications to the application's
	structure itself to the client.

    E.	Ability for server-side re-flow of the entire application layout.  This
	involves modifying and then re-submitting the entire widget tree to the
	APOS layout module, generating new layout information.
    
    F.	Ability for htmlgen drivers to re-generate widget HTML and parameters.
	Changes to the widget tree would then be submitted to the various
	htmlgen drivers to produce changes that need to be sent to the client, in
	the form of new html, stylesheets, and widget tree node / parameter
	values.
