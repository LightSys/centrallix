// Copyright (C) 1998-2007 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function al_childresize(child, oldw, oldh, neww, newh)
    {
    if (oldw != neww || oldh != newh)
	{
	// Need to set h/w before pg_check_resize() does, in order to reflow.
	if (oldw != neww)
	    wgtrSetServerProperty(child, "width", neww);
	if (oldh != newh)
	    wgtrSetServerProperty(child, "height", newh);

	// Now do the reflow
	this.Reflow();
	}
    return {width:neww, height:newh};
    }

function al_reflow_buildlist(node, children)
    {
    for(var i=0; i<node.__WgtrChildren.length; i++)
	{
	var child = node.__WgtrChildren[i];
	if (wgtrIsControl(child) || !wgtrIsVisual(child))
	    al_reflow_buildlist(child, children);
	else
	    children.push(child);
	}
    }

function al_reflow()
    {
    // Get configuration
    var width = wgtrGetServerProperty(this,"width");
    var height = wgtrGetServerProperty(this,"height");
    var spacing = wgtrGetServerProperty(this,"spacing");
    if (!spacing) spacing = 0;
    var cellsize = wgtrGetServerProperty(this,"cellsize");
    if (!cellsize) cellsize = -1;
    var align = wgtrGetServerProperty(this,"align");
    if (!align) align = "left";
    var justify_mode = wgtrGetServerProperty(this,"justify");
    if (!justify_mode) justify_mode = "none";
    var type = "vbox";
    if (wgtrGetServerProperty(this,"style") == "hbox" || wgtrGetType(this) == "widget/hbox")
	type = "hbox";
    var column_width;
    if (type == "vbox")
	column_width = wgtrGetServerProperty(this,"column_width");
    if (!column_width) column_width = width;
    var row_height;
    if (type == "hbox")
	row_height = wgtrGetServerProperty(this,"row_height");
    if (!row_height) row_height = height;

    // Build the child list
    var children = [];
    al_reflow_buildlist(this, children);
    var prevord = 100;
    for(var i=0; i<children.length; i++)
	{
	var child = children[i];
	var ord = wgtrGetServerProperty(child, "autolayout_order");
	if (ord == null || ord < 0) ord = prevord;
	child.__al_ord = ord;
	}
    children.sort(function(e1, e2) { return e1.ord - e2.ord; });

    // Set x and y
    var xo = 0;
    var yo = 0;
    var row_offset = 0;
    var column_offset = 0;
    var xalign = 0;
    var yalign = 0;

    // Determine alignment/offset first.
    for(var i=0; i<children.length; i++)
	{
	var child = children[i];
	var cwidth = wgtrGetServerProperty(child,"width");
	var cheight = wgtrGetServerProperty(child,"height");
	if (type == 'hbox')
	    {
	    if (xo + cwidth > width)
		{
		if (xo > 0 && row_height > 0 && row_offset + row_height*2 + spacing <= height)
		    {
		    row_offset += (row_height + spacing);
		    xo = 0;
		    i--;
		    continue;
		    }
		}
	    if (cwidth + xo > xalign)
		xalign = cwidth + xo;
	    xo += cwidth;
	    xo += spacing;
	    }
	else if (type == 'vbox')
	    {
	    if (yo + cheight > height)
		{
		if (yo > 0 && column_width > 0 && column_offset + column_width*2 + spacing <= width)
		    {
		    column_offset += (column_width + spacing);
		    yo = 0;
		    i--;
		    continue;
		    }
		}
	    if (cheight + yo > yalign)
		yalign = cheight + yo;
	    yo += cheight;
	    yo += spacing;
	    }
	}

    // Now actually set the geometry.
    var xo = 0;
    var yo = 0;
    var row_offset = 0;
    var column_offset = 0;
    if (align == 'center')
	{
	xalign = (width - xalign) / 2;
	yalign = (height - yalign) / 2;
	}
    else if (align == 'left')
	{
	xalign = 0;
	yalign = 0;
	}
    else
	{
	xalign = (width - xalign);
	yalign = (height - yalign);
	}
    for(var i=0; i<children.length; i++)
	{
	var child = children[i];
	var cwidth = wgtrGetServerProperty(child,"width");
	var cheight = wgtrGetServerProperty(child,"height");
	if (type == 'hbox')
	    {
	    if (xo + cwidth > width)
		{
		if (xo > 0 && row_height > 0 && row_offset + row_height*2 + spacing <= height)
		    {
		    row_offset += (row_height + spacing);
		    xo = 0;
		    i--;
		    continue;
		    }
		}
	    if (child.tagName)
		{
		setRelativeX(child, xo + xalign);
		if (wgtrGetServerProperty(child,"r_y") == -1)
		    setRelativeY(child, row_offset);
		else
		    setRelativeY(child, row_offset + wgtrGetServerProperty(child,"r_y"));
		}
	    xo += cwidth;
	    xo += spacing;
	    }
	else if (type == 'vbox')
	    {
	    if (yo + cheight > height)
		{
		if (yo > 0 && column_width > 0 && column_offset + column_width*2 + spacing <= width)
		    {
		    column_offset += (column_width + spacing);
		    yo = 0;
		    i--;
		    continue;
		    }
		}
	    if (child.tagName)
		{
		setRelativeY(child, yo + yalign);
		if (wgtrGetServerProperty(child,"r_x") == -1)
		    setRelativeX(child, column_offset);
		else
		    setRelativeX(child, column_offset + wgtrGetServerProperty(child,"r_x"));
		}
	    yo += cheight;
	    yo += spacing;
	    }
	}
    }

function al_init(l, wparam)
    {
    ifc_init_widget(l);
    l.childresize = al_childresize;
    l.Reflow = al_reflow;
    return l;
    }

// Load indication.
if (window.pg_scripts) pg_scripts['htdrv_autolayout.js'] = true;
