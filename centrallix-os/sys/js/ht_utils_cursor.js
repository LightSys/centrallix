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
	    text_metric.style.width = 24;
	    text_metric.style.visibility = 'hidden';
	    text_metric.innerHTML = '<pre>xx</pre>';
	    setClip(text_metric, 0,16,32,0);
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
	    
	if (cx__capabilities.Dom0IE)
	    {
	    var w2 = getClipWidth(text_metric);
	    var h1 = getClipHeight(text_metric);
	    }
	else if (cx__capabilities.Dom0NS)
	    {	    
	    var w2 = text_metric.clip.width;
	    var h1 = text_metric.clip.height;
	    }
	else
	    {
	    alert("browser is not supported -- ibeam_init()");
	    }
	    	    	    
	if (cx__capabilities.Dom1HTML)
	    {
	    text_metric.innerHTML='<pre>x\nx</pre>';
	    setClip(text_metric, 0,8,48,0);
	    }
	else
	    {
	    text_metric.document.write('<pre>x\nx</pre>');
	    text_metric.document.close();
	    }
	if (cx__capabilities.Dom0IE)
	    {
	    text_metric.charHeight = getClipHeight(text_metric) - h1;
	    text_metric.charWidth = w2 - getClipWidth(text_metric);
	    }
	else if (cx__capabilities.Dom0NS)
	    {
	    text_metric.charHeight = text_metric.clip.height - h1;
	    text_metric.charWidth = w2 - text_metric.clip.width;
	    }
	else
	    {
	    alert("browser is not supported -- ibeam_init()");
	    }
	    
	if (cx__capabilities.Dom1HTML)
	    {
	    ibeam_current = document.createElement("DIV");
	    ibeam_current.style.width = "1px";
	    ibeam_current.runtimeStyle.visibility = 'hidden';
	    ibeam_current.runtimeStyle.backgroundColor = page.dtcolor1;	    
	    pg_set_style(ibeam_current, 'position', 'absolute');	    
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
	ibeam_current.document.layer = ibeam_current;
	}
}
