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

/** Function to handle nonfixed menu activation **/
function mn_activate(aparam)
    {
    x = aparam.X;
    y = aparam.Y;
    if (x == null) x = mn_last_x;
    if (y == null) y = mn_last_y;
    this.moveToAbsolute(x,y);
    this.visibility = 'visible';
    this.zIndex = (mn_top_z++);
    mn_current = this;
    }

/** Our initialization processor function. **/
function mn_init(l,is_p,is_h,po)
    {
/*    l.nofocus = true; */
    l.LSParent = po;
    l.kind = 'mn';
    l.ActionActivate = mn_activate;
    if (is_h == 0)
        {
        w=50;
        y = 0;
        for(i=0;i<l.document.layers.length;i++)
            {
            cl=l.document.layers[i];
            if (cl.clip.width > w) w = cl.clip.width;
            cl.top = y;
            cl.left = 0;
            y=y+cl.clip.height;
            }
        l.clip.height = y+1;
        l.clip.width = w+1;
        }
    else
        {
        x = 0;
        h=20;
        for(i=0;i<l.document.layers.length;i++)
            {
            cl=l.document.layers[i];
            if (cl.clip.height > h) h = cl.clip.height;
            cl.left = x;
            cl.top = 0;
            x=x+cl.clip.width;
            }
        if (l.clip.width < x+1) l.clip.width = x+1;
        l.clip.height = h+1;
        }
    if (is_p == 1) l.visibility = 'visible';
    } 

