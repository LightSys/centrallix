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
	return dt_formatdate(this, this.DateObj, 0);
}

function dt_setvalue(v) {
	this.DateObj = new Date(v);
	this.TmpDateObj = new Date(v);
	if (this.DateObj == 'Invalid Date') {
		this.DateObj = new Date();
		this.TmpDateObj = new Date();
	}
	dt_drawdate(this, this.DateObj);
	dt_drawmonth(this.PaneLayer, this.DateObj);
}

function dt_clearvalue() {
}

function dt_resetvalue() {
}

function dt_enable() {
	this.enabled = 'full';
}

function dt_readonly() {
	this.enabled = 'readonly';
}

function dt_disable() {
	this.enabled = 'disabled';
}

// Date/Time Functions

function dt_init(l,c1,c2,id,bg,fg,fn,w,h,w2,h2) {
	this.enabled = 'full';
	c1.mainlayer = l;
	c2.mainlayer = l;
	l.setvalue   = dt_setvalue;
	l.getvalue   = dt_getvalue;
	l.enable     = dt_enable;
	l.readonly   = dt_readonly;
	l.disable    = dt_disable;
	l.clearvalue = dt_clearvalue;
	l.resetvalue = dt_resetvalue;
	l.fieldname  = fn;
	l.kind  = c1.kind = c2.kind = 'dt';
	l.document.layer  = c1.document.layer = c2.document.layer = l;
	dt_tag_images(l.document, 'dt', l);
	l.w = w; l.h = h;
	l.bg = bg;
	l.fg = fg;
	l.form = fm_current;
	l.DateStr = id;
	l.MonthsAbbrev = Array('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');
	l.VisLayer = c1;
	l.HidLayer = c2;
	l.PaneLayer = dt_create_pane(l,bg,w2,h2,h);
	l.PaneLayer.ml = l;
	l.PaneLayer.HidLayer.Areas = l.PaneLayer.VisLayer.Areas = new Array();
	l.PaneLayer.VisLayer.getfocushandler = dt_getfocus;
	l.PaneLayer.HidLayer.getfocushandler = dt_getfocus;
	l.PaneLayer.VisLayer.losefocushandler = dt_losefocus;
	l.PaneLayer.HidLayer.losefocushandler = dt_losefocus;
	if (id) {
		l.DateObj = new Date(id);
		l.TmpDateObj = new Date(id);
		dt_drawdate(l, l.DateObj);
	} else {
		l.DateObj = new Date();
		l.TmpDateObj = new Date();
		dt_drawdate(l, '');
	}
	dt_drawmonth(l.PaneLayer, l.DateObj);
	if (fm_current) fm_current.Register(l);
	pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'dt', 'dt', 0);
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

function dt_drawdate(l, d) {
	l.HidLayer.document.write('<TABLE border=0 cellspacing=0 cellpadding=0 height=100\% width='+l.w+'>');
	l.HidLayer.document.write('<TR><TD align=center valign=middle nowrap><FONT color=\"'+l.fg+'\">');
	if (d && d != 'Invalid Date')
		l.HidLayer.document.write(dt_formatdate(l, d, 0));
	l.HidLayer.document.write('</FONT></TD></TR></TABLE>');
	l.HidLayer.document.close();
	l.HidLayer.visibility = 'inherit';
	l.VisLayer.visibility = 'hide';
	var t = l.VisLayer;
	l.VisLayer = l.HidLayer;
	l.HidLayer = t;
}

function dt_drawmonth(l, d) {
	var dy=0,r=-1;
	var TmpDate = new Date(d);
	TmpDate.setDate(1);
	var col=TmpDate.getDay();
	var num=htutil_days_in_month(TmpDate);
	while (l.VisLayer.Areas.length) {
		pg_removearea(l.VisLayer.Areas[0]);
		l.VisLayer.Areas.shift();
	}

	var v='<TABLE width=175 height=100 border=0 cellpadding=0 cellspacing=0>';
	for (var i=0; i<35; i++) {
		if (i!=0 && i%7==0) v+='</TR>';
		if (i%7==0) {v+='<TR>\n';r++}
		v+='<TD width=25 height=20 valign=middle align=right>';
		if (i>=col && dy<num) {
			l.HidLayer.Areas[dy]=pg_addarea(l.HidLayer, 1+((i%7)*25), (1+(r*20)), 25, 20, 'dt', 'dt', 0);
			var q=new Date(TmpDate);
			q.setDate(dy+1);
			l.HidLayer.Areas[dy].DateVal = q;
			v += ''+(++dy);
			col++;
		}
		v+='</TD>';
	}
	for (;i <= 31; i++) {  }
	v+='</TR></TABLE>';
	l.HidLayer.document.write(v);
	l.HidLayer.document.close();
	l.HidLayer.visibility = 'inherit';
	l.VisLayer.visibility = 'hide';
	var t = l.VisLayer;
	l.VisLayer = l.HidLayer;
	l.HidLayer = t;

	l.MonHidLayer.document.write('<TABLE border=0 cellspacing=0 cellpadding=0 height=22 width=112><TR><TD align=center valign=middle>'+l.ml.MonthsAbbrev[TmpDate.getMonth()]+', '+(TmpDate.getYear()+1900)+'</TD></TR></TABLE>');
	l.MonHidLayer.document.close();
	l.MonHidLayer.visibility = 'inherit';
	l.MonVisLayer.visibility = 'hide';
	t = l.MonVisLayer;
	l.MonVisLayer = l.MonHidLayer;
	l.MonHidLayer = t;
}

function dt_tag_images(d, t, l) {
	for (i=0; i < d.images.length; i++) {
		d.images[i].kind = t;
		d.images[i].layer = l;
	}
}

function dt_toggle(l) {
	for (i=0; i<l.document.images.length;i++) {
		if (i == 4)
			continue;
		else if (l.document.images[i].src.substr(-14, 6) == 'dkgrey')
			l.document.images[i].src = '/sys/images/white_1x1.png';
		else
			l.document.images[i].src = '/sys/images/dkgrey_1x1.png';
	}
}

function dt_getfocus(a,b,c,d,e,f) {
	dt_current.DateObj = new Date(f.DateVal);
	dt_drawdate(dt_current, f.DateVal);
	dt_current.PaneLayer.visibility = 'hide';
	if (dt_current.form) {
		dt_current.form.FocusNotify(dt_current);
		dt_current.form.DataNotify(dt_current);
	}
	dt_current = null;
}

function dt_losefocus() {
	return true;
}

function dt_create_pane(ml,bg,w,h,h2) {
	l = new Layer(1024);
	l.document.write("<BODY "+bg+">");
	l.document.write("<TABLE border=0 cellpadding=0 cellspacing=0 width="+w+" height="+h+">");
	l.document.write("<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>");
	l.document.write("	<TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(w-2)+"></TD>");
	l.document.write("	<TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>");
	l.document.write("<TR><TD><IMG SRC=/sys/images/white_1x1.png height="+(h-2)+" width=1></TD>");
	l.document.write("	<TD valign=top>");
	l.document.write("	<TABLE height=25 cellpadding=0 cellspacing=0 border=0>");
	l.document.write("	<TR><TD width=18><IMG SRC=/sys/images/ico16aa.gif></TD>");
	l.document.write("		<TD width=18><IMG SRC=/sys/images/ico16ba.gif></TD>");
	l.document.write("		<TD width="+(w-72)+"></TD>");
	l.document.write("		<TD width=18><IMG SRC=/sys/images/ico16ca.gif></TD>");
	l.document.write("		<TD width=18><IMG SRC=/sys/images/ico16da.gif></TD></TR>");
	l.document.write("	</TABLE>");
	l.document.write("	<TABLE width="+w+" cellpadding=0 cellspacing=0 border=0>");
	l.document.write("	<TR><TD align=center><B>S</B></TD>");
	l.document.write("		<TD align=center><B>M</B></TD>");
	l.document.write("		<TD align=center><B>T</B></TD>");
	l.document.write("		<TD align=center><B>W</B></TD>");
	l.document.write("		<TD align=center><B>T</B></TD>");
	l.document.write("		<TD align=center><B>F</B></TD>");
	l.document.write("		<TD align=center><B>S</B></TD></TR>");
	l.document.write("	</TABLE>");
	l.document.write("	</TD>");
	l.document.write("	<TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(h-2)+" width=1></TD></TR>");
	l.document.write("<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>");
	l.document.write("	<TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(w-2)+"></TD>");
	l.document.write("	<TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>");
	l.document.write("</TABLE>");
	l.document.write("</BODY>");
	l.document.close();
	l.HidLayer = new Layer(1024, l);
	l.VisLayer = new Layer(1024, l);
	l.MonHidLayer = new Layer(1024, l);
	l.MonVisLayer = new Layer(1024, l);
	l.HidLayer.y = l.VisLayer.y = 48;
	l.MonHidLayer.x = l.MonVisLayer.x = 38;
	l.MonHidLayer.y = l.MonVisLayer.y = 2;
	l.x = ml.pageX;
	l.y = ml.pageY+h2;
	l.kind = l.HidLayer.kind = l.VisLayer.kind = l.MonHidLayer.kind = l.MonVisLayer.kind = 'dt_pn';
	l.document.layer = l.HidLayer.document.layer = l.VisLayer.document.layer = l;
	l.MonVisLayer.document.layer = l.MonHidLayer.document.layer = l;
	dt_tag_images(l.document, 'dt_pn', l);
	l.document.images[4].kind = 'dtimg_yrdn';
	l.document.images[5].kind = 'dtimg_mndn';
	l.document.images[6].kind = 'dtimg_mnup';
	l.document.images[7].kind = 'dtimg_yrup';
	return l;
}

/*  Event Functions  */
function dt_mousedown(l) {
	p = l;
	if (p.kind == 'dt_day') {
		dt_drawdate(p.ml, p.DateVal);
		p = l.ml; 
	} else if (p.kind == 'dt') {
		dt_toggle(p);
	}
	if (p.kind == 'dt' || p.kind == 'dt_day') {
		if (dt_current) {
			dt_current = null;
			p.PaneLayer.visibility = 'hide';
		} else {
			dt_current = p;
			p.PaneLayer.x = p.pageX;
			p.PaneLayer.y = p.pageY+p.h;
			p.PaneLayer.visibility = 'inherit';
		}
	}
}

function dt_mouseup(l) {
	if (l.kind == 'dt') {
		dt_toggle(l);
	}
}

function dt_yrdn() {
	dt_current.TmpDateObj.setYear(dt_current.TmpDateObj.getYear()+1899);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
}

function dt_yrup() {
	dt_current.TmpDateObj.setYear(dt_current.TmpDateObj.getYear()+1901);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
}

function dt_mndn() {
	dt_current.TmpDateObj.setMonth(dt_current.TmpDateObj.getMonth()-1);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
}

function dt_mnup() {
	dt_current.TmpDateObj.setMonth(dt_current.TmpDateObj.getMonth()+1);
	dt_drawmonth(dt_current.PaneLayer, dt_current.TmpDateObj);
}
