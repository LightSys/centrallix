Document:  Centrallix System Terminology
Author:    Greg Beeley (GRB)
Date:      July 3rd, 2001
License:   Copyright (C) 2001 LightSys Technology Services.  See LICENSE.txt.
-------------------------------------------------------------------------------

OVERVIEW....

    Centrallix is a fairly innovative system, and as such it uses some fairly
    strange terminology.  This document seeks to explain some of that term-
    inology as well as some of Centrallix's internal conventions in order to
    introduce you to the system if you haven't programmed with (or for) it
    before.


TERMS AND DEFINITIONS....

    "Centrallix"

	Represents the idea that the Centrallix application server, when 
	deployed, operates in a very Central capacity in a web-based data
	management/processing environment.  It also is based on the self-
	similarity attribute of fractals, conceptually speaking, in reference
	to Centrallix's ability to embed objects within each other to the nth
	degree and retain those objects' meaning and detail.


    "Inner Type"

        An Object's inner type is its content type.  It describes what the
	object contains.  Some objects, such as filesystem directories, have
	a "system/void" content type, which means no content whatsoever.


    "MTASK"

        MTASK is Centrallix's nonpreemptive multithreading module.  It
	provides a number of services such as multithreading, synchronization,
	a simplified network sockets API, and more.


    "MultiQuery"

	The MultiQuery module is Centrallix's SQL query engine.  The OSML by
	itself only allows queries for subobjects of a single given object, and
	it only allows selection and sorting of that data (WHERE and ORDER BY).
	
	Thus, the MultiQuery module takes a SQL textual query and then uses the
	OSML to gather the data needed to generate the query results.  The
	returned results are returned through the normal OSML "fetch" routine,
	and their attributes obtained through "getattr".

	Currently, the MultiQuery module supports joins (and outer joins), 
	group-by and aggregates, sorting, selection, constant and computed
	fields, and projection (using only certain attributes from the under-
	lying objects).  It can join objects of very different types, such as
	joining a UN*X directory with the UN*X /etc/passwd database in order
	to provide a meaningful directory listing with the owners' real names.


    "Node Object"

        A Node Object is an object whose content type is such that the content
	can be further analyzed and broken down into more meaningful objects
	by another ObjectSystem driver.  It is analogous to a mount point in 
	the UN*X/Linux world.

	At a node object, one ObjectSystem driver's role ends and another one's
	begins.  For example, the ObjectSystem is normally rooted in a normal
	directory on the server, say /var/centrallix/os.  Next, consider what
	happens when a CSV data file is present in a directory there, perhaps
	/var/centrallix/os/data/mydata.csv.  In Centrallix's ObjectSystem,
	that location would be "/data/mydata.csv".  If the application is
	attempting to access a particular row in the file, the location would
	be perhaps "/data/mydata.csv/rows/93".  In this case, the node object
	is "mydata.csv".  This object's type, when viewed by the filesystem
	driver, is "system/uxfile", with a content type (inner type) of 
	"application/datafile".  The OSML knows that the "datafile" inner type
	can be handled by the DAT driver (ascii datafile driver), so it opens
	the file and passes the content through to the DAT driver, which then
	has the intelligence to parse the datafile into "mydata.csv/rows/93",
	the section of the ObjectSystem "path" that it handles in this case.

	Here's an illustration:

	/					[ROOT rootnode pseudo-driver]
	/data/mydata.csv			[UXD filesystem driver]
	      mydata.csv/rows/93		[DAT datafile driver]

	So, "/" is a node object for transfer from the ROOT driver to the
	UXD driver.  You set up that node object when you created the two files
	/usr/local/etc/rootnode and /usr/local/etc/rootnode.type.  Those files
	bootstrap the system and anchor the ObjectSystem.  The "mydata.csv"
	file is the second node object for transfer from the UXD driver, which
	knows about files and directories, to the DAT driver, which knows about
	various kinds of flat ASCII delimited-field data files.

	A strange thing takes place at a node object: the object's original 
	outer type disappears and its inner type becomes its new outer type.
	The new inner type is determined by the new driver.  Let's take for
	example the report-writing objectsystem driver (OSD), called "RPT".

	/					[ROOT rootnode pseudo-driver]
	/data/datareport.rpt			[UXD filesystem driver]
	      datareport.rpt			[DAT datafile driver]

	In this case, the familiar-by-now rootnode is still there.  The UXD
	driver handles the path to get to the .rpt file.  Then, the RPT driver
	takes over - but it doesn't go any further in the path!  OSDs don't 
	necessarily have to.  Instead, the "system/report" inner type then
	becomes an outer type according to the DAT driver, and the new inner
	type is normally "text/html", since the report writer by default 
	generates HTML reports.

	Essentially, the ObjectSystem is three-dimensional instead of two-
	dimensional.  For a developer, it will take a little bit of getting
	used to.... :)  But, end-users will only need to know the location
	(i.e., the URL) to run an application or report!


    "Object"

        A Centrallix object is an abstract entity consisting of several 
	possible components:

	    - An object type, which describes the object itself,
	    - A content type, which describes what the object contains, if it
	      has content.
	    - Content, which is the information or data the object contains.
	      In the case of a file object, the file object may contain HTML
	      data.  Note that the concept of an "HTML file" does not really
	      exist in Centrallix.  You can have a File object containing
	      HTML data, but other things can contain HTML data as well - 
	      including database BLOBs or email attachments.
	    - Attributes, which can be of various data types.  The kinds of
	      attributes an object might have are related more to the object
	      type than to the content type (e.g., a file object may have a
	      permissions bitmask but a BLOB may not).
	    - Methods, depending on the object type.
	    - Events, which are like Centrallix "IRQs" by which the object
	      itself can initiate action within the ObjectSystem via triggers
	      and such.
	    - A Name, which becomes that object's "primary key" as far as 
	      queries are concerned.
	    - An "annotation" which describes the object.  Annotations are
	      available on all objects in the ObjectSystem but are stored 
	      in different ways depending on the type of the object.


    "ObjectSystem"

        The ObjectSystem is Centrallix's high-level analog to a computer's
	filesystem.  It is a collection of objects of various types arranged
	in a familiar tree-and-directory type of structure.  See "Object" for
	more details.  Each Object in the ObjectSystem can potentially have any
	number of sub-objects, which are analogous to files and directories 
	within a directory.


    "ObjectSystem Driver" (or "OSD")

        An ObjectSystem Driver, or OSD, is a Centrallix module that has the
	intelligence to handle objects of a certain type, or objects of certain
	related types.  It can be instantiated many many times in the overall
	ObjectSystem if there are many objects of a type that the OSD knows
	about (when the OSD initializes, it tells the OSML what object types
	it can understand).

	Most OSD's interpret the contents of a Node Object and then break down
	that node object's content (or data on the network resource that the
	node object points to) into a subtree within the ObjectSystem.  That
	subtree is rooted at the node object in question.  For instance, CSV
	data has textual content that is meaningless to most of the OSD's in
	the system.  Except for the DAT (ascii datafile) driver, of course. :)
	The DAT driver takes the CSV data and breaks it down into a subtree
	consisting of two directories, "rows" and "columns", each of which 
	contain objects - one object for each row or for each column, 
	respectively.


    "OSML"

	OSML stands for "ObjectSystem Management Layer".  The OSML is the
	abstraction layer in Centrallix, and it provides a number of services
	to OSD's as well as to the application layer.  The OSML presents a 
	unified interface to the application layer, consisting of a number of
	fairly simple operations: open, close, create, delete, query, close
	query, fetch, getattr, setattr, and so forth.  The OSML also knows how
	to coordinate the efforts of the various OSD's when opening an object
	in the ObjectSystem.

	The OSML also provides services to the OSD's when the OSD author needs
	to simplify the OSD or when the data in question simply doesn't 
	naturally support what is being asked.  For instance, flat ascii CSV
	files don't naturally support sorting (in the same way that a remote
	RDBMS might), so the OSML has a builtin sorting facility when the OSD
	indicates that it cannot arbitrarily provide sorted data in response to
	a "query" request.  This is also true of selection (e.g., a WHERE 
	clause) - if an RDBMS can support WHERE clauses, then there is no need
	for the OSML to do it, but for CSV files and UN*X directories which do
	not support WHERE clauses, the OSML steps in and provides that support.

	The OSML also provides simple transaction services, via the OXT layer.


    "Outer Type"

        An object's object type.  This is the type of the object as a
	"container" - that is, as opposed to what the object "contains".  Outer
	types include files, database rows, directories, POP3 mailboxes, and
	such, whereas inner types (content types) include HTML, CSV data, C
	language code, Text, and so forth.


    "OXT"

        OXT stands for "ObjectSystem Transaction Tree", and is a simple trans-
	action layer for the ObjectSystem.  Some commonplace ObjectSystem
	operations can simply not be performed outside of the context of a
	transaction.  For instance, the creation of a database row object
	requires 1) create the row, 2) set each of the attributes, and 3) close
	the row object.  These three-plus operations must be combined together
	in a form of a transaction.  Some OSD's, therefore, request OXT support
	from the OSML, and the OSML essentially allows the operations to be
	batched until the application commits the operation by closing the 
	object.  The OSD then can take the create and various setattr 
	operations and execute them, creating the database row object.


    "Rootnode"

        The rootnode is a node object that bootstraps the Centrallix server
	and anchors the ObjectSystem's root.  The rootnode's configuration is
	somewhat special because it won't be found in the ObjectSystem itself
	like other node objects.  Instead, that configuration is in a
	predictable location (/usr/local/etc/rootnode).


    "Structure File"

        The Structure File is a basic file format native to Centrallix, and is
	quite similar to other file formats used by programs such as xinetd,
	logrotate, and BIND.  Basically, the Structure File allows for the 
	specification and nesting of various objects and object attributes.

	Currently, most node objects which reference external data sources
	(such as pointing to a POP3 maildrop or to a Sybase RDBMS) use the
	structure file format.  For a simple structure file, look at the
	/usr/local/etc/rootnode file.


-------------------------------------------------------------------------------
