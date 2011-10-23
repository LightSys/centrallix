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

/** Get a value function **/
function checkbox_getvalue()
    {
    return this.document.images[0].is_checked; /* not sure why, but it works - JDR */
    }

/** Set value function **/
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

/** Clear function **/
function checkbox_clearvalue()
    {
    this.setvalue(false);
    }

/** reset value function **/
function checkbox_resetvalue()
    {
    this.setvalue(false);
    }

/** enable function **/
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

/** read-only function - marks the widget as "readonly" **/
function checkbox_readonly()
    {
    this.enabled = false;
    this.document.images[0].uncheckedImage.enabled = this.enabled;
    this.document.images[0].checkedImage.enabled = this.enabled;
    this.document.images[0].enabled = this.enabled;
    }

/** disable function - disables the widget completely (visually too) **/
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

/** Checkbox initializer **/
function checkbox_init(param){
   var l = param.layer;
   l.kind = 'checkbox';
   l.fieldname = param.fieldname;
   l.is_checked = param.checked;
   l.enabled = true;
   l.form = fm_current;
   var img=l.getElementsByTagName('img')[0];
   img.kind = 'checkbox';
   img.form = l.form;
   img.parentLayer = l;
   img.is_checked = l.is_checked;
   img.enabled = l.enabled;
   img.uncheckedImage = new Image();
   img.uncheckedImage.kind = 'checkbox';
   img.uncheckedImage.src = "/sys/images/checkbox_unchecked.gif";
   img.uncheckedImage.is_checked = l.is_checked;
   img.uncheckedImage.enabled = l.enabled;
   img.checkedImage = new Image();
   img.checkedImage.kind = 'checkbox';
   img.checkedImage.src = "/sys/images/checkbox_checked.gif";
   img.checkedImage.is_checked = l.is_checked;
   img.checkedImage.enabled = l.enabled;
   l.setvalue   = checkbox_setvalue;
   l.getvalue   = checkbox_getvalue;
   l.clearvalue = checkbox_clearvalue;
   l.resetvalue = checkbox_resetvalue;
   l.enable     = checkbox_enable;
   l.readonly   = checkbox_readonly;
   l.disable    = checkbox_disable;
   if (fm_current) fm_current.Register(l);

   // Events
   ifc_init_widget(l);
   var ie = l.ifcProbeAdd(ifEvent);
   ie.Add("MouseDown");
   ie.Add("MouseUp");
   ie.Add("MouseOver");
   ie.Add("MouseOut");
   ie.Add("MouseMove");
}

/** Checkbox toggle mode function **/
function checkbox_toggleMode(layer) {
   if (layer.form) {
       if (layer.parentLayer) {
           layer.form.DataNotify(layer.parentLayer);
       } else {
           layer.form.DataNotify(layer);
       }
   }
   if (layer.is_checked) {
       layer.src = layer.uncheckedImage.src;
       layer.is_checked = 0;
   } else {
       layer.src = layer.checkedImage.src;
       layer.is_checked = 1;
   }
}


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_checkbox_moz.js'] = true;
