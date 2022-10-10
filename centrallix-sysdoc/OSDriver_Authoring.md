# ObjectSystem Driver Interface
Author:	  Greg Beeley

Date:	  January 13, 1999

Updated:  March 9, 2011

License:  Copyright (C) 2001-2011 LightSys Technology Services. See LICENSE.txt for more information.

## Table of Contents
- [ObjectSystem Driver Interface](#objectsystem-driver-interface)
  - [Table of Contents](#table-of-contents)
  - [I Introduction](#i-introduction)
  - [II Interface](#ii-interface)
    - [A.  Initialization](#a--initialization)
    - [B.  Opening And Closing Objects](#b--opening-and-closing-objects)
    - [C.  Creating and Deleting Objects.](#c--creating-and-deleting-objects)
    - [D.  Reading and Writing Object Content.](#d--reading-and-writing-object-content)
    - [E.  Querying for Child Objects.](#e--querying-for-child-objects)
    - [F.  Managing Object Attributes](#f--managing-object-attributes)
    - [G.  Managing Object Methods](#g--managing-object-methods)
  - [III Reading the Node Object](#iii-reading-the-node-object)
    - [pSnNode snReadNode(pObject obj)](#psnnode-snreadnodepobject-obj)
    - [pSnNode snNewNode(pObject obj, char* content_type)](#psnnode-snnewnodepobject-obj-char-content_type)
    - [int snWriteNode(pSnNode node)](#int-snwritenodepsnnode-node)
    - [int snDeleteNode(pSnNode node)](#int-sndeletenodepsnnode-node)
    - [int snGetSerial(pSnNode node)](#int-sngetserialpsnnode-node)
    - [pStructInf stParseMsg(pFile inp_fd, int flags)](#pstructinf-stparsemsgpfile-inp_fd-int-flags)
    - [pStructInf stParseMsgGeneric(void* src, int (*read_fn)(), int flags)](#pstructinf-stparsemsggenericvoid-src-int-read_fn-int-flags)
    - [int stGenerateMsg(pFile out_fd, pStructInf info, int flags)](#int-stgeneratemsgpfile-out_fd-pstructinf-info-int-flags)
    - [int stGenerateMsgGeneric(void* dst, int (*write_fn)(), pStructInf info, int flags)](#int-stgeneratemsggenericvoid-dst-int-write_fn-pstructinf-info-int-flags)
    - [pStructInf stCreateStruct(char* name, char* type)](#pstructinf-stcreatestructchar-name-char-type)
    - [pStructInf stAddAttr(pStructInf inf, char* name)](#pstructinf-staddattrpstructinf-inf-char-name)
    - [pStructInf stAddGroup(pStructInf inf, char* name, char* type)](#pstructinf-staddgrouppstructinf-inf-char-name-char-type)
    - [int stAddValue(pStructInf inf, char* strval, int intval)](#int-staddvaluepstructinf-inf-char-strval-int-intval)
    - [pStructInf stLookup(pStructInf inf, char* name)](#pstructinf-stlookuppstructinf-inf-char-name)
    - [int stAttrValue(pStructInf inf, int* intval, char** strval, int nval)](#int-stattrvaluepstructinf-inf-int-intval-char-strval-int-nval)
    - [int stFreeInf(pStructInf this)](#int-stfreeinfpstructinf-this)
  - [IV Memory Management in Centrallix](#iv-memory-management-in-centrallix)
    - [void* nmMalloc(int size)](#void-nmmallocint-size)
    - [void nmFree(void* ptr, int size)](#void-nmfreevoid-ptr-int-size)
    - [void nmStats()](#void-nmstats)
    - [void nmRegister(int size, char* name)](#void-nmregisterint-size-char-name)
    - [void nmDebug()](#void-nmdebug)
    - [void nmDeltas()](#void-nmdeltas)
    - [void* nmSysMalloc(int size)](#void-nmsysmallocint-size)
    - [void nmSysFree(void* ptr)](#void-nmsysfreevoid-ptr)
    - [void* nmSysRealloc(void* ptr, int newsize)](#void-nmsysreallocvoid-ptr-int-newsize)
    - [char* nmSysStrdup(const char* str)](#char-nmsysstrdupconst-char-str)
  - [V Other Utility Modules](#v-other-utility-modules)
    - [A.	XArray (XA) - Arrays](#axarray-xa---arrays)
      - [xaInit(pXArray this, int init_size)](#xainitpxarray-this-int-init_size)
      - [xaDeInit(pXArray this)](#xadeinitpxarray-this)
      - [xaAddItem(pXArray this, void* item)](#xaadditempxarray-this-void-item)
      - [xaAddItemSorted(pXArray this, void* item, int keyoffset, int keylen)](#xaadditemsortedpxarray-this-void-item-int-keyoffset-int-keylen)
      - [xaFindItem(pXArray this, void* item)](#xafinditempxarray-this-void-item)
      - [xaRemoveItem(pXArray this, int index)](#xaremoveitempxarray-this-int-index)
    - [B.	XHash (XH) - Hash Tables](#bxhash-xh---hash-tables)
    - [int xhInit(pXHashTable this, int rows, int keylen)](#int-xhinitpxhashtable-this-int-rows-int-keylen)
      - [int xhDeInit(pXHashTable this)](#int-xhdeinitpxhashtable-this)
      - [int xhAdd(pXHashTable this, char* key, char* data)](#int-xhaddpxhashtable-this-char-key-char-data)
      - [int xhRemove(pXHashTable this, char* key)](#int-xhremovepxhashtable-this-char-key)
      - [char* xhLookup(pXHashTable this, char* key)](#char-xhlookuppxhashtable-this-char-key)
      - [int xhClear(pXHashTable this, int free_blk)](#int-xhclearpxhashtable-this-int-free_blk)
    - [C.	XString (XS) - Strings](#cxstring-xs---strings)
      - [int xsInit(pXString this)](#int-xsinitpxstring-this)
      - [int xsDeInit(pXString this)](#int-xsdeinitpxstring-this)
      - [int xsConcatenate(pXString this, char* text, int len)](#int-xsconcatenatepxstring-this-char-text-int-len)
      - [int xsCopy(pXString this, char* text, int len)](#int-xscopypxstring-this-char-text-int-len)
      - [char* xsStringEnd(pXString this)](#char-xsstringendpxstring-this)
    - [D.	Expression (EXP) - Expression Trees](#dexpression-exp---expression-trees)
      - [pExpression expCompileExpression(char* text, pParamObjects objlist, int lxflags, int cmpflags)](#pexpression-expcompileexpressionchar-text-pparamobjects-objlist-int-lxflags-int-cmpflags)
      - [expFreeExpression(pExpression this)](#expfreeexpressionpexpression-this)
      - [int expEvalTree(pExpression this, pParamObjects objlist)](#int-expevaltreepexpression-this-pparamobjects-objlist)
      - [pParamObjects expCreateParamList()](#pparamobjects-expcreateparamlist)
      - [int expFreeParamList(pParamObjects this)](#int-expfreeparamlistpparamobjects-this)
      - [int expAddParamToList(pParamObjects this, char* name, pObject obj, int flags)](#int-expaddparamtolistpparamobjects-this-char-name-pobject-obj-int-flags)
      - [int expModifyParam(pParamObjects this, char* name, pObject replace_obj)](#int-expmodifyparampparamobjects-this-char-name-pobject-replace_obj)
      - [int expRemoveParamFromList(pParamObjects this, char* name)](#int-expremoveparamfromlistpparamobjects-this-char-name)
      - [int expReverseEvalTree(pExpression tree, pParamObjects objlist)](#int-expreverseevaltreepexpression-tree-pparamobjects-objlist)
    - [E.	MTSession (MSS) - Basic Session Management](#emtsession-mss---basic-session-management)
      - [char* mssUserName()](#char-mssusername)
      - [char* mssPassword()](#char-msspassword)
      - [int mssSetParam(char* paramname, char* param)](#int-msssetparamchar-paramname-char-param)
      - [char* mssGetParam(char* paramname)](#char-mssgetparamchar-paramname)
      - [int mssError(int clr, char* module, char* message, ...)](#int-msserrorint-clr-char-module-char-message-)
      - [int mssErrorErrno(int clr, char* module, char* message, ...)](#int-msserrorerrnoint-clr-char-module-char-message-)
    - [F.	OSML Utility Functions](#fosml-utility-functions)
      - [char* obj_internal_PathPart(pPathname path, int start, int length)](#char-obj_internal_pathpartppathname-path-int-start-int-length)
      - [int obj_internal_AddToPath(pPathname path, char* new_element)](#int-obj_internal_addtopathppathname-path-char-new_element)
      - [int obj_internal_CopyPath(pPathname dest, pPathname src)](#int-obj_internal_copypathppathname-dest-ppathname-src)
      - [void obj_internal_FreePathStruct(pPathname path)](#void-obj_internal_freepathstructppathname-path)
  - [VI Network Connection Functionality](#vi-network-connection-functionality)
    - [pFile netConnectTCP(char* host_name, char* service_name, int flags)](#pfile-netconnecttcpchar-host_name-char-service_name-int-flags)
    - [int netCloseTCP(pFile net_filedesc, int linger_msec, int flags)](#int-netclosetcppfile-net_filedesc-int-linger_msec-int-flags)
    - [int fdWrite(pFile filedesc, char* buffer, int length, int offset, int flags)](#int-fdwritepfile-filedesc-char-buffer-int-length-int-offset-int-flags)
      - [int fdRead(pFile filedesc, char* buffer, int maxlen, int offset, int flags)](#int-fdreadpfile-filedesc-char-buffer-int-maxlen-int-offset-int-flags)
  - [VII Parsing Data](#vii-parsing-data)
    - [pLxSession mlxOpenSession(pFile fd, int flags)](#plxsession-mlxopensessionpfile-fd-int-flags)
    - [pLxSession mlxStringSession(char* str, int flags)](#plxsession-mlxstringsessionchar-str-int-flags)
    - [int mlxCloseSession(pLxSession this)](#int-mlxclosesessionplxsession-this)
    - [int mlxNextToken(pLxSession this)](#int-mlxnexttokenplxsession-this)
    - [char* mlxStringVal(pLxSession this, int* alloc)](#char-mlxstringvalplxsession-this-int-alloc)
    - [int mlxIntVal(pLxSession this)](#int-mlxintvalplxsession-this)
    - [double mlxDoubleVal(pLxSession this)](#double-mlxdoublevalplxsession-this)
    - [int mlxCopyToken(pLxSession this, char* buffer, int maxlen)](#int-mlxcopytokenplxsession-this-char-buffer-int-maxlen)
    - [int mlxHoldToken(pLxSession this)](#int-mlxholdtokenplxsession-this)
    - [int mlxSetOptions(pLxSession this, int options)](#int-mlxsetoptionsplxsession-this-int-options)
    - [int mlxUnsetOptions(pLxSession this, int options)](#int-mlxunsetoptionsplxsession-this-int-options)
    - [int mlxSetReservedWords(pLxSession this, char** res_words)](#int-mlxsetreservedwordsplxsession-this-char-res_words)
    - [int mlxNoteError(pLxSession this)](#int-mlxnoteerrorplxsession-this)
    - [int mlxNotePosition(pLxSession this)](#int-mlxnotepositionplxsession-this)
  - [VIII Objectsystem Driver Testing](#viii-objectsystem-driver-testing)
    - [A.	Object opening, closing, creation, and deletion](#aobject-opening-closing-creation-and-deletion)
    - [B.	Object attribute enumeration, getting, and setting.](#bobject-attribute-enumeration-getting-and-setting)
    - [C.	Object querying (for subobjects)](#cobject-querying-for-subobjects)

## I Introduction
An objectsystem driver's purpose is to provide access to a particular type of local or network data/resource, and to organize that data in a tree- structured heirarchy that can be integrated into the Centrallix's ObjectSystem.  This tree structure will vary based on the data being presented, but will fit the basic ObjectSystem model of a heirarchy of objects, each having attributes, perhaps some methods, and possibly content.  

Each objectsystem driver will implement this subtree structure rooted at what is called the "node" object.  The node has a specifically recognizable object type which the ObjectSystem Management Layer uses to determine which OS Driver to pass control to.  Normally, the 'node' object is a UNIX file either with a particular extension registered with the OSML, or a UNIX file residing in a directory containing a '.type' file, which contains the explicit object type for all objects in that directory without recognizable extensions.

Normally, objectsystem drivers will be able to manage any number of 'node' objects and the subtrees rooted at them.  Each 'node' object will normally relate to a particular instance of a network resource, or in some cases, a group of resources that are easily enumerated.  For example, a POP3 server would be a network resource that an OS driver could be written for.  If the network had multiple POP3 servers, then that one OS driver would be able to access each of them using different node objects.  However, if somehow the OS driver were able to easily enumerate the various POP3 servers on the network (i.e., they responded to some kind of hypothetical broadcast query), then the OS driver author could optionally design the driver to list the POP3 servers under a single node for the whole network.

The structure of the subtree beneath the node object is entirely up to the drivers' author to determine; the OSML does not impose any structural restrictions on such subtrees.

Here is one example of an OS Driver's node object and subtree (this is for the Sybase OS Driver, objdrv_sybase.c):

```
OMSS_DB (type = application/sybase)
    |
    +--- JNetHelp (type = system/table)
    |       |
    |	+--- columns (type = system/table-columns)
    |	|       |
    |	|	+--- document_id (type = system/column)
    |	|	|
    |	|	+--- parent_id (type = system/column)
    |	|	|
    |	|	+--- title (type = system/column)
    |	|	|
    |	|	+--- content (type = system/column)
    |	|
    |	+--- rows (type = system/table-rows)
    |	        |
    |	        +--- 1 (type = system/row)
    |	        |
    |	        +--- 2 (type = system/row)
    |
    +--- Partner (type = system/table)
```

(... and so forth)

In this case the node object would contain the information necessary to access the database, such as server name, database name, max connections to pool, and so forth.  More about the node object and managing its parameters will be discussed later in this document.

OS Drivers support several primary areas of functionality:  opening and closing objects, reading and writing object content (if the object has content), setting and viewing object attributes, executing object methods, and querying an object's child objects based on name and/or attribute values.  Drivers will also support the creation and deletion of objects and/or a set of child objects.

## II Interface
This section describes the standard interface between the OSML and the ObjectSystem driver itself.

### A.  Initialization
Each OS Driver will have an initialization function, normally named xxxInitialize() where 'xxx' is the driver's abbreviative prefix.  This prefix should be attached to each and every function within the OS driver for consistency and project management.  Normally 'xxx' is two to four characters, all lowercase.  This initialization function is called when the Centrallix starts up, and at least at the present time, this initial call to the OS driver must be manually added to the appropriate startup code, currently found in 'centrallix.c'.

Within the initialization function, the driver should initialize all necessary global variables and register itself with the OSML.  Global variables should all be placed inside a single global 'struct', which is normally named similarly to the driver's prefix, except normally in all uppercase.  Under no circumstances should global variables be accessed outside of the module, except via the module's functions.

To register with the OSML, the driver must first allocate an ObjDriver structure and fill in its contents.  

    pObjDriver drv;

    drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));

This involves setting a large number of fields to the appropriate entry points within the OS Driver, as well as telling the OSML what object type(s) are handled by the driver and giving the OSML a description of the driver.  A list of the required entry point functions / fields follows:

| Function/Field       | Description
| -------------------- | ------------
| Open                 | Function that the OSML calls when the user opens an object managed by this driver.
| Close                | Close an open object.
| Create               | Create a new object.
| Delete               | Delete an existing object.
| OpenQuery            | Start a query for child objects.
| QueryDelete          | Delete all objects in the query result set.
| QueryFetch           | Open the next child object in the query's result set.
| QueryClose           | Close an open query.
| Read                 | Read content from the object.
| Write                | Write content to the object.
| GetAttrType          | Get the type of an object's attribute.
| GetAttrValue         | Get the value of an object's attribute.
| GetFirstAttr         | Get the first attribute associated with the object.
| GetNextAttr          | Get the next attribute associated with the object.
| SetAttrValue         | Set the value of an attribute.
| AddAttr              | Add a new attribute to an object.
| OpenAttr             | Open an attribute as if it were an object with content.
| GetFirstMethod       | Get the first method of the object.
| GetNextMethod        | Get the next method of an object.
| ExecuteMethod        | Execute a method with an optional string parameter.

The only method that can be set to NULL is the QueryDelete method, in which case the OSML will call QueryFetch() and Delete() in succession.  However, if the underlying network resource has the capability of intelligently deleting objects matching the query's criteria, this method should be implemented (as with a database server).

Another field in the driver structure is the Capabilities field. This field is a bitmask, and can currently contain zero or more of the following options:

- OBJDRV_C_FULLQUERY: Indicates that this objectsystem driver will intelligently process the query's expression tree specified in the OpenQuery call, and will only return objects that match that expression.  If this flag is missing, the OSML will filter objects returned by QueryFetch so that the calling user does not get objects that do not match the query. Typically this is set by database server drivers.

    THE ABOVE IS OUT-OF-DATE.  From now on, a driver can determine whether to handle the Where and OrderBy on a per-query basis, by setting values in the ObjQuery structure used when opening a new query.  This is because a driver may be able to handle Where and OrderBy for some object listings but not for others.

- OBJDRV_C_TRANS: Indicates that this objectsystem driver requires transaction management by the OSML's transaction layer (the OXT layer).  OS drivers that require this normally are those that for some reason cannot complete operations in independence from one another.  For example, with a database driver, the creation of a new row object and the setting of its attributes must be done as one operation, although the operation requires several calls from the end user's process.  The OXT allows for the grouping of objectsystem calls so that the os driver does not have to complete them independently, but instead can wait until several calls have been made before actually completing the operation.

The 'Name' field should be filled in with a description of the OS driver, with a maximum length of 63 characters (plus the string null terminator).  Normally, the 2-4 letter prefix of the driver is included at the beginning of 'Name', such as "UXD - UNIX filesystem driver".  

Finally, the 'RootContentTypes' field is an XArray containing a list of strings, each of which specifies the node object types that the driver will handle.  Such types are added to this XArray using the normal XArray utility functions, such as:

    xaInit(&drv->RootContentTypes, 16);
    xaAddItem(&drv->RootContentTypes, "system/file");
    xaAddItem(&drv->RootContentTypes, "system/directory");

When the structure has been filled out, the os driver should call the OSML to register itself, using the objRegisterDriver function:

    objRegisterDriver(drv);

The initialization function should return 0 to indicate success, or -1 on failure.  Currently, initialization success/failure is not verified by lsmain.c.

The driver should NOT nmFree() the allocated driver structure unless the objRegisterDriver() routine fails (returns -1).

Note that the RootContentTypes handled by the driver should only include the types of the objects this driver will handle as node objects.  For instance, the Sybase database access driver uses "application/sybase" as its top level type.  It won't register such things as "system/table".

### B.  Opening And Closing Objects
As an overview, the normal procedure for the open routine to follow is this:

1.  Access the node object, or create it, depending on whether the object already exists as well as the open mode flags indicated by the end-user.
2.  Upon successful node object access, determine what additional components of the pathname are to be handled by this driver, and verify that they can be opened, depending on the object's open mode (CREAT, EXCL, etc.)
3.  If it hasn't been already, allocate a structure that will represent this open object and contain information about it and how we're to handle it.  It should include a pointer to the node object.
4.  Perform any operations inherent in the open process that have not already been performed (such as reading database table information, etc., when a db table's row is being accessed).
5.  Return a pointer to the structure allocated in (3) as a void pointer.  The OSML will pass this pointer back to the driver on subsequent calls that involve this object.

The first basic part of the OS driver consists of the Open and Close routines, normally named 'xxxOpen' and 'xxxClose' within the driver, where 'xxx' is the driver's prefix.  The Close routine is normally fairly simple, but the Open routine is one of the most complicated routines in a typical OS driver, for the Open routine must parse the subtree pathname beneath the node object.  For example, if the node object had a pathname like:

    /datasources/OMSS_DB

and the user opened an object called:

    /datasources/OMSS_DB/JNetHelp/rows/1

the OS driver would have to determine what the subtree pathname 'JNetHelp/rows/1' means, since this path will mean different things to different os drivers.

The Open routine also must determine whether the object already exists or not, and if not, whether to create a new object.  This logic is largely dependent on the obj->Mode flags, as if O_CREAT is included, the driver must attempt to create the object if it does not already exist, and if O_EXCL is included, the driver must refuse to open the object if it already exists, as with the UNIX open() system call semantics.  

Finally, if the os driver specified a capability of OBJDRV_C_TRANS, it must pay attention to the current state of the end-user's trans- action.  If a new object is being created, an object is being deleted, or other modifications/additions are being performed, and if the OXT layer indicates a transaction is in process, the driver must either complete the current transaction and then complete the current call, or else add the current delete/create/modify call to the transaction tree (in which case the tree item is preallocated; all the driver needs to do is fill it in).  The transaction layer will be discussed in depth later in this document.

As a part of the Open process, the OS driver will normally allocate an internal structure to represent the current open object, and will return that structure as a void* data type in the return value.  This pointer will be then passed to each of the other driver entry point functions, with the exception of QueryFetch, QueryDelete, and Query- Close, which will be discussed later.

The Open() routine is called with five parameters:

- obj (pObject)
    This is a pointer to the Object sturcture maintained by the OSML.  This structure will contain some important fields for processing the open() request.

    obj->Mode is a bitmask of the O_* flags, which include O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, and O_EXCL.

    obj->Pathname is a Pathname structure which contains the complete parsed pathname for the object.  This structure is defined in the file include/obj.h, and has a buffer for the pathname as well as an array of pointers to the pathname's components.  The function obj_internal_PathPart() can be used to obtain at will any component or series of components of the pathname.

    obj->Pathname->OpenCtl[] contains parameters to the open() operation.  Frequently these params provide additional information on how to open the object.  The use of these parameters is determined by the author of the objectsystem driver.  The parameters are those passed in normal URL fasion (?param=value, etc.). Typically, the only OpenCtl of interest is going to be obj->Pathname->OpenCtl[obj->SubPtr] (see below for SubPtr meaning).

    obj->SubPtr is the number of components in the path that are a part of the node object's path.  For example, in the above path of '/datasources/OMSS_DB', the path would be internally represented as './datasources/ OMSS_DB', and the SubPtr would be 3.

    obj->SubCnt reflects the number of components of the path which are under the control of the current driver.  This includes the node object, so SubCnt will always be at least 1.  For example, when opening '/data/file.csv/rows/1', and the driver in question is the CSV driver, SubPtr would be 3 (includes an "invisible" first component), from '/data/file.csv', and SubCnt would be 3, from 'file.csv/rows/1'. The driver will need to SET THE SUBCNT value in its Open function.  SubPtr is already set.

    obj->Prev is the underlying object as opened by the next-lower-level driver.  It is the duty of this driver to parse the content of that object and do something meaningful with it.

    obj->Prev->Flags contains some critical infor- mation about the underlying object.  If it contains the flag OBJ_F_CREATED, then the underlying object was just created by this open() operation.  In that case, this driver is expected to create the node with snNewNode() (see later in this document) as long as obj->Mode contains O_CREAT.

- mask (int)	
    Indicates the security mask to be given to the object if it is being created.  Typically, this will only apply to files and directories.  The values are the same as UNIX chmod() type values.

- systype (pContentType)
    This param indicates the content type of the node object as determined by the OSML.  The ContentType structure is defined in include/ obj.h, and includes among other things the name of the content type.  For example, for the reporting driver, this type would be "system/report".

- usrtype (char*)
    This param is the requested object type by the user and is normally used when creating a new object, though under some circumstances it may change the way the open operates on an existing object.  For example, the reporting driver can change whether it generates HTML report text or plaintext reports based on usrtype being either "text/html" or "text/plain".

- oxt (pObjTrxTree*)
    This param is only used by object drivers that specified a capability of OBJDRV_C_TRANS.  More on this field later.  For non-transaction-aware drivers, this field can be safely ignored.

    Yes, this param *is* a pointer to a pointer. Essentially, a pointer passed by reference.

The Open routine should return its internal structure pointer on success, or NULL on failure.  It is normal to allocate one such structure per Open call, and for the structure to point, among other things, to shared data describing the node object.  Accessing the node object is described later in this document.

It is important to know what kinds of fields normally are placed in the allocated data structure returned by Open.  These fields are all determined by the driver author, but here are a few typical ones that are helpful to have ("inf" is the pointer to the structure here):

| Field      | Type      | Description
| ---------- | --------- | ------------
| inf->Obj   | pObject   | This is a copy of the 'obj' pointer passed to the Open routine.
| inf->Mask  | int       | The 'mask' argument passed to Open.
| inf->Node  | pSnNode   | A pointer to the node object, as returned from snNewNode() or snReadNode(), or if structure files aren't being used as the node content type, a pointer to whatever structure contains information about the node object.

The Close() routine is called with two parameters:

| Param  | Type         | Description
| ------ | ------------ | ------------
| inf_v  | void*        | This param is the pointer that the Open routine returned.  Normally the driver will cast the void* parameter to some other structure pointer to access the object's information.
| oxt    | pObjTrxTree* | The transaction tree pointer.

The Close routine should return 0 on success or -1 on failure.  The os driver must make sure it properly deallocates the memory used by originally opening the object, such as the internal structure returned by open and passed in as inf_v.

Note the semantics of a Close failure - the object should still be closed in whatever way is still meaningful.  The end-user must deal with the situation by reviewing the returned mssError messages.

Before exiting, the Close routine should make sure it decrements the Open Count (node->OpenCnt--).  Before doing so, it should also perform a snWriteNode() to write any modified node information back to the node object.

### C.  Creating and Deleting Objects.
The Create and Delete functions are used for creating and deleting objects.  Normally, the os driver will process the Pathname in the same manner for Create and Delete as for Open, thus such functionality could be placed in another function.  

As a side note, within Centrallix, the standard function naming convention is to use xxx_internal_FunctionName for functions that are more or less internal to the module and not a part of any standard interface.

The Create routine has parameters identical to the Open routine.  It should return 0 on success and -1 on error.

The Delete routine is passed the following parameters:

| Param  | Type          | Description
| ------ | ------------- | ------------
| obj    | pObject       | The Object structure pointer, used in the same way as in Open and Delete.
| oxt    | pObjTrxTree*  | The transaction tree pointer.

Delete should return 0 on success and -1 on failure.

For many objectsystem drivers, the Create function simply calls the driver's internal Open() with O_CREAT and then its internal Close, although some drivers could manage Create differently from Open.

### D.  Reading and Writing Object Content.
Some, but not all, objects will have content.  If the object does or can have content, the driver should handle these functions as is appropriate.  Otherwise, the driver should return a failure code (-1) from these functions.

The Read routine reads content from the object, as if  reading from a file.  The parameters passed are almost identical to those used in the fdRead command in MTASK: 

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | The generic pointer to the structure returned from Open().
| buffer    | char*         | The destination buffer for the data being read in.
| maxcnt    | int           | The maximum number of bytes to read into the buffer.
| flags     | int           | Either 0 or FD_U_SEEK, in which case the user is specifying the seek offset for the read in the 5th argument.  Of course, not all objects will be seekable, and furthermore, some of the objects handled by the driver may have full or limited seek functionality, even though others may not.
| arg       | int           | Extra argument, currently only used to specify an optional seek offset.
| oxt       | pObjTrxTree*  | The transaction tree pointer.

The Write routine is very similar, except that instead of 'maxcnt', the third argument is 'cnt', and specifies how much data is in the buffer waiting to be written.

Each of these routines should return -1 on failure and return the number of bytes read/written on success.  At end of file or on device hangup, 0 should be returned once, and then subsequent calls should return -1.

### E.  Querying for Child Objects.
Many objects will have the capability of having sub-objects beneath them, called child objects.  In such a case, the parent object becomes a directory of sorts, even though the parent object may also have content, something which is somewhat foreign in the standard filesystem world, but is common for web servers, where opening a directory returns the file 'index.html' on many occasions.

To enumerate a parent object's child objects, the query functions are used.  A query may have a specific criteria so that only objects having certain attributes will be listed.  As mentioned earlier in this document, a driver may or may not choose to intelligently handle those criteria.  The driver has the option of always enumerating all child objects via its query functions, and allowing the OSML filter them and only return to the user the objects that match the criteria.  But it also can do the filtering itself or, more typically, pass the filtering on to the source of the data the driver manages, as with a database server.

The query mechanism can also be used to delete a set of child objects, optionally matching a certain criteria.  The QueryDelete method may be left NULL in the ObjDriver structure if the driver does not implement full query support, in which case the OSML will iterate through the query results and delete the objects one by one.

The first main function for handling queries is OpenQuery.  This function is passed three arguments:

- inf_v (void*)	The value returned from Open for this object.

- query (pObjQuery)	The query structure setup by the OSML.  It will contain several key fields:

    query->QyText: the text of the criteria (i.e., the WHERE clause, in Centrallix SQL syntax)

    query->Tree: the compiled expression tree, which evaluates to nonzero for true or zero for false as the WHERE clause condition.

    query->SortBy[]: an array of expressions giving the various components of the sorting criteria.

    query->Flags: the driver should set and/or clear the flags OBJ_QY_F_FULLQUERY and OBJ_QY_F_FULLSORT if need be.  The former indicates that the driver is willing to handle the full WHERE clause (the query->Tree).  The latter indicates that the driver is willing to handle the sorting of the data as well (in query->SortBy[]).  If the driver can easily have the sorting/selection done (as when querying an RDBMS), it should set these flags. Otherwise, it should let the OSML take care of the ORDER BY and WHERE conditions.

- oxt (pObjTrxTree*)	The transaction tree pointer.

The OpenQuery function should return a void* value, which will within the driver point to a structure used for managing the query.  This structure will normally have a pointer to the inf_v value returned by Open as well, since inf_v is never passed to QueryFetch, QueryDelete or QueryClose.  OpenQuery should return NULL if the object does not support queries or if some other error condition occurs that will prevent the execution of the query.

Once the query is underway with OpenQuery, the user will either start fetching the results with QueryFetch, or will issue a delete operation with QueryDelete.

The QueryFetch routine should return an inf_v pointer to the child object, or NULL if no more child objects are to be returned by the query.  Some drivers may be able to use their internal Open function to generate the newly opened object, although others will directly allocate the inf_v structure and fill it in based on the current queried child object.  QueryFetch will be passed these parameters:

| Parameter  | Type           | Description
| ---------- | -------------- | ------------
| qy_v       | void*          | The value returned by OpenQuery.
| obj        | pObject        | The newly-created object structure that the OSML is using to track the newly queried child object.
| mode       | int            | The open mode for the new object, as with obj->Mode in Open().
| oxt        | pObjTrxTree*   | The transaction tree pointer.

All object drivers will need to add an element to the obj->Pathname structure to indicate the path to the child object being returned. This will involve a process somewhat like this: (given that new_name is the new object's name, qy is the current query structure, which contains a field 'Parent' that points to the inf_v originally returned by Open, and where the inf_v contains a field Obj that points to the Object structure containing a Pathname structure)

    int cnt;
    pObject obj;
    char* new_name;
    pMyDriversQueryInf qy;

    /** Build the filename. **/
    cnt = snprintf(obj->Pathname->Pathbuf, 256, "%s/%s",
        qy->Parent->Obj->Pathname->Pathbuf,new_name);
    if (cnt < 0 || cnt >= 256) return NULL;
    obj->Pathname->Elements[obj->Pathname->nElements++] = 
        strrchr(obj->Pathname->Pathbuf,'/')+1;

QueryDelete is passed the qy_v void* parameter, and an oxt parameter. It should return 0 on successful deletion, and -1 on failure.

QueryClose is also passed qy_v and oxt.  It should close the query, whether or not QueryFetch has been called enough times to enumerate all of the query results.

### F.  Managing Object Attributes
All objects will have at least some attributes.  Five attributes are mandatory: 'name', 'content_type', 'inner_type', 'outer_type', and 'annotation'.  All compliant drivers must implement these five attributes, all of which have a data type of DATA_T_STRING.

Currently, the OS specification includes support for the following data types:

- DATA_T_INTEGER	-	32-bit signed integer.
- DATA_T_STRING	-	Zero-terminated ASCII string.
- DATA_T_DOUBLE	-	Double-precision floating point.
- DATA_T_DATETIME	-	date/time structure.
- DATA_T_MONEY	-	money data type.

True/false or on/off attributes should be treated as DATA_T_INTEGER for the time being with values of 0 and 1.

Here is a description of the functionality of the five mandatory attributes:

| Attribute      | Description
| -------------- | ------------
| 'name'         | This attribute indicates the name of the object, just as it should appear in any directory listing.  The name of the object must be unique for the directory it is in.
| 'content_type' | This is the type of the object's content, given as a MIME-type.
| 'annotation'   | This is an annotation for the object.  While users may not assign annotations to all objects, each object should be able to have an annotation.  Normally the annotation is a short description of what the object is.  For the Sybase driver, annotations for rows are created by assigning an 'expression' to the table in question, such as 'first_name + last_name' for a people table.
| 'inner_type'   | An alias for 'content_type'.  Both should be supported.
| 'outer_type'   | This is the type of the object itself (the container).

A sixth attribute is not mandatory, but is useful if the object might have content that could in turn be a node object (be interpreted by another driver).  This attribute is 'last_modification', of type DATA_T_DATETIME, and should indicate when the object's content was last updated or modified.

The first function to be aware of is the GetAttrType function.  This routine takes the inf_v pointer, the name of the attribute in question, and the oxt* pointer.  It should return the DATA_T_xxx value for the data type of the attribute.

Next is the GetAttrValue function, which takes four parameters: the inf_v pointer, the name of the attribute, a void pointer pointing to where the attribute's value will be put, and the oxt* pointer.  The way the value pointer is handled depends on the data type.  For DATA_T_INTEGER types, the value pointer is assumed to be pointing to a 32-bit integer where the integer value can be written.  For DATA_T_ STRING types, the value pointer is assumed to be pointing to an empty pointer location where a pointer to the string can be stored.  For DATA_T_DATETIME types, the value pointer is assumed to be pointing to an empty pointer where a pointer to a date time structure (from obj.h) can be stored.  And for double values, the value pointer points to a double value where the double will be stored.  In this way, integer and double values are returned from GetAttrValue by value, and string or datetime values are returned from GetAttrValue by reference.  Items returned by reference must be guaranteed to be valid until the object is closed, or another GetAttrValue or SetAttrValue call is made.  This function should return -1 on a non-existent attribute, 0 on success, and 1 if the value is NULL or unset.

UPDATE ON GETATTR/SETATTR:  These functions now, instead of taking a void* pointer for the value, take a pObjData pointer, which points to an ObjData structure.  The POD(x) macro can be used to typecast appropriate pointers to a pObjData pointer.  The ObjData structure is a UNION type of structure, allowing easy manipulation of data of various types.  See 'datatypes.h'.  Note that this is binary compatible with the old way of using a typecasted void pointer.

The SetAttrValue function works much the same way as GetAttrValue, just with the information moving in the opposite direction.  The third parameter, void* value, is treated in the same manner.

The GetFirstAttr and GetNextAttr functions each take two parameters, the inf_v pointer and the oxt* pointer, and are used to iterate through the non-mandatory attributes for the object.  GetFirstAttr should return a string naming the first attribute, and GetNextAttr should iterate through subsequent attributes.  When the attributes are exhausted, these functions should return NULL.  The attributes 'name', 'annotation', and 'content_type' should not be returned.  If the object has no other attributes, GetFirstAttr should return NULL.

AddAttr is used to add a new attribute to an existing object.  Not all objects support this, and many will refuse the operation.  The parameters are as follows: void* inf_v, char* attrname, int type, void* value, and pObjTrxTree* oxt.

OpenAttr is used to open an attribute for objRead/objWrite as if it were an object with content.  Not all object drivers will support this; this routine should return an inf_v pointer for the new descriptor, and takes four parameters: void* inf_v, char* attrname, int mode, and pObjTrxTree* oxt.  The mode is used in the same manner as the Open function.

### G.  Managing Object Methods
Objects may optionally have methods associated with them.  Each method is given a unique name within the object, and can take a single string parameter.  Three functions exist for managing methods.

The first two functions, GetFirstMethod and GetNextMethod, work identically to their counterparts dealing with attributes.  The third function, ExecuteMethod, starts a method executing.  This function takes four parameters:  the inf_v pointer, the name of the method, the optional string parameter, and the oxt* pointer.

## III Reading the Node Object
The Node object has content which controls what resource(s) this driver will actually access, so it is important for the driver to access the node object's content.  If the driver's node objects are structure files (which is normally the case when dealing with a remote network resource), then the SN module can make opening the node object much more painless.  It also performs caching automatically to improve performance.

Note that the Node object will technically ALREADY BE OPEN as an object in the objectsystem.  The OSML does that for you.  If your driver will not use the SN/ST modules, then it should read the node object via the normal objRead() function, and write it via objWrite().  Your driver should NEVER objClose() the node object!  The OSML does that for you.

An objectsystem driver will commonly configure itself by reading a text file at the root of its object subtree.  There are two main modules available for making this easier.  

The normal way to manage object parameters is to use a structure file. Structure files are a little more complicated, but allow for arrays of values for a given attribute name, as well as allowing for tree- structured hierarchies of attributes and values.  Structure files are accessed via the stparse and st_node modules.  The stparse module provides access to the individual attributes and groups of attributes, and the st_node module loads and saves the structure file heirarchies as a whole. The st_node module also provides node caching to reduce disk activity and eliminate repeated parsing of one file.

For example, if two sessions open two files, '/test1.rpt' and '/test2.rpt' the st_node (SN) module will cache the internal representations of these node object files, and for successive uses of these node objects, the physical file will not be re-parsed.  The file will be re-parsed if its timestamp changes.

If the underlying object does not support the attribute "last_modification" (assumed to be the timestamp), then SN prints a warning.  In essence, this warning indicates that changes to the underlying object will not trigger the SN module to re-read the structure file defining the node object. Otherwise, the SN module keeps track of the timestamp, and if it changes, the node object is re-read and re-parsed.

The driver's first course of action to obtain node object data is to open the node object with the SN module.  The SN module's functions are listed below:

### pSnNode snReadNode(pObject obj)
This function reads a Structure File from the already-open node object which is passed in the "obj" parameter in the xxxOpen() routine.  The "obj" parameter has an element, obj->Prev, which is a link to the node object as opened by the previous driver in the OSML's chain of drivers for handling this open().  All you need to know to get the parsed node object is the following:

    pSnNode node;

    node = snReadNode(obj->Prev);

The returned node structure is managed by the SN module and need not be nmFree()ed.  The only thing that must be done is that the driver should increment the node structure's link count like this:

    node->OpenCnt++;

When closing an object (and thus releasing a reference to the Node structure), the driver should decrement the link count.

### pSnNode snNewNode(pObject obj, char* content_type)
This function creates a new node object with a given content type. The open link count should be incremented as appropriate, as before with snReadNode().

    pSnNode node;

    node = snNewNode(obj->Prev, "system/structure");

The "system/structure" argument is the type that will be assigned to the newly created node object.  Note that the underlying object must already exist in order for this to create a node object as that object's content.  Normally the OSML does this for you by commanding the previous driver (handling obj->Prev) to create the underlying object in question.

### int snWriteNode(pSnNode node)
This function writes a node's internal representation back out to the node file.  The node's status (node->Status) should be set to SN_NS_DIRTY in order for the write to actually occur.  Otherwise, snWriteNode() does nothing.

### int snDeleteNode(pSnNode node)
This function deletes a node file.  At this point, does not actually delete the file but instead just removes the node's data structures from the internal node cache.

### int snGetSerial(pSnNode node)
This function returns the serial number of the node.  Each time the node is re-read because of modifications to the file or is written via snWriteNode because of modifications to the internal structure, the serial number is increased.  This is a good way for a driver to refresh internal information that it caches should it determine a node object has changed.

The stparse module is used to examine the parsed contents of the node file. A node file using the stparse module (and thus st_node module) has a structure file format; see StructureFile.txt.  The file format is a tree structure with objects, subobjects, and attributes.  The internal parsed representation is a tree, with each tree node being an object in the structure file, and each node having attributes, each of which is also a tree node.  Thus, there are three different node types in the tree representation: the top-level ST_T_STRUCT element, which can contain subgroups and attributes; a mid-level ST_T_SUBGROUP tree node, which has a content type, name, and can contain attributes and other subgroups, and lastly a ST_T_ATTRIB node which contains an attribute name and attribute values, either integer or string, and optional lists of such up to 64 items in length.  To use this module, include the file stparse.h.

The following functions are used to manage a parsed structure file:

### pStructInf stParseMsg(pFile inp_fd, int flags)
This function is internal-use-only and is used by the st_node module to parse a structure file.

### pStructInf stParseMsgGeneric(void* src, int (*read_fn)(), int flags)
This function is also internal-use-only (unless you want to parse the file manually without st_node's help) and is used to parse the structure file when the structure file isn't being read from an MTASK pFile descriptor.  This is always the case, as the structure file data is being read from a pObject pointer.  In such a case, src is the pObject pointer and read_fn is objRead().

### int stGenerateMsg(pFile out_fd, pStructInf info, int flags)
This function, also internal-use only, is used by the st_node module to write a structure file whose internal representation is given in the 'info' parameter.

### int stGenerateMsgGeneric(void* dst, int (*write_fn)(), pStructInf info, int flags)
This function is stParseMsgGeneric's converse.

### pStructInf stCreateStruct(char* name, char* type)
This function creates a new top-level tree item of type ST_T_STRUCT, with a given name and content-type.

### pStructInf stAddAttr(pStructInf inf, char* name)
This function adds a node of type ST_T_ATTRIB to either a ST_T_STRUCT or ST_T_SUBGROUP type of node, with a given name and no values associated with that name (see AddValue, below).  The new attribute tree node is linked under the 'inf' node passed, and is returned.

### pStructInf stAddGroup(pStructInf inf, char* name, char* type)
This function adds a node of type ST_T_SUBGROUP to either a ST_T_SUBGROUP or ST_T_STRUCT tree node, with a given name and content type (content type such as 'report/query').

### int stAddValue(pStructInf inf, char* strval, int intval)
This function adds a value to an attribute, and can be called multiple times on an attribute to add a list of values.  If 'strval' is not null, a string value is added, otherwise an integer value is added.  The string is NOT copied, but is simply pointed-to.  If the string is non-static, and has a lifetime less than the ST_T_ATTRIB tree node, then the following procedure must be used:

    char* ptr;
    char* nptr;
    pStructInf attr_inf;

    attr_inf = stAddAttr(my_parent_inf, "myattr");
    nptr = (char*)malloc(strlen(ptr)+1);
    if (!nptr) go_report_the_error_and_return;
    strcpy(nptr, ptr);
    stAddValue(attr_inf, nptr, 0);
    attr_inf->StrAlloc[0] = 1;

By following this method (making a copy of the string and then setting the StrAlloc value for that string), when the StructInf tree node is freed by the stparse module, the string will auto- matically be freed as well.

### pStructInf stLookup(pStructInf inf, char* name)
This routine examines all sub-tree-nodes, both group and attribute nodes, for a group or attribute with the given name.  If it finds one, it returns a pointer to the sub-node, otherwise NULL.

### int stAttrValue(pStructInf inf, int* intval, char** strval, int nval)
This function returns the value of the given attribute in an ST_T_ATTRIB tree node.  If a string value is being returned, pass a pointer to the string pointer.  If an integer value is being returned, pass a pointer to an integer.  The pointer not being used must be left NULL.  'nval' can normally be 0, but if the attribute has several values, setting nval to 1,2,3, etc., returns the 2nd, 3rd, 4th item, respectively.  This routing returns -1 if the attribute value did not exist or if the wrong type was requested. It also returns -1 if 'inf' was NULL.

It is common practice to use the stLookup and stAttrValue functions together to retrieve values, and search for an attribute StructInf and retrieve its value in one operation:

    pStructInf inf;
    char* ptr;

        if (stAttrValue(stLookup(inf, "myattr"),NULL,&ptr,0) == 0)
        {
        printf("%s is the value\n", ptr);
        }

### int stFreeInf(pStructInf this)
This function is used to free a StructInf tree node.  It will free any sub-nodes first, so if that is not desired, be sure to disconnect them by removing them from the SubInf array and appropriately adjusting the nSubInf counter, and setting the SubInf array position to NULL.  This function also disconnects the tree node from its parent, if any, so if the parent is already free()'d, be sure to set the node's Parent pointer to NULL.  Any strings marked allocated with the StrAlloc flags will be free()'d.

It is also common practice to bypass the stXxx() functions entirely and access the elements of the StructInf structures themselves.  This is not forbidden, and may be done.  See the file stparse.h for a description of the structure.  For example,

    pStructInf inf;
    int i;

    for(i=0;i<inf->nSubInf;i++)
        {
        if (inf->SubInf[i]->Type == ST_T_ATTRIB)
            {
            /** do stuff with attribute... **/
            }
        }

## IV Memory Management in Centrallix
Centrallix has its own memory manager that caches freshly-deallocated blocks of memory in lists according to size so that they can be quickly reallocated.  This memory manager also catches double-freeing of blocks, making debugging of memory problems a little easier.

In addition the memory manager provides statistics on the hit ratio of allocated blocks coming from the lists vs. malloc(), and information on how many blocks of each size/type are allocated out and cached.  This information can be invaluable in tracking down memory leaks.

One caveat is that this memory manager does not provide a realloc() function, so the standard malloc(), free(), and realloc() must be used for blocks of memory that might grow in size.  This memory manager is also perhaps not the best to use for blocks of memory of arbitrary sizes, but rather is best for allocating structures quickly that are of a specific size and belong to specific objects, such as the StructInf structure or the SnNode structure, and others.  In short, use it for structures, but not for strings.

Empirical testing has shown an increase of performance of around 50% or more in programs with the newmalloc module in use.

The following are the functions for the newmalloc module:

### void* nmMalloc(int size)
This function allocates a block of the given 'size'.  It returns NULL if the memory could not be allocated.

### void nmFree(void* ptr, int size)
This function frees the block of memory.  NOTE THAT THE CALLING FUNCTION MUST KNOW THE SIZE OF THE BLOCK.  Getting this wrong is very bad.  For structures, this is trivial, just use sizeof() just like with nmMalloc().

### void nmStats()
Prints out statistics on how well the memory manager is doing.

### void nmRegister(int size, char* name)
Registers a name with a block size.  This allows the memory manager to be intelligent when reporting block allocation counts. The first argument is the size of the block, the second, an intelligent name for that size of block.  A size can have more than one name.  This function is optional and need not be used except when tracking down memory leaks, but can be used freely.

Typically this function is called in a module's Initialize() function on each of the structures the module uses internally.

### void nmDebug()
Prints out a listing of block allocation counts, giving (by size): 1) number of blocks allocated but not yet freed, 2) number of blocks in the cache, 3) total allocations for this block size, and a list of names (from nmRegister()) for that block size.

### void nmDeltas()
Prints a listing of all blocks whose allocation count has changed, and by how much, since the last nmDeltas() call.  This function is VERY USEFUL FOR MEMORY LEAK DETECTIVE WORK.

### void* nmSysMalloc(int size)
Allocates memory without using the block-caching algorithm.  This is roughly equivalent to malloc(), but pointers returned by malloc and this function are not compatible with each other - i.e., you cannot free() something that was nmSysMalloc'ed, nor can you nmSysFree() something that was malloc'ed.

This function is much better to use on variable-sized blocks of memory.  nmMalloc is better for fixed-size blocks, such as for data structures.

### void nmSysFree(void* ptr)
Frees a block of memory allocated by nmSysMalloc, nmSysStrdup, or nmSysRealloc.

### void* nmSysRealloc(void* ptr, int newsize)
Changes the size of an allocated block of memory that was obtained via nmSysMalloc or nmSysRealloc or nmSysStrdup.  The new pointer may be different if the block had to be moved.  This is the rough equivalent of realloc().  Usage Note:  If you are realloc'ing a block of memory, and need to store pointers to data somewhere inside the block, it is often better to store the offset rather than a full pointer, as a pointer would become invalid if a nmSysRealloc caused the block to move.

### char* nmSysStrdup(const char* str)
Allocates memory for a copy of the string str by using the nmSysMalloc function, and then makes a copy of the string str. It is a rough equivalent of strdup().  The resulting pointer can be free'd using nmSysFree().

Calling free() on a block obtained from nmMalloc() or calling nmFree() on a block obtained from malloc() will not crash the program.  Instead, it will result in either inefficient use of the memory manager, or a huge memory leak, respectively.  These practices will also render the statistics and block count mechanisms useless.

## V Other Utility Modules
There are many other utility modules useful in Centrallix.  These include the xarray module, used for managing growable arrays; the xhash module, used for managing hash tables with no overflow problems and variable-length keys, the xstring module used for managing growable strings; the expression module used for compiling and evaluating expressions; and the mtsession module, used for managing session-level variables and reporting errors.

### A.	XArray (XA) - Arrays
The first is the xarray (XA) module.

#### xaInit(pXArray this, int init_size)
This function initializes an allocated-but-uninitialized xarray. It makes room for 'init_size' items initially, but this is only an optimization.  A typical value for init_size is 16.

#### xaDeInit(pXArray this)
This de-initializes an xarray, but does not free the XArray structure itself.

#### xaAddItem(pXArray this, void* item)
This adds an item to the array.  The item can be a pointer or an integer (but ints will need a typecast on the function call).

#### xaAddItemSorted(pXArray this, void* item, int keyoffset, int keylen)
This adds an item to the xarray, and keeps the array sorted.  The value for sorting is expected to begin at offset 'keyoffset' and continue for 'keylen' bytes.  This only works when pointers are stored in the array, not integers.

#### xaFindItem(pXArray this, void* item)
This returns the offset into the array's items of the given value. An exact match is required.  The array's items are given below:

    XArray xa;
    pStructInf inf;
    int item_id;

    xaInit(&xa, 16);

    [...]

    xaAddItem(&xa, inf);

    [...]

    item_id = xaFindItem(&xa, inf);
    inf == xa.Items[item_id];

#### xaRemoveItem(pXArray this, int index)
This function removes an item from the xarray at the given index.

### B.	XHash (XH) - Hash Tables
The xhash module provides an extensible hashing table interface.  The hash table is a table of linked lists of items, so collisions and overflows are not a problem as in conventional hash tables.

### int xhInit(pXHashTable this, int rows, int keylen)
This initializes a hash table, giving it the given number of rows, and setting the key length.  For variable length keys (null- terminated strings), use a key length of 0 (zero).  The 'rows' should be an odd number, preferably prime, but does not need to be. It SHOULD NOT be a power of 2.  It's value is an optimization depending on how much data you expect to be in the hash table.  If its value is set to 1, the hash search degenerates to a linear array search.  The value should be large enough to comfortably accomodate the elements.  Typical values might be 31 or 255 (though 255 is not prime).

#### int xhDeInit(pXHashTable this)
De-initializes a hash table.

#### int xhAdd(pXHashTable this, char* key, char* data)
Adds an item to the hash table, with a given key value and data pointer.  Both data and key pointers must have a lifetime that exceeds the time that they item is hashed.

#### int xhRemove(pXHashTable this, char* key)
Removes an item with the given key value from the hash table.

#### char* xhLookup(pXHashTable this, char* key)
Returns the data pointer for a given key, or NULL if the item is not found.

#### int xhClear(pXHashTable this, int free_blk)
Clears all items from a hash table.  If free_blk is set to 1, the items are free()'d as they are removed.

### C.	XString (XS) - Strings
The xstring (XS) module is used for managing growable strings.  It is based on a structure containing a small initial string buffer to avoid string allocations for small strings, but with the capability of performing realloc() operations to extend the string space for storing incrementally larger strings.  The interface to this module allows for strings to contain arbitrary data, even null '\0' characters mid-string.  Thus it is useful as an extensible buffer module as well.

#### int xsInit(pXString this)
Initializes an XString structure, to an empty string.

#### int xsDeInit(pXString this)
Deinitializes an XString structure.

#### int xsConcatenate(pXString this, char* text, int len)
Concatenates the string 'text' onto the end of the XString's value. If len is -1, all data up to the null terminater is copied.  If len is set, all data up to length 'len' is copied, including possible '\0' characters.

#### int xsCopy(pXString this, char* text, int len)
Copies the string 'text' into the XString.  Like xsConcatenate, except that the previous string contents are overwritten.

#### char* xsStringEnd(pXString this)
Returns a pointer to the end of the string.  Useful for finding the end of the string without performing:

    pXString xs;

    xs->String + strlen(xs->String)

since the xs module already knows the string length and does not have to search for the null terminator.  Furthermore, since the string can contain nulls, the above statement could produce incorrect results in those situations.

The contents of the XString can be easily referenced via:

    pXString xs;

    printf("This string is %s\n", xs->String);

IMPORTANT NOTE:  Do not store pointers to values within the string while you are still adding text to the end of the string.  If the string ends up realloc()ing, your pointers will be incorrect.  Instead, if data in the middle of the string needs to be pointed to, store offsets from the beginning of the string, not pointers to the string.

For example, this is WRONG:

    pXString xs;
    char* ptr;

    xsInit(&xs);
    xsConcatenate(&xs, "This is the first sentence.  ", -1);
    ptr = xsStringEnd(&xs);
    xsConcatenate(&xs, "This is the second sentence.", -1);
    printf("A pointer to the second sentence is '%s'\n", ptr);

Instead, use pointer aritmetic and do this:

    pXString xs;
    int offset;

    xsInit(&xs);
    xsConcatenate(&xs, "This is the first sentence.  ", -1);
    offset = xsStringEnd(&xs) - xs->String;
    xsConcatenate(&xs, "This is the second sentence.", -1);
    printf("A pointer to the second sentence is '%s'\n",xs->String+offset);


### D.	Expression (EXP) - Expression Trees
The expression (EXP) module is used for compiling, evaluating, reverse- evaluating, and passing parameters to expression strings.  The expression strings are compiled and stored in an expression tree structure.

Expressions can be stand-alone expression trees, or they can take parameter objects.  A parameter object is an open object (from objOpen()) whose values (attributes) are referenced within the expression string.  By using such parameter objects, one expression can be compiled and then evaluated for many different objects with diverse attribute values.

Expression evaluation results in the top-level expression tree node having the final value of the expression, which may be NULL, and may be an integer, string, datetime, money, or double data type.  For example, the final value of

    :myobject:oneattribute == 'yes'

would be integer 1 (true) if the attribute's value is indeed 'yes'.

Reverse expression evaluation takes a given final value and attempts to assign values to the parameter object attributes based on the structure of the expression tree.  It is akin to 'solving for X' in algebraic work, but isn't nearly that 'smart'.  For example, with the previous expression, if the final value was set to 1 (true), then an objSetAttrValue() function would be issued to set myobject's 'oneattribute' to 'yes'.  Trying this with a final value of 0 (false) would result in no assignment to the attribute, since there would be no way of determining the proper value for that attribute (anything other than 'yes' would work).

Reverse evaluation is typically very useful in updateable joins and views.

Here are the basic expression functions:

#### pExpression expCompileExpression(char* text, pParamObjects objlist, int lxflags, int cmpflags)
This function compiles a textual expression into an expression tree.  The 'objlist' lists the parameter objects that are allowed in the expression (see below for param objects maintenance functions).

The 'lxflags' parameter gives a set of lexical analyzer flags for the compilation.  These flags alter the manner in which the input string is tokenized.  A bitmask; possible values are:

| Value            | Description
| ---------------- | ------------
| MLX_F_ICASEK     | automatically convert all keywords (non-quoted strings) to lowercase.
| MLX_F_POUNDCOMM  | allow comment lines that begin with a # sign.
| MLX_F_CCOMM      | allow c-style comments /* */
| MLX_F_CPPCOMM    | allow c-plus-plus comments //
| MLX_F_SEMICOMM   | allow semicolon comments ;this is a comment
| MLX_F_DASHCOMM   | allow double-dash comments --this is a comment
| MLX_F_DASHKW     | keywords can include the dash '-'.  Otherwise, the keyword is treated as two keywords with a minus sign between them.
| MLX_F_FILENAMES  | Treat a non-quoted string beginning with a slash '/' or dot-slash './' as a filename, and allow slashes and dots in the string without quotes needed.
| MLX_F_ICASER     | automatically convert all reserved words to lowercase.  The use of this flag is highly recommended, and in some cases, required.
| MLX_F_ICASE      | same as MLX_F_ICASER | MLX_F_ICASEK.

The 'cmpflags' is a bitmask parameter controlling the compilation of the expression.  It can contain the following values:

| Value               | Description
| ------------------- | ------------
| EXPR_CMP_WATCHLIST  | A list "value,value,value" is expected first in the expression.
| EXPR_CMP_ASCDESC    | Recognize 'asc' and 'desc' following a value as flags to indicate sort order.
| EXPR_CMP_OUTERJOIN  | Recognize the *= and =* syntax as outer joins.

#### expFreeExpression(pExpression this)
Frees an expression tree.

#### int expEvalTree(pExpression this, pParamObjects objlist)
Evaluates an expression against a list of parameter objects.  If the evaluation is successful, returns 0 or 1, otherwise -1.

#### pParamObjects expCreateParamList()
Allocates a new parameter object list, with no parameters.

#### int expFreeParamList(pParamObjects this)
Frees a parameter object list.

#### int expAddParamToList(pParamObjects this, char* name, pObject obj, int flags)
Adds a parameter to the parameter object list.  The 'obj' pointer may be left NULL during the expCompileExpression state of operation but must be set to a value before expEvalTree is called.  Otherwise the attributes that reference that parameter object will result in NULL values in the expression (it's technically not an error). Flags can be EXPR_O_CURRENT if the object is to be marked as the current one, or EXPR_O_PARENT if it is to be marked as the parent object.  Current and Parent objects can be referenced in an expression like this:

    :currentobjattr
    ::parentobjattr

and is thus a shortcut to typing the full object name.

#### int expModifyParam(pParamObjects this, char* name, pObject replace_obj)
This function is used to update a parameter object with a new open pObject returned from objOpen or objQueryFetch.

#### int expRemoveParamFromList(pParamObjects this, char* name)
This function removes a parameter object from the list.

#### int expReverseEvalTree(pExpression tree, pParamObjects objlist)
This function reverse-evaluates a tree.

The results of an expression evaluation can be accessed by examining the
top-level tree node.  The following properties are useful:

| Property           | Description
| ------------------ | ------------
| tree->DataType     | The type of the final value, see 'Managing Object Attributes' above for types.
| tree->Flags        | Contains the bit EXPR_F_NULL if the expression evaluated to NULL.
| tree->Integer      | If DATA_T_INTEGER, this is the integer value.
| tree->String       | If DATA_T_STRING, this is the string value.
| tree->Types.Double | If DATA_T_DOUBLE, this is the double value.
| tree->Types.Date   | If DATA_T_DATETIME, this is the date/time value
| tree->Types.Money  | If DATA_T_MONEY, this is the money value.

There are several other EXP functions used to deal with aggregates and a few other obscure features as well.  Aggregates are mostly handled internally by Centrallix so further explanation should not be necessary here.


### E.	MTSession (MSS) - Basic Session Management
The next utility module to be described here is the mtsession module (MSS). This module is used for session authentication, error reporting, and for storing session-wide variables such as the currently used date format, current username, and current password (for issuing a login request to a remote server).  Care should be taken in the use of Centrallix that its coredump files are NOT in a world-readable location, as the password will be visible in the core file (or just ulimit the core file size to 0).

#### char* mssUserName()
This function returns the current user name.

#### char* mssPassword()
This function returns the password used to login to the Centrallix

#### int mssSetParam(char* paramname, char* param)
This function sets a session parameter.  The parameter MUST be a string value.

#### char* mssGetParam(char* paramname)
Returns the value of a session parameter.  Common ones are:

- dfmt - current date format.
- mfmt - current money format.
- textsize - current max text size from a read of an object's content via objGetAttrValue(obj, "objcontent", POD(&str))

#### int mssError(int clr, char* module, char* message, ...)
Formats and caches an error message for return to the user.  If 'clr' is set to 1, the assumption is that the error was JUST discovered and no other module has had reason to do an mssError on the current problem.  Setting 'clr' to 1 clears all error messages from the current error message list and adds the current message.

'module' is a two-to-five letter abbreviation of the module reporting the error.  Typically it is all upper-case.

'message' is a string for the error message.  As this function will accept a variable-length argument list, the strings '%d' and '%s' can be included in 'message', and will be substituted with the appropriate integer or string arguments, in a similar way to how printf() works.

#### int mssErrorErrno(int clr, char* module, char* message, ...)
Works much the same way as mssError, except checks the current value of 'errno' and includes a description of any error stored there.  Used primarily when a system call was at fault for an error occurring.

Errors that occur inside a session context are normally stored up and not printed until other MSS module routines are called to fetch those errors. Errors occurring outside a session context (such as in Centrallix's network listener) are printed to Centrallix's standard output immediately.

These mssError routines need not be called at every function nesting level when an error happens.  For example, if the expression compiler returns -1 indicating that a compilation error occurred, it probably has set one or more error messages in the error list.  The calling function only needs to provide context information (e.g. _what_ expression failed compilation?) so that the user has enough information to locate the error.  And once the user is told the full context of the expression compilation error, no more information need be returned.

Another example of this is the memory manager, which sets an error message indicating when an nmMalloc() failed.  The user probably does not care what kind of structure failed allocation -- he/she only needs to know that the hardware ran out of resources.  Thus, upon receiving a NULL from nmMalloc, in most cases another mssError need not be issued.

The mssError() routines do not cause the calling function to return.  The function must still clean up after itself and return an appropriate value (like -1 or NULL) to indicate failure.

### F.	OSML Utility Functions
The OSML provides a set of utility functions that make it easier to write
drivers.  Most of them are named obj_internal_XxxYyy or similar.

#### char* obj_internal_PathPart(pPathname path, int start, int length)
The Pathname structure breaks down a pathname into path elements, which are text strings separated by the directory separator '/'. This function takes the given Pathname structure, and returns the number of path elements requested.  For instance, if you have a path:

    /apps/kardia/data/Kardia_DB/p_partner/rows/1

that path would be stored internally in Centrallix as:

    ./apps/kardia/data/Kardia_DB/p_partner/rows/1

To just return "Kardia_DB/p_partner", you could call:

    obj_internal_PathPart(pathstruct, 4, 2);

Note that return values from obj_internal_PathPart are only valid until the next call to PathPart on the given pathname structure.

#### int obj_internal_AddToPath(pPathname path, char* new_element)
This function lengthens the path by one element, adding new_element on to the end of the path.  This function is frequently useful for drivers in the QueryFetch routine where the new child object needs to be appended onto the end of the given path.

This function returns < 0 on failure, or the index of the new element in the path on success.

#### int obj_internal_CopyPath(pPathname dest, pPathname src)
Copies a pathname structure.

#### void obj_internal_FreePathStruct(pPathname path)
Frees a pathname structure.

## VI Network Connection Functionality
Sometimes a driver will need to initiate a network connection.  This can be done via the MTASK module, which provides simple and easy TCP/IP connectivity.  

### pFile netConnectTCP(char* host_name, char* service_name, int flags)
This function connects to a server.  The host name or ascii string for its ip address is in 'host_name'.  The name of the service (from /etc/services) or its numeric representation in a string is the 'service_name'.  Flags can normally be left 0.

### int netCloseTCP(pFile net_filedesc, int linger_msec, int flags)
This function closes a network connection, and optionally waits up to 'linger_msec' milliseconds (1/1000 seconds) for any data written to the connection to make it to the other end before performing the close.  If linger_msec is set to 0, the connection is aborted (reset).  The linger time can be set to 1000 msec or so if no writes were performed on the connection prior to the close.  If a large amount of writes were performed immediately perior to the close, offering to linger for a few more seconds (perhaps 5 or 10, 5000 or 10000 msec), might be a good idea.

### int fdWrite(pFile filedesc, char* buffer, int length, int offset, int flags)
This function writes data to a file descriptor, from a given buffer and length, and to an optional seek offset and with some optional flags.  Flags can be the following:

- FD_U_NOBLOCK - If the write can't be performed immediately, don't perform it at all.
- FD_U_SEEK - The 'offset' value is valid.  Seek to it before writing.  Not valid for network connections.
- FD_U_PACKET - ALL of the data of 'length' in 'buffer' must be written.  Normal write() semantics in UNIX state that not all data has to be written, and the number of bytes actually written is returned.  Setting this flag makes sure all data is really written before returning.

#### int fdRead(pFile filedesc, char* buffer, int maxlen, int offset, int flags)
The complement to the above routine.  Takes the same flags as the above routine, except FD_U_PACKET means that all of 'maxlen' bytes must be read before returning.  This is good for reading a packet that is known to be exactly 'maxlen' bytes long, but which might be broken up into fragments by the network (TCP/IP has a maximum frame transmission size of about 1450 bytes).

## VII Parsing Data
Centrallix provides a lexical analyzer library that can be used for parsing many types of data.  This module, mtlexer (MLX) can either parse data from a pFile descriptor or from a string value.  This lexical analyzer is used by the expression compiler as well.  It is basically a very fancy string tokenizer.

### pLxSession mlxOpenSession(pFile fd, int flags)
This function opens a lexer session from a file source.  See the 'expression' module description previous in this document for more information on the flags.  Some flags of use here but not mentioned in that section are:

| Flag                | Description
| ------------------- | ------------
| MLX_F_EOL           | Return end-of-line as a token.  Otherwise, the end of a line is just considered whitespace.
| MLX_F_EOF           | Return end-of-file as a token.  Otherwise,  if end of file is reached it is an error.
| MLX_F_IFSONLY       | Only return string values separated by tabs, spaces, newlines, and carriage returns.  For example, normally the brace in "this{brace" is a token and that string will result in three tokens, but in IFSONLY mode it is just one token.
| MLX_F_NODISCARD     | This flag indicates to the lexer that the calling function expects to be able to read data normally using fdRead() or another lexer session after the last token is read and the session is closed.  The lexer will then attempt to "unread" bytes that it buffered during the lexical analysis process (it does fdRead() operations in 2k or so chunks).  If this flag is not specified, up to 2k of information after the last token will be discarded and further fdRead()s on the file descriptor will start at an undefined place in the file.
| MLX_F_ALLOWNUL      | Allow NUL characters ('\0') in the input stream.  If this flag is not set, then NUL characters result in an error condition.  This prevents unwary callers from mis-reading a token returned by mlxStringVal if the token contains a NUL.  If ALLOWNUL is turned on, then the caller must ensure that it is safely handling values with NULs.  
| MLX_F_ENFORCEUTF8   | Ensures that all lexed text conforms to UTF-8 encoding standards. If any invalid characters are found, an error token is returned. Must NOT be set at the same time as the MLX_F_ENFORCEASCII flag, described below. 
| MLX_F_ENFORCEASCII  | Ensures that all lexed text conforms to standard 7 bit ascii. If any invalid bytes are found, an error token is returned. Must NOT be set at the same time as the MLX_F_ENFORCEUTF8 flag, described above. 

### pLxSession mlxStringSession(char* str, int flags)
This function opens a lexer session from a text string.  Same as the above function except that the flag MLX_F_NODISCARD makes no sense for the string.

### int mlxCloseSession(pLxSession this)
Closes a lexer session.

### int mlxNextToken(pLxSession this)
Returns the type of the next token in the token stream.  Valid token types are:

| Token                | Meaning                               |
|----------------------|---------------------------------------|
| MLX_TOK_STRING       | String value, as in a "string".       |
| MLX_TOK_INTEGER      | Integer value.                        |
| MLX_TOK_EQUALS       | =                                     |
| MLX_TOK_OPENBRACE    | {                                     |
| MLX_TOK_CLOSEBRACE   | }                                     |
| MLX_TOK_ERROR        | An error has occurred.                |
| MLX_TOK_KEYWORD      | An unquoted string.                   |
| MLX_TOK_COMMA        | ,                                     |
| MLX_TOK_EOL          | End-of-line.                          |
| MLX_TOK_EOF          | End-of-file reached.                  |
| MLX_TOK_COMPARE      | <> != < > >= <= ==                    |
| MLX_TOK_COLON        | :                                     |
| MLX_TOK_OPENPAREN    | (                                     |
| MLX_TOK_CLOSEPAREN   | )                                     |
| MLX_TOK_SLASH        | /                                     |
| MLX_TOK_PERIOD       | .                                     |
| MLX_TOK_PLUS         | +                                     |
| MLX_TOK_ASTERISK     | *                                     |
| MLX_TOK_RESERVEDWD   | Reserved word (special keyword).      |
| MLX_TOK_FILENAME     | Unquoted string starting with / or ./ |
| MLX_TOK_DOUBLE       | Double precision floating point.      |
| MLX_TOK_DOLLAR       | $                                     |
| MLX_TOK_MINUS        | -                                     |

### char* mlxStringVal(pLxSession this, int* alloc)
Gets the string value of the current token.  If 'alloc' is NULL, only the first 255 bytes of the string will be returned, and the rest will be discarded.  If 'alloc' is non-null and set to 0, the routine will set 'alloc' to 1 if it needed to allocate memory for a very long string, otherwise leave it at 0.  If 'alloc' is non- null and set to 1, this routine will ALWAYS allocate memory for the string, whether long or short.

This routine works no matter what the token type, and returns a string representation of the token if not MLX_TOK_STRING.

This routine MAY NOT be called twice for the same token.

Note that if MLX_F_ALLOWNUL is enabled, there is no way to tell from the return value of mlxStringVal() whether a NUL in the returned string is the end-of-string terminator, or whether it existed in the input data stream.  Thus, this function should not be called when MLX_F_ALLOWNUL is being used.  Use mlxCopyToken instead on MLX_TOK_STRING's, as it gives a definitive answer on the token length.  (mlxStringVal can still be used on keywords since those will never contain a NUL, by definition).

### int mlxIntVal(pLxSession this)
Returns the integer value of MLX_TOK_INTEGER tokens, or returns the compare type for MLX_TOK_COMPARE tokens.  The compare type is a bitmask of the following flags:

- MLX_CMP_EQUALS
- MLX_CMP_GREATER
- MLX_CMP_LESS

For MLX_TOK_DOUBLE tokens, returns the whole part.

### double mlxDoubleVal(pLxSession this)
Returns a double precision floating point number for either MLX_TOK_INTEGER or MLX_TOK_DOUBLE values.

### int mlxCopyToken(pLxSession this, char* buffer, int maxlen)
For use instead of mlxStringVal, copies the contents of the current token to a string buffer, up to 'maxlen' characters.  Returns the number of characters copied.  This function can be called multiple times if more data needs to be read from the token.

### int mlxHoldToken(pLxSession this)
Basically causes the next mlxNextToken() to do nothing but return the current token again.  Used for when a routine realizes after mlxNextToken() that it has read one-too-many tokens and needs to 'put a token back'.

### int mlxSetOptions(pLxSession this, int options)
Sets options (MLX_F_xxx) in the middle of a lexer session.  The options that are valid here are MLX_F_ICASE and MLX_F_IFSONLY.

### int mlxUnsetOptions(pLxSession this, int options)
Clears options (see above).

### int mlxSetReservedWords(pLxSession this, char** res_words)
Informs the lexer that a certain list of words are to be returned as MLX_TOK_RESERVEDWD instead of MLX_TOK_KEYWORD.  The list of words should be an array of character strings, with the last string in the list NULL.  mtlexer does not copy this list, so it must be static or have a lifetime greater than that of the lexer session.

### int mlxNoteError(pLxSession this)
Generates an mssError() message of this form:

    MLX:  Error near '<token-string>'

NOTE:  the calling routine may have detected the error long after the actual place where it occurred.  The MLX module just tries to come close :)

### int mlxNotePosition(pLxSession this)
Generates an mssError() message of this form:

    MLX:  Error at line ##

NOTE:  If using a StringSession instead of a pFile session, this may not be accurate, as the string may have come from the middle of a file somewhere.  Use with care.

## VIII Objectsystem Driver Testing
This section contains a list of things that can be done to test an objectsystem driver, to make sure that it is performing all basic operations normally.  We will use the test_obj command line interface for testing here.  For more information on test_obj commands, see the online Centrallix documentation at:  http://www.centrallix.net/docs/docs.php

Testing for memory leaks for each of these items is strongly encouraged, by watching memory utilization using nmDeltas() during repetitive operations (e.g., nmDeltas(), open, close, nmDeltas(), open, close, and then nmDeltas() again).

Testing for more general bugs using the "valgrind" tool is also strongly encouraged, via running these various tests in test_obj while test_obj is running under valgrind.

Magic number checking on data structures is encouraged.  To use magic number checking, determine a magic number value for each of your structures, and code that as a constant #define in your code.  The magic number should be a 32-bit integer, possibly with 0x00 in either the 2nd or 3rd byte of the integer.  Many existing magic number values can be found in the file "magic.h" in centrallix-lib.  The 32-bit integer is placed as the first element of the structure, and set using the macro SETMAGIC(), and then tested using the macros ASSERTMAGIC(), and less commonly, ASSERTNOTMAGIC().  ASSERTMAGIC() should be used any time a pointer to the structure crosses an interface boundary.  It also may be used at the entry to internal methods/functions, or when traversing linked lists of data structures, or when retrieving data structures from an array.

When used in conjunction with nmMalloc() and nmFree(), ASSERTMAGIC also helps to detect the reuse of already-freed memory, since nmFree() tags the first four bytes of the memory block with the constant MGK_FREEMEM. nmFree() also looks for the constant MGK_FREEMEM in the magic number slot to detect already-freed memory (so do not use that same constant for your own magic numbers).

To properly test under Valgrind, centrallix-lib must be compiled with the configure flag --enable-valgrind-integration turned on.  This disables nmMalloc block caching (so that valgrind can properly detect memory leaks and free memory reuse), and it provides better information to valgrind's analyzer regarding MTASK threads.

The term "MUST", as used here, means that the driver will likely cause problems if the functionality is not present.

The term "SHOULD" indicates behavior which is desirable, but may not cause problems if not fully implemented.

The term "MAY" refers to optional, but permissible, behavior.

### A.	Object opening, closing, creation, and deletion

1.  Any object in the driver's subtree, including the node object itself, MUST be able to be opened using objOpen() and then closed using objClose().  Although it does more than just open and close, the "show" command in test_obj can be useful for testing this.

2.  Objects MUST be able to be opened regardless of the location of the node object in the ObjectSystem.  For example, don't just test the driver with the node object in the top level directory of the ObjectSystem - also try it in other subdirectories.

3.  New objects within the driver's subtree SHOULD be able to be created using objOpen with OBJ_O_CREAT, or using objCreate().  The flags OBJ_O_EXCL and OBJ_O_TRUNC should also be supported, where meaningful.

4.  Where possible, OBJ_O_AUTONAME should be supported on object creation.  With this, the name of the object will be set to `*` in the pathname structure, and OBJ_O_CREAT will also be set.  The driver should automatically determine a suitable "name" for the object, and subsequent calls to objGetAttrValue on "name" should return the determined name.  A driver MAY choose to return NULL for "name" until after certain object properties have been set and an objCommit operation performed.  A driver MUST NOT return `*` for the object name unless `*` is truly the name chosen for the object.

5.  A driver SHOULD support deletion of any object in its subtree with the exception of the node object itself.  Deletion may be done directly with objDelete(), or on an already-open object using objDeleteObj().  A driver MAY refuse to delete an object if the object still contains deletable sub-objects.  Some objects in the subtree might inherently not be deletable apart from the parent objects of said objects.  In those cases, deletion should not succeed.

### B.	Object attribute enumeration, getting, and setting.
1.  The driver MUST NOT return system attributes (name, inner_type, and so forth) when enumerating with objGetFirst/NextAttr.

2.  The driver does not need to handle objGetAttrType on the system attributes.  The OSML does this.

3.  The driver SHOULD support the attribute last_modification if at all reasonable.  Not all objects can have this property however.

4.  The driver SHOULD support the attribute "annotation" if reasonable to do so.  Database drivers should have a configurable "row annotation expression" to auto-generate annotations from existing row content, where reasonable.  The driver MAY permit the user to directly set annotation values.  The driver MUST return an empty string ("") for any annotation values that are unavailable.

5.  Drivers MUST coherently name EVERY object.  Names MUST be unique in any given "directory".

6.  Drivers MAY choose to omit some attributes from normal attribute enumeration.

7.  The "show" command in test_obj is a good way to display a list of attributes for an object.

8.  Attribute enumeration, retrieval, and modification MUST work equally well on objects returned by objOpen() and objects returned by objQueryFetch().

9.  If a driver returns an attribute during attribute enumeration, then that attribute MUST return a valid type via objGetAttrType.

10. A driver MUST return -1 and error with a "type mismatch" type of error from objGet/SetAttrValue, if the data type is inappropriate.

11. A driver MAY choose to perform auto-conversion of data types on certain attributes, but SHOULD NOT perform such auto conversion on a widespread wholesale basis.

12. A driver MAY support the DATA_T_CODE attribute data type.

13. Drivers MAY support DATA_T_INTVEC and DATA_T_STRINGVEC.

14. Drivers MAY support objAddAttr and objOpenAttr.

15. Drivers MAY support methods on objects.  Objects without any methods should be indicated by a NULL return value from the method enumeration functions.

16. When returning attribute values, the value MUST remain valid at least until the next call to objGetAttrValue, objSetAttrValue, or objGetAttrType, or until the object is closed, whichever occurs first.  Drivers MUST NOT require the caller to free attribute memory.

17. When objSetAttrValue is used, drivers MUST NOT depend on the referenced value (in the POD) being valid past the end of the call to objSetAttrValue().

### C.	Object querying (for subobjects)

1.  If an object cannot support queries for subobjects, the OpenQuery call SHOULD fail.

2.  If an object can support the existence of subobjects, but has no subobjects, the OpenQuery should succeed, but calls to QueryFetch MUST return NULL.

3.  Objects returned by QueryFetch MUST remain valid even after the query is closed using QueryClose.

4.  Objects returned by QueryFetch MUST also be able to be passed to OpenQuery to check for the existence of further subobjects, though the OpenQuery call is permitted to fail as in (C)(1) above.

5.  Any name returned by objGetAttrValue(name) on a queried subobject MUST be able to be used to open the same object using objOpen().

6.  Drivers which connect to resources which are able to perform sorting and/or selection (filtering) of records or objects SHOULD use the OBJ_QY_F_FULLSORT and OBJ_QY_F_FULLQUERY flags (see previous discussion) as well as pass on the sorting and filtering expressions to the remote resource so that resource can do the filtering and/or sorting.

7.  If the driver's remote resource can filter and/or sort, but can only do so imperfectly (e.g., the resource cannot handle the potential complexity of all sorting/selection expressions, but can handle parts of them), then OBJ_QY_F_FULLSORT and/or OBJ_QY_F_FULL- QUERY MUST NOT be used.  However the remote resource MAY still provide partial sorting and/or selection of data.

8.  Drivers SHOULD NOT use OBJ_QY_F_FULLSORT and OBJ_QY_F_FULLQUERY if there is no advantage to letting the resource perform these operations (usually, however, if the resource provides such functionality, there is advantage to letting the resource perform those operations.  However, the coding burden to provide the filtering and sorting expressions to the resource, and in the correct format for the resource, may be not worth the work).

9.  Testing of query functionality can be done via test_obj's "query", "csv", and "ls" (or "list") commands.  To test for nested querying of objects returned from QueryFetch, a SUBTREE select can be used with the "query" or "csv" commands.

10. Drivers which support full sorting or full querying MUST be able to handle the attribute "name" in the expression tree for the sort or query criteria.  The "name" attribute SHOULD be mapped to an expression which reflects how "name" is constructed for objects, such as changing "name" to "convert(varchar, prikeyfield1) + '|' + convert(varchar, prikeyfield2)" or whatever is appropriate.
