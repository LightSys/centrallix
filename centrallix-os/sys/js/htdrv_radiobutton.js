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

function add_radiobutton(optionPane, parentPane, selected, ml) {
	optionPane.kind = 'radiobutton';
	optionPane.document.layer = optionPane;
	optionPane.mainlayer = ml;
	optionPane.optionPane = optionPane;
	optionPane.setPane = optionPane.layers.radiobuttonpanelbuttonsetpane;
	optionPane.unsetPane = optionPane.layers.radiobuttonpanelbuttonunsetpane;
	optionPane.layers.radiobuttonpanelbuttonsetpane.kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonsetpane.mainlayer = ml;
	optionPane.layers.radiobuttonpanelbuttonsetpane.optionPane = optionPane;
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.layer = optionPane.layers.radiobuttonpanelbuttonsetpane;
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].mainlayer = ml;
	optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].layer = optionPane.layers.radiobuttonpanelbuttonsetpane;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonunsetpane.mainlayer = ml;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.optionPane = optionPane;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.layer = optionPane.layers.radiobuttonpanelbuttonunsetpane;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].kind = 'radiobutton';
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].mainlayer = ml;
	optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].layer = optionPane.layers.radiobuttonpanelbuttonunsetpane;
	optionPane.layers.radiobuttonpanellabelpane.kind = 'radiobutton';
	optionPane.layers.radiobuttonpanellabelpane.optionPane = optionPane;
	optionPane.layers.radiobuttonpanellabelpane.mainlayer = ml;
	optionPane.layers.radiobuttonpanellabelpane.document.layer = optionPane.layers.radiobuttonpanellabelpane;
	parentPane.buttonList.push(optionPane);
	if (selected) {
		optionPane.layers.radiobuttonpanelbuttonsetpane.visibility = 'inherit';
		optionPane.layers.radiobuttonpanelbuttonunsetpane.visibility = 'hidden';
		parentPane.selectedOption = optionPane;
		parentPane.defaultSelectedOption = optionPane;
	} else {
		optionPane.layers.radiobuttonpanelbuttonsetpane.visibility = 'hidden';
		optionPane.layers.radiobuttonpanelbuttonunsetpane.visibility = 'inherit';
	}
}

function radiobuttonpanel_init(parentPane,fieldname,flag,borderpane,coverpane,titlepane,main_bg,outline_bg) {
	if(flag==1) {
		parentPane.bgColor=main_bg;
		borderpane.bgColor=outline_bg;
		coverpane.bgColor=main_bg;
		titlepane.bgColor=main_bg;
	}
	if(flag==2) {
		parentPane.background.src=main_bg;
		borderpane.background.src=outline_bg;
		coverpane.background.src=main_bg;
		titlepane.background.src=main_bg;
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
	parentPane.fieldname = fieldname;
	parentPane.form = fm_current;
	parentPane.document.layer = parentPane;
	parentPane.mainlayer = parentPane;
	parentPane.kind = 'radiobutton';
	borderpane.document.layer = borderpane;
	borderpane.mainlayer = parentPane;
	borderpane.kind = 'radiobutton';
	coverpane.document.layer = coverpane;
	coverpane.mainlayer = parentPane;
	coverpane.kind = 'radiobutton';
	titlepane.document.layer = titlepane;
	titlepane.mainlayer = parentPane;
	titlepane.kind = 'radiobutton';
	if (fm_current) fm_current.Register(parentPane);
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
