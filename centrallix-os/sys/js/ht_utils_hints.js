// Copyright (C) 1998-2004 LightSys Technology Services, Inc.
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
// ht_utils_hints.js - serialized presentation hints parsing
//

// cx_hints_style - style information for hints
// Keep in sync with obj.h in main distribution.
var cx_hints_style = new Object();
cx_hints_style.bitmask = 1;
cx_hints_style.list = 2;
cx_hints_style.buttons = 4;
cx_hints_style.notnull = 8;
cx_hints_style.strnull = 16;
cx_hints_style.grouped = 32;
cx_hints_style.readonly = 64;
cx_hints_style.hidden = 128;
cx_hints_style.password = 256;
cx_hints_style.multiline = 512;
cx_hints_style.highlight = 1024;
cx_hints_style.lowercase = 2048;
cx_hints_style.uppercase = 4096;
cx_hints_style.tabpage = 8192;
cx_hints_style.sepwindow = 16384;
cx_hints_style.alwaysdef = 32768;

// cx_set_hints() - initializes hints information for a given
// form field.
function cx_set_hints(element, hstr, hinttype)
    {
    if (!element.cx_hints) element.cx_hints = new Object();
    if (element.cx_hints.hstr == hstr) return;
    element.cx_hints[hinttype] = cx_parse_hints(hstr);
    cx_merge_hints(element);
    if (element.hintschanged) element.hintschanged();
    }


// cx_merge_hint_integer_min() - takes the minimum value of the two given
// integers and uses that.
function cx_merge_hint_integer_min(i1,i2)
    {
    if (i1 == null) return i2;
    if (i2 == null) return i1;
    if (i1 < i2) return i1;
    return i2;
    }

// cx_merge_hint_bitmask() - takes the bitwise OR of the two given
// integers.  b1 and b2 are the actual values, bm1 and bm2 specify
// which bits are valid in b1 and b2.  b1/bm1 settings take priority.
function cx_merge_hint_bitmask(b1, bm1, b2, bm2)
    {
    if (b1 == null) return b2;
    if (b2 == null) return b1;
    return (b2 & ~bm1) | b1;
    }

// cx_merge_hint_string() - if both strings are set, goes with the
// first one given.
function cx_merge_hint_string(s1, s2)
    {
    if (s1 == null) return s2;
    return s1;
    }

// cx_AND() - returns the logical AND of the two truth values
function cx_AND(v1, v2)
    {
    return v1 && v2;
    }

// cx_FIRST() - return the first value that is set.
function cx_FIRST(v1, v2)
    {
    if (v1 == null) return v2;
    return v1;
    }

// cx_merge_hint_expr() - merges two expressions into one, combining them
// with the given function.
function cx_merge_hint_expr(e1, e2, fn)
    {
    if (!e2) return e1;
    if (!e1) return e2;
    return fn + '((' + e1 + '),(' + e2 + '))';
    }

// cx_merge_hint_array() - merges two array lists of strings by picking
// out the common strings between them.
function cx_merge_hint_array(a1, a2)
    {
    var i;
    var j;
    var na = new Array();

	if (!a1) return a2;
	if (!a2) return a1;
	for(i=0;i<a1.length;i++)
	    {
	    for(j=0;j<a2.length;j++)
		{
		if (a1[i] == a2[j])
		    {
		    na.push(a1[i]);
		    }
		}
	    }

    return na;
    }

function cx_merge_two_hints(h1,h2)
    {
    var nh;

	nh = cx_parse_hints('');
	if (!h1 && !h2) return null;
	if (!h1) return h2;
	if (!h2) return h1;
	nh.Constraint = cx_merge_hint_expr(h1.Constraint, h2.Constraint, 'cx_AND');
	nh.DefaultExpr = cx_merge_hint_expr(h1.DefaultExpr, h2.DefaultExpr, 'cx_FIRST');
	nh.MinValue = cx_merge_hint_expr(h1.MinValue, h2.MinValue, 'min');
	nh.MaxValue = cx_merge_hint_expr(h1.MaxValue, h2.MaxValue, 'max');
	nh.EnumList = cx_merge_hint_array(h1.EnumList, h2.EnumList);
	nh.EnumQuery = cx_merge_hint_string(h1.EnumQuery, h2.EnumQuery);
	nh.Format = cx_merge_hint_string(h1.Format, h2.Format);
	nh.AllowChars = cx_merge_hint_string(h1.AllowChars, h2.AllowChars);
	nh.BadChars = cx_merge_hint_string(h1.BadChars, h2.BadChars);
	nh.Length = cx_merge_hint_integer_min(h1.Length, h2.Length);
	nh.VisualLength = cx_merge_hint_integer_min(h1.VisualLength, h2.VisualLength);
	nh.VisualLength2 = cx_merge_hint_integer_min(h1.VisualLength2, h2.VisualLength2);
	nh.BitmaskRO = cx_merge_hint_bitmask(h1.BitmaskRO, 0, h2.BitmaskRO, 0);
	nh.Style = cx_merge_hint_bitmask(h1.Style, h1.StyleMask, h2.Style, h2.StyleMask);
	nh.StyleMask = cx_merge_hint_bitmask(h1.StyleMask, 0, h2.StyleMask, 0);
	nh.GroupID = cx_merge_hint_integer_min(h1.GroupID, h2.GroupID);
	nh.OrderID = cx_merge_hint_integer_min(h1.OrderID, h2.OrderID);
	nh.GroupName = cx_merge_hint_string(h1.GroupNAme, h2.GroupName);
	nh.FriendlyName = cx_merge_hint_string(h1.FriendlyName, h2.FriendlyName);

    return nh;
    }

// cx_merge_hints() - take the different type of hints available in
// the form element, and merge them into one hints structure.
// Currently supports 'app' hints and 'data' hints.  The resulting
// hints info is the restrictive combination of the app and data hints.
// In the event of a conflict, app hints take priority (the data ones
// will be checked at the lower level in the server anyhow).
function cx_merge_hints(element)
    {
    var nh;

	// merge them.
	nh = cx_merge_two_hints(element.cx_hints['app'], element.cx_hints['data']);
	nh = cx_merge_two_hints(nh, element.cx_hints['widget']);
	if (!nh) nh = cx_parse_hints('');
	element.cx_hints['all'] = nh;

	// set display
	cx_hints_setup(element);

    return;
    }

// cx_parse_hints() - take a serialized hints string and return
// the components broken down into a presentation hints object.
function cx_parse_hints(hstr)
    {
    var ph = new Object();

	// default hint data
	ph.EnumQuery = null;
	ph.Format = null;
	ph.AllowChars = null;
	ph.BadChars = null;
	ph.Length = null;
	ph.VisualLength = null;
	ph.VisualLength2 = null;
	ph.BitmaskRO = 0;
	ph.Style = 0;
	ph.StyleMask = 0;
	ph.GroupID = null;
	ph.GroupName = null;
	ph.OrderID = null;
	ph.FriendlyName = null;
	ph.EnumList = null;
	ph.MinValue = null;
	ph.MaxValue = null;
	ph.Constraint = null;
	ph.DefaultExpr = null;

	// get the information array first.
	if (!hstr || !hstr.charAt) return ph;
	if (hstr.charAt(0) == "?") hstr = hstr.substr(1);
	ph.hstr = hstr;
	if (hstr == "") return ph;
	var pha = hstr.split("&");

	// Go through it one piece at a time.
	for(var i=0; i<pha.length; i++)
	    {
	    var attrval = pha[i].split("=",2);
	    if (attrval.length == 2)
		{
		switch(attrval[0])
		    {
		    case "DefaultExpr":
			ph.DefaultExpr = unescape(attrval[1]);
			break;
		    case "Constraint":
			ph.Constraint = unescape(attrval[1]);
			break;
		    case "MinValue":
			ph.MinValue = unescape(attrval[1]);
			break;
		    case "MaxValue":
			ph.MaxValue = unescape(attrval[1]);
			break;
		    case "EnumList":
			ph.EnumList = attrval[1].split(",");
			for(var j=0; j<ph.EnumList.length; j++)
			    {
			    ph.EnumList[j] = unescape(ph.EnumList[j]);
			    }
			break;
		    case "EnumQuery":
			ph.EnumQuery = unescape(attrval[1]);
			break;
		    case "Format":
			ph.Format = unescape(attrval[1]);
			break;
		    case "AllowChars":
			ph.AllowChars = unescape(attrval[1]);
			break;
		    case "BadChars":
			ph.BadChars = unescape(attrval[1]);
			break;
		    case "Length":
			ph.Length = parseInt(attrval[1]);
			break;
		    case "VisualLength":
			ph.VisualLength = parseInt(attrval[1]);
			break;
		    case "VisualLength2":
			ph.VisualLength2 = parseInt(attrval[1]);
			break;
		    case "BitmaskRO":
			ph.BitmaskRO = parseInt(attrval[1]);
			break;
		    case "Style":
			var twostyles = attrval[1].split(",",2);
			if (twostyles.length == 1) twostyles[1] = twostyles[0];
			ph.Style = parseInt(twostyles[0]);
			ph.StyleMask = parseInt(twostyles[1]);
			break;
		    case "GroupID":
			ph.GroupID = parseInt(attrval[1]);
			break;
		    case "GroupName":
			ph.GroupName = unescape(attrval[1]);
			break;
		    case "OrderID":
			ph.OrderID = parseInt(attrval[1]);
			break;
		    case "FriendlyName":
			ph.FriendlyName = unescape(attrval[1]);
			break;
		    }
		}
	    }

    return ph;
    }

// cx_hints_setup() - when the form element is just being set up on 
// screen or when the hints have changed, this sets up the element
// from the given hints.
function cx_hints_setup(e)
    {

	// Readonly?  Use the control's DISABLED property if so.
	if (e.cx_hints && (e.cx_hints['all'].Style & cx_hints_style.readonly) && e.disable)
	    e.disable();

    return;
    }

// cx_hints_startnew() - when creation of a record is beginning.
function cx_hints_startnew(e)
    {

	// Set default all the time
	if (e.cx_hints && e.cx_hints['all'].DefaultExpr) 
	    {
	    e.setvalue(eval(e.cx_hints['all'].DefaultExpr));
	    if (e.form) e.form.DataNotify(e);
	    }

    return;
    }

// cx_hints_startmodify() - when modification of a form element is
// beginning to take place.
function cx_hints_startmodify(e)
    {

	// Set default only if 'alwaysdef' enabled
	if (e.cx_hints && e.cx_hints['all'].DefaultExpr && (e.cx_hints['all'].Style & cx_hints_style.alwaysdef))
	    {
	    e.setvalue(eval(e.cx_hints['all'].DefaultExpr));
	    if (e.form) e.form.DataNotify(e);
	    }

    return;
    }

// cx_hints_checkmodify() - validate a modified value; returns ov if nv
// is not valid, or nv if the new value is valid.  May return a 
// modified new value.
function cx_hints_checkmodify(e, ov, nv)
    {

	// no change or no hints?
	if (ov == nv || !e.cx_hints) return nv;
	var h = e.cx_hints['all'];

	// uppercase/lowercase
	if ((h.Style & cx_hints_style.lowercase) && typeof nv == 'string') nv = nv.toLowerCase();
	if ((h.Style & cx_hints_style.uppercase) && typeof nv == 'string') nv = nv.toUpperCase();

	// max length
	if (h.Length && h.Length < ('' + nv).length) return ov;

	// badchars/allowchars
	if (h.AllowChars)
	    {
	    for(var i = 0; i<(''+nv).length; i++) if (h.AllowChars.indexOf((''+nv).charAt(i)) < 0) return ov;
	    }
	if (h.BadChars)
	    {
	    for(var i = 0; i<h.BadChars.length; i++) if ((''+nv).indexOf(h.BadChars.charAt(i)) >= 0) return ov;
	    }

    return nv;
    }


// cx_hints_teststyle() - test whether a given style bit is set or not.  Returns
// 0 if off, 1 if on, and null if not set at all.
function cx_hints_teststyle(e, b)
    {
    if (!e.cx_hints) return null;
    var h = e.cx_hints['all'];
    if (!(h.StyleMask & b)) return null;
    return (h.Style & b)?1:0;
    }
