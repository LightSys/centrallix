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

/** Get value function **/
function rte_getvalue()
    {
    return this.value;
    }

/** Set value function **/
function rte_setvalue(txt)
    {
    txt = htutil_obscure(txt);
    this.content = txt;
    this.value = txt;
    this.txa.value = txt;
    }

function rte_action_set_value(ap)
    {
    var txt = ap.Value?ap.Value:"";
    this.setvalue(txt);
    if (this.form) this.form.DataNotify(this, true);
    cn_activate(this, 'DataChange');
    }

function rte_action_insert_text(ap)
    {
    var txt = ap.Text?ap.Text:"";
    if (ap.SetFocus && tx_current != this)
	{
	pg_setkbdfocus(this, null, null, null);
	}
    if (tx_current == this)
	{
	// Insert at insertion point / i-beam cursor as if user typed it
	for(var i = 0; i < txt.length; i++)
	    {
	    var k = txt.charCodeAt(i);
	    if (k >= 32 && k < 127)
		this.keyhandler(this, null, k);
	    }
	}
    else
	{
	// Append
	if (this.form) this.form.DataNotify(this, true);
	this.setvalue(this.getvalue() + txt);
	if (this.form) this.form.DataNotify(this, true);
	cn_activate(this, 'DataChange');
	}
    }

function rte_action_set_focus(aparam)
    {
    var x = (typeof aparam.X == 'undefined')?null:aparam.X;
    var y = (typeof aparam.Y == 'undefined')?null:aparam.Y;
    pg_setkbdfocus(this, null, x, y);
    }

/** Clear function **/
function rte_clearvalue()
    {
    this.setvalue(null);
    }

/** Enable control function **/
function tx_enable()
    {
    this.enabled='full';
    $(this.txa).prop('disabled',false);
    }

/** Disable control function **/
function tx_disable()
    {
    this.enabled='disabled';
    $(this.txa).prop('disabled',true);
    }

/** Readonly-mode function **/
function rte_readonly()
    {
    this.enabled='readonly';
    $(this.txa).prop('disabled',true);
    }

function rte_paste(e)
    {
    }

function rte_receiving_input(e)
    {
    var tx=this.mainlayer;
    var sel = document.getSelection();
    var range = sel.getRangeAt(0);
    var rstart = range.startOffset;
    var rend = range.endOffset;
    var curtxt = this.value;
    var changed = false;

    var oldtxt = tx.content;
    var newcurtxt = cx_hints_checkmodify(tx, oldtxt, curtxt, tx._form_type);
    if (newcurtxt != curtxt)
	{
	pg_addsched_fn(tx, function()
	    {
	    this.value = newcurtxt;
	    this.mainlayer.content = newcurtxt;
	    }, [], 10);
	}
    else
	{
	tx.content = curtxt;
	}
    tx.changed=true;
    cn_activate(tx,"DataModify", {Value:curtxt, FromKeyboard:1, FromOSRC:0, OldValue:oldtxt});
    if (tx.form) tx.form.DataNotify(tx);

    return;
    }

function rte_keydown(e)
    {
    var tx = this.mainlayer;

    // check before keypress...
    if (isCancel(tx.ifcProbe(ifEvent).Activate('BeforeKeyPress', {Code:e.keyCode, Name:htr_code_to_keyname(e.keyCode)})))
	{
	e.preventDefault();
	return;
	}

    if (e.keyCode == (KeyboardEvent.DOM_VK_TAB || 9) && !e.shiftKey)
	{
	if (tx.form) tx.form.TabNotify(tx);
	cn_activate(tx, 'TabPressed', {Shift:0});
	tx.DoDataChange(0, 1);
	}
    else if (e.keyCode == (KeyboardEvent.DOM_VK_TAB || 9) && e.shiftKey)
	{
	if (tx.form) tx.form.ShiftTabNotify(tx);
	cn_activate(tx, 'TabPressed', {Shift:1});
	tx.DoDataChange(0, 1);
	}
    else if (e.keyCode == (KeyboardEvent.DOM_VK_ESCAPE || 27))
	{
	if (tx.form) tx.form.EscNotify(tx);
	cn_activate(tx, 'EscapePressed', {});
	}
    else
	{
	return;
	}

    e.preventDefault();
    return;
    }

function rte_keyup(e)
    {
    }

function rte_keypress(e)
    {
    }

/** richtextedit keyboard handler **/
function rte_keyhandler(l,e,k)
    {
    if (l.enabled!='full') return 1;
    cn_activate(l, "KeyPress", {Code:k, Name:e.keyName, Modifiers:e.modifiers, Content:l.content});
    if (e.keyName == 'f3') return true;
    return false;
    }

/** Set focus to a new richtextedit **/
function rte_select(x,y,l,c,n,a,k)
    {
    if (this.enabled != 'full') return 0;
    this.txa.focus();
    var got_focus = $(this.txa).is(':focus');
    if (!got_focus)
	pg_addsched_fn(this.txa, function() { this.focus() }, {}, 200);
    this.has_focus = true;
    tx_current = this;
    if(this.form)
	if (!this.form.FocusNotify(this)) return 0;
    cn_activate(this, 'GetFocus');
    return 1;
    }

/** Take focus away from richtextedit **/
function rte_deselect(p)
    {
    this.txa.blur();
    this.has_focus = false;
    tx_current = null;
    if (this.changed)
	{
	if (!p || !p.nodatachange)
	    {
	    this.DoDataChange(0, 1);
	    this.changed=false;
	    }
	}
    cn_activate(this,"LoseFocus", {});
    return true;
    }

function rte_do_data_change(from_osrc, from_kbd)
    {
    var nv = cx_hints_checkmodify(this, this.value, this.content, null, true);
    if (nv != this.content)
	{
	this.value = nv;
	this.content = nv;
	this.txa.value = nv;
	}
    if (isCancel(this.ifcProbe(ifEvent).Activate('BeforeDataChange', {OldValue:this.value, Value:nv, FromOSRC:from_osrc, FromKeyboard:from_kbd})))
	{
	this.content = this.value;
	this.txa.value = this.value;
	return false;
	}
    this.oldvalue = this.value;
    this.value = nv;
    cn_activate(this, "DataChange", {Value:this.value, OldValue:this.oldvalue, FromOSRC:from_osrc, FromKeyboard:from_kbd});
    }

/** richtextedit initializer **/
function rte_init(param)
    {
    var l = param.layer;
    ifc_init_widget(l);
    l.txa = $(l).find('richtextedit').get(0);
    l.txa.value = ' ';
    htr_init_layer(l,l,'rte');
    htr_init_layer(l.txa, l, 'rte');
    l.fieldname = param.fieldname;
    l.keyhandler = rte_keyhandler;
    l.getfocushandler = rte_select;
    l.losefocushandler = rte_deselect;
    l.getvalue = rte_getvalue;
    l.setvalue = rte_setvalue;
    l.clearvalue = rte_clearvalue;
    l.DoDataChange = rte_do_data_change;
    l.setoptions = null;
    l.enablenew = rte_enable;
    l.disable = rte_disable;
    l.readonly = rte_readonly;
    if (param.isReadonly)
        {
        l.enablemodify = rte_disable;
        l.enabled = 'disable';
	$(l.txa).prop('disabled',true);
        }
    else
        {
        l.enablemodify = rte_enable;
        l.enabled = 'full';
	$(l.txa).prop('disabled',false);
        }
    l.mode = param.mode; // 0=text, 1=html, 2=wiki
    l.isFormStatusWidget = false;
    if (cx__capabilities.CSSBox)
	pg_addarea(l, -1, -1, $(l).width()+3, $(l).height()+3, 'tbox', 'tbox', param.isReadonly?0:3);
    else
	pg_addarea(l, -1, -1, $(l).width()+1, $(l).height()+1, 'tbox', 'tbox', param.isReadonly?0:3);
    if (param.form)
	l.form = wgtrGetNode(l, param.form);
    else
	l.form = wgtrFindContainer(l,"widget/form");
    if (l.form) l.form.Register(l);
    l.changed = false;
    l.value = null;
    l.content = null;
    l.changed = false;
    $(l.txa).on("input", rte_receiving_input);
    $(l.txa).on("keydown", rte_keydown);
    $(l.txa).on("keyup", rte_keyup);
    $(l.txa).on("keypress", rte_keypress);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("BeforeKeyPress");
    ie.Add("KeyPress");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    ie.Add("GetFocus");
    ie.Add("LoseFocus");
    ie.Add("DataChange");
    ie.Add("DataModify");
    ie.Add("TabPressed");
    ie.Add("EscapePressed");

    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("InsertText", rte_action_insert_text);
    ia.Add("SetFocus", rte_action_set_focus);
    ia.Add("SetValue", rte_action_set_value);

    return l;
    }

// Event handlers
function tx_mouseup(e)
    {
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tx_mousedown(e)
    {
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tx_mouseover(e)
    {
    if (e.kind == 'tx')
        {
        if (!tx_cur_mainlayer)
            {
            cn_activate(e.mainlayer, 'MouseOver');
            tx_cur_mainlayer = e.mainlayer;
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tx_mousemove(e)
    {
    if (tx_cur_mainlayer && e.kind != 'rte')
        {
	// This is MouseOut Detection!
        cn_activate(tx_cur_mainlayer, 'MouseOut');
        tx_cur_mainlayer = null;
        }
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_richtextedit.js'] = true;
