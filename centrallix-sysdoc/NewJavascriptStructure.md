Document:   New JavaScript Structure for Centrallix
Date:	    30-Oct-2014
Author:	    Greg Beeley (GRB)
===============================================================================

OVERVIEW...

    A need exists to update the object and widget model used by the Centrallix
    platform's JavaScript.  Currently, the object model is more reminiscent of
    C-style namespace and object controls (via functions in a flat namespace,
    made unique via module function prefixes).  This document describes an
    updated system for Centrallix's JavaScript.


GLOBAL OBJECT MODEL...

    All Centrallix-specific functionality will be accessed through a global
    called $CX.  This top-level global will have the following members:

	$CX.Widget - an object containing one object for each type of widget
		supported by the Centrallix platform.  Each of these sub-
		objects will be a constructor for the widget in question.

	$CX.Util - an object containing a set of utility functions that
		widget modules can utilize.

	$CX.Properties - an object containing informational properties:

		$CX.Properties.AKey - the session-group-app key.
		$CX.Properties.User - the currently active user

	$CX.Tree - the widget tree's root (i.e., a page widget).

	$CX.Namespaces - the list of active namespaces

	$CX.Time - information about the current time and date, including:

		$CX.Time.Init.Server - the server's time at app init
		$CX.Time.Init.ServerTZ - the server's time zone at app init
		$CX.Time.Init.Client - the client's time at app init

	$CX.Templates - a list of currently active templates

	$CX.Scripts - a list of currently loaded scripts

	$CX.Endorsements - a list of active security endorsements

	$CX.Apps - a list of applications in the current app group (or,
		essentially, a list of browser windows that are linked together
		as a part of the current app group).


WIDGET OBJECT MODEL...

    Each widget object (sub-object of $CX.Widget) will consist of a constructor
    function wrapped in a closure.  The constructor itself will be more
    traditional in nature, having public/privileged/private components to it,
    but the closure wrapping allows us to have private static members for each
    class of widget.

    The widget objects in the widget tree will all be synthetic objects
    created by these widget constructors, rather than DOM objects.  All DOM
    DIVs, IFRAMEs, and IMGs associated with a given widget will have a property
    called "cxwidget" which points back to the widget object.  The widget
    objects will also have references to the DOM, which will vary on a
    per-widget basis, but the consistent name for the highest level DOM
    reference (typically the outermost DIV) for a widget object will be "DOM".

    DOM objects may be created by the widget constructor, or they may be
    provided (imported) if they were created, say, by the server during web
    app generation.
