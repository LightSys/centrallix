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
    for (i=0; i < images.length; i++) {
	images[i].kind = t;
	images[i].layer = l;
	if (ml) images[i].mainlayer = ml;
	}
    }
