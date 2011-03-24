// Copyright (C) 1998-2003 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function ca_keyhandler(l,e,k)
    {
    return true;
    }

function ca_getfocus(x,y,l,c,n)
    {
    return 0;
    }

function ca_losefocus()
    {
    return true;
    }


function ca_lookup_field(rec, fld)
    {
    for(var i=0;i<rec.length;i++)
	if (rec[i] && rec[i].oid == fld) return rec[i].value;
    return null;
    }


function ca_redraw_year_start_year(yr)
    {
    var txt = "<table border=0 cellspacing=2 cellpadding=2 width=100%><tr><td " + this.cell_bg + " align=center><font size=+4><b>" + yr + "</b></font></td></tr></table>";
    return txt;
    }

function ca_redraw_year_close_year(yr)
    {
    var txt = "<br>";
    return txt;
    }

function ca_redraw_year_start_month(mo)
    {
    var month_names = new Array("January","February","March","April","May","June","July","August","September","October","November","December");
    var txt = "";
    if (mo%3 == 0) txt += "<table border=0 cellspacing=2 cellpadding=2 width=100% cols=3><tr>";
    txt += "<td colspan=1 " + this.cell_bg + " valign=top align=center>" + month_names[mo] + "<br>";
    return txt;
    }

function ca_redraw_year_close_month(mo)
    {
    var txt = "</td>";
    if (mo%3 == 2) txt += "</tr></table>";
    return txt;
    }

function ca_redraw_year_start_day(d,wd,ld,w)
    {
    var txt = "";
    if (d==0) txt += "<table cols=7 border=0 cellspacing=0 cellpadding=0 width=" + w + ">";
    if (d==0 || wd==0) txt += "<tr>";
    if (d==0) for (var i=0; i<wd; i++) txt += "<td>&nbsp;</td>";
    txt += "<td align=center>";
    //if (d==0) 
    //	{
    //	this.sublayer.txt += "<pre>";
    //	for(var i=0; i<wd; i++) this.sublayer.txt += "   ";
    //	}
    return txt;
    }

function ca_redraw_year_close_day(d,wd,ld)
    {
    var txt = "</td>";
    if (d==ld) for(var i=wd; i<6;i++) txt += "<td>&nbsp;</td>";
    if (d==ld || wd==6) txt += "</tr>";
    if (d==ld) txt += "</table>";
    //if (wd != 6) this.sublayer.txt += " ";
    //if (d==ld) this.sublayer.txt += "</pre>";
    //if (wd==6) this.sublayer.txt += "<br>";
    return txt;
    }


function ca_redraw_year_doyearlayer(yr)
    {
    var ltxt = this.RedrawYearStartYear(yr);
    return ltxt;
    }


function ca_redraw_year_domonthlayer(yr,mo)
    {
    var curdate = new Date();
    var curmonth,curday;
    var ltxt = "";
    var mdays = Array(31,28,31, 30,31,30, 31,31,30, 31,30,31);
    var w = Math.floor((this.clip.width - 20)/3);
    var cdstr = "";
    var ld;
    var r,rec;
    var nitems;
    
    curdate.setYear(yr);
    curdate.setSeconds(59);
    curdate.setHours(23);
    curdate.setMinutes(59);
    for(curmonth = mo; curmonth <= mo + 2; curmonth++)
	{
	curdate.setMonth(curmonth,1);
	ltxt += this.RedrawYearStartMonth(curmonth);
	ld = mdays[curmonth];
	if (yr%4 == 0 && yr%400 != 0 && curmonth == 1) ld = 29;
	for(curday = 0; curday < ld; curday++)
	    {
	    curdate.setDate(curday + 1);
	    //if (curyear == 101 && curmonth == 3 && curday == 0) alert("date is " + curdate.toString());
	    ltxt += this.RedrawYearStartDay(curday, curdate.getDay(), ld-1, w);
	    nitems=0;
	    while(this.rec)
		{
		if (this.evdate <= curdate)
		    {
		    nitems++;
		    this.r++;
		    if (this.r <= this.osrc.LastRecord) this.rec = this.osrc.replica[this.r]; else this.rec = null;
		    if (this.rec) this.evdate = new Date(ca_lookup_field(this.rec,this.eventdatefield));
		    }
		else
		    {
		    break;
		    }
		}
	    if (curday+1 < 10) cdstr = " " + (curday+1); else cdstr = "" + (curday+1);
	    if (nitems)
		ltxt += "<small><b>" + cdstr + "</b></small>";
	    else
		ltxt += "<small>" + cdstr + "</small>";
	    ltxt += this.RedrawYearCloseDay(curday, curdate.getDay(), ld-1);
	    }
	ltxt += this.RedrawYearCloseMonth(curmonth);
	}
    return ltxt;
    }


// Hide a layer
function ca_hidelayer(l)
    {
    l.visibility = 'hidden';
    this.layercache.push(l);
    return;
    }


// Redraw in year mode
function ca_redraw_year()
    {
    var mdays = Array(31,28,31, 30,31,30, 31,31,30, 31,30,31);
    var curyear = -1, curmonth = -1, curweek = -1, curday = -1;
    var curdate = new Date();
    var nitems;
    var y;
    var yl,ml;
    curdate.setSeconds(59);
    curdate.setHours(23);
    curdate.setMinutes(59);
    if (this.osrc && this.osrc.LastRecord - this.osrc.FirstRecord + 1 > 0)
	{
	var firstdate = new Date(ca_lookup_field(this.osrc.replica[this.osrc.FirstRecord],this.eventdatefield));
	var lastdate = new Date(ca_lookup_field(this.osrc.replica[this.osrc.LastRecord],this.eventdatefield));
	this.r = this.osrc.FirstRecord;
	this.rec = this.osrc.replica[this.r];
	this.evdate = new Date(ca_lookup_field(this.rec,this.eventdatefield));
	}
    else
	{
	var firstdate = new Date();
	var lastdate = new Date();
	this.r = -1;
	this.rec = null;
	this.evdate = null;
	}
    y = 0;
    for(curyear = firstdate.getYear(); curyear <= lastdate.getYear(); curyear++)
	{
	// Do year layer...
	yl = this.layercache.pop();
	if (!yl) yl = new Layer(this.clip.width,this);
	yl.y = y;
	yl.document.write(this.RedrawYearDoYearLayer(curyear+1900));
	yl.document.close();
	y += yl.clip.height;
	yl.visibility = 'inherit';

	// Do months layers...
	if (y < this.clip.height) for(curmonth = 0; curmonth <= 11; curmonth += 3)
	    {
	    ml = this.layercache.pop();
	    if (!ml) ml = new Layer(this.clip.width, this);
	    ml.y = y;
	    ml.document.write(this.RedrawYearDoMonthLayer(curyear+1900, curmonth));
	    ml.document.close();
	    y += ml.clip.height;
	    ml.visibility = 'inherit';
	    }
	}
    return;
    }

// Redraw in month mode
function ca_redraw_month()
    {
    }

// Redraw in week mode
function ca_redraw_week()
    {
    }

// Redraw in day mode
function ca_redraw_day()
    {
    }

// Redraw the calendar sublayer
function ca_redraw()
    {
    var a;
    // Make invisible and clear the clickable areas
    //this.sublayer.visibility = 'hide';
    while (a = this.arealist.pop()) pg_removearea(a);

    // Redraw according to display mode
    //alert("start");
    //this.sublayer.txt = "<html><head></head><body " + this.main_bg + " text='" + this.textcolor + "'>";
    if (this.dispmode == 'year')
	this.RedrawYear();
    else if (this.dispmode == 'month')
	this.RedrawMonth();
    else if (this.dispmode == 'week')
	this.RedrawWeek();
    else if (this.dispmode == 'day')
	this.RedrawDay();
    //this.sublayer.txt += "</body></html>";
    //alert("done");
    //this.sublayer.document.write(this.sublayer.txt);
    //this.sublayer.document.close();

    // Resize and make visible.
    //this.sublayer.visibility = 'inherit';
    pg_resize(this); // get main layer height correct
    pg_resize(this.parentLayer); // get containing object height correct
    return;
    }

// Change the day or event focus
function ca_refocus(obj)
    {
    return;
    }

// Returns true if the calendar is ready for osrc to discard data
function ca_cb_is_discard_ready()
    {
    return true;
    }

// This is called when data we requested is starting to become available from
// the centrallix server.  Hide the existing data in anticipation of a 
// redraw.
function ca_cb_data_available()
    {
    //this.sublayer.visibility = 'hidden';
    return true;
    }

// This is called when objects we requested are actually now available from the
// osrc - which basically means the object focus changed from one to another.
// (request is ususally getfirst/getnext).
function ca_cb_object_available(obj)
    {
    if (!this.init) pg_addsched("Redraw()",this, 0);
    this.init = true;
    this.Refocus(obj);
    return true;
    }

// This is called when the data in the replica has changed due to the osrc's 
// windowing of the result set.  We need to redraw the widget, since we just
// display what's in the replica.
function ca_cb_replica_moved()
    {
    this.Redraw();
    return true;
    }

// This is called when a requested OSRC operation completes.
function ca_cb_operation_complete()
    {
    return true;
    }

// This is called when a new object has been created in the osrc or when the osrc
// receives a replication message from the server that a new object has been
// created.
function ca_cb_object_created()
    {
    this.Redraw();
    return true;
    }

// This is called when an object in the replica has been deleted
function ca_cb_object_deleted()
    {
    this.Redraw();
    return true;
    }

// This is called when an object in the replica has been modified.
function ca_cb_object_modified()
    {
    this.Redraw();
    return true;
    }

// Event handlers
function ca_mouseup(e)
    {
    if (e.kind == 'ca') cn_activate(e.layer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function ca_mousedown(e)
    {
    if (e.kind == 'ca') cn_activate(e.layer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function ca_mouseover(e)
    {
    if (e.kind == 'ca') cn_activate(e.layer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function ca_mouseout(e)
    {
    if (e.kind == 'ca') cn_activate(e.layer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
function ca_mousemove(e)
    {
    if (e.kind == 'ca') cn_activate(e.layer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Widget initialization.
function ca_init(l,main_bg,cell_bg,textcolor,dispmode,eventdatefield,eventdescfield,eventnamefield,eventpriofield,minprio,w,h)
    {
    l.kind = 'ca';
    if (cx__capabilities.Dom0NS) 
	l.document.layer = l;
    else
	l.layer = l;
    l.mainlayer = l;
    l.clip.width = w;
    l.maxwidth = l.clip.width;
    l.minwidth = l.clip.width;
    //l.sublayer = new Layer(l.clip.width, l);
    //l.sublayer.mainlayer = l;
    //l.sublayer.sublayer = l.sublayer;
    //l.sublayer.document.layer = l.sublayer;
    //l.sublayer.visibility = 'hidden';
    //l.sublayer.kind = 'ca';
    l.arealist = new Array();
    l.init = false;
    l.layercache = new Array();
    l.yearmode = new Object();
    l.yearmode.yearlayers = new Object();
    l.yearmode.monthlayers = new Object();
    l.scrolloffset = 0;
    l.scrollsize = 0;

    // User-supplied Properties
    l.dispmode = dispmode;
    l.eventdatefield = eventdatefield;
    l.eventdescfield = eventdescfield;
    l.eventnamefield = eventnamefield;
    l.eventpriofield = eventpriofield;
    l.minprio = minprio;
    l.main_bg = main_bg;
    l.cell_bg = cell_bg;
    l.textcolor = textcolor;

    // Page area (focus handling) callbacks.
    l.keyhandler = ca_keyhandler;
    l.getfocushandler = ca_getfocus;
    l.losefocushandler = ca_losefocus;
    //pg_addarea(l, -1,-1,l.clip.width+1,l.clip.height+1, 'ebox', 'ebox', is_readonly?0:3);

    // Internal functions
    l.Redraw = ca_redraw;
    l.Refocus = ca_refocus;
    l.RedrawYear = ca_redraw_year;
    l.RedrawYearStartYear = ca_redraw_year_start_year;
    l.RedrawYearCloseYear = ca_redraw_year_close_year;
    l.RedrawYearStartMonth = ca_redraw_year_start_month;
    l.RedrawYearCloseMonth = ca_redraw_year_close_month;
    l.RedrawYearStartDay = ca_redraw_year_start_day;
    l.RedrawYearCloseDay = ca_redraw_year_close_day;
    l.RedrawYearDoYearLayer = ca_redraw_year_doyearlayer;
    l.RedrawYearDoMonthLayer = ca_redraw_year_domonthlayer;
    l.RedrawMonth = ca_redraw_month;
    l.RedrawWeek = ca_redraw_week;
    l.RedrawDay = ca_redraw_day;
    l.HideLayer = ca_hidelayer;

    // Standard OSRC callback and data setup for an OSRC client.
    l.IsDiscardReady = ca_cb_is_discard_ready;
    l.DataAvailable = ca_cb_data_available;
    l.ObjectAvailable = ca_cb_object_available;
    l.ReplicaMoved = ca_cb_replica_moved;
    l.OperationComplete = ca_cb_operation_complete;
    l.ObjectDeleted = ca_cb_object_deleted;
    l.ObjectCreated = ca_cb_object_created;
    l.ObjectModified = ca_cb_object_modified;
    l.IsUnsaved = false;
    l.osrc = wgtrFindContainer(l, "widget/osrc");
    if (l.osrc) 
	l.osrc.Register(l);

    // Events
    ifc_init_widget(l);
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseUp");
    ie.Add("MouseDown");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    return l;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_calendar.js'] = true;
