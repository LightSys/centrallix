Document:  HTML Generation Subsystem Update Design Spec
Author:    Greg Beeley (GRB)
Date:      July 17th, 2002
-------------------------------------------------------------------------------

A.  OVERVIEW....

    This document describes an important update to the HTMLGEN subsystem in
    Centrallix.  This update will actually bring the HTMLGEN subsystem closer
    to its original design (the current design is a simplified one), and will
    allow for several important pieces of functionality which will greatly
    extend the flexibility of the HTMLGEN subsystem and its widget modules.


B.  BENEFITS....

    The benefits of this update will be as follows:

        1.  Composite widget support.  This will permit widgets to be
	    functionally embedded within other widgets such that they are
	    actually an operational part of the other widget.  An example
	    of this is an editbox being used as a part of a combo dropdown
	    list box or a combo date/time widget (where the value can be
	    keyed in as well as selected from a dropdown list or calendar
	    dialog).  Another example will be scrollbars.

	2.  Auto-layout support.  Certain containers (such as menus) have
	    intelligence about how items should be arranged within them.
	    Thus, it is not meaningful for the app designer to specify
	    absolute (x,y) positions for the contained subwidgets.  This
	    update will allow the container to control certain aspects of
	    the geometry and positioning of subwidgets.

	3.  Auto-generated widgets.  Similarly to the composite widget
	    support, this update will allow a widget to auto-generate needed
	    subwidgets to enhance functionality.  An example of this might be
	    the auto-generation of connectors where needed in order to 
	    simplify application design, or the auto-generation of labels
	    for editboxes and the like.

	4.  Dynamically loadable widgets.  This will allow, for instance,
	    a htmlwindow widget to be loaded from the server into the 
	    running application without having to reload the page or jump to
	    another page.  It will potentially also allow, for instance, a
	    dynamic_server dropdown box or static table to be reloaded from
	    the server, thus gaining new or changed data values.
		
		a.  NOTE: the browser-compatiblity/feasibility of this has
		    *not* been tested.  Reloaded layers may or may not
		    cooperate with the rest of the application.

	5.  Dynamically created apps or app components.  This will allow,
	    for instance, a property sheet to be auto-generated for an object
	    based on that object's presentation hints and attribute list
	    *alone*.  The intermediate htmlgen page rendering step will allow
	    for this.

	6.  Themed widget generation.  A theme registry will be created that
	    will allow substitutions for some hard-coded values currently in
	    the DHTML widget drivers, including defaults, images, and certain
	    sizes.

    The drawbacks to this update will be:

	1.  Performance.  The performance of the htmlgen subsystem will be
	    negatively affected due to the extra steps needed to build the
	    necessary data structures.

    I feel the benefits gained will outweigh the performance drawback, by
    far.


C.  DHTML GENERATION PROCESS CHANGE OVERVIEW....

    Currently, the htmlgen subsystem uses two basic steps:

	1.  Nested calling of drivers to convert the data from the
	    objectsystem into the segmented page data (header, events, css
	    layers, init, cleanup, body, variables, etc.)

	2.  Outputting of the DHTML data, appropriately pieced together.

    The updated htmlgen subsystem will be a bit more complex, but individual
    widget drivers will not be more complex unless they need the functionality.

    The new procedure will be as follows:

	1.  The ht_render.c main subsystem file will read the app from the
	    objectsystem and build a tree structure in memory of the entire
	    app file, including widgets, types, subwidgets, and attributes.

	2.  Starting at the topmost widget and working down through the
	    tree depth-first, Verify() routines will be called for each 
	    widget in the tree - for example, the page driver's Verify()
	    routine will be called first to handle the top-level page
	    widget.  The Verify() routine may choose to do nothing (as
	    it currently behaves), or it may choose to do the following:

		a.  Do a quick sanity check on the widget attributes as
		    needed, or on what subwidgets are or are not present
		    in the in-memory tree.

		b.  Set attribute values on subwidgets (as an intelligent
		    container would do) in the in-memory tree (not in the
		    objectsystem itself).

		c.  Generate new widgets (such as connectors) into the
		    in-memory tree which represents the app file.

		d.  Generate new tightly-bound composite widgets into the
		    in-memory tree, to be used specifically by the widget
		    itself (such as an editbox within a calendar widget).
		    More about the implications of this later...

	3.  Whenever new widgets are added to the in-memory tree, they are
	    slated to have Verify() routines called on them as well.  These
	    Verify() calls may result in more widgets being added to the
	    tree in memory; this is OK.  The process will continue until
	    all widgets have been visited by a Verify() call.

	4.  Nested calling of driver Render() methods to convert the data
	    in the in-memory tree into the segmented page data (header,
	    events, init, cleanup, layers, etc.).

		a.  A new set of OSML-like API functions will be created to
		    handle the GetAttrValue() needs.  
		    
		b.  A substantial search-and-replace operation will commence
		    for all widget drivers to update them.

	5.  Outputting of the DHTML data, appropriately pieced together.


D.  OVERVIEW OF OTHER CHANGES....

    In addition to this change in the generation/render process, a second
    change will be made - when net_http receives a request for a widget 
    other than a page or frameset, it will pass it on to the DHTML generation
    subsystem to be rendered.  The htmlgen subsystem, recognizing that it is
    not a full page but rather an individual widget, will not generate the
    full page header and footer, but just that which is required for the
    layer to be meaningfully loaded.

    Finally, a theme registry will be created to allow widget modules to derive
    defaults and other otherwise-hardcoded values from, thus allowing the look
    and feel of the generated application to be changed without significantly
    affecting the basic operation of the application (and usability of the .app
    file as well).  The theme registry will consist of tabular data present in
    the objectsystem, and pointed to by an entry in the centrallix.conf file
    under a section labeled "ht_render".  A default theme will be specified in
    centrallix.conf, but the theme may be selected by the application, and can
    be overridden by the user.


E.  IMPACT....

    These changes will have the following impact:

        1.  All widget drivers will need to be updated to use the htmlgen
	    GetAttrValue() function instead of accessing the objectsystem
	    directly.

	2.  Widget objects (the w_obj) will no longer be passed into the
	    widget drivers as "pObject" types, but rather as an in-memory
	    tree structure node.

	3.  Widget drivers will no longer use any direct OSML access calls to
	    obtain attribute values or loop through subwidgets, but instead
	    will use functions in the ht_render subsystem itself to do these
	    things.  The functions will have a similar API to the OSML
	    functions.

	4.  The ht_render module will have to do some preparatory work on the
	    app to move it from objectsystem to memory before any widget 
	    drivers are called.

	5.  The ht_render module will need to implement the logic to do the
	    first Verify() stage before doing the Render() stage.

	6.  The htmlgen subsystem will need to know not to do a full page
	    rendering operation when told to render just a single subtree of
	    widgets not comprising an entire application (more on this later).


F.  COMPOSITE WIDGET INTERACTION

    This update introduces composite widget support, or the facility to embed
    one or more widgets logically within another so that the group of widgets
    act as a single widget.  Here we'll take a look at how those widgets will
    interact with one another.

	1.  Server-Side Interaction.


G.  DETAILS OF CHANGES....

    1.	Interface between widget driver modules and ht_render module.

	Driver Interface Functions:

	    a.	Verify()	This method will be responsible for verifying
				the attributes on the widget, set the widget's
				container offsets, create any needed
				subwidgets (e.g., for composite widgets and so
				forth), and set geometries (and/or other
				properties) on subwidgets if appropriate.

	    b.	Render()	This method retains its traditional function
				of generating the DHTML page data for the
				widget.

	    c.	Initialize()	This method is not in the driver structure,
				but is the standard initialization call that
				modules must all have.

	HTMLGen Interface Functions, page building:

	    a.

	HTMLGen Interface Functions, app structure access / modification:

	    a.


    2.	Structure format for storage of application data in the in-memory
	data structure for intermediate use.

	Format:

	    a.  int		Magic number
	    b.  void*		(reserved)
	    c.  int		Flags
	    d.  char[64]	Widget type (such as "widget/page")
	    e.  char[64]	Widget name
	    f.  int (4 ea)	Requested Geometry: x, y, width, height
	    g.  char (4 ea)	Flexibility: x, y, width, height (range 0-100)
	    h.	double (4 ea)	Internal flexibility computation, x/y/w/h
	    i.	int (4 ea)	Actual Geometry: x, y, width, height
	    j.	int (4 ea)	Container Offsets: top, bottom, left, right
	    k.  XArray		List of additional properties
	    l.  XArray		List of child widgets

	Flags:

	    1<<0 (1)		Verified -- drv->Verify() called

	Additional property structure.  This structure is generic enough that
	it may be useful for other things as well, as an extension of ObjData
	perhaps - could be included in obj.h.

	    a.	int		Magic number
	    b.	void*		(reserved)
	    c.	char[64]	Property name
	    d.	int		Data type (DATA_T_xxx)
	    e.	ObjData		Data value
	    f.	char[16]	Small string buffer for string data, unioned
				with storage for money, datetime, stringvec,
				and intvec type values.


    3.	Theme registry.

	Fields.  A theme consists of multiple records in the theme registry,
	so this field list builds only one record (a small part) of a theme.
	The implementation should also provide for a way to lookup theme
	registry values based on integer tags rather than string values, since
	the string values can consume much time to compare.  A widget driver
	would lookup the integer ID's during initialization, for instance.

	This means that the fields won't necessarily be implemented in a
	strictly tabular fashion - some kind of a tree structure will be
	needed, and it will be important to keep widget and property index
	numbers consistent across themes to preserve the performance benefit
	of integer ID lookup.

	    a.	char[8]		Theme name
	    b.	char[8]		Parent theme name (for inheriting values)
	    c.	char[64]	Widget type
	    d.	{struct}	Additional property structure, see above.
				This can include many aspects of a widget, not
				just properties.


    4.	Size determination.

	The application (or an individual container) will be able,
	potentially, to adapt to different sizes.  Thus, the sizing
	information will need to be passed to the main htmlgen render
	function.

	If the application is being loaded as a whole, a javascript 'startup'
	code segment will need to be present to link to the application by
	reading the window width/height from the browser and then launching
	the main application.  The htmlgen subsystem can do this by detecting
	if the width and height have been specified, and if not, deploying a
	small javascript segment to the client to determine the width and
	height and then re-launch the application.

	If a container is being reloaded, then the container will be
	responsible for sending the width and height information to the server
	when the layer in question reloads.  Similarly, if a new widget is
	being loaded, the routine loading the new widget will need to specify
	the size information.

	The width and height will be specified as HTTP parameters in the URL,
	using the following names:

	    cx__width	- width of the container or window
	    cx__height	- height of the container or window


    5.	Reloadable container functionality in HTMLGen.

	Reloadable containers will be implemented for container widgets whose
	contents are purely contained in exactly one layer which contains no
	other objects.

	When this is done, the open object passed to htmlgen will be the
	object of the container widget itself.  However, the container widget
	will not be re-rendered into the generated document; only its contents
	will be.  Nevertheless, the container widget will need to be built in
	the in-memory data structure so that the container widget driver's 
	Verify() function can (potentially) auto-configure the child widgets'
	geometry (and other) aspects.

	Before the container is reloaded, its contents will have to be
	cleared.  Because the contents can include widgets with global
	variable references, those references will have to be deleted via a
	recursive search mechanism through the container.  If the child
	widgets have "DeInit" methods on them, they are called.

	After the container is reloaded, it will have freshly uninitialized
	widgets within it.  A function "startup()" will be called in the
	container's document upon load, which will be analogous to the main
	document's startup() function, and generated in much the same manner.
	Because the script init lines in that function will be referencing
	objects in the DOM from an absolute perspective, a method will need to
	be provided to give the script init some context.  This will be done
	by passing a the container's global name to the htmlgen subsystem,
	passed as a URL parameter, as follows:

	    cx__parent	- reference to parent container document.

	That document will be given to the widgets as their parent context.
	The mechanism is already in place for the widgets to generate the
	correctly formatted script init lines.

	If the container has a "ContainerReloaded" method on it, that method
	will be called when the container's contents have been reloaded.


    6.	Dynamically loaded widget functionality.

	This is very similar to reloadable containers, with the exception that
	almost any dynamic widget can be dynamically loaded (static mode
	widgets are the exception, since they aren't built from layers).

	In this case, the container widget driver's Verify() and Render()
	functions will both be utilized.


    7.	HTMLGen interface to the remainder of Centrallix.

	The HTMLGen subsystem will continue to supply the remainder of
	Centrallix with one main function, htrRender().  However, this
	function will take some new parameters in order to properly support
	sizing, dynamic loading, and dynamic reloading.

	The new parameters are as follows:

	    a.	'w' (int)	The width, in pixels, of the area to be loaded
	    b.	'h' (int)	The height in pixels of the area to be loaded
	    c.	'parent' (char*) The parent document that the content is to be
				loaded into ("document" for most things, or 
				something else for dynamic loading).


    8.	New widget properties.

	There will be a few new widget properties available due to the dynamic
	loading support and to the sizing support.

	The following properties are used to manage sizing; some of them are
	existing properties but are listed here for completeness.  These are
	only applicable on visual widgets, and in some cases not all may apply
	to a given widget.

	    'x' (int)		The X coordinate of the widget in pixels
	    'y' (int)		The Y coordinate of the widget in pixels
	    'width' (int)	The width of the widget in pixels
	    'height' (int)	The height of the widget in pixels
	    'xflex' (int)	The flexibility of the X coordinate (0-100)
	    'yflex' (int)	The flexibility of the Y coordinate (0-100)
	    'widthflex' (int)	The flexibility of the width (0-100)
	    'heightflex' (int)	The flexibility of the height (0-100)

	Flexibilities determine the "exactness" needed for the given geometric
	parameter, and are always optional.  If omitted, they default to 0,
	complete precision, which is compatible with current Centrallix
	application behavior.  A value of 100 means complete flexibility to be
	resized and/or move if the application size is different from the page
	size.

	The 'width' and 'height' are now valid on a page object or frameset
	object; the system will use these values to determine how far off the
	actual window size is from the application's "design" size, and will
	adjust the contents accordingly.  Widgets with higher flexibility will
	adjust more to a change in page size, relative to those with lower
	flexibility settings.

	The page widget will have the following five geometric properties now
	as well to control the size of the application.

	    'autoadjust' (yes/no) Whether the system will automatically adjust
				the browser window size if the window is
				larger than the maximum size or smaller than
				the minimum size.  If set to 'no', the app
				will end up either being smaller than the
				visible area or will take up more room than is
				visible, resulting in scrollbars.
	    'minwidth' (int)	The minimum width, in pixels, of the app
	    'maxwidth' (int)	The maximum width, in pixels, of the app
	    'minheight' (int)	The minimum height, in pixels, of the app
	    'maxheight' (int)	The maximum height, in pixels, of the app

	In order to allow other layout methods to be used, the following
	properties are now valid on containers:

	    'layouttype' (enum)	The method for arranging child objects within
				the current object.  Can be one of 'free' (the
				default), 'vertical', 'horizontal', or
				'tabular'.

	    'marginspacing' (int) The padding around the outer edges of the
				inside of the container.  Affects the position
				of the (0,0) origin within the container.
				Defaults to 0.

	    'innerspacing' (int) The padding between widgets within the
				container.  Only meaningful for layout types
				other than 'free'.  Defaults to 0.

	The 'free' layout type indicates that objects are placed in the
	container according to their (x,y) positions relative to the upper
	left corner of the container.  With a 'vertical' layout type, the 'x'
	coordinate retains its traditional meaning, but the 'y' coordinate
	then becomes an ordered number (0 to n-1) indicating the order in which
	the widget is to appear, vertically, within the container.  Nonvisual
	widgets are ignored for this purpose.

	The 'horizontal' layout type is the reverse of 'vertical': the 'y'
	coordinate retains its normal pixel-based meaning, and the 'x'
	coordinate provides the order (0 to n-1) in which the widget appears
	horizontally within the container.  Finally, the 'tabular' layout type
	abstracts both the 'x' and 'y' and creates a 'grid' in the container,
	where widgets are placed in the grid from (0,0) to (n-1,n-1) in ordered
	fashion (where (2,2) is the third column across and third column down
	from the top).

	A given widget cannot occupy more than one cell in a 'tabular' layout
	situation (no equivalent of 'rowspan' or 'colspan'), for simplicity at
	the current time.  Those features may be added in later.

	Flexibility settings are valid for all layout methods.


    9.	Flexibility calculation.

	This section details how the width, height, x, and y are computed from
	the requested coordinates and the given flexibility settings, and only
	applies to containers using the default free-layout method (the
	default).

	When a container is larger or smaller than anticipated, there will be
	slack, to be made up for, or too little room, requiring compression of
	the visible widgets.  Call these amounts the 'xdelta' and 'ydelta',
	which are the difference between the requested size and the design
	size as specified in the application.

	The amount that a widget's width and height are adjusted to make up
	for the xdelta and ydelta are proportional to the widget's flex
	setting relative to other widgets' flex settings in that container.
	The calculation process involves computing which widgets are "next to"
	which other widgets, which is simplest for the 'tabular' layout method
	and most complex for the 'free' layout method.

	[...incomplete...]


H.  IMPLEMENTATION ORDER....

    Obviously this document details some very extensive feature additions, and
    not all features need be added immediately.  This section seeks to list
    the (approximate) priorities of the various components.

    1.	First priority is the multi-phase rendering process, which provides
	the core functionality for many of these features.  This will involve
	porting the widget driver modules away from direct OSML access to
	access the in-memory tree using ht_render API functions instead.

    2.	Next priority is the addition of further ht_render API functions to
	allow widget modules to create, on the fly, child widgets in the tree.
	This provides for the composite widget support as well as
	automatically generated connectors and such in certain situations.

    3.	Third would probably be the automatic size and parent determination
	mechanism, which involves issuing a very short page to the client if
	the size data is not specified.  This short page will contain a short
	javascript snippet which will obtain the page size and do an automatic
	reload with the size specified.  It will also automatically resize the
	browser window if the window is too large or too small.

    [...incomplete...]

