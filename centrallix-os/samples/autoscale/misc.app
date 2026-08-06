$Version=2$

// Autoscale visual test harness -- display widgets page.
//
// These are the everyday non-container, non-form widgets.  Most of them
// size themselves from their own content (text metrics, image dimensions)
// rather than purely from the layout engine, so they are where a resize
// tends to produce wrong wrapping, wrong clipping or a wrong aspect ratio
// rather than a wrong rectangle.
//
// Several of those drivers default to fl_width=0 (widget/image,
// widget/button, widget/imagebutton and widget/clock), and one rigid widget
// pins the whole layout column it sits in or crosses.  The panes below are
// packed edge to edge so they can easily become completely inflexible.
misc "widget/page"
    {
    title = "Autoscale Harness -- Misc";
    width = 1000; height = 700;
    bgcolor = "#c8c8c8";
    textcolor = "black";

    // The geometry the server was told to lay this page out at.  This
    // comes from the URL rather than from the page widget, because
    // runserver() expressions are evaluated while the widget tree is
    // still being parsed -- before wgtrVerify() runs the layout -- and
    // they cannot reference another widget in any case.
    cx__geom "widget/parameter" { type=string; }

    navbar "widget/component"
	{
	x=0; y=0; width=692; height=52;
	fl_height=0;
	path="nav.cmp";
	page_file="misc.app";
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
	text="design size: 1000 x 700";
	font_size=11; align=right; valign=middle;
	}
    page_edge "widget/pane"
	{
	x=986; y=6; width=8; height=40;
	style=flat; bgcolor="#d00000"; fl_width=0; fl_height=0;
	}

    // ---------------------------------------------------------------
    // Label variants.  Alignment, wrapping and clipping all depend on the
    // label's final width, so these are a direct readout of whether the
    // label got the width the layout engine intended.
    // ---------------------------------------------------------------
    lbl_title "widget/label"
	{
	x=8; y=58; width=330; height=18;
	text="labels: alignment, wrapping, clipping";
	font_size=12; align=left; valign=middle;
	}
    lbl_frame "widget/pane"
	{
	x=8; y=78; width=330; height=202;
	style=lowered; bgcolor="#d8d8d8";

	l_left   "widget/label" { x=6; y=6;  width=316; height=18; text="align left";   font_size=12; align=left;   valign=middle; }
	l_center "widget/label" { x=6; y=26; width=316; height=18; text="align center"; font_size=12; align=center; valign=middle; }
	l_right  "widget/label" { x=6; y=46; width=316; height=18; text="align right";  font_size=12; align=right;  valign=middle; }
	l_bold   "widget/label" { x=6; y=66; width=316; height=18; text="style bold";   font_size=12; align=left;   valign=middle; style=bold; }
	l_ital   "widget/label" { x=6; y=86; width=316; height=18; text="style italic"; font_size=12; align=left;   valign=middle; style=italic; }

	// Wraps: the number of lines must change as the window widens.
	l_wrap "widget/label"
	    {
	    x=6; y=106; width=316; height=40; allow_break=yes; font_size=11; align=left; valign=top;
	    text="WRAPPING FILLER 0....5...10...15...20...25...30...35...40...45...50...55...60 END OF WRAP";
	    }

	// Does not wrap: must clip cleanly at the right edge.
	l_clip "widget/label"
	    {
	    x=6; y=150; width=150; height=18; allow_break=no; font_size=11; align=left; valign=middle;
	    text="CLIP 0....5...10...15...20...25 END";
	    }
	l_ellip "widget/label"
	    {
	    x=6; y=172; width=150; height=18; allow_break=no; overflow_ellipsis=yes; font_size=11; align=left; valign=middle;
	    text="ELLIPSIS 0....5...10...15...20 END";
	    }

	l_f10 "widget/label" { x=166; y=150; width=156; height=18; text="font_size 10"; font_size=10; align=left; valign=middle; }
	l_f16 "widget/label" { x=166; y=170; width=156; height=22; text="font_size 16"; font_size=16; align=left; valign=middle; }
	}

    // ---------------------------------------------------------------
    // Buttons.  Note that the textbutton driver never reads font_size, so
    // its label is always the default size no matter what is set here.
    // ---------------------------------------------------------------
    btn_title "widget/label"
	{
	x=346; y=58; width=330; height=18;
	text="buttons: textbutton, button, imagebutton";
	font_size=12; align=left; valign=middle;
	}
    btn_frame "widget/pane"
	{
	x=346; y=78; width=330; height=202;
	style=lowered; bgcolor="#c0c0c0";

	tb_plain "widget/textbutton"
	    {
	    x=6; y=6; width=150; height=22; text="tristate=no"; tristate=no;
	    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
	    }
	tb_tri "widget/textbutton"
	    {
	    x=164; y=6; width=158; height=22; text="tristate=yes"; tristate=yes;
	    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
	    }
	tb_round "widget/textbutton"
	    {
	    x=6; y=34; width=150; height=22; text="radius 8"; tristate=no;
	    bgcolor="#9098c0"; fgcolor1=black; fgcolor2=white;
	    border_style=solid; border_color="#404040"; border_radius=8;
	    }
	tb_none "widget/textbutton"
	    {
	    x=164; y=34; width=158; height=22; text="border none"; tristate=no;
	    bgcolor="#9098c0"; fgcolor1=black; fgcolor2=white;
	    border_style=none;
	    }
	tb_img "widget/textbutton"
	    {
	    x=6; y=62; width=316; height=34; text="image_position=left"; tristate=no;
	    bgcolor="#b0b0b0"; fgcolor1=black; fgcolor2=white;
	    image="/sys/images/CX.png"; image_position=left;
	    image_width=18; image_height=18; image_margin=4;
	    }

	bt_text "widget/button"
	    {
	    x=6; y=102; width=150; height=26; type=text; text="button type=text";
	    bgcolor="#a8b0a8"; fgcolor1=black; fgcolor2=white; tristate=no;
	    fl_width=1;
	    }
	bt_over "widget/button"
	    {
	    x=164; y=102; width=158; height=26; type=textoverimage; text="textoverimage";
	    image="/sys/images/grey_gradient.png";
	    bgcolor="#a8b0a8"; fgcolor1=black; fgcolor2=white; tristate=no;
	    fl_width=1;
	    }

	ib_first "widget/imagebutton"
	    {
	    x=6; y=140; width=18; height=18;
	    image="/sys/images/ico16aa.gif";
	    pointimage="/sys/images/ico16ab.gif";
	    clickimage="/sys/images/ico16ac.gif";
	    disabledimage="/sys/images/ico16ad.gif";
	    }
	ib_l "widget/label"
	    { x=32; y=140; width=290; height=18; text="imagebutton (fixed 18x18)"; font_size=11; align=left; valign=middle; }

	btn_note "widget/label"
	    {
	    x=6; y=166; width=316; height=30; allow_break=yes; font_size=11; align=left; valign=top;
	    text="textbutton ignores font_size entirely, so its text never scales.";
	    }
	}

    // ---------------------------------------------------------------
    // Images.  aspect=stretch fills the box; anything else preserves the
    // ratio.  Both boxes are the same size, so after a resize the
    // stretched one should distort and the preserved one should not.
    // ---------------------------------------------------------------
    img_title "widget/label"
	{
	x=684; y=58; width=308; height=18;
	text="images: stretch vs preserve";
	font_size=12; align=left; valign=middle;
	}
    img_frame "widget/pane"
	{
	x=684; y=78; width=308; height=202;
	style=lowered; bgcolor="#d8d8d8";

	img_s_l "widget/label" { x=6;   y=4; width=140; height=16; text="stretch";  font_size=11; align=center; valign=middle; }
	img_p_l "widget/label" { x=154; y=4; width=140; height=16; text="preserve"; font_size=11; align=center; valign=middle; }

	img_stretch "widget/image"
	    {
	    x=6; y=22; width=140; height=90;
	    source="/samples/collegiate_peaks.jpg"; aspect=stretch;
	    text="stretched test image";
	    fl_width=1;
	    }
	img_preserve "widget/image"
	    {
	    x=154; y=22; width=140; height=90;
	    source="/samples/collegiate_peaks.jpg"; aspect=preserve;
	    text="aspect-preserved test image";
	    fl_width=1;
	    }

	img_logo "widget/image"
	    {
	    x=6; y=120; width=288; height=52;
	    source="/sys/images/centrallix_374x66.png"; aspect=preserve;
	    text="Centrallix logo";
	    fl_width=1;
	    }

	img_tiny "widget/image"
	    {
	    x=6; y=178; width=24; height=24;
	    source="/samples/city_town.gif"; aspect=preserve;
	    text="24x24 icon";
	    }
	img_tiny_l "widget/label"
	    { x=36; y=180; width=258; height=18; text="24x24 source, drawn 1:1"; font_size=11; align=left; valign=middle; }
	}

    // ---------------------------------------------------------------
    // HTML widget with inline content.  Its 'content' property is written
    // straight into the body, so no external file is needed.  The lines
    // are rulers: watch where they wrap as the window changes.
    // ---------------------------------------------------------------
    html_title "widget/label"
	{
	x=8; y=290; width=490; height=18;
	text="html widget (inline content, mode=dynamic)";
	font_size=12; align=left; valign=middle;
	}
    html_frame "widget/pane"
	{
	x=8; y=310; width=490; height=190;
	style=lowered; bgcolor="#ffffff";

	the_html "widget/html"
	    {
	    x=2; y=2; width=486; height=186;
	    mode=dynamic;
	    content="<b>HTML WIDGET FILLER -- NOTHING TO READ HERE</b><br>LINE 01 0....5...10...15...20...25...30...35...40 END-01<br>LINE 02 0....5...10...15...20...25...30...35...40 END-02<br>LINE 03 0....5...10...15...20...25...30...35...40 END-03<br>LINE 04 0....5...10...15...20...25...30...35...40 END-04<br>LINE 05 0....5...10...15...20...25...30...35...40 END-05<br>LINE 06 0....5...10...15...20...25...30...35...40 END-06<br>LINE 07 LAST LINE -- END OF HTML CONTENT";
	    }
	}

    // ---------------------------------------------------------------
    // Menus.  A menu with no 'direction' property is horizontal; only the
    // exact string 'vertical' switches it.
    // ---------------------------------------------------------------
    menu_title "widget/label"
	{
	x=506; y=290; width=486; height=18;
	text="menus: horizontal bar and vertical list";
	font_size=12; align=left; valign=middle;
	}
    menu_frame "widget/pane"
	{
	x=506; y=310; width=486; height=190;
	style=lowered; bgcolor="#c0c0c0";

	menu_bar "widget/menu"
	    {
	    x=4; y=4; width=478; row_height=22;
	    direction=horizontal;
	    bgcolor="#b0b0b0"; fgcolor="black";
	    highlight_bgcolor="#8090c0"; active_bgcolor="#6070a0";

	    mb_1 "widget/menuitem" { label="Alpha"; value="a"; }
	    mb_2 "widget/menuitem" { label="Bravo"; value="b"; }
	    mb_3 "widget/menuitem" { label="Charlie"; value="c"; }
	    mb_4 "widget/menuitem" { label="Delta"; value="d"; }
	    }

	menu_list "widget/menu"
	    {
	    x=4; y=36; width=200; column_width=200; row_height=22;
	    direction=vertical;
	    bgcolor="#b0b0b0"; fgcolor="black";
	    highlight_bgcolor="#8090c0"; active_bgcolor="#6070a0";

	    ml_1 "widget/menuitem" { label="Short"; value="1"; }
	    ml_2 "widget/menuitem" { label="A much longer entry 0....5...10"; value="2"; }
	    ml_3 "widget/menuitem" { label="Checked item"; value="3"; checked=yes; }
	    ml_4 "widget/menuitem" { label="With an icon"; value="4"; icon="/sys/images/CX.png"; }
	    }

	menu_note "widget/label"
	    {
	    x=212; y=36; width=270; height=60; allow_break=yes; font_size=11; align=left; valign=top;
	    text="The menu driver registers no Select event server-side, so clicks here are not wired to anything.";
	    }
	}

    // ---------------------------------------------------------------
    // Clocks, alerter and timer.
    //
    // The clock repaints itself every second and the timer fires every two
    // seconds, so both keep running while you drag the window edge.  If a
    // resize breaks their redraw, they freeze or misplace their text.
    // ---------------------------------------------------------------
    live_title "widget/label"
	{
	x=8; y=510; width=984; height=18;
	text="live widgets: clocks, timer, alerter";
	font_size=12; align=left; valign=middle;
	}
    live_frame "widget/pane"
	{
	x=8; y=530; width=984; height=160;
	style=lowered; bgcolor="#b0b0b0";

	clock_12_l "widget/label" { x=6; y=6; width=240; height=16; text="12 hour, with seconds"; font_size=11; align=left; valign=middle; }
	clock_12 "widget/clock"
	    {
	    x=6; y=24; width=240; height=44;
	    size=18; seconds=yes; ampm=yes;
	    fgcolor1="#000000";
	    fl_width=1;
	    }

	clock_24_l "widget/label" { x=258; y=6; width=240; height=16; text="24 hour, shadowed"; font_size=11; align=left; valign=middle; }
	clock_24 "widget/clock"
	    {
	    x=258; y=24; width=240; height=44;
	    size=18; seconds=yes; hrtype=24;
	    shadowed=yes; fgcolor1="#000000"; fgcolor2="#909090"; shadowx=2; shadowy=2;
	    fl_width=1;
	    }

	// Fires every two seconds and stamps the time onto the button.
	the_timer "widget/timer"
	    {
	    msec=1000; auto_start=1; auto_reset=1;
	    t_conn "widget/connector"
		{ event=Expire; target=tick_btn; action=SetText; Text=runclient(getdate()); }
	    }
	tick_l "widget/label" { x=510; y=6; width=240; height=16; text="timer"; font_size=11; align=left; valign=middle; }
	tick_btn "widget/textbutton"
	    {
	    x=510; y=24; width=240; height=24; text="waiting for first tick"; tristate=no;
	    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
	    }

	// Nonvisual; driven entirely by the two buttons below.
	the_alerter "widget/alerter" { }

	alert_l "widget/label" { x=762; y=6; width=216; height=16; text="alerter"; font_size=11; align=left; valign=middle; }
	alert_btn "widget/textbutton"
	    {
	    x=762; y=24; width=104; height=24; text="Alert"; tristate=no;
	    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
	    alert_btn_c "widget/connector"
		{ event=Click; target=the_alerter; action=Alert; Parameter=runclient("alerter test message"); }
	    }
	confirm_btn "widget/textbutton"
	    {
	    x=874; y=24; width=104; height=24; text="Confirm"; tristate=no;
	    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
	    confirm_btn_c "widget/connector"
		{ event=Click; target=the_alerter; action=Confirm; Parameter=runclient("alerter confirm test"); }
	    }

	live_note "widget/label"
	    {
	    x=6; y=80; width=972; height=44; allow_break=yes; font_size=11; align=left; valign=top;
	    text="Drag the window edge while these are running. The clocks should keep ticking in place and the timer button should keep updating; freezing or drifting text after a resize is the bug to look for. The alerter discards the Confirm result, so only the dialog itself is testable.";
	    }
	}
    }
