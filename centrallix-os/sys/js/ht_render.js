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


function htr_event(e)
    {
    var cx__event = new Object();
    if(cx__capabilities.Dom2Events)
	{
	cx__event.Dom2Event = e;
	cx__event.type = e.type;

	// move up from text nodes and spans to containers
	var t = e.target;
	while(t.nodeType == Node.TEXT_NODE || t.nodeName == 'SPAN')
	    t = t.parentNode;

	cx__event.target = t;

	cx__event.pageX = e.clientX;
	cx__event.pageY = e.clientY;
	}
    else if(cx__capabilities.Dom0NS)
	{
	cx__event.NSEvent = e;
	cx__event.type = e.type;
	cx__event.target = e.target;
	cx__event.pageX = e.pageX;
	cx__event.pageY = e.pageY;
	cx__event.which = e.which;
	cx__event.modifiers = e.modifiers;

	cx__event.x = e.x;
	cx__event.y = e.y;
	cx__event.width = e.width;
	cx__event.height = e.height;
	cx__event.layerX = e.layerX;
	cx__event.layerY = e.layerY;
	cx__event.which = e.which;
	cx__event.modifiers = e.modifiers;
	cx__event.data = e.data;
	cx__event.screenX = e.screenX;
	cx__event.screenY = e.screenY;
	}
    else if(cx__capabilities.Dom0IE)
	{
	cx__event.IEEvent = window.event;
	cx__event.type = e.type;
	cx__event.target = e.srcElement;
	cx__event.pageX = e.clientX;
	cx__event.pageY = e.clientY;
	}
    if(e.altKey && e.type && e.type.substring(0,5) == "mouse")
	{
	htr_alert_obj(cx__event,3);
	}
    return cx__event;
    }

function htr_alert_obj(obj,maxlevels)
    {
    if(window.alreadyopen)
	return;
    window.alreadyopen = true;
    var w = window.open();
    w.document.open();
    w.document.write("<pre>\n");
    w.document.write(htr_obj_to_text(obj,0,maxlevels));
    w.document.write("</pre>\n");
    w.document.close();
    }

function htr_obj_to_text(obj,level,maxlevels)
    {
    if(level >= maxlevels)
	return "";
    var j = "";
    for(var i in obj)
	{
	var attr = obj[i];
	if(typeof(attr)=='function')
	    attr = "function";
	if(i == 'innerHTML' || i == 'outerHTML')
	    attr = "[ HTML REMOVED ]";
	j+= htr_build_tabs(level) + i + ": " + attr + "\n";
	if(attr == obj[i])
	    j += htr_obj_to_text(obj[i],level+1,maxlevels);
	}
    return j;
    }

function htr_build_tabs(level)
    {
    if(level==0)
	return "";
    return "	"+htr_build_tabs(level-1);
    }
