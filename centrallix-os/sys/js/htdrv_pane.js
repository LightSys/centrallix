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
    if(!cx__capabilities.Dom0NS && cx__capabilities.CSS1)
	{
	ml = l;
	}

    l.mainlayer = ml;
    l.kind = "pn";
    ml.kind = "pn";

    if(cx__capabilities.Dom0NS)
	{
	l.document.layer = ml;
	ml.document.layer = ml;
	}
    else if(cx__capabilities.Dom1HTML)
	{
	l.layer = ml;
	ml.layer = ml;
	}
    else
	{
	alert('browser not supported');
	}

    ml.maxheight = l.clip.height-2;
    ml.maxwidth = l.clip.width-2;

    if(cx__capabilities.Dom0NS)
	{
	htutil_tag_images(l,'pn',ml,ml);
	}
    else if(cx__capabilities.Dom1HTML)
	{
	htutil_tag_images(l,'pn',ml,ml);
	}
    else
	{
	alert('browser not supported');
	}

    return l;
    }
