/*
**  htdrv_datetime.js
**  Function declarations for the Date/Time htmlgen driver
*/

/*  Form interaction functions */

function dt_getvalue() {
	return dt_formatdate(this, this.wdate, 0);
	return str;
}

function dt_setvalue(v) {
	this.wdate = new Date(v);
	this.tmpdate = new Date(v);
	if (this.wdate == 'Invalid Date') {
		this.wdate = new Date();
		this.tmpdate = new Date();
	}
	dt_drawdate(this.lbdy, this.tmpdate);
	dt_drawmonth(this.ld, this.tmpdate);
	dt_drawtime(this, this.tmpdate);
}

function dt_clearvalue() {
	this.wdate = new Date('');
	this.tmpdate = new Date('');
	dt_drawdate(this.lbdy, '');
}

function dt_resetvalue() {
	this.wdate = new Date(this.initialdateStr);
	this.tmpdate = new Date(this.initialdateStr);
	dt_drawdate(this.lbdy, this.tmpdate);
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


/*  Functions for Internal Use */

function dt_init(l,lbg1,lbg2,lbdy,limg,l2,w,h,dt,bgcolor,fgcolor,fn) {
	this.enabled = 'full';
	l.fieldname = fn;
	l.setvalue = dt_setvalue;
	l.getvalue = dt_getvalue;
	l.enable = dt_enable;
	l.readonly = dt_readonly;
	l.disable = dt_disable;
	l.clearvalue = dt_clearvalue;
	l.resetvalue = dt_resetvalue;
	l.form = fm_current;
	l.kind = 'dt';
	lbg1.kind = 'dt';
	lbg2.kind = 'dt';
	lbdy.kind = 'dt';
	limg.kind = 'dt';
	l2.kind = 'dt';
	limg.document.images[0].kind = 'dt';
	limg.document.images[0].subkind = 'dt_button';
	limg.document.images[0].layer = l;
	lbdy.subkind = 'dt_button';
	lbg1.subkind = 'dt_button';
	lbg2.subkind = 'dt_button';
	lbdy.subkind = 'dt_button';
	limg.subkind = 'dt_button';
	l2.subkind = 'dt_dropdown';
	l.document.layer = l;
	lbg1.document.layer = l;
	lbg2.document.layer = l;
	lbdy.document.layer = l;
	limg.document.layer = l;
	l2.document.layer = l2;
	l2.document.mainlayer = l;
	l.initialdateStr = dt;
	if (dt) l.wdate = new Date(dt);
	else l.wdate = new Date();
	l.tmpdate = new Date(l.wdate);
	l.std_w = 182;
	l.std_h = 190;
	l.w = w;
	l.h = h;
	l.ld = l2;
	l.lbg1 = lbg1;
	l.lbg2 = lbg2;
	l.lbdy = lbdy;
	l.limg = limg;
	l.colorBG = bgcolor;
	l.colorFG = fgcolor;
	l.mode = 0;
	l.ld.contentLayer = new Layer(1024, l.ld);
	l.ld.contentLayer.visibility = 'inherit';
	l.ld.hiddenLayer = new Layer(1024, l.ld);
	l.ld.hiddenLayer.visibility = 'hide';
	l.strMonthsAbbrev = Array('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');
	l.strDaysAbbrev = Array('Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat');
	l.ld.contentLayer.dayLayersArray = Array();
	l.ld.hiddenLayer.dayLayersArray = Array();
	l.vl = l.ld.contentLayer; // visible layer
	l.hl = l.ld.hiddenLayer; // hidden layer
	l.vl.clip.height=500; l.vl.clip.width=500;
	l.hl.clip.height=500; l.hl.clip.width=500;
	l.ld.tl_hr = new Layer(1024, l.ld); // hour display layer
	l.ld.tl_hr.x = 3; l.ld.tl_hr.y = 168;
	l.ld.tl_hr.clip.width = 70; l.ld.tl_hr.clip.height=20;
	l.ld.tl_hr.visibility = 'inherit';
	l.ld.tl_mn = new Layer(1024, l.ld); // minute display layer
	l.ld.tl_mn.x = 76; l.ld.tl_mn.y = 168;
	l.ld.tl_mn.clip.width = 70; l.ld.tl_mn.clip.height=20;
	l.ld.tl_mn.visibility = 'inherit';
	for (var i=0; i < 31; i++) {
		l.ld.contentLayer.dayLayersArray[i] = new Layer(128, l.ld.contentLayer);
		l.ld.contentLayer.dayLayersArray[i].losefocushandler = dt_losefocus;
		l.ld.contentLayer.dayLayersArray[i].document.mainlayer = l;
		l.ld.contentLayer.dayLayersArray[i].document.layer = l.ld.contentLayer.dayLayersArray[i];
		l.ld.contentLayer.dayLayersArray[i].kind = 'dt';
		l.ld.contentLayer.dayLayersArray[i].visibility = 'inherit';
		l.ld.contentLayer.dayLayersArray[i].subkind = 'dt_day';
		l.ld.hiddenLayer.dayLayersArray[i] = new Layer(128, l.ld.hiddenLayer);
		l.ld.hiddenLayer.dayLayersArray[i].losefocushandler = dt_losefocus;
		l.ld.hiddenLayer.dayLayersArray[i].document.mainlayer = l;
		l.ld.hiddenLayer.dayLayersArray[i].document.layer = l.ld.hiddenLayer.dayLayersArray[i];
		l.ld.hiddenLayer.dayLayersArray[i].kind = 'dt';
		l.ld.hiddenLayer.dayLayersArray[i].visibility = 'inherit';
		l.ld.hiddenLayer.dayLayersArray[i].subkind = 'dt_day';
	}
	l.ld.hdr = new Layer(1024, l.ld);
	l.ld.hdr1 = new Layer(1024, l.ld);
	l.ld.hdr2 = new Layer(1024, l.ld);
	l.ld.hdr3 = new Layer(1024, l.ld);
	l.ld.hdr4 = new Layer(1024, l.ld);
	dt_drawmonth(l.ld, l.tmpdate);
	if (l.initialdateStr) dt_drawdate(l.lbdy, l.tmpdate);
	dt_drawtime(l, l.tmpdate);
	if (fm_current) fm_current.Register(l);
}

function dt_formatdate(l, d, fmt) {
	var str;
	switch (fmt) {
		case 0:
		default:
			str  = l.strMonthsAbbrev[d.getMonth()] + ' ';
			str += d.getDate() + ', ';
			str += d.getYear()+1900 + ', ';
			str += dt_strpad(d.getHours(), '0', 2)+':';
			str += dt_strpad(d.getMinutes(), '0', 2);
			break;
	}
	return str;
}

function dt_toggledd(l) {
	if (l.ld.visibility == 'hide')
		l.ld.visibility = 'show';
	else
		l.ld.visibility = 'hide';
}

function dt_setmode(layer,mode) {
	layer.mode = mode;
	if (mode == 0) { /* CLICK */ 
		layer.lbg1.bgColor = '#7a7a7a';
		layer.lbg2.bgColor = '#ffffff';
		layer.lbdy.moveBy(1,1);
		layer.limg.moveBy(1,1);
		layer.lbdy.clip.width -= 1;
		layer.lbdy.clip.height -= 1;
	} else if (mode == 1) { /* UNCLICK */
		layer.lbg1.bgColor = '#ffffff';
		layer.lbg2.bgColor = '#7a7a7a';
		layer.lbdy.moveBy(-1,-1);
		layer.limg.moveBy(-1,-1);
		layer.lbdy.clip.width += 1;
		layer.lbdy.clip.height += 1;
	}
}

function dt_drawmonth(l, d) {
	l.hdr.x = 28; l.hdr.y = 2;
	l.hdr.visibility = 'inherit';
	l.hdr.document.write('<table '+l.document.mainlayer.colorBG+' width=124 height=20 cellpadding=0 cellspacing=0 border=0><tr><td align=center valign=middle><b>'+l.document.mainlayer.strMonthsAbbrev[d.getMonth()]+' '+(d.getYear()+1900)+'</b></td></tr></table>');
	l.hdr.document.layer = l;
	l.hdr.document.kind = 'dt';
	l.hdr.document.subkind = 'dt_dropdown';
	l.hdr.document.close();

	l.hdr1.x = 2; l.hdr.y = 2;
	l.hdr1.visibility = 'inherit';
	l.hdr1.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&lt;&lt;</B></td></tr></table>');
	l.hdr1.document.mainlayer = l.document.mainlayer;
	l.hdr1.document.kind = 'dt';
	l.hdr1.document.subkind = 'dt_yearprev';
	l.hdr1.document.close();
	l.hdr1.losefocushandler = dt_losefocus;

	l.hdr2.x = 27; l.hdr.y = 2;
	l.hdr2.visibility = 'inherit';
	l.hdr2.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&lt;</B></td></tr></table>');
	l.hdr2.document.mainlayer = l.document.mainlayer;
	l.hdr2.document.kind = 'dt';
	l.hdr2.document.subkind = 'dt_monthprev';
	l.hdr2.document.close();
	l.hdr2.losefocushandler = dt_losefocus;

	l.hdr3.x = l.document.mainlayer.std_w-52; l.hdr.y = 2;
	l.hdr3.visibility = 'inherit';
	l.hdr3.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&gt;</B></td></tr></table>');
	l.hdr3.document.mainlayer = l.document.mainlayer;
	l.hdr3.document.kind = 'dt';
	l.hdr3.document.subkind = 'dt_monthnext';
	l.hdr3.document.close();
	l.hdr3.losefocushandler = dt_losefocus;

	l.hdr4.x = l.document.mainlayer.std_w-27; l.hdr.y = 2;
	l.hdr4.visibility = 'inherit';
	l.hdr4.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&gt;&gt;</B></td></tr></table>');
	l.hdr4.document.mainlayer = l.document.mainlayer;
	l.hdr4.document.kind = 'dt';
	l.hdr4.document.subkind = 'dt_yearnext';
	l.hdr4.document.close();
	l.hdr4.losefocushandler = dt_losefocus;

	l.day = new Layer(1024, l);
	l.day.x = 1; l.day.y = 21;
	l.day.visibility = 'inherit';
	l.day.document.write('<table width=181 height=20 cellpadding=0 cellspacing=0 border=0><tr>');
	l.day.document.write('<td align=center valign=middle width=25><b>S</b></td>');
	l.day.document.write('<td align=center valign=middle width=25><b>M</b></td>');
	l.day.document.write('<td align=center valign=middle width=25><b>T</b></td>');
	l.day.document.write('<td align=center valign=middle width=25><b>W</b></td>');
	l.day.document.write('<td align=center valign=middle width=25><b>T</b></td>');
	l.day.document.write('<td align=center valign=middle width=25><b>F</b></td>');
	l.day.document.write('<td align=center valign=middle width=25><b>S</b></td>');
	l.day.document.write('</tr></table>');
	l.day.document.layer = l;
	l.day.kind = 'dt';
	l.day.subkind = 'dt_dropdown';
	l.day.document.close();
	var row=0;
	var tmpDate = new Date(d);
	tmpDate.setDate(1);
	var col=tmpDate.getDay();
	for (var i=1; i <= dt_getdaysinmonth(tmpDate); i++) {
		if (col!=0 && col%7==0) { row++; col=0; }
		dt_drawday(l.document.mainlayer.ld.hiddenLayer.dayLayersArray[i-1], i, col, row);
		col++;
	}
	for (;i <= 31; i++) { l.document.mainlayer.ld.hiddenLayer.dayLayersArray[i-1].visibility = 'hide'; }
	l.document.mainlayer.ld.hiddenLayer.visibility = 'inherit';
	l.document.mainlayer.ld.contentLayer.visibility = 'hidden';
	tmp = l.document.mainlayer.ld.contentLayer;
	l.document.mainlayer.ld.contentLayer = l.document.mainlayer.ld.hiddenLayer;
	l.document.mainlayer.ld.hiddenLayer = tmp;
}

function dt_drawday(l, dy, col, row) {
	l.dateVal = new Date(l.document.mainlayer.tmpdate);
	l.dateVal.setDate(dy);
	l.x = 1+(col*26);
	l.y = 42+(row*20);
	l.clip.width=25; l.clip.height=20;
	l.visibility = 'inherit';
	l.document.write('<TABLE border=0 cellspacing=0 cellpadding=0 width=25 height=20');
	l.document.write('><TR><TD align=center valign=middle>'+dy+'</TD></TR></TABLE>');
	l.document.close();
	pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'dt', 'dt', 0);
}

function dt_losefocus() {
	return true;
}

function dt_drawdate(l, d) {
	l.document.write('<TABLE border=0 cellspacing=0 cellpadding=0 '+l.document.layer.colorBG+' width='+(l.document.layer.w-20)+' height='+(l.document.layer.h-2)+'>');
	l.document.write('<TR><TD align=center valign=middle nowrap><FONT color=\"'+l.document.layer.colorFG+'\">');
	if (d && d != 'Invalid Date')
		l.document.write(dt_formatdate(l.document.layer, d, 0));
	l.document.write('</FONT></TD></TR></TABLE>');
	l.document.close();
}

function dt_drawtime(l, d) {
	lhr = l.ld.tl_hr;
	lmn = l.ld.tl_mn;
	lhr.document.write('<table width=70 height=20 cellpadding=0 cellspacing=0 border=0><tr><td align=right width=58>');
	lmn.document.write(dt_strpad(l.wdate.getHours(), '0', 2));
	lhr.document.write('&nbsp;</td><td width=12>');
	lhr.document.write('<table cellpadding=0 cellspacing=0 border=0 width=12><tr><td valign=middle>');
	lhr.document.write('<img src=/sys/images/spnr_up.gif></td></tr><tr><td>');
	lhr.document.write('<img src=/sys/images/spnr_down.gif>');
	lhr.document.write('</td></tr></table>');
	lhr.document.write('</td></tr></table>');
	lhr.document.images[0].kind = 'dt';
	lhr.document.images[0].subkind = 'dt_hour_up';
	lhr.document.images[0].mainlayer = l;
	lhr.document.images[1].kind = 'dt';
	lhr.document.images[1].subkind = 'dt_hour_down';
	lhr.document.images[1].mainlayer = l;
	lhr.document.close();
	lmn.document.write('<table width=70 height=20 cellpadding=0 cellspacing=0 border=0><tr><td align=right width=58>');
	lmn.document.write(dt_strpad(l.wdate.getMinutes(), '0', 2));
	lmn.document.write('&nbsp;</td><td width=12>');
	lmn.document.write('<table cellpadding=0 cellspacing=0 border=0 width=12><tr><td valign=middle>');
	lmn.document.write('<img src=/sys/images/spnr_up.gif></td></tr><tr><td>');
	lmn.document.write('<img src=/sys/images/spnr_down.gif>');
	lmn.document.write('</td></tr></table>');
	lmn.document.write('</td></tr></table>');
	lmn.document.images[0].kind = 'dt';
	lmn.document.images[0].subkind = 'dt_min_up';
	lmn.document.images[0].mainlayer = l;
	lmn.document.images[1].kind = 'dt';
	lmn.document.images[1].subkind = 'dt_min_down';
	lmn.document.images[1].mainlayer = l;
	lmn.document.close();
}

/**
 **  Here is the leap year algorithm used.  To the best of my knowledge
 **  it's the best way to detect leap years.  If I am wrong, please
 **  correct me.  - LME (July 2002)
 **
 **	IF year is divisible by 4, it is a leap year
 **	EXCEPT if a year is divisible by 100, it is not a leap year
 **	EXCEPT if a year is divisible by 400, then it IS a leap year.
 **/
function dt_isleapyear(d) {
	var yr = d.getYear()+1900;
	if (yr % 4 == 0) {
		if (yr % 100 == 0 && yr % 400 != 0) {
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

function dt_getdaysinmonth(d) {
	switch (d.getMonth()) {
		case 0: return 31;
		case 1: return (dt_isleapyear(d)?29:28);
		case 2: return 31;
		case 3: return 30;
		case 4: return 31;
		case 5: return 30;
		case 6: return 31;
		case 7: return 31;
		case 8: return 30;
		case 9: return 31;
		case 10: return 30;
		case 11: return 31;
	}
}


/* Event Handler Functions */

function dt_event_mousedown1(targetLayer) {
	if (targetLayer.subkind == 'dt_day') {
		var ml = targetLayer.document.mainlayer;
		var td = new Date(targetLayer.dateVal);
		td.setHours(ml.wdate.getHours());
		td.setMinutes(ml.wdate.getMinutes());
		ml.wdate = new Date(td);
		dt_drawdate(ml.lbdy, ml.wdate);
		if (dt_current.form)
			dt_current.form.DataNotify(dt_current);
		dt_current.document.layer.ld.visibility = 'hide';
		dt_current = null;
	} else if (targetLayer.subkind == 'dt_yearprev') {
		var ml = targetLayer.mainlayer;
		ml.tmpdate.setYear(ml.tmpdate.getYear()+1899);
		dt_drawmonth(ml.ld, ml.tmpdate);
	} else if (targetLayer.subkind == 'dt_yearnext') {
		var ml = targetLayer.mainlayer;
		ml.tmpdate.setYear(ml.tmpdate.getYear()+1901);
		dt_drawmonth(ml.ld, ml.tmpdate);
	} else if (targetLayer.subkind == 'dt_monthnext') {
		var ml = targetLayer.mainlayer;
		ml.tmpdate.setMonth(ml.tmpdate.getMonth()+1);
		dt_drawmonth(ml.ld, ml.tmpdate);
	} else if (targetLayer.subkind == 'dt_monthprev') {
		var ml = targetLayer.mainlayer;
		ml.tmpdate.setMonth(ml.tmpdate.getMonth()-1);
		dt_drawmonth(ml.ld, ml.tmpdate);
	} else if (targetLayer.subkind == 'dt_hour_up') {
		var d = targetLayer.mainlayer.wdate;
		if (d.getHours() < 23) {
			targetLayer.mainlayer.tmpdate = new Date(d);
			d.setHours(d.getHours()+1);
			dt_drawtime(targetLayer.mainlayer, d);
			dt_drawdate(targetLayer.mainlayer.lbdy, d);
		}
		if (targetLayer.mainlayer.form)
			targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);
	} else if (targetLayer.subkind == 'dt_hour_down') {
		var d = targetLayer.mainlayer.wdate;
		if (d.getHours() > 0) {
			targetLayer.mainlayer.tmpdate = new Date(d);
			d.setHours(d.getHours()-1);
			dt_drawtime(targetLayer.mainlayer, d);
			dt_drawdate(targetLayer.mainlayer.lbdy, d);
		}
		if (targetLayer.mainlayer.form)
			targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);
	} else if (targetLayer.subkind == 'dt_min_up') {
		var d = targetLayer.mainlayer.wdate;
		if (d.getMinutes() < 59) {
			targetLayer.mainlayer.tmpdate = new Date(d);
			d.setMinutes(d.getMinutes()+1);
			dt_drawtime(targetLayer.mainlayer, d);
			dt_drawdate(targetLayer.mainlayer.lbdy, d);
		}
		if (targetLayer.mainlayer.form)
			targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);
	} else if (targetLayer.subkind == 'dt_min_down') {
		var d = targetLayer.mainlayer.wdate;
		if (d.getMinutes() > 0) {
			targetLayer.mainlayer.tmpdate = new Date(d);
			d.setMinutes(d.getMinutes()-1);
			dt_drawtime(targetLayer.mainlayer, d);
			dt_drawdate(targetLayer.mainlayer.lbdy, d);
		}
		if (targetLayer.mainlayer.form)
			targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);
	}
	else {
		dt_current.document.layer.ld.visibility = 'hide';
		dt_current = null;
	}
}

function dt_event_mousedown2(targetLayer) {
	if (targetLayer.subkind != 'dt_dropdown') {
		if (targetLayer.form)
			targetLayer.form.FocusNotify(targetLayer);
		dt_setmode(targetLayer,0);
		targetLayer.ld.visibility = 'inherit';
		dt_current = targetLayer;
	}
}

