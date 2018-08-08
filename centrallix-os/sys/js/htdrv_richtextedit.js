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

/** Get value function for underlying text area **/
function rte_getvalue()
    {
    return this.richeditor.getData();
    }


//presets the RICHTEXTEDITOR's value
 function rte_setvalue(txt)
    {
    txt = htutil_obscure(txt);
    this.richeditor.setData(txt)
    }

//like set value, but fires events and notifications as well
function rte_action_set_value(ap)
    {
    var txt = ap.Value?ap.Value:"";
    this.richeditor.setData(txt);
    if (this.form) this.form.DataNotify(this, true);
    cn_activate(this, 'DataChange');
    }

function rte_action_insert_text(ap)
    {
    alert('insert text was called, a currently unimplemented function in the js file');
    /*var txt = ap.Text?ap.Text:"";
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
        */
    }

function rte_action_set_focus(aparam)
    {
    alert('set focus was called, a currently unimplemented function in the js file');
    /*
    var x = (typeof aparam.X == 'undefined')?null:aparam.X;
    var y = (typeof aparam.Y == 'undefined')?null:aparam.Y;
    pg_setkbdfocus(this, null, x, y);
    */
    }

/** Clear function **/
function rte_clearvalue()
    {
    this.richeditor.setData('');
    }

/** Enable control function **/
function rte_enable()
    {
    this.richeditor.setReadOnly(false);
    this.enabled='full';
    $(this.richeditor).prop('disabled',false);
    }

/** Disable control function **/
function rte_disable()
    {
    this.richeditor.setReadOnly(true);
    this.enabled='disabled';
    }

/** Readonly-mode function **/
function rte_readonly()
    {
    this.richeditor.setReadOnly(true);
    this.enabled='readonly';
    }

function rte_paste(e)
    {
    alert('the rte_paste function was called in the js file... why? its not implemented and ckeditor does it');
    }

function rte_receiving_input(e)
    {
    alert('rte_receiving_input was called, a unimplemented function in the js file');
    /*
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

    return; */
    }

function rte_keydown(e)
    {
    alert('rte_keydown was called, an unimplemented function in the js file.');
    /*var tx = this.mainlayer;

    // check before keypress...
    if (isCancel(tx.ifcProbe(ifEvent).Activate('BeforeKeyPress', {Code:e.data.keyCode, Name:htr_code_to_keyname(e.data.keyCode)})))
	{
	e.data.preventDefault();
	return;
	}

    if (e.data.keyCode == (KeyboardEvent.DOM_VK_TAB || 9) && !e.shiftKey)
	{
	if (tx.form) tx.form.TabNotify(tx);
	cn_activate(tx, 'TabPressed', {Shift:0});
	rte.DoDataChange(0, 1);
	}
    else if (e.data.keyCode == (KeyboardEvent.DOM_VK_TAB || 9) && e.shiftKey)
	{
	if (tx.form) tx.form.ShiftTabNotify(tx);
	cn_activate(tx, 'TabPressed', {Shift:1});
	rte.DoDataChange(0, 1);
	}
    else if (e.data.keyCode == (KeyboardEvent.DOM_VK_ESCAPE || 27))
	{
	if (tx.form) tx.form.EscNotify(tx);
	cn_activate(tx, 'EscapePressed', {});
	}
    else
	{
	return;
	}

    e.preventDefault();*/
    return;
    }

function rte_keyup(e)
    {
    alert('rte_keyup was called...how? look in the js file, currently does nothing.');
    }

function rte_keypress(e)
    {
     alert('rte_keypress was called...how? look in the js file, currently does nothing.');
    }

/** richtextedit keyboard handler **/
function rte_keyhandler(l,e,k)
    {
    if (l.readonly !='false') return 1;
    cn_activate(l, "KeyPress", {Code:k, Name:e.keyName, Modifiers:e.modifiers, Content:l.content});
    if (e.keyName == 'f3') return true;
    return false;
    }

/** Set focus to a new richtextedit **/
function rte_select(x,y,l,c,n,a,k)
    {
    if (this.enabled != 'full') return 0;
    this.focus();
    this.richeditor.focusManager.focus();
    var got_focus = $(this.richeditor).is(':focus');
    if (!got_focus)
      pg_addsched_fn(this.richeditor, function() { this.focus() }, {}, 200);
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
    this.rte.blur();
    this.richeditor.focusManager.blur();
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
    var nv = cx_hints_checkmodify(this, this.richeditor.getData(), this.richeditor.getData(), null, true);
    if (nv != this.content)
	{
	this.richeditor.setData(nv);

	}
    if (isCancel(this.ifcProbe(ifEvent).Activate('BeforeDataChange', {OldValue:this.richeditor.getData(nv), Value:nv, FromOSRC:from_osrc, FromKeyboard:from_kbd})))
	{

	return false;
	}
    var oldvalue = this.richeditor.getData();
    this.richeditor.setData(nv);
    cn_activate(this, "DataChange", {Value:this.value, OldValue:oldvalue, FromOSRC:from_osrc, FromKeyboard:from_kbd});
    }

/** richtextedit initializer **/
function rte_init(param)
    {
    var l = param.layer;
    ifc_init_widget(l);
    l.rte = $(l).find('textarea').get(0); //l.rte is the textarea object, NOT THE RICHTEXTEDITOR
    l.rte.value = '';
    htr_init_layer(l,l,'rte');
    htr_init_layer(l.rte, l, 'rte');
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



    var pxheight = $(l.rte).height(); //height and with found in centrallix or inherited
    var pxwidth = $(l.rte).width();


    //this is the richTextarea, using values from centrallix and config.js with l.id being a textarea to be replaced
    l.richeditor = CKEDITOR.replace(l.id,{customConfig: "config.js", height : pxheight, width : pxwidth });

    // upon loading of the CKEDITOR instance, resizes it to account for toolbars.
    // this means that the size given in centrallix will include the toolbar size as well
    l.richeditor.on('loaded',function(){l.richeditor.resize(pxwidth,pxheight,false);});

//The following function is meant to notice when data in the ckeditor is changing and set off the internal DoDataChange.
//There are two cases, when the editor is preset with data using fieldname and when it is  not.
//The change event fired by the ckeditor fires twice with most changes and fires when data is preloaded.
//The following If statement makes the init use two different listeners, one ignores the first change fire cauesed by the editor preloadihg.

    var preset='true';
    l.richeditor.on('afterSetData',function(){
        if (preset=='false'){
                l.richeditor.once('change',function(){
                        if (l.form) l.form.DataNotify(l, true);
                        cn_activate(l, 'DataChange');
                });
        }
        else{
                l.richeditor.once('change',function(){
                        l.richeditor.once('change',function(){
                                if (l.form) l.form.DataNotify(l, true);
                                cn_activate(l, 'DataChange');
                                preset='false';
                        });
                });
        }
    });


//listens for escape key(keycode 27), and tab (keycode 9).
// this listener still activates, but does not do anything on other key presses
        l.richeditor.on("key", function(evt){
                var tx = this.mainlayer;

                if(evt.data.keyCode==27){
                        var typingArea = l.richeditor.editable();
                        typingArea.hasFocus=false;

                        if(l.form) l.form.EscNotify(l);
                        cn_activate(l,'EscapePressed',{});
                        cn_activate(l,"LoseFocus",{});
                        l.losefocushandler();
                }
                if(evt.data.keyCode==9){

                        if (l.form) l.form.TabNotify(l);
        	        cn_activate(l, 'TabPressed', {Shift:0});
        	        l.DoDataChange(0, 1);
                        evt.cancel();
                }
        });

//the focus event is fired by the CKEDITOR when an editior instance GAINS focus.
// this listener activates the getfocushandler defined in init.
    l.richeditor.on('focus',function(e){
                l.getfocushandler();
    });

//non functional currently, lets the richtexteditor be a readonly field (if you would want that???)
    if (param.isReadonly)
        {
        l.enablemodify = rte_disable;
        l.enabled='disable';
        l.richeditor.on("loaded",function(){l.richeditor.setReadOnly(true);});
        }
    else
        {
        l.enablemodify = rte_enable;
        l.enabled='full';
        l.richeditor.on("loaded",function(){l.richeditor.setReadOnly(false);});
        }
    l.mode = param.mode; // 0=text, 1=html, 2=wiki
    l.isFormStatusWidget = false;
    if (cx__capabilities.CSSBox)
	pg_addarea(l, -1, -1, $(l).width()+3, $(l).height()+3, 'tbox', 'tbox', param.isReadonly?0:3);
    else
	pg_addarea(l, -1, -1, $(l).width()+1, $(l).height()+1, 'tbox', 'tbox', param.isReadonly?0:3);
    if (param.form)
	l.form = wgtrGetNode(l, param.form);
    else if (!(l.fieldname===""))
	l.form = wgtrFindContainer(l,"widget/form");

    if (l.form) l.form.Register(l);
    l.changed = false;
    l.value = null;
    l.content = null;
    l.changed = false;
    $(l.rte).on("input", rte_receiving_input);
    $(l.rte).on("keydown", rte_keydown);
    $(l.rte).on("keyup", rte_keyup);
    $(l.rte).on("keypress", rte_keypress);

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
function rte_mouseup(e)
    {
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function rte_mousedown(e)
    {
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function rte_mouseover(e)
    {
    if (e.kind == 'tx')
        {
        if (!rte_cur_mainlayer)
            {
            cn_activate(e.mainlayer, 'MouseOver');
            rte_cur_mainlayer = e.mainlayer;
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function rte_mousemove(e)
    {
    if (rte_cur_mainlayer && e.kind != 'rte')
        {
	// This is MouseOut Detection!
        cn_activate(rte_cur_mainlayer, 'MouseOut');
        rte_cur_mainlayer = null;
        }
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_richtextedit.js'] = true;
