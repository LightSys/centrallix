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

function cn_activate(t,f,eparam)
    {
    if (!t || !t['Event'+f]) return;
    if (!eparam)
	{
	eparam = new Object();
	eparam.Caller = t;
	var d = 1;
	}
    if (t['Event' + f].constructor == Array)
	{
	for(var fn in t['Event' + f])
	    {
	    var x = t['Event' + f][fn](eparam);
	    }
	if(d) delete eparam;
	return x;
	}
    else
	{
	if(d) delete eparam;
	return t['Event' + f](eparam);
	}
    }

// would be nice if this could go through the wgtr module, but the
// sequence of events at startup makes that tricky - this gets called
// before the wgtr stuff is initialized
function cn_add(e)
    {
    if (this.LSParent['Event' + e] == null)
	this.LSParent['Event' + e] = new Array();
    this.LSParent['Event' + e][this.LSParent['Event' + e].length] = this.RunEvent;
    }

function cn_init(p,f)
    {
    this.Add = cn_add;
    this.type = 'cn';
    this.LSParent = p;
    this.RunEvent = f;
    }
