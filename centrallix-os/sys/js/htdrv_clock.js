function cl_init(l,c1,c2,f,bg,s,fg1,fg2,fs,m,b,sox,soy,sf,apf,mt) {
	c1.document.layer = c1;
	c2.document.layer = c2;
	c1.mainlayer=l;
	c2.mainlayer=l;
	l.mainlayer=l;
	c1.kind='cl';
	c2.kind='cl';
	l.kind='cl';
	l.contentLayer = c1;
	l.hiddenLayer = c2;
	l.shadowed = s;
	l.moveable = m;
	l.bold = b;
	l.fgColor1 = fg1;
	l.fgColor2 = fg2;
	l.fontSize = fs;
	l.showSecs = sf;
	l.showAmPm = apf;
	l.milTime = mt;
	if (s==1) {
		l.contentLayer.shadowLayer = new Layer(l.document.width-sox,l);
		l.contentLayer.shadowLayer.moveTo(sox,soy);
		l.contentLayer.shadowLayer.zIndex = l.contentLayer.zIndex-1;
		l.contentLayer.shadowLayer.visibility = 'inherit';
		l.hiddenLayer.shadowLayer = new Layer(l.document.width-sox,l);
		l.hiddenLayer.shadowLayer.moveTo(sox,soy);
		l.hiddenLayer.shadowLayer.zIndex = l.hiddenLayer.zIndex-1;
		l.hiddenLayer.shadowLayer.visibility = 'hidden';
	}
	cl_update_time(l);
	return l;
}

function cl_get_time(l) {
	var t = new Date();
	var time = new Object();
	time.hrs = t.getHours();
	time.mins = t.getMinutes();
	time.secs = t.getSeconds();
	time.msecs = t.getMilliseconds();
	time.formated = cl_format_time(l,time);
	return time;
}

function cl_update_time(l) {
	var time = cl_get_time(l);
	l.hiddenLayer.document.write("<table width="+l.document.width+" height="+l.document.height+" border=0><tr><td valign='center' align='center' nowrap><font face='fixed' size='+"+l.fontSize+"' color='"+l.fgColor1+"'>"+((l.bold)?'<b>':'')+time.formated+((l.bold)?'</b>':'')+"</font></td></tr></table>");
	l.hiddenLayer.document.close();
	if (l.shadowed) {
		l.hiddenLayer.shadowLayer.document.write("<table width="+l.document.width+" height="+l.document.height+" border=0><tr><td valign='center' align='center' nowrap><font face='fixed' size='+"+l.fontSize+"' color='"+l.fgColor2+"'>"+((l.bold)?'<b>':'')+time.formated+((l.bold)?'</b>':'')+"</font></td></tr></table>");
		l.hiddenLayer.shadowLayer.document.close();
		l.contentLayer.shadowLayer.visibility = 'hidden';
		l.hiddenLayer.shadowLayer.visibility = 'inherit';
	}
	l.contentLayer.visibility = 'hidden';
	l.hiddenLayer.visibility = 'inherit';
	var tmp = l.contentLayer;
	l.contentLayer = l.hiddenLayer;
	l.hiddenLayer = tmp;
	if (l.shadowed) {
		l.hiddenLayer = tmp;
		tmp = l.contentLayer.shadowLayer;
		l.contentLayer.shadowLayer = l.hiddenLayer.shadowLayer;
		l.hiddenLayer.shadowLayer = tmp;
	}
	if (l.showSecs) setTimeout(cl_update_time,1000-time.msecs,l);
	else setTimeout(cl_update_time,(60-time.secs)*1000-time.msecs,l);
}

function cl_format_time(l,t) {
	if (t.mins<10) t.mins = htutil_strpad(t.mins,0,2);
	if (t.secs<10) t.secs = htutil_strpad(t.secs,0,2);
	if (t.hrs >= 12 && !l.milTime) {
		t.ampm = "pm";
		if (t.hrs > 12) t.hrs = t.hrs - 12;
	}
	else t.ampm = "am";
	var timef = t.hrs+":"+t.mins+((l.showSecs)?":"+t.secs:"")+((l.showAmPm)?(" "+t.ampm):"");
	return timef;
}
