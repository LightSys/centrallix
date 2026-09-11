$Version=2$

// Autoscale visual test harness -- legacy / low-use widgets.
//
// These widgets are barely used outside the test tree and some are already
// suspect.  They are quarantined here ON PURPOSE: a bad render or an outright
// failure happens where it cannot make a problem in a widely-used widget
// harder to spot.  Treat a failure on this page as low priority unless it is
// a regression against master.
//
// This page is designed at 1000x900 because it has more to fit in, which is a
// mild extra test in itself: a design taller than the browser window
// exercises the scale-down path the other pages never reach.
//
// The "Deliberately not included" pane below lists the widgets the harness
// leaves out entirely, and why.
legacy "widget/page"
    {
    title = "Autoscale Harness -- Legacy";
    width = 1000; height = 900;
    bgcolor = "#c8c8c8";
    textcolor = "black";

    // The geometry the server was told to lay this page out at, read from
    // the URL: runserver() expressions are evaluated as the widget tree is
    // parsed, before the layout runs, and cannot reference another widget.
    cx__geom "widget/parameter" { type=string; }

    navbar "widget/component"
	{
	x=0; y=0; width=692; height=52;
	fl_height=0;
	path="nav.cmp";
	page_file="legacy.app";
	}

    geom_note "widget/label"
	{
	x=700; y=6; width=280; height=18;
	text = runserver(condition(:this:cx__geom = 'design', 'server render: design', 'server render: ' + convert('string', (charindex(substring(lower(:this:cx__geom),1,1),'0123456789abcdef')-1)*4096 + (charindex(substring(lower(:this:cx__geom),2,1),'0123456789abcdef')-1)*256 + (charindex(substring(lower(:this:cx__geom),3,1),'0123456789abcdef')-1)*16 + (charindex(substring(lower(:this:cx__geom),4,1),'0123456789abcdef')-1)) + ' x ' + convert('string', (charindex(substring(lower(:this:cx__geom),5,1),'0123456789abcdef')-1)*4096 + (charindex(substring(lower(:this:cx__geom),6,1),'0123456789abcdef')-1)*256 + (charindex(substring(lower(:this:cx__geom),7,1),'0123456789abcdef')-1)*16 + (charindex(substring(lower(:this:cx__geom),8,1),'0123456789abcdef')-1))));
	font_size=11; align=right; valign=middle;
	}
    design_note "widget/label"
	{
	x=700; y=28; width=280; height=18;
	text="design size: 1000 x 900";
	font_size=11; align=right; valign=middle;
	}
    page_edge "widget/pane"
	{
	x=986; y=6; width=8; height=40;
	style=flat; bgcolor="#d00000"; fl_width=0; fl_height=0;
	}

    warn "widget/pane"
	{
	x=8; y=58; width=984; height=40;
	style=raised; bgcolor="#d8c8a0";

	warn_l "widget/label"
	    {
	    x=8; y=4; width=968; height=32; allow_break=yes; font_size=12; align=left; valign=top;
	    text="Low-priority page. These widgets are legacy or nearly unused, and some are broken for reasons unrelated to scaling. They are isolated here so a failure cannot obscure a problem in a widget that matters.";
	    }
	}

    // ---------------------------------------------------------------
    // Multiscroll.  Parts that declare a height pin to the top or the
    // bottom; the single part with no height becomes the scrolling middle.
    // The header and footer should keep their exact heights while only the
    // middle grows and shrinks.
    // ---------------------------------------------------------------
    ms_title "widget/label"
	{
	x=8; y=106; width=320; height=18;
	text="multiscroll (pinned ends, scrolling middle)";
	font_size=12; align=left; valign=middle;
	}
    ms_frame "widget/pane"
	{
	x=8; y=126; width=320; height=300;
	style=lowered; bgcolor="#909090";

	the_ms "widget/multiscroll"
	    {
	    x=4; y=4; width=310; height=292;
	    bgcolor="#d0d0d0";

	    ms_head "widget/multiscrollpart"
		{
		height=32; always_visible=yes;
		ms_head_bg "widget/pane"
		    { x=0; y=0; width=310; height=32; style=flat; bgcolor="#4060a0"; }
		ms_head_l "widget/label"
		    { x=6; y=7; width=290; height=18; text="pinned header (32px)"; font_size=11; fgcolor="white"; align=left; valign=middle; }
		}

	    ms_body "widget/multiscrollpart"
		{
		ms_b01 "widget/label" { x=6; y=4;   width=280; height=18; text="BODY 01 0....5...10...15 END-01"; font_size=11; align=left; valign=middle; }
		ms_b02 "widget/label" { x=6; y=26;  width=280; height=18; text="BODY 02 0....5...10...15 END-02"; font_size=11; align=left; valign=middle; }
		ms_b03 "widget/label" { x=6; y=48;  width=280; height=18; text="BODY 03 0....5...10...15 END-03"; font_size=11; align=left; valign=middle; }
		ms_b04 "widget/label" { x=6; y=70;  width=280; height=18; text="BODY 04 0....5...10...15 END-04"; font_size=11; align=left; valign=middle; }
		ms_b05 "widget/label" { x=6; y=92;  width=280; height=18; text="BODY 05 0....5...10...15 END-05"; font_size=11; align=left; valign=middle; }
		ms_b06 "widget/label" { x=6; y=114; width=280; height=18; text="BODY 06 0....5...10...15 END-06"; font_size=11; align=left; valign=middle; }
		ms_b07 "widget/label" { x=6; y=136; width=280; height=18; text="BODY 07 0....5...10...15 END-07"; font_size=11; align=left; valign=middle; }
		ms_b08 "widget/label" { x=6; y=158; width=280; height=18; text="BODY 08 0....5...10...15 END-08"; font_size=11; align=left; valign=middle; }
		ms_b09 "widget/label" { x=6; y=180; width=280; height=18; text="BODY 09 0....5...10...15 END-09"; font_size=11; align=left; valign=middle; }
		ms_b10 "widget/label" { x=6; y=202; width=280; height=18; text="BODY 10 0....5...10...15 END-10"; font_size=11; align=left; valign=middle; }
		ms_b11 "widget/label" { x=6; y=224; width=280; height=18; text="BODY 11 0....5...10...15 END-11"; font_size=11; align=left; valign=middle; }
		ms_b12 "widget/label" { x=6; y=246; width=280; height=18; text="BODY 12 LAST -- END OF BODY"; font_size=11; align=left; valign=middle; }
		}

	    ms_foot "widget/multiscrollpart"
		{
		height=28; always_visible=yes;
		ms_foot_bg "widget/pane"
		    { x=0; y=0; width=310; height=28; style=flat; bgcolor="#a04040"; }
		ms_foot_l "widget/label"
		    { x=6; y=5; width=290; height=18; text="pinned footer (28px)"; font_size=11; fgcolor="white"; align=left; valign=middle; }
		}
	    }
	}

    // ---------------------------------------------------------------
    // Terminal.  Backed by ruler.shl in this directory, which prints a
    // numbered character ruler and exits immediately.  The widget is a fixed
    // grid of rows x cols, so the ruler shows at a glance how many columns
    // are really visible and whether the grid still lines up.  Loading the
    // page spawns a short-lived process on the server; with the system/shell
    // driver unavailable the widget renders an empty grid, which is expected.
    // ---------------------------------------------------------------
    term_title "widget/label"
	{
	x=336; y=106; width=400; height=18;
	text="terminal (ruler.shl prints a grid and exits)";
	font_size=12; align=left; valign=middle;
	}
    term_frame "widget/pane"
	{
	x=336; y=126; width=400; height=300;
	style=lowered; bgcolor="#000000";

	the_term "widget/terminal"
	    {
	    x=4; y=4;
	    rows=20; cols=60;
	    fontsize=10;
	    source="/samples/autoscale/ruler.shl";
	    }
	}

    // ---------------------------------------------------------------
    // Widgets deliberately left out, and why.
    // ---------------------------------------------------------------
    skipped "widget/pane"
	{
	x=744; y=126; width=248; height=300;
	style=raised; bgcolor="#b0b0b0";

	sk_title "widget/label"
	    {
	    x=8; y=6; width=232; height=18;
	    text="Deliberately not included"; font_size=13; style=bold; align=left; valign=middle;
	    }
	sk_1 "widget/label"
	    {
	    x=8; y=30; width=232; height=46; allow_break=yes; font_size=11; align=left; valign=top;
	    text="widget/spinner -- requires Netscape 4 DOM and errors out on any current browser, which would break this page.";
	    }
	sk_2 "widget/label"
	    {
	    x=8; y=80; width=232; height=58; allow_break=yes; font_size=11; align=left; valign=top;
	    text="widget/window -- opens a separate browser window and registers no actions at all, so nothing can trigger it from a connector.";
	    }
	sk_3 "widget/label"
	    {
	    x=8; y=142; width=232; height=58; allow_break=yes; font_size=11; align=left; valign=top;
	    text="widget/frameset -- renders each frame as its own HTML document, so it cannot share a page with anything and scales per frame.";
	    }
	sk_4 "widget/label"
	    {
	    x=8; y=204; width=232; height=40; allow_break=yes; font_size=11; align=left; valign=top;
	    text="widget/execmethod -- CSV objects expose no methods, so there is nothing for it to call.";
	    }
	sk_5 "widget/label"
	    {
	    x=8; y=248; width=232; height=44; allow_break=yes; font_size=11; align=left; valign=top;
	    text="widget/rule, widget/variable, widget/template -- nonvisual or declarative, so they have no geometry to scale.";
	    }
	}

    // ---------------------------------------------------------------
    // Map and objcanvas.  Both plot their contents from data rows, reading
    // x/y/width/height/color/image off each record, and reuse the existing
    // CanvasObjects sample data.  The map also needs the OpenLayers bundle
    // under /sys/js.
    // ---------------------------------------------------------------
    canvas_osrc "widget/osrc"
	{
	sql = "select :x, :y, :width, :height, :color, :image, :type, :description from /samples/CanvasObjects.csv/rows";
	baseobj = "/samples/CanvasObjects.csv/rows";
	replicasize = 32;
	readahead = 32;
	autoquery = onload;

	map_title "widget/label"
	    {
	    x=8; y=436; width=490; height=18;
	    text="map (OpenLayers-backed)";
	    font_size=12; align=left; valign=middle;
	    }
	map_frame "widget/pane"
	    {
	    x=8; y=456; width=490; height=224;
	    style=lowered; bgcolor="#909090";

	    the_map "widget/map"
		{
		x=2; y=2; width=486; height=220;
		background="/samples/collegiate_peaks.jpg";
		source=canvas_osrc;
		allow_selection=yes;
		show_selection=yes;
		}
	    }

	oc_title "widget/label"
	    {
	    x=506; y=436; width=486; height=18;
	    text="objcanvas (same data, no external library)";
	    font_size=12; align=left; valign=middle;
	    }
	oc_frame "widget/pane"
	    {
	    x=506; y=456; width=486; height=224;
	    style=lowered; bgcolor="#909090";

	    the_oc "widget/objcanvas"
		{
		x=2; y=2; width=482; height=220;
		background="/samples/collegiate_peaks.jpg";
		source=canvas_osrc;
		allow_selection=yes;
		show_selection=yes;
		}
	    }
	}

    // ---------------------------------------------------------------
    // Calendar.  Nothing in the repository uses this widget, and it carries
    // a core bug two decades older than this branch: htdrv_calendar.c reads
    // the "displaymode" STRING into the integer 'minpriority', so
    // minpriority never works and the read writes a pointer-sized value into
    // an int.  That is why the calendar sits here.
    //
    // events.csv stores day offsets; the query turns them into dates around
    // today with dateadd(), so something is always visible in the current
    // month.
    // ---------------------------------------------------------------
    cal_title "widget/label"
	{
	x=8; y=690; width=984; height=18;
	text="calendar over events.csv (events placed relative to today)";
	font_size=12; align=left; valign=middle;
	}
    cal_frame "widget/pane"
	{
	x=8; y=710; width=984; height=180;
	style=lowered; bgcolor="#ffffff";

	cal_osrc "widget/osrc"
	    {
	    sql = "select event_name = :title, event_desc = :descr, :prio, event_date = dateadd('day', :offset, getdate()) from /samples/autoscale/events.csv/rows";
	    replicasize = 64;
	    readahead = 64;
	    autoquery = onload;

	    the_cal "widget/calendar"
		{
		x=2; y=2; width=980; height=176;
		displaymode="month";
		eventdatefield="event_date";
		eventnamefield="event_name";
		eventdescfield="event_desc";
		eventpriofield="prio";
		bgcolor="#ffffff"; textcolor="black";
		}
	    }
	}
    }
