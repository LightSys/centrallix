#ifndef _OBJ_H
#define _OBJ_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj.h,v 1.40 2008/04/06 20:37:36 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/obj.h,v $

    $Log: obj.h,v $
    Revision 1.40  2008/04/06 20:37:36  gbeeley
    - (change) adding obj_internal_FreePathStruct() to deinitialize a pathname
      structure without actually freeing it (e.g. if the pathname structure is
      a part of a larger structure).

    Revision 1.39  2008/02/25 23:14:33  gbeeley
    - (feature) SQL Subquery support in all expressions (both inside and
      outside of actual queries).  Limitations:  subqueries in an actual
      SQL statement are not optimized; subqueries resulting in a list
      rather than a scalar are not handled (only the first field of the
      first row in the subquery result is actually used).
    - (feature) Passing parameters to objMultiQuery() via an object list
      is now supported (was needed for subquery support).  This is supported
      in the report writer to simplify dynamic SQL query construction.
    - (change) objMultiQuery() interface changed to accept third parameter.
    - (change) expPodToExpression() interface changed to accept third param
      in order to (possibly) copy to an already existing expression node.

    Revision 1.38  2007/06/09 19:53:46  gbeeley
    - (feature) adding objGetPathname() method to return the full path of an
      object that is already open.

    Revision 1.37  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.36  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.35  2007/03/21 04:48:09  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.

    Revision 1.34  2007/03/01 21:54:05  gbeeley
    - (feature) stub out code for 'createonly' presentation hints style flag

    Revision 1.33  2006/11/16 20:15:54  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.32  2005/09/24 20:15:42  gbeeley
    - Adding objAddVirtualAttr() to the OSML API, which can be used to add
      an attribute to an object which invokes callback functions to get the
      attribute values, etc.
    - Changing objLinkTo() to return the linked-to object (same thing that
      got passed in, but good for style in reference counting).
    - Cleanup of some memory leak issues in objOpenQuery()

    Revision 1.31  2005/02/26 06:42:38  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.30  2004/07/20 21:28:52  mmcgill
    *   ht_render
        -   Added code to perform verification of widget-tree prior to
            rendering.
        -   Added concept of 'pseudo-types' for widget-drivers, e.g. the
            table driver getting called for 'table-column' widgets. This is
            necessary now since the 'table-column' entry in an app file will
            actually get put into its own widget node. Pseudo-type names
            are stored in an XArray in the driver struct during the
            xxxInitialize() function of the driver, and BEFORE ANY CALLS TO
            htrAddSupport().
        -   Added htrLookupDriver() to encapsulate the process of looking up
            a driver given an HtSession and widget type
        -   Added 'pWgtrVerifySession VerifySession' to HtSession.
            WgtrVerifySession represents a 'verification context' to be used
            by the xxxVerify functions in the widget drivers to schedule new
            widgets for verification, and otherwise interact with the
            verification system.
    *   xxxVerify() functions now take a pHtSession parameter.
    *   Updated the dropdown, tab, and table widgets to register their
        pseudo-types
    *   Moved the ObjProperty out of obj.h and into wgtr.c to internalize it,
        in anticipation of converting the Wgtr module to use PTODs instead.
    *   Fixed some Wgtr module memory-leak issues
    *   Added functions wgtrScheduleVerify() and wgtrCancelVerify(). They are
        to be used in the xxxVerify() functions when a node has been
        dynamically added to the widget tree during tree verification.
    *   Added the formbar widget driver, as a demonstration of how to modify
        the widget-tree during the verification process. The formbar widget
        doesn't actually do anything during the rendering process excpet
        call htrRenderWidget on its subwidgets, but during Verify it adds
        all the widgets necessary to reproduce the 'form control pane' from
        ors.app. This will eventually be done even more efficiently with
        component widgets - this serves as a tech test.

    Revision 1.29  2004/07/19 15:30:42  mmcgill
    The DHTML generation system has been updated from the 2-step process to
    a three-step process:
        1)	Upon request for an application, a widget-tree is built from the
    	app file requested.
        2)	The tree is Verified (not actually implemented yet, since none of
    	the widget drivers have proper Verify() functions - but it's only
    	a matter of a function call in net_http.c)
        3)	The widget drivers are called on their respective parts of the
    	tree structure to generate the DHTML code, which is then sent to
    	the user.

    To support widget tree generation the WGTR module has been added. This
    module allows OSML objects to be parsed into widget-trees. The module
    also provides an API for building widget-trees from scratch, and for
    manipulating existing widget-trees.

    The Render functions of all widget drivers have been updated to make their
    calls to the WGTR module, rather than the OSML, and to take a pWgtrNode
    instead of a pObject as a parameter.

    net_internal_GET() in net_http.c has been updated to call
    wgtrParseOpenObject() to make a tree, pass that tree to htrRender(), and
    then free it.

    htrRender() in ht_render.c has been updated to take a pWgtrNode instead of
    a pObject parameter, and to make calls through the WGTR module instead of
    the OSML where appropriate. htrRenderWidget(), htrRenderSubwidgets(),
    htrGetBoolean(), etc. have also been modified appropriately.

    I have assumed in each widget driver that w_obj->Session is equivelent to
    s->ObjSession; in other words, that the object being passed in to the
    Render() function was opened via the session being passed in with the
    HtSession parameter. To my understanding this is a valid assumption.

    While I did run through the test apps and all appears to be well, it is
    possible that some bugs were introduced as a result of the modifications to
    all 30 widget drivers. If you find at any point that things are acting
    funny, that would be a good place to check.

    Revision 1.28  2004/06/22 16:06:53  mmcgill
    Added the flag OBJ_F_NOCACHE, which a driver can set in its xxxOpen call
    to tell OSML not to add the opened object to the Directory Cache.

    Revision 1.27  2004/06/12 04:02:27  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.26  2004/06/12 00:10:14  mmcgill
    Chalk one up under 'didn't understand the build process'. The remaining
    os drivers have been updated, and the prototype for objExecuteMethod
    in obj.h has been changed to match the changes made everywhere it's
    called - param is now of type pObjData, not void*.

    Revision 1.25  2004/05/07 01:18:23  gbeeley
    - support for StyleMask addition to the Style hint, which allows the
      determination of which hints have been set, not just whether they are on
      and off.  Also added 'negative' styles, e.g., 'allownull' vs 'notnull'.

    Revision 1.24  2004/02/24 20:11:01  gbeeley
    - fixing some date/time related problems
    - efficiency improvement for net_http allowing browser to actually
      cache .js files and images.

    Revision 1.23  2003/11/12 22:21:39  gbeeley
    - addition of delete support to osml, mq, datafile, and ux modules
    - added objDeleteObj() API call which will replace objDelete()
    - stparse now allows strings as well as keywords for object names
    - sanity check - old rpt driver to make sure it isn't in the build

    Revision 1.22  2003/05/30 17:58:26  gbeeley
    - turned off OSML API debugging
    - fixed bug in WriteOneAttr() that was truncating a string

    Revision 1.21  2003/05/30 17:39:50  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.20  2003/04/25 05:06:56  gbeeley
    Added insert support to OSML-over-HTTP, and very remedial Trx support
    with the objCommit API method and Commit osdriver method.  CSV datafile
    driver is the only driver supporting it at present.

    Revision 1.19  2003/04/25 04:09:28  gbeeley
    Adding insert and autokeying support to OSML and to CSV datafile
    driver on a limited basis (in rowidkey mode only, which is the only
    mode currently supported by the csv driver).

    Revision 1.18  2003/04/25 02:43:27  gbeeley
    Fixed some object open nuances with node object caching where a cached
    object might be open readonly but we would need read/write.  Added a
    xhandle-based session identifier for future use by objdrivers.

    Revision 1.17  2003/04/24 19:28:10  gbeeley
    Moved the OSML open node object cache to the session level rather than
    global.  Otherwise, the open node objects could be accessed by the
    wrong user in the wrong session context, which is, er, "bad".

    Revision 1.16  2003/04/24 03:10:00  gbeeley
    Adding AllowChars and BadChars to presentation hints base
    implementation.

    Revision 1.15  2003/04/04 05:02:43  gbeeley
    Added more flags to objInfo dealing with content and seekability.
    Added objInfo capability to objdrv_struct.

    Revision 1.14  2003/03/31 23:23:38  gbeeley
    Added facility to get additional data about an object, particularly
    with regard to its ability to have subobjects.  Added the feature at
    the driver level to objdrv_ux, and to the "show" command in test_obj.

    Revision 1.13  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.12  2002/08/10 02:09:44  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

    Revision 1.11  2002/06/19 23:27:36  gbeeley
    Added a few more presentations hints options.

    Revision 1.10  2002/06/09 23:44:47  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.9  2002/05/03 03:51:21  gbeeley
    Added objUnmanageObject() and objUnmanageQuery() which cause an object
    or query to not be closed automatically on session close.  This should
    NEVER be used with the intent of keeping an object or query open after
    session close, but rather it is used when the object or query would be
    closed in some other way, such as 'hidden' objects and queries that the
    multiquery layer opens behind the scenes (closing the multiquery objects
    and queries will cause the underlying ones to be closed).
    Also fixed some problems in the OSML where some objects and queries
    were not properly being added to the session's open objects and open
    queries lists.

    Revision 1.8  2002/04/25 17:59:59  gbeeley
    Added better magic number support in the OSML API.  ObjQuery and
    ObjSession structures are now protected with magic numbers, and
    support for magic numbers in Object structures has been improved
    a bit.

    Revision 1.7  2002/04/25 04:26:07  gbeeley
    Basic overhaul of objdrv_sybase to fix some security issues, improve
    robustness with key data in particular, and so forth.  Added a new
    flag to the objDataToString functions: DATA_F_SYBQUOTE, which quotes
    strings like Sybase wants them quoted (using a pair of quote marks to
    escape a lone quote mark).

    Revision 1.6  2002/02/21 06:32:54  jorupp
    updated obj_rootnode.c to use supplied rootnode
    removed warning in obj.h

    Revision 1.5  2002/02/14 00:55:20  gbeeley
    Added configuration file centrallix.conf capability.  You now MUST have
    this file installed, default is /usr/local/etc/centrallix.conf, in order
    to use Centrallix.  A sample centrallix.conf is found in the centrallix-os
    package in the "doc/install" directory.  Conf file allows specification of
    file locations, TCP port, server string, auth realm, auth method, and log
    method.  rootnode.type is now an attribute in the conf file instead of
    being a separate file, and thus is no longer used.

    Revision 1.4  2001/10/16 23:53:01  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.3  2001/10/02 15:43:13  gbeeley
    Updated data type conversion functions.  Converting to string now can
    properly escape quotes in the string.  Converting double to string now
    formats the double a bit better.

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:00:53  gbeeley
    Centrallix Core initial import

    Revision 1.3  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.2  2001/08/07 02:46:38  gbeeley
    Fixed library makefile issue and objInitialize() prototype.

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "cxlib/xhashqueue.h"
#include "cxlib/mtask.h"
#include "cxlib/datatypes.h"
#include "stparse_ne.h"
#include "cxlib/newmalloc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "cxlib/xhandle.h"
#include "ptod.h"

#define OBJSYS_DEFAULT_ROOTNODE		"/usr/local/etc/rootnode"
#define OBJSYS_DEFAULT_ROOTTYPE		"/usr/local/etc/rootnode.type"
#define OBJSYS_DEFAULT_TYPES_CFG	"/usr/local/etc/types.cfg"

#define OBJSYS_MAX_PATH		256
#define OBJSYS_MAX_ELEMENTS	16
#define OBJSYS_MAX_ATTR		64

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define OBJSYS_NOT_ISA		(0x80000000)

#define OBJ_U_SEEK	FD_U_SEEK
#define OBJ_U_PACKET	FD_U_PACKET

/** Pathname analysis structure **/
typedef struct
    {
    char		Pathbuf[OBJSYS_MAX_PATH];
    int			nElements;
    char*		Elements[OBJSYS_MAX_ELEMENTS];
    char*		OpenCtlBuf;
    int			OpenCtlLen;
    int			OpenCtlCnt;
    pStruct		OpenCtl[OBJSYS_MAX_ELEMENTS];
    int			LinkCnt;
    }
    Pathname, *pPathname;

extern char* obj_type_names[];

extern char* obj_short_months[];
extern char* obj_long_months[];
extern unsigned char obj_month_days[];
extern char* obj_short_week[];
extern char* obj_long_week[];
extern char* obj_default_date_fmt;
extern char* obj_default_money_fmt;
extern char* obj_default_null_fmt;

#define IS_LEAP_YEAR(x) ((x)%4 == 0 && ((x)%100 != 0 || (x)%400 == 0))

/** Datatype conversion functions - flags **/
#define DATA_F_QUOTED		1
#define DATA_F_SINGLE		2
#define DATA_F_NORMALIZE	4
#define DATA_F_SYBQUOTE		8	/* use '' to quote a ', etc */
#define DATA_F_CONVSPECIAL	16	/* convert literal CR LF and TAB to \r \n and \t */


/** Presentation Hints structure ---
 ** Any fields can be NULL to indicate that the hint doesn't apply.
 ** EnumQuery must return one or two columns: a data column followed by an
 ** optional description column.  Bitmasked fields can have a maximum of
 ** 32 distinct bits; the data column should be an integer from 0 to 31.
 **/
typedef struct _PH
    {
    void*		Constraint;	/* expression controlling what are valid values */
    void*		DefaultExpr;	/* expression defining the default value. */
    void*		MinValue;	/* minimum value expression */
    void*		MaxValue;	/* maximum value expression */
    XArray		EnumList;	/* list of string values */
    char*		EnumQuery;	/* query to enumerate the possible values */
    char*		Format;		/* presentation format - datetime or money */
    char*		AllowChars;	/* characters allowed in this string, if null all allowed */
    char*		BadChars;	/* characters *not* allowed in this string */
    int			Length;		/* Max length of data that can be entered */
    int			VisualLength;	/* length of data field as presented to user */
    int			VisualLength2;	/* used primarily for # lines in a multiline edit */
    unsigned int	BitmaskRO;	/* which bits, if any, in bitmask are read-only */
    int			Style;		/* style information - bitmask OBJ_PH_STYLE_xxx */
    int			StyleMask;	/* which style information is actually supplied */
    int			GroupID;	/* for grouping fields together.  -1 if not grouped */
    char*		GroupName;	/* Name of group, or NULL if no group or group named elsewhere */
    int			OrderID;	/* allows for reordering the fields (e.g., tab order) */
    char*		FriendlyName;	/* Presented 'friendly' name of this attribute */
    }
    ObjPresentationHints, *pObjPresentationHints;

/*** Hints style info - keep in sync with ht_utils_hints.js in centrallix-os ***/
#define OBJ_PH_STYLE_BITMASK	1	/* items from EnumQuery or EnumList are bitmasked */
#define OBJ_PH_STYLE_LIST	2	/* use a list style presentation for enum types */
#define OBJ_PH_STYLE_BUTTONS	4	/* use radio button or checkboxes for enum types */
#define OBJ_PH_STYLE_NOTNULL	8	/* field does not allow nulls */
#define OBJ_PH_STYLE_STRNULL	16	/* empty string == null */
#define OBJ_PH_STYLE_GROUPED	32	/* check GroupID for grouping fields together */
#define OBJ_PH_STYLE_READONLY	64	/* user can't modify */
#define OBJ_PH_STYLE_HIDDEN	128	/* don't present this field to the user */
#define OBJ_PH_STYLE_PASSWORD	256	/* hide string as user types */
#define OBJ_PH_STYLE_MULTILINE	512	/* string value allows multiline editing */
#define OBJ_PH_STYLE_HIGHLIGHT	1024	/* highlight this attribute */
#define OBJ_PH_STYLE_LOWERCASE	2048	/* This attribute is lowercase-only */
#define OBJ_PH_STYLE_UPPERCASE	4096	/* This attribute is uppercase-only */
#define OBJ_PH_STYLE_TABPAGE	8192	/* Prefer tabpage layout for grouped fields */
#define OBJ_PH_STYLE_SEPWINDOW	16384	/* Prefer separate windows for grouped fields */
#define OBJ_PH_STYLE_ALWAYSDEF	32768	/* Always reset default value on any modify */
#define OBJ_PH_STYLE_CREATEONLY	65536	/* Writable only during record creation */
#define OBJ_PH_STYLE_MULTISEL	131072	/* Multiple select */


/** objectsystem driver **/
typedef struct _OSD
    {
    char	Name[64];
    XArray	RootContentTypes;
    int		Capabilities;
    void*	(*Open)();
    int		(*Close)();
    int		(*Create)();
    int		(*Delete)();
    int		(*DeleteObj)();
    void*	(*OpenQuery)();
    int		(*QueryDelete)();
    void*	(*QueryFetch)();
    void*	(*QueryCreate)();
    int		(*QueryClose)();
    int		(*Read)();
    int		(*Write)();
    int		(*GetAttrType)();
    int		(*GetAttrValue)();
    char*	(*GetFirstAttr)();
    char*	(*GetNextAttr)();
    int		(*SetAttrValue)();
    int		(*AddAttr)();
    void*	(*OpenAttr)();
    char*	(*GetFirstMethod)();
    char*	(*GetNextMethod)();
    int		(*ExecuteMethod)();
    pObjPresentationHints (*PresentationHints)();
    int		(*Info)();
    int		(*Commit)();
    }
    ObjDriver, *pObjDriver;

#define OBJDRV_C_FULLQUERY	1
#define OBJDRV_C_TRANS		2
#define OBJDRV_C_LLQUERY	4
#define OBJDRV_C_ISTRANS	8
#define OBJDRV_C_ISMULTIQUERY	16
#define OBJDRV_C_INHERIT	32	/* driver desires inheritance support */
#define OBJDRV_C_ISINHERIT	64	/* driver is the inheritance layer */
#define OBJDRV_C_OUTERTYPE	128	/* driver layering depends on outer, not inner, type */
#define OBJDRV_C_NOAUTO		256	/* driver should never be automatically invoked */

/** objxact transaction tree **/
typedef struct _OT
    {
    int			Status;		/* OXT_S_xxx */
    int			OpType;		/* OXT_OP_xxx */
    struct _OT*		Parent;		/* Parent tree node */
    struct _OT*		Parallel;	/* Linked-list stack of parallel ops */
    struct _OT*		Next;		/* Next link in time-ordered completion */
    struct _OT*		Prev;		/* With the above; doubly-linked */
    XArray		Children;	/* Child tree nodes */
    void*		Object;		/* pObject pointer */
    int			AllocObj;	/* if object was allocated for this */
    char		UsrType[80];	/* User requested create type */
    int			Mask;		/* User requested create mask */
    pObjDriver		LLDriver;
    void*		LLParam;
    int			LinkCnt;
    char*		PathPtr;
    char		AttrName[64];
    int			AttrType;
    void*		AttrValue;
    }
    ObjTrxTree, *pObjTrxTree;



/** objectsystem session **/
typedef struct _OSS
    {
    int			Magic;
    char		CurrentDirectory[OBJSYS_MAX_PATH];
    XArray		OpenObjects;
    XArray		OpenQueries;
    pObjTrxTree		Trx;
    XHashQueue		DirectoryCache;		/* directory entry cache */
    handle_t		Handle;
    }
    ObjSession, *pObjSession;

#define OXT_S_PENDING	0
#define OXT_S_VISITED	1
#define OXT_S_COMPLETE	2
#define OXT_S_FAILED	3

#define OXT_OP_NONE	0
#define OXT_OP_CREATE	1
#define OXT_OP_SETATTR	2
#define OXT_OP_DELETE	3


/** Content type descriptor **/
typedef struct _CT
    {
    char	Name[64];
    char	Description[256];
    XArray	Extensions;
    XArray	IsA;
    int		Flags;
    XArray	RelatedTypes;
    XArray	RelationLevels;
    void*	TypeNameObjList;	/* pParamObjects */
    void*	TypeNameExpression;	/* pExpression */
    }
    ContentType, *pContentType;

#define CT_F_HASCONTENT		1
#define CT_F_HASCHILDREN	2
#define CT_F_TOPLEVEL		4

/** object additional information / capabilities **/
typedef struct _OA
    {
    int		Flags;		/* OBJ_INFO_F_xxx, below */
    int		nSubobjects;	/* count of subobjects, if known */
    }
    ObjectInfo, *pObjectInfo;

/** info flags **/
#define	OBJ_INFO_F_NO_SUBOBJ		(1<<0)	/* object has no subobjects */
#define OBJ_INFO_F_HAS_SUBOBJ		(1<<1)	/* object has at least one subobject */
#define OBJ_INFO_F_CAN_HAVE_SUBOBJ	(1<<2)	/* object *can* have subobjects */
#define OBJ_INFO_F_CANT_HAVE_SUBOBJ	(1<<3)	/* object *cannot* have subobjects */
#define OBJ_INFO_F_SUBOBJ_CNT_KNOWN	(1<<4)	/* number of subobjects is known */
#define OBJ_INFO_F_CAN_ADD_ATTR		(1<<5)	/* attributes can be added to object */
#define OBJ_INFO_F_CANT_ADD_ATTR	(1<<6)	/* attributes cannot be added to object */
#define OBJ_INFO_F_CAN_SEEK_FULL	(1<<7)	/* OBJ_U_SEEK fully supported on object */
#define OBJ_INFO_F_CAN_SEEK_REWIND	(1<<8)	/* OBJ_U_SEEK supported with '0' offset */
#define OBJ_INFO_F_CANT_SEEK		(1<<9)	/* OBJ_U_SEEK never honored */
#define OBJ_INFO_F_CAN_HAVE_CONTENT	(1<<10)	/* object can have content */
#define OBJ_INFO_F_CANT_HAVE_CONTENT	(1<<11)	/* object cannot have content */
#define OBJ_INFO_F_HAS_CONTENT		(1<<12)	/* object actually has content, even if zero-length */
#define OBJ_INFO_F_NO_CONTENT		(1<<13)	/* object does not have content, objRead() would fail */
#define OBJ_INFO_F_SUPPORTS_INHERITANCE	(1<<14)	/* object can support inheritance attr cx__inherit, etc. */


/** object virtual attribute - these are attributes which persist only while
 ** the object is open, and whose values and types are obtained by invoking
 ** external function calls.  Can be used to dynamically add an attribute to
 ** an open object.
 **/
typedef struct _OVA
    {
    struct _OVA*	Next;		/* next virtual attr in the list */
    char		Name[32];	/* name of the attribute */
    void*		Context;	/* arbitrary data from addvirtualattr() caller */
    int			(*TypeFn)();	/* GetAttrType */
    int			(*GetFn)();	/* GetAttrValue */
    int			(*SetFn)();	/* SetAttrValue */
    int			(*FinalizeFn)(); /* called when about to close the object */
    }
    ObjVirtualAttr, *pObjVirtualAttr;


/** objectsystem open fd **/
typedef struct _OF
    {
    int		Magic;		/* Magic number for object */
    pObjDriver	Driver;		/* os-Driver handling this object */
    pObjDriver	TLowLevelDriver; /* for transaction management */
    pObjDriver	ILowLevelDriver; /* for inheritance mechanism */
    void*	Data;		/* context param passed to os-Driver */
    struct _OF*	Obj;		/* if this is an attribute, this is its obj */
    XArray	Attrs;		/* Attributes that are open. */
    pPathname	Pathname;	/* Pathname of this object */
    short 	SubPtr;		/* First component of path handled by this driver */
    short 	SubCnt;		/* Number of path components used by this driver */
    short	Flags;		/* OBJ_F_xxx, see below */
    int		Mode;		/* Mode: O_RDONLY / O_CREAT / etc */
    pContentType Type;
    pObjSession	Session;
    int		LinkCnt;	/* Don't _really_ close until --LinkCnt == 0 */
    pXString	ContentPtr;	/* buffer for accessing obj:objcontent */
    struct _OF*	Prev;		/* open object for accessing the "node" */
    struct _OF*	Next;		/* next object for intermediate opens chain */
    ObjectInfo	AdditionalInfo;	/* see ObjectInfo definition above */
    void*	NotifyItem;	/* pObjReqNotifyItem; not-null when notifies are active on this */
    pObjVirtualAttr VAttrs;	/* virtual attributes - call external fn()'s to obtain the data */
    void*	EvalContext;	/* a pParamObjects list -- for evaluation of runserver() exprs */
    void*	AttrExp;	/* an expression used for the above */
    char*	AttrExpName;	/* Name of attr for above expression */
    }
    Object, *pObject;

#define OBJ_F_ROOTNODE		1	/* is rootnode object, handle specially */
#define	OBJ_F_CREATED		2	/* O_CREAT requested; object didn't exist but was created */
#define OBJ_F_DELETE		4	/* object should be deleted on final close */
#define OBJ_F_NOCACHE		8	/* object should *not* be cached by the Directory Cache */


/** structure used for sorting a query result set. **/
typedef struct _SRT
    {
    XArray	SortPtr[2];	/* ptrs to sort key data */
    XArray	SortPtrLen[2];	/* lengths of sort key data */
    XArray	SortNames[2];	/* names of objects */
    XString	SortDataBuf;	/* buffer for sort key data */
    XString	SortNamesBuf;	/* buffer for object names */
    }
    ObjQuerySort, *pObjQuerySort;


/** object query information **/
typedef struct _OQ
    {
    int		Magic;
    pObject	Obj;
    char*	QyText;
    void*	Tree;	/* pExpression */
    void*	SortBy[16];	/* pExpression [] */
    void*	ObjList; /* pParamObjects */
    void*	Data;
    int		Flags;
    int		RowID;
    pObjQuerySort SortInf;
    pObjDriver	Drv;		/* used for multiquery only */
    pObjSession	QySession;	/* used for multiquery only */
    }
    ObjQuery, *pObjQuery;

#define OBJ_QY_F_ALLOCTREE	1
#define OBJ_QY_F_FULLQUERY	2
#define OBJ_QY_F_FULLSORT	4
#define OBJ_QY_F_FROMSORT	8


/*** Event and EventHandler structures ***/
typedef struct _OEH
    {
    char		ClassCode[16];
    int			(*HandlerFunction)();
    }
    ObjEventHandler, *pObjEventHandler;

typedef struct _OE
    {
    char		DirectoryPath[OBJSYS_MAX_PATH];
    char*		XData;
    char*		WhereClause;
    char		ClassCode[16];
    int			Flags;
    pObjEventHandler	Handler;
    }
    ObjEvent, *pObjEvent;

#define OBJ_EV_F_NOSAVE		1	/* internal - don't rewrite events file */


extern int obj_internal_DiscardDC(pXHashQueue hq, pXHQElement xe, int locked);

/** directory entry caching data **/
typedef struct _DC
    {
    char		Pathname[OBJSYS_MAX_PATH];
    pObject		NodeObj;
    }
    DirectoryCache, *pDirectoryCache;

#define DC_F_ISDIR	1


/*** ObjectSystem Globals ***/
typedef struct
    {
    XArray	OpenSessions;		/* open ObjectSystem sessions */
    XHashTable	TypeExtensions;		/* extension -> type mapping */
    XHashTable	DriverTypes;		/* type -> driver mapping */
    XArray	Drivers;		/* Registered driver list */
    XHashTable	Types;			/* Just a registered type list */
    XArray	TypeList;		/* Iterable registered type list */
    int		UseCnt;			/* for LRU cache list */
    pObjDriver	TransLayer;		/* Transaction layer */
    pObjDriver	MultiQueryLayer;	/* MQ module */
    pObjDriver	InheritanceLayer;	/* dynamic inheritance module */
    XHashTable	EventHandlers;		/* Event handlers, by class code */
    XHashTable	EventsByXData;		/* Events, by class code/xdata */
    XHashTable	EventsByPath;		/* Events, by pathname */
    XArray	Events;			/* Table of events */
    pContentType RootType;		/* Type of root node */
    char	RootPath[OBJSYS_MAX_PATH]; /* Path to root node file */
    pObjDriver	RootDriver;
    HandleContext SessionHandleCtx;	/* context for session handles */
    XHashTable	NotifiesByPath;		/* objects with RequestNotify() */
    long long	PathID;			/* pseudo-paths for multiquery */
    }
    OSYS_t;

extern OSYS_t OSYS;


/*** structure for managing request-notify ***/
typedef struct
    {
    int		TotalFlags;		/* OBJ_RN_F_xxx bitwise ORed from all */
    char	Pathname[OBJSYS_MAX_PATH];  /* path to object */
    XArray	Requests;		/* all requests for notification */
    }
    ObjReqNotify, *pObjReqNotify;


/*** structure for ONE request-notify ***/
typedef struct _ORNI
    {
    struct _ORNI* Next;
    pObjReqNotify NotifyStruct;		/* parent structure */
    pObject	Obj;			/* object handle */
    int		Flags;			/* OBJ_RN_F_xxx for this requestor */
    int		(*CallbackFn)();	/* callback function */
    void*	CallerContext;		/* passed in by caller */
    MTSecContext SavedSecContext;	/* security context of requestor */
    }
    ObjReqNotifyItem, *pObjReqNotifyItem;

/*** structure for delivery of notification ***/
typedef struct
    {
    pObjReqNotifyItem __Item;		/* for internal use only */
    pObject	Obj;			/* the object being modified */
    void*	Context;		/* context provided on ReqNotify() */
    int		What;			/* what is being modified - OBJ_RN_F_xxx */
    char	Name[MAX(OBJSYS_MAX_ATTR,OBJSYS_MAX_PATH)];	/* attribute name or object name, if needed */
    TObjData	NewAttrValue;		/* new value of attr */
    int		PtrSize;		/* size of Ptr data */
    int		Offset;			/* if content, offset to Ptr data */
    int		NewSize;		/* if content, new size of content */
    void*	Ptr;			/* actual information */
    int		IsDel;			/* 1 if Name is being deleted, 0 if added */
    }
    ObjNotification, *pObjNotification;


/*** OSML debugging flags ***/
#define OBJ_DEBUG_F_APITRACE	1

/*** Debugging control ***/
/*#define OBJ_DEBUG		(OBJ_DEBUG_F_APITRACE)*/
#define OBJ_DEBUG		(0)

/*** Debugging output macro ***/
#define OSMLDEBUG(f,p ...) if (OBJ_DEBUG & (f)) printf(p);


/*** ObjectSystem Driver Library - parameters structure ***/
typedef struct
    {
    char	Name[32];
    int		Type;
    int		IntParam;
    char	StringParam[80];
    }
    ObjParam, *pObjParam;

/*** Flags to be passed to objOpen() in the 'mode' ***/
#define OBJ_O_CREAT	(O_CREAT)
#define OBJ_O_TRUNC	(O_TRUNC)
#define OBJ_O_RDWR	(O_RDWR)
#define OBJ_O_RDONLY	(O_RDONLY)
#define OBJ_O_WRONLY	(O_WRONLY)
#define OBJ_O_EXCL	(O_EXCL)

#define OBJ_O_AUTONAME	(1<<30)
#define OBJ_O_NOINHERIT	(1<<29)

#define OBJ_O_CXOPTS	(OBJ_O_AUTONAME | OBJ_O_NOINHERIT)

#if (OBJ_O_CXOPTS & (O_CREAT | O_TRUNC | O_ACCMODE | O_EXCL))
#error "Conflict in objectsystem OBJ_O_xxx options, sorry!!!"
#endif


/*** Flags for objRequestNotify() replication function ***/
#define	OBJ_RN_F_INIT		(1<<0)		/* send initial state */
#define	OBJ_RN_F_ATTRIB		(1<<1)		/* attribute changes */
#define OBJ_RN_F_CONTENT	(1<<2)		/* content changes */
#define OBJ_RN_F_SUBOBJ		(1<<3)		/* subobject insert/delete */
#define OBJ_RN_F_SUBTREE	(1<<4)		/* all descendents */


/** objectsystem main functions **/
int objInitialize();
int rootInitialize();
int oxtInitialize();
int objRegisterDriver(pObjDriver drv);

/** objectsystem session functions **/
pObjSession objOpenSession(char* current_dir);
int objCloseSession(pObjSession this);
int objSetWD(pObjSession this, pObject wd);
char* objGetWD(pObjSession this);
int objSetDateFmt(pObjSession this, char* fmt);
char* objGetDateFmt(pObjSession this);
int objUnmanageObject(pObjSession this, pObject obj);
int objUnmanageQuery(pObjSession this, pObjQuery qy);
int objCommit(pObjSession this);

/** objectsystem object functions **/
pObject objOpen(pObjSession session, char* path, int mode, int permission_mask, char* type);
int objClose(pObject this);
int objCreate(pObjSession session, char* path, int permission_mask, char* type);
int objDelete(pObjSession session, char* path);
int objDeleteObj(pObject this);
pObject objLinkTo(pObject this);
pObjectInfo objInfo(pObject this);
char* objGetPathname(pObject this);

/** objectsystem directory/query functions **/
pObjQuery objMultiQuery(pObjSession session, char* query, void* objlist);
pObjQuery objOpenQuery(pObject obj, char* query, char* order_by, void* tree, void** orderby_exp);
int objQueryDelete(pObjQuery this);
pObject objQueryFetch(pObjQuery this, int mode);
pObject objQueryCreate(pObjQuery this, char* name, int mode, int permission_mask, char* type);
int objQueryClose(pObjQuery this);

/** objectsystem content functions **/
int objRead(pObject this, char* buffer, int maxcnt, int offset, int flags);
int objWrite(pObject this, char* buffer, int cnt, int offset, int flags);

/** objectsystem attribute functions **/
int objGetAttrType(pObject this, char* attrname);
int objSetEvalContext(pObject this, void* objlist);
#if 1
int objSetAttrValue(pObject this, char* attrname, int data_type, pObjData val);
int objGetAttrValue(pObject this, char* attrname, int data_type, pObjData val);
#else
#define _OBJATTR_CONV
int objSetAttrValue();
int objGetAttrValue();
#endif
char* objGetFirstAttr(pObject this);
char* objGetNextAttr(pObject this);
pObject objOpenAttr(pObject this, char* attrname, int mode);
int objAddAttr(pObject this, char* attrname, int type, pObjData val);
pObjPresentationHints objPresentationHints(pObject this, char* attrname);
int objFreeHints(pObjPresentationHints ph);
int objAddVirtualAttr(pObject this, char* attrname, void* context, int (*type_fn)(), int (*get_fn)(), int (*set_fn)(), int (*finalize_fn)());

/** objectsystem method functions **/
char* objGetFirstMethod(pObject this);
char* objGetNextMethod(pObject this);
int objExecuteMethod(pObject this, char* methodname, pObjData param);

/** objectsystem driver library functions **/
pXArray objParamsRead(pFile fd);
int objParamsWrite(pFile fd, pXArray params);
int objParamsLookupInt(pXArray params, char* name);
char* objParamsLookupString(pXArray params, char* name);
int objParamsSet(pXArray params, char* name, char* stringval, int intval);
int objParamsFree(pXArray params);
int obj_internal_IsA(char* type1, char* type2);
int obj_internal_FreePath(pPathname this);
int obj_internal_FreePathStruct(pPathname this);
pPathname obj_internal_NormalizePath(char* cwd, char* name);
int obj_internal_AddChildTree(pObjTrxTree parent_oxt, pObjTrxTree child_oxt);
pObject obj_internal_AllocObj();
int obj_internal_FreeObj(pObject);

/** objectsystem transaction functions **/
int obj_internal_FreeTree(pObjTrxTree oxt);
pObjTrxTree obj_internal_AllocTree();
pObjTrxTree obj_internal_FindTree(pObjTrxTree oxt, char* path);
int obj_internal_SetTreeAttr(pObjTrxTree oxt, int type, pObjData val);
pObjTrxTree obj_internal_FindAttrOxt(pObjTrxTree oxt, char* attrname);

/** objectsystem path manipulation **/
char* obj_internal_PathPart(pPathname path, int start_element, int length);
int obj_internal_PathPrefixCnt(pPathname full_path, pPathname prefix);
int obj_internal_CopyPath(pPathname dest, pPathname src);
int obj_internal_AddToPath(pPathname path, char* new_element);

/** objectsystem datatype functions **/
int objDataToString(pXString dest, int data_type, void* data_ptr, int flags);
double objDataToDouble(int data_type, void* data_ptr);
int objDataToInteger(int data_type, void* data_ptr, char* format);
int objDataToDateTime(int data_type, void* data_ptr, pDateTime dt, char* format);
int objDataToMoney(int data_type, void* data_ptr, pMoneyType m);
char* objDataToStringTmp(int data_type, void* data_ptr, int flags);
int objDataCompare(int data_type_1, void* data_ptr_1, int data_type_2, void* data_ptr_2);
char* objDataToWords(int data_type, void* data_ptr);
int objCopyData(pObjData src, pObjData dst, int type);
int objTypeID(char* name);
int objDebugDate(pDateTime dt);
int objDataFromString(pObjData pod, int type, char* str);


/** objectsystem replication services - open object notification (Rn) system **/
int objRequestNotify(pObject this, int (*callback_fn)(), void* context, int what);
int obj_internal_RnDelete(pObjReqNotifyItem item);
int obj_internal_RnNotifyAttrib(pObject this, char* attrname, pTObjData newvalue, int send_this);
int objDriverAttrEvent(pObject this, char* attr_name, pTObjData newvalue, int send_this);


/** objectsystem event handler stuff -- for os drivers etc **/
int objRegisterEventHandler(char* class_code, int (*handler_function)());
int objRegisterEvent(char* class_code, char* pathname, char* where_clause, int flags, char* xdata);
int objUnRegisterEvent(char* class_code, char* xdata);

#endif /*_OBJ_H*/
