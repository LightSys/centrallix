# ObjectSystem Driver Overview
Author:    Greg Beeley (GRB)

Date:      June 16th, 2003

## Overview
This document provides a summary of how an ObjectSystem driver works and the role that it fills within Centrallix.  It does not seek to go into detail about the inner workings of an ObjectSystem driver, but rather views the driver as a unit, within its context in Centrallix, including its relationship to other Centrallix components.

## Table of Contents
- [ObjectSystem Driver Overview](#objectsystem-driver-overview)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [The Objectsystem Driver](#the-objectsystem-driver)
  - [The Node Object](#the-node-object)
  - [Different Types of Drivers](#different-types-of-drivers)
    - [A.	External-Data Drivers](#aexternal-data-drivers)
    - [B.	Content Drivers](#bcontent-drivers)
    - [C.	Translation Drivers](#ctranslation-drivers)
    - [D.	Special-purpose Drivers](#dspecial-purpose-drivers)
  - [Services a Driver Provides](#services-a-driver-provides)
    - [A.	Opening and Closing objects](#aopening-and-closing-objects)
    - [B.	Creating and Deleting objects](#bcreating-and-deleting-objects)
    - [C.	Getting and Setting object attributes](#cgetting-and-setting-object-attributes)
    - [D.	Querying for subobjects](#dquerying-for-subobjects)
    - [E.	Executing Methods on objects](#eexecuting-methods-on-objects)
    - [F.	Reading and Writing object content](#freading-and-writing-object-content)
  - [How the OSML Interacts with the Driver](#how-the-osml-interacts-with-the-driver)
    - [A.	Initialization.](#ainitialization)
    - [B.	Accessing Objects.](#baccessing-objects)
    - [C.	Querying for Subobjects.](#cquerying-for-subobjects)
    - [D.  Enumerating Attributes of an Object.](#d--enumerating-attributes-of-an-object)
  - [How the Driver Interacts with the OSML](#how-the-driver-interacts-with-the-osml)

## The Objectsystem Driver
An objectsystem driver (OSD) is a module which allows the Centrallix system to access data of a specific type or data which is stored in a specific manner.  In this way, it is similar in concept to a "filesystem driver" within an operating system or perhaps in some ways even a database access driver for ODBC or similar systems.

The objectsystem drivers are important components of the data abstraction mechanism in Centrallix, managed by the OSML, or ObjectSystem Management Layer.  Together, the drivers and OSML create the "objectsystem", which is a filesystem-like collection of objects and subobjects arranged in a tree-structured hierarchy, where each piece of data is fully accessible and queryable from a functional standpoint.

## The Node Object
The objectsystem, as mentioned, consists of a tree-structured hierarchy, which actually consists of many smaller hierarchies of related objects, or subtrees.  For example, a given database may consist of a set of tables, and those tables will consist of columns and rows.  The database, tables, columns, and rows all comprise a subtree which is managed by one specific driver - in this case a driver with intelligence about that particular type of database server.

The driver may in reality manage a large number of such subtrees, each referring to a different database with different tables, columns, and rows within it.  The root objects of these subtrees (in this case, the database objects themselves) are called "node objects".  The node object is the top, or beginning, of a subtree of objects that a given driver works with, and as such, plays the critical role of joining that subtree into the rest of the objectsystem, thus the term 'node'.

However, unlike normal filesystems (where the 'mount point' is analogous to our 'node objects'), Centrallix automatically connects the subtrees into the ObjectSystem based on object types.  For example, if Centrallix finds data of a "CSV" type in a local file, it will transform the CSV data into a node object and invoke the CSV objectsystem driver to handle a subtree of columns and rows for the CSV data.  This happens wherever CSV data occurs, whether in a file, email attachment, database BLOB, or other similar type of location.  

For example, consider this situation where CSV data occurs in the file "/DirectoryOne/FileTwo.csv":

```
/ (root node object)
    |
    +-- DirectoryOne
    |	    |
    |	    +-- FileOne.txt
    |	    |
    |	    +-- FileTwo.csv
    |	    |
    |	    +-- FileThree.html
    |
    +-- DirectoryTwo
    |
    +-- DirectoryThree
```

If Centrallix had no intelligence about CSV data's structure, in the same way it has no intelligence about TXT data's structure, then the above diagram is how things would appear in the ObjectSystem.  However, Centrallix does have intelligence about CSV data, via the "datafile" objectsystem driver, and so "FileTwo.csv" becomes a node object, with the resulting tree looking something like this:

```
/ (root node object)
    |
    +-- DirectoryOne
    |	    |
    |	    +-- FileOne.txt
    |	    |
    |	    +-- FileTwo.csv
    |	    |	    |
    |	    |	    +-- columns
    |	    |	    |	    |
    |	    |	    |	    +-- first_name
    |	    |	    |	    |
    |	    |	    |	    +-- last_name
    |	    |	    |
    |	    |	    +-- rows
    |	    |	     	    |
    |	    |	     	    +-- 0
    |	    |	     	    |
    |	    |	     	    +-- 1
    |	    |	     	    |
    |	    |	     	    +-- 2
    |	    |
    |	    +-- FileThree.html
    |
    +-- DirectoryTwo
    |
    +-- DirectoryThree
```

So, in this case, "FileTwo.csv" serves in two very different roles:  first, as a file in the filesystem, and second, as a "table" of rows and columns. It marks the transition between the local filesystem OSD and the datafile OSD.

Thus, the datafile OSD does not obtain its information about the CSV data directly from the local filesystem, but instead from the filesystem OSD. This allows the datafile OSD to process CSV data from almost any location, rather than only from the local filesystem.

## Different Types of Drivers
ObjectSystem Drivers generally fall into one of several categories.  We'll take a look at them in this section.

### A.	External-Data Drivers
The external-data driver is an OSD which obtains most or all of its information from outside the ObjectSystem, but uses information in the ObjectSystem for configuration purposes.  An example of this is a driver that accesses an external database server - its node object in the ObjectSystem doesn't contain the actual database of course, but rather just the information about how to contact the database.  The node may contain information about the remote server's IP address, TCP port, database name, and other similar types of configuration information.  In this sense, the node *links* to the remote data, rather than being the data itself.

### B.	Content Drivers
A content driver has intelligence about how to analyze, pick apart, and break down content into meaningful data or structure.  It often needs no additional external information to help it sort through the content.  An example of this is the CSV datafile driver, which takes CSV content and breaks it down into its individual columns and rows.

### C.	Translation Drivers
A translation driver takes existing content or structure and transforms it into a different kind of content or structure that is useful in a different way than the original data.  Often, the original data is highly structured but the resulting content or data is in more of a summary form or a 'product' of sorts.  One example is the report writer driver, which can take information from all over the objectsystem and condense it down into a single piece of content which summarizes that data in a meaningful way.  Another example is the querytree OSD, which builds a new tree structure from data it obtains from various locations in the objectsystem.

### D.	Special-purpose Drivers
Some OSD's are used for special purposes, usually internal to the OSML itself.  One example of this is the "rootnode" OSD, which is used to configure the root of the objectsystem.  It normally is set to use a local filesystem directory for the root, but essentially almost any object type can be the root if needed.

## Services a Driver Provides
An OSD will need to provide serveral important services to the OSML for the type of data that it manages.  The most significant of these are summarized below.

### A.	Opening and Closing objects
The driver will need to provide a way of opening and closing objects within the subtree of objects that it manages.  In the case of some drivers, like the report writer, the subtree is just one object - the generated report itself.  In the case of other drivers like a database access driver, several different types of objects might be involved: rows, columns, the database itself, and so forth.  The driver in such cases will need to determine which kind of object is being opened and respond accordingly.

### B.	Creating and Deleting objects
The driver will need to provide the capability to create new objects and delete existing ones, if applicable.

### C.	Getting and Setting object attributes
Objects will have various attributes associated with them.  The driver will provide a way for these attributes to be accessed.

### D.	Querying for subobjects
Many objects will have subobjects (like files in a directory or attachments in an email).  The driver will need to provide a way to obtain the list of subobjects for a given object, possibly sorting the results or selecting subobjects that meet a criteria, although the OSML can itself provide sorting and selection if the driver can't (drivers accessing remote data, such as in a database, will be more likely to be able to provide sorting and selection services; others should just let the OSML take care of those tasks).

### E.	Executing Methods on objects
Some objects will have certain methods associated with them, and in these cases the driver will need to provide a way to execute the method.  An example of a method would be compacting a CSV datafile that has accumulated empty space from deletes or modifications.

### F.	Reading and Writing object content
For objects that have content, drivers will need to allow for an object's content to be read from and written to.

## How the OSML Interacts with the Driver
In this section we'll examine the API between the objectsystem driver and the OSML, from the OSML's perspective.  In the next section we'll take a look at it more from the driver's perspective and define some additional services the OSML provides to the driver.  For more information about the details of these various routines, see the 'objectsystem driver authoring guide'.

### A.	Initialization.
Although outside the context of the OSML at least initially, the first thing that happens to a driver is that its Initialize() routine is called when Centrallix first starts up.  The Initialize() routine is not passed any arguments, and should return 0 on success and a negative value on failure (either just -1 or -ERROR where ERROR is a value from /usr/include/errno.h or the files errno.h includes).

For an ObjectSystem driver, the Initialize routine is expected to register the driver with the OSML so that the OSML knows to use the driver for the types of data the driver supports.  For more information on registration with the OSML, see the 'objectsystem driver authoring guide'.

    1.	Centrallix --> OSD		Initialize() routine called.
    2.	               OSD		Driver initializes itself
    3.	               OSD --> OSML	Driver registers with OSML
    4.	               OSD <-- OSML	Register succeeds or fails
    5.	Centrallix <-- OSD		Init succeeds or fails

### B.	Accessing Objects.
When the OSML needs to access an object handled by the driver, then the driver's Open() routine is called.  The result of the Open will be a handle (opaque pointer) to the open object from the driver's perspective.  When the OSML is done with the object, it will call the Close() routine for that object, passing the same opaque pointer to the driver's Close() routine that was obtained from the Open() routine.

The driver will need to access the node object via the OSML to figure out where to connect to externally or to obtain the content that it should be breaking down into meaningful structure.

    1.	OSML --> OSD			Open object
    2.	         OSD --> OSML		Accesses node using OSML API
    3.	         OSD <-- OSML		Driver gets data for node obj.
    4.	OSML <-- OSD			Returns object handle

    5.	OSML --> OSD			Close object
    6.	         OSD			Cleans up object
    7.	OSML <-- OSD			Close routine returns

Here is a more detailed look at how a row in our CSV file from the previous example might be opened.  Note that many of the details have been left out in order to highlight the overall process.

In this view, the various pieces are abbreviated as follows to make things fit on the page:  U=user, O=OSML, R=rootnode driver, F=filesystem driver, D=datafile driver.

    1. U -> O           User requests that the object /DirectoryOne/FileTwo.csv/rows/0 be opened.
    2.      O -> R      Open an instance of the / object and
            O <- R      use that as the first object in the 
                        chain of open objects needed to get 
                        at the object the user wants.
    3.      O           Discovers that rootnode points to a local filesystem tree.
    4.      O -> F      OSML tries to get local filesystem driver to open the given path, /DirectoryOne/FileTwo.csv/rows/0
    5.      O <- F      Filesystem driver reports that it can only open /DirectoryOne/FileTwo.csv, and returns a handle for it.
    6.      O           Adds an object to the chain of open objects for the open file, and figures out that the content of the FileTwo.csv is CSV data.
    7.      O -> D      OSML tries to get the datafile driver to open up FileTwo.csv/rows/0, the remainder of the path.
    8.           D -> O datafile driver uses the OSML API to access the open object created in step 5 above (the file).
    9.                O -> F    OSML requests content for the file from the local filesystem driver.
    10.               O <- F    Local filesystem driver returns the needed content.
    11.          D <- O         content of the file is returned back to the datafile driver.
    12.     O <- D      datafile driver reports to the OSML that it could open the entire rest of the path, FileTwo.csv/rows/0, and returns a handle for that CSV row.
    13.     O           Adds a third object to the open object chain, for the CSV row, and realizes that entire path has been handled, thus completing the open operation.
    14.   U <- O        An open object handle is returned to the user for the CSV row object, accessed via the filesystem file, and that accessed via the rootnode.

In the end of this process, an open object chain has been created which contains the following three objects:

    Rootnode (/)  -->  Local File (FileTwo.csv)  -->  CSV Row (0)

All that the user sees or cares about is the open CSV row.  However, the datafile driver is quite interested in the content of "FileTwo.csv" and the local filesystem driver is quite interested in the rootnode (so it can find out what directory in the local filesystem to look in in order to locate "/DirectoryOne/FileTwo.csv").

Once an object has been accessed, either via the open procedure described here, or via fetching the object from a query, various operations can be performed on the object, including reading/writing its content, getting/setting attributes, executing methods, and running queries for subobjects.  We'll take a look at queries next.  For each of these operations, the OSML will always pass the handle for the object back to the driver so the driver knows what object is being dealt with (this is the same value that the driver returns in its Open routine, but is not the value that is returned to the user - the user sees a pObject pointer, which is a structure the OSML maintains, but the driver is passed whatever type of pointer it chose to return from its Open routine).

### C.	Querying for Subobjects.
It is extremely important for the OSML to be able to get a list of subobjects for a given object from the driver.  It will do this by performing a Query.

Queries at the driver level are only performed on open objects - in order for an object to be queried, it MUST be opened first!

There are several OSML->driver calls that are involved in a query operation:

| Call          | Description
| ------------- | ------------
| 1. Open       | The Open call is used to open the object that is to be queried for subobjects.
| 2. OpenQuery  | This routine requests that a query for subobjects be started.  It does not return any of the subobjects.
| 3. QueryFetch | This routine obtains the next item in the query results.  It should be completely indistin- guishable from an item opened by Open directly instead of querying.  In other words, querying /File.csv/rows and fetching row 0 should be exactly the same as opening /File.csv/rows/0.
| 4. QueryClose | Closes an open query, but does not close the objects fetched by the query.  Those will be closed with the Close routine, just like objects obtained with Open.
| 5. Close      | Closes an object, whether obtained via Open or via QueryFetch.

Next is an example of how a query might proceed for the three row objects in our CSV file example.  For this example, we'll assume that /DirectoryOne/FileTwo.csv/rows has already been opened as in the previous example.  We'll also use the same abbreviations.

```
1.	U -> O			User requests a query of all row objects from /DirectoryOne/FileTwo.csv/ rows/ which have a row id not equal to 1.
2.	     O -> D		OSML opens a query on the datafile driver asking for all such rows.
3.	     O <- D		Datafile driver reports that it can do the query but that the selection of matching rows is left up to the OSML, and returns a handle for the query.
4.	U <- O			OSML returns an open query handle to the user (but not the same structure as returned by #3 above).
5.	U -> O			User requests that the next subobject be fetched.
6.	     O -> D		OSML asks the datafile driver for the next row object.
7.	          D -> O	Datafile driver accesses the local file object in order to read the content of the CSV row.
8.	               O -> F	OSML reads the data from the local file itself.
9.	               O <- F	Local filesystem driver returns the text of the row at the offset requested
10.	          D <- O	OSML returns the text of the row to the datafile driver.
11.	     O <- D		Datafile driver parses the row into its attributes and passes the open row object back to the OSML.
12.	     O			The OSML examines the object to see if it meets the query criteria.  Since the row id is 0, which is != 1, the row does match.
13.	U <- O			The fetched object is returned to the user.
14.	U -> O			User decides to keep the first object open for the time being and fetches the next one.
15.	     O -> D		OSML fetches the next row from the datafile driver. ...			(detail omitted for brevity)
16.	     O <- D		Datafile driver returns the next row, with row id 1.
17.	     O			OSML examines the object, which does not match the criteria.
18.	     O -> D		OSML closes the row id 1 object.
19.	     O <- D		Datafile driver cleans up the open object and returns OK to the OSML.
20.	     O -> D		OSML fetches the next row from the datafile driver. ...			(detail omitted for brevity)
21.	     O <- D		Datafile driver returns row id 2.
22.	     O			OSML examines the row, which matches the criteria.
23.	U <- O			Row object is returned to the user.
24.	U -> O			User fetches next object.
25.	     O -> D		OSML tries to get next row from the datafile OSD. ...			(detail omitted for brevity)
26.	     O <- D		Datafile driver returns NULL to say that there are no more rows.
27.	U <- O			OSML tells user that the row was the last row.
28.	U -> O			User goes ahead and closes the query, but leaves the two row objects open for the time. -- objQueryClose()
29.	     O -> D		OSML tells the datafile OSD that the query is now closed, via QueryClose.
30.	     O <- D		Datafile driver says "OK, closed."
31.	U <- O			OSML tells the user the query has now been closed, via return from the objQueryClose OSML API routine.
32.	U -> O			User finally decides to close the first row via objClose()
33.	     O -> D		OSML calls Close on the row id 0 object on the datafile OSD, using the handle returned by QueryFetch.
34.	     O <- D		Datafile driver says "ok, closed" by returning from Close.
35.	U <- O			OSML tells user object is closed by returning successfully from objClose.
36.	U -> O			User finally decides to close the last row via objClose()
37.	     O -> D		OSML calls Close on the row id 2 object on the datafile OSD, using the handle returned by QueryFetch.
38.	     O <- D		Datafile driver says "ok, closed" by returning from Close.
39.	U <- O			OSML tells user object is closed by returning successfully from objClose.
```

### D.  Enumerating Attributes of an Object.
The driver will need to allow the OSML to enumerate all of the object's attributes in addition to getting/setting their values.  This is done via the GetFirstAttr and GetNextAttr OSD calls.  GetFirstAttr is used to start the enumeration, and returns the name of the first attribute. GetNextAttr returns subsequent attributes.  Either can return NULL if no more attributes are found.  The following 'system' attributes should not be returned by the attribute enumeration functions as they must be present on all objects.

| Attribute                    | Description
| ---------------------------- | ------------
| name                         | The name (or primary key value) of the object.
| annotation                   | An object's annotation.
| inner_type (or content_type) | the type of the content of an object.
| content_type                 | Same as above.
| outer_type                   | The type of the object itself.

The system attribute 'last_modification' can be returned in the enumeration if desired, as not all objects will have this attribute, though it is important for letting the OSML know when an object has been modified.

## How the Driver Interacts with the OSML
The objectsystem driver will need to use the OSML for several important things.

First, the OSML provides a number of convenience or utility functions that the driver will likely use in the course of operation.  For more details on these, see the 'objectsystem driver authoring guide'.

Second the driver will use the normal OSML API for accessing not only its node object (obj->Prev internally) but also for accessing any other objects that it might need along the way (like the report writer OSD might do). Below is a quick list of some of the OSML API routines:

- objOpen
- objClose
- objCreate
- objDelete
- objOpenQuery
- objQueryFetch
- objQueryClose
- objQueryDelete
- objGetAttrType
- objGetAttrValue
- objSetAttrValue
- objAddAttr
- objRead
- objWrite
- objGetFirstAttr
- objGetNextAttr
- objGetFirstMethod
- objGetNextMethod
- objExecMethod
- objInfo
- objCommit
- objOpenSession
- objCloseSession
- objPresentationHints
