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
    var metrics = {};

    // are we mobile?
    var is_mobile = (window.navigator.userAgent.indexOf('Mobile') >= 0);

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

    // make the app big enough to get rid of the address bar on mobile platforms
    if (is_mobile)
	{
	if (screen.width == window.outerWidth)
	    // Firefox Mobile
	    metrics.page_h = parseInt(window.outerHeight*window.innerWidth/window.outerWidth + 0.5);
	else
	    // Chrome Mobile
	    metrics.page_h = parseInt(screen.height*window.document.body.clientWidth/screen.width + 0.5);
	}

    // Character width/height
    metrics.char_h = document.getElementById("l1").offsetHeight - document.getElementById("l2").offsetHeight;
    metrics.char_w = document.getElementById("l2").offsetWidth - document.getElementById("l1").offsetWidth;
    metrics.para_h = document.getElementById("l2").offsetHeight;

    loc += 'cx__geom=' + tohex16(metrics.page_w) + tohex16(metrics.page_h) + tohex16(metrics.char_w) + tohex16(metrics.char_h) + tohex16(metrics.para_h);
    window.location.replace(loc);
    }


// Load indication
if (window.pg_scripts) pg_scripts['startup.js'] = true;
