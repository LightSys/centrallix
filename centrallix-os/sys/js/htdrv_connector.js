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
    if (t['Event' + f].constructor == Array)
	{
	for(var fn in t['Event' + f])
	    x = t['Event' + f][fn](eparam);
	return x;
	}
    else
	return t['Event' + f](eparam);
    }

function cn_add(w,e)
    {
    if (w['Event' + e] == null)
	w['Event' + e] = new Array();
    w['Event' + e][w['Event' + e].length] = this.RunEvent;
    }

function cn_init(p,f)
    {
    this.Add = cn_add;
    this.type = 'cn';
    this.LSParent = p;
    this.RunEvent = f;
    }
