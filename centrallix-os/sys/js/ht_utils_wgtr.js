//
// Widget Tree module
// (c) 2004-2014 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

// wgtrNamespace - an object representing a namespace of object names,
// typically a complete widget tree, or perhaps a subset of widgets inside
// a widget/repeat.
function wgtrNamespace(namespace_id, parent_namespace)
    {
    // Set up the namespace
    this.NamespaceID = namespace_id;
    this.WidgetList = {};
    this.ParentNamespace = parent_namespace;
    }


// wgtrSetupTree - walk a given widget tree, and set up the wgtr properties.
// Pass in the declared tree, and this thing returns the actual tree of real
// widgets.
//
function wgtrSetupTree(tree, parent)
    {
    var w;
    var nsobj;

    // Recursively set up the widget tree, connecting the tree with
    // the DOM.
    w = wgtrSetupTree_r(tree, null, parent);

    // Setup any composite nodes for repeated widgets.
    wgtrSetupRepeats(w);

    return w;
    }


function wgtrSetupTree_r(tree, ns, parent)
    {
    var _parentobj = parent?parent.obj:null;
    var _parentctr = parent?parent.cobj:null;

    // Setup the DOM object representing this widget.
    if (!tree.obj)
	tree.obj = {};
    else if (tree.obj == 'window')
	tree.obj = window;
    else
	tree.obj = document.getElementById(tree.obj);
	//with (tree) tree.obj = eval(tree.obj);

    // Setup the DOM object that will contain sub-widgets of this widget (this
    // could be an inner DIV, for instance).
    var _obj = tree.obj;
    if (!tree.cobj)
	tree.cobj = _obj;
    else if (tree.cobj == '_parentctr')
	tree.cobj = _parentctr;
    else
	with (tree) tree.cobj = eval(tree.cobj);

    // Create and/or switch to a new namespace if required
    if (tree.namespace && (!ns || tree.namespace != ns.NamespaceID))
	{
	if (!pg_namespaces[tree.namespace])
	    pg_namespaces[tree.namespace] = new wgtrNamespace(tree.namespace, ns);
	ns = pg_namespaces[tree.namespace];
	}

    // Initialize the widget and its sub-widgets.
    wgtrAddToTree(tree.obj, tree.cobj, tree.name, tree.type, _parentobj, tree.vis, tree.ctl, tree.scope, tree.sn, tree.param, ns);
    if (tree.sub)
	for(var i=0; i<tree.sub.length; i++)
	    wgtrSetupTree_r(tree.sub[i], ns, tree);
    return tree.obj;
    }


// wgtrReEvaluate - on the change of an expression's constituent properties,
// re-evaluate the expression as a whole.
function wgtrReEvaluate()
    {
    }

// wgtrWatchParams - look for server parameters supplied as expressions, and
// set up the automatic re-evaluation infrastructure for those expressions.
function wgtrWatchParams(node, paramlist, callback)
    {
    var _context = node.__WgtrNamespace;
    var _this = node;
    window.__cur_exp = null;

    // Set up all expression-based params requested.
    for(var i=0; i<paramlist.length; i++)
	{
	var paramname = paramlist[i];

	for(var p in node.__WgtrParams)
	    {
	    var param = node.__WgtrParams[p];

	    // A matching expression-based parameter?
	    if (p == paramname && param && typeof param == 'object' && param.exp)
		{
		// Call function to obtain initial value
		//param.val = param.exp(_this, _context);

		// Watch properties that comprise this exp
		for(var i=0; i<param.props.length; i++)
		    {
		    var prop = param.props[i];
		    }
		}
	    }
	}
    }


// wgtrSetupRepeats - sets up composite tree nodes for repeated widgets.  This
// allows repeated widgets to be referred to in aggregate - sending an Action
// to all at once, receiving events from all, or retrieving properties from all
// using aggregate functions like min(), max(), etc.
function wgtrSetupRepeats(root)
    {
    wgtrSetupRepeats_r(root, null);
    }

function wgtrSetupRepeats_r(node, rptnodelist)
    {
    // Are we in a repeat widget?
    if (node.__WgtrType == "widget/repeat" && rptnodelist == null)
	rptnodelist = {};

    // Set up all child widgets first, so we have the list from them.
    for(var i=0; i<node.__WgtrChildren.length; i++)
	{
	var sublist = rptnodelist?{}:null;
	wgtrSetupRepeats_r(node.__WgtrChildren[i], sublist);

	if (rptnodelist)
	    {
	    for(var n in sublist)
		{
		if(rptnodelist[n])
		    rptnodelist[n] = rptnodelist[n].concat(sublist[n]);
		else
		    rptnodelist[n] = sublist[n];
		}
	    }
	}

    // If this is a repeat node, add the virtual (composite) nodes to it
    // so that the repeated nodes can be referenced en masse, via actions,
    // events, and aggregate functions like min() and max().
    if (node.__WgtrType == "widget/repeat")
	{
	for(var nodename in rptnodelist)
	    {
	    var rptnode = new wgtrRepeatNode(nodename, rptnodelist[nodename]);
	    wgtrAddToTree(rptnode, null, nodename, "widget/repeat-composite", node, false, false, null, null, null, node.__WgtrNamespace);
	    }
	}

    // Add this node to the list
    if (rptnodelist)
	{
	if (typeof rptnodelist[node.__WgtrName] == "undefined")
	    rptnodelist[node.__WgtrName] = [];
	rptnodelist[node.__WgtrName].push(node);
	}
    }


function wgtr_rn_Action(aparam, actionname)
    {
    var rval;
    for(var i=0; i<this.nodelist.length; i++)
	{
	rval = this.nodelist[i].ifcProbe(ifAction).Invoke(actionname, aparam);
	if (isCancel(rval)) return rval;
	}
    return 0;
    }


function wgtr_rn_ValueChanging(srcobj, prop, oldval, newval)
    {
    var newprops = [];
    var oldprops = [];

    for(var i=0; i<this.nodelist.length; i++)
	{
	if (this.nodelist[i] != srcobj)
	    {
	    newprops.push(wgtrGetProperty(this.nodelist[i], prop));
	    oldprops.push(wgtrGetProperty(this.nodelist[i], prop));
	    }
	else
	    {
	    newprops.push(newval);
	    oldprops.push(oldval);
	    }
	}
    return this.ifcProbe(ifValue).Changing(prop, newprops, true, oldprops, true);
    }


function wgtr_rn_Value(propname)
    {
    var props = [];

    for(var i=0; i<this.nodelist.length; i++)
	{
	props.push(wgtrGetProperty(this.nodelist[i], propname));
	wgtrWatchProperty(this.nodelist[i], propname, this, wgtr_rn_ValueChanging);
	}
    return props;
    }


function wgtr_rn_NewEvent(eventname)
    {
    for(var i=0; i<this.nodelist.length; i++)
	{
	this.nodelist[i].ifcProbe(ifEvent).Hook(eventname, wgtr_rn_Event, this);
	}
    }


function wgtr_rn_Event(eparam)
    {
    return this.ifcProbe(ifEvent).Activate(eparam._EventName, eparam);
    }


// wgtrRepeatNode - create a "repeat composite" nonvisual widget
function wgtrRepeatNode(nodename, nodelist)
    {
    this.nodelist = nodelist;
    this.wgtr_rn_ValueChanging = wgtr_rn_ValueChanging;
    ifc_init_widget(this);
    var ie = this.ifcProbeAdd(ifEvent);
    ie.SetNewEventCallback(wgtr_rn_NewEvent);
    var ia = this.ifcProbeAdd(ifAction);
    ia.SetUndefinedAction(wgtr_rn_Action);
    var iv = this.ifcProbeAdd(ifValue);
    iv.SetNonexistentCallback(wgtr_rn_Value, null);
    }


// wgtrAddToTree - takes an existing object, and makes it a widget tree node,
// grafting it in to a tree as the child of the given parent.
function wgtrAddToTree	(   obj,	    // the object to graft into the tree
			    cobj,	    // the container for subobjects
			    name,	    // the name of the object
			    type,	    // the widget type 
			    parent,	    // the parent node of this object
			    is_visual,	    // is the node a visual node?
			    is_control,	    // a control node?
			    scope,	    // scope of widget -- local vs application vs session
			    scopename,	    // name of widget to use in extended scope
			    params,	    // an object containing named parameters
			    ns		    // namespace object (type wgtrNamespace)
			)
    {
	if (!obj) alert('no object for wgtr node: ' + name);
	obj.__WgtrType = type;
	obj.__WgtrVisual = is_visual;
	obj.__WgtrControl = is_control;
	obj.__WgtrChildren = [];
	obj.__WgtrName = name;
	obj.__WgtrContainer = cobj;
	obj.__WgtrScope = scope?scope:"local";
	obj.__WgtrNamespace = ns;
	obj.__WgtrParams = params;

	if (scope)
	    {
	    if (!scopename) scopename = name;
	    if (scope == 'application')
		pg_appglobals[scopename] = obj;
	    else if (scope == 'session')
		pg_sessglobals[scopename] = obj;
	    }

	if (parent) 
	    {
	    obj.__WgtrParent = parent;
	    obj.__WgtrRoot = parent.__WgtrRoot;
	    parent.__WgtrChildren.push(obj);
	    }
	else
	    {
	    obj.__WgtrParent = null;
	    obj.__WgtrRoot = obj;
	    }
	
	obj.__WgtrNamespace.WidgetList[name] = obj;

    }


function wgtrDeinitTree(tree)
    {
	if (!tree || !tree.__WgtrName) 
	    { pg_debug("wgtrDeinitTree - node was not a WgtrNode!\n"); return false; }

	for(var c in tree.__WgtrChildren)
	    wgtrDeinitTree(tree.__WgtrChildren[c]);
	if (tree.destroy_widget)
	    tree.destroy_widget();
    }


function wgtrRemoveNamespace(ns)
    {
    if (!pg_namespaces[ns]) return false;
    pg_namespaces[ns] = null;
    window[ns] = null;
    window['startup_' + ns] = null;
    window['build_wgtr_' + ns] = null;
    }


// Get geometry of a widget (if it is visible)
/*
function wgtrGetGeom(node)
    {
    if (!wgtrIsVisual(node)) return null;
    var domel = node.__WgtrContainer;
    if (domel == node)
	var geom = {left:0, top:0;
    else
	var geom = {left:wgtrGetServerProperty(node,'x'), top:wgtrGetServerProperty(node,'y') };
    var offsets = $(domel).offset();
    geom.left += offsets.left;
    geom.top += offsets.top;
    geom.width = wgtrGetServerProperty(node,'width');
    geom.height = wgtrGetServerProperty(node,'height');
    return geom;
    }
*/


function wgtrUndefinedObject() { }

function wgtrIsUndefined(prop)
    {
    return (typeof prop == 'object' && prop && prop.constructor == wgtrUndefinedObject);
    }


// wgtrGetServerProperty() - return a server-supplied property value
function wgtrGetServerProperty(node, prop_name, def)
    {
    var val = node.__WgtrParams[prop_name];
    if (typeof val == 'undefined')
	return def;
    else if (typeof val == 'object' && val && val.exp)
	{
	//var _context = window[node.__WgtrNamespace.NamespaceID];
	var _context = node.__WgtrNamespace;
	var _this = node;
	window.__cur_exp = null;
	return val.exp(_this, _context);
	}
    else
	return val;
    }

function wgtrIsExpressionServerProperty(node, prop_name)
    {
    var val = node.__WgtrParams[prop_name];
    return (val && (typeof val == 'object') && val.exp);
    }

function wgtrGetServerPropertyPrecedents(node, prop_name)
    {
    var val = node.__WgtrParams[prop_name];
    if (!val || (typeof val != 'object') || !val.exp)
	return [];
    return val.props;
    }

function wgtrSetServerProperty(node, prop_name, value)
    {
    node.__WgtrParams[prop_name] = value;
    }


// wgtrGetProperty - returns the value of a given property
// returns null on failure
function wgtrGetProperty(node, prop_name)
    {
    var prop = wgtrProbeProperty(node, prop_name);
    if (wgtrIsUndefined(prop))
	{
	//alert('Application error: "' + prop_name + '" is undefined for object "' + wgtrGetName(node) + '"');
	return null;
	}
    return prop;
    }


function wgtrProbeProperty(node, prop_name)
    {
    var prop = null;
    var newnode;

	// make sure the parameters are legitimate
	if (!node || !node.__WgtrName) 
	    { 
	    pg_debug("wgtrGetProperty - object passed as node was not a WgtrNode!\n"); return null; 
	    }

	// Indirect reference?
	if (node.reference && (newnode = node.reference()))
	    node = newnode;

	// If the Page, Component-decl, or objectsource widget, check for params
	if (node.__WgtrType == "widget/component-decl" || node.__WgtrType == "widget/page" || node.__WgtrType == "widget/osrc")
	    {
	    for(var child in node.__WgtrChildren)
		{
		child = node.__WgtrChildren[child];
		if (child.__WgtrType == 'widget/parameter' && ((!child.realname && child.__WgtrName == prop_name) || (child.realname == prop_name)))
		    {
		    return child.getvalue();
		    }
		}
	    }

	// If widget has a get-value function, use it
	if (node.ifcProbe && node.ifcProbe(ifValue))
	    {
	    if (node.ifcProbe(ifValue).Exists(prop_name))
		{
		return node.ifcProbe(ifValue).getValue(prop_name);
		}
	    else
		{
		prop = new wgtrUndefinedObject();
		}
	    }

	// check for the existence of the asked-for property
	if (typeof (node[prop_name]) == 'undefined')
	    {
	    //pg_debug("wgtrGetProperty - widget node "+node.WgtrName+" does not have property "+prop_name+'\n');
	    prop = new wgtrUndefinedObject();
	    }

	// check widget params provided by the server
	if (wgtrIsUndefined(prop) && (typeof node.__WgtrParams[prop_name]) != 'undefined')
	    {
	    prop = wgtrGetServerProperty(node, prop_name);
	    }

	// some canonical properties
	if (wgtrIsUndefined(prop))
	    {
	    if (prop_name == 'x') prop = getRelativeX(node);
	    else if (prop_name == 'y') prop = getRelativeY(node);
	    else if (prop_name == 'width') prop = pg_get_style(node, 'width');
	    else if (prop_name == 'height') prop = pg_get_style(node, 'height');
	    }

	if (prop === null)
	    prop = node[prop_name];

	// return the property value
	return prop;
    }


// wgtrWatchProperty - watch a property for changes and call a callback
// function when a change happens.

function wgtrWatchProperty(node, prop_name, callback_obj, callback_fn)
    {
    var realnode = node;
    var newnode;
    var realprop = prop_name;

	// make sure the parameters are legitimate
	if (!node || !node.__WgtrName) 
	    { 
	    pg_debug("wgtrGetProperty - object passed as node was not a WgtrNode!\n"); return null; 
	    }

	// Indirect reference?
	if (realnode.reference && (newnode = realnode.reference()))
	    realnode = newnode;

	// Component/page/osrc?  Check params
	if (realnode.__WgtrType == "widget/component-decl" || realnode.__WgtrType == "widget/page" || realnode.__WgtrType == "widget/osrc")
	    {
	    for(var child in node.__WgtrChildren)
		{
		child = node.__WgtrChildren[child];
		if (child.__WgtrType == 'widget/parameter' && ((!child.realname && child.__WgtrName == realprop) || (child.realname == realprop)))
		    {
		    realnode = child;
		    realprop = 'value';
		    }
		}
	    }

	// Implements ifValue?  If so, use that to watch.
	if (realnode.ifcProbe(ifValue))
	    {
	    if (realnode.ifcProbe(ifValue).Exists(realprop))
		{
		realnode.ifcProbe(ifValue).Watch(realprop, callback_obj, callback_fn);
		return true;
		}
	    else
		return false;
	    }

	// Does not implement ifValue.  Check for property
	if (typeof (realnode[realprop]) == 'undefined')
	    return false;

	// Watch using normal prop watch function.
	htr_cwatch(realnode, realprop, callback_obj, callback_fn);

    return true;
    }


// wgtrGetParent - returns the parent node of the given node
// if there is no parent (top level in the tree), then return
// null.
function wgtrGetParent(node)
    {

	if (!node || !node.__WgtrName)
	    {
	    pg_debug("wgtrGetProperty - object passed as node was not a WgtrNode!\n"); 
	    return null; 
	    }

	return node.__WgtrParent;

    }


// wgtrGetPropertyValue - sets a property of a given node.
// if the property does not already exist, the function fails.
// returns true on success, false on failuer
function wgtrSetProperty(node, prop_name, value)
    {
    var newnode;

	// make sure the parameters are legitimate
	if (!node || !node.__WgtrName) { pg_debug("wgtrGetProperty - object passed as node was not a WgtrNode!\n"); return null; }

	// Indirect reference?
	if (node.reference && (newnode = node.reference()))
	    node = newnode;

	// If the Page, Component-decl, or objectsource widget, check for params
	if (node.__WgtrType == "widget/component-decl" || node.__WgtrType == "widget/page" || node.__WgtrType == "widget/osrc")
	    {
	    for(var child in node.__WgtrChildren)
		{
		child = node.__WgtrChildren[child];
		if (child.__WgtrType == 'widget/parameter' && ((!child.realname && child.__WgtrName == prop_name) || (child.realname == prop_name)))
		    {
		    return child.setvalue(value);
		    }
		}
	    }

	// set the desired property
	if (node.ifcProbe && node.ifcProbe(ifValue) && node.ifcProbe(ifValue).Exists(prop_name))
	    node.ifcProbe(ifValue).setValue(prop_name, value);
	else
	    {
	    // check for the existence of the asked-for property
	    if (typeof (node[prop_name]) == 'undefined') 
		{ 
		pg_debug("wgtrSetProperty - widget node "+node.__WgtrName+" does not have property "+prop_name+'\n');
		return false;
		}
	    node[prop_name]=value;
	    }

	return true;
    }


// wgtrGetNode, wgtrGetNodeRef, wgtrGetNodeUnchecked - gets a particular node
// in the tree based on its name returns the node, or null on fail.
function wgtrGetNodeUnchecked(tree, node_name, type)
    {
    var i;
    var child;
    var node = null;
    var newnode;
    var ns;

	// Search the namespace for the widget.
	if (typeof tree == 'string')
	    {
	    ns = pg_namespaces[tree];
	    }
	else if (tree.NamespaceID)
	    {
	    ns = tree;
	    }
	else
	    {
	    // make sure the parameters are legitimate
	    if (!tree || !tree.__WgtrName) 
		{ pg_debug("wgtrGetNode - node was not a WgtrNode!\n"); return false; }

	    ns = tree.__WgtrNamespace;
	    }
	while (ns)
	    {
	    if (ns.WidgetList[node_name])
		{
		node = ns.WidgetList[node_name];
		break;
		}
	    ns = ns.ParentNamespace;
	    }

	// If not found, search application and session global scopes
	if (!node)
	    {
	    if (pg_appglobals[node_name])
		node = pg_appglobals[node_name];
	    else if (pg_sessglobals[node_name])
		node = pg_sessglobals[node_name];
	    else
		{
		// Still not found? Search all pages in this session for
		// session globals that might match
		for(var win in window.pg_appwindows)
		    {
		    var w = window.pg_appwindows[win];
		    if (w && w.wobj && w.wobj.pg_sessglobals)
			node = w.wobj.pg_sessglobals[node_name];
		    if (node) break;
		    }
		}
	    }

    return node;
    }


function wgtrGetNode(tree, node_name, type)
    {
    var node = wgtrGetNodeUnchecked(tree, node_name, type);

	if (!node)
	    {
	    if (typeof tree == 'string')
		alert('Application error: "' + node_name + '" is undefined in namespace "' + tree + '"');
	    else
		alert('Application error: "' + node_name + '" is undefined in application/component "' + wgtrGetName(tree) + '"');
	    }

	// Indirect reference?
	if (node && node.reference)
	    {
	    if (newnode = node.reference())
		node = newnode;
	    else
		return null;
	    }

	if (node && type && node.__WgtrType != type)
	    alert('Application error: "' + node_name + '" (type ' + node.__WgtrType + ') is not of expected type "' + type + '"');

	return node;
    }


function wgtrGetNodeRef(tree, node_name, type)
    {
    var node = wgtrGetNodeUnchecked(tree, node_name, type);

	if (!node)
	    {
	    if (typeof tree == 'string')
		alert('Application error: "' + node_name + '" is undefined in namespace "' + tree + '"');
	    else
		alert('Application error: "' + node_name + '" is undefined in application/component "' + wgtrGetName(tree) + '"');
	    }

	if (node && type && node.__WgtrType != type)
	    alert('Application error: "' + node_name + '" (type ' + node.__WgtrType + ') is not of expected type "' + type + '"');

	return node;
    }


// wgtrAddEventFunc - associate a function with an event for a particular widget.
// returns true on success, false on error
function wgtrAddEventFunc(node, event_name, func)
    {
    var w;

	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrAddEventFunc - node was not a WgtrNode!\n"); return false; }

	if (node['Event'+event_name] == null)
	    node['Event'+event_name] = new Array();
	node['Event'+event_name][node['Event'+event_name].length] = func;
	
	return true;
    }


// wgtrWalk - for debugging purposes, walks the tree and prints information about
// each node
function wgtrWalk(tree, x)
    {
    var i;
    var txt;

	if (!x) x = 0;

	txt = tree.__WgtrName+" '"+tree.__WgtrType+"'";
	if (tree.__WgtrVisual)
	    {
	    txt = txt+" x: "+wgtrGetProperty(tree, "x");
	    txt = txt+" y: "+wgtrGetProperty(tree, "y");
	    txt = txt+" fieldname: "+wgtrGetProperty(tree, "fieldname");
	    }
	else
	    {
	    txt = txt+" non-visual";
	    }
	txt=txt+"\n";
	for (i=0;i<x;i++) txt = ' '+txt;
	pg_debug(txt);
	for (i=0;i<tree.__WgtrChildren.length;i++)
	    {
	    wgtrWalk(tree.__WgtrChildren[i], x+2);
	    }
    }


function wgtrIsNode(tree)
    {
    return(tree && tree.__WgtrName);
    }


function wgtrIsVisual(tree)
    {
    return tree && tree.__WgtrVisual;
    }


function wgtrIsControl(tree)
    {
    return tree && tree.__WgtrControl;
    }


function wgtrIsRoot(tree)
    {
    return tree == tree.__WgtrRoot;
    }


function wgtrGetContainer(tree)
    {
    if (!tree.__WgtrContainer) return null;
    return tree.__WgtrContainer;
    }


function wgtrGetParentContainer(tree)
    {
    return wgtrGetContainer(wgtrGetParent(tree));
    }


function wgtrNodeList(node)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrNodeList - node was not a WgtrNode!\n"); return false; }
	return node.__WgtrNamespace.WidgetList;
    }


// Get all subnodes, recursively, within the namespace
function wgtrAllSubNodes(node)
    {
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrAllSubNodes - node was not a WgtrNode!\n"); return false; }

	var nodearr = node.__WgtrChildren.slice(0);
	var subnodearr = new Array();
	for(var n in nodearr)
	    {
	    subnodearr = subnodearr.concat(wgtrAllSubNodes(nodearr[n]));
	    }
	nodearr = nodearr.concat(subnodearr);

	return nodearr;
    }


function wgtrReplaceNode(oldnode, newnode, newcont)
    {
	// make sure this is actually a tree
	if (!oldnode || !oldnode.__WgtrName) 
	    { pg_debug("wgtrReplaceNode - oldnode was not a WgtrNode!\n"); return false; }
	
	var children = oldnode.__WgtrChildren;

	if (!newcont) newcont = newnode;
	wgtrAddToTree(newnode, newcont, oldnode.__WgtrName, oldnode.__WgtrType, oldnode.__WgtrParent, oldnode.__WgtrVisual, oldnode.__WgtrControl, null, null, oldnode.__WgtrParams, oldnode.__WgtrNamespace);

	for(var i=0; i<children.length; i++)
	    {
	    children[i].__WgtrParent = newnode;
	    }
	newnode.__WgtrChildren = children;

	if (newnode.__WgtrParent)
	    {
	    for(var i=0; i<newnode.__WgtrParent.__WgtrChildren.length; i++)
		{
		if (newnode.__WgtrParent.__WgtrChildren[i] == oldnode)
		    {
		    newnode.__WgtrParent.__WgtrChildren.splice(i, 1);
		    break;
		    }
		}
	    }
	oldnode.__WgtrName = null;
    }

function wgtrGetType(node)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetType - oldnode was not a WgtrNode!\n"); return false; }
	return node.__WgtrType;
    }


// This is a static form function to find the form that contains the given
// element.  Replacement for the old fm_current logic.
function wgtrFindContainer(node,type)
    {
    var t,p,n;
    // make sure this is actually a tree
    if (!node || !node.__WgtrName) 
	{ pg_debug("wgtrFindContainer - oldnode was not a WgtrNode!\n"); return false; }
    while(1)
	{
	p = wgtrGetParent(node);
	if (node.reference && (n = node.reference()))
	    node = n;
	t = wgtrGetType(node);
	if (t == type) return node;
	if (!p) return null;
	node = p;
	}
    }

// this finds a containing widget, across components.
function wgtrGlobalFindContainer(node, type)
    {
    var c;
    if (!node || !node.__WgtrName) 
	{ pg_debug("wgtrGlobalFindContainer - node was not a WgtrNode!\n"); return false; }

    while(1)
	{
	// first, try finding container normally.
	c = wgtrFindContainer(node, type);
	if (c) return c;
	if (wgtrGetType(wgtrGetRoot(node)) == "widget/component-decl")
	    node = wgtrGetRoot(node).shell;
	else
	    return null;
	}
    }

function wgtrGetName(node)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetName - node was not a WgtrNode!\n"); return false; }
	return node.__WgtrName;
    }


function wgtrGetNamespace(node)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetName - node was not a WgtrNode!\n"); return false; }
	return node.__WgtrNamespace.NamespaceID;
    }

function wgtrGetRoot(node)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetRoot - node was not a WgtrNode!\n"); return false; }
	return node.__WgtrRoot;
    }

/*function wgtrGetAttrValue(node, attrname)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetAttrValue - node was not a WgtrNode!\n"); return null; }
	if (typeof (node[attrname]) == 'undefined')
	    alert('Application error: "' + attrname + '" is undefined for object "' + wgtrGetName(node) + '"');
	return node[attrname];
    }*/

function wgtrCheckReference(v)
    {
    if (wgtrIsNode(v))
	{
	while (v.reference)
	    {
	    var v2 = v.reference();
	    if (v2 == v || v2 == null) break;
	    v = v2;
	    }
	return wgtrGetNamespace(v) + ":" + wgtrGetName(v);
	}
    return null;
    }

function wgtrDereference(r)
    {
    if (!r || !r.split) return null;
    var n = r.split(":", 2);
    if (!n || !n[0] || !n[1]) return null;
    var ns = pg_namespaces[n[0]];
    if (!ns)
	{
	// Look at other namespaces in other windows in this session.
	for(var win in window.pg_appwindows)
	    {
	    var w = window.pg_appwindows[win];
	    if (w && w.wobj && w.wobj.pg_namespaces)
		ns = w.wobj.pg_namespaces[n[0]];
	    if (ns) break;
	    }
	}
    if (!ns)
	{
	alert('Application error: namespace "' + n[0] + '" is undefined');
	return null;
	}
    // get first widget in Namespace
    for(var wname in ns.WidgetList)
	{
	var nswidget = ns.WidgetList[wname];
	break;
	}
    //return wgtrGetNode(ns.NamespaceID, n[1]);
    return wgtrGetNode(nswidget, n[1]);
    }

function wgtrFind(v)
    {
    for(var n in pg_namespaces)
	{
	if ((typeof pg_namespaces[n].WidgetList[v]) != 'undefined')
	    return pg_namespaces[n].WidgetList[v];
	}
    return null;
    }

// Finds a directly-attached child node.
//
function wgtrGetChild(node, childname)
    {
    for(var i=0; i<node.__WgtrChildren.length; i++)
	{
	if (node.__WgtrChildren[i].__WgtrName == childname)
	    return node.__WgtrChildren[i];
	}
    }


// Finds all children, in this namespace or sub-namespaces, that are
// of a given type, even if they are nested several more levels inside
// other non-visual widgets (a visual widget between parent and child
// will render the child invisible to this function).
function wgtrFindMatchingDescendents(node, childtype)
    {
    var arr = [];
    for(var i=0; i<node.__WgtrChildren.length; i++)
	{
	var child = node.__WgtrChildren[i];
	if (child.__WgtrType == childtype)
	    arr.push(child);
	else if (!wgtrIsVisual(child) && (child.__WgtrNamespace.NamespaceID == node.__WgtrNamespace.NamespaceID || (child.__WgtrNamespace.ParentNamespace && child.__WgtrNamespace.ParentNamespace.NamespaceID == node.__WgtrNamespace.NamespaceID)))
	    {
	    var check = wgtrFindMatchingDescendents(child, childtype);
	    if (check)
		arr = arr.concat(check);
	    }
	}
    return arr;
    }


// Finds a child/descendent in this namespace or in any sub-namespaces
// of the parent's namespace.
//
function wgtrFindDescendent(node, childname, childns)
    {
    for(var i=0; i<node.__WgtrChildren.length; i++)
	{
	var child = node.__WgtrChildren[i];
	if (child.__WgtrName == childname && child.__WgtrNamespace.NamespaceID == childns)
	    return child;
	if (!wgtrIsVisual(child) && (child.__WgtrNamespace.NamespaceID == node.__WgtrNamespace.NamespaceID || (child.__WgtrNamespace.ParentNamespace && child.__WgtrNamespace.ParentNamespace.NamespaceID == node.__WgtrNamespace.NamespaceID)))
	    {
	    var check = wgtrFindDescendent(child, childname, childns);
	    if (check)
		return check;
	    }
	}
    return null;
    }

function wgtrGetChildren(node)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetChildren - node was not a WgtrNode!\n"); return false; }

	return node.__WgtrChildren;
    }

function wgtrRegisterContainer(node, container)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrRegisterContainer - node was not a WgtrNode!\n"); return false; }
	if (!container || !container.__WgtrName) 
	    { pg_debug("wgtrRegisterContainer - container was not a WgtrNode!\n"); return false; }
	node.__WgtrTreeContainer = container;
	return;
    }

function wgtrIsChild(node, subnode)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetChild - node was not a WgtrNode!\n"); return false; }
	if (!subnode || !subnode.__WgtrName) 
	    { pg_debug("wgtrGetChild - subnode was not a WgtrNode!\n"); return false; }

	while(subnode)
	    {
	    if (subnode.__WgtrTreeContainer)
		subnode = subnode.__WgtrTreeContainer;
	    else
		subnode = wgtrGetParent(subnode);
	    if (subnode == node)
		return true;
	    }
	return false;
    }

function wgtrGetGeom(node)
    {
    if (!node.tagName || node.tagName != 'DIV')
	{
	// fixme -- not all widgets have a visual DIV
	if (wgtrGetType(node) == "widget/component")
	    {
	    var cmp_geom = node.getGeom();
	    return {x:getPageX(wgtrGetParentContainer(node)) + cmp_geom.x,
		    y:getPageY(wgtrGetParentContainer(node)) + cmp_geom.y,
		    width:cmp_geom.width,
		    height:cmp_geom.height};
	    }
	return null;
	}
    else
	{
	//return {x:getPageX(node),y:getPageY(node),width:getClipWidth(node),height:getClipHeight(node)};
	return {x:$(node).offset().left,y:$(node).offset().top,width:getClipWidth(node),height:getClipHeight(node)};
	}
    }

function wgtrFindInSubtree(subtree, cur_node, nodetype)
    {
    if (!subtree) return null;
    if (!cur_node) cur_node = subtree;

    // find the next one
    var find_node = cur_node;
    while(true)
	{
	if (find_node.__WgtrChildren && find_node.__WgtrChildren.length > 0)
	    {
	    // Just examined parent, jump to first child next.
	    find_node = find_node.__WgtrChildren[0];
	    }
	else if (find_node.__WgtrType == "widget/component" && find_node.components && find_node.components.length > 0)
	    {
	    find_node = find_node.components[0].cmp;
	    }
	else 
	    {
	    var continue_recurse = true;
	    while(continue_recurse)
		{
		if (find_node.__WgtrParent)
		    {
		    // find the index of this child in the parent's child array so we
		    // can locate the next sibling
		    for(var i=0;i<find_node.__WgtrParent.__WgtrChildren.length; i++)
			{
			if (find_node.__WgtrParent.__WgtrChildren[i] == find_node)
			    {
			    if (find_node.__WgtrParent.__WgtrChildren.length > i+1)
				{
				// Just examined one child, go to sibling next
				find_node = find_node.__WgtrParent.__WgtrChildren[i+1];
				continue_recurse = false;
				}
			    else
				{
				// Examined all siblings, go back up the tree to find
				// siblings of parent(s)
				find_node = find_node.__WgtrParent;
				}
			    break;
			    }
			}
		    }
		else if (find_node.__WgtrType == 'widget/component-decl')
		    {
		    // This is a component root, instead of siblings look in the
		    // component instance list.
		    for(var i=0;i<find_node.shell.components.length; i++)
			{
			if (find_node.shell.components[i].cmp == find_node)
			    {
			    if (find_node.shell.components.length > i+1)
				{
				// go to next instance
				find_node = find_node.shell.components[i+1].cmp;
				continue_recurse = false;
				}
			    else
				{
				// Examined all instances in the component, go back up to container.
				find_node = find_node.shell;
				}
			    break;
			    }
			}
		    }

		// Don't recurse back past the top of the subtree.
		if (find_node == subtree) break;
		}
	    }

	// Found a matching node?
	if (!nodetype || find_node.__WgtrType == nodetype)
	    break;

	// If we're back where we started, return null.
	if (find_node == cur_node)
	    return null;
	}

    return find_node;
    }


// Load indication
if (window.pg_scripts) pg_scripts['ht_utils_wgtr.js'] = true;
