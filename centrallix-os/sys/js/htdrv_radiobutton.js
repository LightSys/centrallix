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

function rb_getvalue() {
	if (this.form && this.form.mode == "Query"){
	    var vals = new Array();
	    for(var i in this.buttonList){
		if(htr_getvisibility(this.buttonList[i].optionPane.setPane) == 'inherit')
		    vals.push(this.buttonList[i].optionValue);
	    }
	    return vals;
	}
	if (this.selectedOption) /* return "" if nothing is selected */
		return this.selectedOption.optionValue;
	 else
		return null;
}

// Luke (03/04/02) -
// The behavior of this is in direct combination with the clearvalue function.
// if set_value() gets called with a value that _is_ in the list of values, then
/// obviously that button gets marked.  However, if the called parameter is not
// in the list, an alert message pops up.  SEE ALSO:  Note on clearvalue()
function rb_setvalue(v) {
	for (var i=0; i < this.buttonList.length; i++) {
		if (this.buttonList[i].optionValue == v) {
			radiobutton_toggle(this.buttonList[i]);
			return;
		}
	}
	this.clearvalue();
	//alert('Warning: "'+v+'" is not in the radio button list.');
}

function rb_cb_getvalue(p) {
	return this.getvalue();
}

function rb_cb_setvalue(p, v) {
	return this.setvalue(v);
}

function rb_action_setvalue(aparam) {
	if (typeof aparam.Value != 'undefined') {
		this.ifcProbe(ifValue).Changing('value', aparam.Value, true, this.value, true);
		return this.setvalue(aparam.Value);
	}
	else
		return null;
}


/*  Luke (03/04/02)  This unchecks ALL radio buttons.  As such, nothing is selected. */
function rb_clearvalue() {

	if(this.SelectedOption){
		htr_setvisibility(this.selectedOption.unsetPane, 'inherit');
		htr_setvisibility(this.selectedOption.setPane, 'hidden');
		this.selectedOption = null;
		this.value = null;
		this.valueIndex = null;
	}
}

function rb_resetvalue() {
	if (this.selectedOption != this.defaultSelectedOption) {
		radiobutton_toggle(this.defaultSelectedOption);
		this.value = this.selectedOption.optionValue;
		this.valueIndex = this.selectedOption.valueIndex;
	}
}

function rb_enable() {
	this.mainlayer.enabled = true;
	for (i=0;i<this.buttonList.length;i++) {
		pg_set(this.buttonList[i].setImage, 'src', '/sys/images/radiobutton_set.gif');
		pg_set(this.buttonList[i].unsetImage, 'src', '/sys/images/radiobutton_unset.gif');
	}
}

function rb_readonly() {
	this.mainlayer.enabled = false;
	for (i=0;i<this.buttonList.length;i++) {
	}
}

function rb_disable() {
	this.mainlayer.enabled = false;
	for (i=0;i<this.buttonList.length;i++) {
		pg_set(this.buttonList[i].setImage, 'src', '/sys/images/radiobutton_set_dis.gif');
		pg_set(this.buttonList[i].unsetImage, 'src', '/sys/images/radiobutton_unset_dis.gif');
	}
}

function add_radiobutton(optionPane, param) {
	var rb = wgtrGetParent(optionPane);
	rb.rbCount++;
	optionPane.valueIndex = rb.rbCount;
	/*optionPane.kind = 'radiobutton';
	optionPane.document.layer = optionPane;*/
	htr_init_layer(optionPane, rb, 'radiobutton');
	//optionPane.mainlayer = rb;
	optionPane.optionPane = optionPane;
	optionPane.isSelected = param.selected;
	optionPane.valueStr = param.valuestr;
	optionPane.labelStr = param.labelstr;

	optionPane.setPane = param.buttonset;
	optionPane.setPane.optionPane = optionPane;
	htr_init_layer(optionPane.setPane, rb, 'radiobutton');
	htutil_tag_images(optionPane.setPane, 'radiobutton', optionPane.setPane, rb);
	optionPane.setImage = pg_images(optionPane.setPane)[0];

	optionPane.unsetPane = param.buttonunset;
	optionPane.unsetPane.optionPane = optionPane;
	htr_init_layer(optionPane.unsetPane, rb, 'radiobutton');
	htutil_tag_images(optionPane.unsetPane, 'radiobutton', optionPane.unsetPane, rb);
	optionPane.unsetImage = pg_images(optionPane.unsetPane)[0];

	optionPane.labelPane = param.label;
	optionPane.labelPane.optionPane = optionPane;
	htr_init_layer(optionPane.labelPane, rb, 'radiobutton');

	optionPane.valuePane = param.value;
	optionPane.valuePane.optionPane = optionPane;
	htr_init_layer(optionPane.valuePane, rb, 'radiobutton');
	var lnk = pg_links(optionPane.valuePane);
	if (lnk && lnk[0])
	    optionPane.optionValue = lnk[0].text;
	else
	    optionPane.optionValue = null;

	rb.buttonList.push(optionPane);
	if (param.selected) {
		htr_setvisibility(optionPane.setPane, 'inherit');
		htr_setvisibility(optionPane.unsetPane, 'hidden');
		rb.selectedOption = optionPane;
		rb.value = rb.selectedOption.optionValue;
		rb.valueIndex = rb.selectedOption.valueIndex;
		rb.defaultSelectedOption = optionPane;
	} else {
		htr_setvisibility(optionPane.setPane, 'hidden');
		htr_setvisibility(optionPane.unsetPane, 'inherit');
	}

	optionPane.yOffset = getRelativeY(optionPane)+getRelativeY(rb.coverPane)+getRelativeY(rb.borderPane);
	pg_addarea(rb, getRelativeX(optionPane), optionPane.yOffset, getClipWidth(optionPane), pg_parah+4, optionPane, 'rb', 3);
}

function rb_getfocus(xo,yo,l,c,n,a,from_kbd)
    {
    cn_activate(this, "GetFocus", {});
    if(this.form) this.form.FocusNotify(this);
    this.kbdSelected = c;
    return 1;
    }

function rb_losefocus()
    {
    this.kbdSelected = null;
    cn_activate(this, "LoseFocus", {});
    return true;
    }

function rb_keyhandler(l, e, k)
    {
    if (k == 9 && !e.shiftKey) // tab pressed
	{
	for(var i=0;i<this.buttonList.length;i++)
	    {
	    if (this.kbdSelected == this.buttonList[i])
		{
		if (i < this.buttonList.length-1)
		    {
		    this.kbdSelected = this.buttonList[i+1];
		    pg_setkbdfocus(this, null, 10, this.kbdSelected.yOffset+5);
		    }
		else if (!this.form)
		    {
		    this.kbdSelected = this.buttonList[0];
		    pg_setkbdfocus(this, null, 10, this.kbdSelected.yOffset+5);
		    }
		else
		    {
		    this.form.TabNotify(this);
		    }
		break;
		}
	    }
	}
    else if (k == 9 && e.shiftKey) // shift-tab
	{
	for(var i=0;i<this.buttonList.length;i++)
	    {
	    if (this.kbdSelected == this.buttonList[i])
		{
		if (i > 0)
		    {
		    this.kbdSelected = this.buttonList[i-1];
		    pg_setkbdfocus(this, null, 10, this.kbdSelected.yOffset+5);
		    }
		else if (!this.form)
		    {
		    this.kbdSelected = this.buttonList[this.buttonList.length-1];
		    pg_setkbdfocus(this, null, 10, this.kbdSelected.yOffset+5);
		    }
		else
		    {
		    this.form.ShiftTabNotify(this);
		    }
		break;
		}
	    }
	}
    else if (k == 32) // space pressed
	{
	if (this.kbdSelected)
	    {
	    this.ifcProbe(ifValue).Changing('value', this.kbdSelected.value, true, this.value, true);
	    radiobutton_toggle(this.kbdSelected);
	    }
	}
    else if (k == 13) // return pressed
	{
	if (this.kbdSelected && (!this.form || this.form.mode != "Query"))
	    {
	    this.ifcProbe(ifValue).Changing('value', this.kbdSelected.value, true, this.value, true);
	    radiobutton_toggle(this.kbdSelected);
	    }
	if (this.form) this.form.RetNotify(this);
	}
    else if (k == 27) // esc pressed
	{
	if (this.form) this.form.EscNotify(this);
	}
    else if ((k >= 65 && k <= 90) || (k >= 97 && k <= 122) || (k >= 48 && k <= 57))
	{
	// find as you type
	if (k < 97 && k > 57) 
	    {
	    var k_lower = k + 32;
	    var k_upper = k;
	    k = k + 32;
	    }
	else if (k > 57)
	    {
	    var k_lower = k;
	    var k_upper = k - 32;
	    }
	else
	    {
	    var k_lower = k;
	    var k_upper = k;
	    }

	this.time_stop = new Date();
	if (this.time_start && this.time_start < this.time_stop 
		&& (this.time_stop - this.time_start) >= 1000) 
	    {
	    this.keystring = null;
	    this.match = null;
	    this.lastmatch = null;
	    this.time_start = null;
	    this.time_stop = null;
	    }
		
	if (!this.keystring)
	    {
	    for (var i=0; i < this.buttonList.length; i++)
		{
		if (this.buttonList[i].labelStr.substring(0, 1) == 
			String.fromCharCode(k_upper) ||
		    this.buttonList[i].labelStr.substring(0, 1) == 
			String.fromCharCode(k_lower))
		    {
		    radiobutton_toggle(this.buttonList[i]);
		    this.ifcProbe(ifValue).Changing('value', this.buttonList[i].value, true, this.value, true);
		    this.kbdSelected = this.buttonList[i];
		    pg_setkbdfocus(this, null, 10, this.kbdSelected.yOffset+1);
		    this.lastmatch = this.buttonList[i];
		    break;
		    }
		}
	    this.keystring = String.fromCharCode(k);
	    this.time_start = new Date();

	    if (!this.lastmatch) 
		{
		this.keystring = null;
		this.match = null;
		this.lastmatch = null;
		this.time_start = null;
		this.time_stop = null;
		this.clearvalue();
		}
	    }
	
	//find as you type code
	else
	    {
	    this.keystring += String.fromCharCode(k);
	    this.match = false;
	    this.time_start = new Date();
			
	    for (var i=0; i < this.buttonList.length; i++) 
		{
		if ((this.buttonList[i].labelStr.substring(0, 
			this.keystring.length).toLowerCase())
			== this.keystring && !this.match)
		    {
		    radiobutton_toggle(this.buttonList[i]);
		    this.kbdSelected = this.buttonList[i];
		    pg_setkbdfocus(this, null, 10, this.kbdSelected.yOffset+1);
		    //found a good match
		    this.match = true;
		    this.lastmatch = this.buttonList[i];
		    break;
		    }		
		}
	    if (!this.match) 
		{
		this.keystring = this.keystring.substring(0, 
			(this.keystring.length - 1));
		radiobutton_toggle(this.lastmatch);
		if (this.lastmatch)
		    {
		    this.kbdSelected = this.lastmatch;
		    pg_setkbdfocus(this, null, 10, this.kbdSelected.yOffset+1);
		    }
	    	this.match = true;
		}
	    }
	}
    return false;
    }

function rb_changemode(){
    if(this.form && this.form.mode == 'Query'){
	for (i=0;i<this.buttonList.length;i++) {
		pg_set(this.buttonList[i].setImage, 'src', '/sys/images/checkbox_checked.gif');
		pg_set(this.buttonList[i].unsetImage, 'src', '/sys/images/checkbox_unchecked.gif');
		//htutil_tag_images(this.buttonList[i].optionPane.setPane, 'radiobutton', this.buttonList[i].optionPane.setPane, 'rb');	
		//htutil_tag_images(this.buttonList[i].optionPane.unsetPane, 'radiobutton', this.buttonList[i].optionPane.unsetPane, 'rb');
	}
	//clear the value
	if(this.mainlayer.selectedOption){
		htr_setvisibility(this.mainlayer.selectedOption.unsetPane, 'inherit');
		htr_setvisibility(this.mainlayer.selectedOption.setPane, 'hidden');
		this.mainlayer.selectedOption = null;
	}

    }
    else {
	if(this.mainlayer.enabled)
	    for (i=0;i<this.buttonList.length;i++) {
		pg_set(this.buttonList[i].setImage, 'src', '/sys/images/radiobutton_set.gif');
		pg_set(this.buttonList[i].unsetImage, 'src', '/sys/images/radiobutton_unset.gif');
		if(this.buttonList[i].optionPane != this.mainlayer.selectedOption){
		    htr_setvisibility(this.buttonList[i].optionPane.unsetPane, 'inherit');
		    htr_setvisibility(this.buttonList[i].optionPane.setPane, 'hidden');
		}
		else{
		    htr_setvisibility(this.buttonList[i].optionPane.unsetPane, 'hidden');
		    htr_setvisibility(this.buttonList[i].optionPane.setPane, 'inherit');
		}
	    }
	else
	    for (i=0;i<this.buttonList.length;i++) {
		pg_set(this.buttonList[i].setImage, 'src', '/sys/images/radiobutton_set_dis.gif');
		pg_set(this.buttonList[i].unsetImage, 'src', '/sys/images/radiobutton_unset_dis.gif');
		if(this.buttonList[i].optionPane != this.mainlayer.selectedOption){
		    htr_setvisibility(this.buttonList[i].optionPane.unsetPane, 'inherit');
		    htr_setvisibility(this.buttonList[i].optionPane.setPane, 'hidden');
		}
		else{
		    htr_setvisibility(this.buttonList[i].optionPane.unsetPane, 'hidden');
		    htr_setvisibility(this.buttonList[i].optionPane.setPane, 'inherit');
		}
	    }
    }
}

function radiobuttonpanel_init(param) {
	var parentPane = param.parentPane;
	var borderpane = param.borderPane;
	var coverpane = param.coverPane;
	var titlepane = param.titlePane;
	if (cx__capabilities.Dom1HTML)
	    titlepane.styleobj = titlepane.getElementsByTagName('table')[0];
	else
	    titlepane.styleobj = titlepane;
	if (param.mainBackground) {
		htr_setbackground(parentPane, param.mainBackground);
		htr_setbackground(coverpane, param.mainBackground);
		htr_setbackground(titlepane.styleobj, param.mainBackground);
	}
	if (param.outlineBackground) {
		htr_setbackground(borderpane, param.outlineBackground);
	}
	parentPane.coverPane = coverpane;
	parentPane.borderPane = borderpane;
	parentPane.buttonList = new Array();
	parentPane.setvalue = rb_setvalue;
	parentPane.getvalue = rb_getvalue;
	parentPane.clearvalue = rb_clearvalue;
	parentPane.resetvalue = rb_resetvalue;
	parentPane.enabled = true;
	parentPane.enable = rb_enable;
	parentPane.disable = rb_disable;
	parentPane.readonly = rb_readonly;
	parentPane.getfocushandler = rb_getfocus;
	parentPane.losefocushandler = rb_losefocus;
	parentPane.keyhandler = rb_keyhandler;
	parentPane.kbdSelected = null;
	parentPane.value = null;
	parentPane.valueIndex = null;
	htr_init_layer(parentPane, parentPane, 'radiobutton');
	if (borderpane) {
		htr_init_layer(borderpane, parentPane, 'radiobutton');
	}
	if (coverpane) {
		htr_init_layer(coverpane, parentPane, 'radiobutton');
	}
	if (titlepane) {
		htr_init_layer(titlepane, parentPane, 'radiobutton');
	}
	parentPane.rbCount = 0;
	parentPane.fieldname = param.fieldname;
	if (param.form) 
	    parentPane.form = wgtrGetNode(parentPane, param.form);
	if (!parentPane.form && parentPane.fieldname)
	    parentPane.form = wgtrFindContainer(parentPane,"widget/form");
	if (parentPane.form)
	    parentPane.form.Register(parentPane);

	// Events
	ifc_init_widget(parentPane);
	var ie = parentPane.ifcProbeAdd(ifEvent);
	ie.Add("Click");
	ie.Add("GetFocus");
	ie.Add("LoseFocus");
	ie.Add("MouseDown");
	ie.Add("MouseUp");
	ie.Add("MouseOver");
	ie.Add("MouseOut");
	ie.Add("MouseMove");
	ie.Add("DataChange");
	if(parentPane.form){
	    parentPane.form.ifcProbe(ifEvent).Hook("StatusChange",rb_changemode,parentPane);
	}

	var iv = parentPane.ifcProbeAdd(ifValue);
	iv.Add("value", rb_cb_getvalue, rb_cb_setvalue);
	iv.Add("valueindex", 'valueIndex');

	var ia = parentPane.ifcProbeAdd(ifAction);
	ia.Add("SetValue", rb_action_setvalue);
	return parentPane;
}

function radiobutton_toggle(layer) {
	if(!layer) return;
	if(layer.mainlayer.form && layer.mainlayer.form.mode == "Query"){
	    layer.mainlayer.form.DataNotify(layer.mainlayer);
	    if(htr_getvisibility(layer.optionPane.unsetPane) == 'inherit'){
		htr_setvisibility(layer.optionPane.unsetPane, 'hidden');
		htr_setvisibility(layer.optionPane.setPane, 'inherit');
	    }
	    else {
		htr_setvisibility(layer.optionPane.unsetPane, 'inherit');
		htr_setvisibility(layer.optionPane.setPane, 'hidden');
	    }
	    cn_activate(layer.mainlayer, 'DataChange', {Value:layer.mainlayer.value});
	    return;
	}
	if (layer.mainlayer.selectedOption != layer.optionPane) {
		if(layer.mainlayer.form)
		    layer.mainlayer.form.DataNotify(layer.mainlayer);
		if (layer.mainlayer.selectedOption) {
		    htr_setvisibility(layer.mainlayer.selectedOption.unsetPane, 'inherit');
		    htr_setvisibility(layer.mainlayer.selectedOption.setPane, 'hidden');
		}
		htr_setvisibility(layer.optionPane.setPane, 'inherit');
		htr_setvisibility(layer.optionPane.unsetPane, 'hidden');
		layer.mainlayer.selectedOption = layer.optionPane;
		layer.mainlayer.value = layer.mainlayer.selectedOption.optionValue;
		layer.mainlayer.valueIndex = layer.mainlayer.selectedOption.valueIndex;
		cn_activate(layer.mainlayer, 'DataChange', {Value:layer.mainlayer.value});
	}
}

function radiobutton_mouseup(e) {
        if (e.layer != null && e.kind == 'radiobutton') {
		if (e.mainlayer.enabled) {
			cn_activate(e.mainlayer, 'Click');
			cn_activate(e.mainlayer, 'MouseUp');
		}
        }
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function radiobutton_mousedown(e) {
	if (e.layer != null && e.kind == 'radiobutton') {
		if (e.mainlayer.enabled){
			if(e.layer.optionPane) {
				if (e.mainlayer.form) e.mainlayer.form.FocusNotify(e.mainlayer);
				if (e.layer && e.layer.optionPane && e.layer.optionPane.optionValue)
					e.mainlayer.ifcProbe(ifValue).Changing('value', e.layer.optionPane.optionValue, true, e.mainlayer.value, true);
				radiobutton_toggle(e.layer);
			}
			cn_activate(e.mainlayer, 'MouseDown');
		}
	}
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function radiobutton_mouseover(e) {
	if (e.layer != null && e.kind == 'radiobutton') {
		if (e.mainlayer.enabled && !util_cur_mainlayer) {
			cn_activate(e.mainlayer, 'MouseOver');
			util_cur_mainlayer = e.mainlayer;
		}
	}
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function radiobutton_mousemove(e) {
	if (util_cur_mainlayer && e.kind != 'radiobutton') {
		if (util_cur_mainlayer.mainlayer.enabled) cn_activate(util_cur_mainlayer.mainlayer, 'MouseOut');  // This is MouseOut Detection!
			util_cur_mainlayer = null;
	}
	if (e.layer != null && e.kind == 'radiobutton') {
		if (e.mainlayer.enabled) cn_activate(e.mainlayer, 'MouseMove');
	}
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}
