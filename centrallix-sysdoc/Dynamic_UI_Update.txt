Document:   Dynamic UI Update
Author:	    Greg Beeley (GRB)
Date:	    18-Nov-2013
-------------------------------------------------------------------------------

OVERVIEW...

    To take the Centrallix application server to the "next level", one very
    important aspect will be to make the UI more dynamic in nature.  This
    encompasses several areas.

    A.	Consistent handling of all properties as dynamic client-side maintained
	properties.

	Infrastructure needs to be put in place to manage ALL widget properties
	as client-side maintained properties.  This means that properties will
	not have to have special case checks for dynamic runclient() type
	expressions, but rather all properties will support this feature,
	unless specifically ignored or rejected by the widget itself.

    B.	Consistent passing of properties to widget initializers.

	Widget initializers will no longer have a custom string of parameters
	passed to them, but instead the properties will be handled by more of
	a general-purpose mechanism.

    C.	Updatable widget properties.

	Currently, once a widget is rendered, such properties as x, y, height,
	and so forth, cannot be changed, due to the rendering method.  What is
	needed is a way for these properties to be modifiable on-the-fly.

	This will involve the widget HTML drivers deriving a set of additional
	properties from the provided widget settings, and passing all of those
	properties to the client.  When the underlying data changes, the
	widget driver will be invoked to generate the entire set of properties
	that the widget needs, and then pass that on to the client via JSON
	AJAX data.

    D.	Templated widgets.

	These sorts of templates are different from the WGTR widget templates,
	and will serve to define the basic HTML and CSS structure of each
	widget.  This structure will be used both by the server to generate
	the initial app, as well as delivered to the client so that the
	client can create and modify widgets on-the-fly.

    E.	Dynamic resizing.

	Currently, the APOS module computes dimensions and positions for
	widgets at the time the app is rendered, and these geometries cannot
	be changed until the app is reloaded.

	What is needed is for APOS to take the app layout and generate a set
	of formulas -- expressions -- that define the sizing and positions of
	the widgets in the app.  These expressions can be used dynamically on
	the client to adjust the widgets when the app or parts thereof are
	resized.

    F.	Animations.

	Various animations will be needed for different geometry and
	visibility changes.  For example, opening and closing windows,
	resizing or moving a pane or window, etc.

    G.	User-Agent compatibility.

	The use of jQuery/Mootools will be needed for better compatibility
	with a variety of user agents, including Firefox, Chrome, Opera,
	Safari, and even MSIE.

    H.	Universal decorative properties.

	Some decorative properties will need to be made universal across all
	widgets that they would reasonably apply to, including drop shadows,
	corner/border radius, border colors and styles, and more.

