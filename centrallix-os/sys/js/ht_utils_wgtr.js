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
    var txt;
    var element;
    var i;

//	if (document.layers.dbgwnd)
//	    {
//	    element = document.layers.dbgwnd.document.forms.dbgform.elements.dbgtxt;
//	    txt = element.value;
//	    for (i=0;i<indent;i++) { txt = txt+" "; }
//	    element.value = txt+msg+"\n";
//	    }
	
    }

// WgtrNode - constructor for a widget tree node
// geometry is pulled from the layer
function WgtrNode   (	name,				// widget's name
			type,				// widget's type (form, editbox, etc)
			its_obj,			// object (layer, if visual) that the node is associated with
			is_visible			// boolean - is the layer a visual layer?		
		    )
    {
	this.WGTRNODE = true;	    // for 'type-checking' in other functions

	// fill out the standard object properties
	this.name = name;
	this.type = type;
	this.wgt_obj = its_obj;
	if (is_visible)
	    {
	    this.x = this.wgt_obj.left;
	    this.y = this.wgt_obj.top;
	    this.width = this.wgt_obj.width;
	    this.height = this.wgt_obj.height;
	    }
	this.children = new Array(0);
	this.visual = is_visible;
    }


// wgtrGetPropertyValue - returns the value of a given property
// returns null on failure
function wgtrGetProperty(node, prop_name)
    {
	// make sure the parameters are legitimate
	if (!node || !node.WGTRNODE) { wgtr_internal_Debug("wgtrGetProperty - object passed as node was not a WgtrNode!"); return null; }
	if (typeof(prop_name) != "string") { wgtr_internal_Debug("wgtrGetProperty - prop_name was not a string!"); return null; }

	// check for the existence of the asked-for property
	if (typeof eval("node.wgt_obj."+prop_name) == "undefined") 
	    { 
	    wgtr_internal_Debug("wgtrGetProperty - widget node "+node.name+" does not have property "+prop_name);
	    return null;
	    }

	// return the property value
	return eval("node.wgt_obj."+prop_name);
    }


// wgtrGetPropertyValue - sets a property of a given node.
// if the property does not already exist, the function fails.
// returns true on success, false on failuer
function wgtrSetProperty(node, prop_name, value)
    {
	// make sure the parameters are legitimate
	if (!node || !node.WGTRNODE) { wgtr_internal_Debug("wgtrGetProperty - object passed as node was not a WgtrNode!"); return null; }
	if (typeof(prop_name) != "string") { wgtr_internal_Debug("wgtrGetProperty - prop_name was not a string!"); return null; }

	// check for the existence of the asked-for property
	if (!eval("node.wgt_obj."+prop_name)) 
	    { 
	    wgtr_internal_Debug("wgtrSetProperty - widget node "+node.name+" does not have property "+prop_name);
	    return false;
	    }

	// set the desired property
	eval("node.wgt_obj."+prop_name+"=value");
	return true;
    }



// wgtrAddProperty - adds a property to a wgt node
// fails if the property already exists. returns true on success, false on fail
function wgtrAddProperty(node, prop_name)
    {
	// make sure the parameters are legitimate
	if (!node || !node.WGTRNODE) { wgtr_internal_Debug("wgtrGetProperty - object passed as node was not a WgtrNode!"); return null; }
	if (typeof(prop_name) != "string") { wgtr_internal_Debug("wgtrGetProperty - prop_name was not a string!"); return null; }

	// check for the existence of the asked-for property
	if (eval("node.wgt_obj."+prop_name)) 
	    { 
	    wgtr_internal_Debug("wgtrAddProperty - widget node "+node.name+" already has property "+prop_name);
	    return false;
	    }
	
	// add the property
	eval("node.wgt_obj."+prop_name+"=\"undefined\"");
	return true;
    }


// wgtrAddChild - adds a child node to a node. returns true on success, false on fail
function wgtrAddChild(node, child_node)
    {
	// make sure the parameters are legitimate
	if (!node || !node.WGTRNODE) 
	    { 
	    wgtr_internal_Debug("wgtrAddChild - object passed as node was not a WgtrNode!"); 
	    return false; 
	    }
	if (!child_node || !child_node.WGTRNODE) 
	    { 
	    wgtr_internal_Debug("wgtrAddChild - object passed as child_node was not a WgtrNode!"); 
	    return false; 
	    }
	node.children.push(child_node);
	child_node.parent = node;

	return true;
    }


// wgtrDeleteChild - removes a child node from a node, and deletes the child node
function wgtrDeleteChild(node, child_node_name)
    {
    var i, j;	    // counters

	// make sure the parameters are legitimate
	if (!node || !node.WGTRNODE) 
	    { 
	    wgtr_internal_Debug("wgtrDeleteChild - object passed as node was not a WgtrNode!"); 
	    return false; 
	    }
	if (typeof(child_node_name) != "string") 
	    { 
	    wgtr_internal_Debug("wgtrDeleteChild - child_node_name was not a string!"); 
	    return false; 
	    }

	// find the child node
	for (i=0;i<node.children.length;i++)
	    {
	    if (node.children[i].name == child_node_name) break;
	    }
	// did we find it?
	if (i == node.children.length) 
	    { 
	    wgtr_internal_Debug("wgtrDeleteChild - '"+child_node_name+"' is not a child of "+node.name); 
	    return false; 
	    }

	// remove the child from the array
	for (j=i+1;j<node.children.length;j++) node.children[j-1]=node.children[j];
	node.children.pop();
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
	if (typeof(node_name) != "string") 
	    { wgtr_internal_Debug("wgtrGetNode - node_name was not a string!"); return false; }
	
	for (i=0;i<tree.children.length;i++)
	    {
	    if (tree.children[i].name == node_name) return tree.children[i];
	    if ( (child = wgtrGetNode(tree.children[i], node_name)) != null) return child;
	    }
	return null;
    }


function wgtrWalk(tree, x)
    {
    var i;
    var txt;

	if (!x) x = 0;

	txt = tree.name+" '"+tree.type+"'";
	if (tree.visual)
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
	for (i=0;i<tree.children.length;i++)
	    {
	    wgtrWalk(tree.children[i], x+2);
	    }
    }
