WidgetTree.txt
    - Matt McGill July '04

SYNOPSIS
An overview of the widget tree module (wgtr), and how it affects widget
authoring.

DESCRIPTION OF WGTR MODULE

The nature of a Centrallix Application, and the way in which it is 
defined, makes it easy to represent in a tree structure, where each wdiget is
a node in a 'widget tree.' Previously, this 'widget tree' was only represented
inside the structure parsing module, and by the inherently tree-like
application definitions, and indirectly (in a browser-dependent format)  by 
the DOM on the client-side.

The Wgtr module provides a way of representing 'widget-trees' explicitly,
and a means of creating, accessing, and modifying them. On the client-side,
the Wgtr module provides a browser-independent way of representing the
structure of an application and accessing its widgets.


REASONS FOR THE WGTR MODULE

The main motivation for the Wgtr module is flexibility. Previously, an
application was rendered by calling a set of widget-specific functions
recursively, accessing the application definition directly through the OSML.
Accessing the application definition through the OSML does not allow for easy
manipulation of the application definition prior to actually rendering that
application and sending it to the client.

A more flexible approach is to build a widget-tree data structure out of the
application definition in a step seperate from the actual rendering process.
Once an application definition has been parsed into its corresponding tree,
the tree can be modified in whatever ways are desired before being rendered.
This opens up a number of possibilities:
    * Component widgets
	Support for user-defined widgets that make use of existing widgets
	and other component widgets can be implemented - the component widget
	driver can create the widgets specified by the user and graft them
	into the tree.
    * Auto-positioning
	Once the tree has been generated, a pass through the tree could be
	made that would allow widgets to adjust their size and position,
	and the sizes and positions of other nodes, based on the size of the
	application's containing window and constraints specified by the user.
	This could potentially lift some of the burden of precisely locating
	each widget off the application-author.
    * Verification
	After the initial generation of a tree, an additional pass through
	the tree could allow widget drivers to inspect their widget nodes and
	identify errors. If possible the errors can be corrected. If not, a
	message can be printed indicating the error and the rendering process
	can be skipped for that tree, instead of interrupting the rendering
	process part-way through.
    * Auto-generation
	More of the burden of application authoring could be lifted by
	automating common and repetitive tasks. For example, there may be
	instances where connector widgets can be generated automatically to
	save developer time.
    * A well-defined way of representing the widget hierarchy client-side
	Before the Wgtr module, widget objects on the client-side were
	referenced by global variables, and hierarchy information was
	represented by a combination of the DOM and extra client-side code.
	The Wgtr module on the client-side preserves the ability to
	reference widget objects by global variables, but embeds all hierarchy
	information directly into those objects, and provides a browser-
	independent way of retrieving that information.
    * A framework in which to place Interface support
	To properly support component widgets, interface support is being
	worked into Centrallix as this document is being authored. A stand-
	alone widget-tree data structure provides a convinient framework for
	storing interface information on a per-widget basis. When pushed
	down to the client with the application, this interface information
	will allow an application to interact with dynamically loaded
	component widgets in a well-defined manner. The container of a
	dynamically loaded component widget can, for example, verify that
	a loaded component meets some required set of functionality by
	checking that it implements certain interfaces. The client-side Wgtr
	module provides this functionality.
    * Ability to merge dynamicly-loaded components with existing Apps
	When a new component, perhaps representing an entire window, is
	loaded dynamically into an existing application, its tree can be
	grafted onto the main application tree, effectively making that
	component a part of the application (provided all of its interaction
	with the application is through the Client-side Wgtr module). For
	example, a component containing a stock form can be dynamically loaded
	into an application, and simply walk up the tree to find its parent
	osrc widget.


SERVER-SIDE WGTR VERSUS CLIENT-SIDE WGTR

The Wgtr module is actually comprised of multiple 'sub-modules.' The main
module resides on the server, and is responsible for creating widget trees,
whether from application definitions, on the fly, or via component
declarations. The server-side module provides all the functionality necessary
for creating, modifying, and rendering a widget-tree.

It is a long-term goal to add multiple deployment methods to Centrallix.
Currently, applications are rendered to the user in DHTML, but support for
additional deployment methods, like XUL, will eventually be included. The
server-side Wgtr module is meant to encapsulate everything widget-related that
is independent of the deployment method that will be used. For example, the
'verification' step of the rendering process is indenepent of the deployment
method, and therefore the responsibility of the widget drivers in the Wgtr
module. The default initialization of a widget also falls in this category,
along with the responsibility of linking interface information with  widgets.

In addition to the server-side 'sub-module,' there is a client-side Wgtr sub-
module for the DHTML deployment method. Presumably there will be similar
client-side modules for other deployment methods as well. The client-side
Wgtr module is less concerned with the creation of widget-trees than the
server-side is, and more concerned with offering a well-defined method for
accessing widget properties, events, actions, and so on.

This client-side module is meant to be a tool for widget authors, and a
context for dynamically loaded components to exist in.

*******************
*  SERVER-SIDE    *
*******************

DESCRIPTION OF MODULE INTERFACE

    WGTR CREATION AND DESTRUCTION
	pWgtrNode wgtrParseObject(pObjSession s, char* path, int mode, int permission_mask, char* type)
	    Parses an OSML object into a widget tree. When each node is
	    created, it's corresponding driver's New() function is called 
	    do perform widget-specific initialization.

	pWgtrNode wgtrParseOpenObject(pObject obj)
	    Does the same as the above, but given an open OSML object
	    rather than a path to said object.

	void wgtrFree(pWgtrNode tree)
	    Frees all memory associated with a widget tree

	pWgtrNode wgtrNewNode(	char* name, char* type, 
				int rx, int ry, int rwidth, int rheight,
				int flx, int fly, int flwidth, int flheight)
	    Creates a single widget node, with the given parameters.
	    Additional properties can be added with wgtrAddProperty(). The
	    corresponding widget driver's New() function is called to handle
	    any widget-specific initialization.

    ACCESSORS
	int wgtrGetPropertyType(pWgtrNode widget, char* name)
	    Retrieves the Centrallix of a given widget property.

	int wgtrGetPropertyValue(pWgtrNode widget, char* name, int datatype, pObjData val)
	    Retrieves the value of a given widget property.

	char* wgtrFirstPropertyName(pWgtrNode widget)
	    Get the name of the first non-standard widget property. Standard
	    widget properties are name, type, and default geometry-related
	    properties

	char* wgtrNextPropertyName(pWgtrNode widget)
	    Get the name of the next non-standard widget property. Returns
	    null when the entire list has been cycled through.

    MOIDFIERS
	int wgtrAddProperty(pWgtrNode widget, char* name, int datatype, pObjData val)
	    Add a property to a widget.

	int wgtrDeleteProperty(pWgtrNode widget, char* name)
	    Remove a property from a widget.

	int wgtrSetProperty(pWgtrNode widget, char* name, int datatype, pObjData val)
	    Set the value of a property on a widget.

	int wgtrDeleteChild(pWgtrNode widget, char* child_name)
	    Delete a child node of a widget.

	int wgtrAddChild(pWgtrNode widget, pWgtrNode child)
	    Add a child to a widget.

	pWgtrNode wgtrFirstChild(pWgtrNode tree)
	    Return the widget's first child node.

	pWgtrNode wgtrNextChild(pWgtrNode tree)
	    Return the widget's next child node.

    MISC FUNCTIONS
	pObjPresentationHints wgtrWgtToHints(pWgtrNode widget)
	    Converts a widget node into a presentation hints structure.
	    This is primarily for cases where hints information might be
	    embedded in an application definition, to provide an easy way to
	    convert it to a proper hints structure.

	pExpression wgtrGetExpr(pWgtrNode widget, char* attrname)
	    For returning an attribute as a compiled expression.

    VERIFICATION FUNCTIONS
	int wgtrVerify(pWgtrNode tree)
	    Verifies a widget tree. The verification process consists of
	    verifying every node in the tree. New nodes can be added during
	    the process, and these must be verified as well for the process
	    to be completed.

	int wgtrScheduleVerify(pWgtrVerifySession vs, pWgtrNode widget)
	    Used primarily within a widget driver's Verify() function for
	    scheduling a node that has just been added for verification.

	int wgtrCancelVerify(pWgtrVerifySession cs, pWgtrNode widget)
	    Used to cancel a pending verification, perhaps before deleting
	    a node during the verification process.

    WGTR DRIVER-RELATED FUNCTIONS
	int wgtrRegisterDriver(char* name, int (*Verify)(), int (*New)())
	    Should be called by each widget driver to register itself with
	    the Wgtr module.

	int wgtrAddType(char* name, char* type_name)
	    Once a widget driver has registered with the Wgtr module, it needs
	    to inform the module which widget types it is responsible for. One
	    widget driver may support multiple types - for example, the drop-
	    down widget supports widget/dropdown and widget/dropdownitem.

	int wgtrAddDeploymentMethod(char* method, int (*Render)())
	    Each deployment method should register its Render() function with
	    the Wgtr module.

	int wgtrRender(pFile output, pObjSession obj_s, pWgtrNode tree, pStruct params, char* method)
	    Renders a widget tree using the given deployment method.

WGT DRIVERS VERSUS HT DRIVERS

    The htmlgen system was originally responsible for some tasks that were not
    actually tied specifically to DHMTL. For example, verifying a widget tree
    is a largely deployment-independent operation. With the addition of the
    WGTR module, there was a better place to put that deployment-independent
    code - widget drivers in the WGTR module.

    With this change comes a change in terminology. 'Widget drivers' are now
    the drivers that are a part of the WGTR module, responsible for handling
    all widget-related deployment-independent code. 'HT drivers' are the old
    widget drivers - the code responsible for generating DHTML for a widget.
    
WIDGET DRIVERS
    Widget drivers are located in the wgtr/ folder of the centrallix cvs
    module, and have a wgtdrv_ prefix.

    Each widget driver should contain three functions: an 'initialize'
    function, a 'verify' function, and a 'new' function.

    * 'Initializate' function
	The Initialize function for a widget driver is similar to the one for
	an HT driver - any initialization code that might be needed for that
	driver should be placed here. The function takes no parameters.

	The function is also responsible for registering with the WGTR module
	via a call to wgtrRegisterDriver, and for declaring which widget types
	should be associated with that widget driver.

	Currently this function is called from wgtrInitialize().
    * 'New' function
	The 'New' function is called any time a new widget node of a type
	associated with the widget driver is being created by the WGTR module.
	The function receives the node being created, after the WGTR module
	has initialized as much of the structure as it can. The 'New' function
	is the place to do things like setting flags and declaring supported
	interfaces. For example, all non-visual widgets should have the
	WGTR_F_NONVISUAL flag set here.

	A widget driver can declare that a node implements an interface by
	calling wgtrImplementsInterface(), and passing in the node and a
	string referencing the interface.
    * 'Verify' function
	The 'Verify' function takes the current verification context as a
	parameter, and is the place for deployment-independent code that
	checks to make sure a given widget tree is valid, and does automatic
	tweaking of nodes in the widget tree. For example, auto-positioning
	would be done here, and checks for all required parameters would also
	be done here (this is currently done in the HT drivers, if it's done
	at all, but should be moved here). For a good example, see the formbar
	widget. During the verification step, the formbar actually creates an
	entire child sub-tree, along with appropriate connectors.

THE VERIFICATION PROCESS
    The Verification process is a pre-processing step that is performed on a
    widget tree prior to rendering that tree. Widgets can be tweaked based on
    their positions in the tree, new widgets can be added, and problems can be
    detected. The wgtrVerify() performs the verification process on a widget
    tree.

    When wgtrVerify() is called, the first thing it does is create a
    'verification context', represented by a WgtrVerifySession struct. This
    struct groups together all the information necessary for the verification
    process - the widget tree being verified, the verification queue, and
    the current widget, among other things.

    The verification queue is then built by traversing the tree recursively
    and adding each node to the queue as it is encountered. Then for each node
    in the queue, its driver is looked up and its Verify() function is called,
    until the queue is empty. New nodes can be added to the tree, and should
    be added to the verification process as well with wgtrScheduleVerify(). If
    a node modifies an already-verified node, it can reschedule that node for
    verification, but extreme care should be taken to avoid infinite loops if
    this is done.

RENDERING A WIDGET TREE
    Previous to the addition of the WGTR module, net drivers would call the
    htrRender() function directly to render an application. This is not
    desirable, since Centrallix is meant to eventually support multiple
    deployment methods.

    To keep the rendering process modularized and abstracted, the rendering of
    a widget tree now goes through the WGTR module - each deployment method
    should register its Render() function with the WGTR module via a call to
    wgtrAddDeploymentMethod().

    When a net driver needs to render an application, it now calls
    wgtrRender(), passing along the tree to be rendered and a string
    representing the deployment method to use. The WGTR module is responsible
    for looking up and calling the appropriate Render function.

ITERATORS
    Support for iterators to iterate through trees was planned for the WGTR
    module, and has been stubbed out; however, the iterator functionality has
    not yet been implemented. If it proves not to be worth the effort
    required, the stubs should be removed.

*******************
*  CLIENT-SIDE    *
*******************

DESCRIPTION OF INTERFACE

These functions can all be found in ht_utils_wgtr.js.

* wgtr_internal_Print(msg, indent), wgtr_internal_Debug(msg, indent)
    These functions were used for some debugging, and are temporary. They're
    coming out in the near future (if they aren't already gone), so don't use
    them. They're not even browser-independent.

* wgtrAddToTree(obj, name, type, parent, is_visual)
    * obj:	    the object being added to the tree
    * name:	    the name of the widget (eval(name) should be equivalent to 
		    obj)
    * type:	    the type of the widget, ex. 'widget/connector'
    * parent:	    the parent object of the widget
    * is_visual:    true if widget is visual, false otherwise

    This function is used for constructing the widget tree during the startup
    process. It adds some properties prefixed with 'Wgtr' to obj, effectively
    grafting obj into the widget tree.

* wgtrGetProperty(node, prop_name)
    * node:	    the node from which to get the property
    * prop_name:    string containing the name of the property to retrieve

    This function returns the value of a client-side property of the given 
    widget. Returns the value of the property, or null on error.

* wgtrSetProperty(node, prop_name, value)
    * node:	    the node to set the property on
    * prop_name:    the name of the property to set
    * value:	    the value to assign to prop_name

    This function assigns a value to a client-side property of a widget tree.
    The property must already exist - the function will not add a property to
    a widget. Trying to set a property that does not exist results in failure.
    The function returns true if the property is successfully set, false
    otherwise.

* wgtrGetNode(tree, node_name)
    * tree:	    root of the widget tree containing the sought-after node
    * node_name:    name of the node to retrieve

    This function walks the tree attempting to find the requested node.
    Returns the node on success, null on error. Note that this isn't terribly
    useful right now, as all nodes are globally accessible. It's likely that
    this could be removed.

* wgtrAddEventFunc(node, event_name, func)
    * node:	    the widget node being manipulated
    * event_name:   string  containing the name of the event to add a function
		    for
    * func:	    function to be added

    This function addes a function to a wiget's list of functions to trigger
    for a given event.

*wgtrWalk(tree, x)
    * tree:	    the widget tree to traverse
    * x:	    indentation level

    This function is for debugging purposes. It walks the widget tree,
    printing information about each node in order to verify that the tree was
    constructed correctly.

BUILDING THE CLIENT-SIDE TREE

    The client-side widget tree is build immediately after the server-side
    widget tree has been rendered via the call to htrRenderWidget() on the
    root node of the tree in htrRender(). htr_internal_BuildClientWgtr() is
    the function responsible for building the tree.

    The client-side widget tree building code on the client-side is contained
    in the function 'build_wgtr()'. This function is generated via a series of
    calls to htrAddScriptWgtr(), each of which adds a line. On the client,
    build_wgtr() is called at the end of the startup() function.

    htr_internal_BuildClientWgtr() declares a local variable to be used
    in the function as a stack for keeping track of the current parent node.
    Then it calls htr_internal_BuildClientWgtr_r(), which recursively adds all
    the code necessary to link the global objects that are the product of the
    various xx_init() calls for the widgets into a tree. Dummy objects are
    created to be used as nodes when a widget is not associated with an
    object.

PURPOSES OF THE CLIENT-SIDE WIDGET TREE
    
    The client-side widget tree is at this point largely unused. It was
    designed in such a way that it would peacefully coexist with rest of the
    code, and link things together without breaking anything.

    The client-side WGTR code currently lacks important functionality, and is
    not usefull without that functionality. The client-side WGTR code is meant
    to provide a browser-independent way of representing the relationships
    among the widgets in an application. To make the module useful,
    functionality for retrieving this information needs to be included. For
    example, functions that return the visual and non-visual parents of a
    widget, or provide access to a widget's children,  might be useful.

    Making full use of the client-side WGTR module would require significant
    modifications to the existing widget set. While this might not be a bad
    idea at *some* point in the future, it's not critical. However, authors
    are encouraged to make use of the fact that the information about the
    relationships among widgets is now embedded in the widget objects
    themselves, when coding new widgets.
