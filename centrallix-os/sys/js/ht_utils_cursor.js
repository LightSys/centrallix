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


// This function will create an ibeam cursor, and 
// calculate text dimentions for use with text editing 
// of any sort.
function ibeam_init()
    {
    if (!text_metric || !ibeam_current)
	{	
	if (cx__capabilities.Dom1HTML)
	    {
	    text_metric = document.createElement("DIV");
	    //text_metric.style.width = 24;
	    text_metric.style.visibility = 'hidden';
	    text_metric.style.position = 'absolute';
	    text_metric.innerHTML = '<pre>xx</pre>';
	    //setClip(text_metric, 0,16,32,0);
	    document.body.appendChild(text_metric);
	    }
	else
	    {
	    // clip values will be written, each character (x) is 8px wide
	    text_metric = new Layer(24);
	    text_metric.visibility = 'hidden';
	    text_metric.document.write('<pre>xx</pre>');
	    text_metric.document.close();
	    }
	    
	var w2 = getClipWidth(text_metric);
	var h1 = getClipHeight(text_metric);
	    	    	    
	if (cx__capabilities.Dom1HTML)
	    {
	    text_metric = document.createElement("DIV");
	    //text_metric.style.width = 24;
	    text_metric.style.visibility = 'hidden';
	    text_metric.style.position = 'absolute';
	    text_metric.innerHTML='<pre>x\nx</pre>';
	    //setClip(text_metric, 0,8,48,0);
	    document.body.appendChild(text_metric);
	    }
	else
	    {
	    text_metric.document.write('<pre>x\nx</pre>');
	    text_metric.document.close();
	    }

	text_metric.charHeight = getClipHeight(text_metric) - h1;
	text_metric.charWidth = w2 - getClipWidth(text_metric);
	    
	if (cx__capabilities.Dom1HTML)
	    {
	    ibeam_current = document.createElement("DIV");
	    pg_set_style(ibeam_current,'width',"1");
	    htr_setvisibility(ibeam_current,'hidden');
	    htr_setbgcolor(ibeam_current, page.dtcolor1);
	    ibeam_current.style.position = 'absolute';
	    document.body.appendChild(ibeam_current);
	    }
	else
	    {
	    ibeam_current = new Layer(1);
	    ibeam_current.visibility = 'hidden';
	    ibeam_current.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');
	    ibeam_current.document.close();
	    }
	    
	resizeTo(ibeam_current,1,text_metric.charHeight);
	setClipHeight(ibeam_current, text_metric.charHeight);
	setClipWidth(ibeam_current, 1);
	if (ibeam_current.document && ibeam_current.document != document) 
	    ibeam_current.document.layer = ibeam_current;
	}
    }
