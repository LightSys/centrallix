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
	if(this.form && this.form.mode == 'Query' && this.sbr && this.DateObj)
	    return new Array('>= ' + dt_formatdate(this, this.DateObj, 3),'<= ' + dt_formatdate(this, this.DateObj2, 3));
	else if(this.form && this.form.mode == 'Query' && this.DateObj)
	    return dt_formatdate(this, this.DateObj, 3);
	else if (this.DateObj)
	    return dt_formatdate(this, this.DateObj, 4);
	else
	    return null;
}

function dt_cb_getvalue(a) {
	return this.getvalue();
}

function dt_cb_setvalue(a,v) {
	return this.ifcProbe(ifAction).Invoke("SetValue", {Value:v});
}

function dt_setvalue(v,nodrawdate) {
	var oldval = this.getvalue();
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
	if(!nodrawdate) dt_drawdate(this, this.DateObj);
	if (this.PaneLayer) {
		dt_drawmonth(this.PaneLayer, this.DateObj);
		if (!this.date_only)
			dt_drawtime(this.PaneLayer, this.DateObj);
	}
	this.ifcProbe(ifValue).Changing('value', this.getvalue(), true, oldval, true);
}

function dt_setvalue2(v,nodrawdate) {
	if (v) {
		this.DateObj2 = new Date(v);
		this.TmpDateObj2 = new Date(v);
		if (this.DateObj2 == 'Invalid Date') {
			this.DateObj2 = new Date();
			this.TmpDateObj2 = new Date();
		}
	} else {
		this.DateObj2 = null;
		this.TmpDateObj2 = null;
	}
	if(!nodrawdate) dt_writedate(this, dt_formatdate(this,this.DateObj2,2));
	if (this.PaneLayer2) {
		dt_drawmonth(this.PaneLayer2, this.DateObj2);
		if (!this.date_only)
			dt_drawtime(this.PaneLayer2, this.DateObj2);
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
	pg_images(this)[0].src = '/sys/images/ico17.gif';
	if (this.bg) htr_setbgcolor(this, this.bg);
	if (this.bgi) htr_setbgimage(this, this.bgi);
}

function dt_readonly() {
	this.enabled = 'readonly';
	pg_images(this)[0].src = '/sys/images/ico17.gif';
	if (this.bg) htr_setbgcolor(this, this.bg);
	if (this.bgi) htr_setbgimage(this, this.bgi);
}

function dt_disable() {
	this.enabled = 'disabled';
	//this.bgColor = '#e0e0e0';
	pg_images(this)[0].src = '/sys/images/ico17d.gif';
	if (this.bg) htr_setbgcolor(this, '#e0e0e0');
}

function dt_actionsetvalue(aparam) {
    //htr_alert(aparam,1);
    if ((typeof aparam.Value) != 'undefined') {
	this.setvalue(aparam.Value, false);
	if (this.form) this.form.DataNotify(this, true);
	cn_activate(this,"DataChange", {});
    }
}

function dt_changemode(){
    var l = this.mainlayer;
    if(this.form && this.form.mode == 'Query' && this.sbr){
	if(!this.DateObj) this.DateObj = new Date();
	if(!this.DateObj2)  this.DateObj2 = new Date();
	this.DateObj.setHours(0);
	this.DateObj.setMinutes(0);
	this.DateObj.setSeconds(0);
	this.DateObj2.setHours(23);
	this.DateObj2.setMinutes(59);
	this.DateObj2.setSeconds(59);
	this.PaneLayer = null;
	this.PaneLayer2 = null;
    }
    //get rid of old pane layers because they will need to be relabeled
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

	l.sbr = param.sbr;
	
	//l.mainlayer = l;
	//c1.mainlayer = l;
	//c2.mainlayer = l;
	l.setvalue   = dt_setvalue;
	l.setvalue2  = dt_setvalue2;
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

	// Date only / default time setting
	l.date_only = ((typeof param.donly) == 'undefined')?0:param.donly;
	l.default_time = ((typeof param.dtime) == 'undefined')?'':param.dtime;
	var regex_timeformat = /(\d{0,2}):(\d{0,2})(:(\d{0,2})){0,1}/;
	l.dtime_vals = regex_timeformat.exec(l.default_time);

	l.DateStr = param.id;
	l.MonthsAbbrev = Array('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');
	l.VisLayer = c1;
	l.HidLayer = c2;
	if (param.id) {
		l.DateObj = new Date(param.id);
		l.TmpDateObj = new Date(param.id);
		l.DateObj2 = new Date(param.id);
		l.TmpDateObj2 = new Date(param.id);
		dt_drawdate(l, l.DateObj);
	} else {
		//l.DateObj = new Date();
		l.DateObj = null;
		l.TmpDateObj = new Date();
		//l.DateObj2 = new Date();
		l.DateObj2 = null;
		l.TmpDateObj2 = new Date();
		dt_drawdate(l, '');
	}
	if (param.form)
	    l.form = wgtrGetNode(l, param.form);
	else
	    l.form = wgtrFindContainer(l,"widget/form");
	if (l.form) l.form.Register(l);
	pg_addarea(l, -1, -1, getClipWidth(l)+3, getClipHeight(l)+3, 'dt', 'dt', 3);

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

	if(l.form)
	    l.form.ifcProbe(ifEvent).Hook('StatusChange',dt_changemode,l);

	// Actions
	var ia = l.ifcProbeAdd(ifAction);
	ia.Add("SetValue", dt_actionsetvalue);

	// Values
	var iv = l.ifcProbeAdd(ifValue);
	iv.Add("value", dt_cb_getvalue, dt_cb_setvalue);

	return l;
}

// Called before the popup or month pane is used.  We do this to defer the
// complex pane drawing operation until we actually need it (thus faster page
// load times).
function dt_prepare(l) {
	// Create the pane if needed.
	if((!l.form || l.form.mode != 'Query' || !l.sbr) && l.PaneLayer2){
	    l.PaneLayer = null;
	    l.PaneLayer2 = null;
	}
	if (!l.PaneLayer) {
		l.PaneLayer = dt_create_pane(l,l.ubg,l.w2,l.h2,l.h,"Start");
		l.PaneLayer.ml = l;
		l.PaneLayer.myid = "PaneLayer1";
		l.PaneLayer.HidLayer.Areas = l.PaneLayer.VisLayer.Areas = new Array();
		l.PaneLayer.VisLayer.getfocushandler = dt_getfocus_day;
		l.PaneLayer.HidLayer.getfocushandler = dt_getfocus_day;
		l.PaneLayer.VisLayer.losefocushandler = dt_losefocus_day;
		l.PaneLayer.HidLayer.losefocushandler = dt_losefocus_day;
	}
	if (!l.PaneLayer2) {
	    l.PaneLayer2 = dt_create_pane(l,l.ubg,l.w2,l.h2,l.h,"End");
	    l.PaneLayer2.ml = l;
	    l.PaneLayer2.myid = "PaneLayer2";
	    l.PaneLayer2.HidLayer.Areas = l.PaneLayer2.VisLayer.Areas = new Array();
	    l.PaneLayer2.VisLayer.getfocushandler = dt_getfocus_day;
	    l.PaneLayer2.HidLayer.getfocushandler = dt_getfocus_day;
	    l.PaneLayer2.VisLayer.losefocushandler = dt_losefocus_day;
	    l.PaneLayer2.HidLayer.losefocushandler = dt_losefocus_day;
		
	}
	// redraw the month & time.
	if(l.form && l.form.mode == 'Query' && l.sbr){
	    l.TmpDateObj2 = (l.DateObj2?(new Date(l.DateObj2)):null);
	    dt_drawmonth(l.PaneLayer2, l.TmpDateObj2);
	    if (!l.date_only)
		dt_drawtime(l.PaneLayer2, l.TmpDateObj2);
	    if (!l.TmpDateObj2) l.TmpDateObj2 = new Date();
	}
	l.TmpDateObj = (l.DateObj?(new Date(l.DateObj)):null);
	dt_drawmonth(l.PaneLayer, l.TmpDateObj);
	if (!l.date_only)
	    dt_drawtime(l.PaneLayer, l.TmpDateObj);
	if (!l.TmpDateObj) l.TmpDateObj = new Date();
}

function dt_formatdate(l, d, fmt) {
	var str;
	switch (fmt) {
		case 1: str = l.MonthsAbbrev[d.getMonth()] + ' ';
			str += d.getDate() + ', ';
			str += d.getYear()+1900;
			str += ' - ';
			str += l.MonthsAbbrev[l.DateObj2.getMonth()] + ' ';
			str += l.DateObj2.getDate() + ', ';
			str += l.DateObj2.getYear() + 1900;
			break;
		case 2: str = l.MonthsAbbrev[l.DateObj.getMonth()] + ' ';
			str += l.DateObj.getDate() + ', ';
			str += l.DateObj.getYear() + 1900;
			str += ' - ';
			str += l.MonthsAbbrev[d.getMonth()] + ' ';
			str += d.getDate() + ', ';
			str += d.getYear()+1900;
			break;
		case 3: //this one is compatible with an sql query
			str = d.getDate() + ' ';
			str += l.MonthsAbbrev[d.getMonth()] + ' ';
			str += d.getYear() + 1900 + ' ';
			str += htutil_strpad(d.getHours(), '0', 2)+':';
			str += htutil_strpad(d.getMinutes(), '0', 2);
			break;
		case 4:	// format for returned data value
			str  = l.MonthsAbbrev[d.getMonth()] + ' ';
			str += d.getDate() + ', ';
			str += d.getYear()+1900;
			str += ', ';
			str += htutil_strpad(d.getHours(), '0', 2)+':';
			str += htutil_strpad(d.getMinutes(), '0', 2)+':';
			str += htutil_strpad(d.getSeconds(), '0', 2);
			break;
		case 0:	// format for visual
		default:
			str  = l.MonthsAbbrev[d.getMonth()] + ' ';
			str += d.getDate() + ', ';
			str += d.getYear()+1900;
			if (!l.date_only) {
			    str += ', ';
			    str += htutil_strpad(d.getHours(), '0', 2)+':';
			    str += htutil_strpad(d.getMinutes(), '0', 2);
			}
	}
	return str;
}

function dt_writedate(l, txt) {
	var v = '<TABLE border=0 cellspacing=0 cellpadding=0 height=100\% width='+l.w+'>';
	v += '<TR><TD align=center valign=middle nowrap><FONT color=\"'+l.fg+'\">';
	v += htutil_encode(htutil_obscure(txt));
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
	var dy=0,r=-1,rows=5;
	var TmpDate;
	if (d) 
	    TmpDate = new Date(d);
	else 
	    TmpDate = new Date();
	if (l.ml.default_time) {
	    TmpDate.setHours(l.ml.dtime_vals[1]?l.ml.dtime_vals[1]:0);
	    TmpDate.setMinutes(l.ml.dtime_vals[2]?l.ml.dtime_vals[2]:0);
	    TmpDate.setSeconds(l.ml.dtime_vals[4]?l.ml.dtime_vals[4]:0);
	}
	TmpDate.setDate(1);
	var col=TmpDate.getDay();
	var num=htutil_days_in_month(TmpDate);
	while (l.VisLayer.Areas.length) {
		pg_removearea(l.VisLayer.Areas[0]);
		l.VisLayer.Areas.shift();
	}
	pg_removearea(l.VisLayer.NullArea);

	var cur_dy = null;
	if (l.myid != 'PaneLayer2' && l.ml.DateObj && TmpDate.getMonth() == l.ml.DateObj.getMonth() && TmpDate.getYear() == l.ml.DateObj.getYear())
	    cur_dy = l.ml.DateObj.getDate();
	if (l.myid == 'PaneLayer2' && l.ml.DateObj2 && TmpDate.getMonth() == l.ml.DateObj2.getMonth() && TmpDate.getYear() == l.ml.DateObj2.getYear())
	    cur_dy = l.ml.DateObj2.getDate();
	
	rows=Math.ceil((num+col)/7);
	if(l.ml.form && l.ml.form.mode == 'Query' && l.ml.sbr){
	    pg_set_style(l,'height',rows*20+110);
	    setClipHeight(l,rows*20+110);
	    if (!l.ml.date_only) {
		moveTo(l.TimeHidLayer,0,rows*20+76);
		moveTo(l.TimeVisLayer,0,rows*20+76);
	    }
	}
	else{
	    pg_set_style(l,'height',rows*20+90);
	    setClipHeight(l,rows*20+90);
	    if (!l.ml.date_only) {
		moveTo(l.TimeHidLayer,0,rows*20+56);
		moveTo(l.TimeVisLayer,0,rows*20+56);
	    }
	}
	var v='<TABLE width=175 height='+rows*20  +' border=0 cellpadding=0 cellspacing=0>';
	for (var i=0; i<7*rows; i++) {
		if (i!=0 && i%7==0) v+='</TR>';
		if (i%7==0) {v+='<TR>\n';r++}
		v+='<TD width=25 height=18 valign=middle align=right>';
		if (i>=col && dy<num) {
			l.HidLayer.Areas[dy]=pg_addarea(l.HidLayer, 1+((i%7)*25), (1+(r*18)), 25, 18, 'dt', 'dt_day', 3);
			l.HidLayer.Areas[dy].parentPaneId = l.myid;
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
	//for (;i <= 31; i++) {  }

	// write items for No Date (null) and Today.
	v+='</tr><tr><td colspan=4 align=center height=18 valign=middle><i>(<u>N</u>o Date)</i></td><td colspan=3 align=center height=18 valign=middle><i>(<u>T</u>oday)</i></td>';
	l.HidLayer.NullArea = pg_addarea(l.HidLayer, 1, 1+(r+1)*18, 100, 16, 'dt', 'dt_null', 3);
	l.HidLayer.NullArea.DateVal = null;
	l.HidLayer.NullArea.parentPaneId = l.myid;
	l.HidLayer.TodayArea = pg_addarea(l.HidLayer, 101, 1+(r+1)*18, 83, 16, 'dt', 'dt_today', 3);
	l.HidLayer.TodayArea.DateVal = new Date();
	l.HidLayer.TodayArea.parentPaneId = l.myid;
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
	var tvl = l.TimeVisLayer;
	if (!tvl.hours) {
		tvl.hours = htr_new_layer(20,tvl);
		htr_init_layer(tvl.hours,l,'dt_pn');
		moveTo(tvl.hours,16,10);
		htr_setvisibility(tvl.hours,'inherit');
	}
	htr_write_content(tvl.hours, d?htutil_strpad(d.getHours(),0,2):'');
	if (!tvl.minutes) {
		tvl.minutes = htr_new_layer(20,tvl);
		moveTo(tvl.minutes,52,10);
		htr_init_layer(tvl.minutes,l,'dt_pn');
		htr_setvisibility(tvl.minutes,'inherit');
	}
	htr_write_content(tvl.minutes, ':&nbsp;' + (d?htutil_strpad(d.getMinutes(),0,2):''));
	if (!tvl.seconds) {
		tvl.seconds = htr_new_layer(20,tvl);
		moveTo(tvl.seconds,90,10);
		htr_init_layer(tvl.seconds,l,'dt_pn');
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
	for (var i=0; i<imgs.length;i++) {
		if (i == 0)
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
	l.setvalue(d,false);
	if (l.form) l.form.DataNotify(l);
	cn_activate(l, 'DataChange');
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
				/*var d = new Date(dt.typed_content);
				if (d.getFullYear() < (new Date()).getFullYear()-90 && dt.typed_content.indexOf(d.getFullYear()) < 0)
					d.setFullYear(d.getFullYear() + 100);
				dt_setdata(dt,d);
				*/
				dt_parse_date(dt,dt.typed_content,true); //true tells it to actually set the date not just display it in the pane
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
				/*var d = new Date(dt.typed_content);
				if (d.getFullYear() < (new Date()).getFullYear()-90 && dt.typed_content.indexOf(d.getFullYear()) < 0)
					d.setFullYear(d.getFullYear() + 100);
				dt_setdata(dt,d);
				*/
				dt_parse_date(dt,dt.typed_content,true);
			} else {
				dt_setdata(dt,dt.TmpDateObj);
			}
		}
		if (dt.form) {
			if (e.shiftKey)
				dt.form.ShiftTabNotify(dt);
			else
				dt.form.TabNotify(dt);
		}
	} else if (k == 32) {		// spacebar
		if (!dt_current) {
			dt_expand(dt);
			dt_current = dt;
		} else if (dt.typed_content) {
			dt.typed_content += String.fromCharCode(k);
			dt_update_typed(dt);
		}
	}/* else if (k == 'n'.charCodeAt() || k == 'N'.charCodeAt()) {
		if (dt_current) {
			dt_collapse(dt);
			dt_current = null;
			dt_setdata(dt,null);
		}
		if (dt.form) dt.form.DataNotify(dt);
		cn_activate(dt, 'DataChange');
	} else if (k == 't'.charCodeAt() || k == 'T'.charCodeAt()) {
		if (dt_current) {
			dt_collapse(dt);
			dt_current = null;
			dt_setdata(dt,new Date());
		} else {
			dt_setdata(dt,new Date());
		}
		if (dt.form) dt.form.DataNotify(dt);
		cn_activate(dt, 'DataChange');
	}*/
	else if ((k >= 97 && k <= 122) ||( k>=65 && k<=90) || k ==45 || (k >= 48 && k < 58)) { //letters or numbers or hyphen
	    if(!dt_current) {
		dt_expand(dt);
		dt_current = dt;
	    }
	    dt.typed_content += String.fromCharCode(k);
	    dt_update_typed(dt);
	    if(dt.form) dt.form.DataNotify(dt);
	    cn_activate(dt, 'DataChange');
	} else if (e.keyText == '/' || e.keyText == '-' || e.keyText == ':') {
		if (dt_current && dt.typed_content) {
			dt.typed_content += e.keyText;
			dt_update_typed(dt);
		}
	} else if (k == '/'.charCodeAt() || k == ':'.charCodeAt()) {
		if (dt_current && dt.typed_content) {
			dt.typed_content += String.fromCharCode(k);
			dt_update_typed(dt);
		}
	} else if (k == 8) {		// backspace
		if (dt_current && dt.typed_content) {
			dt_current = dt;
			dt.typed_content = dt.typed_content.substr(0,dt.typed_content.length-1);
			dt_update_typed(dt);
		}
	}

	return true;
}

//offset is used because so that this will work with a date that starts somewhere in the middle of a regular expression
//e.g. in query mode the user should enter date1-date2.  The first 5 captured values (1-5 since 0 is the whole match) will
//pertain to the first date and the second 5 will pertain to the second date so we use an offset of 5.
function dt_get_parsed_date(dt,origdate,vals,offset,now){
	/* javascript has a quirk that if the current date is say, july 31, then setting the month to february (1) will
	   overflow and the month will actually get set to March (2)!!! So initialize this to 1,1,1 to make sure this
	   doesn't happen! */
	var d = new Date(1,1,1);
	vals[1+offset]--;
	d.setYear((vals[3+offset])?vals[3+offset]:origdate.getFullYear());
	d.setMonth((vals[1+offset]>=0)?vals[1+offset]:origdate.getMonth());
	d.setDate((vals[2+offset])?vals[2+offset]:origdate.getDate());
	d.setHours((vals[4+offset])?vals[4+offset]:origdate.getHours());
	d.setMinutes((vals[5+offset])?vals[5+offset]:origdate.getMinutes());    
	d.setSeconds((vals[5+offset])?0:origdate.getSeconds()); // if min specified, set sec=0
	if (d.getFullYear() < now.getFullYear()-90 && dt.typed_content.indexOf(d.getFullYear()) < 0) //ten year window
	    d.setFullYear(d.getFullYear() + 100);
	if(!(vals[3+offset])){ //year not entered 
	    if (d.getMonth()==11 && now.getMonth()==0) //one month window
		d.setFullYear(now.getFullYear()-1);
	    else
		d.setFullYear(now.getFullYear());
	}
	return d;
}

function dt_parsewords(dt) {
    var retval;
    var reg_now = /^now$/i;
    var reg_tod = /^t(oday)?$/i;
    var reg_yest = /^yesterday$/i;
    if(dt.typed_content.match(reg_now) || dt.typed_content.match(reg_tod))
	return new Date();
    else if(dt.typed_content.match(reg_yest)){
	retval = new Date();
	retval.setDate(retval.getDate()-1);
	return retval;
    }
    else return null;
}

function dt_parse_date(dt,content,drawdate){
    var defaults = new Date();
    var d1,d2;
    var regex_letters = /.*[a-zA-Z]+.*/; //if there are letters
    if (dt.default_time) {
	defaults.setHours(dt.dtime_vals[1]?dt.dtime_vals[1]:0);
	defaults.setMinutes(dt.dtime_vals[2]?dt.dtime_vals[2]:0);
	defaults.setSeconds(dt.dtime_vals[4]?dt.dtime_vals[4]:0);
    }
    if(regex_letters.exec(content)){
	d1 = dt_parsewords(dt);
	dt.setvalue(d1,!drawdate);
    }
    else{
	if(dt.form && dt.form.mode == 'Query' && dt.sbr){
	    var regex_dateformat = /(\d{0,2})\/?(\d{0,2})\/?(\d{0,4})(?: (\d{0,2}):(\d{0,2})){0,1}(?:\s?-\s?)?(\d{0,2})\/?(\d{0,2})\/?(\d{0,4})(?: (\d{0,2}):(\d{0,2})){0,1}/;
	    var vals = regex_dateformat.exec(content);
	    var origdate = (dt_current)?dt_current.TmpDateObj:defaults;
	    d1 = dt_get_parsed_date(dt,origdate,vals,0,defaults);
	    origdate = (dt_current)?dt_current.TmpDateObj2:defaults;
	    d2 = dt_get_parsed_date(dt,origdate,vals,5,defaults);
	    
	    dt.setvalue(d1,!drawdate);
	    dt.setvalue2(d2,!drawdate);
	}
	else{
	    //if(drawdate) dt_setdata(dt,d);
	    var regex_dateformat = /(\d{0,2})[-\/]?(\d{0,2})[-\/]?(\d{0,4})(?: (\d{0,2}):(\d{0,2})){0,1}/;
	    var vals = regex_dateformat.exec(content);
	    var origdate = (dt_current)?dt_current.TmpDateObj:defaults;
	    d1 = dt_get_parsed_date(dt,origdate,vals,0,defaults);
	    dt.setvalue(d1,!drawdate);
	}
    }
    if (dt.form) dt.form.DataNotify(dt);
    cn_activate(dt, 'DataChange');
}

// dt_update_typed takes the typed-in content and updates the values of
// the visible control
function dt_update_typed(l) {
	dt_writedate(l,l.typed_content);
	dt_parse_date(l,l.typed_content,false);	
}


// dt_getfocus is called when the user selects or tabs to the control
function dt_getfocus(x,y,l) {
	if (this.enabled != 'full' || (this.form && !this.form.is_focusable)) return 0;
	if(l.form) l.form.FocusNotify(l);
	cn_activate(l, 'GetFocus');
	return 1;
}

function dt_addday(c,e,f){
    //this adds a day if we are in query mode
    if(f.parentPaneId == 'PaneLayer1'){
	dt_writedate(dt_current,dt_formatdate(dt_current,f.DateVal,1));
    }
    else{
	dt_writedate(dt_current,dt_formatdate(dt_current,f.DateVal,2));
    }
}

// dt_getfocus_day is called when the user selects a particular date/time
// in the popup calendar window
function dt_getfocus_day(a,b,c,d,e,f) {
	// check area
	if (e != 'dt_today' && e != 'dt_day' && e != 'dt_null') return;
	if (!dt_current) return;

	var oldval = dt_current.getvalue();

	// hide the drop down part of the control
	if(!dt_current.form || dt_current.form.mode != 'Query' || !dt_current.sbr){
	    dt_collapse(dt_current);
	}

	// Tell form we just got focus
	if (dt_current.form) 
		dt_current.form.FocusNotify(dt_current);

	// Set the data value (after focus; form might reset our data
	// value after it transitions to 'new' mode from 'nodata'
	if (e == 'dt_today') f.DateVal = new Date();
	if(f.parentPaneId != 'PaneLayer2'){
	    if (!dt_current.TmpDateObj) {
		    dt_current.TmpDateObj = new Date(dt_current.DateObj);
		    if (dt_current.default_time) {
			    dt_current.TmpDateObj.setHours(dt_current.dtime_vals[1]?dt_current.dtime_vals[1]:0);
			    dt_current.TmpDateObj.setMinutes(dt_current.dtime_vals[2]?dt_current.dtime_vals[2]:0);
			    dt_current.TmpDateObj.setSeconds(dt_current.dtime_vals[4]?dt_current.dtime_vals[4]:0);
		    }
	    }
	    if (f.DateVal) {
		dt_current.DateObj = new Date(f.DateVal);
		//if (dt_current.TmpDateObj && e != 'dt_today') {
		    //dt_current.DateObj.setHours(dt_current.TmpDateObj.getHours(), dt_current.TmpDateObj.getMinutes(), dt_current.TmpDateObj.getSeconds());
		var now = new Date();
		if (!dt_current.default_time && dt_current.DateObj.getDate() == now.getDate() && dt_current.DateObj.getMonth() == now.getMonth() && dt_current.DateObj.getYear == now.getYear()) {
		    dt_current.DateObj.setHours(now.getHours(), now.getMinutes(), now.getSeconds());
		}
	    } else {
		dt_current.DateObj = null;
	    }
	}
	else {
	    if(!dt_current.TmpDateObj2)
		dt_current.TmpDateObj2 = new Date(dt_current.DateObj2);
	    if(f.DateVal) {
		dt_current.DateObj2 = new Date(f.DateVal);
		if(dt_current.TmpDateObj2 && e != 'dt_today') {
		    dt_current.DateObj2.setHours(dt_current.TmpDateObj2.getHours(), dt_current.TmpDateObj2.getMinutes(), dt_current.TmpDateObj.getSeconds());
		}
	    } else {
		dt_current.DateObj2 = null;
	    }
	}
	// Tell form the value changed
	if (dt_current.form) {
		dt_current.form.DataNotify(dt_current);
		cn_activate(dt_current, 'DataChange');
	}
	var newval = dt_current.getvalue();
	if (newval != oldval)
	    dt_current.ifcProbe(ifValue).Changing('value', newval, true, oldval, true);

	// draw the date/time on the collapsed control
	if(dt_current.form && dt_current.form.mode == 'Query' && dt_current.sbr){
	    dt_addday(c,e,f);
	}
	else{
	    dt_drawdate(dt_current, dt_current.DateObj);
	
	    // no longer current.
	    dt_current = null;
	}
}

function dt_losefocus() {
	cn_activate(this, 'LoseFocus');
	return true;
}

function dt_losefocus_day() {
	return true;
}

function dt_create_pane(ml,bg,w,h,h2,name) {
	var str;
	var imgs;
	//l = new Layer(1024);
	var l = htr_new_layer(1024,ml);
	htr_setbackground(l,bg);
	htr_init_layer(l,l,'dt_pn'); //dkasper uncommented
	htr_setvisibility(l,'hidden');

	str = "<BODY "+bg+">";
	str += "<TABLE border=0 cellpadding=0 cellspacing=0 width="+w+" height="+(h+40)+">";
	str += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>";
	str += "	<TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(w-2)+"></TD>";
	str += "	<TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>";
	/*if (ml.form.mode == 'Query'){
	    str += "<TR><TD width=175 height='18' align=center>"+name+"</TD></TR>";
	    moveTo(ml.HidLayer, 0, 68);
	    moveTo(ml.VisLayer, 0, 68);
	    h+=20;
	    //moveTo(ml.MonHidLayer, 38, 22);
	    //moveTo(ml.MonVisLayer, 38, 22);
	    //moveTo(ml.TimeHidLayer, 0, 176);
	    //moveTo(ml.TimeVisLayer, 0, 176);
	}
	else{
	    moveTo(ml.HidLayer, 0, 48);
	    moveTo(ml.VisLayer, 0, 48);
	    h-=20;
	    //moveTo(ml.MonHidLayer, 38, 2);
	    //moveTo(ml.MonVisLayer, 38, 2);
	    //moveTo(ml.TimeHidLayer, 0, 156);
	    //moveTo(ml.TimeVisLayer, 0, 156);
	}*/
	str += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height="+(h-2)+" width=1></TD>";
	str += "	<TD valign=top>";
	if(ml.form && ml.form.mode =='Query' && ml.sbr){
	    str += "        <TABLE height=20 cellpadding=0 cellspacing=0 border=0>";
	    str += "        <TR><TD width="+w+" align=center><b>"+name+"</b></TD></TR></TABLE>";
	    h+=20;
	}
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
	pg_stackpopup(l,ml);
	setClipHeight(l,h);
	setClipWidth(l,w);
	
	//l.HidLayer = new Layer(1024, l);
	l.HidLayer = htr_new_layer(1024,l);
	//l.VisLayer = new Layer(1024, l);
	l.VisLayer = htr_new_layer(1024,l);
	//l.MonHidLayer = new Layer(1024, l);
	l.MonHidLayer = htr_new_layer(116,l);
	//l.MonVisLayer = new Layer(1024, l);
	l.MonVisLayer = htr_new_layer(116,l);
	if (!ml.date_only) {
	    l.TimeHidLayer = htr_new_layer(1024, l);
	    l.TimeVisLayer = htr_new_layer(1024, l);
	}
	if(ml.form && ml.form.mode == 'Query' && ml.sbr){
	    moveTo(l.HidLayer, 0, 68);
	    moveTo(l.VisLayer, 0, 68);
	    moveTo(l.MonHidLayer, 38, 22);
	    moveTo(l.MonVisLayer, 38, 22);
	    if (!ml.date_only) {
		moveTo(l.TimeHidLayer, 0, 176);
		moveTo(l.TimeVisLayer, 0, 176);
	    }
	}
	else{
	    moveTo(l.HidLayer, 0, 48);
	    moveTo(l.VisLayer, 0, 48);
	    moveTo(l.MonHidLayer, 38, 2);
	    moveTo(l.MonVisLayer, 38, 2);
	    if (!ml.date_only) {
		moveTo(l.TimeHidLayer, 0, 156);
		moveTo(l.TimeVisLayer, 0, 156);
	    }
	}
	//l.HidLayer.y = l.VisLayer.y = 48;
	//l.MonHidLayer.x = l.MonVisLayer.x = 38;
	//l.MonHidLayer.y = l.MonVisLayer.y = 2;
	l.x = ml.pageX;
	l.y = ml.pageY+h2;
	//l.ml = ml;
	//l.kind = l.HidLayer.kind = l.VisLayer.kind = l.MonHidLayer.kind = l.MonVisLayer.kind = 'dt_pn';
	//l.document.layer = l.HidLayer.document.layer = l.VisLayer.document.layer = l;
	//l.MonVisLayer.document.layer = l.MonHidLayer.document.layer = l;

	l.HidLayer.kind = 'dt_pn';
	l.VisLayer.kind = 'dt_pn';
	//htr_init_layer(l.HidLayer,l,'dt_pn');
	//htr_init_layer(l.VisLayer,l,'dt_pn');
	htr_init_layer(l.MonHidLayer,l,'dt_pn');
	htr_init_layer(l.MonVisLayer,l,'dt_pn');
	if (!ml.date_only) {
	    htr_init_layer(l.TimeHidLayer,l,'dt_pn');
	    htr_init_layer(l.TimeVisLayer,l,'dt_pn');
	}
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
	if (!ml.date_only)
	    dt_inittime(l);
	return l;
}

// expand the date/time control
function dt_expand(l) {
	dt_prepare(l);
	pg_stackpopup(l.PaneLayer, l);
	pg_positionpopup(l.PaneLayer, getPageX(l), getPageY(l), l.h, l.w);
	htr_setvisibility(l.PaneLayer, 'inherit');
	if(l.form && l.form.mode == 'Query' && l.sbr){
	    pg_stackpopup(l.PaneLayer2, l);
	    pg_positionpopup(l.PaneLayer2, getPageX(l)+getClipWidth(l.PaneLayer)+5, getPageY(l), l.h, l.w);
	    htr_setvisibility(l.PaneLayer2, 'inherit');
	}
	l.typed_content = '';
}

// collapse the date/time control
function dt_collapse(l) {
	htr_setvisibility(l.PaneLayer, 'hidden');
	if(l.PaneLayer2)
	    htr_setvisibility(l.PaneLayer2, 'hidden');
}

/*  Event Functions  */
function dt_domousedown(l) {
	p = l;
	if (p.kind == 'dt_day') {
		dt_drawdate(p.ml, p.DateVal);
		p = l.ml; 
	} else if (p.kind == 'dt' && p.mainlayer.enabled == 'full') {
		dt_toggle(p.mainlayer);
	}
	if (p.kind == 'dt' || p.kind == 'dt_day') {
		if (dt_current && (((!dt_current.form || dt_current.form.mode != 'Query' || !dt_current.sbr) && p.kind =='dt_day') || p.kind == 'dt')) {
			dt_current = null;
			dt_collapse(p.mainlayer);
			if (p.mainlayer.typed_content)
				dt_parse_date(p.mainlayer,p.mainlayer.typed_content,true);
			p.mainlayer.typed_content = '';
		} else if (p.mainlayer.enabled == 'full' && (!p.mainlayer.form || p.mainlayer.form.is_focusable)) {
			dt_current = p.mainlayer;
			dt_expand(p.mainlayer);
		}
	}
}

function dt_domouseup(l) {
	if (l.kind == 'dt' && l.mainlayer.enabled == 'full') {
		dt_toggle(l.mainlayer);
	}
	if (dt_timeout) {
		clearTimeout(dt_timeout);
		dt_timeout = null;
		dt_timeout_fn = null;
		if (!l.ml.date_only) {
			dt_set_rocker(l.TimeVisLayer.sc_img, null);
			dt_set_rocker(l.TimeVisLayer.mn_img, null);
			dt_set_rocker(l.TimeVisLayer.hr_img, null);
		}
	}
}

function dt_set_rocker(i,y) {
	pg_debug("y=" + y + "\n");
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

function dt_yrdn(first,pagey,id) {
	if(id=='PaneLayer1'){
	dt_current.TmpDateObj.setYear(dt_current.TmpDateObj.getYear()+1899);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_yrdn;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	else{
	    //alert("PaneLayer2");
	    dt_current.TmpDateObj2.setYear(dt_current.TmpDateObj2.getYear()+1899);
	    dt_drawmonth(dt_current.PaneLayer2,dt_current.TmpDateObj2);
	    dt_timeout_fn = dt_yrdn;
	    if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
}

function dt_yrup(first,pagey,id) {
	if(id=='PaneLayer1'){
	dt_current.TmpDateObj.setYear(dt_current.TmpDateObj.getYear()+1901);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_yrup;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	else{
	    //alert("PaneLayer2");
	    dt_current.TmpDateObj2.setYear(dt_current.TmpDateObj2.getYear()+1901);
	    dt_drawmonth(dt_current.PaneLayer2,dt_current.TmpDateObj2);
	    dt_timeout_fn = dt_yrup;
	    if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
}

function dt_mndn(first,pagey,id) {
	if(id=='PaneLayer1'){
	dt_current.TmpDateObj.setMonth(dt_current.TmpDateObj.getMonth()-1);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_mndn;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	else{
	dt_current.TmpDateObj2.setMonth(dt_current.TmpDateObj2.getMonth()-1);
	dt_drawmonth(dt_current.PaneLayer2, dt_current.TmpDateObj2);
	dt_timeout_fn = dt_mndn;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
}

function dt_mnup(first,pagey,id) {
	if(id=='PaneLayer1'){
	dt_current.TmpDateObj.setMonth(dt_current.TmpDateObj.getMonth()+1);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_mnup;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	else{
	dt_current.TmpDateObj2.setMonth(dt_current.TmpDateObj2.getMonth()+1);
	dt_drawmonth(dt_current.PaneLayer2, dt_current.TmpDateObj2);
	dt_timeout_fn = dt_mnup;
	if (first) dt_timeout = setTimeout(dt_do_timeout, 300);
	}
}

function dt_edhr(first,pagey,id) {
	if(id=='PaneLayer1'){
	if (first){ var y = pagey - getPageY(dt_current.PaneLayer.TimeVisLayer);
		    dt_set_rocker(dt_current.PaneLayer.TimeVisLayer.hr_img, y);
		    dt_img_y = y;
		    dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	//used to use y instead of dt_img_y, but this doesn't work because of the way y gets set now
	dt_current.TmpDateObj.setHours(dt_current.TmpDateObj.getHours() + ((dt_img_y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_edhr;
	}
	else{
	if (first){ var y = pagey - getPageY(dt_current.PaneLayer2.TimeVisLayer);
		    dt_set_rocker(dt_current.PaneLayer2.TimeVisLayer.hr_img, y);
		    dt_img_y = y;
		    dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	//used to use y instead of dt_img_y, but this doesn't work because of the way y gets set now
	dt_current.TmpDateObj2.setHours(dt_current.TmpDateObj2.getHours() + ((dt_img_y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer2, dt_current.TmpDateObj2);
	dt_timeout_fn = dt_edhr;
	}
}

function dt_edmn(first,pagey,id) {
	if(id=='PaneLayer1'){
	if (first){ var y = pagey - getPageY(dt_current.PaneLayer.TimeVisLayer);
		    dt_set_rocker(dt_current.PaneLayer.TimeVisLayer.mn_img, y);
		    dt_img_y = y;
		    dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	dt_current.TmpDateObj.setMinutes(dt_current.TmpDateObj.getMinutes() + ((dt_img_y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_edmn;
	}
	else{
	if (first){ var y = pagey - getPageY(dt_current.PaneLayer2.TimeVisLayer);
		    dt_set_rocker(dt_current.PaneLayer2.TimeVisLayer.mn_img, y);
		    dt_img_y = y;
		    dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	//used to use y instead of dt_img_y, but this doesn't work because of the way y gets set now
	dt_current.TmpDateObj2.setMinutes(dt_current.TmpDateObj2.getMinutes() + ((dt_img_y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer2, dt_current.TmpDateObj2);
	dt_timeout_fn = dt_edmn;
	}
}

function dt_edsc(first,pagey,id) {
	if(id=='PaneLayer1'){
	if (first){ var y = pagey - getPageY(dt_current.PaneLayer.TimeVisLayer);
		    dt_set_rocker(dt_current.PaneLayer.TimeVisLayer.sc_img, y);
		    dt_img_y = y;
		    dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	dt_current.TmpDateObj.setSeconds(dt_current.TmpDateObj.getSeconds() + ((dt_img_y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer, dt_current.TmpDateObj);
	dt_timeout_fn = dt_edsc;
	}
	else{
	if (first){ var y = pagey - getPageY(dt_current.PaneLayer2.TimeVisLayer);
		    dt_set_rocker(dt_current.PaneLayer2.TimeVisLayer.sc_img, y);
		    dt_img_y = y;
		    dt_timeout = setTimeout(dt_do_timeout, 300);
	}
	//used to use y instead of dt_img_y, but this doesn't work because of the way y gets set now
	dt_current.TmpDateObj2.setSeconds(dt_current.TmpDateObj2.getSeconds() + ((dt_img_y>19)?-1:1));
	dt_drawtime(dt_current.PaneLayer2, dt_current.TmpDateObj2);
	dt_timeout_fn = dt_edsc;
	}
}

function dt_mousedown(e) {
	if (e.target.kind && e.target.kind.substr(0,5) == 'dtimg') {
		eval('dt_' + e.target.kind.substr(6,4)+'(true,'+e.pageY + ',\'' + e.layer.myid + '\')');
	} else {
		if (e.kind && e.kind.substr(0,2) == 'dt') {
			dt_domousedown(e.layer);
			if (e.kind == 'dt') cn_activate(e.mainlayer, 'MouseDown');
		} else if (dt_current && dt_current != e.mainlayer && !(!e.kind && dt_current.form && dt_current.form.mode =='Query' && dt_current.sbr)) {
			dt_collapse(dt_current);
			dt_current = null;
		}
	}
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function dt_mouseup(e) {
	if (e.kind && e.kind.substr(0,2) == 'dt') {
		dt_domouseup(e.layer);
		if (e.kind == 'dt') {
			cn_activate(e.mainlayer, 'MouseUp');
			cn_activate(e.mainlayer, 'Click');
		}
	}
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function dt_mousemove(e) {
	if (e.kind && e.kind == 'dt') cn_activate(e.mainlayer, 'MouseMove');
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function dt_mouseover(e) {
	if (e.kind && e.kind == 'dt') cn_activate(e.mainlayer, 'MouseOver');
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function dt_mouseout(e) {
	if (e.kind && e.kind == 'dt') cn_activate(e.mainlayer, 'MouseOut');
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_datetime.js'] = true;
