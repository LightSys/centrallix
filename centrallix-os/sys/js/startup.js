// Copyright (C) 1998-2006 LightSys Technology Services, Inc.
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
//
// This file is used when an app is initially loaded and the browser
// metrics have not yet been sent to the server.  These metrics control
// page layout operation on the server.
//

function tohex16(n)
    {
    var digits = ['0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'];
    return '' + digits[(n/4096)&0xF] + digits[(n/256)&0xF] + digits[(n/16)&0xF] + digits[n&0xF];
    }


function startup()
    {
    var loc = window.location.href;
    //var re1 = new RegExp('cx__[^&]*');
    //var re2 = new RegExp('([?&])&*');
    var metrics = new Object();
    //while(loc.match(re1))
    //    {
    //    loc = loc.replace(re1,'');
    //    }
    //while(loc.match(re2))
    //    {
    //    loc = loc.replace(re2,'');
    //    }
    //loc = loc.replace(new RegExp('[?&]*$'),'');
    loc = loc.replace(new RegExp('([?&])cx__geom[^&]*([&]?)'),
	    function (str,p1,p2) { return p2?p1:''; });
    if (loc.indexOf('?') >= 0)
        loc += '&';
    else
        loc += '?';
    if (window.innerHeight)
        {
	metrics.page_w = window.innerWidth;
	metrics.page_h = window.innerHeight;
        }
    else if (window.document.body && window.document.body.clientWidth)
        {
	metrics.page_w = window.document.body.clientWidth;
	metrics.page_h = window.document.body.clientHeight;
        }
    if (document.getElementById)
	{
	// IE, Moz
	metrics.char_h = document.getElementById("l1").offsetHeight - document.getElementById("l2").offsetHeight;
	metrics.char_w = document.getElementById("l2").offsetWidth - document.getElementById("l1").offsetWidth;
	metrics.para_h = document.getElementById("l2").offsetHeight;
	/*if (metrics.char_w == 0)
	    {
	    // browser sized widths to whole screen width?
	    re = /rect\((.*), (.*), (.*), (.*)\)/;
	    c1 = getComputedStyle(document.getElementById("l2"), null).getPropertyCSSValue('clip').cssText;
	    c2 = getComputedStyle(document.getElementById("l1"), null).getPropertyCSSValue('clip').cssText;
	    metrics.char_w = (re.exec(c1))[2] - (re.exec(c2))[2];
	    }
	confirm(metrics.char_w);*/
	}
    else
	{
	// NS4
	metrics.char_h = document.layers["l1"].clip.height - document.layers["l2"].clip.height;
	metrics.char_w = document.layers["l2"].clip.width - document.layers["l1"].clip.width;
	metrics.para_h = document.layers["l2"].clip.height;
	}
    //loc += 'cx__geom=' + metrics.page_w + 'x' + metrics.page_h;
    loc += 'cx__geom=' + tohex16(metrics.page_w) + tohex16(metrics.page_h) + tohex16(metrics.char_w) + tohex16(metrics.char_h) + tohex16(metrics.para_h);
    window.location.replace(loc);
    }

