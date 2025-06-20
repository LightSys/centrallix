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
    return this.value;
    //return (this.is_checked == -1)?null:this.is_checked;
    }

function checkbox_action_setvalue(aparam)
    {
    this.setvalue(aparam.Value);
    if (this.form) this.form.DataNotify(this, false);
    }

function checkbox_setvalue(v)
    {
    this.is_checked_initial = null;
    if (v == '1') v = 1;
    else if (v == '0') v = 0;
    if (v == null)
	this.is_checked = -1;
    else
	this.is_checked = v?1:0;
    this.value = v;
    pg_set(this.img, 'src', this.imgfiles[this.enabled][this.is_checked+1]);
    }

function checkbox_clearvalue()
    {
    if (cx_hints_teststyle(this, cx_hints_style.notnull) && (!this.form || !this.form.IsQueryMode()))
	this.setvalue(0);
    else
	this.setvalue(null);
    this.is_checked_initial = -1;
    }

function checkbox_resetvalue()
    {
    this.clearvalue();
    }

function checkbox_enable()
    {
    this.enabled = 1;
    pg_set(this.img, 'src', this.imgfiles[this.enabled][this.is_checked+1]);
    }

function checkbox_readonly()
    {
    this.enabled = 0;
    pg_set(this.img, 'src', this.imgfiles[this.enabled][this.is_checked+1]);
    }

function checkbox_disable()
    {
    this.readonly();
    }


// page focus interaction functions

function checkbox_getfocus()
    {
    if (!this.enabled) 
	return 0;
    if (this.form) this.form.FocusNotify(this);
    return 1;
    }

function checkbox_losefocus()
    {
    return true;
    }

function checkbox_keyhandler(l,e,k)
    {
    if (k == 9)		// tab pressed
	{
	if (e.shiftKey)
	    {
	    if (this.form) this.form.ShiftTabNotify(this);
	    }
	else
	    {
	    if (this.form) this.form.TabNotify(this);
	    }
	}
    else if (k == 32)	// spacebar pressed
	{
	checkbox_toggleMode(this, true);
	}
    else if (k == 13)	// return pressed
	{
	if (this.form) this.form.RetNotify(this);
	}
    else if (k == 27)	// esc key pressed
	{
	if (this.form) this.form.EscNotify(this);
	}
    return true;
    }


// presentation hints (may) have changed
function checkbox_hintschanged(ht)
    {
    // Set default=0 if nulls not allowed
    if (ht != 'widget')
	{
	if (cx_hints_teststyle(this, cx_hints_style.notnull))
	    {
	    cx_set_hints(this, "DefaultExpr=0&Style=" + cx_hints_style.notnull + "," + cx_hints_style.notnull, "widget", true);
	    }
	else
	    {
	    cx_set_hints(this, "Style=" + cx_hints_style.notnull + "," + cx_hints_style.notnull, "widget", true);
	    }
	}

    // If NULL setting has changed, modify appearance appropriately, but
    // only if user has not already changed it to not-null
    if (this.is_checked_initial == -1)
	{
	if (this.is_checked == -1 && cx_hints_teststyle(this, cx_hints_style.notnull))
	    {
	    this.is_checked = 0;
	    this.value = 0;
	    pg_set(this.img, 'src', this.imgfiles[this.enabled][this.is_checked+1]);
	    }
	else if (this.is_checked == 0 && !cx_hints_teststyle(this, cx_hints_style.notnull))
	    {
	    this.is_checked = -1;
	    this.value = null;
	    pg_set(this.img, 'src', this.imgfiles[this.enabled][this.is_checked+1]);
	    }
	}
    }


// Event handlers
function checkbox_mousedown(e)
    {
    if (e.kind == 'checkbox' && e.layer.enabled)
	{
	checkbox_toggleMode(e.layer, false);
	cn_activate(e.layer, "MouseDown");
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function checkbox_mouseup(e)
    {
    if (e.kind == 'checkbox' && e.layer.enabled) 
	cn_activate(e.layer, "MouseUp");
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function checkbox_mouseover(e)
    {
    if (e.kind == 'checkbox' && e.layer.enabled) 
	cn_activate(e.layer, "MouseOver");
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function checkbox_mouseout(e)
    {
    if (e.kind == 'checkbox' && e.layer.enabled) 
	cn_activate(e.layer, "MouseOut");
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function checkbox_mousemove(e)
    {
    if (e.kind == 'checkbox' && e.layer.enabled) 
	cn_activate(e.layer, "MouseMove");
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// checked: -1 = null, 0 = unchecked, 1 = checked.
// enabled: 0 = disabled, 1 = enabled

function checkbox_init(param)
    {
    var l = param.layer;
    htr_init_layer(l, l, 'checkbox');
    l.fieldname = param.fieldname;
    l.is_checked = param.checked;
    l.is_checked_initial = param.checked;
    l.value = (l.is_checked == -1)?null:l.is_checked;
    l.enabled = param.enabled;
    if (param.form)
	l.form = wgtrGetNode(l, param.form);
    else
	l.form = wgtrFindContainer(l,"widget/form");
    var imgs = pg_images(l);
    imgs[0].kind = 'checkbox';
    imgs[0].layer = l;
    imgs[0].mainlayer = l;
    l.img = imgs[0];

    // image filenames
    l.imgfiles = new Array(0,1);
    l.imgfiles[0] = new Array('/sys/images/checkbox_null_dis.gif','/sys/images/checkbox_unchecked_dis.gif','/sys/images/checkbox_checked_dis.gif');
    l.imgfiles[1] = new Array('/sys/images/checkbox_null.gif','/sys/images/checkbox_unchecked.gif','/sys/images/checkbox_checked.gif');

    // hints interaction - default don't allow nulls - 
    // but app and data can override this.
    l.hintschanged = checkbox_hintschanged;
    cx_set_hints(l, "DefaultExpr=0&Style=" + cx_hints_style.notnull + "," + cx_hints_style.notnull, "widget", true);

    // focus interaction
    l.getfocushandler  = checkbox_getfocus;
    l.losefocushandler = checkbox_losefocus;
    l.keyhandler = checkbox_keyhandler;
    pg_addarea(l, -1, -1, 13, 13, 'cb', 'cb', 3);

    // form interaction
    l.setvalue   = checkbox_setvalue;
    l.getvalue   = checkbox_getvalue;
    l.clearvalue = checkbox_clearvalue;
    l.resetvalue = checkbox_resetvalue;
    l.enable     = checkbox_enable;
    l.readonly   = checkbox_readonly;
    l.disable    = checkbox_disable;
    if (l.form) l.form.Register(l);

    // Events
    ifc_init_widget(l);
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("DataChange");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", checkbox_action_setvalue);

    return l;
    }

function checkbox_toggleMode(l, from_kbd) 
    {
    if (!l.enabled) 
	return;

    // update the value
    l.is_checked++;
    if (l.is_checked == 2) 
	l.is_checked = -1;
    if (l.is_checked == -1 && cx_hints_teststyle(l,cx_hints_style.notnull) && (!l.form || !l.form.IsQueryMode()))
	l.is_checked = 0;
    this.is_checked_initial = null;

    l.value = (l.is_checked == -1)?null:l.is_checked;

    if (l.form) 
	{
	l.form.DataNotify(l);
	}

    // update the image
    pg_set(l.img, 'src', l.imgfiles[l.enabled][l.is_checked+1]);

    cn_activate(l, 'DataChange', {Value:l.value, FromKeyboard:from_kbd, FromOSRC:0});
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_checkbox.js'] = true;
