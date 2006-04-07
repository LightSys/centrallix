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

function cl_init(param){
	var l = param.layer;	
	var c1 = param.c1;
	var c2 = param.c2;
	var f = param.fieldname;
	var bg = param.background;

	//alert(getPageX(l) + "," + getPageY(l));
	//alert(l.id);
	htr_init_layer(l,l,'cl');
	htr_init_layer(c1,l,'cl');
	htr_init_layer(c2,l,'cl');
        l.contentLayer = c1;
        l.hiddenLayer = c2;
        l.shadowed = param.shadowed;
        l.moveable = param.moveable;
        l.bold = param.bold;
        l.fgColor1 = param.foreground1;
        l.fgColor2 = param.foreground2;
        l.fontSize = param.fontsize;
        l.showSecs = param.showSecs;
        l.showAmPm = param.showAmPm;
        l.milTime = param.milTime;
        if (param.shadowed==1) {
		l.contentLayer.shadowLayer = htr_new_layer(getClipWidth(l)-param.sox,l);
		moveTo(l.contentLayer.shadowLayer, param.sox, param.soy);
		htr_setzindex(l.contentLayer.shadowLayer, htr_getzindex(l.contentLayer)-1);
		htr_setvisibility(l.contentLayer.shadowLayer, 'inherit');
                l.hiddenLayer.shadowLayer = htr_new_layer(getClipWidth(l)-param.sox,l);
                moveTo(l.hiddenLayer.shadowLayer, param.sox, param.soy);
                htr_setzindex(l.hiddenLayer.shadowLayer, htr_getzindex(l.hiddenLayer)-1);
                htr_setvisibility(l.hiddenLayer.shadowLayer, 'hidden');
        }
        cl_update_time(l);

	// Events
	ifc_init_widget(l);
	var ie = l.ifcProbeAdd(ifEvent);
	ie.Add("MouseUp");
	ie.Add("MouseDown");
	ie.Add("MouseOver");
	ie.Add("MouseMove");
	ie.Add("MouseOut");

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
	var str = "<table width="+getClipWidth(l)+" height="+getClipHeight(l)+" border=0><tr><td valign='center' align='center' nowrap><font size='+"+l.fontSize+"' color='"+l.fgColor1+"'><pre style='margin:0px; padding:0px;'>"+((l.bold)?'<b>':'')+time.formated+((l.bold)?'</b>':'')+"</pre></font></td></tr></table>";
	htr_write_content(l.hiddenLayer, str);
	if (l.shadowed) {
		str = "<table width="+getClipWidth(l)+" height="+getClipHeight(l)+" border=0><tr><td valign='center' align='center' nowrap><font size='+"+l.fontSize+"' color='"+l.fgColor2+"'><pre style='margin:0px; padding:0px;'>"+((l.bold)?'<b>':'')+time.formated+((l.bold)?'</b>':'')+"</pre></font></td></tr></table>";
		htr_write_content(l.hiddenLayer.shadowLayer, str);
		htr_setvisibility(l.contentLayer.shadowLayer, 'hidden');
		htr_setvisibility(l.hiddenLayer.shadowLayer, 'inherit');
	}
	htr_setvisibility(l.contentLayer, 'hidden');
	htr_setvisibility(l.hiddenLayer, 'inherit');
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
	if (t.hrs == 0 && !l.milTime) 
	    t.hrs += 12;
	if (l.miltime) 
	    t.hrs = htutil_strpad(t.hrs, '0', 2);
	else
	    t.hrs = htutil_strpad(t.hrs, ' ', 2);
	var timef = t.hrs+":"+t.mins+((l.showSecs)?":"+t.secs:"")+((l.showAmPm)?(" "+t.ampm):"");
	return timef;
}
