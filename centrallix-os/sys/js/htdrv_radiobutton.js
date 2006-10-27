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
	if (this.selectedOption)/* return "" if nothing is selected */
		return this.selectedOption.layers.radiobuttonpanelvaluepane.document.anchors[0].name;
	 else
		return "";
}

// Luke (03/04/02) -
// The behavior of this is in direct combination with the clearvalue function.
// if set_value() gets called with a value that _is_ in the list of values, then
// obviously that button gets marked.  However, if the called parameter is not
// in the list, an alert message pops up.  SEE ALSO:  Note on clearvalue()
function rb_setvalue(v) {
	optsLayerArray = this.layers[0].document.layers[0].document.layers;
	for (var i=0; i < optsLayerArray.length; i++) {
		if (optsLayerArray[i].layers.radiobuttonpanelvaluepane.document.anchors[0].name == v) {
			radiobutton_toggle(optsLayerArray[i]);
			return;
		}
	}
	alert('Warning: "'+v+'" is not in the radio button list.');
}

/*  Luke (03/04/02)  This unchecks ALL radio buttons.  As such, nothing is selected. */
function rb_clearvalue() {
	if (this.selectedOption) {
		this.selectedOption.unsetPane.visibility = 'inherit';
		this.selectedOption.setPane.visibility = 'hidden';
		this.selectedOption = null;
	}
}

function rb_resetvalue() {
	if (this.selectedOption != this.defaultSelectedOption) {
		radiobutton_toggle(this.defaultSelectedOption);
	}
}

function rb_enable() {
	this.mainlayer.enabled = true;
	for (i=0;i<this.buttonList.length;i++) {
		this.buttonList[i].setPane.document.images[0].src = '/sys/images/radiobutton_set.gif';
		this.buttonList[i].unsetPane.document.images[0].src = '/sys/images/radiobutton_unset.gif';
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
		this.buttonList[i].setPane.document.images[0].src = '/sys/images/radiobutton_set_dis.gif';
		this.buttonList[i].unsetPane.document.images[0].src = '/sys/images/radiobutton_unset_dis.gif';
	}
}

function add_radiobutton(optionPane, selected) {
	optionPane.kind = 'radiobutton';
	optionPane.document.layer = optionPane;
	optionPane.mainlayer = wgtrGetParent(optionPane);
	optionPane.optionPane = optionPane;
	optionPane.setPane = optionPane.layers.radiobuttonpanelbuttonsetpane;
	optionPane.unsetPane = optionPane.layers.radiobuttonpanelbuttonunsetpane;
	optionPane.layers.radiobuttonpanelbuttonsetpane.kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonsetpane.mainlayer = optionPane.mainlayer;
	optionPane.layers.radiobuttonpanelbuttonsetpane.optionPane = optionPane;
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.layer = optionPane.layers.radiobuttonpanelbuttonsetpane;
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].mainlayer = optionPane.mainlayer;
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].layer = optionPane.layers.radiobuttonpanelbuttonsetpane;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonunsetpane.mainlayer = optionPane.mainlayer;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.optionPane = optionPane;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.layer = optionPane.layers.radiobuttonpanelbuttonunsetpane;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].mainlayer = optionPane.mainlayer;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].layer = optionPane.layers.radiobuttonpanelbuttonunsetpane;
	optionPane.layers.radiobuttonpanellabelpane.kind = 'radiobutton';
	optionPane.layers.radiobuttonpanellabelpane.optionPane = optionPane;
	optionPane.layers.radiobuttonpanellabelpane.mainlayer = optionPane.mainlayer;
	optionPane.layers.radiobuttonpanellabelpane.document.layer = optionPane.layers.radiobuttonpanellabelpane;
	optionPane.mainlayer.buttonList.push(optionPane);
	if (selected) {
		optionPane.layers.radiobuttonpanelbuttonsetpane.visibility = 'inherit';
		optionPane.layers.radiobuttonpanelbuttonunsetpane.visibility = 'hidden';
		optionPane.mainlayer.selectedOption = optionPane;
		optionPane.mainlayer.defaultSelectedOption = optionPane;
	} else {
		optionPane.layers.radiobuttonpanelbuttonsetpane.visibility = 'hidden';
		optionPane.layers.radiobuttonpanelbuttonunsetpane.visibility = 'inherit';
	}
}

function radiobuttonpanel_init(param) {
	var parentPane = param.parentPane;
	var borderpane = param.borderPane;
	var coverpane = param.coverPane;
	var titlepane = param.titlePane;
	if(param.flag==1) {
		htr_setbgcolor(parentPane, param.mainBackground);
		htr_setbgcolor(borderpane, param.outlineBackground);
		htr_setbgcolor(coverpane, param.mainBackground);
		htr_setbgcolor(titlepane, param.mainBackground);
	}
	if(param.flag==2) {
		htr_setbgimage(parentPane, param.mainBackground);
		htr_setbgimage(borderpane, param.outlineBackground);
		htr_setbgimage(coverpane, param.mainBackground);
		htr_setbgimage(titlepane, param.mainBackground);
	}
	parentPane.buttonList = new Array();
	parentPane.setvalue = rb_setvalue;
	parentPane.getvalue = rb_getvalue;
	parentPane.clearvalue = rb_clearvalue;
	parentPane.resetvalue = rb_resetvalue;
	parentPane.enabled = true;
	parentPane.enable = rb_enable;
	parentPane.disable = rb_disable;
	parentPane.readonly = rb_readonly;
	parentPane.fieldname = param.fieldname;
	parentPane.form = wgtrFindContainer(parentPane,"widget/form");
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
	if (parentPane.form) parentPane.form.Register(parentPane);

	// Events
	ifc_init_widget(parentPane);
	var ie = parentPane.ifcProbeAdd(ifEvent);
	ie.Add("Click");
	ie.Add("MouseDown");
	ie.Add("MouseUp");
	ie.Add("MouseOver");
	ie.Add("MouseOut");
	ie.Add("MouseMove");
	ie.Add("DataChange");
	
	return parentPane;
}

function radiobutton_toggle(layer) {
	if(!layer) return;
	if(layer.mainlayer.name);
	if (layer.mainlayer.selectedOption != layer.optionPane) {
		if(layer.mainlayer.form)
			layer.mainlayer.form.DataNotify(layer.mainlayer);
		if (layer.mainlayer.selectedOption) {
			layer.mainlayer.selectedOption.unsetPane.visibility = 'inherit';
			layer.mainlayer.selectedOption.setPane.visibility = 'hidden';
		}
		layer.optionPane.setPane.visibility = 'inherit';
		layer.optionPane.unsetPane.visibility = 'hidden';
		layer.mainlayer.selectedOption = layer.optionPane;
		cn_activate(layer.mainlayer, 'DataChange');
	}
}

function radiobutton_mouseup(e) {
        if (e.layer != null && e.kind == 'radiobutton') {
		if (e.mainlayer.enabled) {
			if(e.layer.optionPane) {
				if (e.mainlayer.form) e.mainlayer.form.FocusNotify(e.mainlayer);
					radiobutton_toggle(e.layer);
			}
			cn_activate(e.mainlayer, 'Click');
			cn_activate(e.mainlayer, 'MouseUp');
		}
        }
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function radiobutton_mousedown(e) {
	if (e.layer != null && e.kind == 'radiobutton') {
		if (e.mainlayer.enabled) 
			cn_activate(e.mainlayer, 'MouseDown');
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
