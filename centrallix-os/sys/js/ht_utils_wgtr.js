//
// DHTML wgtr module
// Netscape 4 version
//


function wgtr_internal_Print(msg, indent)
    {
    var txt;
    var element;
    var i;

	if (document.layers.dbgwnd)
	    {
	    element = document.layers.dbgwnd.document.forms.dbgform.elements.dbgtxt;
	    txt = element.value;
	    for (i=0;i<indent;i++) { txt = txt+" "; }
	    element.value = txt+msg+"\n";
	    }
	
    }

function wgtr_internal_Debug(msg, indent)
    {
/*
    var txt;
    var element;
    var i;

	if (document.layers.dbgwnd)
	    {
	    element = document.layers.dbgwnd.document.forms.dbgform.elements.dbgtxt;
	    txt = element.value;
	    for (i=0;i<indent;i++) { txt = txt+" "; }
	    element.value = txt+msg+"\n";
	    }
*/	
    }

// wgtrAddToTree - takes an existing object, and makes it a widget tree node,
// grafting it in to a tree as the child of the given parent.
function wgtrAddToTree	(   obj,	    // the object to graft into the tree
			    name,	    // the name of the object
			    type,	    // the widget type 
			    parent,	    // the parent node of this object
			    is_visual	    // is the node a visual node?
			)
    {
	obj.WgtrType = type;
	obj.WgtrVisual = is_visual;
	obj.WgtrChildren = new Array();
	obj.WgtrParent = parent;
	obj.WgtrName = name;
	obj.WGTRNODE = true;	// so we'll be able to verify in other functions that we're
				// operating on a proper widget node

	// if this is the root of the tree, parent will be null
	if (parent != null) { parent.WgtrChildren.push(obj); }
    }

// wgtrGetPropertyValue - returns the value of a given property
// returns null on failure
function wgtrGetProperty(node, prop_name)
    {
    var prop;

	// make sure the parameters are legitimate
	if (!node || !node.WGTRNODE) { wgtr_internal_Debug("wgtrGetProperty - object passed as node was not a WgtrNode!"); return null; }

	// check for the existence of the asked-for property
	prop = node[prop_name];
	if (typeof prop == "undefined") 
	    { 
	    wgtr_internal_Debug("wgtrGetProperty - widget node "+node.WgtrName+" does not have property "+prop_name);
	    return null;
	    }

	// return the property value
	return prop;
    }


// wgtrGetPropertyValue - sets a property of a given node.
// if the property does not already exist, the function fails.
// returns true on success, false on failuer
function wgtrSetProperty(node, prop_name, value)
    {
	// make sure the parameters are legitimate
	if (!node || !node.WGTRNODE) { wgtr_internal_Debug("wgtrGetProperty - object passed as node was not a WgtrNode!"); return null; }

	// check for the existence of the asked-for property
	if (!node[prop_name]) 
	    { 
	    wgtr_internal_Debug("wgtrSetProperty - widget node "+node.WgtrName+" does not have property "+prop_name);
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

	// make sure the parameters are legitimate
	if (!tree || !tree.WGTRNODE) 
	    { wgtr_internal_Debug("wgtrGetNode - node was not a WgtrNode!"); return false; }
	
	for (i=0;i<tree.children.length;i++)
	    {
	    if (tree.WgtrChildren[i].WgtrName == node_name) return tree.WgtrChildren[i];
	    if ( (child = wgtrGetNode(tree.WgtrChildren[i], node_name)) != null) return child;
	    }
	return null;
    }

// wgtrAddEventFunc - associate a function with an event for a particular widget.
// returns true on success, false on error
function wgtrAddEventFunc(node, event_name, func)
    {
    var w;

	// make sure this is actually a tree
	if (!node || !node.WGTRNODE) 
	    { wgtr_internal_Debug("wgtrGetNode - node was not a WgtrNode!"); return false; }

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

	txt = tree.WgtrName+" '"+tree.WgtrType+"'";
	if (tree.WgtrVisual)
	    {
	    txt = txt+" x: "+wgtrGetProperty(tree, "x");
	    txt = txt+" y: "+wgtrGetProperty(tree, "y");
	    txt = txt+" fieldname: "+wgtrGetProperty(tree, "fieldname");
	    }
	else
	    {
	    txt = txt+" non-visual";
	    }
	wgtr_internal_Print(txt, x);
	for (i=0;i<tree.WgtrChildren.length;i++)
	    {
	    wgtrWalk(tree.WgtrChildren[i], x+2);
	    }
    }
