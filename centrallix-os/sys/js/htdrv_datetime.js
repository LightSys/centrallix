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

function dt_getvalue() {
	if (this.DateObj)
	    return dt_formatdate(this, this.DateObj, 0);
	else
	    return null;
}

function dt_setvalue(v) {
	if (v) {
		this.DateObj = new Date(v);
		this.TmpDateObj = new Date(v);
		if (this.DateObj == 'Invalid Date') {
			this.DateObj = new Date();
			this.TmpDateObj = new Date();
		}
	} else {
		this.DateObj = null;
		this.TmpDateObj = null;
	}
	dt_drawdate(this, this.DateObj);
	if (this.PaneLayer) {
		dt_drawmonth(this.PaneLayer, this.DateObj);
		dt_drawtime(this.PaneLayer, this.DateObj);
	}
}

function dt_clearvalue() {
	this.DateObj = null;
	this.TmpDateObj = new Date();
	dt_drawdate(this, this.DateObj);
}

function dt_resetvalue() {
}

function dt_enable() {
	this.enabled = 'full';
	//this.bgColor = this.bg;
	if (this.bg) htr_setbgcolor(this, this.bg);
	if (this.bgi) htr_setbgimage(this, this.bgi);
}

function dt_readonly() {
	this.enabled = 'readonly';
	if (this.bg) htr_setbgcolor(this, this.bg);
	if (this.bgi) htr_setbgimage(this, this.bgi);
}

function dt_disable() {
	this.enabled = 'disabled';
	//this.bgColor = '#e0e0e0';
	if (this.bg) htr_setbgcolor(this, '#e0e0e0');
}

// Date/Time Functions
function dt_init(param){
	var l = param.layer;
	var c1 = param.c1;
	var c2 = param.c2;
	var bg = param.background;
	var w = param.width;
	var w2 = param.width2;
	var h = param.height;
	var h2 = param.height2;
	l.enabled = 'full';
	//l.mainlayer = l;
	//c1.mainlayer = l;
	//c2.mainlayer = l;
	l.setvalue   = dt_setvalue;
	l.getvalue   = dt_getvalue;
	l.enable     = dt_enable;
	l.readonly   = dt_readonly;
	l.disable    = dt_disable;
	l.clearvalue = dt_clearvalue;
	l.resetvalue = dt_resetvalue;
	l.fieldname  = param.fieldname;
	l.getfocushandler  = dt_getfocus;
	l.losefocushandler = dt_losefocus;
	l.keyhandler = dt_keyhandler;
	//l.kind  = c1.kind = c2.kind = 'dt';
	//l.document.layer  = c1.document.layer = c2.document.layer = l;
	htr_init_layer(l,l,'dt');
	htr_init_layer(c1,l,'dt');
	htr_init_layer(c2,l,'dt');
	//dt_tag_images(l.document, 'dt', l);
	htutil_tag_images(l,'dt',l,l);
	l.w = w; l.h = h;
	l.bg = htr_extract_bgcolor(bg);
	l.ubg = bg;
	l.fg = param.foreground;
	l.w2 = w2;
	l.h2 = h2;
	l.form = fm_current;
	l.DateStr = param.id;
	l.MonthsAbbrev = Array('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');
	l.VisLayer = c1;
	l.HidLayer = c2;
	if (param.id) {
		l.DateObj = new Date(param.id);
		l.TmpDateObj = new Date(param.id);
		dt_drawdate(l, l.DateObj);
	} else {
		l.DateObj = new Date();
		l.TmpDateObj = new Date();
		dt_drawdate(l, '');
	}
	if (fm_current) fm_current.Register(l);
	pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'dt', 'dt', 3);

	// Events
	ifc_init_widget(l);
	var ie = l.ifcProbeAdd(ifEvent);
	ie.Add('DataChange');
	ie.Add('GetFocus');
	ie.Add('LoseFocus');
	ie.Add('Click');
	ie.Add('MouseDown');
	ie.Add('MouseUp');
	ie.Add('MouseOver');
	ie.Add('MouseOut');
	ie.Add('MouseMove');

	return l;
}

// Called before the popup or month pane is used.  We do this to defer the
// complex pane drawing operation until we actually need it (thus faster page
// load times).
function dt_prepare(l) {
	// Create the pane if needed.
	if (!l.PaneLayer) {
		l.PaneLayer = dt_create_pane(l,l.ubg,l.w2,l.h2,l.h);
		l.PaneLayer.ml = l;
		l.PaneLayer.HidLayer.Areas = l.PaneLayer.VisLayer.Areas = new Array();
		l.PaneLayer.VisLayer.getfocushandler = dt_getfocus_day;
		l.PaneLayer.HidLayer.getfocushandler = dt_getfocus_day;
		l.PaneLayer.VisLayer.losefocushandler = dt_losefocus_day;
		l.PaneLayer.HidLayer.losefocushandler = dt_losefocus_day;
	}

	// redraw the month & time.
	l.TmpDateObj = (l.DateObj?(new Date(l.DateObj)):null);
	dt_drawmonth(l.PaneLayer, l.TmpDateObj);
	dt_drawtime(l.PaneLayer, l.TmpDateObj);
	if (!l.TmpDateObj) l.TmpDateObj = new Date();
}

function dt_formatdate(l, d, fmt) {
	var str;
	switch (fmt) {
		case 0:
		default:
			str  = l.MonthsAbbrev[d.getMonth()] + ' ';
			str += d.getDate() + ', ';
			str += d.getYear()+1900 + ', ';
			str += htutil_strpad(d.getHours(), '0', 2)+':';
			str += htutil_strpad(d.getMinutes(), '0', 2);
			break;
	}
	return str;
}

function dt_writedate(l, txt) {
	var v = '<TABLE border=0 cellspacing=0 cellpadding=0 height=100\% width='+l.w+'>';
	v += '<TR><TD align=center valign=middle nowrap><FONT color=\"'+l.fg+'\">';
	v += txt;
	v += '</FONT></TD></TR></TABLE>';
	htr_write_content(l.HidLayer, v);
	htr_setvisibility(l.HidLayer, 'inherit');
	htr_setvisibility(l.VisLayer, 'hidden');
	var t = l.VisLayer;
	l.VisLayer = l.HidLayer;
	l.HidLayer = t;
}

function dt_drawdate(l, d) {
	if (d && d != 'Invalid Date')
		dt_writedate(l, dt_formatdate(l, d, 0));
	else
		dt_writedate(l, '');
}

function dt_drawmonth(l, d) {
	var dy=0,r=-1;
	var TmpDate;
	if (d) 
	    TmpDate = new Date(d);
	else 
	    TmpDate = new Date();
	TmpDate.setDate(1);
	var col=TmpDate.getDay();
	var num=htutil_days_in_month(TmpDate);
	while (l.VisLayer.Areas.length) {
		pg_removearea(l.VisLayer.Areas[0]);
		l.VisLayer.Areas.shift();
	}
	pg_removearea(l.VisLayer.NullArea);

	var cur_dy = null;
	if (l.ml.DateObj && TmpDate.getMonth() == l.ml.DateObj.getMonth() && TmpDate.getYear() == l.ml.DateObj.getYear())
	    cur_dy = l.ml.DateObj.getDate();

	var v='<TABLE width=175 height=100 border=0 cellpadding=0 cellspacing=0>';
	for (var i=0; i<35; i++) {
		if (i!=0 && i%7==0) v+='</TR>';
		if (i%7==0) {v+='<TR>\n';r++}
		v+='<TD width=25 height=18 valign=middle align=right>';
		if (i>=col && dy<num) {
			l.HidLayer.Areas[dy]=pg_addarea(l.HidLayer, 1+((i%7)*25), (1+(r*18)), 25, 18, 'dt', 'dt_day', 3);
			var q=new Date(TmpDate);
			q.setDate(dy+1);
			l.HidLayer.Areas[dy].DateVal = q;
			if (cur_dy == dy+1) v += '<b>';
			v += ''+(++dy);
			if (cur_dy == dy) v += '</b>';
			col++;
		}
		v+='</TD>';
	}
	for (;i <= 31; i++) {  }

	// write items for No Date (null) and Today.
	v+='</tr><tr><td colspan=4 align=center height=18 valign=middle><i>(<u>N</u>o Date)</i></td><td colspan=3 align=center height=18 valign=middle><i>(<u>T</u>oday)</i></td>';
	l.HidLayer.NullArea = pg_addarea(l.HidLayer, 1, 1+(r+1)*18, 100, 16, 'dt', 'dt_null', 3);
	l.HidLayer.NullArea.DateVal = null;
	l.HidLayer.TodayArea = pg_addarea(l.HidLayer, 101, 1+(r+1)*18, 83, 16, 'dt', 'dt_today', 3);
	l.HidLayer.TodayArea.DateVal = new Date();
	v+='</TR></TABLE>';

	// Init the month data layer.
	//l.HidLayer.document.write(v);
	//l.HidLayer.document.close();
	htr_write_content(l.HidLayer,v);
    	//l.HidLayer.visibility = 'inherit';
	htr_setvisibility(l.HidLayer,'inherit');
	//l.VisLayer.visibility = 'hide';
	htr_setvisibility(l.VisLayer,'hidden');
	var t = l.VisLayer;
	l.VisLayer = l.HidLayer;
	l.HidLayer = t;

	//l.MonHidLayer.document.write('<TABLE border=0 cellspacing=0 cellpadding=0 height=22 width=112><TR><TD align=center valign=middle>'+l.ml.MonthsAbbrev[TmpDate.getMonth()]+', '+(TmpDate.getYear()+1900)+'</TD></TR></TABLE>');
	//l.MonHidLayer.document.close();
	//l.MonHidLayer.visibility = 'inherit';
	//l.MonVisLayer.visibility = 'hide';
	var x = '<TABLE border=0 cellspacing=0 cellpadding=0 height=22 width=112><TR><TD align=center valign=middle>'+l.ml.MonthsAbbrev[TmpDate.getMonth()]+', '+(TmpDate.getYear()+1900)+'</TD></TR></TABLE>';
	htr_write_content(l.MonHidLayer,x);
	htr_setvisibility(l.MonHidLayer,'inherit');
	htr_setvisibility(l.MonVisLayer,'hidden');
	t = l.MonVisLayer;
	l.MonVisLayer = l.MonHidLayer;
	l.MonHidLayer = t;
}

function dt_inittime(l) {
	var v = "";

	// build the structure for the time
	v = "<table border=0 cellspacing=0 cellpadding=0><tr><td><img src=/sys/images/dkgrey_1x1.png width=183 height=1></td></tr><tr><td><img src=/sys/images/white_1x1.png width=183 height=1></td></tr><tr><td height=5></td></tr></table>";
	v += "<table border=0 cellspacing=0 cellpadding=0><tr><td valign=middle><img src=/sys/images/trans_1.gif height=1 width=38></td><td><img src=/sys/images/spnr_both.gif width=10 height=20></td><td valign=middle><img src=/sys/images/trans_1.gif height=1 width=30></td><td><img src=/sys/images/spnr_both.gif width=10 height=20></td><td valign=middle><img src=/sys/images/trans_1.gif height=1 width=30></td><td><img src=/sys/images/spnr_both.gif width=10 height=20></td><td valign=middle align=center>&nbsp;&nbsp;(24h)</td></tr></table>\n";
	htr_write_content(l.TimeHidLayer, v);
	htr_setvisibility(l.TimeHidLayer, 'inherit');
	htr_setvisibility(l.TimeVisLayer, 'hidden');
	var imgs = pg_images(l.TimeHidLayer);
	l.TimeHidLayer.hr_img = imgs[3];
	imgs[3].kind = 'dtimg_edhr';
	imgs[3].layer = l;
	l.TimeHidLayer.mn_img = imgs[5];
	imgs[5].kind = 'dtimg_edmn';
	imgs[5].layer = l;
	l.TimeHidLayer.sc_img = imgs[7];
	imgs[7].kind = 'dtimg_edsc';
	imgs[7].layer = l;
	var t = l.TimeHidLayer;
	l.TimeHidLayer = l.TimeVisLayer;
	l.TimeVisLayer = t;
}

function dt_drawtime(l, d) {
	// need to create time layers?
	var tvl = l.TimeVisLayer;
	if (!tvl.hours) {
		tvl.hours = htr_new_layer(60,tvl);
		moveTo(tvl.hours,16,10);
		htr_setvisibility(tvl.hours,'inherit');
	}
	htr_write_content(tvl.hours, d?htutil_strpad(d.getHours(),0,2):'');
	if (!tvl.minutes) {
		tvl.minutes = htr_new_layer(60,tvl);
		moveTo(tvl.minutes,52,10);
		htr_setvisibility(tvl.minutes,'inherit');
	}
	htr_write_content(tvl.minutes, ':&nbsp;' + (d?htutil_strpad(d.getMinutes(),0,2):''));
	if (!tvl.seconds) {
		tvl.seconds = htr_new_layer(60,tvl);
		moveTo(tvl.seconds,90,10);
		htr_setvisibility(tvl.seconds,'inherit');
	}
	htr_write_content(tvl.seconds, ':&nbsp;' + (d?htutil_strpad(d.getSeconds(),0,2):''));
}

/*function dt_tag_images(d, t, l) {
	for (i=0; i < d.images.length; i++) {
		d.images[i].kind = t;
		d.images[i].layer = l;
	}
}*/

function dt_toggle(l) {
	var imgs = pg_images(l);
	//for (i=0; i<l.document.images.length;i++) {
	for (i=0; i<imgs.length;i++) {
		if (i == 4)
			continue;
		//else if (l.document.images[i].src.substr(-14, 6) == 'dkgrey')
		else if (imgs[i].src.substr(-14, 6) == 'dkgrey')
			//l.document.images[i].src = '/sys/images/white_1x1.png';
			imgs[i].src = '/sys/images/white_1x1.png';
		else
			//l.document.images[i].src = '/sys/images/dkgrey_1x1.png';
			imgs[i].src = '/sys/images/dkgrey_1x1.png';
	}
}


// sets the value and indicates to the form a data change event
function dt_setdata(l,d) {
	l.setvalue(d);
	if (l.form) l.form.DataNotify(l);
	cn_activate(dt_current, 'DataChange');
}


// dt_keyhandler takes care of the keyboard input.
function dt_keyhandler(l,e,k) {
	var dt = this.mainlayer;
	if (dt.enabled != 'full') return 1;
	
	// handle control characters
	if (k == 13) {			// Return key
		if (dt_current) {
			dt_collapse(dt);
			dt_current = null;
			if (dt.typed_content) {
				var d = new Date(dt.typed_content);
				if (d.getFullYear() < (new Date()).getFullYear()-90 && dt.typed_content.indexOf(d.getFullYear()) < 0)
					d.setFullYear(d.getFullYear() + 100);
				dt_setdata(dt,d);
			} else {
				dt_setdata(dt,dt.TmpDateObj);
			}
		} else {
			if (dt.form) dt.form.RetNotify(dt);
		}
	} else if (k == 27) {		// ESC key
		if (dt_current) {
			dt_collapse(dt);
			dt_current = null;
			dt_setdata(dt,dt.DateObj);
		} else {
			if (dt.form) dt.form.EscNotify(dt);
		}
	} else if (k == 9) {		// tab key
		if (dt_current) {
			dt_collapse(dt);
			dt_current = null;
			if (dt.typed_content) {
				var d = new Date(dt.typed_content);
				if (d.getFullYear() < (new Date()).getFullYear()-90 && dt.typed_content.indexOf(d.getFullYear()) < 0)
					d.setFullYear(d.getFullYear() + 100);
				dt_setdata(dt,d);
			} else {
				dt_setdata(dt,dt.TmpDateObj);
			}
			if (dt.form) dt.form.TabNotify(dt);
		} else {
			if (dt.form) dt.form.TabNotify(dt);
		}
	} else if (k == 32) {		// spacebar
		if (!dt_current) {
			dt_expand(dt);
			dt_current = dt;
		} else if (dt.typed_content) {
			dt.typed_content += String.fromCharCode(k);
			dt_update_typed(dt);
		}
	} else if (k == 'n'.charCodeAt() || k == 'N'.charCodeAt()) {
		if (dt_current) {
			dt_collapse(dt);
			dt_current = null;
			dt_setdata(dt,null);
		}
	} else if (k == 't'.charCodeAt() || k == 'T'.charCodeAt()) {
		if (dt_current) {
			dt_collapse(dt);
			dt_current = null;
			dt_setdata(dt,new Date());
		} else {
			dt_setdata(dt,new Date());
		}
	} else if (k >= 48 && k < 58) { // 0 - 9
		if (!dt_current) {
			dt_expand(dt);
			dt_current = dt;
		}
		dt.typed_content += String.fromCharCode(k);
		dt_update_typed(dt);
	} else if (k == '/'.charCodeAt() || k == ':'.charCodeAt()) {
		if (dt_current && dt.typed_content) {
			dt.typed_content += String.fromCharCode(k);
			dt_update_typed(dt);
		}
	} else if (k == 8) {		// backspace
		if (dt_current && dt.typed_content) {
			dt.typed_content = dt.typed_content.substr(0,dt.typed_content.length-1);
			dt_update_typed(dt);
		}
	}

	return false;
}


// dt_update_typed takes the typed-in content and updates the values of
// the visible control
function dt_update_typed(l) {
	dt_writedate(l,l.typed_content);
	
	// FIXME - put incremental updates to visible control here
}


// dt_getfocus is called when the user selects or tabs to the control
function dt_getfocus(x,y,l) {
	if (this.enabled != 'full') return 0;
	cn_activate(l, 'GetFocus');
	return 1;
}


// dt_getfocus_day is called when the user selects a particular date/time
// in the popup calendar window
function dt_getfocus_day(a,b,c,d,e,f) {
	// check area
	if (e != 'dt_today' && e != 'dt_day' && e != 'dt_null') return;

	// hide the drop down part of the control
	dt_collapse(dt_current);

	// Tell form we just got focus
	if (dt_current.form) 
		dt_current.form.FocusNotify(dt_current);

	// Set the data value (after focus; form might reset our data
	// value after it transitions to 'new' mode from 'nodata'
	if (e == 'dt_today') f.DateVal = new Date();
	if (!dt_current.TmpDateObj) 
		dt_current.TmpDateObj = new Date(dt_current.DateObj);
	if (f.DateVal) {
	    dt_current.DateObj = new Date(f.DateVal);
	    if (dt_current.TmpDateObj && e != 'dt_today') {
		dt_current.DateObj.setHours(dt_current.TmpDateObj.getHours(), dt_current.TmpDateObj.getMinutes(), dt_current.TmpDateObj.getSeconds());
	    }
	} else {
	    dt_current.DateObj = null;
	}

	// Tell form the value changed
	if (dt_current.form) {
		dt_current.form.DataNotify(dt_current);
		cn_activate(dt_current, 'DataChange');
	}

	// draw the date/time on the collapsed control
	dt_drawdate(dt_current, dt_current.DateObj);

	// no longer current.
	dt_current = null;
}

function dt_losefocus() {
	cn_activate(this, 'LoseFocus');
	return true;
}

function dt_losefocus_day() {
	return true;
}

function dt_create_pane(ml,bg,w,h,h2) {
	var str;
	var imgs;
	//l = new Layer(1024);
	var l = htr_new_layer(1024,ml);
	//htr_init_layer(l,ml,'dt_pn');
	htr_setvisibility(l,'hidden');

	str = "<BODY "+bg+">";
	str += "<TABLE border=0 cellpadding=0 cellspacing=0 width="+w+" height="+h+">";
	str += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>";
	str += "	<TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(w-2)+"></TD>";
	str += "	<TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>";
	str += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height="+(h-2)+" width=1></TD>";
	str += "	<TD valign=top>";
	str += "	<TABLE height=25 cellpadding=0 cellspacing=0 border=0>";
	str += "	<TR><TD width=18><IMG SRC=/sys/images/ico16aa.gif></TD>";
	str += "		<TD width=18><IMG SRC=/sys/images/ico16ba.gif></TD>";
	str += "		<TD width="+(w-72)+"></TD>";
	str += "		<TD width=18><IMG SRC=/sys/images/ico16ca.gif></TD>";
	str += "		<TD width=18><IMG SRC=/sys/images/ico16da.gif></TD></TR>";
	str += "	</TABLE>";
	str += "	<TABLE width="+w+" cellpadding=0 cellspacing=0 border=0>";
	str += "	<TR><TD align=center><B>S</B></TD>";
	str += "		<TD align=center><B>M</B></TD>";
	str += "		<TD align=center><B>T</B></TD>";
	str += "		<TD align=center><B>W</B></TD>";
	str += "		<TD align=center><B>T</B></TD>";
	str += "		<TD align=center><B>F</B></TD>";
	str += "		<TD align=center><B>S</B></TD></TR>";
	str += "	</TABLE>";
	str += "	</TD>";
	str += "	<TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(h-2)+" width=1></TD></TR>";
	str += "<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>";
	str += "	<TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(w-2)+"></TD>";
	str += "	<TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>";
	str += "</TABLE>";
	str += "</BODY>";
	//l.document.close();
	htr_write_content(l,str);
	//htr_setbgcolor(l,bg);
	pg_stackpopup(l,ml);
	setClipHeight(l,h);
	setClipWidth(l,w);
	
	//l.HidLayer = new Layer(1024, l);
	l.HidLayer = htr_new_layer(1024,l);
	//l.VisLayer = new Layer(1024, l);
	l.VisLayer = htr_new_layer(1024,l);
	//l.MonHidLayer = new Layer(1024, l);
	l.MonHidLayer = htr_new_layer(1024,l);
	//l.MonVisLayer = new Layer(1024, l);
	l.MonVisLayer = htr_new_layer(1024,l);
	l.TimeHidLayer = htr_new_layer(1024, l);
	l.TimeVisLayer = htr_new_layer(1024, l);
	moveTo(l.TimeHidLayer, 0, 156);
	moveTo(l.TimeVisLayer, 0, 156);
	l.HidLayer.y = l.VisLayer.y = 48;
	l.MonHidLayer.x = l.MonVisLayer.x = 38;
	l.MonHidLayer.y = l.MonVisLayer.y = 2;
	l.x = ml.pageX;
	l.y = ml.pageY+h2;
	l.ml = ml;
	
	l.kind = l.HidLayer.kind = l.VisLayer.kind = l.MonHidLayer.kind = l.MonVisLayer.kind = 'dt_pn';
	l.document.layer = l.HidLayer.document.layer = l.VisLayer.document.layer = l;
	l.MonVisLayer.document.layer = l.MonHidLayer.document.layer = l;
	
	//dt_tag_images(l.document, 'dt_pn', l);
	htutil_tag_images(l,'dt_pn',l,l);
	imgs = pg_images(l);
	//l.document.images[4].kind = 'dtimg_yrdn';
	imgs[4].kind = 'dtimg_yrdn';
	//l.document.images[5].kind = 'dtimg_mndn';
	imgs[5].kind = 'dtimg_mndn';
	//l.document.images[6].kind = 'dtimg_mnup';
	imgs[6].kind = 'dtimg_mnup';
	//l.document.images[7].kind = 'dtimg_yrup';
	imgs[7].kind = 'dtimg_yrup';

	dt_inittime(l);
	return l;
}

// expand the date/time control
function dt_expand(l) {
	dt_prepare(l);
	pg_stackpopup(l.PaneLayer, l);
	pg_positionpopup(l.PaneLayer, getPageX(l), getPageY(l), l.h, l.w);
	htr_setvisibility(l.PaneLayer, 'inherit');
	l.typed_content = '';
}

// collapse the date/time control
function dt_collapse(l) {
	htr_setvisibility(l.PaneLayer, 'hidden');
}

/*  Event Functions  */
function dt_mousedown(l) {
	p = l;
	if (p.kind == 'dt_day') {
		dt_drawdate(p.ml, p.DateVal);
		p = l.ml; 
	} else if (p.kind == 'dt' && p.enabled == 'full') {
		dt_toggle(p);
	}
	if (p.kind == 'dt' || p.kind == 'dt_day') {
		if (dt_current) {
			dt_current = null;
			dt_collapse(p);
		} else if (p.enabled == 'full') {
			dt_current = p;
			dt_expand(p);
		}
	}
}

function dt_mouseup(l) {
	if (l.kind == 'dt' && l.enabled == 'full') {
		dt_toggle(l);
	}
	if (dt_timeout) {
		clearTimeout(dt_timeout);
		dt_timeout = null;
		dt_timeout_fn = null;
		dt_set_rocker(l.TimeVisLayer.sc_img, null);
		dt_set_rocker(l.TimeVisLayer.mn_img, null);
		dt_set_rocker(l.TimeVisLayer.hr_img, null);
	}
}

function dt_set_rocker(i,y) {
	if (!y) 
	    i.src = '/sys/images/spnr_both.gif';
	else if (y < 20)
	    i.src = '/sys/images/spnr_both_up.gif';
	else
	    i.src = '/sys/images/spnr_both_down.gif';
}

function dt_do_timeout() {
	dt_timeout_fn(false, dt_img_y);
	dt_timeout = setTimeout(dt_do_timeout, 50);
}

function dt_yrdn(first) {
	dt_current.TmpDateObj.setYear(dt_current.TmpDateObj.getYear()+1899);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_yrdn;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
}

function dt_yrup(first) {
	dt_current.TmpDateObj.setYear(dt_current.TmpDateObj.getYear()+1901);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_yrup;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
}

function dt_mndn(first) {
	dt_current.TmpDateObj.setMonth(dt_current.TmpDateObj.getMonth()-1);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_mndn;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
}

function dt_mnup(first) {
	dt_current.TmpDateObj.setMonth(dt_current.TmpDateObj.getMonth()+1);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_mnup;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
}

function dt_edhr(first,y) {
	if (first) dt_set_rocker(dt_current.PaneLayer.TimeVisLayer.hr_img, y);
	dt_current.TmpDateObj.setHours(dt_current.TmpDateObj.getHours() + ((y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_edhr;
	dt_img_y = y;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
}

function dt_edmn(first,y) {
	if (first) dt_set_rocker(dt_current.PaneLayer.TimeVisLayer.mn_img, y);
	dt_current.TmpDateObj.setMinutes(dt_current.TmpDateObj.getMinutes() + ((y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_edmn;
	dt_img_y = y;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
}

function dt_edsc(first,y) {
	if (first) dt_set_rocker(dt_current.PaneLayer.TimeVisLayer.sc_img, y);
	dt_current.TmpDateObj.setSeconds(dt_current.TmpDateObj.getSeconds() + ((y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_edsc;
	dt_img_y = y;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
}
