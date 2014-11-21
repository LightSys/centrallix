// Copyright (C) 1998-2001 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function htutil_tag_images(d,t,l,ml)
    {
    var images = pg_images(d);
    if (!images) return;
    for (var i=0; i < images.length; i++) {
	images[i].kind = t;
	images[i].layer = l;
	if (ml) images[i].mainlayer = ml;
	}
    }

function htutil_point(wthis, x, y, at, bc, fc, p1, p2)
    {
    // Determine x/y to point at
    if (at)
	{
	if (typeof at == 'object' && wgtrIsNode(at))
	    var widget = at;
	else
	    var widget = wgtrGetNode(at);
	if (!widget)
	    return { "p1":p1, "p2":p2 };
	}
    else if (x || y)
	{
	x = x?x:0;
	y = y?y:0;
	}
    else
	{
	if (p1)
	    {
	    $(p1).css({"visibility": "hidden"});
	    $(p2).css({"visibility": "hidden"});
	    }
	return { "p1":p1, "p2":p2 };
	}

    // Get width/height of the pane
    var w = $(wthis).outerWidth();
    var h = $(wthis).outerHeight();

    // Ensure x/y is within the "point diamond"
    if (x+y <= 0 || x+y >= w+h || x-y >= w || y-x >= h)
	return { "p1":p1, "p2":p2 };

    // Determine border color and fill color
    bc = bc?bc:wgtrGetServerProperty(wthis, "border_color", "black");
    fc = fc?fc:wgtrGetServerProperty(wthis, "bgcolor", "white");

    // Compute side/direction, offset, and size
    if (x<0)
	{
	var side = 'left';   var size = 0-x; var offset = y-size; var top = offset; var left = -size*2;
	var c1 = "transparent " + bc + " transparent transparent";
	var c2 = "transparent " + fc + " transparent transparent";
	var doffs = {x:1, y:0};
	}
    else if (y<0)
	{
	var side = 'top';    var size = 0-y; var offset = x-size; var top = -size*2; var left = offset;
	var c1 = "transparent transparent " + bc + " transparent";
	var c2 = "transparent transparent " + fc + " transparent";
	var doffs = {x:0, y:1};
	}
    else if (x>w)
	{
	var side = 'right';  var size = x-w; var offset = y-size; var top = offset; var left = w - 1;
	var c1 = "transparent transparent transparent " + bc;
	var c2 = "transparent transparent transparent " + fc;
	var doffs = {x:-1, y:0};
	}
    else if (y>h)
	{
	var side = 'bottom'; var size = y-h; var offset = x-size; var top = h - 1; var left = offset;
	var c1 = bc + " transparent transparent transparent";
	var c2 = fc + " transparent transparent transparent";
	var doffs = {x:0, y:-1};
	}
    if (size%2 == 0)
	{
	// avoid stair-stepping
	size++; 
	offset--;
	if (side == 'left') left -= 2;
	if (side == 'top') top -= 2;
	}

    // Create the "point divs" if needed
    if (!p1)
	{
	p1 = htr_new_layer(size*2, document);
	p2 = htr_new_layer(size*2, document);
	}

    // Set the CSS to enable the point divs
    $(p1).css
	({
	"position": "absolute",
	"width": size*2 + "px",
	"height": size*2 + "px",
	"border-width": size + "px",
	"border-style": "solid",
	"box-sizing": "border-box",
	"content": "",
	"top": (top + $(wthis).offset().top) + "px",
	"left": (left + $(wthis).offset().left) + "px",
	"border-color": c1,
	"visibility": "inherit",
	"z-index": htr_getzindex(wthis) + 1
	});
    $(p2).css
	({
	"position": "absolute",
	"width": size*2 + "px",
	"height": size*2 + "px",
	"border-width": size + "px",
	"border-style": "solid",
	"box-sizing": "border-box",
	"content": "",
	"top": (top + doffs.y + $(wthis).offset().top) + "px",
	"left": (left + doffs.x + $(wthis).offset().left) + "px",
	"border-color": c2,
	"visibility": "inherit",
	"z-index": htr_getzindex(wthis) + 2
	});

    return { "p1":p1, "p2":p2 };
    }


// Load indication
if (window.pg_scripts) pg_scripts['ht_utils_layers.js'] = true;
