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


// This function will create an ibeam cursor, and 
// calculate text dimentions for use with text editing 
// of any sort.
function ibeam_init()
    {
    if (!text_metric || !ibeam_current)
	{
	text_metric = new Layer(24);
	text_metric.visibility = 'hidden';
	text_metric.document.write('<pre>xx</pre>');
	text_metric.document.close();
	w2 = text_metric.clip.width;
	h1 = text_metric.clip.height;
	text_metric.document.write('<pre>x\nx</pre>');
	text_metric.document.close();
	text_metric.charHeight = text_metric.clip.height - h1;
	text_metric.charWidth = w2 - text_metric.clip.width;
	ibeam_current = new Layer(1);
	ibeam_current.visibility = 'hidden';
	ibeam_current.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');
	ibeam_current.document.close();
	ibeam_current.resizeTo(1,text_metric.charHeight);
	ibeam_current.document.layer = ibeam_current;
	}
}
