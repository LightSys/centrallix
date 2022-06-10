# DHTML Widget Driver Authoring Guide

Author: Greg Beeley

Date: October 23, 2001

License: Copyright (C) 2001 LightSys Technology Services.  See LICENSE.txt.

## Table of Contents
- [DHTML Widget Driver Authoring Guide](#dhtml-widget-driver-authoring-guide)
  - [Table of Contents](#table-of-contents)
  - [I. Introduction](#i-introduction)
  - [II. Centrallix DHTML Page Model](#ii-centrallix-dhtml-page-model)
    - [A.	Page Header](#apage-header)
    - [B.	Variable Declarations](#bvariable-declarations)
    - [C.	Script Functions](#cscript-functions)
    - [D.	Event Handlers](#devent-handlers)
    - [E.	Initialization](#einitialization)
    - [F.	HTML Body Parameters](#fhtml-body-parameters)
    - [G.	HTML Body](#ghtml-body)
  - [III Page Generation Process](#iii-page-generation-process)
  - [IV  HTML Generator Interface](#iv--html-generator-interface)
    - [A.	Initialization](#ainitialization)
      - [Function: htrAllocDriver() returns pHtDriver](#function-htrallocdriver-returns-phtdriver)
      - [Function: htrRegisterDriver(pHtDriver drv) returns int](#function-htrregisterdriverphtdriver-drv-returns-int)
      - [Function: htrAddAction(pHtDriver drv, char* action_name) returns int](#function-htraddactionphtdriver-drv-char-action_name-returns-int)
      - [Function: htrAddEvent(pHtDriver drv, char* event_name) returns int](#function-htraddeventphtdriver-drv-char-event_name-returns-int)
      - [Function: htrAddParam(pHtDriver drv, char* eventaction, char* param_name, int datatype) returns int](#function-htraddparamphtdriver-drv-char-eventaction-char-param_name-int-datatype-returns-int)
      - [Structure: HtDriver (pHtDriver is the pointer typedef)](#structure-htdriver-phtdriver-is-the-pointer-typedef)
    - [B.  Callback Methods](#b--callback-methods)
    - [C.	Page Generation API Methods](#cpage-generation-api-methods)
      - [htrAddStylesheetItem(pHtSession s, char* html_text) returns int](#htraddstylesheetitemphtsession-s-char-html_text-returns-int)
      - [htrAddHeaderItem(pHtSession s, char* html_text) returns int](#htraddheaderitemphtsession-s-char-html_text-returns-int)
      - [htrAddBodyItem(pHtSession s, char* html_text) returns int](#htraddbodyitemphtsession-s-char-html_text-returns-int)
      - [htrAddBodyParam(pHtSession s, char* html_param) returns int](#htraddbodyparamphtsession-s-char-html_param-returns-int)
      - [htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags) returns int](#htraddscriptglobalphtsession-s-char-var_name-char-initialization-int-flags-returns-int)
      - [htrAddScriptFunction(pHtSession s, char* fn_name, char* fn_text, int flags) returns int](#htraddscriptfunctionphtsession-s-char-fn_name-char-fn_text-int-flags-returns-int)
      - [htrAddScriptInit(pHtSession s, char* init_text) returns int](#htraddscriptinitphtsession-s-char-init_text-returns-int)
      - [htrAddEventHandler(pHtSession s, char* event_src, char* event, char* drvname, char* handler_code) returns int](#htraddeventhandlerphtsession-s-char-event_src-char-event-char-drvname-char-handler_code-returns-int)
      - [htrDisableBody(pHtSession s) returns int](#htrdisablebodyphtsession-s-returns-int)
      - [htrRenderWidget(pHtSession s, pObject widget_obj, int z, char* parentname, char* parentobj) returns int](#htrrenderwidgetphtsession-s-pobject-widget_obj-int-z-char-parentname-char-parentobj-returns-int)
      - [htrRenderSubwidgets(pHtSession s, pObject widget_obj, char* docname, char* layername, int z) returns int](#htrrendersubwidgetsphtsession-s-pobject-widget_obj-char-docname-char-layername-int-z-returns-int)
    - [V Frequently-Used OSML Methods](#v-frequently-used-osml-methods)
      - [objGetAttrValue(pObject obj, char* attrname, pObjData value) returns int](#objgetattrvaluepobject-obj-char-attrname-pobjdata-value-returns-int)
      - [objGetAttrType(pObject obj, char* attrname) returns int](#objgetattrtypepobject-obj-char-attrname-returns-int)
      - [objOpenQuery(pObject obj, char* where, char* orderby, void* tree, void** orderby_exp) returns pObjQuery](#objopenquerypobject-obj-char-where-char-orderby-void-tree-void-orderby_exp-returns-pobjquery)
      - [objQueryClose(pObjQuery query) returns int](#objqueryclosepobjquery-query-returns-int)
      - [objQueryFetch(pObjQuery query, int mode) returns pObject](#objqueryfetchpobjquery-query-int-mode-returns-pobject)
      - [objClose(pObject obj) returns int](#objclosepobject-obj-returns-int)
  - [VI Other Frequently-Used Utility Methods](#vi-other-frequently-used-utility-methods)
    - [A.	Memory Manager Methods](#amemory-manager-methods)
      - [nmMalloc(int size)](#nmmallocint-size)
      - [nmFree(void* ptr, int size)](#nmfreevoid-ptr-int-size)
      - [nmSysMalloc(int size)](#nmsysmallocint-size)
      - [nmSysFree(void* ptr)](#nmsysfreevoid-ptr)
      - [nmSysStrdup(char* ptr)](#nmsysstrdupchar-ptr)
      - [nmSysRealloc(void* ptr, int newsize)](#nmsysreallocvoid-ptr-int-newsize)
    - [B.	Session and Error Reporting Methods](#bsession-and-error-reporting-methods)
      - [mssError(int reset, char* modname, char* fmt, ...)](#msserrorint-reset-char-modname-char-fmt-)
  - [VII Development by Prototyping](#vii-development-by-prototyping)
    - [A.	Designing your Widget](#adesigning-your-widget)
    - [B.	Generating the First "Rough Draft"](#bgenerating-the-first-rough-draft)
    - [C.	Prototyping the widget](#cprototyping-the-widget)

## I. Introduction
A DHTML Widget Driver (or, just "widget driver") provides the functionality needed to generate DHTML code (HTML and JavaScript) to make a widget smoothly operate in a browser.  Widget drivers are probably the simplest extension component in Centrallix to author, but this document seeks to shed some light on the otherwise obscure widget interactions and on the HTML generation subsystem interface.

Widgets in Centrallix come in two basic forms: visual widgets and nonvisual widgets.  Visual widgets usually represent some sort of control: a button, a single-line edit box, and the like.  Nonvisual widgets normally represent some kind of functionality, such as form widgets, event-action connector widgets, and JavaScript custom function widgets.

Most visual widgets are built using DHTML layers, represented as absolute- positioned CSS style sheets.

Next, we'll take a look at the seven basic subsections that Centrallix uses to build a DHTML application.

## II. Centrallix DHTML Page Model
HTML applications have the advantage of being extremely easy to deploy. However, maintaining a set of DHTML applications can be a nightmare due to the way a HTML page is put together.  Centrallix addresses this problem by abstracting the layout of the DHTML page a little bit in order to make writing and using DHTML widgets a much easier task.

Centrallix divides the DHTML page into seven basic subsections, which will be discussed in the following parts of this document.

### A.	Page Header

The page header consists of all HTML code which resides between the `<HEAD>` and `</HEAD>` tags in the page.  This includes META tags, the page title, as well as style sheet (layer) declarations.

### B.	Variable Declarations

The next section, within a `<SCRIPT LANGUAGE=JavaScript>` block, defines global variables for the page.  These usually take the form of control variables for some of the widgets as well as (typically) one global variable each for the instances of each widget, so that the widget can be referenced by its name on the page.  Variable declarations consist of a variable name and a default initialization value.

### C.	Script Functions

Each widget driver uses a set of page-independent script functions to "power" the instances of the widget type it manages.  These script functions will probably at some point be moved out of the widget driver C code and into a separately-loadable (and thus cachable) set of .js files, but for now they are generated directly into the DHTML page.

The connector nonvisual widget also generates page-dependent script functions in this category.

### D.	Event Handlers

Most widget drivers install one or more event handlers.  These event handlers, for those who are familiar with the way DHTML event handling is performed, operate entirely at the top-level "document" and do not rely on the browser's idea of a "trickle-down" or "trickle-up" event processing model.  The event handlers are snippets of javascript code (not complete functions) which are grouped together when the final page is generated (i.e., all of the "mousemove" event handlers are put into a single javascript function).

Widget driver code in the event handlers will have to be able to in some way "recognize" if the event occured on a widget that the driver manages.  This is normally done by "tagging" the widget's layers with a "kind" attribute containing a short string identifying the type of widget (e.g., "eb" for the editbox widget, "tb" for the text button widget, etc.)

### E.	Initialization

The initialization code is very app-dependent, and is called as soon as the page finishes loading (the BODY tag onload event).  Typically, each widget driver will have installed a script function (see 'C', above), which initializes an instance of a widget.  That initialization function is normally called, once per widget, from the initialization section of the page.

### F.	HTML Body Parameters

The HTML `<BODY>` tag has some special parameters which need to be accessed by the widget drivers, and in particular by the 'page' widget driver.  These parameters, for instance, set page colors and background images.

### G.	HTML Body

The HTML body contains the content of the page.  Typically, the layers which were declared in the page header (section A) are nested together and fleshed out with content in the HTML body via the use of properly nested `<DIV>` tags.

## III Page Generation Process

The process by which the HTML generation subsystem as a whole generates a page is composed of two steps.  First, the widget drivers are activated in a hierarchical fashion to generate each widget instance into the HTML page data structures maintained by the subsystem.  These data structures reflect the seven various components of the page as described in the previous section.

Second, the HTML generation subsystem converts these data structures into the HTML page itself.

The first step (involving the widget drivers) begins with the subsystem opening the top-level object in the page (either a "widget/page" type object or a "widget/frameset" type object), looking up the correct driver to handle that type of widget, and then calling that widget's Render() function using that open object (which is a pObject, returned from the objOpen() OSML API method).

The responsibility of the page widget driver (and, any other widget driver for that matter) is to perform a number of things when its Render() function is called.

1.  Use the OSML objGetAttrValue/objGetAttrType calls to obtain the necessary configuration data about the widget instance (such as x and y locations, background colors, etc).  Information regarding these OSML API methods is provided later in this document.
2.  Generate any needed variables, functions, event handlers, header text, initialization code, and html body parameters.
3.  Generate most of the html body, up to the point where a <DIV> has been begun that represents the layer that will contain any subwidgets.
4.  Tell the HTML generation system to generate all subwidgets, if this widget can contain subwidgets (some, such as an editbox or checkbox, cannot).
5.  Finish the containing layer from (3) above as well as any other layers that have not been finished yet.


## IV  HTML Generator Interface
This section of the HTML Widget Driver authoring guide defines the interface functionality with the HTML generating subsystem.  Widget drivers use these routines to actually build the page and its widgets.

Modules should #include the following files: "ht_render.h", "obj.h", "mtask.h", "mtsession.h".

### A.	Initialization

Module initialization is performed in the xxxInitialize() function for the module, where 'xxx' is the module's chosen prefix.  Most widget drivers have prefixes of the form 'htxx' where 'xx' is a two-to-four letter abbreviation of the module, such as 'eb' for the editbox widget.

The initialization process mainly consists of 1) allocating a new driver structure, 2) filling out the appropriate fields in the structure, 3) adding events and/or actions to the driver structure, 4) initializing any module globals, and finally, 5) registering the driver structure with the HTML generation subsystem.  The module's Initialize() function should return 0 on success.

#### Function: htrAllocDriver() returns pHtDriver

This function allocates a new initialized HtDriver structure. After calling this function and obtaining the HtDriver pointer, the module's Initialize() routine should set up the structure appropriately and then register it with the next function below.

#### Function: htrRegisterDriver(pHtDriver drv) returns int

This function registers a new widget driver with the DHTML generation subsystem.  On success, it returns 0; on failure it returns -1.

#### Function: htrAddAction(pHtDriver drv, char* action_name) returns int

Adds an Action to the catalog of actions associated with this widget driver.

#### Function: htrAddEvent(pHtDriver drv, char* event_name) returns int

Adds an Event to the catalog of events associated with this widget driver.

#### Function: htrAddParam(pHtDriver drv, char* eventaction, char* param_name, int datatype) returns int

Adds a parameter (with a given data type) to an event or action.

#### Structure: HtDriver (pHtDriver is the pointer typedef)

| Attribute    | Type          | Description
| ------------ | ------------- | -----------
| Name         | char[64]      | A descriptive name of the driver.
| WidgetName   | char[64]      | A one-word name of the widget.  This name will be used in the structure file type name when declaring an instance of the widget, as in "widget/widgetname" where "widgetname" is this property.
| Render       | function ptr. | The module's Initialize() function should set this function pointer to the module's Render() function.  See below for more information on Render().
| Verify       | function ptr. | This function pointer should be set to the module's Verify() function, which is not yet used.  Verify() should simply return 0.

All other elements of this structure are "private" and should not be modified or accessed directly by the widget driver.  Instead, there are API functions which are used to set up these values.  If you look at the already-implemented drivers in the Centrallix distribution, many of them *do* access these directly, but only because the additional API functions were not written at that time. The additional API functions make the initialization process considerably simpler, so please do use them :)

### B.  Callback Methods

During initialization, the module specified two callback methods which the HTML generation subsystem will use to create widgets within the page.  Note that these functions are provided by the widget driver itself, not by the HTML generation subsystem in general.

The first method, Verify(), does nothing at current.  These Verify() methods should simply return 0 (success) when called.

    int htxxVerify();

The second method, Render(), actually generates an instance of a widget into the page.  It should return 0 (success) upon successfully generating the widget, or -1 (failure) if something goes wrong in the process (such as missing configuration data).

    int htxxRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj);

The 's' parameter is the generator subsystem session structure, which provides the context for the generation of the page.  Almost all of the API functions require the session structure to be passed as a parameter.

The 'w_obj' parameter is an open OSML object, either obtained with the objOpen() call or with objQueryFetch().  The object should be treated as a readonly object.

The 'z' parameter is the current Z-buffer level for the generated page.  The widget driver should use this Z value as a 'base value' for its generated layers and objects, and pass a Z value one higher than the highest it used, when calling htrRenderSubwidgets() or similar.

The 'parentname' parameter is the name of the "document" type object that will be containing the widget.

The 'parentobj' parameter is the name of the "layer" object that will contain the widget.  Normally this is related to the above parameter, such as a 'parentname' of 'thing.document' would correspond to a 'parentobj' of 'thing'.

### C.	Page Generation API Methods

The following list of functions are used by widget drivers to actually generate the DHTML and scripting needed to support widget instances.

#### htrAddStylesheetItem(pHtSession s, char* html_text) returns int
This functions adds a stylesheet definition to the header between the tags `<STYLE TYPE="text/css"></STYLE>`.  It is recommended to maintain a consistant indention style (using a tab '\t' character) at the beginning of these lines.

Example:

`htrAddStylesheetItem(s, "\t#pgtop { POSITION:absolute; VISIBILITY:hidden;");`

#### htrAddHeaderItem(pHtSession s, char* html_text) returns int
This function adds text directly to the HTML header section of the generated document (between `<HEAD>` and `</HEAD>`).  The html_text string's content is copied into subsystem data structures.

It is generally recommended to maintain a reasonably consistent indenting method when generating the HTML.  See some of the existing widget drivers for examples of this.

The 'pHtSession' parameter 's' is passed to the Render() function as the first parameter.

Example:

`htrAddHeaderItem(s, "    <TITLE>This is a title</TITLE>\n");`

#### htrAddBodyItem(pHtSession s, char* html_text) returns int
This function adds text directly into the HTML body of the document, and works in a manner identical to the above HeaderItem function.

Example:

`htrAddBodyItem(s, "<DIV id=lb0base>Label:</DIV>\n");`

#### htrAddBodyParam(pHtSession s, char* html_param) returns int
This function adds a 'parameter' to the HTML `<BODY>` tag of the document.

Example:

`htrAddBodyParam(s, "bgcolor=white");`

#### htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags) returns int
This function adds a global script variable to the page.  The 'var_name' parameter contains the name of the variable, and the 'initialization' parameter contains its default value in the variable declaration.

The 'flags' parameter tells the rendering subsystem whether it should auto-free the pointed-to value of either or both of the 'var_name' and 'initialization'.  It can contain one or more of the following values ORed together:

- HTR_F_NAMEALLOC	    -	auto-free the var_name when complete.
- HTR_F_VALUEALLOC    -	auto-free 'initialization' when done.

Example:

```
ptr = nmMalloc(strlen(name)+1);
if (!ptr) return -1;
strcpy(ptr,name);
htrAddScriptGlobal(s, ptr, "null", HTR_F_NAMEALLOC);
```

#### htrAddScriptFunction(pHtSession s, char* fn_name, char* fn_text, int flags) returns int
This function adds a unique function to the document.  Usually, the function consists of constant text, and so the 'flags' argument is normally left as zero.  However, if the driver allocates a block of memory for the function text or name, it can request that the HTML generation subsystem free the string's memory when the document has been completely sent to the client.  Drivers can do this by using one or more of the following 'flags':

- HTR_F_NAMEALLOC	    -	auto-free fn_name.
- HTR_F_VALUEALLOC    -	auto-free fn_text.

Example:

```
htrAddScriptFunction(s, "lb_hide", "\n"
    "function lb_hide()\n"
    "    {\n"
    "    this.visibility = 'hidden';\n"
    "    return true;\n"
    "    }\n", 0);
```

#### htrAddScriptInit(pHtSession s, char* init_text) returns int
This function adds script code to the main initialization function for the whole HTML page.  This function works just like the AddHeaderItem and AddBodyItem functions - the init_text is copied into subsystem data structures and thus its memory can be freed or re-used immediately after the function call (unlike the AddScript- Function and AddScriptGlobal functions).

The init_text should begin with four spaces and end with a line- feed character (\n), to match the generated HTML style.

The initialization code will be executed after the entire page has been loaded into the browser but before any event triggering is enabled.

It is normally expected that a widget will initialize itself before any subwidgets, so this function should be called when necessary *before* any subwidgets are generated.

Example:

`htrAddScriptInit(s, "    lb_init(document.layers.lb0base);\n");`

#### htrAddEventHandler(pHtSession s, char* event_src, char* event, char* drvname, char* handler_code) returns int
This function adds an event handler script segment (not a complete function declaration).  The event handler will manage an event of a given type, for a given type of widget.

The 'event_src' parameter is always "document" for the time being.

The 'event' parameter is the type of event.  Allowed types are events such as "MOUSEMOVE", "MOUSEOVER", "MOUSEOUT", "MOUSEDOWN", "MOUSEUP", "KEYPRESS", and so forth.

The 'drvname' parameter is the two-to-four letter abbreviation of the widget driver, such as 'eb' for the 'editbox' widget.  This parameter is used by the subsystem to keep event scripts for the different widgets distinct from one another.

Finally, the 'handler_code' parameter is the text of the script segment.

All of the string values here are *copied* by the subsystem into internal session structures.

IMPORTANT NOTE:  Calling this function twice for the same event and the same widget driver will not have the expected result.  Because the script segments are keyed internally by driver, event source, and event type, such subsequent calls will be completely ignored by the HTML generation subsystem.  Widget drivers should group all code for a given event type together in one call to this function.

Example:

```
htrAddEventHandler(s, "document", "MOUSEOVER", "lb",
    "    if (e.target && e.target.kind == 'lb')\n"
    "        {\n"
    "        e.target.bgcolor = '#ff0000';\n"
    "        }\n" );
```

#### htrDisableBody(pHtSession s) returns int
This function is used to disable the output of the `<BODY>` and `</BODY>` tags, including any body parameters added with the API function htrAddBodyParam().  This is primarly used by the frameset widget driver to suppress the output of the body in favor of the frameset declarations.  Note that it probably does not make sense to call htrRenderSubwidgets() when the body has been disabled :)

#### htrRenderWidget(pHtSession s, pObject widget_obj, int z, char* parentname, char* parentobj) returns int
This function is used to render a subwidget within the current widget.  In order to use this function, the calling driver most likely will have done an objOpenQuery() operation, fetching any subwidgets with objQueryFetch(), and then proceeding to call this function with the pObject returned from the fetch operation.  Note that this function does *not* close the open widget object; if the widget driver opens an object with objOpen or objQueryFetch, it must also close that object with objClose.

Parameter 's' is the session parameter.

The 'widget_obj' parameter is a pObject handle returned most likely from objQueryFetch.

The 'z' parameter is the z-index that the subwidget should start to be generated at.  It should be one higher than the highest z index used by the current widget.

The 'parentname' parameter is the name of the document object into which the subwidget will be rendered.  This is needed so that the subwidget can reference its CSS layers / DIV sections correctly.

The 'parentobj' parameter is normally the name of the layer (or window) which contains the above mentioned document object.

#### htrRenderSubwidgets(pHtSession s, pObject widget_obj, char* docname, char* layername, int z) returns int
This function automates the generation of subwidgets for those widgets that are simple containers.  Instead of having to do an objOpenQuery type of loop, in this case the calling driver just passes its own widget object (from its Render() function call) to htrRenderSubwidgets(), and this function does all of the query work.

See the htrRenderWidget() function for further details on the parameters (the widget_obj parameter is the primary difference).

### V Frequently-Used OSML Methods
This section details the OSML methods that are most frequently used by widget drivers.  This isn't intended to be a complete OSML API reference.

#### objGetAttrValue(pObject obj, char* attrname, pObjData value) returns int
This function obtains the value of an attribute from the application definition file, probably a ".app" structure file.  The first function parameter, 'obj' is the open object, probably that from the Render() function parameter.

The second parameter, 'attrname' is the (usually) case-sensitive name of the attribute whose value is being obtained.

The third parameter is a pointer to a C union type.  This union is used to pass data of various types to and from some of the OSML API functions.  Basically, it allows the passed value to be one of several types:

    - pointer to an integer,
    - pointer to a char pointer or char array,
    - pointer to a double-precision floating point value,
    - pointer to a pointer to a DateTime structure,
    - pointer to a pointer to a MoneyType structure.

The POD(x) macro is generally used to do the appropriate typecasting, to avoid needless declarations of pObjData variables.  The POD(x) macro is defined as:  ((pObjData)(x))

This API function returns -1 if the attribute was not found, or on other types of errors.  It returns 1 if the attribute was found but was NULL.  It returns 0 if the attribute was found and its value was successfully returned.

Examples:

    int x;
    char* strval;
    if (objGetAttrValue(w_obj, "x", POD(&x)) != 0) x=0;
    if (objGetAttrValue(w_obj, "style", POD(&strval)) != 0) strval="";

#### objGetAttrType(pObject obj, char* attrname) returns int
This function returns the data type of a given attribute.  See the objGetAttrValue() function reference for information about the two parameters involved.

This function returns an integer, which will be -1 if the attribute was not found, or one of the following values, defined in the file "datatypes.h":

- DATA_T_INTEGER:	an integer value
- DATA_T_STRING:	a char* string value
- DATA_T_DOUBLE:	a double-precision floating point value
- DATA_T_MONEY:	a 48-bit money representation
- DATA_T_DATETIME:	a date/time structure

There are a few other possibilities, but none should be relevant to the driver author at this time.

Example:

    if (objGetAttrType(w_obj, "x") != DATA_T_INTEGER)
    {
    mssError(0,"HTLB","Labels need an integer 'x' property");
    return -1;
    }

#### objOpenQuery(pObject obj, char* where, char* orderby, void* tree, void** orderby_exp) returns pObjQuery
This function is used to issue a query request for subobjects of a given parent object, which makes it just perfect for discovering subwidgets declared within a given widget.

The 'obj' parameter is the pObject reference to the parent object.

The 'where' parameter is a textual expression constraining what objects are returned, such as ":x == 1".  This parameter is normally left as the empty string "" by widget drivers.

The 'orderby' parameter is used to change the sorting of the returned objects.  It is normally left NULL by widget drivers.

The 'tree' and 'orderby_exp' parameters are normally left NULL by widget drivers, but could contain an expression and an array of expressions, respectively, that are precompiled selection constraints and sorting keys.  The expressions in question are expression trees of type 'pExpression'.

The returned value can be NULL if an error occurred or if the parent object *cannot* contain subobjects.  Otherwise, a pObjQuery reference is returned.

Example:

    pObjQuery qy;
    pObject subobj;
    qy = objOpenQuery(w_obj, "", NULL, NULL, NULL);
    if (qy)
    {
    while ((subobj = objQueryFetch(qy,O_RDONLY)))
        {
        /** do some stuff here with subobj **/

        /** close the subobj **/
        objClose(subobj);
        }
    objQueryClose(qy);
    }

#### objQueryClose(pObjQuery query) returns int
This closes an open query.  It also cancels pending results, if need be, if the calling module did not fetch them all.  It returns 0 on success and -1 on failure (but, in all cases on failure the query will be closed if it indeed was open).

#### objQueryFetch(pObjQuery query, int mode) returns pObject
This function is used to obtain a subobject returned from an open query.  It opens the object with the given 'mode', which is normally O_RDONLY for widget driver queries.  The returned pObject can then be used in objGetAttrValue() calls and the like.

This function returns an open object on success, or NULL if there are no more results available.

#### objClose(pObject obj) returns int
This function is used to close an open object.  All objects that a widget driver opens with objOpen() or objQueryFetch() must be closed using this function prior to the widget driver returning from its Render() function.

This function returns 0 on success or -1 on an error, but in all cases if the object was open, it will be closed by the objClose() call.

## VI Other Frequently-Used Utility Methods
The following is a list of methods that widget drivers might frequently use, but which are not specific to the OSML nor to the HTML generation subsystem.

### A.	Memory Manager Methods
Almost never use malloc() or free() in Centrallix code.  Instead, use one of the below sets of routines.

#### nmMalloc(int size)
#### nmFree(void* ptr, int size)
These are centrallix-lib memory manager wrappers.  This pair of methods are normally used for allocating fixed blocks of data, such as for "structs" and "unions".  The size of the block must be known at nmFree() time.  These methods are not compatible with the below nmSysMalloc() and nmSysFree().  It is not recommended to mix these functions with malloc() and free(), although doing so will not cause a program crash.  However, doing this could result in at the least inefficiency, and at the worst a serious memory leak.

#### nmSysMalloc(int size)
#### nmSysFree(void* ptr)
#### nmSysStrdup(char* ptr)
#### nmSysRealloc(void* ptr, int newsize)
These are more centrallix-lib memory manager functions that are designed to work with memory allocations which require realloc- type functionality or which work with blocks of data which are not typically consistent in size.  The size of the block does not need to be known at nmSysFree() time.  These methods are NOT COMPATIBLE WITH nmMalloc/nmFree NOR WITH malloc/free.  DO NOT MIX THEM!!!!  (e.g., what you malloc() with one set of methods, free() with the same set of methods).

In the HTML generation subsystem, the nmMalloc()/nmFree() methods should be used in conjunction with the functions which take the HTR_F_NAMEALLOC/HTR_F_VALUEALLOC routines.  Furthermore, the nmMalloc()d strings should be just long enough to contain the string and zero terminator, and no more.  Of course, strings need not be allocated with nmMalloc() if they are constants, and in that case the HTR_F_xxxALLOC flags shouldn't be supplied.

### B.	Session and Error Reporting Methods

#### mssError(int reset, char* modname, char* fmt, ...)
This function is used for error reporting.  It adds an error to the error stack for the current session.

Set 'reset' to 1 if the error is being "discovered" entirely by this module.  Set it to 0 if the error is in response to an error status being returned from another API function, such as NULL being returned from nmMalloc() or -1 being returned by the objGetAttrValue function.  Note that most modules don't add another message to the error stack upon detecting a memory allocation failure, since the nmMalloc() routine would have already added a generic 'memory allocation failed' message.

'modname' is an abbreviation for the module generating the error. For the HTML widget drivers, this is normall of the form "HTxx" where "xx" is the abbreviation of the module.

'fmt' is a format string that can contain '%d' and '%s' in order to substitute integer and string values, respectively from the variable argument list.

## VII Development by Prototyping
Probably one of the easiest ways of developing a new widget for Centrallix is via prototyping the widget in a simple DHTML page, first outside of the Centrallix server.  The following section details a recommended approach to developing a new widget for Centrallix.

### A.	Designing your Widget
The first step is normally to, either formally or informally, decide on a basic design for the widget.  One should consider some of the below issues when considering a design:

1.	What and how many layers should be used?  Enough layers should be used to make the widget function smoothly, but layer must be kept to a minimum, as they do use browser resources.  A widget that is absolutely positioned (via X and Y) MUST have at least one layer.  Nonvisual widgets typically do not have any layers.

2.	How will the widget look?  How will it interact with the user?

3.	What events (mouseup, mousedown, etc.) will need to be trapped?

4.	Will the widget receive mouse, keyboard, and/or data focus?  If keyboard focus, it should receive keyboard input.  Also, whether the widget receives keyboard focus or not, will it use hotkeys?

5.	Can the widget contain other visual subwidgets?  Can it contain nonvisual subwidgets? (the latter is necessary if the widget is to have Events).

6.	Will the size of the widget's contents change over time?  If so and if the widget can contain visual subwidgets, then it may need to set some values to interact with the pg_resize() mechanism.

Sometimes it can be helpful to draft the widget's design by simply creating a DHTML page outside of Centrallix that does some of what you need.  However, some widgets need the services of other Centrallix widgets in order to be functional.  In this case, the following process can be helpful.

### B.	Generating the First "Rough Draft"
To get started, copy a simple widget to a new file.  Go to the htmlgen directory, and either make a copy of htdrv_pane for a visual widget, or perhaps htdrv_execmethod for a nonvisual widget.  These widgets can provide simple starting points.

Edit the new file and do a file-wide search & replace on the module's prefix.  For the pane widget, for instance, replace all instances of "htpn" with your driver's prefix and "HTPN" with the upcase version of your driver's prefix.  Then, modify the Name and WidgetName strcpy()'s in the Initialize function to reflect your driver (see the section on Initialization for more information).

It is usually best to remove all functions, event handlers, and global script variables from the driver to get started.  It is usually also best to remove all but one CSS layer and to remove all of the HTML BODY code as well.  To get started with a visual widget, create one starting layer (the base layer) with one layer, say named "xy0base" (the 0 comes from the idcnt, as each widget must be uniquely numbered).

Change the pn_init() function call and definition appropriately to one to be used by your widget.  In this case, xy_init() would be appropriate.  Since pn_init() passes two layers in initialization, you may need to remove the reference to one of them since you are starting with just one layer  (in the pane widget, one layer is used as the container, and one outer layer to draw the 3-D border around the container, by using an HTML table).

Add the following code to the HTML body

    "<DIV id=xy0base><!--Code It Here--></DIV>"

so you can find the place to prototype your HTML code to design the visual aspect of the widget.  Of course, the 0 should come from the idcnt here as well, so that it matches that in the HTML header's CSS declarations.  And 'xy' should match the last part of the module's prefix (minus the 'ht' part; in this case the module's Initialize function would be called 'htxyInitialize()').

You will probably want to remove most of the objGetAttrValue() params from the Render() function in the widget, except those needed to position a visual layer on screen.

Finally, delete all log entries from the CVS header in the file except the dollar-sign-Log-dollar-sign line, of course, and change the author/date/etc. information in the file header.

To get the widget added to the system, edit Makefile and add the new widget's .o file to the XHTDRIVERS= line.  Don't include the path to the driver; the makefile does that automatically.  Then, edit lsmain.c and add the widget's Initialize function to the startup code there alongside the other calls to widget Initialize() functions.  Be sure to add it somewhere *after* the main htrInitialize() call.

Rebuild the Centrallix binary, and build a testbed .app file in the OS directory which creates a new widget of the appropriate type, embedded in appropriate other widgets to ensure proper testing and such.  Be sure to create only ONE new widget instance; otherwise the prototyping process can become more difficult.

Run Centrallix and load the test application, and save it to a file in the objectsystem.  Be sure NOT to save it as a '.app' file, rather use a '.html' extension!!!  Then, copy the html file to a second working copy in the same directory.  Don't modify the original, rather only modify the working copy.  You'll see why....

### C.	Prototyping the widget
Once you've generated the widget, edit the source to the working copy of the page.  Find the spot where the "Code It Here" HTML comment is, and use that to enter the basic HTML to code the widget.  Also, find the init function and where the layer is declared and add layers as needed.

You can view the page at any time by pointing Netscape to the working copy of the .html file via the running Centrallix server.

Once the widget is working well enough to be cemented into the system as a widget, do a "diff" between the working copy and the original rough draft copy of the .html pages.  This will tell you what you had to add to make the widget work.  Using those rough additions as a starting point, make changes to the widget driver to handle those needed additions and changes.  Then, recompile Centrallix.

Note that eventually it will be easier to simply make a minor change and recompile the server than to go through the prototyping process again.  But, during the initial development of a complex widget, this prototyping process can help a great deal.
