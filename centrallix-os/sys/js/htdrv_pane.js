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

function pn_init(l,ml)
    {
    l.mainlayer = ml;
    l.kind = "pn";
    l.document.Layer = l;
    l.document.layer = ml;
    ml.kind = "pn";
    ml.document.Layer = ml;
    ml.document.layer = ml;
    ml.maxheight = l.clip.height-2;
    ml.maxwidth = l.clip.width-2;
    return l;
    }
