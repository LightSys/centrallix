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

// Form manipulation

function dd_getvalue() {
	return this.labelLayer.value;
}

function dd_setvalue(v) {
	for (i=0; i < this.values.length; i++) {
		if (this.values[i] == v) {
			dd_write_item(this.labelLayer, this.labels[i], this.values[i], this);
			return true;
		}
	}
	return false;
}

function dd_clearvalue() {
	dd_write_item(this.labelLayer, '', '');
}

function dd_resetvalue() {
	this.clearvalue();
}

function dd_enable() {
	this.topLayer.document.images[8].src = '/sys/images/ico15b.gif';
	this.enabled = 'full';
}

function dd_readonly() {
	this.enabled = 'readonly';
}

function dd_disable() {
	this.topLayer.document.images[8].src = '/sys/images/ico15a.gif';
	this.enabled = 'disabled';
}

// Normal functions

function dd_keyhandler(l,e,k) {
	if (!dd_current) return;
	if (dd_current.enabled != 'full') return 1;
	if ((k >= 65 && k <= 90) || (k >= 97 && k <= 122)) {
		if (k < 97) {
			k_lower = k + 32;
			k_upper = k;
			k = k + 32;
	} else {
		k_lower = k;
		k_upper = k - 32;
	}
	if (!dd_lastkey || dd_lastkey != k) {
		for (i=0; i < this.labels.length; i++) {
			if (this.labels[i].substring(0, 1) == String.fromCharCode(k_upper) ||
			    this.labels[i].substring(0, 1) == String.fromCharCode(k_lower)) {
				dd_hilight_item(this.itemLayers[i]);
				i=this.labels.length;
				}
			}
		} else {
			var first = -1;
			var last = -1;
			var next = -1;
			for (i=0; i < this.labels.length; i++) {
				if (this.labels[i].substring(0, 1) == String.fromCharCode(k_upper) ||
				    this.labels[i].substring(0, 1) == String.fromCharCode(k_lower)) {
					if (first < 0) { first = i; last = i; }
					for (var j=i; j < this.labels.length && 
					    (this.itemLayers[j].label.substring(0, 1) == String.fromCharCode(k_upper) ||
					     this.itemLayers[j].label.substring(0, 1) == String.fromCharCode(k_lower)); j++) {
						if (this.itemLayers[j] == this.selectedItem)
							next = j + 1;
						last = j;
					}
					if (next <= last)
						dd_hilight_item(this.itemLayers[next]);
					else
						dd_hilight_item(this.itemLayers[first]);
					i=this.labels.length;
				}
			}
		}
	} else if (k == 13 && dd_lastkey != 13) {
		dd_select_item(this.selectedItem);
		dd_unhilight_item(this.selectedItem);
	}
	dd_lastkey = k;
	return false;
}

function dd_hilight_item(l) {
	if (l.topLayer.selectedItem)
		dd_unhilight_item(l.topLayer.selectedItem);
	l.topLayer.selectedItem = l;
	l.bgColor = dd_current.colorHilight;
	range = l.topLayer.scrollPanelLayer.viewRangeBottom - l.topLayer.scrollPanelLayer.viewRangeTop;
	if (l.topLayer.scrollPanelLayer.viewRangeBottom < l.y+16) {
		l.topLayer.scrollPanelLayer.viewRangeTop = l.y - range + 16;
		l.topLayer.scrollPanelLayer.viewRangeBottom = l.topLayer.scrollPanelLayer.viewRangeTop + range;
	} else if (l.topLayer.scrollPanelLayer.viewRangeTop > l.y) {
		l.topLayer.scrollPanelLayer.viewRangeTop = l.y;
		l.topLayer.scrollPanelLayer.viewRangeBottom = l.y + range;
	}
	l.topLayer.scrollPanelLayer.y = -l.topLayer.scrollPanelLayer.viewRangeTop;
}

function dd_unhilight_item(l) {
	l.bgColor = l.topLayer.colorBack;
	l.topLayer.selectedItem = null;
}

function dd_select_item(l) {
	dd_current.document.images[8].src = '/sys/images/ico15b.gif';
	dd_current.ddLayer.visibility = 'hide';
	if (l.subkind == 'dropdown_item' && dd_current.enabled == 'full') {
		dd_write_item(dd_current.labelLayer, l.label, l.value, dd_current);
		if(dd_current.form)
			dd_current.form.DataNotify(dd_current);
	}
	dd_current = null;
	dd_lastkey = null;
}

function dd_losefocus() {
	 return true;
}

function dd_write_item(itemLayer, label, value) {
	itemLayer.document.write('<table cellpadding=2 cellspacing=0 height=16 border=0><tr><td valign=middle>'+label+'</td></tr></table>');
	itemLayer.document.close();
	itemLayer.label = label;
	itemLayer.value = value;
	itemLayer.kind = 'dropdown';
}

function dd_additem(l, label, value) {
	l.labels.push(label);
	l.values.push(value);
	var tmpLayer = new Layer(1024, l.scrollPanelLayer);
	tmpLayer.kind = 'dropdown';
	tmpLayer.subkind = 'dropdown_item';
	tmpLayer.topLayer = l;
	tmpLayer.document.layer = tmpLayer;
	tmpLayer.bgColor = l.colorBack;
	tmpLayer.label = label;
	tmpLayer.value = value;
	tmpLayer.x = 0;
	tmpLayer.y = ((l.numItems) * 16);
	tmpLayer.clip.width = l.clip.width - 20;
	tmpLayer.clip.height = 16;
	tmpLayer.visibility = 'inherit';
	l.itemLayers.push(tmpLayer);
	dd_write_item(tmpLayer, label, value);
	l.numItems++;
	l.scrollPanelLayer.clip.height = l.numItems * 16;
	if (l.numItems > l.numDispElements && !l.dispScrollLayer) {
		l.dispScrollLayer = true;
		l.ddLayer.clip.width = l.clip.width;
		l.scrollLayer = new Layer(1024, l.ddLayer);
		l.scrollLayer.bgColor = l.colorBack;
		l.scrollLayer.visibility = 'inherit';
		l.scrollLayer.kind = 'dropdown';
		l.scrollLayer.subkind = 'dropdown_scroll';
		l.scrollLayer.x = l.ddLayer.clip.width - 18;
		l.scrollLayer.document.write('<table height='+(l.numDispElements*16)+' border=0 cellspacing=0 cellpadding=0 width=18>');
		l.scrollLayer.document.write('<tr><td align=right><img src=/sys/images/ico13b.gif name=up></td></tr><tr><td align=right>');
		l.scrollLayer.document.write('<img src=/sys/images/trans_1.gif height='+((l.numDispElements*16)-34)+' width=18 name=thumb>');
		l.scrollLayer.document.write('</td></tr><tr><td align=right><img src=/sys/images/ico12b.gif name=down></td></tr></table>');
		l.scrollLayer.document.close();
		l.scrollLayer.document.images[0].topLayer = l;
		l.scrollLayer.document.images[1].topLayer = l;
		l.scrollLayer.document.images[2].topLayer = l;
		l.scrollLayer.document.images[0].kind = 'dropdown';
		l.scrollLayer.document.images[1].kind = 'dropdown';
		l.scrollLayer.document.images[2].kind = 'dropdown';
		l.scrollLayer.document.images[0].subkind = 'dropdown_scroll';
		l.scrollLayer.document.images[1].subkind = 'dropdown_scroll';
		l.scrollLayer.document.images[2].subkind = 'dropdown_scroll';
	} else if (!l.dispScrollLayer) {
		l.scrollPanelLayer.viewRangeBottom = (l.numItems * 16);
		l.ddLayer.clip.height = ((l.numItems) * 16) + 2;
		l.bg1Layer.clip.height = l.ddLayer.clip.height - 1;
		l.bg2Layer.clip.height = l.bg1Layer.clip.height - 1;
	}
}

function dd_init(l, clr_b, clr_h, fn, disp) {
	l.numItems = 0;
	l.numDispElements = disp;
	if (l.numDispElements < 4) l.numDispElements = 4;
	l.fieldname = fn;
	l.colorBack = clr_b;
	l.colorHilight = clr_h;
	l.enabled = 'full';
	l.form = fm_current;
	l.topLayer = l;
	l.dispScrollLayer = false;
	l.document.layer = l;
	l.kind = 'dropdown';
	l.subkind = 'dropdown_top';
	l.itemLayers = new Array();
	l.labels = new Array();
	l.values = new Array();
	for (var i=0; i < l.document.images.length; i++) {
		l.document.images[i].kind = 'dropdown';
		l.document.images[i].topLayer = l;
		l.document.images[i].layer = l;
	}

	l.ddLayer = new Layer(1024);
	l.ddLayer.layer = l.ddLayer;
	l.ddLayer.kind = 'dropdown';
	l.ddLayer.subkind = 'dropdown_bottom';
	l.ddLayer.topLayer = l;
	l.ddLayer.document.layer = l.ddLayer;
	l.ddLayer.clip.width = l.clip.width - 18;
	l.ddLayer.bgColor = '#ffffff';

	l.labelLayer = new Layer(1024, l);
	l.labelLayer.document.layer = l;
	l.labelLayer.kind = 'dropdown';
	l.labelLayer.topLayer = l;
	l.labelLayer.mainlayer = l;
	l.labelLayer.clip.width = l.clip.width - 20;
	l.labelLayer.clip.height = 16;
	l.labelLayer.x = 1;
	l.labelLayer.y = 1;
	l.labelLayer.visibility = 'inherit';

	l.bg1Layer = new Layer(1024, l.ddLayer);
	l.bg1Layer.layer = l.bg1Layer;
	l.bg1Layer.kind = 'dropdown';
	l.bg1Layer.bgColor = '#888888';
	l.bg1Layer.visibility = 'inherit';
	l.bg1Layer.top = 1;
	l.bg1Layer.x = 1;
	l.bg1Layer.clip.width = l.ddLayer.clip.width;

	l.bg2Layer = new Layer(1024, l.bg1Layer);
	l.bg2Layer.visibility = 'inherit';
	l.bg2Layer.bgColor = '#cc0000';
	l.bg2Layer.clip.left = 0;
	l.bg2Layer.clip.top = 0;
	l.bg2Layer.clip.width = l.bg1Layer.clip.width - 2;

	l.scrollPanelLayer = new Layer(1024, l.bg2Layer);
	l.scrollPanelLayer.layer = l.scrollPanelLayer;
	l.scrollPanelLayer.topLayer = l;
	l.scrollPanelLayer.bgColor = '#cc0000';
	l.scrollPanelLayer.kind = 'dropdown';
	l.scrollPanelLayer.visibility = 'inherit';
	l.scrollPanelLayer.clip.left = 0;
	l.scrollPanelLayer.clip.top = 0;
	l.scrollPanelLayer.clip.width = l.bg2Layer.clip.width;
	l.scrollPanelLayer.viewRangeTop = 0;
	l.scrollPanelLayer.viewRangeBottom = 0;

	l.setvalue = dd_setvalue;
	l.getvalue = dd_getvalue;
	l.enable = dd_enable;
	l.readonly = dd_readonly;
	l.disable = dd_disable;
	l.clearvalue = dd_clearvalue;
	l.resetvalue = dd_resetvalue;
	l.keyhandler = dd_keyhandler;
	l.losefocushandler = dd_losefocus;
	pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'dropdown', 'dropdown', 1);
	if (fm_current) fm_current.Register(l);
}
