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
    //if (tree.name == 'mainwin') htr_alert(_parentctr,1);
    var e;
    e = tree.obj;
    if (tree.name == 'mainwin')
	{
	with (tree) var x = eval("_parentctr");
	//htr_alert(x,1);
	}
    with (tree) tree.obj = eval(tree.obj);
    //if (!tree.obj) alert('expression failed: ' + e);
    var _obj = tree.obj;
    //if (ns) alert(typeof window);
    e = tree.cobj;
    with (tree) tree.cobj = eval(tree.cobj);
    //if (!tree.cobj) alert('ctr expression failed: ' + e);
    //if (tree.name == 'debugwin') htr_alert(tree.cobj,1);
    //if (ns) alert(tree.cobj);
    if (ns) tree.obj.__WgtrNamespace = ns;
    wgtrAddToTree(tree.obj, tree.cobj, tree.name, tree.type, _parentobj, tree.vis);
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
			    is_visual	    // is the node a visual node?
			)
    {
	if (!obj) alert('no object for wgtr node: ' + name);
	obj.__WgtrType = type;
	obj.__WgtrVisual = is_visual;
	obj.__WgtrChildren = new Array();
	obj.__WgtrName = name;
	obj.__WgtrContainer = cobj;

	if (parent) 
	    {
	    obj.__WgtrParent = parent;
	    obj.__WgtrRoot = parent.__WgtrRoot;
	    parent.__WgtrChildren.push(obj);
	    }
	else
	    {
	    obj.__WgtrParent = null;
	    obj.__WgtrChildList = new Array();
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


// wgtrGetPropertyValue - returns the value of a given property
// returns null on failure
function wgtrGetProperty(node, prop_name)
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

	// If the Page or Component-decl widget, check for params
	if (node.__WgtrType == "widget/component-decl" || node.__WgtrType == "widget/page")
	    {
	    for(var child in node.__WgtrChildren)
		{
		child = node.__WgtrChildren[child];
		if (child.__WgtrType == 'widget/parameter' && child.__WgtrName == prop_name)
		    {
		    return child.getvalue();
		    }
		}
	    }

	// check for the existence of the asked-for property
	if (typeof (node[prop_name]) == 'undefined')
	    {
	    alert('Application error: "' + prop_name + '" is undefined for object "' + wgtrGetName(node) + '"');
	    //pg_debug("wgtrGetProperty - widget node "+node.WgtrName+" does not have property "+prop_name+'\n');
	    return null;
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
	// make sure the parameters are legitimate
	if (!node || !node.__WgtrName) { pg_debug("wgtrGetProperty - object passed as node was not a WgtrNode!\n"); return null; }

	// check for the existence of the asked-for property
	if (!node[prop_name]) 
	    { 
	    pg_debug("wgtrSetProperty - widget node "+node.__WgtrName+" does not have property "+prop_name+'\n');
	    return false;
	    }

	// set the desired property
	node[prop_name]=value;
	return true;
    }

// wgtrGetNode - gets a particular node in the tree based on its name
// returns the node, or null on fail.
function wgtrGetNode(tree, node_name)
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
	else
	    alert('Application error: "' + node_name + '" is undefined in application/component "' + wgtrGetName(tree) + '"');

	// Indirect reference?
	if (node && node.reference && (newnode = node.reference()))
	    {
	    node = newnode;
	    }

	return node;
    }

// wgtrAddEventFunc - associate a function with an event for a particular widget.
// returns true on success, false on error
function wgtrAddEventFunc(node, event_name, func)
    {
    var w;

	// make sure this is actually a tree
	if (!node || !node.__WgtrName) 
	    { pg_debug("wgtrGetNode - node was not a WgtrNode!\n"); return false; }

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
	if (v.reference) v = v.reference();
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

