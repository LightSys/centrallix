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

function fs_setvalue(m) {
    this.currentMode = m;
    if (this.imagestyle == "large") {
	if (this.currentMode == 'View') {;
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL01.png');
	} else if (this.currentMode == 'Modify') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL02.png');
	} else if (this.currentMode == 'New') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL03.png');
	} else if (this.currentMode == 'Query' || this.currentMode == 'QueryExec') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL04.png');
	} else {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL05.png');
	}
    } else if (this.imagestyle == "largeflat") {
	if (this.currentMode == 'View') {;
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF01.png');
	} else if (this.currentMode == 'Modify') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF02.png');
	} else if (this.currentMode == 'New') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF03.png');
	} else if (this.currentMode == 'Query') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF04.png');
	} else if (this.currentMode == 'QueryExec') {
		pg_set(pg_images(this)[0],'src','/sys/images/searchingLF.gif');
	} else {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF05.png');
	}
    } else {
	if (this.currentMode == 'View') {;
		pg_set(pg_images(this)[0],'src','/sys/images/formstat01.gif');
	} else if (this.currentMode == 'Modify') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat02.gif');
	} else if (this.currentMode == 'New') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat03.gif');
	} else if (this.currentMode == 'Query' || this.currentMode == 'QueryExec') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat04.gif');
	} else {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat05.gif');
	}
    }
}

// Reveal callback passes the info on to the form widget.
function fs_cb_reveal(e) {
    //alert(this.name + ' got ' + e.eventName);
    switch (e.eventName) {
	case 'Reveal':
	case 'Obscure':
	    if (this.form) this.form.Reveal(this,e);
	    break;
	case 'RevealCheck':
	case 'ObscureCheck':
	    if (this.form) this.form.Reveal(this,e);
	    else pg_reveal_check_ok(e);
	    break;
    }
    return true;
}

// Event handlers
function fs_mousedown(e) {
    if (e.kind == 'formstatus') cn_activate(e.layer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}
function fs_mouseup(e) {
    if (e.kind == 'formstatus') cn_activate(e.layer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}
function fs_mouseover(e) {
    if (e.kind == 'formstatus') cn_activate(e.layer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}
function fs_mouseout(e) {
    if (e.kind == 'formstatus') cn_activate(e.layer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}
function fs_mousemove(e) {
    if (e.kind == 'formstatus') cn_activate(e.layer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

// Init routine
function fs_init(param) {
    var l = param.layer;
    var images = pg_images(l);
    htr_init_layer(l,l,"formstatus");
    ifc_init_widget(l);
    images[0].layer = l;
    images[0].mainlayer = l;
    images[0].kind = 'formstatus';
    l.currentMode = 'NoData';
    l.isFormStatusWidget = true;
    l.setvalue = fs_setvalue;
    l.imagestyle = param.style;
    l.form = wgtrFindContainer(l,"widget/form");
    if (l.form) {
	l.form.Register(l);
    }

    // Request reveal/obscure notifications
    l.Reveal = fs_cb_reveal;
    if (pg_reveal_register_listener(l)) {
	// already visible
	l.form.Reveal(l,{ eventName:'Reveal' });
    }

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    return l;
}
