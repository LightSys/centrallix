//
// DHTML wgtr module
// Netscape 4 version
//


// wgtrSetupTree - walk a given widget tree, and set up the wgtr properties.
// Pass in the declared tree, and this thing returns the actual tree of real
// widgets.
//

function wgtrSetupTree(tree, ns, parent)
    {
    var _parentobj = parent?parent.obj:null;
    var _parentctr = parent?parent.cobj:null;

    if (!tree.obj)
	tree.obj = {};
    else
	with (tree) tree.obj = eval(tree.obj);

    var _obj = tree.obj;
    if (!tree.cobj)
	tree.cobj = _obj;
    else if (tree.cobj == '_parentctr')
	tree.cobj = _parentctr;
    else
	with (tree) tree.cobj = eval(tree.cobj);

    if (ns) tree.obj.__WgtrNamespace = ns;
    wgtrAddToTree(tree.obj, tree.cobj, tree.name, tree.type, _parentobj, tree.vis, tree.scope, tree.sn);
    if (tree.sub)
	for(var i=0; i<tree.sub.length; i++)
	    wgtrSetupTree(tree.sub[i], null, tree);
    return tree.obj;
    }


// wgtrAddToTree - takes an existing object, and makes it a widget tree node,
// grafting it in to a tree as the child of the given parent.
function wgtrAddToTree	(   obj,	    // the object to graft into the tree
			    cobj,	    // the container for subobjects
			    name,	    // the name of the object
			    type,	    // the widget type 
			    parent,	    // the parent node of this object
			    is_visual,	    // is the node a visual node?
			    scope,	    // scope of widget -- local vs application vs session
			    scopename	    // name of widget to use in extended scope
			)
    {
	if (!obj) alert('no object for wgtr node: ' + name);
	obj.__WgtrType = type;
	obj.__WgtrVisual = is_visual;
	obj.__WgtrChildren = [];
	obj.__WgtrName = name;
	obj.__WgtrContainer = cobj;
	obj.__WgtrScope = scope?scope:"local";

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
	    obj.__WgtrChildList = [];
	    obj.__WgtrRoot = obj;
	    }
	
	obj.__WgtrRoot.__WgtrChildList[name] = obj;

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


function wgtrUndefinedObject() { }

function wgtrIsUndefined(v)
    {
    return (typeof prop == 'object' && prop && prop.constructor == wgtrUndefinedObject);
    }


// wgtrGetProperty - returns the value of a given property
// returns null on failure
function wgtrGetProperty(node, prop_name)
    {
    var prop = wgtrProbeProperty(node, prop_name);
    if (wgtrIsUndefined(prop))
	{
	alert('Application error: "' + prop_name + '" is undefined for object "' + wgtrGetName(node) + '"');
	return null;
	}
    return prop;
    }


function wgtrProbeProperty(node, prop_name)
    {
    var prop;
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
	if (node.ifcProbe(ifValue))
	    {
	    if (node.ifcProbe(ifValue).Exists(prop_name))
		{
		return node.ifcProbe(ifValue).getValue(prop_name);
		}
	    else
		{
		return new wgtrUndefinedObject();
		}
	    }

	// check for the existence of the asked-for property
	if (typeof (node[prop_name]) == 'undefined')
	    {
	    //pg_debug("wgtrGetProperty - widget node "+node.WgtrName+" does not have property "+prop_name+'\n');
	    return new wgtrUndefinedObject();
	    }

	prop = node[prop_name];

	// return the property value
	return prop;
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

// wgtrGetNode - gets a particular node in the tree based on its name
// returns the node, or null on fail.
function wgtrGetNode(tree, node_name, type)
    {
    var i;
    var child;
    var node = null;
    var newnode;

	// make sure the parameters are legitimate
	if (!tree || !tree.__WgtrName) 
	    { pg_debug("wgtrGetNode - node was not a WgtrNode!\n"); return false; }
	
	/*for (i=0;i<tree.children.length;i++)
	    {
	    if (tree.WgtrChildren[i].WgtrName == node_name) return tree.WgtrChildren[i];
	    if ( (child = wgtrGetNode(tree.WgtrChildren[i], node_name)) != null) return child;
	    }*/
	if (tree.__WgtrRoot.__WgtrChildList[node_name]) 
	    node = tree.__WgtrRoot.__WgtrChildList[node_name];
	else if (pg_appglobals[node_name])
	    node = pg_appglobals[node_name];
	else if (pg_sessglobals[node_name])
	    node = pg_sessglobals[node_name];
	else
	    alert('Application error: "' + node_name + '" is undefined in application/component "' + wgtrGetName(tree) + '"');

	// Indirect reference?
	if (node && node.reference)
	    {
	    if (newnode = node.reference())
		node = newnode;
	    else
		return null;
	    }

	if (type && node.__WgtrType != type)
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


function wgtrIsRoot(tree)
    {
    return tree == tree.__WgtrRoot;
    }


function wgtrGetContainer(tree)
    {
    if (!tree.__WgtrContainer) return null;
    return tree.__WgtrContainer;
    }


function wgtrNodeList(node)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrNodeList - node was not a WgtrNode!\n"); return false; }
	return node.__WgtrRoot.__WgtrChildList;
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
	wgtrAddToTree(newnode, newcont, oldnode.__WgtrName, oldnode.__WgtrType, oldnode.__WgtrParent, oldnode.__WgtrVisual);

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
	return node.__WgtrRoot.__WgtrNamespace;
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
    if (!pg_namespaces[n[0]])
	{
	alert('Application error: namespace "' + n[0] + '" is undefined');
	return null;
	}
    return wgtrGetNode(pg_namespaces[n[0]], n[1]);
    }

function wgtrFind(v)
    {
    for(var n in pg_namespaces)
	{
	if ((typeof pg_namespaces[n].__WgtrChildList[v]) != 'undefined')
	    return pg_namespaces[n].__WgtrChildList[v];
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
	node.__WgtrCmpContainer = container;
	return;
    }

function wgtrIsChild(node, subnode)
    {
	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetChild - node was not a WgtrNode!\n"); return false; }
	if (!subnode || !subnode.__WgtrName) 
	    { pg_debug("wgtrGetChild - subnode was not a WgtrNode!\n"); return false; }

	var nodert = wgtrGetRoot(node);
	while(subnode)
	    {
	    if (subnode.__WgtrCmpContainer)
		subnode = subnode.__WgtrCmpContainer;
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
	    return {x:getPageX(wgtrGetContainer(wgtrGetParent(node))) + cmp_geom.x,
		    y:getPageY(wgtrGetContainer(wgtrGetParent(node))) + cmp_geom.y,
		    width:cmp_geom.width,
		    height:cmp_geom.height};
	    }
	return null;
	}
    else
	{
	return {x:getPageX(node),y:getPageY(node),width:getClipWidth(node),height:getClipHeight(node)};
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

