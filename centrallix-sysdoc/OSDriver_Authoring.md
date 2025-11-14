# ObjectSystem Driver Interface

**Author**:  Greg Beeley

**Date**:    January 13, 1999

**Updated**: November 27, 2025

**License**: Copyright (C) 2001-2011 LightSys Technology Services. See LICENSE.txt for more information.

## Table of Contents
- [ObjectSystem Driver Interface](#objectsystem-driver-interface)
  - [Table of Contents](#table-of-contents)
  - [I Introduction](#i-introduction)
  - [II Interface](#ii-interface)
    - [Function: Open](#function-open)
    - [Function: OpenChild()](#function-openchild)
    - [Function: Close()](#function-close)
    - [Function: Create()](#function-create)
    - [Function: Delete()](#function-delete)
    - [Function: DeleteObj()](#function-deleteobj)
    - [Function: Read()](#function-read)
    - [Function: Write()](#function-write)
    - [Function: OpenQuery()](#function-openquery)
    - [Function: QueryDelete()](#function-querydelete)
    - [Function: QueryFetch()](#function-queryfetch)
    - [Function: QueryCreate()](#function-querycreate)
    - [Function: QueryClose()](#function-queryclose)
    - [Function: GetAttrType()](#function-getattrtype)
    - [Function: GetAttrValue()](#function-getattrvalue)
    - [Function: GetFirstAttr()](#function-getfirstattr--getnextattr)
    - [Function: GetNextAttr()](#function-getfirstattr--getnextattr)
    - [Function: SetAttrValue()](#function-setattrvalue)
    - [Function: AddAttr()](#function-addattr)
    - [Function: OpenAttr()](#function-openattr)
    - [Function: GetFirstMethod()](#function-getfirstmethod--getnextmethod)
    - [Function: GetNextMethod()](#function-getfirstmethod--getnextmethod)
    - [Function: ExecuteMethod()](#function-executemethod)
    - [Function: PresentationHints()](#function-presentationhints)
    - [Function: Info()](#function-info)
    - [Function: Commit()](#function-commit)
    - [Function: GetQueryCoverageMask()](#function-getquerycoveragemask)
    - [Function: GetQueryIdentityPath()](#function-getqueryidentitypath)
  - [III Reading the Node Object](#iii-reading-the-node-object)
    - [Module: st_node](#module-st_node)
    - [st_node: snReadNode()](#st_node-snreadnode)
    - [st_node: snNewNode()](#st_node-snnewnode)
    - [st_node: snWriteNode()](#st_node-snwritenode)
    - [st_node: snDelete()](#st_node-sndeletenode)
    - [st_node: snGetSerial()](#st_node-sngetserial)
    - [st_node: snGetLastModification()](#st_node-sngetlastmodification)
    - [Module: stparse](#module-stparse)
    - [stparse: stStructType()](#stparse-ststructtype)
    - [stparse: stLookup()](#stparse-stlookup)
    - [stparse: stAttrValue()](#stparse-stattrvalue)
    - [stparse: stGetExpression()](#stparse-stgetexpression)
    - [stparse: stCreateStruct()](#stparse-stcreatestruct)
    - [stparse: stAddAttr()](#stparse-staddattr)
    - [stparse: stAddGroup()](#stparse-staddgroup)
    - [stparse: stAddValue()](#stparse-staddvalue)
    - [stparse: stFreeInf()](#stparse-stfreeinf)
    - [stparse: Using Fields Directly](#stparse-using-fields-directly)
  - [IV Memory Management in Centrallix](#iv-memory-management-in-centrallix)
    - [nmMalloc()](#nmmalloc)
    - [nmFree()](#nmfree)
    - [nmStats()](#nmstats)
    - [nmRegister()](#nmregister)
    - [nmDebug()](#nmdebug)
    - [nmDeltas()](#nmdeltas)
    - [nmSysMalloc()](#nmsysmalloc)
    - [nmSysRealloc()](#nmsysrealloc)
    - [nmSysStrdup()](#nmsysstrdup)
    - [nmSysFree()](#nmsysfree)
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
An objectsystem driver's purpose is to provide access to a particular type of local or network data/resource.  Specific information about the resource to be accessed (such as credentials for a database, queries for selecting data, the auth token for an API, etc.) is stored in a file that is openned by the relevant driver.  For example, the query driver (defined in `objdrv_query.c`) opens `.qy` files, which store one or more ObjectSQL queries used to fetch data.

When the object system starts up, each driver registers one or more type names that it supports (e.g. `"system/query"` for the query driver).  When a file is openned, the object system uses the file's type name to select which driver to use. It finds this type name with one of two strategies.  If the file has an extension (e.g. `example.qy`), that extension can be mapped to a type name using `types.cfg` (e.g. `.qy` maps to `"system/query"`).  Althernatively, the file may reside in a directory containing a `.type` file which explicitly specifies the type name for all files in that directory without recognizable extensions.

Once a file is openned, the driver should organize provided data into a tree-structured hierarchy, which becomes part of the path used by Centrallix's ObjectSystem.  For example, when opening `example.qy` in the ObjectSystem, the driver makes `/rows` and `/columns` available, allowing for paths such as `/apps/data/example.qy/rows`.  The root of a driver's tree (`example.qy`) is called the driver's "node" object, and most paths traverse the root nodes of multiple drivers.  A driver author is free to define any manner of tree structures for representing data available within their driver.  However, the structure should fit the basic ObjectSystem model of a hierarchy of objects, each having attributes, and optionally some methods and/or content.

A driver can be openned multiple times, leading one driver to have multiple "node" objects, also called instances.  Typically, each "node" object relates to a particular instance of a network resource.  For example, an instance of a POP3 driver might represent a POP3 server on the network.  If the network had multiple POP3 servers, this driver could be used to access each of them through different node objects (e.g. `dev.pop3`, `prod.pop3`, etc.).  However, if somehow the OS driver were able to easily enumerate the various POP3 servers on the network (i.e., they responded to some kind of hypothetical broadcast query), then the OS driver author could also design the driver to list the POP3 servers under a single node for the whole network.

The structure of the subtree beneath the node object is entirely up to the drivers' author to determine; the OSML does not impose any structural restrictions on such subtrees.  Each object within this structure (e.g. `/example.qy`) can have three types of readable data:
- Child objects (e.g. `/rows`) which can have their own data.
- Content, which can be read similar to reading a file.
- Query data, allowing the object to be queried for information.

Thus, parent objects with child objects behave similarly to a directory, although they can still have separate readable data _and_ queryable data. This may seem foreign in the standard file system paradime, however, it is common for web servers, where opening a directory often returns `index.html` file in that directory, or some other form of information to allow further navigation.  Querying an object was originally intended as a way to quickly traversal of its child objects, although queries are not required to be implemented this way.

Below is an example of the Sybase driver's node object and its subtrees of child objects (defined in `objdrv_sybase.c`):

```sh
OMSS_DB (type = "application/sybase")
    |
    +----- JNetHelp (type = "system/table")
    |    |
    |    +----- columns (type = "system/table-columns")
    |    |    |
    |    |    +----- document_id (type = "system/column")
    |    |    |
    |    |    +----- parent_id (type = "system/column")
    |    |    |
    |    |    +----- title (type = "system/column")
    |    |    |
    |    |    +----- content (type = "system/column")
    |    |
    |    +----- rows (type = "system/table-rows")
    |    |
    |    +----- 1 (type = "system/row")
    |    |
    |    +----- 2 (type = "system/row")
    |
    +----- Partner (type = "system/table")
```

(... and so forth)

In this case, the `OMSS_DB` file becomes the driver's node object. This file would contain the information necessary to access the database, such as server name, database name, max connections to pool, and so forth.

OS Drivers support several primary areas of functionality:
- Opening and closing objects.
- Creating and deleting node objects (optional).
- Reading and writing object content (optional).
- Getting and (optionally) setting object attributes.
- Executing object methods (optional).
- Querying data attributes (optional).

Using the example above, we can query from the database using a statement like `select :title from /OMSS_DB/JNetHelp/rows`, which will open a sybase driver instance, then open a query and repeatedly fetch rows, getting the `title` attribute from each row.


## II Interface
This section describes the standard interface between the OSML and the ObjectSystem driver itself.  Every driver should implement certain required functions.  (**Note**: Many drivers "implement" some required functions to simply fail with a not implemented or not supported error.  For example, most database drivers "implement" `Read()` and `Write()` this way because database content should be queried, not read).  Various optional functions are also available, which a driver is not required to implement.

<!-- TODO: Greg
  --- Double check the information in this table. Some of it is missing,
  --- and I had to guess a lot of it by looking at how various drivers were
  --- implemented. Also, which functions are optional and which are required
  --- seems very chaotic and I vaguely remember there being a driver that
  --- does not implement a ton of "required" functions, so please double
  --- check that this information is 100% correct.
  --->
The driver should implement an `Initialize()` function, as well as the following (* indicates required functions):
| Function Name                                             | Description
| --------------------------------------------------------- | ------------
| [Open](#function-open)*                                   | Opens a new driver instance object on a given root node.
| [OpenChild](#function-openchild)                          | ???
| [Close](#function-close)*                                 | Close an open object created by either `Open()` or `QueryFetch()`.
| [Create](#function-create)                                | Create a new driver root node object.
| [Delete](#function-delete)                                | Delete an existing driver root node object.
| [DeleteObj](#function-deleteobj)*                         | ???
| [OpenQuery](#function-openquery)**                        | Start a new query for child objects of a given object.
| [QueryDelete](#function-querydelete)                      | Delete specific objects from a query's result set.
| [QueryFetch](#function-queryfetch)**                      | Open the next child object in the query's result set.
| [QueryCreate](#function-querycreate)                      | ???
| [QueryClose](#function-queryclose)**                      | Close an open query.
| [Read](#function-read)*                                   | Read content from the object.
| [Write](#function-write)*                                 | Write content to the object.
| [GetAttrType](#function-getattrtype)*                     | Get the type of a given object's attribute.
| [GetAttrValue](#function-getattrvalue)*                   | Get the value of a given object's attribute.
| [GetFirstAttr](#function-getfirstattr--getnextattr)*      | Get the name of the object's first attribute.
| [GetNextAttr](#function-getfirstattr--getnextattr)*       | Get the name of the object's next attribute.
| [SetAttrValue](#function-setattrvalue)                    | Set the value of an object's attribute.
| [AddAttr](#function-addattr)                              | Add a new attribute to an object.
| [OpenAttr](#function-openattr)                            | Open an attribute as if it were an object with content.
| [GetFirstMethod](#function-getfirstmethod--getnextmethod) | Get the name of an object's first method.
| [GetNextMethod](#function-getfirstmethod--getnextmethod)  | Get the name of an object's next method.
| [ExecuteMethod](#function-executemethod)                  | Execute a method with a given name and optional parameter string.
| [PresentationHints](#function-presentationhints)          | Get info about an object's attributes.
| [Info](#function-info)*                                   | Get info about an object instance.
| [Commit](#function-commit)                                | Commit changes made to an object.
| [GetQueryCoverageMask](#function-getquerycoveragemask)    | ???
| [GetQueryIdentityPath](#function-getqueryidentitypath)    | ???

_*Function is always required._

_**Function is required to support queries._


---
### Abbreviative Prefix
Each OS Driver will have an abbreviation prefix, such as `qy` for the query driver or `sydb` for the sybase database driver.  This prefix should be prepended to the start of every public function name within the OS driver for consistency and scope management (e.g. `qyInitialize()`, `sydbQueryFetch()`, etc.). Normally, a driver's abbreviation prefix is two to four characters, all lowercase and may be the same as a file extension the driver supports. However, this is not an absolute requirement (see the cluster driver in `objdrv_cluster.c` which supports `.cluster` files using an abbreviation prefix of `cluster`).

This document uses `xxx` to refer to an unspecified abbreviative prefix.

---
### Internal Functions
It is highly likely that driver authors will find shared functionality in the following functions, or wish to abstract out functionality from any of them for a variety of reasons.  When creating additional internal functions in this way, they should be named using the convention of `xxx_internal_FunctionName()`, or possibly `xxxi_FunctionName()` for short.

---
### Function: Initialize
```c
/*** @returns 0 if successful, or
 ***         -1 if an error occurred.
 ***/
int xxxInitialize(void)
```
- ⚠️ **Warning**: Currently, the success/failure of this function is ignored by the caller.
- 📖 **Note**: Unlike other functions defined in the driver, each  driver author must manually add this call to the start up code, found in the `cxDriverInit()` function in `centrallix.c`.

The initialization function is called when the Centrallix starts up, and should register the driver with the OSML and initialize necessary global variables.  It is recommended to place global variables in a single global 'struct' that is named with the driver's prefix in all uppercase.  Global variables should **NOT** be accessed from outside the driver.  Instead, the driver should define functions to access them, allowing it to abstract details away from other drivers.

To register itself with the OSML, the driver should first allocate an ObjDriver structure and initialize its contents:

```c
pObjDriver drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
if (drv == NULL) goto error_handling;
memset(drv, 0, sizeof(ObjDriver));
...
```

To initialize this struct, the driver must:
- Provide a name (in `drv->Name`).
- Provide an array of supported root node types (in `drv->RootContentTypes`).
- Provide capability flags (in `drv->Capabilities`).
- Provide function pointers to implemented functions (see [II Interface](#ii-interface) for a list).

#### Name
The `name` field is a 64 character buffer (allowing names up to 63 characters, with a null terminator). It usually follows the format of the driver abbreviation prefix (in all uppercase), followed by a dash, followed by a descriptive name for the driver.

For example:
```c
if (strcpy(drv->Name, "SYBD - Sybase Database Driver") == NULL) goto error_handling;
```

#### RootContentTypes
The `RootContentTypes` field is an XArray containing a list of strings, representing the type names that the driver can open.  This should only include types the driver will handle as root nodes, not other objects created by the driver.  Thus, the sybase driver would include `"application/sybase"`, but not `"system/table"`.

For example:
```c
if (xaInit(&(drv->RootContentTypes), 2) != 0) goto error_handling;
if (xaAddItem(&(drv->RootContentTypes), "application/sybase") < 0) goto error_handling;
if (xaAddItem(&(drv->RootContentTypes), ""system/query"") < 0) goto error_handling;
```

- 📖 **Note**: To make a specific file extension (like `.qy`) open in a driver, edit `types.cfg` to map that file extension to an available root content type supported by the driver (such as `"system/query"`).

#### Capabilities
The capabilities field is a bitmask which can contain zero or more of the following flags:

- `OBJDRV_C_FULLQUERY`: Indicates that this objectsystem driver will intelligently process the query's expression tree specified in the OpenQuery call, and will only return objects that match that expression.  If this flag is missing, the OSML will filter objects returned by QueryFetch so that the calling user does not get objects that do not match the query. Typically this is set by database server drivers.
  - > **THE ABOVE IS OUT-OF-DATE** (May 16th, 2022): A driver can now determine whether to handle the Where and OrderBy on a per-query basis, by setting values in the ObjQuery structure used when opening a new query.  This allows a because a driver to handle Where and OrderBy for some object listings but not others.

- `OBJDRV_C_TRANS`: Indicates that this objectsystem driver requires transaction management by the OSML's transaction layer (the OXT layer).  OS drivers that require this normally are those that for some reason cannot complete operations in independence from one another.  For example, with a database driver, the creation of a new row object and the setting of its attributes must be done as one operation, although the operation requires several calls from the end user's process.  The OXT allows for the grouping of objectsystem calls so that the os driver does not have to complete them independently, but instead can wait until several calls have been made before actually completing the operation.

#### Registering the Driver Struct
When all values within the structure have been initialized, the driver should call the OSML to register itself, using the `objRegisterDriver()` function:

```c
if (objRegisterDriver(drv) != 0) goto error_handling;
```


---
### Function: Open()
```c
void* xxxOpen(pObject parent, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt);
```

The `Open()` function opens a given file to create a new driver instance. This procedure normally includes the following steps:

1.  Access or create the node object, depending on specified flags and whether or not it already exists.
2.  Parse additional contents of the path after the root node.
3.  Allocate a structure that will represent the open object, including a pointer to the node object.
4.  Perform other opening operations (such as reading database table information, etc., when a db table's row is being accessed).
5.  Return a pointer to the node instance as a void pointer.  This pointer will be passed as `void* inf_v` to the driver in subsequent calls involving this object (except the Query functions, discussed below).

- 📖 **Note - Transactions**: If the os driver specified the `OBJDRV_C_TRANS` capability, it must respect the current state of the user's transaction.  If a new object is being created, an object is being deleted, or other modifications/additions are being performed, and if the OXT layer indicates a transaction is in process, the driver must either complete the current transaction and then complete the current call, or else add the current delete/create/modify call to the transaction tree (in which case the tree item is preallocated; all the driver needs to do is fill it in).  This is handled using the transaction tree parameter (`oxt : pObjTrxTree*`).  The transaction later is discussed in depth in the ??? section.
<!-- TODO: Greg - The transaction layer does not seem to be discussed anywhere. Did I miss it? -->
<!-- TODO: Israel - Add section link above, once we find the correct section for the transaction layer. -->

#### Accessing the Node Object
If `O_CREAT` and `O_EXCL` are both specified in `parent->Mode`, the driver should **only** create a new file and fail if the file already exists (refusing to open and read it).  Otherwise, the driver should read an existing file, or create one if it does not exist and `O_CREAT` is specified, failing if no file can be read or created.

#### Parsing Path Contents
The task of parsing the provided path into the subtree beneath its root node is one of the more complex operations for a driver.  For example, the path to a driver's root node might be `/datasources/OMSS_DB` and the user opens an object called `/datasources/OMSS_DB/JNetHelp/rows/1`.  In this case, the OS driver must parse the meaning of the subtree path `JNetHelp/rows/1`, storing the data targetted by the user into the driver instance to allow later method calls to access the correct data.

#### Parameters
The `Open()` routine is called with five parameters:

- `obj : pObject`: A pointer to the Object structure maintained by the OSML.  This structure includes some useful fields:
    
    - `obj->Mode : int`: A bitmask of the O_* flags, which include: `O_RDONLY` (read only), `O_WRONLY` (write only), `O_RDWR` (read/write), `O_CREAT` (create), `O_TRUNC` (truncate), and `O_EXCL` (exclusive, see above).
    
    - `obj->Pathname : pPathname`: A pointer to a Pathname struct (defined in `include/obj.h`) which contains the complete parsed pathname for the object.  This provides a buffer for the pathname as well as an array of pointers to the pathname's components.  The function `obj_internal_PathPart()` can be used to obtain at will any component or series of components of the pathname.

    - `obj->Pathname->OpenCtl : pStruct[]`: Parameters for the open() operation, as defined by the driver author. These are specified in the path in a similar way to URLs (`example.qy?param1=value&param2=other_value`).  Drivers typically only use `obj->Pathname->OpenCtl[obj->SubPtr]` (see SubPtr below) to retrieve their own parameters, ignoring parameters passed to other drivers in the path.

    - `obj->SubPtr : short`: The number of components in the path that are a part of the path to the root node object, including the `.` for the top level directory.  For example, in the above path of `/data/file.csv`, the path would be internally represented as `./ data/ file.csv`, so SubPtr is 3.

    - `obj->SubCnt : short`: _The driver should set this value_ to show the number of components it controls.  This includes the root node object, so `SubCnt` will always be at least 1.  For example, when opening `/data/file.csv/rows/1`, the CSV driver will read the `SubPtr` of 3 (see above), representing `./ data/ file.csv`. It will then set a `SubCnt` of 3, representing that it will control `file.csv /rows /1`.  (The driver only sets `SubCnt`, `SubPtr` is provided.)

    - `obj->Prev : pObject`: The underlying object as opened by the next-lower-level driver. The file can be accessed and parsed by calling functions and passing this pointer to them (such as the st_parse functions, see below).  **DO NOT attempt to open the file directly with a call like `fopen()`,** as this would require hard coding the path to the root directory of the object system, which *will* break if the code runs on another machine.

    - `obj->Prev->Flags : short`: Contains some useful flags about the underlying object, such as:
        - `OBJ_F_CREATED`: The underlying object was just created by this open() operation.  In that case, this driver is expected to create the node with `snNewNode()` (see later in this document) as long as `obj->Mode` contains `O_CREAT`.
	<!-- TODO: Greg - Should we document any more of the flags? OBJ_F_ROOTNODE? OBJ_F_DELETE? OBJ_F_METAONLY? -->

- `mask : int`: The permission mask to be given to the object, if it is being created.  Typically, this will only apply to files and directories, so most drivers can ignore it.  The values are the same as the UNIX [octal digit permissions](https://en.wikipedia.org/wiki/Chmod#:~:text=Octal%20digit%20permission) used for the `chmod()` command.

- `sys_type : pContentType`: Indicates the content type of the node object as determined by the OSML.  The ContentType structure is defined in `include/obj.h`. `sys_type->Name` lists the name of the content type (e.g. `"system/query"` for the query driver).
<!-- TODO: Greg - What happens if the object doesn't exist yet? Is this NULL? -->

- `usr_type : char*`: The object type requested by the user.  This is normally used when creating a new object, though some drivers also use it when opening an existing object.  For example, the reporting driver generates HTML report text or plaintext reports if `usr_type` is `"text/html"` or `"text/plain"` (respectively).

- `oxt : pObjTrxTree*`: The transaction tree, used when the driver specifies the `OBJDRV_C_TRANS` capability.  More on this field later.  Non-transaction-aware drivers can safely ignore this field.
  
  📖 **Note**: Yes, this param *is* a pointer to a pointer.  Essentially, a pointer passed by reference.


The `Open()` routine should return a pointer to an internal driver structure on success, or `NULL` on failure.  It is normal to allocate one such structure per `Open()` call, and for one of the structure fields to point to shared data describing the node object.  Accessing the node object is described later in this document.

While driver instance structures may vary, some fields are common in most drivers (`inf` is the pointer to the structure here):

| Field      | Type      | Description
| ---------- | --------- | ------------
| inf->Obj   | pObject   | A copy of the `obj` pointer passed to `Open()`.
| inf->Mask  | int       | The `mask` argument passed to `Open()`.
| inf->Node  | pSnNode   | A pointer to the node object. This can come from `snNewNode()` or `snReadNode()` (for structure files), or other node struct information.


---
### Function: OpenChild()
*(Optional)*
```c
void* xxxOpenChild(void* inf_v, pObject obj, char* child_name, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt);
```
**No documentation provided.**

---
### Function: Close()
```c
int xxxClose(void* inf_v, pObjTrxTree* oxt);
```
The close function closes a driver instance, freeing all allocated data and releasing all shared memory such as open connections, files, or other driver instances.  The driver must ensure that all memory allocated by originally opening the object (or allocated by other functions that may be called on an open object) is properly deallocated.  This includes the internal structure returned by `Open()`, or by `QueryFetch()`, which is passed in as `inf_v`.  The driver may also need to decrement the Open Count (`node->OpenCnt--`) if it had to increment this value during `Open()`.  Before doing so, it should also perform a `snWriteNode()` to write any modified node information to the node object.

- 📖 **Note**: Remember that the passed driver instance may originally be from a call to `Open()` or a call to `QueryFetch()`.

- 📖 **Note**: Even if close fails, the object should still be closed in whatever way is possible.  The end-user should deal with the resulting situation by reviewing the `mssError()` messages left by the driver.

- 📖 **Note**: Information may be left unfreed if it is stored in a cache for later use.

The `Close()` routine is called with two parameters:

| Param  | Type         | Description
| ------ | ------------ | ------------
| inf_v  | void*        | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| oxt    | pObjTrxTree* | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

The Close routine should return 0 on success or -1 on failure.  


### Function: Create()
```c
int xxxCreate(pObject obj, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt);
```
The `Create()` function is used to create a new object, and uses the same parameters and return value as `Open()` (documented in detail above).  This often means adding a new file to the file system to represent the object.  Many drivers do not implement this and recommend that driver end-users create files using a standard text editor or programatically using more general means, such as general structure file generation.  If implemented, this function frequently requires very similar path parsing functionality to `Open()`.

- 📖 **Note**: For many drivers, the `Create()` function calls the driver's `Open()` function with `O_CREAT`, then calls its `Close()` function, although some drivers may manage this differently.


### Function: Delete()
```c
int clusterDelete(pObject obj, pObjTrxTree* oxt);
```
The `Delete()` function is used to delete an object, which often means removing a file from the file system.  The Delete routine is passed the following parameters:

| Param  | Type          | Description
| ------ | ------------- | ------------
| obj    | pObject       | The Object structure pointer, used in the same way as in Open and Delete.
| oxt    | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

Delete should return 0 on success and -1 on failure.


### Function: DeleteObj()
```c
int xxxDeleteObj(void* inf_v, pObjTrxTree* oxt);
```
**No documentation provided.**


### Function: Read()
```c
int xxxRead(void* inf_v, char* buffer, int max_cnt, int offset, int flags, pObjTrxTree* oxt);
```
<!-- TODO: Greg - Above is the signature that the cluster driver uses.  However, this conflicts with the documented parameters below (the `arg` parameter is missing).  Can you please resolve this conflict? -->

The `Read()` function reads content from objects that have content, similar to reading content from a file.  If the object does or can have content, the driver should handle these functions as is appropriate.  Otherwise, the driver should return a failure code (-1) and call `mssError()` in these functions.

The parameters passed are intentionally similar to the `fdRead()` function in `mtask.c`: 

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| buffer    | char*         | The buffer where read data should be stored.
| max_cnt   | int           | The maximum number of bytes to read into the buffer.
| flags     | int           | Either `0` or `FD_U_SEEK`. If `FD_U_SEEK` is specified, the caller should specify a seek offset in the 5th argument (`arg`).
| arg       | int           | Extra argument, currently only used to specify the optional seek offset.
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

- 📖 **Note**: Not all objects can be seekable and some of the objects handled by the driver may have limited seek functionality, even if others do not.

Each of these routines should return -1 on failure and return the number of bytes read/written on success.  At end of file or on device hangup, 0 should be returned once, and then subsequent calls should return -1.


### Function: Write()
```c
int xxxWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
```
<!-- TODO: Greg - Update this to match any fixes made to Read(). -->
The `Write()` function is very similar to the `Read()` function above, allowing the caller to write data to objects of supporting drivers with content.  However, the third argument (`max_cnt`) is replaced with `cnt`, specifying the number of bytes of data in the buffer that should be written.


### Function: OpenQuery()
```c
void* xxxOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt);
```
The `OpenQuery()` function opens a new query instance struct for fetching query results from a specific driver instance.  Queries are often used to enumerate an object's child objects, although this is not a requirement.  Queries may include specific criteria, and the driver may decide to intelligently handle them (either manually or, more often, by passing them on to a lower level driver or database) or simply to enumerating all results with its query functions.  In the latter case, the OSML layer will filter results and only return objects that match the criteria to the user.

`OpenQuery()` is passed three parameters:
| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| query     | pObjQuery     | A query structure created by the object system.
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

The `query : pObjQuery` parameter contains several useful fields:
| Parameter       | Type                    | Description
| --------------- | ----------------------- | ------------
| query->QyText   | char*                   | The text specifying the criteria (i.e., the WHERE clause, in Centrallix SQL syntax).
| query->Tree     | void* (pExpression)     | The compiled expression tree. This expression evaluates to a nonzero value for `true` if the where clause is satisfied, or zero for `false` if it is not.
| query->SortBy[] | void*[] (pExpression[]) | An array of expressions giving the various components of the sorting criteria.
| query->Flags    | int                     | The driver should set and/or clear the `OBJ_QY_F_FULLQUERY` and `OBJ_QY_F_FULLSORT` flags, if needed.

The `OBJ_QY_F_FULLQUERY` flag indicates that the driver will handle the full WHERE clause specified in `query->Tree`.

The `OBJ_QY_F_FULLSORT` flag indicates that the driver will handle all sorting for the data specified in `query->SortBy[]`.

If the driver can easily handle sorting/selection (as when querying an database), it should set these flags. Otherwise, it should let the OSML handle the ORDER BY and WHERE conditions to avoid unnecessary work for the driver author.

The `OpenQuery()` function returns a `void*` for the query instance struct, which will be passed to the other query functions (`QueryDelete()`, `QueryFetch()`, and `QueryClose()`).  This structure normally points to the driver instance struct to allow easy access to queried data.  `OpenQuery()` returns `NULL` if the object does not support queries or if an error occurs, in which case `mssError()` should be called before returning.


### Function: QueryDelete()
*(Optional)*
```c
int xxxQueryDelete(void* qy_v, pObjTrxTree* oxt);
```
<!-- TODO: Greg - I got this function signature from the sybase driver, and I'm suspicious that it may be incomplete. -->
Deletes results in the query result set, optionally matching a certain criteria. `QueryDelete()` is passed two parameters:

| Parameter | Type          | Description
| --------- | ------------- | ------------
| qy_v      | void*         | A query instance pointer (returned from `QueryOpen()`).
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

`QueryDelete()` returns 0 to indicate a successful deletion, or -1 to indicate failure, in which case `mssError()` should be called before returning.

If a delete is needed and this method is not implemented, the OSML will iterate through the query results and delete the objects one by one.


### Function: QueryFetch()
```c
void* xxxQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt);
```
The `QueryFetch()` function fetches a driver instance pointer (aka. an `inf_v` pointer) to a child object, or `NULL` if there are no more child objects.  It may be helpful to think of `QueryFetch()` as similar to an alternate form of `Open()`, even if your driver does not implement the functionality to `Open()` every object that can be found with `QueryFetch()`.  In fact, some drivers may use an internal `Open()` function to generate the opened objects.

`QueryFetch()` takes four parameters:

| Parameter  | Type          | Description
| ---------- | ------------- | ------------
| qy_v       | void*         | A query instance struct (returned by `OpenQuery()`).
| obj        | pObject       | An object structure that the OSML uses to track the newly queried child object.
| mode       | int           | The open mode for the new object, the same as `obj->Mode` in `Open()`.
| oxt        | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

The driver should add an element to the `obj->Pathname` structure to indicate the path of the returned child object. This will involve a process somewhat like this, where:
- `new_name : char*` is the new object's name.
- `qy : pMyDriversQueryInf` is the current query structure.
- `qy->Parent->Obj->Pathname : pPathname` points to the affected Pathname struct.

```c
    int count;
    pObject obj;
    char* new_name;
    pMyDriversQueryInf qy;

    /** Build the new filename. **/
    count = snprintf(obj->Pathname->Pathbuf, 256, "%s/%s", qy->Parent->Obj->Pathname->Pathbuf, new_name);
    if (count < 0 || 256 <= count) return NULL;
    obj->Pathname->Elements[obj->Pathname->nElements++] = strrchr(obj->Pathname->Pathbuf, '/') + 1;
```

### Function: QueryCreate()
```c
void* xxxQueryCreate(void* qy_v, pObject new_obj, char* name, int mode, int permission_mask, pObjTrxTree *oxt);
```
<!-- TODO: Greg - I guessed this object signature from multiquery.  However, the variable names appear to be inconsistent with similar functions, and I'm overall not very confident about this. -->
**No documentation provided.**


### Function: QueryClose()
```c
int xxxQueryClose(void* qy_v, pObjTrxTree* oxt);
```
The close function closes a query instance, freeing all allocated data and releasing all shared memory such as open connections, files, or other driver instances.  This function operates very similarly to `Close()`, documented in detail above.  The query should be closed, whether or not `QueryFetch()` has been called enough times to enumerate all of the query results.


### Object Attributes
All objects can have attributes, and there are five required attributes that all drivers must implement (explained below).

Currently, the OS specification includes support for the following data types:

| Name              | Description
| ----------------- | ------------
| `DATA_T_INTEGER`  | 32-bit signed integer.
| `DATA_T_STRING`   | Null-terminated ASCII string.
| `DATA_T_DOUBLE`   | Double-precision floating point number.
| `DATA_T_DATETIME` | Date/time structure.
| `DATA_T_MONEY`    | Money structure.

See `datatypes.h` for more information.

For `true`/`false` or `on`/`off` attributes, use `DATA_T_INTEGER` where 0 indicates `false` and 1 indicates `true`.

The following five attributes are required (all are of type `DATA_T_STRING`):

| Attribute    | Description
| ------------ | ------------
| name         | The name of the object, just as it appears in any directory listing.  The name of the object must always be unique for its directory.
| annotation   | A short description of the object.  While users may not assign annotations to all objects, each object should be able to have an annotation.  For example, in the Sybase driver, annotations for rows are created by assigning an 'expression' to the table in question, such as `first_name + last_name` for a people table.
| content_type | The type of the object's content, given as a MIME-type. Specify `"system/void"` if the object does not have content.
| inner_type   | An alias for 'content_type'.  Both should be supported.
| outer_type   | This is the type of the object itself (the container). Specify `"system/row"` for objects that can be queried.

The `last_modification : DATA_T_DATETIME` attribute is a sixth, optional attribute that may be useful in some situations.  This attribute should indicate the last time that the object's content was modified or updated.

<!-- TODO: Greg - Should I cover some of the other common attributes here? `cx__download_as`? `cx__pathpart`? etc. -->


### Function: GetAttrType()
```c
int xxxGetAttrType(void* inf_v, char* attr_name, pObjTrxTree* oxt);
```
The `GetAttrType()` function returns DATA_T_xxx value for the datatype of the requested. It takes three parameters:

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| attr_name | char*         | The name of the attribute to be queried.
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

This function should return `DATA_T_UNAVAILABLE` if the requested attribute does not exist on the driver instance. It should return -1 to indicate an error, in which case `mssError()` should be called before returning.

For example, calling the following on any driver should return `DATA_T_STRING`.
```c
int datatype = driver->GetAttrType(inf_v, 'name', oxt);
```


### Function: GetAttrValue()
```c
int xxxGetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt);
```
The `GetAttrValue()` function takes four parameters:

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| attr_name | char*         | The name of the attribute to be queried.
| val       | pObjData      | A pointer to a location where the value of the attribute should be stored.
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

The value pointer should be handled in different ways, depending on the type:
- For `DATA_T_INTEGER` types, it is assumed to point to a 32-bit integer where the value should be written.
- For `DATA_T_STRING` types, it is assumed to point to an empty `char*` location where a pointer to a string should be written.
- For `DATA_T_DOUBLE` types, it is assumed to point to a double value where the double should be written.
- For `DATA_T_DATETIME` types, it is assumed to point to an empty `pDateTime` where a pointer to a date time struct (see `obj.h`) should be written.

In this way, integer and double values are returned by value, and string or datetime values are returned by reference.  Items returned by reference are guaranteed to be valid until either the object is closed, or another call to `GetAttrValue()` or `SetAttrValue()` call is made on the same driver (which ever happens first).

This function should return -1 on a non-existent attribute, 0 on success, and 1 if the value is `NULL` or undefined / unset.

- 📖 **Note**: The caller of this function can use the POD(x) macro to typecast appropriate pointers to the pObjData pointer, passed to this function.  The ObjData structure is a UNION type of structure, allowing easy manipulation of data of various types.  See `datatypes.h` for more information.

- 📖 **Note**: In legacy code, a typecasted void* was used instead of a pObjData pointer used today.  This method was binary compatible the current solution because the pObjData is a pointer to a struct union.  See `datatypes.h` for more information.


### Function: SetAttrValue()
```c
int xxxSetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* oxt);
```
The `SetAttrValue()` function is the same as `GetAttrValue()`, however it sets the value by reading it from the `val` parameter instead of getting the value by writing it to the `val` parameter.  The return value is also identical, and `mssError()` should be invoked on failure, or if setting attributes programatically is not implemented.


### Function: GetFirstAttr() & GetNextAttr()
```c
char* xxxGetFirstAttr(void* inf_v, pObjTrxTree* oxt);
char* xxxGetNextAttr(void* inf_v, pObjTrxTree* oxt);
```
These functions return the names of attributes that can be queried on an object.  They both take the same two parameters.

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

These functions should only return the names of significant values, so `name`, `annotation`, etc. should not be returned from these functions, even though they are required to be valid values for any object.  Typically, this is implemented by `GetFirstAttr()` resetting some internal value in the driver `inf_v`, then returning the result of `GetNextAttr()`. `GetNextAttr()` extracts a string from an array or other list of valid attribute names for the object and increments the internal counter.  Once the attributes are exhausted, `GetNextAttr()` returns `NULL` and `GetFirstAttr()` can be used to restart and begin querying elements from the start of the list again.  If an object has no significant attributes, `GetFirstAttr()` and `GetNextAttr()` both return NULL.


### Function: AddAttr()
```c
int clusterAddAttr(void* inf_v, char* attr_name, int type, pObjData val, pObjTrxTree* oxt);
```
The `AddAttr()` function adds a new attribute to an existing object.  Not all objects support this, and many will refuse the operation.  The parameters are the same as those of `GetAttrValue()` and `SetAttrValue()`, documented in detail above.


### Function: OpenAttr()
```c
void* clusterOpenAttr(void* inf_v, char* attr_name, int mode, pObjTrxTree* oxt);
```
The `OpenAttr()` function is used to open an attribute for `objRead()`/`objWrite()` as if it were an object with content.  Not all object drivers will support this, and many will refuse the operation.

This function takes 4 parameters. `inf_v`, `attr_name`, and `oxt` are the same as they are for `GetAttrValue()` and `SetAttrValue()`. `mode` is the same as it is for `Open()`. This function should return an `inf_v` pointer for the new descriptor (similar to `Open()` and `QueryFetch()` above).


### Function: ExecuteMethod()
```c
int clusterExecuteMethod(void* inf_v, char* method_name, pObjData param, pObjTrxTree* oxt);
```
The `ExecuteMethod()` function is used to execute a method on an object.  This feature is rarely used, but some drivers have created methods for actions like dropping their cache or printing debug information.  Each method has a unique name within that object, and can take a single string parameter.

The `ExecuteMethod()` function takes four parameters:

| Parameter   | Type          | Description
| ----------- | ------------- | ------------
| inf_v       | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| method_name | char*         | The name of the method to be executed.
| param       | pObjData      | A pointer to a location where the string value of the param is stored.
| oxt         | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

- 📖 **Note**: The `pObjData` type of the `param` parameter makes it possible that other types of parameters could be supported in the future, however, this is not currently implemented.

The function returns 0 on success, and -1 to indicate an error, in which case `mssError()` should be called before returning.


### Function: GetFirstMethod() & GetNextMethod()
```c
char* xxxGetFirstMethod(void* inf_v, pObjTrxTree* oxt);
char* xxxGetNextMethod(void* inf_v, pObjTrxTree* oxt);
```
These functions work the same as `GetFirstAttr()` and `GetNextAttr()` (respectively), except that they return the method names instead of the attribute names.


### Function: PresentationHints()
```c
pObjPresentationHints xxxPresentationHints(void* inf_v, char* attr_name, pObjTrxTree* oxt);
```
The `PresentationHints()` function allows the caller to request extra information about a specific attribute on a specific driver instance object. Most of this information is intended to be used for displaying the attribute in a user interface, although it can also be useful for general data validation. As such, many drivers may not implement this function.

The `PresentationHints()` function takes three parameters:

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| attr_name | char*         | The name of the requested attribute.
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

The returns a new pObjPresentationHints struct on success, or NULL to indicate an error, in which case `mssError()` should be called before returning.  This struct should be allocated using `nmMalloc()`, and memset to zero, like this:
```c
pObjPresentationHints hints = nmMalloc(sizeof(ObjPresentationHints));
if (hints == NULL) goto error_handling;
memset(hints, 0, sizeof(ObjPresentationHints));
```

The return value, `hints : ObjPresentationHints`, contains the following useful fields which the function should set to give various useful information about the attribute.
- `hints->Constraint : void*`: An expression for determining if a value is valid.
- `hints->DefaultExpr : void*`: An expression defining the default value.
- `hints->MinValue : void*`: An expression defining the minimum valid value.
- `hints->MaxValue : void*`: An expression defining the maximum valid value.
- `hints->EnumList : XArray`: If the attribute is a string enum, this XArray lists the valid string values.
- `hints->EnumQuery : char*`: A query string which enumerates the valid values a string enum attribute.
- `hints->Format : char*`: presentation format - datetime or money  <!-- TODO: Greg - We should add detail here. -->
- `hints->AllowChars : char*`: An array of all valid characters for a string attribute, NULL to allow all characters.
- `hints->BadChars : char*`: An array of all invalid characters for a string attribute.
- `hints->Length : int`: The maximum length of data that can be included in a string attribute.
- `hints->VisualLength : int`: The length that the attribute should be displayed if it is show to the user.
- `hints->VisualLength2 : int`: The number of lines to use in a multi-line edit box for the attribute.
- `hints->BitmaskRO : unsigned int`: which bits, if any, in bitmask are read-only <!-- TODO: Greg - Clarify which bitmask? -->
- `hints->Style : int`: Style flags, documented below.
- `hints->StyleMask : int`: A mask for which style flags were set and which were left unset / undefined.
- `hints->GroupID : int`: Used to assign attributes to groups. Use -1 if the attribute is not in a group.
- `hints->GroupName : char*`: The name of the group to which this attribute belongs, or NULL if it is ungrouped or if the group is named elsewhere.
- `hints->OrderID : int`: Used to specify an attribute order.
- `hints->FriendlyName : char*`: Used to specify a "display name" for an attribute (e.g. `n_rows` might have a friendly name of `"Number of Rows"`). Should be `nmSysMalloc()`ed, often using `nmSysStrdup()`.

- ⚠️ **Warning**: Behavior is undefined if:
  - If a character is included in both `hints->AllowChars` and `hints->BadChars`.
  - The data is longer than length.

The `hints->Style` field can be set with several useful flags. To specify that a flag is not set (e.g. to specify explicitly that a field does allow `NULL`s), set the coresponding bit in the `hints->StyleMask` field while leaving the the bit in the `hints->Style` field set to 0.

The following macros are provided for setting style flags:
- `OBJ_PH_STYLE_BITMASK`: The items in `hints->EnumList` or `hints->EnumQuery` are bitmasked.
- `OBJ_PH_STYLE_LIST`: List-style presentation should be used for the values of an enum attribute.
- `OBJ_PH_STYLE_BUTTONS`: Radio buttons or check boxes should be used for the presentation of enum attribute values.
- `OBJ_PH_STYLE_NOTNULL`: The attribute does not allow `NULL` values.
- `OBJ_PH_STYLE_STRNULL`: An empty string (`""`) should be treated as a `NULL` value.
- `OBJ_PH_STYLE_GROUPED`: The GroupID should be checked and so that fields can be grouped together.
- `OBJ_PH_STYLE_READONLY`: The user is not allowed to modify this attribute.
- `OBJ_PH_STYLE_HIDDEN`: This attribute should be hidden and not presented to the user.
- `OBJ_PH_STYLE_PASSWORD`: Values in this attribute should be hidden, such as for passwords.
- `OBJ_PH_STYLE_MULTILINE`: String values should allow multiline editting.
- `OBJ_PH_STYLE_HIGHLIGHT`: This attribute should be highlighted when presented to the user.
- `OBJ_PH_STYLE_LOWERCASE`: This attribute only allows lowercase characters.
- `OBJ_PH_STYLE_UPPERCASE`: This attribute only allows uppercase characters.
- `OBJ_PH_STYLE_TABPAGE`: Prefer the tab-page layout for grouped fields.
- `OBJ_PH_STYLE_SEPWINDOW`: Prefer separate windows for grouped fields.
- `OBJ_PH_STYLE_ALWAYSDEF`: Always reset the default value when this attribute is modified.
- `OBJ_PH_STYLE_CREATEONLY`: This attribute is writeable only when created, after that it is read only.
- `OBJ_PH_STYLE_MULTISEL`: Multiple select <!-- TODO: Greg - We should add detail here. -->
- `OBJ_PH_STYLE_KEY`: This attribute is a primary key.
- `OBJ_PH_STYLE_APPLYCHG`: Presentation hints should be applied on DataChange instead of on DataModify.


### Function: Info()
```c
int xxxInfo(void* inf_v, pObjectInfo info);
```
The `Info()` function allows the caller to request extra information about a specific driver instance object. It takes two parameters:

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| info      | pObjectInfo   | A driver info struct allocated by the caller which the driver sets with information.

The `pObjectInfo` struct has two fields: `Flags` and `nSubobjects`.  This function should set `info->Flags` to 0 (to ensure no uninitialized noise gets into the data), then & it with all of the following flags that apply to that object.
- `OBJ_INFO_F_CAN_HAVE_SUBOBJ` / `OBJ_INFO_F_CANT_HAVE_SUBOBJ`: Indicates that the object can or cannot have subobjects.
- `OBJ_INFO_F_HAS_SUBOBJ` / `OBJ_INFO_F_NO_SUBOBJ`: Indicates that the object has or does not have subobjects.
- `OBJ_INFO_F_SUBOBJ_CNT_KNOWN`: Indicates that we know the number of subobjects.  If set, the count should be stored in `info->nSubobjects`.
- `OBJ_INFO_F_CAN_HAVE_CONTENT` / `OBJ_INFO_F_CANT_HAVE_CONTENT`: Indicates that the object can or cannot have content (see `Read()` / `Write()`).
- `OBJ_INFO_F_HAS_CONTENT` / `OBJ_INFO_F_NO_CONTENT`: Indicates that this object does or does not have content (see `Read()` / `Write()`).
- `OBJ_INFO_F_CAN_SEEK_FULL`: Seeking is fully supported (both forwards and backwards) on the object.
- `OBJ_INFO_F_CAN_SEEK_REWIND`: Seeking is only supported with an offset of `0`.
- `OBJ_INFO_F_CANT_SEEK`: Seeking is not supported at all.
- `OBJ_INFO_F_CAN_ADD_ATTR` / `OBJ_INFO_F_CANT_ADD_ATTR`: Indicates that the object does or does not allow attributes to be added with the [AddAttr()](#function-addattr) function.
- `OBJ_INFO_F_SUPPORTS_INHERITANCE`: Indicates that the object supports inheritance through attributes such as `cx__inherit`.  See ??? for more information about object inheritance.
<!-- TODO: Greg - Is there supposed to be a section about driver inheritance around here somewhere? Did I miss it? -->
<!-- TODO: Israel - Add link to section about inheritance, once I find it, if it exists. -->
- `OBJ_INFO_F_FORCED_LEAF`: Indicates that the object is forced to be a 'leaf' unless ls__type used.
- `OBJ_INFO_F_TEMPORARY`: Indicates that this is a temporary object without a vaoid pathname.
<!-- The possible typo `vaoid` is intentional, mirroring the wording used in `obj.h`. -->
<!-- TODO: Greg - Is `vaoid` a typo? I copied it from the comment in `obj.h`. -->

The function returns 0 on success, and -1 to indicate an error, in which case `mssError()` should be called before returning.


### Function: Commit()
```c
int xxxCommit(void* inf_v, pObjTrxTree *oxt);
```
**No documentation provided.**


### Function: GetQueryCoverageMask()
```c
int xxxGetQueryCoverageMask(pObjQuery this);
```
**No documentation provided.**


### Function: GetQueryIdentityPath()
```c
int xxxGetQueryIdentityPath(pObjQuery this, char* pathbuf, int maxlen);
```
**No documentation provided.**



## III Reading the Node Object
A driver will commonly configure itself by reading text content from its node object file, at the root of its object subtree.  This content may define what resource(s) a driver should provide, how it should access or compute them, and other similar information.  Most drivers use the structure file format for their node objects because SN module makes parsing, reading, and writing these files easier.  It also performs caching automatically to improve performance.

- 📖 **Note**: The node object will **already be open** as an object in the ObjectSystem: The OSML does this for each driver.  If a driver does not use the SN/ST modules, then it should read and write the node object directly with `objRead()` and `objWrite()`.  A driver should **NEVER** `objClose()` the node object!  The OSML handles that.

Although using the structure file format may be complex, it allows significant flexibility.  Data is structured in hierarchies where each sub-object can have named attributes as well as sub-objects.  Centrallix is filled with examples of this, including any `.qy`, `.app`, `.cmp`, or `.cluster` file.

Structure files are accessed via the st_node (SN) and stparse (SP) modules.  The st_node module loads and saves the structure file heirarchies as a whole.  It also manages caching to reduce disk activity and eliminate repeated parsing of the same file.  The stparse module provides access to the individual attributes and groups of attributes within a node structure file.

For example, if two sessions open two files, `/test1.rpt` and `/test2.rpt` the st_node module will cache the internal representations of these node object files, and for successive uses of these node objects, the physical file will not be re-parsed.  The file will be re-parsed if its timestamp changes.

If the underlying object does not support the attribute "last_modification" (assumed to be the timestamp), then st_node prints a warning.  In essence, this warning indicates that changes to the underlying object will not trigger the st_node module to re-read the structure file defining the node object.  Otherwise, the st_node module keeps track of the timestamp, and if it changes, the node object is re-read and re-parsed.

### Module: st_node
To obtain node object data, the driver should first open the node object with the st_node module.  To use this module, include the file `st_node.h`, which provides the following functions (read `st_node.c` for more functions and additional information):


### st_node: snReadNode()
```c
pSnNode snReadNode(pObject obj);
```
The `snReadNode()` function reads a Structure File from the `obj` parameter, which should be a previously openned object.  In a driver's `Open()` function, this is `obj->Prev` (the node object as opened by the previous driver in the OSML's chain of drivers).

**Usage:**
```c
pSnNode node = snReadNode(obj->Prev);
if (node == NULL) goto error_handling;
```

The returned node structure is managed by the SN module and does not need to be `nmFree()`ed.  Instead, the driver should increment the node structure's link count for as long as it intends to use this structure, using `node->OpenCnt++;`.  When the structure is no longer needed (e.g. when the driver instance is closed), the driver should decrement the link count.


### st_node: snNewNode()
```c
pSnNode snNewNode(pObject obj, char* content_type);
```
The `snNewNode()` function creates a new node object of the given content type. The open link count should be incremented and decremented when appropriate, as with `snReadNode()`.

**Usage:**
```c
pSnNode node = snNewNode(obj->Prev, "system/structure");
if (node == NULL) goto error_handling;
```

In this case, the new structure file will have the type: `"system/structure"`.

- 📖 **Note**: This function only creates node object content, so the underlying object file must already exist.   The OSML should do this for you because the previous driver (`obj->Prev`) creates the underlying object.


### st_node: snWriteNode()
```c
int snWriteNode(pSnNode node);
```
The `snWriteNode()` function writes a node's internal data back out to the node file, if the node's status (`node->Status`) is set to `SN_NS_DIRTY`.  Otherwise, `snWriteNode()` does nothing.


### st_node: snDelete()
```c
int snDelete(pSnNode node);
```
The `snDelete()` function deletes a node by removing the node's data from the internal node cache.

- 📖 **Note**: This does not actually delete the node file.


### st_node: snGetSerial()
```c
int snGetSerial(pSnNode node);
```
The `snGetSerial()` function returns the serial number of the node.

Each time the node is re-read because of modifications to the node file or is written with because `snWriteNode()` was called after modifications to the internal structure, the serial number is increased.  This is a good way for a driver to determine if the node file has changed so it can refresh internal cached data.


### st_node: snGetLastModification()
```c
pDateTime snGetLastModification(pSnNode node);
```
The `snGetLastModification()` function returns the date and time that a file was last modified.  This pointer will remain valid as long as the passed `pSnNode` struct remains valid.  It is managed by the `st_node` module, so the caller should not free the returned pointer.  This function promises not to fail and return `NULL`.


### Module: stparse
The stparse module is used to examine the parsed contents of the node file using the structure file format; see [StructureFile.txt](../centrallix-doc/StructureFile.txt).  This format is a tree structure with node objects that can each have sub-objects and named attributes.  Thus, stparse uses three distinct node types:
- `ST_T_STRUCT`: The top-level node, containing the subtrees and attributes in the file.
- `ST_T_SUBGROUP`: A mid-level type for subobjects within the top-level node. Each subgroup has a content type, name, and may contain attributes and other subgroups.
- `ST_T_ATTRIB`: A bottom-level type for each named attribute. Each attribute has a name and values, either of type integer or string, and optional lists of such up to 64 items in length.

To use this module, include the file `stparse.h`, which includes the following functions (read `stparse.c` for more functions and additional information):


### stparse: stStructType()
```c
int stStructType(pStructInf this);
```
The `stStructType()` function returns the struct type of the past `pStructInf` parameter, which is either `ST_T_ATTRIB` or `ST_T_SUBGROUP` (see above).

- ⚠️ **Warning**: The root node of type `ST_T_STRUCT` will return `ST_T_SUBGROUP` from this function.  If you wish to avoid this, read `inf->Type` (see [stparse: Using Fields Directly](#stparse-using-fields-directly) for more info).  It is unclear whether this behavior is a bug or a feature.  I've decided to call it a feature! ;)


### stparse: stLookup()
```c
pStructInf stLookup(pStructInf inf, char* name);
```
The `stLookup()` function searches all sub-tree nodes for a group or attribute of the given name and returns a pointer to it or returns `NULL` if no group or attribute was found.


### stparse: stAttrValue()
```c
int stAttrValue(pStructInf inf, int* intval, char** strval, int nval);
```
This function gets the value of the given attribute in an `ST_T_ATTRIB` node.  If the value is an integer, the caller should pass a pointer to an integer where it can be stored.  If the value is a string, the caller should pass a pointer to string (aka. a `char*`) where char* for the string can be stored.  The unused alternate pointer must be left `NULL`.  `nval` can normally be 0, but if the attribute has several values, setting nval to 1, 2, 3, etc., returns the 2nd, 3rd, 4th item, respectively.

This function returns -1 if the attribute value did not exist, if the wrong type was requested, or if 'inf' was `NULL`.

It is common practice to use `stLookup()` and `stAttrValue()` or `stGetExpression()` (see below) together to retrieve values, for example (where `inf` is a `pStructInfo` variable from somewhere):

```c
char* ptr;
if (stAttrValue(stLookup(inf, "my_attr"), NULL, &ptr, 0) != 0)
    goto error_handling;
printf("The value is: %s\n", ptr);
```


### stparse: stGetExpression()
```c
pExpression stGetExpression(pStructInf this, int nval);
```
Returns a pointer to an expression that represents the value of the nval-th element of the given struct.


### stparse: stCreateStruct()
```c
pStructInf stCreateStruct(char* name, char* type);
```
This function creates a new top-level tree item of type `ST_T_STRUCT`, with a given name and content-type.


### stparse: stAddAttr()
```c
pStructInf stAddAttr(pStructInf inf, char* name);
```
This function adds a node of type `ST_T_ATTRIB` to either an `ST_T_STRUCT` or an `ST_T_SUBGROUP` type of node, with a given name and no values (see AddValue, below).  The new attribute tree node is linked under the `inf` node passed, and is returned.


### stparse: stAddGroup()
```c
pStructInf stAddGroup(pStructInf inf, char* name, char* type);
```
This function adds a node of type `ST_T_SUBGROUP` to either an `ST_T_SUBGROUP` or an `ST_T_STRUCT` tree node, with a given name and content type (content type such as `"report/query"`).


### stparse: stAddValue()
```c
int stAddValue(pStructInf inf, char* strval, int intval);
```
This function adds a value to an attribute, and can be called multiple times on an attribute to add a list of values.  If `strval` is not null, a string value is added, otherwise an integer value is added.  The string is NOT copied, but is simply pointed-to.  If the string is non-static, and has a lifetime less than the `ST_T_ATTRIB` tree node, then the following procedure should be used, where `str` is the string pointer to the string:

```c
pStructInf attr_inf = stAddAttr(my_parent_inf, "my_attr");
if (attr_inf == NULL) goto error_handling;

char* new_str = (char*)malloc(strlen(str) + 1lu);
if (new_str == NULL) goto error_handling;
strcpy(new_str, str);
stAddValue(attr_inf, new_str, 0);
attr_inf->StrAlloc[0] = 1;
```

With this method (making a copy of the string and then setting the StrAlloc value for that string), the string is automatically freed when the StructInf tree node is freed by the stparse module.


### stparse: stFreeInf()
```c
int stFreeInf(pStructInf this);
```
This function is used to free a `StructInf` tree node.  This also recursively frees sub-tree nodes, so these should be disconnected before calling if they are still needed.  To do this, remove them from the SubInf array by appropriately adjusting the nSubInf counter and setting the SubInf array position to `NULL`.  This function also disconnects the tree node from its parent, if any, so if the parent is already `free()`'d, prevent this behavior by setting the node's Parent pointer to `NULL` before calling this function.  Any strings marked allocated with the StrAlloc flags will also be `free()`'d by this function, so update that flag if necessary.


### stparse: Using Fields Directly
It is also common practice to bypass the stparse functions entirely and access the elements of the `StructInf` struct directly, which is allowed.  (See `stparse.h` for more information about this structure.)

For example (assuming `inf` is a `pStructInfo` variable in scope):
```c
for (unsigned int i = 0u; i < inf->nSubInf; i++)
    {
    switch (inf->SubInf[i]->Type)
	{
	case ST_T_ATTRIB:
	/** Do stuff with attribute... **/
	break;
	
	case ST_T_SUBGROUP:
	/** Do stuff with group... **/
	break;
	
	...
	}
    }
```



## IV Memory Management in Centrallix
<!-- TODO: Greg - It feels like this should be documented somewhere else and linked here because most Centrallix devs will need to know this even if they never write a driver. -->
Centrallix has its own memory management wrapper that caches deallocated blocks of memory by size to allow for faster reuse.  This wrapper also detects double-freeing of blocks (sometimes), making debugging of memory problems just a little bit easier.

In addition, the memory manager provides statistics on the hit ratio of allocated blocks coming from the lists vs. `malloc()`, and on how many blocks of each size/type are `malloc()`ed and cached.  This information can be helpful for tracking down memory leaks.  Empirical testing has shown an increase of performance of around 50% or more in programs with the newmalloc module in use.

One caveat is that this memory manager does not provide `nmRealloc()` function, only `nmMalloc()` and `nmFree()`.  Thus, either `malloc()`, `free()`, and `realloc()` or `nmSysMalloc()`, `nmSysFree()`, and `nmSysRealloc()` should be used for blocks of memory that might vary in size.

- 📖 **Note**: This memory manager is usually the wrong choice for blocks of memory of arbitrary sizes.  It is intended for allocating structures quickly that are of a specific size.  For example, allocated space for a struct that is always the same size.

- 🥱 **tl;dr**: Use `nmMalloc()` for structs, not for strings.

- ⚠️ **Warning**: Calling `free()` on a block obtained from `nmMalloc()` or calling `nmFree()` on a block obtained from `malloc()` might not crash the program immediately.  Instead, it will result in either inefficient use of the memory manager, or a significant memory leak, respectively.  These practices will also lead to incorrect results from the statistics and block count mechanisms.


The following are the functions for the newmalloc module:

### nmMalloc()
```c
void* nmMalloc(int size);
```
This function allocates a block of the given `size`.  It returns `NULL` if the memory could not be allocated.


### nmFree()
```c
void nmFree(void* ptr, int size);
```
This function frees the block of memory.

- ⚠️ **Warning**: The caller **must know the size of the block.**  Getting this wrong is very bad!!  For structures, this is trivial, simply use `sizeof()`, exactly the same as with `nmMalloc()`.


### nmStats()
```c
void nmStats(void);
```
Prints statistics about the memory manager, for debugging and optimizing.

For example:
```
NewMalloc subsystem statistics:
   nmMalloc: 0 calls, 0 hits (-nan%)
   nmFree: 0 calls
   bigblks: 0 too big, 0 largest size
```

<!-- TODO: Greg - In my personal testing, it always printed this same output.  Possibly some flags need to be enabled to get this to work properly? -->


### nmRegister()
```c
void nmRegister(int size, char* name);
```
Registers an inteligent name with a block size.  This allows the memory manager to be intelligent when reporting block allocation counts.  A given size can have more than one name.  This function is optional and not required for any production code to work, but using it can make tracking down memory leaks easier.

This function is usually called in a module's `Initialize()` function on each of the structures the module uses internally.


### nmDebug()
```c
void nmDebug(void);
```
Prints a listing of block allocation counts, giving (by size):
- The number of blocks allocated but not yet freed.
- The number of blocks in the cache.
- The total allocations for this block size.
- A list of names (from `nmRegister()`) for that block size.


### nmDeltas()
```c
void nmDeltas(void);
```
Prints a listing of all blocks whose allocation count has changed, and by how much, since the last `nmDeltas()` call.  This function is VERY USEFUL FOR MEMORY LEAK DETECTIVE WORK.


### nmSysMalloc()
```c
void* nmSysMalloc(int size);
```
Allocates memory without using the block-caching algorithm.  This is roughly equivalent to `malloc()`, but pointers returned by malloc and this function are not compatible with each other - i.e., you cannot `free()` something that was `nmSysMalloc()`'ed, nor can you `nmSysFree()` something that was `malloc()`'ed.

- 📖 **Note**: This function is much better to use on variable-sized blocks of memory.  `nmMalloc()` is better for fixed-size blocks, such as for data structures.


### nmSysRealloc()
```c
void* nmSysRealloc(void* ptr, int newsize);
```
Changes the size of an allocated block of memory that was obtained from `nmSysMalloc()`, `nmSysRealloc()`, or `nmSysStrdup()`.  The new pointer may be different if the block has to be moved.  This is the rough equivalent of `realloc()`.

- 📖 **Note**: If you are `realloc()`'ing a block of memory and need to store pointers to data somewhere inside the block, it is often better to store an offset rather than a full pointer.  This is because a full pointer becomes invalid if a `nmSysRealloc()` causes the block to move.


### nmSysStrdup()
```c
char* nmSysStrdup(const char* str);
```
Allocates memory using `nmSysMalloc()` function and copies the string `str` into this memory.  It is a rough equivalent of `strdup()`.  The resulting pointer can be free'd using `nmSysFree()`.


### nmSysFree()
```c
void nmSysFree(void* ptr);
```
Frees a block of memory allocated by `nmSysMalloc()`, `nmSysRealloc()`, or `nmSysStrdup()`.



## V Other Utility Modules
<!-- TODO: Greg - As I mentioned in passing to you before, this section and the previous section (IV) feel like they should be documented elsewhere and maybe linked here. Also, this section should probably be split into multiple sections (or files) instead of cramming this many functions into a single section. -->
<!-- TODO: Israel - Finish documenting this section after Greg has reviewed the above TODO. -->
The Centrallix library (`centralllix-lib`) has a host of useful utility modules.  These include `xarray`, used for managing growable arrays; `xstring`, used for managing growable strings; `xhash`, used for managing hash tables with no overflow problems and variable-length keys; `expression`, used for compiling and evaluating expressions; and `mtsession`, used for managing session-level variables and reporting errors.


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

```c
    XArray xa;
    pStructInf inf;
    int item_id;

    xaInit(&xa, 16);

    [...]

    xaAddItem(&xa, inf);

    [...]

    item_id = xaFindItem(&xa, inf);
    inf == xa.Items[item_id];
```

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

```c
    pXString xs;

    xs->String + strlen(xs->String)
```

since the xs module already knows the string length and does not have to search for the null terminator.  Furthermore, since the string can contain nulls, the above statement could produce incorrect results in those situations.

The contents of the XString can be easily referenced via:

```c
    pXString xs;

    printf("This string is %s\n", xs->String);
```

IMPORTANT NOTE:  Do not store pointers to values within the string while you are still adding text to the end of the string.  If the string ends up realloc()ing, your pointers will be incorrect.  Instead, if data in the middle of the string needs to be pointed to, store offsets from the beginning of the string, not pointers to the string.

For example, this is WRONG:

```c
    pXString xs;
    char* ptr;

    xsInit(&xs);
    xsConcatenate(&xs, "This is the first sentence.  ", -1);
    ptr = xsStringEnd(&xs);
    xsConcatenate(&xs, "This is the second sentence.", -1);
    printf("A pointer to the second sentence is '%s'\n", ptr);
```

Instead, use pointer aritmetic and do this:

```c
    pXString xs;
    int offset;

    xsInit(&xs);
    xsConcatenate(&xs, "This is the first sentence.  ", -1);
    offset = xsStringEnd(&xs) - xs->String;
    xsConcatenate(&xs, "This is the second sentence.", -1);
    printf("A pointer to the second sentence is '%s'\n",xs->String+offset);
```


### D.	Expression (EXP) - Expression Trees
The expression (EXP) module is used for compiling, evaluating, reverse- evaluating, and passing parameters to expression strings.  The expression strings are compiled and stored in an expression tree structure.

Expressions can be stand-alone expression trees, or they can take parameter objects.  A parameter object is an open object (from objOpen()) whose values (attributes) are referenced within the expression string.  By using such parameter objects, one expression can be compiled and then evaluated for many different objects with diverse attribute values.

Expression evaluation results in the top-level expression tree node having the final value of the expression, which may be NULL, and may be an integer, string, datetime, money, or double data type.  For example, the final value of

```
    :myobject:oneattribute == 'yes'
```

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

```
    :currentobjattr
    ::parentobjattr
```

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

```
    /apps/kardia/data/Kardia_DB/p_partner/rows/1
```

that path would be stored internally in Centrallix as:

```
    ./apps/kardia/data/Kardia_DB/p_partner/rows/1
```

To just return "Kardia_DB/p_partner", you could call:

```
    obj_internal_PathPart(pathstruct, 4, 2);
```

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

- `FD_U_NOBLOCK` - If the write can't be performed immediately, don't perform it at all.
- `FD_U_SEEK` - The 'offset' value is valid.  Seek to it before writing.  Not valid for network connections.
- `FD_U_PACKET` - ALL of the data of 'length' in 'buffer' must be written.  Normal write() semantics in UNIX state that not all data has to be written, and the number of bytes actually written is returned.  Setting this flag makes sure all data is really written before returning.

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
