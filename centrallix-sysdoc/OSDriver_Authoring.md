<!-------------------------------------------------------------------------->
<!-- Centrallix Application Server System                                 -->
<!-- Centrallix Core                                                      -->
<!--                                                                      -->
<!-- Copyright (C) 1998-2012 LightSys Technology Services, Inc.           -->
<!--                                                                      -->
<!-- This program is free software; you can redistribute it and/or modify -->
<!-- it under the terms of the GNU General Public License as published by -->
<!-- the Free Software Foundation; either version 2 of the License, or    -->
<!-- (at your option) any later version.                                  -->
<!--                                                                      -->
<!-- This program is distributed in the hope that it will be useful,      -->
<!-- but WITHOUT ANY WARRANTY; without even the implied warranty of       -->
<!-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        -->
<!-- GNU General Public License for more details.                         -->
<!--                                                                      -->
<!-- You should have received a copy of the GNU General Public License    -->
<!-- along with this program; if not, write to the Free Software          -->
<!-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             -->
<!-- 02111-1307  USA                                                      -->
<!--                                                                      -->
<!-- A copy of the GNU General Public License has been included in this   -->
<!-- distribution in the file "COPYING".                                  -->
<!--                                                                      -->
<!-- File:        OSDriver_Authoring.md                                   -->
<!-- Author:      Greg Beeley                                             -->
<!-- Creation:    January 13th, 1999                                      -->
<!-- Description: Describes useful information and processes for writing  -->
<!--              and designing new Centrallix Object System drivers.     -->
<!--              Rewritten and expanded by Israel Fuller in November and -->
<!--              Descember of 2025.                                      -->
<!-------------------------------------------------------------------------->

# ObjectSystem Driver Interface

**Author**:  Greg Beeley

**Date**:    January 13, 1999

**Updated**: December 11, 2025

**License**: Copyright (C) 2001-2025 LightSys Technology Services. See LICENSE.txt for more information.



## Table of Contents
- [ObjectSystem Driver Interface](#objectsystem-driver-interface)
  - [Table of Contents](#table-of-contents)
  - [I Introduction](#i-introduction)
  - [II Interface](#ii-interface)
    - [Abbreviation Prefix](#abbreviation-prefix)
    - [Internal Functions](#internal-functions)
    - [Function: Initialize()](#function-initialize)
    - [Function: Open()](#function-open)
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
  - [V Module: XArray](#v-module-xarray)
    - [xaNew()](#xanew)
    - [xaFree()](#xafree)
    - [xaInit()](#xainit)
    - [xaDeInit()](#xadeinit)
    - [xaAddItem()](#xaadditem)
    - [xaAddItemSorted()](#xaadditemsorted)
    - [xaAddItemSortedInt32()](#xaadditemsortedint32)
    - [xaGetItem()](#xagetitem)
    - [xaFindItem()](#xafinditem)
    - [xaFindItemR()](#xafinditemr)
    - [xaRemoveItem()](#xaremoveitem)
    - [xaClear()](#xaclear)
    - [xaClearR()](#xaclearr)
    - [xaCount()](#xacount)
    - [xaInsertBefore()](#xainsertbefore)
    - [xaInsertAfter()](#xainsertafter)
  - [VI Module: XHash](#vi-module-xhash)
    - [xhInitialize()](#xhinitialize)
    - [xhInit()](#xhinit)
    - [xhDeInit()](#xhdeinit)
    - [xhAdd()](#xhadd)
    - [xhRemove()](#xhremove)
    - [xhLookup()](#xhlookup)
    - [xhClear()](#xhclear)
    - [xhForEach()](#xhforeach)
    - [xhClearKeySafe()](#xhclearkeysafe)
  - [VII Module: XString](#vii-module-xstring)
    - [xsNew()](#xsnew)
    - [xsFree()](#xsfree)
    - [xsInit()](#xsinit)
    - [xsDeInit()](#xsdeinit)
    - [xsCheckAlloc()](#xscheckalloc)
    - [xsConcatenate()](#xsconcatenate)
    - [xsCopy()](#xscopy)
    - [xsStringEnd()](#xsstringend)
    - [xsConcatPrintf()](#xsconcatprintf)
    - [xsPrintf()](#xsprintf)
    - [xsWrite()](#xswrite)
    - [xsRTrim()](#xsrtrim)
    - [xsLTrim()](#xsltrim)
    - [xsTrim()](#xstrim)
    - [xsFind()](#xsfind)
    - [xsFindRev()](#xsfindrev)
    - [xsSubst()](#xssubst)
    - [xsReplace()](#xsreplace)
    - [xsInsertAfter()](#xsinsertafter)
    - [xsGenPrintf_va()](#xsgenprintf_va)
    - [xsGenPrintf()](#xsgenprintf)
    - [xsString()](#xsstring)
    - [xsLength()](#xslength)
    - [xsQPrintf_va(), xsQPrintf(), & xsConcatQPrintf()](#xsqprintf_va-xsqprintf--xsconcatqprintf)
  - [VIII Module: Expression](#viii-module-expression)
    - [expCompileExpression())](#expallocexpression)
    - [expFreeExpression()](#expfreeexpression)
    - [expCompileExpression()](#expcompileexpression)
    - [expCompileExpressionFromLxs()](#expcompileexpressionfromlxs)
    - [expPodToExpression()](#exppodtoexpression)
    - [expExpressionToPod()](#expexpressiontopod)
    - [expDuplicateExpression()](#expduplicateexpression)
    - [expIsConstant()](#expisconstant)
    - [expEvalTree()](#expevaltree)
    - [expCreateParamList()](#expcreateparamlist)
    - [expFreeParamList()](#expfreeparamlist)
    - [expAddParamToList()](#expaddparamtolist)
    - [expModifyParam()](#expmodifyparam)
    - [expRemoveParamFromList()](#expremoveparamfromlist)
    - [expSetParamFunctions()](#expsetparamfunctions)
    - [expReverseEvalTree()](#expreverseevaltree)
  - [IX MTSession](#ix-module-mtsession)
    - [mssUserName()](#mssusername)
    - [mssPassword()](#msspassword)
    - [mssSetParam()](#msssetparam)
    - [mssGetParam()](#mssgetparam)
    - [mssError()](#msserror)
    - [mssErrorErrno()](#msserrorerrno)
  - [X Path Handling Functions](#x-path-handling-functions)
    - [obj_internal_PathPart()](#obj_internal_pathpart)
    - [obj_internal_AddToPath()](#obj_internal_addtopath)
    - [obj_internal_CopyPath](#obj_internal_copypath)
    - [obj_internal_FreePathStruct()](#obj_internal_freepathstruct)
  - [XI Network Connection Functionality](#vi-network-connection-functionality)
    - [netConnectTCP()](#netconnecttcp)
    - [netCloseTCP()](#netclosetcp)
    - [fdWrite()](#fdwrite)
    - [fdRead()](#fdread)
  - [XII Parsing Data](#xii-parsing-data)
    - [mlxOpenSession()](#mlxopensession)
    - [mlxStringSession()](#mlxstringsession)
    - [mlxCloseSession()](#mlxclosesession)
    - [mlxNextToken()](#mlxnexttoken)
    - [mlxStringVal()](#mlxstringval)
    - [mlxIntVal()](#mlxintval)
    - [mlxDoubleVal()](#mlxdoubleval)
    - [mlxCopyToken()](#mlxcopytoken)
    - [mlxHoldToken()](#mlxholdtoken)
    - [mlxSetOptions()](#mlxsetoptions)
    - [mlxUnsetOptions()](#mlxunsetoptions)
    - [mlxSetReservedWords()](#mlxsetreservedwords)
    - [mlxNoteError()](#mlxnoteerror)
    - [mlxNotePosition()](#mlxnoteposition)
  - [XIII Driver Testing](#xiii-driver-testing)
    - [Object opening, closing, creation, and deletion](#aobject-opening-closing-creation-and-deletion)
    - [Object attribute enumeration, getting, and setting.](#bobject-attribute-enumeration-getting-and-setting)
    - [Object querying (for subobjects)](#cobject-querying-for-subobjects)



## I Introduction
An objectsystem driver's purpose is to provide access to a particular type of local or network data/resource.  Specific information about the resource to be accessed (such as credentials for a database, queries for selecting data, the auth token for an API, etc.) is stored in a file that is opened by the relevant driver.  For example, the query driver (defined in `objdrv_query.c`) opens `.qy` files, which store one or more ObjectSQL queries used to fetch data.

When the object system starts up, each driver registers one or more type names that it supports (e.g. `"system/query"` for the query driver).  When a file is opened, the object system uses the file's type name to select which driver to use. It finds this type name with one of two strategies.  If the file has an extension (e.g. `example.qy`), that extension can be mapped to a type name using `types.cfg` (e.g. `.qy` maps to `"system/query"`).  Althernatively, the file may reside in a directory containing a `.type` file which explicitly specifies the type name for all files in that directory without recognizable extensions.

Once a file is opened, the driver should organize provided data into a tree-structured hierarchy, which becomes part of the path used by Centrallix's ObjectSystem.  For example, when opening `example.qy` in the ObjectSystem, the driver makes `/rows` and `/columns` available, allowing for paths such as `/apps/data/example.qy/rows`.  The root of a driver's tree (`example.qy`) is called the driver's "node" object, and most paths traverse the node objects of multiple drivers.  The root of the entire tree is a special driver called the root node which is used to begin traversal.  Within its tree, a driver author is free to define any manner of hierarchical structures for representing available data.  However, the structure should fit the basic ObjectSystem model of a hierarchy of objects, each having attributes, and optionally some methods and/or content.

A driver can be opened multiple times, leading one driver to have multiple "node" objects, also called instances.  Typically, each "node" object relates to a particular instance of a resource.  For example, say you are designing a driver to access MySQL databases.  You could design the driver file to describe a MySQL instance.  Thus, the node object for this driver could have children for each database in that instance (e.g. `Kardia_DB`, `mysql`, and even the system databases used by MySQL to manage the database internals).  Another design would be for each driver file to describe one MySQL database.  Thus, you could make a `Kardia_DB` file to access that database, and the children of that node object would be each table in the database.  A third design option would be for each driver file to describe a MySQL table.  Thus, you make a `p_partner` file to access members of the partner table, a `p_contact_info` file to access contact info for parterners, etc. with each node object having children for the rows in the table.  This last option would require the developer to create a _lot_ of files (and would probably also make joins hard to implement), so in this case, it's probably not the best.  Ultimately, though, these design choices are up to the driver author.

an instance of a POP3 driver might represent a POP3 server on the network.  If the network had multiple POP3 servers, this driver could be used to access each of them through different node objects (e.g. `dev.pop3`, `prod.pop3`, etc.).  However, if somehow the OS driver were able to easily enumerate the various POP3 servers on the network (i.e., they responded to some kind of hypothetical broadcast query), then the OS driver author could also design the driver to list the POP3 servers under a single node for the whole network.

The structure of the subtree beneath the node object is entirely up to the drivers' author to determine; the OSML does not impose any structural restrictions on such subtrees.  Each object within this structure (e.g. `/example.qy`) can have three types of readable data:
- Child objects (e.g. `/rows`) which can have their own data.
- Content, which can be read similar to reading a file.
- Query data, allowing the object to be queried for information.

Thus, parent objects with child objects behave similarly to a directory, although they can still have separate readable data _and_ queryable data. This may seem foreign in the standard file system paradime, however, it is common for web servers, where opening a directory often returns `index.html` file in that directory, or some other form of information to allow further navigation.  Querying an object was originally intended as a way to quickly traversal of its child objects, although queries are not required to be implemented this way.

Below is an example of the Sybase driver's node object and its subtrees of child objects (defined in `objdrv_sybase.c`):

```sh
Kardia_DB (type = "application/mysql")
    |
    +----- p_partner (type = "system/table")
    |    |
    |    +----- columns (type = "system/table-columns")
    |    |    |
    |    |    +----- p_partner_key (type = "system/column")
    |    |    |
    |    |    +----- p_given_name (type = "system/column")
    |    |    |
    |    |    +----- p_surname (type = "system/column")
    |    |    |
    |    |    ...
    |    |
    |    +----- rows (type = "system/table-rows")
    |    |    |
    |    |    +----- 1 (type = "system/row")
    |    |    |
    |    |    +----- 2 (type = "system/row")
    |    |    |
    |    |    ...
    |    |
    |    ...
    |
    +----- p_contact_info (type = "system/table")
    |      |
    |      ...
    ...
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
| [Open](#function-open)*                                   | Opens a new driver instance object on a given node object.
| [OpenChild](#function-openchild)                          | Opens a single child object of the provided object by name.
| [Close](#function-close)*                                 | Close an open object created by either `Open()` or `QueryFetch()`.
| [Create](#function-create)                                | Create a new driver node object. (Not currently used because the OSML calls the driver Open with the `O_WRONLY \| O_CREAT \| O_EXCL` options instead. See [Open()](#function-open) below for more info.)
| [Delete](#function-delete)                                | Used for general object deletion. Drivers can implement `DeleteObj()` instead.
| [DeleteObj](#function-deleteobj)*                         | Replacement for `Delete()` which operates on an already-open object.
| [OpenQuery](#function-openquery)**                        | Start a new query for child objects of a given object.
| [QueryDelete](#function-querydelete)                      | Delete specific objects from a query's result set.
| [QueryFetch](#function-queryfetch)**                      | Open the next child object in the query's result set.
| [QueryCreate](#function-querycreate)                      | Currently just a stub function that is not fully implemented.
| [QueryClose](#function-queryclose)**                      | Close an open query.
| [Read](#function-read)*                                   | Read content from the object.
| [Write](#function-write)*                                 | Write content to the object.
| [GetAttrType](#function-getattrtype)*                     | Get the type of a given object's attribute.
| [GetAttrValue](#function-getattrvalue)*                   | Get the value of a given object's attribute.
| [GetFirstAttr](#function-getfirstattr--getnextattr)*      | Get the name of the object's first attribute.
| [GetNextAttr](#function-getfirstattr--getnextattr)*       | Get the name of the object's next attribute.
| [SetAttrValue](#function-setattrvalue)*                   | Set the value of an object's attribute.
| [AddAttr](#function-addattr)                              | Add a new attribute to an object.
| [OpenAttr](#function-openattr)                            | Open an attribute as if it were an object with content.
| [GetFirstMethod](#function-getfirstmethod--getnextmethod) | Get the name of an object's first method.
| [GetNextMethod](#function-getfirstmethod--getnextmethod)  | Get the name of an object's next method.
| [ExecuteMethod](#function-executemethod)                  | Execute a method with a given name and optional parameter string.
| [PresentationHints](#function-presentationhints)          | Get info about an object's attributes.
| [Info](#function-info)*                                   | Get info about an object instance.
| [Commit](#function-commit)                                | Commit changes made to an object, ensuring that all modifications in the current transaction are completed and the transaction is closed before returning.
| [GetQueryCoverageMask](#function-getquerycoveragemask)    | Should be left `NULL` outside the MultiQuery module.
| [GetQueryIdentityPath](#function-getqueryidentitypath)    | Should be left `NULL` outside the MultiQuery module.

_*Function is always required._

_**Function is always required, but can always return NULL if queries are not supported._


---
### Abbreviation Prefix
Each OS Driver will have an abbreviation prefix, such as `qy` for the query driver or `sydb` for the sybase database driver.  This prefix should be prepended to the start of every public function name within the OS driver for consistency and scope management (e.g. `qyInitialize()`, `sydbQueryFetch()`, etc.). Normally, a driver's abbreviation prefix is two to four characters, all lowercase and may be the same as a file extension the driver supports. However, this is not an absolute requirement (see the cluster driver in `objdrv_cluster.c` which supports `.cluster` files using an abbreviation prefix of `cluster`).

This document uses `xxx` to refer to an unspecified abbreviation prefix.

- 📖 **Note**: Once an abbreviation prefix has been selected, the driver author should add it to the [Prefixes.md](Prefixes.md) file.


### Internal Functions
It is highly likely that driver authors will find shared functionality in the following functions, or wish to abstract out functionality from any of them for a variety of reasons.  When creating additional internal functions in this way, they should be named using the convention of `xxx_internal_FunctionName()`, or possibly `xxxi_FunctionName()` for short.

---
### Function: Initialize()
```c
/*** @returns 0 if successful, or
 ***         -1 if an error occurred.
 ***/
int xxxInitialize(void)
```
- ⚠️ **Warning**: For compiled drivers, the success/failure of this function is ignored by the caller.  However, for drivers loaded as modules, the return value is checked in order to determine whether to keep the module loaded.  In either case, `mssError()` should be called for any failure (other than memory allocation failures).
- 📖 **Note**: Unlike other functions defined in the driver, each driver author must manually add this call to the start up code, found in the `cxDriverInit()` function in `centrallix.c`.

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
- Provide an array of supported root types (in `drv->RootContentTypes`).
- Provide capability flags (in `drv->Capabilities`).
- Provide function pointers to implemented functions (see [II Interface](#ii-interface) for a list).

#### Name
The `name` field is a 64 character buffer (allowing names up to 63 characters, with a null terminator). It usually follows the format of the driver abbreviation prefix (in all uppercase), followed by a dash, followed by a descriptive name for the driver.

For example:
```c
if (strcpy(drv->Name, "SYBD - Sybase Database Driver") == NULL) goto error_handling;
```

#### RootContentTypes
The `RootContentTypes` field is an XArray containing a list of strings, representing the type names that the driver can open.  This should only include types the driver will open as node objects at the root of its tree, not other objects created by the driver within that tree.  Thus, the sybase driver would include `"application/sybase"`, but not `"system/table"`.

For example:
```c
if (xaInit(&(drv->RootContentTypes), 2) != 0) goto error_handling;
if (xaAddItem(&(drv->RootContentTypes), "application/sybase") < 0) goto error_handling;
if (xaAddItem(&(drv->RootContentTypes), ""system/query"") < 0) goto error_handling;
```

- 📖 **Note**: To make a specific file extension (like `.qy`) open in a driver, edit `types.cfg` to map that file extension to an available root content type supported by the driver (such as `"system/query"`).

#### Capabilities
The capabilities field is a bitmask which can contain zero or more of the following flags:

- `OBJDRV_C_FULLQUERY`: Indicates that this objectsystem driver will intelligently process the query's expression tree specified in the `OpenQuery()` call, and will only return objects that match that expression.  If this flag is missing, the OSML will filter objects returned by `QueryFetch()` so that the calling user does not get objects that do not match the query. Typically this is set by database server drivers.
  - > **THE ABOVE IS OUT-OF-DATE** (May 16th, 2022): A driver can now determine whether to handle the `Where` and `OrderBy` on a per-query basis, by setting values in the ObjQuery structure used when opening a new query.  This allows a driver to handle `Where` and `OrderBy` selectively for some object listings but not others.

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
2.  Parse additional contents of the path after the driver node object.
3.  Allocate a structure that will represent the open object, including a pointer to the node object.
4.  Perform other opening operations (such as reading database table information, etc., when a db table's row is being accessed).
5.  Return a pointer to the node instance as a void pointer.  This pointer will be passed as `void* inf_v` to the driver in subsequent calls involving this object (except the Query functions, discussed below).

- 📖 **Note - Transactions**: If the os driver specified the `OBJDRV_C_TRANS` capability, it must respect the current state of the user's transaction.  If a new object is being created, an object is being deleted, or other modifications/additions are being performed, and if the OXT layer indicates a transaction is in process, the driver must either complete the current transaction and then complete the current call, or else add the current delete/create/modify call to the transaction tree (in which case the tree item is preallocated; all the driver needs to do is fill it in).  This is handled using the transaction tree parameter (`oxt : pObjTrxTree*`).

#### Accessing the Node Object
If `O_CREAT` and `O_EXCL` are both specified in `parent->Mode`, the driver should **only** create a new file and fail if the file already exists (refusing to open and read it).  Otherwise, the driver should read an existing file, or create one if it does not exist and `O_CREAT` is specified, failing if no file can be read or created.

#### Parsing Path Contents
The task of parsing the provided path into the subtree beneath its node object is one of the more complex operations for a driver.  For example, the path to a driver's node object might be `/datasources/Kardia_DB` and the user opens an object called `/datasources/Kardia_DB/p_partner/rows/1`.  In this case, the OS driver must parse the meaning of the subtree path `p_partner/rows/1`, storing the data targetted by the user into the driver instance to allow later method calls to access the correct data.

#### Parameters
The `Open()` routine is called with five parameters:

- `parent : pObject`: A pointer to the Object structure maintained by the OSML.  This structure includes some useful fields:
    
    - `parent->Mode : int`: A bitmask of the OBJ_O_* flags, which include: `OBJ_O_RDONLY` (read only), `OBJ_O_WRONLY` (write only), `OBJ_O_RDWR` (read/write), `OBJ_O_CREAT` (create), `OBJ_O_TRUNC` (truncate), and `OBJ_O_EXCL` (exclusive, see above).
    
    - `parent->Pathname : pPathname`: A pointer to a Pathname struct (defined in `include/obj.h`) which contains the complete parsed pathname for the object.  This provides a buffer for the pathname as well as an array of pointers to the pathname's components.  The function `obj_internal_PathPart()` can be used to obtain at will any component or series of components of the pathname.

    - `parent->Pathname->OpenCtl : pStruct[]`: Parameters for the open() operation, as defined by the driver author. These are specified in the path in a similar way to URLs (`example.qy?param1=value&param2=other_value`).  Drivers typically only use `parent->Pathname->OpenCtl[parent->SubPtr]` (see SubPtr below) to retrieve their own parameters, ignoring parameters passed to other drivers in the path.

    - `parent->SubPtr : short`: The number of components in the path that are a part of the path to the driver's node object, including the `.` for the top level directory and the driver's node object.  For example, in the above path of `/data/file.csv`, the path would be internally represented as `./ data/ file.csv`, so SubPtr is 3.
    
      - For example, use `obj_internal_PathPart(parent->Pathname, parent->SubPtr - 1, 1)` to get the name of the file being openned, and use `obj_internal_PathPart(parent->Pathname, 0, parent->SubPtr)` to get the path.

    - `parent->SubCnt : short`: _The driver should set this value_ to show the number of components it controls.  This includes the driver's node object, so `SubCnt` will always be at least 1.  For example, when opening `/data/file.csv/rows/1`, the CSV driver will read the `SubPtr` of 3 (see above), representing `./ data/ file.csv`. It will then set a `SubCnt` of 3, representing that it controls `file.csv /rows /1`.  (The driver only sets `SubCnt`; `SubPtr` is provided.)

    - `parent->Prev : pObject`: The underlying object as opened by the next-lower-level driver. The file can be accessed and parsed by calling functions and passing this pointer to them (such as the st_parse functions, see below).  **DO NOT attempt to open the file directly with a call like `fopen()`,** as this would require hard coding the path to the root directory of the object system, which *will* break if the code runs on another machine.

    - `parent->Prev->Flags : short`: Contains some useful flags about the underlying object, such as:
        - `OBJ_F_CREATED`: The underlying object was just created by this open() operation.  In that case, this driver is expected to create the node with `snNewNode()` (see later in this document) as long as `parent->Mode` contains `O_CREAT`.

- `mask : int`: The permission mask to be given to the object, if it is being created.  Typically, this will only apply to files and directories, so most drivers can ignore it.  The values are the same as the UNIX [octal digit permissions](https://en.wikipedia.org/wiki/Chmod#:~:text=Octal%20digit%20permission) used for the `chmod()` command.

- `sys_type : pContentType`: Indicates the content type of the node object as determined by the OSML.  The ContentType structure is defined in `include/obj.h`. `sys_type->Name` lists the name of the content type (e.g. `"system/query"` for the query driver).  This is also the type used to select which driver should open the node object, so it will be one of the types registered in the `Initialize()` function.

- `usr_type : char*`: The object type requested by the user.  This is normally used when creating a new object, though some drivers also use it when opening an existing object.  For example, the reporting driver generates HTML report text or plaintext reports if `usr_type` is `"text/html"` or `"text/plain"` (respectively).

- `oxt : pObjTrxTree*`: The transaction tree, used when the driver specifies the `OBJDRV_C_TRANS` capability.  More on this field later.  Non-transaction-aware drivers can safely ignore this field.

  - 📖 **Note**: Yes, this param *is* a pointer to a pointer.  Essentially, a pointer passed by reference.  This allows the driver to create a new transaction tree even if none is in progress.


The `Open()` routine should return a pointer to an internal driver structure on success, or `NULL` on failure.  It is normal to allocate one such structure per `Open()` call, and for one of the structure fields to point to shared data describing the node object.  Accessing the node object is described later in this document.

While driver instance structures may vary, some fields are common in most drivers (`inf` is the pointer to the structure here):

| Field     | Type    | Description                                     |
|-----------|---------|-------------------------------------------------|
| inf->Obj  | pObject | A copy of the `obj` pointer passed to `Open()`. |
| inf->Mask | int     | The `mask` argument passed to `Open()`.         |
| inf->Node | pSnNode | A pointer to the node object.                   |

The driver's node pointer typically comes from `snNewNode()` or `snReadNode()` (for structure files), but it can also be other node struct information.

---
### Function: OpenChild()
*(Optional)*
```c
void* xxxOpenChild(void* inf_v, pObject obj, char* child_name, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt);
```
Opens a single child object of the provided object by name.  Conceptually, this is similar to querying the object for all children where the name attribute equals the passed `child_name` parameter and fetching only the first result.  This function is used to open children of a driver that do not map well into the driver's node object tree.  For example, the query file driver uses this function to allow the caller to open a temporary collection declared in that query file.

The `OpenChild()` function is called with two parameters:

| Param      | Type         | Description                                                               |
|------------|--------------|---------------------------------------------------------------------------|
| inf_v      | void*        | A driver instance pointer (returned from `Open()` or `QueryFetch()`).     |
| obj        | pObject      | An object?                                                                |
| child_name | char*        | The value for the name attribute of the child object to be openned.       |
| mask       | int          | The permission mask to be given to the object (if created).*              |
| sys_type   | pContentType | Indicates the content type of the node object as determined by the OSML.* |
| usr_type   | char*        | The object type requested by the user.*                                   |
| oxt        | pObjTrxTree* | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.         |

<!-- TODO: Greg - I assume the object above is the parent, similar to open... but how is it that different from `inf_v` in this case? -->

*See [`Open()`](#function-open) above for more info.

The `OpenChild()` function should a pointer to the node object for the newly openned child on success or `NULL` on failure.  

---
### Function: Close()
```c
int xxxClose(void* inf_v, pObjTrxTree* oxt);
```
The close function closes a driver instance, freeing all allocated data and releasing all shared memory such as open connections, files, or other driver instances.  The driver must ensure that all memory allocated by originally opening the object (or allocated by other functions that may be called on an open object) is properly deallocated.  This includes the internal structure returned by `Open()`, or by `QueryFetch()`, which is passed in as `inf_v`.  The driver may also need to decrement the Open Count (`node->OpenCnt--`) if it had to increment this value during `Open()`.  Before doing so, it should also perform a `snWriteNode()` to write any modified node information to the node object.

- 📖 **Note**: Remember that the passed driver instance may originally be from a call to `Open()` or a call to `QueryFetch()`.

- 📖 **Note**: Even if close fails, the object should still be closed in whatever way is possible.  The end-user should deal with the resulting situation by reviewing the `mssError()` messages left by the driver.

- 📖 **Note**: Information may be left unfreed if it is stored in a cache for later use.

The `Close()` function is called with two parameters:

| Param | Type         | Description                                                           |
|-------|--------------|-----------------------------------------------------------------------|
| inf_v | void*        | A driver instance pointer (returned from `Open()` or `QueryFetch()`). |
| oxt   | pObjTrxTree* | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.     |

The `Close()` function should return 0 on success or -1 on failure.  


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

| Param | Type         | Description                                                               |
|-------|--------------|---------------------------------------------------------------------------|
| obj   | pObject      | The Object structure pointer, used in the same way as in Open and Delete. |
| oxt   | pObjTrxTree* | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.         |

`Delete()` should return 0 on success and -1 on failure.


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

| Parameter | Type         | Description                                                                                                                  |
|-----------|--------------|------------------------------------------------------------------------------------------------------------------------------|
| inf_v     | void*        | A driver instance pointer (returned from `Open()` or `QueryFetch()`).                                                        |
| buffer    | char*        | The buffer where read data should be stored.                                                                                 |
| max_cnt   | int          | The maximum number of bytes to read into the buffer.                                                                         |
| offset    | int          | An optional seek offset.                                                                                                     |
| flags     | int          | Either `0` or `FD_U_SEEK`. If `FD_U_SEEK` is specified, the caller should specify a seek offset in the 5th argument (`arg`). |
| oxt       | pObjTrxTree* | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.                                                            |

- 📖 **Note**: Not all objects can be seekable and some of the objects handled by the driver may have limited seek functionality, even if others do not.

Each of these routines should return -1 on failure and return the number of bytes read/written on success.  At end of file or on device hangup, 0 should be returned once, and then subsequent calls should return -1.

- 📖 **Note**: There is no separate seek command to help mitigate [Time-of-check to time-of-use attacks](https://en.wikipedia.org/wiki/Time-of-check_to_time-of-use).  To seek without reading data, specify a buffer size of zero.


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

| Parameter | Type         | Description                                                           |
|-----------|--------------|-----------------------------------------------------------------------|
| inf_v     | void*        | A driver instance pointer (returned from `Open()` or `QueryFetch()`). |
| query     | pObjQuery    | A query structure created by the object system.                       |
| oxt       | pObjTrxTree* | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.     |

The `query : pObjQuery` parameter contains several useful fields:
| Parameter       | Type                    | Description
| --------------- | ----------------------- | ------------
| query->QyText   | char*                   | The text specifying the criteria (i.e., the WHERE clause, in Centrallix SQL syntax).
| query->Tree     | void* (pExpression)     | The compiled expression tree. This expression evaluates to a nonzero value for `true` if the where clause is satisfied, or zero for `false` if it is not.
| query->SortBy[] | void*[] (pExpression[]) | An array of expressions giving the various components of the sorting criteria.
| query->Flags    | int                     | The driver should set and/or clear the `OBJ_QY_F_FULLQUERY` and `OBJ_QY_F_FULLSORT` flags, if needed.

The `OBJ_QY_F_FULLQUERY` flag indicates that the driver will handle the full `where` clause specified in `query->Tree`.  Even if this flag is not specified, the driver is still free to use the provided `where` clause to pre-filter data, which improves performance when the Object System does its final filtering.  However, setting this flag disables the Object System filtering because it promises that the driver will _always_ handle _all_ filtering for _every_ valid queries.

The `OBJ_QY_F_FULLSORT` flag indicates that the driver will handle all sorting for the data specified in `query->SortBy[]`.

If the driver can easily handle sorting/selection (as when querying an database), it should set these flags. Otherwise, it should let the OSML handle the ORDER BY and WHERE conditions to avoid unnecessary work for the driver author.

The `OpenQuery()` function returns a `void*` for the query instance struct, which will be passed to the other query functions (`QueryDelete()`, `QueryFetch()`, and `QueryClose()`).  This structure normally points to the driver instance struct to allow easy access to queried data.  `OpenQuery()` returns `NULL` if the object does not support queries or if an error occurs, in which case `mssError()` should be called before returning.


### Function: QueryDelete()
*(Optional)*
```c
int xxxQueryDelete(void* qy_v, pObjTrxTree* oxt);
```
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
if (count < 0 || 256 <= count) goto error_handling;
obj->Pathname->Elements[obj->Pathname->nElements++] = strrchr(obj->Pathname->Pathbuf, '/') + 1;
```

### Function: QueryCreate()
```c
void* xxxQueryCreate(void* qy_v, pObject new_obj, char* name, int mode, int permission_mask, pObjTrxTree *oxt);
```
The `QueryCreate()` function is just a stub function that is not fully implemented yet.  Simply not providing it (aka. setting the location in the driver initialization struct to `NULL`) is fine.


### Function: QueryClose()
```c
int xxxQueryClose(void* qy_v, pObjTrxTree* oxt);
```
The `QueryClose()` function closes a query instance, freeing all allocated data and releasing all shared memory such as open connections, files, or other driver instances.  This function operates very similarly to `Close()`, documented in detail above.  The query should be closed, whether or not `QueryFetch()` has been called enough times to enumerate all of the query results.


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
| name         | The name of the object, just as it appears in any directory listing.  The name of the object must always be unique for its level in the tree (e.g. a unique file name in a directory, the primary key of a database row, etc.).
| annotation   | A short description of the object.  While users may not assign annotations to all objects, each object should be able to have an annotation.  For example, in the Sybase driver, annotations for rows are created by assigning an 'expression' to the table in question, such as `first_name + last_name` for a people table.  This attribute should _never_ be null, however, it can be an empty string (`""`) if the driver has no meaningful way to provide an annotation.
| content_type | The type of the object's content, given as a MIME-type.  Specify `"system/void"` if the object does not have content.
| inner_type   | An alias for 'content_type'.  Both should be supported.
| outer_type   | This is the type of the object itself (the container).

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
| datatype  | int           | The expected datatype for the requested value.
| val       | pObjData      | A pointer to a location where the value of the attribute should be stored.
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

The value pointer points to a union struct which can hold one of several types of data in the same memory location.  Which type of data is expected depends on the value of the `datatype` parameter.
| Field       | Datatype           | Description
| ----------- | ------------------ | -----------
| `Integer`   | `DATA_T_INTEGER`   | An int where the value should be written.
| `String`    | `DATA_T_STRING`    | A `char*` where a pointer to the string should be written.
| `Double`    | `DATA_T_DOUBLE`    | A double where the double should be written.
| `DateTime`  | `DATA_T_DATETIME`  | A `pDateTime` where a pointer to the `DateTime` struct (see [`datatypes.h`](../centrallix/include/datatypes.h)) should be written.
| `IntVec`    | `DATA_T_INTVEC`    | A `pIntVec` where a pointer to the `IntVec` struct (see [`datatypes.h`](../centrallix/include/datatypes.h)) should be written.
| `StringVec` | `DATA_T_STRINGVEC` | A `pStringVec` where a pointer to the `StringVec` struct (see [`datatypes.h`](../centrallix/include/datatypes.h)) should be written.
| `Money`     | `DATA_T_MONEY`     | A `pMoneyType` where a pointer to the `MoneyType` struct (see [`datatypes.h`](../centrallix/include/datatypes.h)) should be written.
| `Generic`   | ?                  | A `void*` to somewhere where something should be written should be written (usually implementation dependant).

In this way, `int`s and `double`s can be returned by value while other types are returned by reference.  Items returned by reference must be guaranteed to be valid until either the object is closed, or another `GetAttrValue()` or `SetAttrValue()` call is made on the same driver (which ever happens first).

This function should return 0 on success, 1 if the value is `NULL` or undefined / unset, or -1 on a non-existent attribute or other error.

- 📖 **Note**: The caller can use the `POD(x)` macro to typecast appropriate pointers to the `pObjData` pointer.  For example:
    ```c
    char* name;
    if (xxxGetAttrValue(obj, "name", DATA_T_STRING, POD(&name)) != 0)
        goto error_handling;
    printf("Object name: \"%s\"\n", name);
    ```

- 📖 **Note**: In legacy code, a typecasted `void*` was used instead of a `pObjData` pointer used today.  This method was binary compatible the current solution because of the union struct implementation (See [`datatypes.h`](../centrallix/include/datatypes.h) for more information).


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
- `hints->Format : char*`: A presentation format for datetime or money types, such as `"dd MMM yyyy HH:mm"` or `"$0.00"`.  See `obj_datatypes.c` (near line 100) for more information creating a presentation format.
- `hints->AllowChars : char*`: An array of all valid characters for a string attribute, NULL to allow all characters.
- `hints->BadChars : char*`: An array of all invalid characters for a string attribute.  If a character appears in both `hints->BadChars` and `hints->AllowChars`, the character should be rejected.
- `hints->Length : int`: The maximum length of data that can be included in a string attribute.
- `hints->VisualLength : int`: The length that the attribute should be displayed if it is show to the user.
- `hints->VisualLength2 : int`: The number of lines to use in a multi-line edit box for the attribute.
- `hints->BitmaskRO : unsigned int`: If the value is an integer that represents a bit mask, _this_ bit mask shows which bits of that bitmask are read-only.
- `hints->Style : int`: Style flags, documented below.
- `hints->StyleMask : int`: A mask for which style flags were set and which were left unset / undefined.
- `hints->GroupID : int`: Used to assign attributes to groups. Use -1 if the attribute is not in a group.
- `hints->GroupName : char*`: The name of the group to which this attribute belongs, or NULL if it is ungrouped or if the group is named elsewhere.
- `hints->OrderID : int`: Used to specify an attribute order.
- `hints->FriendlyName : char*`: Used to specify a "display name" for an attribute (e.g. `n_rows` might have a friendly name of `"Number of Rows"`). Should be [`nmSysMalloc()`](#nmsysmalloc)ed, often using [`nmSysStrdup()`](#nmsysstrdup).

- ⚠️ **Warning**: Behavior is undefined if:
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
- `OBJ_PH_STYLE_MULTISEL`: This enum attribute can accept more than one value from the list of valid values.  Think of using checkboxes instead of radio buttons (although the flag does requirement this UI decision).
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
- `OBJ_INFO_F_SUPPORTS_INHERITANCE`: Indicates that the object supports inheritance through attributes such as `cx__inherit`.
- `OBJ_INFO_F_FORCED_LEAF`: Indicates that the object is forced to be a 'leaf' unless ls__type used.
- `OBJ_INFO_F_TEMPORARY`: Indicates that this is a temporary object without a valid pathname.

The function returns 0 on success, and -1 to indicate an error, in which case `mssError()` should be called before returning.


### Function: Commit()
```c
int xxxCommit(void* inf_v, pObjTrxTree *oxt);
```
The `Commit()` function immediately completes the current transaction, ensuring that all writes are applied to the affected data before returning.  For example, if the current transaction involves creating a database row, this call will ensure that the row is created and the transaction is closed before returning.  This allows the caller to ensure that actions in a transaction have been completed without needing to close the object, which they may wish to continue using.

The `Commit()` function takes two parameters:

| Parameter | Type          | Description
| --------- | ------------- | ------------
| inf_v     | void*         | A driver instance pointer (returned from `Open()` or `QueryFetch()`).
| oxt       | pObjTrxTree*  | The transaction tree pointer for the `OBJDRV_C_TRANS` capability.

The function returns 0 on success, and -1 to indicate an error, in which case `mssError()` should be called before returning.


### Function: GetQueryCoverageMask()
```c
int xxxGetQueryCoverageMask(pObjQuery this);
```
This function is only intended to be used by the MultiQuery module.  Any other driver should not provide this function by setting the appropriate struct field to `NULL`.


### Function: GetQueryIdentityPath()
```c
int xxxGetQueryIdentityPath(pObjQuery this, char* pathbuf, int maxlen);
```
This function is only intended to be used by the MultiQuery module.  Any other driver should not provide this function by setting the appropriate struct field to `NULL`.



## III Reading the Node Object
A driver will commonly configure itself by reading text content from its node object file, at the root of its object subtree.  This content may define what resource(s) a driver should provide, how it should access or compute them, and other similar information.  Most drivers use the structure file format for their node objects because SN module makes parsing, reading, and writing these files easier.  It also performs caching automatically to improve performance.

- 📖 **Note**: The node object will **already be open** as an object in the ObjectSystem: The OSML does this for each driver.  If a driver does not use the SN/ST modules, then it should read and write the node object directly with `objRead()` and `objWrite()`.  A driver should **NEVER** `objClose()` the node object!  The OSML handles that.

Although using the structure file format may be complex, it allows significant flexibility, as well as greater consistency across drivers.  The use of this shared syntax across different drivers makes learning to use a new driver far easier than it would be if they all used unique, custom syntax for specifying properties.  In the structure file syntax, data is structured in hierarchies where each sub-object can have named attributes as well as sub-objects.  Centrallix has many examples of this, including any `.qy`, `.app`, `.cmp`, or `.cluster` file.

Structure files are accessed via the st_node (SN) and stparse (SP) modules.  The st_node module loads and saves the structure file heirarchies as a whole.  It also manages caching to reduce disk activity and eliminate repeated parsing of the same file.  The stparse module provides access to the individual attributes and groups of attributes within a node structure file.

For example, if two sessions open two files, `/test1.rpt` and `/test2.rpt` the st_node module will cache the internal representations of these node object files, and for successive uses of these node objects, the physical file will not be re-parsed.  The file will be re-parsed if its timestamp changes.

If the underlying object does not support the attribute "last_modification" (assumed to be the timestamp), then st_node prints a warning.  In essence, this warning indicates that changes to the underlying object will not trigger the st_node module to re-read the structure file defining the node object.  Otherwise, the st_node module keeps track of the timestamp, and if it changes, the node object is re-read and re-parsed.

### Module: st_node
To obtain node object data, the driver should first open the node object with the st_node module.  To use this module, include the file `st_node.h`, which provides the following functions (read `st_node.c` for more functions and additional information):


### st_node: snReadNode()
```c
pSnNode snReadNode(pObject obj);
```
The `snReadNode()` function reads a Structure File from the `obj` parameter, which should be a previously opened object.  In a driver's `Open()` function, this is `obj->Prev` (the node object as opened by the previous driver in the OSML's chain of drivers).

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

- ⚠️ **Warning**: The node object root of type `ST_T_STRUCT` will return `ST_T_SUBGROUP` from this function.  In most cases, treating this node as ust another subgroup simplifies logic for the caller.  However, if you wish to avoid this behavior, read `inf->Type` (see [stparse: Using Fields Directly](#stparse-using-fields-directly) for more info).


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
This function adds a value to an attribute, and can be called multiple times on an attribute to add a list of values.  If `strval` is not null, a string value is added, otherwise an integer value is added.  The string is NOT copied, but is simply pointed-to.  If the string is non-static, and has a lifetime less than the `ST_T_ATTRIB` tree node, then the following procedure should be used to allocate a new string which will have the correct lifetime:  (In this example, `str` is the string pointer to the string.)

```c
pStructInf attr_inf = stAddAttr(my_parent_inf, "my_attr");
if (attr_inf == NULL) goto error_handling;

char* new_str = (char*)nmSysMalloc(strlen(str) + 1lu);
if (new_str == NULL) goto error_handling;
strcpy(new_str, str);
stAddValue(attr_inf, new_str, 0);
attr_inf->StrAlloc[0] = 1;
```

With this method (making a copy of the string and then setting the StrAlloc value for that string), the string is automatically freed when the StructInf tree node is freed by the stparse module.

<!-- TODO: Greg - I just realized I didn't explain what this function returns and that info isn't clear from the implementation. Could you explain the meaning of the int that this function returns? -->

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
Centrallix has its own memory management wrapper that caches deallocated blocks of memory by size for faster reuse.  This wrapper also detects double-freeing of blocks (sometimes), making debugging of memory problems just a little bit easier.

In addition, the memory manager provides statistics on the hit ratio of allocated blocks coming from the lists vs. `malloc()`, and on how many blocks of each size/type are `malloc()`ed and cached.  This information can be helpful for tracking down memory leaks.  Empirical testing has shown an increase of performance of around 50% or more in programs that use newmalloc.

One caveat is that this memory manager does not provide `nmRealloc()` function, only `nmMalloc()` and `nmFree()`.  Thus, either `malloc()`, `free()`, and `realloc()` or [`nmSysMalloc()`](#nmsysmalloc), [`nmSysFree()`](#nmsysfree), and [`nmSysRealloc()`](#nmsysrealloc) should be used for blocks of memory that might vary in size.

- 📖 **Note**: This memory manager is usually the wrong choice for blocks of memory of arbitrary, inconsistent sizes.  It is intended for allocating structures quickly that are of a specific size.  For example, allocated space for a struct that is always the same size.

- 🥱 **tl;dr**: Use `nmMalloc()` for structs, not for strings.

- ⚠️ **Warning**: Do not mix and match, even though calling `free()` on a block obtained from `nmMalloc()` or calling `nmFree()` on a block obtained from `malloc()` might not crash the program immediately.  However, it may result in either inefficient use of the memory manager, or a significant memory leak, respectively.  These practices will also lead to incorrect results from the statistics and block count mechanisms.


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
   nmMalloc: 20244967 calls, 19908369 hits (98.337%)
   nmFree: 20233966 calls
   bigblks: 49370 too big, 32768 largest size
```

- ⚠️ **Warning**: Centrallix-lib must be built with the configure option `--enable-debugging` for this function to work.  Otherwise, all the stats will be zeros.


### nmRegister()
```c
void nmRegister(int size, char* name);
```
Registers an inteligent name for block of the specified size.  This allows the memory manager to give more information when reporting block allocation counts.  A given size can have more than one name.  This function is optional and not required for any production usecases, but using it can make tracking down memory leaks easier.

This function is usually called in a module's `Initialize()` function on each of the structures the module uses internally.


### nmDebug()
```c
void nmDebug(void);
```
Prints a listing of block allocation counts, giving (by size):
- The number of blocks allocated but not yet freed.
- The number of blocks in the cache.
- The total allocations for this block size.
- A list of names (from [`nmRegister()`](#nmregister)) for that block size.


### nmDeltas()
```c
void nmDeltas(void);
```
Prints a listing of all blocks whose allocation count has changed, and by how much, since the last `nmDeltas()` call.  This function is VERY USEFUL FOR MEMORY LEAK DETECTIVE WORK.


### nmSysMalloc()
```c
void* nmSysMalloc(int size);
```
Allocates memory without using the block-caching algorithm.  This is roughly equivalent to `malloc()`, but pointers returned by malloc and this function are not compatible - i.e., you cannot `free()` something that was [`nmSysMalloc()`](#nmsysmalloc)'ed, nor can you [`nmSysFree()`](#nmsysfree) something that was `malloc()`'ed.

- 📖 **Note**: This function is much better to use on variable-sized blocks of memory.  `nmMalloc()` is better for fixed-size blocks, such as for structs.


### nmSysRealloc()
```c
void* nmSysRealloc(void* ptr, int newsize);
```
Changes the size of an allocated block of memory that was obtained from [`nmSysMalloc()`](#nmsysmalloc), [`nmSysRealloc()`](#nmsysrealloc), or [`nmSysStrdup()`](#nmsysstrdup).  The new pointer may be different if the block needs to be moved.  This is the rough equivalent of `realloc()`.

- 📖 **Note**: If you are `realloc()`'ing a block of memory and need to store pointers to data somewhere inside the block, it is often better to store an offset rather than a full pointer.  This is because a full pointer becomes invalid if a [`nmSysRealloc()`](#nmsysrealloc) causes the block to move.


### nmSysStrdup()
```c
char* nmSysStrdup(const char* str);
```
Allocates memory using the [`nmSysMalloc()`](#nmsysmalloc) function and copies the string `str` into this memory.  It is a rough equivalent of `strdup()`.  The resulting pointer can be free'd using [`nmSysFree()`](#nmsysfree).


### nmSysFree()
```c
void nmSysFree(void* ptr);
```
Frees a block of memory allocated by [`nmSysMalloc()`](#nmsysmalloc), [`nmSysRealloc()`](#nmsysrealloc), or [`nmSysStrdup()`](#nmsysstrdup).



## V Module: XArray
The xarray (xa) module is intended to manage sized growable arrays, similar to a light-weight arraylist implementation.  It includes the `XArray`, which has the following fields:
- `nItems : int`: The number of items in the array.
- `nAlloc : int`: Internal variable to store the size of the allocated memory.
- `Items : void**`: The allocated array of items.

- 📖 **Note**: Some code occasionally sets `nAlloc` to 0 after an XArray struct has been deinitialized to indicate that the relevant data is no longer allocated.  Other than this, it is only used internally by the library.

- ⚠️ **Warning**: Do not mix calls to [`xaNew()`](#xanew)/[`xaFree()`](#xafree) with calls to [`xaInit()`](#xainit)/[`xaDeInit()`](#xadeinit).  Every struct allocated using new must be freed, and ever struct allocated using init must be deinitted.  Mixing these calls can lead to memory leaks, bad frees, and crashes.


### xaNew()
```c
pXArray xaNew(int init_size);
```
Allocates a new `XArray` struct on the heap (using [`nmMalloc()`](#nmmalloc) for caching) and returns a pointer to it, or returns `NULL` if an error occurs.

### xaFree()
```c
int xaFree(pXArray this);
```
Frees a `pXArray` allocated using [`xaNew`](#xanew), returning 0 if successful or -1 if an error occurs.

### xaInit()
```c
int xaInit(pXArray this, int init_size);
```
This function initializes an allocated (but uninitialized) xarray. It makes room for `init_size` items initially, but this is only an optimization.  A typical value for `init_size` is 16.  Remember to [`xaDeInit`](#xadeinit) this xarray, do **not** [`xaFree`](#xafree) it.

This function returns 0 on success, or -1 if an error occurs.

### xaDeInit()
```c
int xaDeInit(pXArray this);
```
This function de-initializes an xarray, but does not free the XArray structure itself.  This is useful if the structure is a local variable allocated using [`xaInit()`](#xainit).

This function returns 0 on success, or -1 if an error occurs.

For example:
```c
XArray arr;
if (xaInit(&arr, 16) != 0) goto handle_error;

/** Use the xarray. **/

if (arr.nAlloc != 0 && xaDeInit(&arr) != 0) goto handle_error;
arr.nAlloc = 0;
```

### xaAddItem()
```c
int xaAddItem(pXArray this, void* item);
```
This function adds an item to the end of the xarray.  The item is assumed to be a `void*`, but this function will _not_ follow pointeres stored in the array.  Thus, other types can be typecast and stored into that location (such as an `int`).

This function returns 0 on success, or -1 if an error occurs.

### xaAddItemSorted()
```c
int xaAddItemSorted(pXArray this, void* item, int keyoffset, int keylen);
```
This function adds an item to a sorted xarray while maintaining the sorted property.  The value for sorting is expected to begin at the offset given by `keyoffset` and continue for `keylen` bytes.  This function _will_ follow pointers are stored in the array so casting other types to store them is not allowed (as it is with [`xaAddItem()`](#xaadditem)).

### xaAddItemSortedInt32()
```c
int xaAddItemSortedInt32(pXArray this, void* item, int keyoffset)
```
<!-- TODO: Greg - How does this work? Does it assume that the item pointers are typecasted 32-bit ints or that they point to 32-bit ints? -->

### xaGetItem()
```c
void* xaGetItem(pXArray this, int index)
```
This function returns an item given a specific index into the xarray, or `NULL` if the index is out of bounds.  If the bounds check needs to be omitted for performance and the caller can otherwise verify that no out of bounds read is possible (e.g. because they are iterating from 0 to `xarray->nItems`), the caller should access `xarray->Items` directly.  Either way, the result may need to be typecasted or stored in a variable of a specific type for it to be useable, and error checking for `NULL` values should be used.

### xaFindItem()
```c
int xaFindItem(pXArray this, void* item);
```
This function returns array index for the provided item in the array, or -1 if the item could not be found.  Requires an exact match, so two `void*` pointing to different memory with identical contents are not considered equal by this function.  If the data is actually another datatype typecasted as a `void*`, all 8 bytes must be identical for a match.

For example:
```c
void* data = &some_data;

XArray xa;
xaInit(&xa, 16);

...

xaAddItem(&xa, data);

...

int item_id = xaFindItem(&xa, data);
assert(data == xa.Items[item_id]);
```

### xaFindItemR()
```c
int xaFindItemR(pXArray this, void* item);
```
This function works the same as [`xaFindItem()`](#xafinditem), however it iterates in reverse, giving a slight performance boost, especially for finding items near the end of the array.

### xaRemoveItem(pXArray this, int index)
```c
int xaRemoveItem(pXArray this, int index)
```
This function removes an item from the xarray at the given the index, then shifts all following items back to fill the gap created by the removal.  XArray is not optimized for removing multiple items efficiently.  This function returns 0 on success, or -1 if an error occurs.

### xaClear()
```c
int xaClear(pXArray this, int (*free_fn)(), void* free_arg);
```
This function removes all elements from the xarray, leaving it empty.  `free_fn()` is invoked on each element with a `void*` to the element to be freed as the first argument and `free_arg` as the second argument (the return value of `free_fn()` is always ignored).  This function returns 0 on success (even if the `free_fn()` returns an error), or -1 if an error is detected.

### xaClearR()
```c
int xaClearR(pXArray this, int (*free_fn)(), void* free_arg);
```
This function works the same as [`xaClear()`](#xaclear), except that it is slightly faster because the free function is evaluated on items in reverse order.

### xaCount()
```c
int xaCount(pXArray this);
```
This function returns the number of items in the xarray, or -1 on error.  It is equivalent to accessing `xarray->nItems` (although the latter expression will not return an error).

### xaInsertBefore()
```c
int xaInsertBefore(pXArray this, int index, void* item)
```
This function inserts an item before the specified index, moving all following items forward to make space.  The new item cannot be inserted past the end of the array.  This function returns the index on success, or -1 if an error occurs.

### xaInsertAfter()
```c
int xaInsertAfter(pXArray this, int index, void* item)
```
This function inserts an item after the specified index, moving all following items forward to make space.  The new item cannot be inserted past the end of the array.  This function returns the index on success, or -1 if an error occurs.



## VI Module: XHash
The xhash (xh) module provides an extensible hash table interface.  The hash table is a table of linked lists of items, so collisions and overflows are handled by this data structure (although excessive collisions still cause a performance loss).  This implementation also supports variable-length keys for more flexible usecases.

- ⚠️ **Warning**: All `xhXYZ()` function calls assume that the `pXHashTable this` arg points to a valid hashtable struct.  All non-init functions assume that this struct has been validly initialized and has not yet been freed.  If these conditions are not met, the resulting behavior is undefined.

### xhInitialize()
```c
int xhInitialize();
```
Initialize the random number table for hash computation, returning 0 on success or -1 if an error occurs.  Normally, you can assume someone else has already called this during program startup.

### xhInit()
```c
int xhInit(pXHashTable this, int rows, int keylen);
```
This function initializes a hash table, setting the number of rows and the key length.  Specify a `keylen` of 0 for for variable length keys (aka. null-terminated strings).  The `rows` should be an odd number, preferably prime (although that isn't required).  `rows` **SHOULD NOT** be a power of 2.  Providing this value allows the caller to optimize it based on how much data they expect to be stored in the hash table.  If this value is set to 1, the hash search degenerates to a linear array search with extra overhead.  Thus, the value should be large enough to comfortably accommodate the elements with minimal collisions.  Typical values include 31, 251, or 255 (though 255 is not prime).

### xhDeInit()
```c
int xhDeInit(pXHashTable this);
```
This function deinitializes a hash table struct, freeing all rows.  Note that the stored data is not freed and neither are the keys as this data is assumed to be the responsibility of the caller.  Returns 0 on success, or -1 if an error occurs.

### xhAdd()
```c
int xhAdd(pXHashTable this, char* key, char* data);
```
Adds an item to the hash table, with a given key value and data pointer.  Both data and key pointers must have a lifetime that exceeds the time that they item is hashed, as they are assumed to be the responsibility of the caller.  This function returns 0 on success, or -1 if an error occurs.

### xhRemove()
```c
int xhRemove(pXHashTable this, char* key);
```
This function removes an item with the given key value from the hash table.  It returns 0 if the item was successfully removed, or -1 if an error occurs (including failing to find the item).

### xhLookup()
```c
char* xhLookup(pXHashTable this, char* key);
```
This function returns a pointer to the data associated with the given key, or `NULL` if an error occurs (including failing to find the key).

### xhClear()
```c
int xhClear(pXHashTable this, int (*free_fn)(), void* free_arg);
```
Clears all items from a hash table.  If a `free_fn()` is provided, it will be invoked with each data pointer as the first argument and `free_arg` as the second argument as items are removed.  The return value of the `free_fn()` is ignored.  This function returns 0 on success (even if the `free_fn()` returns an error), or -1 if an error is detected.

### xhForEach()
```c
int xhForEach(pXHashTable this, int (*callback_fn)(pXHashEntry, void*), void* each_arg);
```
This function executes an operation on each entry of the hash table entry.  The provided callback function will be called with each entry (in an arbitrary order).  This function is provided 2 parameters: the current hash table entry, and a `void*` argument specified using `each_arg`.  If any invocation of the callback function returns a value other than 0, the `xhForEach()` will immediately fail, returning that value as the error code.

This function returns 0 if the function executes successfully, 1 if the callback function is `NULL`, or n (where n != 0) if the callback function returns n.  It does not return any error code other than 1 or any error codes returned by `callback_fn()`.

### xhClearKeySafe()
```c
int xhClearKeySafe(pXHashTable this, void (*free_fn)(pXHashEntry, void*), void* free_arg);
```
This function clears all contents from the hash table.  The free function is passed each hash entry struct and `free_arg`, allowing it to free both the value and key, if needed, and the free function is not allowed to return an error code.  This function returns 0 for success as long as `free_fn()` is nonnull, otherwise it returns -1.



## VII Module: XString
The xstring (xs) module is used for managing growable strings.  It is based on a structure containing a small initial string buffer to avoid string allocations for small strings.  However, it can also perform `realloc()` operations to extend the string space for storing incrementally larger strings.  This module allows for strings to contain arbitrary data, even NULL (`'\0'`) characters mid-string.  Thus, it can also be used as an extensible buffer for arbitrary binary data.

- 📖 **Note**: The contents of the XString can be easily referenced with the `xstring->String` field in the xstring struct.

- ⚠️ **Warning**: Do not mix calls to [`xsNew()`](#xsnew)/[`xsFree()`](#xsfree) with calls to [`xsInit()`](#xsinit)/[`xsDeInit()`](#xsdeinit).  Every struct allocated using new must be freed, and ever struct allocated using init must be deinitted.  Mixing these calls can lead to memory leaks, bad frees, and crashes.

### xsNew()
```c
pXString xsNew()
```
This function allocates a new XString structure to contain a new, empty string.  It uses [`nmMalloc()`](#nmmalloc) because the XString struct is always a consistant size.  This function returns a pointer to the new string if successful, or `NULL` if an error occurs.

### xsFree()
```c
void xsFree(pXString this);
```
This function frees an XString structure allocated with [`xsNew()`](#xsnew), freeing all associated memory.

### xsInit()
```c
int xsInit(pXString this);
```
This function initializes an XString structure to contain a new, empty string.  This function returns 0 if successful, or -1 if an error occurs.

### xsDeInit()
```c
int xsDeInit(pXString this);
```
This function deinitializes an XString structure allocated with [`xsInit()`](#xsinit), freeing all associated memory.  This function returns 0 if successful, or -1 if an error occurs.

### xsCheckAlloc()
```c
int xsCheckAlloc(pXString this, int addl_needed);
```
This function will optionally allocate more memory, if needed, given the currently occupied data area and the additional space required (specified with `addl_needed`).  This function returns 0 if successful, or -1 if an error occurs.

### xsConcatenate()
```c
int xsConcatenate(pXString this, char* text, int len);
```
This function concatenates the `text` string onto the end of the XString's value.  If `len` is set, that number of characters are copied, including possible null characters (`'\0'`).  If `len` is -1, all data up to the null-terminater is copied.  This function returns 0 if successful, or -1 if an error occurs.

- ⚠️ **Warning**: Do not store pointers to values within the string while adding text to the end of the string.  The string may be reallocated to increase space, causing such pointers to break.  Instead, use offset indexes into the string and calculate pointers on demand with `xs->String + offset`.
    
    For example, **DO NOT**:
    ```c
    XString xs;
    if (xsInit(&xs) != 0) goto handle_error;
    
    if (xsConcatenate(&xs, "This is the first sentence. ", -1) != 0) goto handle_error;
    char* ptr = xsStringEnd(&xs); /* Stores string pointer! */
    if (xsConcatenate(&xs, "This is the second sentence.", -1) != 0) goto handle_error;
    
    /** Print will probably read invalid memory. **/
    printf("A pointer to the second sentence is '%s'\n", ptr);
    
    ...
    
    if (xsDeInit(&xs) != 0) goto handle_error;
    ```
    
    Instead, use indexes and pointer arithmetic like this:
    ```c
    XString xs;
    if (xsInit(&xs) != 0) goto handle_error;
    
    if (xsConcatenate(&xs, "This is the first sentence. ", -1) != 0) goto handle_error;
    int offset = xsStringEnd(&xs) - xs->String; /* Stores index offset. */
    if (xsConcatenate(&xs, "This is the second sentence.", -1) != 0) goto handle_error;
    
    /** Print will probably work fine. **/
    printf("A pointer to the second sentence is '%s'\n", xs->String + offset);
    
    ...
    
    if (xsDeInit(&xs) != 0) goto handle_error;
    ```

### xsCopy()
```c
int xsCopy(pXString this, char* text, int len);
```
This function copies the string `text` into the XString, overwriting any previous contents.  This function returns 0 if successful, or -1 if an error occurs.

### xsStringEnd()
```c
char* xsStringEnd(pXString this);
```
This function returns a pointer to the end of the string.  This function is more efficient than searching for a null-terminator using `strlen()` because the xs module already knows the string length.  Furthermore, since some string may contain nulls, using `strlen()` may produce an incorrect result.

### xsConcatPrintf()
```c
int xsConcatPrintf(pXString this, char* fmt, ...);
```
This function prints additional data onto the end of the string.  It is similar to printf, however, only the following features are supported:
- `%s`: Add a string (`char*`).
- `%d`: Add a number (`int`).
- `%X`: Add something?
- `%%`: Add a `'%'` character.
Attempting to use other features of printf (such as `%lf`, `%c`, `%u`, etc.) will cause unexpected results.

This function returns 0 if successful, or -1 if an error occurs.

### xsPrintf()
```c
int xsPrintf(pXString this, char* fmt, ...);
```
This function works the same as [`xsConcatPrintf()`](#xsconcatprintf), except that it overwrites the previous string instead of appending to it.  This function returns 0 if successful, or -1 if an error occurs.

### xsWrite()
```c
int xsWrite(pXString this, char* buf, int len, int offset, int flags);
```
This function writes data into the xstring, similar to using the standard fdWrite or objWrite API.  This function can thus be used as a value for `write_fn`, for those functions that require this (such as the `expGenerateText()` function).  This function returns `len` if successful, or -1 if an error occurs.

### xsRTrim()
```c
int xsRTrim(pXString this);
```
This function trims whitespace characters (spaces, tabs, newlines, and line feeds) from the right side of the xstring.  This function returns 0 if successful, or -1 if an error occurs.

### xsLTrim()
```c
int xsLTrim(pXString this);
```
This function trims whitespace characters (spaces, tabs, newlines, and line feeds) from the left side of the xstring.  This function returns 0 if successful, or -1 if an error occurs.

### xsTrim()
```c
int xsTrim(pXString this);
```
This function trims whitespace characters (spaces, tabs, newlines, and line feeds) from both sides of the xstring.  This function returns 0 if successful, or -1 if an error occurs.

### xsFind()
```c
int xsFind(pXString this, char* find, int findlen, int offset)
```
This function searches for a specific string (`find`) in the xstring, starting at the provided `offset`.  `findlen` is the length of the provided string, allowing it to include null characters (pass -1 to have the length calculated using `strlen(find)`).  This function returns the index where the string was found if successful, or -1 if an error occurs (including the string not being found).

### xsFind()
```c
int xsFindRev(pXString this, char* find, int findlen, int offset)
```
This function works the same as [`xsFind()`](#xsfind) except that it searches from the end of the string, resulting in better performance if the value is closer to the end of the string.  This function returns the index where the string was found if successful, or -1 if an error occurs (including the string not being found).

### xsSubst()
```c
int xsSubst(pXString this, int offset, int len, char* rep, int replen)
```
This function substitutes a string into a given position in an xstring.  This does not search for matches as with [`xsReplace()`](#xsrepalce), instead the position (`offset`) and length (`len`) must be specified.  Additionally, the length of the replacement string (`replen`) can be specified handle null characters.  Both `len` and `replen` can be left blank to generate them using `strlen()`.  This function returns 0 if successful, or -1 if an error occurs.

### xsReplace()
```c 
int xsReplace(pXString this, char* find, int findlen, int offset, char* rep, int replen);
```
This function searches an xString for the specified string (`find`) and replaces that string with another specified string (`rep`).  Both strings can have their length specified (`findlen` and `replen` respectively), or left as -1 to generate it using `strlen()`.  This function returns the starting offset of the replace if successful, or -1 if an error occurs (including the string not being found).

### xsInsertAfter()
```c
int xsInsertAfter(pXString this, char* ins, int inslen, int offset);
```
This function inserts the specified string (`ins`) at offset (`offset`).  The length of the string can be specified (`inslen`), or left as -1 to generate it using `strlen()`.  This function returns the new offset after the insertion (i.e. `offset + inslen`), or -1 if an error occurs.

### xsGenPrintf_va()
```c
int xsGenPrintf_va(int (*write_fn)(), void* write_arg, char** buf, int* buf_size, const char* fmt, va_list va);
```
This function performs a `printf()` operation to an `xxxWrite()` style function.

In the wise words of Greg Beeley from 2002:
> This routine isn't really all that closely tied to the XString module, but this seemed to be the best place for it.  If a `buf` and `buf_size` are supplied (`NULL` otherwise), then `buf` MUST be allocated with the `nmSysMalloc()` routine.  Otherwise, **kaboom!**  This routine will grow `buf` if it is too small, and will update `buf_size` accordingly.

This function returns the printed length (>= 0) on success, or -(errno) if an error occurs.

### xsGenPrintf()
```c
int xsGenPrintf(int (*write_fn)(), void* write_arg, char** buf, int* buf_size, const char* fmt, ...);
```
This function works the same as [`xsGenPrintf_va()`](#xsgenprintf_va), but with a more convenient signature for the developer.

### xsString()
```c
char* xsString(pXString this);
```
This function returns the stored string after checking for various errors, or returns `NULL` if an error occurs.

### xsLength()
```c
xsLength(pXString this);
```
This function returns the length of the string in constant time (since this value is stored in `this->Length`) checking for various errors, or returns `NULL` if an error occurs.

<!-- TODO: Greg - So why do we need xsStringEnd() again when `this->String + this->Length` or `this->String + xsLength()` appears to also solve all the same problems? Is it just to support legacy code? -->

### xsQPrintf_va(), xsQPrintf(), & xsConcatQPrintf()
```c
int xsQPrintf_va(pXString this, char* fmt, va_list va);
int xsQPrintf(pXString this, char* fmt, ...);
int xsConcatQPrintf(pXString this, char* fmt, ...);
```
These functions use the `QPrintf` to add data to an xstring.  They return 0 on success, or some other value on failure.



## VIII Module: Expression
The expression (EXP) module is used for compiling, evaluating, reverse-evaluating, and managing parameters for expression strings.  The expression strings are compiled and stored in an expression tree structure.

Expressions can be stand-alone expression trees, or they can take parameter objects.  A parameter object is an open object (from `objOpen()`) whose values (attributes) are referenced within the expression string.  By using such parameter objects, one expression can be compiled and then evaluated for many different objects with diverse attribute values.

Expression evaluation results in the top-level expression tree node having the final value of the expression, which may be `NULL`, and may be an integer, string, datetime, money, or double data type.  For example, the final value of `:myobject:oneattribute == 'yes'` is the integer 1, `true`, if the attribute's value is indeed `'yes'` (and the integer 0, `false`, otherwise).

Expression reverse-evaluation takes a given final value and attempts to assign values to the parameter object attributes based on the structure of the expression tree.  It is akin to 'solving for X' in algebraic work, but isn't nearly that 'smart'.  For example, with the previous expression, if the final value was set to 1 (`true`), then an `objSetAttrValue()` function would be called to set myobject's `oneattribute` to `yes`.  Trying this with a final value of 0 (`false`) would result in no assignment to the attribute, since there would be no way of determining the proper value for that attribute (anything other than `yes` would work).

Reverse evaluation is typically very useful in updateable joins and views.

The expression module includes the following functions:

### expAllocExpression()
```c
pExpression expAllocExpression();
```
This function allocates space to store a new expression tree, returning a pointer to the allocated memory or `NULL` if an error occurs.

### expFreeExpression()
```c
int expFreeExpression(pExpression this);
```
This function frees an expression tree allocated using `expAllocExpression()`, returning 0 if successful or -1 if an error occurs.

### expCompileExpression()
```c
pExpression expCompileExpression(char* text, pParamObjects objlist, int lxflags, int cmpflags);
```
This function compiles a textual expression into an expression tree.  The `objlist` lists the parameter objects that are allowed in the expression (see below for param objects maintenance functions).

The `lxflags` parameter is a bitmask that provides flags which will be passed to the lexer.  These flags alter the manner in which the input string is tokenized.  For information about these flags, see [`mlxOpenSession()`](#mlxopensession).

The `cmpflags` parameter is a bitmask that provides flags which will be passed to the expression compiler.  It can contain the following values:

| Value                | Description
| -------------------- | ------------
| `EXPR_CMP_ASCDESC`   | Recognize `asc`/`desc` following a value as flags to indicate sort order.
| `EXPR_CMP_OUTERJOIN` | Recognize the `*=` and `=*` syntax for left and right outer joins.
| `EXPR_CMP_WATCHLIST` | A list (`"value,value,value"`) is expected first in the expression.
| `EXPR_CMP_LATEBIND`  | Allow late object-name binding.
| `EXPR_CMP_RUNSERVER` | Compile as a `runserver` expression (for dynamic binding).
| `EXPR_CMP_RUNCLIENT` | Compile as a `runclient` expression (for client-side binding).
| `EXPR_CMP_REVERSE`   | Lookup names in the reverse order.

### expCompileExpressionFromLxs()
```c
pExpression expCompileExpressionFromLxs(pLxSession s, pParamObjects objlist, int cmpflags);
```
This function is similar to [`expCompileExpression()`](#expcompileexpression), excpet that it compiles from a provided lexer session instead of from a string. 

### expPodToExpression()
```c
pExpression expPodToExpression(pObjData pod, int type, pExpression provided_exp)
```
This function builds an expression node from a single piece of data, passed using the `pObjData` of the given datatype.  This function can be used to initialize a provided expression (`provided_exp`), or it will allocate a new one if none is provided (aka. `provided_exp` is `NULL`).

For example, the following code creates an expression representing the integer 1.
```c
int value = 1;
pExpression exp = expPodToExpression(POD(value), DATA_T_INTEGER, NULL);
```

This function returns a pointer to the expression if successful, or `NULL` if an error occurs.

- 📖 **Note**: There is also a `expPtodToExpression()` function for working with the `Ptod` (pointer to object data) struct.

### expExpressionToPod()
```c
int expExpressionToPod(pExpression this, int type, pObjData pod);
```
This function reverses the functionality of [`expPodToExpression()`](#exppodtoexpression) to instead read data from an evaluated expression.  Be careful, this does not evaluate the expression if it is not already evaluated.  This function returns 0 if successful, 1 if the expression is NULL, or -1 if an error occurs.

- 📖 **Note**: The source code for this function can be a useful reference when interacting with expression structures, such as when implementing the c code for an exp_function.

- 📖 **Note**: There is also a `expExpressionToPtod()` function for working with the `Ptod` (pointer to object data) struct.

### expDuplicateExpression()
```c
pExpression expDuplicateExpression(pExpression this);
```
This function creates a recursive deep copy of the expression and associated expression tree, returning a pointer to this new copy if successful and `NULL` if an error occurs.

### expIsConstant()
```c
int expIsConstant(pExpression this);
```
This function returns a truthy value if the provided expression is of a type that is always the same, such as an integer, string, double, etc. Otherwise, it returns a falsy value.

### expEvalTree()
```c
int expEvalTree(pExpression this, pParamObjects objlist);
```
This function evaluates the expression using the provided list of parameter objects.  It returns 0 if successful or 1 if the result is `NULL`, and -1 if an error occurs.

### expCreateParamList()
```c
pParamObjects expCreateParamList();
```
This function allocates and returns a new parameter object list containing no parameters, or returns `NULL` if an error occurs.

### expFreeParamList()
```c
int expFreeParamList(pParamObjects this);
```
This function frees a parameter object list, returning 0 if successful and -1 if an error occurs.

### expAddParamToList()
```c
int expAddParamToList(pParamObjects this, char* name, pObject obj, int flags);
```
This function adds a parameter to the parameter object list.  The `obj` pointer may be left `NULL` during the expCompileExpression state of operation but must be set to a value before expEvalTree is called.  Otherwise the attributes that reference that parameter object will result in `NULL` values in the expression. (Although this _technically_ is not an error, it's usually not intended behavior).  Flags can be `EXPR_O_CURRENT` if the object is to be marked as the current one, or `EXPR_O_PARENT` if it is to be marked as the parent object.  Current and Parent objects can be referenced in an expression like this:

```
:currentobjattr
::parentobjattr
```

### expModifyParam()
```c
int expModifyParam(pParamObjects this, char* name, pObject replace_obj);
```
This function is used to update a parameter object with a new open pObject, possibly one returned from `objOpen()` or `objQueryFetch()`.  This function returns 0 if successful and -1 if an error occurs.

### expRemoveParamFromList()
```c
int expRemoveParamFromList(pParamObjects this, char* name);
```
This function removes a parameter object from the list, returning 0 if successful and -1 if an error occurs.

- 📖 **Note**: There is also a `expRemoveParamFromListById()` function.

### expSetParamFunctions()
```c
int expSetParamFunctions(pParamObjects this, char* name, int (*type_fn)(), int (*get_fn)(), int (*set_fn)());
```
This function sets the param accessor functions used to access params on a specific name.  Some example function signatures for the `type_fn()`, `get_fn()`, and `set_fn()` are provided below:

```c
static int ci_GetParamType(void* v, char* attr_name);
static int ci_GetParamValue(void* v, char* attr_name, int datatype, pObjData val);
static int ci_SetParamValue(void* v, char* attr_name, int datatype, pObjData val);
```

- `v : void*` is the object provided in `expAddParamToList()` (or a similar function).
- `attr_name : char*` is the string name for the requested attribute.
- `datatype : int` is the data type for the requested attribute.
- `val : pObjectData` is either a buffer in which to store the requested data (`ci_GetParamValue()`) or a buffer containing data that will be copied to the parameter `ci_SetParamValue()`.

All three of these functions return 0 for success, 1 if the attribute is `NULL`, or -1 if an error occurs.  The `expSetParamFunctions()` function returns 0 if the functions were set successfully, or -1 if an error occurs.

### expReverseEvalTree()
```c
int expReverseEvalTree(pExpression tree, pParamObjects objlist)l
```
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



## IX Module: MTSession 
The mtsession (MSS) module is used for session authentication, error reporting, and for storing session-wide variables such as the current date format, username, and password (used when issuing a login request to a remote server).  Care should be taken in the use of Centrallix that its coredump files are NOT in a world-readable location, as the password will be visible in the coredump file (or just ulimit the core file size to 0).
<!-- TODO: Greg - This feels like important security info that should be documented somewhere more obvious than burried at the end of a random paragraph at the start of section 9 in a 13 section document about driver authoring. -->

### mssInitialize()
```c
int mssInitialize(char* authmethod, char* authfile, char* logmethod, int logall, char* log_progname);
```
This function initializes the session manager and sets global variables used in this module.  It returns 0 if successful and -1 if an error occurs.

### mssUserName()
```c
char* mssUserName();
```
This function returns the current user name, or `NULL` an error occurs.

### mssPassword()
```c
char* mssPassword();
```
This function returns the current user's password that they used to log into Centrallix, or `NULL` an error occurs.

### mssSetParam()
```c
int mssSetParam(char* paramname, char* param);
```
This function sets the session parameter of the provided name (`paramname`) to the provided value (`param`).  The parameter MUST be a string value.  This function returns 0 if successful, or -1 an error occurs.

### mssGetParam()
```c
char* mssGetParam(char* paramname);
```
Returns the value of a session parameter of the provided name (`paramname`), or `NULL` if an error occurs.  Common session parameters include:
- `dfmt`: The current date format.
- `mfmt`: The current money format.
- `textsize`: The current max text size from a read of an object's content via `objGetAttrValue(obj, "objcontent", POD(&str))`

### mssError()
```c
int mssError(int clr, char* module, char* message, ...);
```
Formats and caches an error message for return to the user.  This function returns 0 if successful, or -1 if an error occurred.

| Parameter | Type          | Description
| --------- | ------------- | ------------
| crl       | int           | If set to 1, all previous error messages are cleared. Set this when the error is initially discovered and no other module is likely to have made a relevant `mssError()` call for the current error.
| module    | char*         | A two-to-five letter abbreviation of the module reporting the error.  This is typically the module or driver's abbreviation prefix in full uppercase letters (although that is not required).  This is intended to help the developer find the source of the error faster.
| message   | char*         | A string error message, accepting format specifiers like `%d` and `%s` which are supplied by the argument list, similar to `printf()`.
| ...       | ...           | Parameters for the formatting.

Errors that occur inside a session context are normally stored up and not printed until other MSS module routines are called to fetch the errors.  Errors occurring outside a session context (such as in Centrallix's network listener) are printed to Centrallix's standard output immediately.

The `mssError()` function is not required to be called at every function nesting level when an error occurs.  For example, if the expression compiler returns -1 indicating that a compilation error occurred, it has probably already added one or more error messages to the error list.  The calling function should only call `mssError()` if doing so would provide additional context or other useful information (e.g. _What_ expression failed compilation? _Why_ as an expression being compiled? etc.).  However, it is far easier to give too little information that too much, so it can often be best to air on the side of calling `mssError()` with information that might be irrelevant, rather than skipping it and leaving the developer confused.

- 📖 **Note**: The `mssError()` routines do not cause the calling function to return or exit.  The function must still clean up after itself and return an appropriate value (such as `-1` or `NULL`) to indicate failure.

- ⚠️ **Warning**: Even if `-1` is returned, the error message may still be sent to the user in some scenarios.  This is not guaranteed, though.

- ⚠️ **Warning**: `%d` and `%s` are the ONLY supported format specifier for this function.  **DO NOT** use any other format specifiers like `%lf`, `%u`, `%lu`, `%c` etc.  **DO NOT** attempt to include `%%` for a percent symbol in your error message, as misplaced percent symbols often break this function. If you wish to use these features of printf, it is recommended to print the error message to a buffer and pass that buffer to `mssError()`, as follows:
    ```c
    char err_buf[256];
    snprintf(err_buf, sizeof(err_buf),
	"Incorrect values detected: %u, %g (%lf), '%c'",
	unsigned_int_value, double_value, char_value
    );
    if (mssError(1, "EXMPL", "%s", err_buf) != 0)
	{
	fprintf(stderr, "ERROR! %s\n", err_buf);
	}
    return -1;
    ```


### mssErrorErrno()
```c
int mssErrorErrno(int clr, char* module, char* message, ...);
```
This function works the same way as [`mssError`](#mssError), except checks the current value of `errno` and includes a description of any error stored there.  This is useful if a system call or other library function is responsible for this error.




## X Path Handling Functions
The OSML provides a set of utility functions that make it easier to handle path structs when writing drivers.  Most of them are named `obj_internal_XxxYyy()` or similar.

### obj_internal_PathPart()
```c
char* obj_internal_PathPart(pPathname path, int start, int length);
```
The Pathname structure breaks down a pathname into path elements, which are text strings separated by the directory separator `'/'`.  This function takes the given Pathname structure and returns the number of path elements requested (using `length`) after skipping to the `start`th element (where element 0 is the starting `.` that begins any Centrallix path).

For example, given the path:
```bash
/apps/kardia/data/Kardia_DB/p_partner/rows/1
```
Centrallix stores the path internally as the following (see [Parsing Path Contents](#parsing-path-contents) and [Parameters](#parameters) above):
```bash
./apps/kardia/data/Kardia_DB/p_partner/rows/1
```
Thus, calling `obj_internal_PathPart(pathstruct, 4, 2);` will return `"Kardia_DB/p_partner"` because the `.` is the 0th element, making `Kardia_DB` the 4th element, and we have requested two elements.

- 📖 **Note**: The values returned from `obj_internal_PathPart()` use an internal buffer, so they are only valid until the next call to a PathPart function on the given pathname structure.

### obj_internal_AddToPath()
```c
int obj_internal_AddToPath(pPathname path, char* new_element);
```
This function lengthens the path by one element, adding new_element on to the end of the path.  This function is frequently useful for drivers in the QueryFetch routine where the new child object needs to be appended onto the end of the given path.

This function returns the index of the new element in the path on success, or a value less than 0 on failure.

### obj_internal_CopyPath()
```c
int obj_internal_CopyPath(pPathname dest, pPathname src);
```
This function copies a pathname structure from the `src` to the `dest`, returning 0 if successful or -1 if an error occurs.

### obj_internal_FreePathStruct()
```c
void obj_internal_FreePathStruct(pPathname path);
```
This function frees a pathname structure.



## XI Network Connection Functionality
Sometimes, a driver may need to initiate a network connection.  This can be done via the `MTASK` module, which provides simple and easy TCP/IP connectivity.  It includes many functions, only a few of which are documented below:

### netConnectTCP()
```c
pFile netConnectTCP(char* host_name, char* service_name, int flags);
```
This function creates a client socket and connects it to a server on a given TCP service/port and host name.  It takes the following three parameters:
- `host_name`: The host name or ascii string for the host's ip address.
- `service_name`: The name of the service (from `/etc/services`) or its numeric representation as a string.
- `flags`: Normally left 0.

- 📖 **Note**: The `NET_U_NOBLOCK` flag causes the function to return immediately even if the connection is still being established.  Further reads and writes will block until the connection either establishes or fails.

This function returns the connection file descriptor if successful, or `NULL` if an error occurs.

### netCloseTCP()
```c
int netCloseTCP(pFile net_filedesc, int linger_msec, int flags);
```
This function closes a network connection (either a TCP listening, server, or client socket).  It will also optionally waits up to `linger_msec` milliseconds (1/1000 seconds) for any data written to the connection to make it to the other end before performing the close.  If `linger_msec` is set to 0, the connection is aborted (reset).  The linger time can be set to 1000 msec or so if no writes were performed on the connection prior to the close.  If a large amount of writes were performed immediately prior to the close, offering to linger for a few more seconds (perhaps 5 or 10 by specifying 5000 or 10000 msec) can be a good idea.

### fdWrite()
```c
int fdWrite(pFile filedesc, char* buffer, int length, int offset, int flags);
```
This function writes data to an open file descriptor, from a given `buffer` and `length` of data to write.  It also takes an optional seek `offset` and and `flags`, which can be zero or more of:
- `FD_U_NOBLOCK` - If the write can't be performed immediately, don't perform it at all.
- `FD_U_SEEK` - The `offset` value is valid.  Seek to it before writing.  Not allowed for network connections.
- `FD_U_PACKET` - *ALL* of the data specified by `length` in `buffer` must be written.  Normal `write()` semantics in UNIX state that not all data has to be written, and the number of bytes actually written is returned.  Setting this flag makes sure all data is really written before returning.

### fdRead()
```c
int fdRead(pFile filedesc, char* buffer, int maxlen, int offset, int flags);
```
This function works the same as [`fdWrite()`](#fdwrite) except that it reads data instead of writing it.  It takes the same flags as above, except that `FD_U_PACKET` now requires that all of `maxlen` bytes must be read before returning.  This is good for reading a packet of a known length that might be broken up into fragments by the network (TCP/IP has a maximum frame transmission size of about 1450 bytes).


## XII Parsing Data
The mtlexer (MLX) module is a lexical analyzer library provided by Centrallix for parsing many types of data.  It can parse data from either a `pFile` descriptor or from a string value.  This lexical analyzer is also used by the [expression compiler](#viii-module-expression).  In simple terms, it's a very fancy string tokenizer.

### mlxOpenSession()
```c
pLxSession mlxOpenSession(pFile fd, int flags);
```
This function opens a lexer session, using a file descripter as its source.  Some of the more useful values for `flags` include:

| Value             | Description
| ----------------- | ------------
| `MLX_F_ICASEK`    | Automatically convert all keywords (non-quoted strings) to lowercase.
| `MLX_F_ICASER`    | Automatically convert all reserved words to lowercase.  This flag is highly recommended, and in some cases, required.
| `MLX_F_ICASE`     | Same as MLX_F_ICASER | MLX_F_ICASEK.
| `MLX_F_POUNDCOMM` | Respect # comment at the start of the line (`#comment`).
| `MLX_F_CCOMM`     | Respect c-style comments (`/*comment*/`).
| `MLX_F_CPPCOMM`   | Respect c-plus-plus comments (`//comment`).
| `MLX_F_SEMICOMM`  | Respect semicolon comments (`;comment`).
| `MLX_F_DASHCOMM`  | Respect double-dash comments (`--comment`).
| `MLX_F_EOL`       | Return end-of-line as a token.  Otherwise, this is considered whitespace.
| `MLX_F_EOF`       | Return end-of-file as a token.  Otherwise, reaching end of file is an error.
| `MLX_F_ALLOWNUL`  | Allow null characters (`'\0'`) in the input stream, which otherwise cause an error.  If this flag is set, the caller must ensure that null characters are handled safely.
| `MLX_F_IFSONLY`   | Only return string values separated by tabs, spaces, newlines, and carriage returns.  For example, normally the brace in `"this{brace"` is a token and that string will result in three tokens, but in `IFSONLY` mode it is just one token.
| `MLX_F_DASHKW`    | Keywords can include the dash (`-`).  Otherwise, the keyword is treated as two keywords with a minus sign between them.
| `MLX_F_FILENAMES` | Treat a non-quoted string beginning with a slash (`/`) or dot-slash (`./`) as a filename, and allow slashes and dots in the string without requiring quotes.
| `MLX_F_NODISCARD` | Attempt to unread unused buffered data rather than discarding it, allowing the calling function to continue reading with `fdRead()` or another lexer session after the last token is read and the session is closed.  The lexer `fdRead()`s in 2k or so chunks for performance, and normally discards this data when done, causing future file decriptors to start at an undefined file location.
| `MLX_F_DBLBRACE`  | Treat `{{` and `}}` as double brace tokens, not two single brace tokens.
| `MLX_F_NOUNESC`   | Do not remove escapes in strings.
| `MLX_F_SSTRING`   | Differentiate between strings values using `""` and `''`.

This function returns a pointer to the new lexer session if successful, or `NULL` if an error occurs.

### mlxStringSession()
```c
pLxSession mlxStringSession(char* str, int flags);
```
This function opens a lexer session, using a text string as its source.  The flags are the same as [`mlxOpenSession()`](#mlxopensession) above, except that `MLX_F_NODISCARD` has no effect.

This function returns a pointer to the new lexer session if successful, or `NULL` if an error occurs.

### mlxCloseSession()
```c
int mlxCloseSession(pLxSession this);
```
This function closes a lexer session, freeing all associated data.  This does not also close the file descriptor used to open the lexer session, as this is assumed to be managed by the caller.  This function returns 0 if successful, and -1 if an error occurs.

### mlxNextToken()
```c
int mlxNextToken(pLxSession this);
```
Returns the type of the next token in the token stream.  Valid token types are:

| Token                   | Required Flag     | Meaning                                     |
|-------------------------|-------------------|---------------------------------------------|
| `MLX_TOK_BEGIN`         | -                 | Beginning of the input stream.              |
| `MLX_TOK_STRING`        | -                 | String value, e.g. `"string"`.              |
| `MLX_TOK_INTEGER`       | -                 | Integer value, e.g. `42`.                   |
| `MLX_TOK_EQUALS`        | -                 | `=`                                         |
| `MLX_TOK_OPENBRACE`     | -                 | `{`                                         |
| `MLX_TOK_CLOSEBRACE`    | -                 | `}`                                         |
| `MLX_TOK_ERROR`         | -                 | An error has occurred.                      |
| `MLX_TOK_KEYWORD`       | -                 | A keyword (unquoted string).                |
| `MLX_TOK_COMMA`         | -                 | `,`                                         |
| `MLX_TOK_EOL`           | `MLX_F_EOL`       | End-of-line.                                |
| `MLX_TOK_EOF`           | `MLX_F_EOF`       | End-of-file reached.                        |
| `MLX_TOK_COMPARE`       | -                 | `<>` `!=` `<` `>` `>=` `<=` `==`            |
| `MLX_TOK_COLON`         | -                 | `:`                                         |
| `MLX_TOK_OPENPAREN`     | -                 | `(`                                         |
| `MLX_TOK_CLOSEPAREN`    | -                 | `)`                                         |
| `MLX_TOK_SLASH`         | -                 | `/`                                         |
| `MLX_TOK_PERIOD`        | -                 | `.`                                         |
| `MLX_TOK_PLUS`          | -                 | `+`                                         |
| `MLX_TOK_ASTERISK`      | -                 | `*`                                         |
| `MLX_TOK_RESERVEDWD`    | -                 | Reserved word (special keyword).            |
| `MLX_TOK_FILENAME`      | `MLX_F_FILENAMES` | Unquoted string starting with / or ./       |
| `MLX_TOK_DOUBLE`        | -                 | Double precision floating point.            |
| `MLX_TOK_DOLLAR`        | -                 | `$`                                         |
| `MLX_TOK_MINUS`         | -                 | `-`                                         |
| `MLX_TOK_DBLOPENBRACE`  | `MLX_F_DBLBRACE`  | `{{`                                        |
| `MLX_TOK_DBLCLOSEBRACE` | `MLX_F_DBLBRACE`  | `}}`                                        |
| `MLX_TOK_SYMBOL`        | -                 | `+-=.,<>` etc.                              |
| `MLX_TOK_SEMICOLON`     | -                 | `;`                                         |
| `MLX_TOK_SSTRING`       | `MLX_F_SSTRING`   | Single quote string value, e.g. `'string'`. |
| `MLX_TOK_POUND`         | -                 | `#`                                         |
| `MLX_TOK_MAX`           | -                 | Max token value (internal).                 |

### mlxStringVal()
```c
char* mlxStringVal(pLxSession this, int* alloc);
```
This function gets the string value of the current token.  If `alloc` is `NULL`, only the first 255 bytes of the string will be returned, and the rest will be discarded.  If `alloc` is non-null and set to 0, the routine will set `alloc` to 1 if it needed to allocate memory for a very long string, otherwise leave it as 0.  If `alloc` is non-null and set to 1, this routine will _always allocate memory for the string, whether long or short.

This routine works no matter what the token type, and returns a string representation of the token if not `MLX_TOK_STRING`.

This routine MAY NOT be called twice for the same token.

- ⚠️ **Warning**: This function should not be called when `MLX_F_ALLOWNUL` is being used because it may return a null character, giving the caller no way to know whether it is the null-terminator or it simply existed in the input data stream.  In this case, `mlxCopyToken()` should be used instead, as it gives a definitive answer on the token length.  (`mlxStringVal()` can still be used on keywords, though, since they never contain a null, by definition).

### mlxIntVal()
```c
int mlxIntVal(pLxSession this);
```
This function returns the integer value of `MLX_TOK_INTEGER` tokens, or returns the compare type for `MLX_TOK_COMPARE` tokens.  The compare type is a bitmask of the `MLX_CMP_EQUALS`, `MLX_CMP_GREATER`, and `MLX_CMP_LESS` flags.  For `MLX_TOK_DOUBLE` tokens, this function returns the whole part.

### mlxDoubleVal()
```c
double mlxDoubleVal(pLxSession this);
```
This function returns a double precision floating point number for either `MLX_TOK_INTEGER` or `MLX_TOK_DOUBLE` values.

### mlxCopyToken()
```c
int mlxCopyToken(pLxSession this, char* buffer, int maxlen);
```
This function copies the contents of the current token to a string buffer, up to `maxlen` characters.  It should be used instead of `mlxStringVal()`, _especially_ where null characters may be involved.  This function returns the number of characters copied on success, or -1 on failure, and it can be called multiple times if more data needs to be read from the same token.

### mlxHoldToken()
```c
int mlxHoldToken(pLxSession this);
```
This function "puts back" a token, causing the next `mlxNextToken()` to return the current token again.  This is useful when a function realizes after `mlxNextToken()` that it has read one-too-many.  This function returns 0 on success, or -1 if an error occurs.

### mlxSetOptions()
```c
int mlxSetOptions(pLxSession this, int options);
```
This function sets the options (`MLX_F_xxx`) for an active lexer session.  The options that are valid here are `MLX_F_ICASE` and `MLX_F_IFSONLY`.  This function returns 0 if successful, or -1 if an error occurs.

### mlxUnsetOptions()
```c
int mlxUnsetOptions(pLxSession this, int options);
```
Clears options set by [`mlxSetOptions()`](#mlxsetoptions).  This function returns 0 if successful, or -1 if an error occurs.

### mlxSetReservedWords()
```c
int mlxSetReservedWords(pLxSession this, char** res_words);
```
This function sets the lexer to return the list of `res_words` as `MLX_TOK_RESERVEDWD` tokens instead of `MLX_TOK_KEYWORD` tokens.  The list of words should be an array of character strings, with the last string in the list being `NULL`.  This function returns 0 if successful, or -1 if an error occurs.

- ⚠️ **Warning**: `mtlexer` does not copy this list! Ensure that it has a lifetime longer than that of the lexer session.

### mlxNoteError()
```c
int mlxNoteError(pLxSession this);
```
This function generates an `mssError()` message of the form:
```bash
MLX:  Error near '<token-string>'
```

- 📖 **Note**: The calling routine may have detected the error long after the actual place where it occurred.  The MLX module just tries to come close :)

### mlxNotePosition()
```c
int mlxNotePosition(pLxSession this);
```
This function generates an mssError() message of this form:
```bash
MLX:  Error at line ##
```

- 📖 **Note**:  If using a `StringSession` instead of a `pFile` session, this may not be accurate, as the string may have come from the middle of a file somewhere.  Use with care.



## XIII Driver Testing
This section contains a list of things that can be done to test an objectsystem driver and ensure that it preforms all basic operations correctly, using the [test_obj command line interface](http://www.centrallix.net/docs/docs.php).

It is strongly recommended to test for invalid reads, writes, frees, and memory leaks during each of these by watching memory utilization using nmDeltas() during repetitive operations (e.g., nmDeltas(), open, close, nmDeltas(), open, close, and then nmDeltas() again).

Testing for more general memory bugs using the "valgrind" tool is also strongly encouraged, via running these various tests in test_obj while test_obj is running under valgrind.  To properly test under Valgrind, centrallix-lib must be compiled with the configure flag `--enable-valgrind-integration` turned on.  This disables `nmMalloc()` block caching (so that valgrind can properly detect memory leaks and free memory reuse), and it provides better information to valgrind's analyzer regarding MTASK threads.

Magic number checking on data structures is encouraged.  To use magic number checking, determine a magic number value for each of your structures, and add a #define for that constant in your code.  The magic number should be a 32-bit integer, possibly with 0x00 in either the 2nd or 3rd byte of the integer.  Many existing magic number values can be found in [magic.h](../centrallix-lib/include/magic.h).  The 32-bit integer is placed as the first element of the structure, and set using the `SETMAGIC()` macro, then tested using the macros `ASSERTMAGIC()` macro or, less commonly, `ASSERTNOTMAGIC()`. Common times to `ASSERTMAGIC()` include:
- Any time a pointer to the structure crosses an interface boundary.
- At the entry to internal methods/functions.
- When traversing linked lists of data structures.
- When retrieving data structures from an array.
- etc.

When used in conjunction with `nmMalloc()` and `nmFree()`, `ASSERTMAGIC` also helps to detect the reuse of already-freed memory, since `nmFree()` tags the first four bytes of the memory block with the constant `MGK_FREEMEM`. `nmFree()` also looks for the constant `MGK_FREEMEM` in the magic number slot to detect already-freed memory. (**DO NOT** use that constant for your own magic numbers!)

The term "**MUST**", as used here, means that the driver will likely cause problems if the functionality is not present.

The term "**SHOULD**" indicates behavior which is desirable, but that might not cause immediate problems if not fully implemented.

The term "**MAY**" refers to optional, but permissible, behavior.


### A. Opening, closing, creating, and deleting

1.  Any object in the driver's subtree, including the node object itself, MUST be able to be opened using `xxxOpen()` and then closed using `xxxClose()`.  Although it does more than just open and close, the "show" command in test_obj can be useful for testing this.

2.  Objects MUST be able to be opened regardless of the location of the node object in the ObjectSystem.  For example, don't just test the driver with the node object in the top level directory of the ObjectSystem - also try it in other subdirectories.

3.  New objects within the driver's subtree SHOULD be able to be created using `xxxOpen()` with `OBJ_O_CREAT`, or using `objCreate()`.  The flags `OBJ_O_EXCL` and `OBJ_O_TRUNC` should also be supported, where meaningful.

4.  Where possible, `OBJ_O_AUTONAME` should be supported on object creation.  With this, the name of the object will be set to `*` in the pathname structure, and `OBJ_O_CREAT` will also be set.  The driver should automatically determine a suitable "name" for the object, and subsequent calls to objGetAttrValue on "name" should return the determined name.  A driver MAY choose to return NULL for "name" until after certain object properties have been set and an `xxxCommit()` operation performed.  A driver MUST NOT return `*` for the object name unless `*` is truly the name chosen for the object.

5.  A driver SHOULD support deletion of any object in its subtree with the exception of the node object itself.  Deletion may be done directly with `xxxDelete()`, or on an already-open object using `xxxDeleteObj()`.  A driver MAY refuse to delete an object if the object still contains deletable sub-objects.  Some objects in the subtree might inherently not be deletable apart from the parent objects of said objects.  In those cases, deletion should not succeed.


### B. Attributes

1.  The driver MUST NOT return system attributes (name, inner_type, etc) when enumerating with `xxxGetFirst()`/`xxxNextAttr()`.

2.  The driver MAY choose not to handle `xxxGetAttrType` on the system attributes.  The OSML handles this.

3.  The driver SHOULD support the attribute `last_modification` if at all reasonable.  Not all objects can have this property however.

4.  The driver SHOULD support the attribute "annotation" if reasonable to do so.  Database drivers should have a configurable "row annotation expression" to auto-generate annotations from existing row content, where reasonable.  The driver MAY permit the user to directly set annotation values.  The driver MUST return an empty string ("") for any annotation values that are unavailable.

5.  Drivers MUST coherently name EVERY object.  Names MUST be unique in any given "directory".

6.  Drivers MAY choose to omit some attributes from normal attribute enumeration.

7.  The "show" command in test_obj is a good way to display a list of attributes for an object.

8.  Attribute enumeration, retrieval, and modification MUST work equally well on objects returned by `xxxOpen()` and objects returned by `xxxQueryFetch()`.

9.  If a driver returns an attribute during attribute enumeration, then that attribute MUST return a valid type via `xxxGetAttrType`.

10. A driver MUST return -1 and error with a "type mismatch" type of error from `xxxGetAttrValue()`/`xxxSetAttrValue()`, if the data type is inappropriate.

11. A driver MAY choose to perform auto-conversion of data types on certain attributes, but SHOULD NOT perform such auto conversion on a widespread wholesale basis.

12. A driver MAY support the `DATA_T_CODE` attribute data type.

13. Drivers MAY support `DATA_T_INTVEC` and `DATA_T_STRINGVEC`.

14. Drivers MAY support `xxxAddAttr()` and `xxxOpenAttr()`.

15. Drivers MAY support methods on objects.  Objects without any methods should be indicated by a `NULL` return value from the method enumeration functions.

16. When returning attribute values, the value MUST remain valid at least until the next call to `xxxGetAttrValue()`, `xxxSetAttrValue()`, or `xxxGetAttrType()`, or until the object is closed, whichever occurs first.  Drivers MUST NOT require the caller to free attribute memory.

17. When `xxxSetAttrValue()` is used, drivers MUST NOT depend on the referenced value (in the POD) being valid past the end of the call to `xxxSetAttrValue()`.


### C. Querying Subobjects

1.  If an object cannot support queries for subobjects, `xxxOpenQuery()` call SHOULD fail.

2.  If an object can support the existence of subobjects, but has no subobjects, the `xxxOpenQuery()` should succeed, but calls to `xxxQueryFetch()` MUST return `NULL`.

3.  Objects returned by `xxxQueryFetch()` MUST remain valid even after the query is closed using `xxxQueryClose()`.

4.  Objects returned by `xxxQueryFetch()` MUST also be able to be passed to `xxxOpenQuery()` to check for the existence of further subobjects, though the `xxxOpenQuery()` call is permitted to fail as in (C)(1) above.

5.  Any name returned by `xxxGetAttrValue(name)` on a queried subobject MUST be usable to open the same object using `xxxOpen()`.

6.  Drivers which connect to resources which are able to perform sorting and/or selection (filtering) of records or objects SHOULD use the [`OBJ_QY_F_FULLSORT`](#function-openquery) and [`OBJ_QY_F_FULLQUERY`](#function-openquery) flags.  Further, they SHOULD pass on the sorting and filtering expressions to the remote resource so that resource can optimize sorting and/or filtering as needed.

7.  If the driver's remote resource can filter and/or sort, but can only do so imperfectly (e.g., the resource cannot handle the potential complexity of all sorting/selection expressions, but can handle parts of them), then `OBJ_QY_F_FULLSORT` and/or `OBJ_QY_F_FULL`- QUERY MUST NOT be used.  However the remote resource MAY still provide partial sorting and/or selection of data.

8.  Drivers SHOULD NOT use `OBJ_QY_F_FULLSORT` and `OBJ_QY_F_FULLQUERY` if there is no advantage to letting the resource perform these operations (usually, however, if the resource provides such functionality, there is advantage to letting the resource perform those operations.  However, the coding burden to provide the filtering and sorting expressions to the resource, and in the correct format for the resource, may be not worth the work).

9.  Testing of query functionality can be done via test_obj's "query", "csv", and "ls" (or "list") commands.  To test for nested querying of objects returned from QueryFetch, a SUBTREE select can be used with the "query" or "csv" commands.

10. Drivers which support full sorting or full querying MUST be able to handle the attribute "name" in the expression tree for the sort or query criteria.  The "name" attribute SHOULD be mapped to an expression which reflects how "name" is constructed for objects, such as changing "name" to "convert(varchar, prikeyfield1) + '|' + convert(varchar, prikeyfield2)" or whatever is appropriate.
