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

// Form interaction functions 

function checkbox_getvalue()
    {
    return this.document.images[0].is_checked; /* not sure why, but it works - JDR */
    }

function checkbox_setvalue(v)
    {
    if (v)
	{
	this.document.images[0].src = this.document.images[0].checkedImage.src;
	this.document.images[0].is_checked = 1;
	this.is_checked = 1;
	}
    else
	{
	this.document.images[0].src = this.document.images[0].uncheckedImage.src;
	this.document.images[0].is_checked = 0;
	this.is_checked = 0;
	}
    }

function checkbox_clearvalue()
    {
    this.setvalue(false);
    }

function checkbox_resetvalue()
    {
    this.setvalue(false);
    }

function checkbox_enable()
    {
    this.enabled = true;
    this.document.images[0].uncheckedImage.enabled = this.enabled;
    this.document.images[0].checkedImage.enabled = this.enabled;
    this.document.images[0].enabled = this.enabled;
    this.document.images[0].uncheckedImage.src = '/sys/images/checkbox_unchecked.gif';
    this.document.images[0].checkedImage.src = '/sys/images/checkbox_checked.gif';
    if (this.is_checked)
	{
	this.document.images[0].src = this.document.images[0].checkedImage.src;
	}
    else
	{
	this.document.images[0].src = this.document.images[0].uncheckedImage.src;
	}
    }

function checkbox_readonly()
    {
    this.enabled = false;
    this.document.images[0].uncheckedImage.enabled = this.enabled;
    this.document.images[0].checkedImage.enabled = this.enabled;
    this.document.images[0].enabled = this.enabled;
    }

function checkbox_disable()
    {
    this.readonly();
    this.document.images[0].uncheckedImage.src = '/sys/images/checkbox_unchecked_dis.gif';
    this.document.images[0].checkedImage.src = '/sys/images/checkbox_checked_dis.gif';
    if (this.is_checked)
	{
	this.document.images[0].src = this.document.images[0].checkedImage.src;
	}
    else
	{
	this.document.images[0].src = this.document.images[0].uncheckedImage.src;
	}
    }


function checkbox_init(l,fieldname,checked) 
    {
    l.kind = 'checkbox';
    l.mainlayer = l;
    l.fieldname = fieldname;
    l.is_checked = checked;
    l.enabled = true;
    l.form = fm_current;
    if(cx__capabilities.Dom1HTML)
	{
	l.img = l.getElementsByTagName("img")[0];
	}
    else if(cx__capabilities.Dom0NS)
	{
	l.img = l.document.images[0];
	}
    l.img.kind = 'checkbox';
    l.img.form = l.form;
    l.img.parentLayer = l;
    l.img.layer = l;
    l.img.is_checked = l.is_checked;
    l.img.enabled = l.enabled;
    l.img.uncheckedImage = new Image();
    l.img.uncheckedImage.kind = 'checkbox';

    if(cx__capabilities.Dom1HTML)
	l.img.uncheckedImage.setAttribute('src','/sys/images/checkbox_unchecked.gif');
    else
	l.img.uncheckedImage.src = '/sys/images/checkbox_unchecked.gif';

    l.img.uncheckedImage.is_checked = l.is_checked;
    l.img.uncheckedImage.enabled = l.enabled;
    l.img.checkedImage = new Image();
    l.img.checkedImage.kind = 'checkbox';

    if(cx__capabilities.Dom1HTML)
	l.img.checkedImage.setAttribute('src','/sys/images/checkbox_checked.gif');
    else
	l.img.checkedImage.src = '/sys/images/checkbox_checked.gif';

    l.img.checkedImage.is_checked = l.is_checked;
    l.img.checkedImage.enabled = l.enabled;
    l.setvalue   = checkbox_setvalue;
    l.getvalue   = checkbox_getvalue;
    l.clearvalue = checkbox_clearvalue;
    l.resetvalue = checkbox_resetvalue;
    l.enable     = checkbox_enable;
    l.readonly   = checkbox_readonly;
    l.disable    = checkbox_disable;
    if (fm_current) fm_current.Register(l);
    return l;
    }

function checkbox_toggleMode(layer) 
    {
    layer = layer.img;
    if (layer.form) 
	{
	if (layer.parentLayer)
	    layer.form.DataNotify(layer.parentLayer);
	else
	    layer.form.DataNotify(layer);
	}
    if (layer.is_checked) 
	{
	if(cx__capabilities.Dom1HTML)
	    {
	    layer.setAttribute('src',layer.uncheckedImage.src);
	    }
	else
	    {
	    layer.src = layer.uncheckedImage.src;
	    }
	layer.is_checked = 0;
	} 
    else 
	{
	if(cx__capabilities.Dom1HTML)
	    {
	    layer.setAttribute('src',layer.checkedImage.src);
	    }
	else
	    {
	    layer.src = layer.checkedImage.src;
	    }
	layer.is_checked = 1;
	}
    cn_activate(layer.parentLayer, 'DataChange');
    }
