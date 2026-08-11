$Version=2$

// Autoscale visual test harness -- container widgets page.
//
// Each container here reserves some edge space for parts it draws itself
// (a tab strip, a titlebar, a scrollbar gutter) and lays its children out
// inside what is left.  Those reserved amounts are declared in the widget
// drivers under centrallix/wgtr/ and must stay in step with what the
// matching centrallix/htmlgen/ driver actually draws.
//
// Every container below holds four corner markers of a fixed 12x12 size.
// The markers show exactly where the container thinks its client area is:
// if a marker is clipped, floating away from its corner, or changing size,
// the reserved edge space and the drawn geometry disagree.
containers "widget/page"
    {
    title = "Autoscale Harness -- Containers";
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
	page_file="containers.app";
	}

    // Server render geometry.  Every page carries this same note in the
    // same place: the strip to the right of the nav component.
    //
    // ':containers:width' and ':containers:height' are the page's COMPUTED
    // size, which is the geometry the server actually laid this page out
    // at.  cx__geom is stripped from the address bar once the page loads,
    // so this note is the only way to see which size a page was rendered
    // at after the fact.  It does NOT track window resizing -- that is the
    // point: compare it against the current window size.
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

    // Pinned to the page's own right edge, for comparison with the nav
    // component's marker at the right edge of the strip.
    page_edge "widget/pane"
	{
	x=986; y=6; width=8; height=40;
	style=flat; bgcolor="#d00000"; fl_width=0; fl_height=0;
	}

    // ---------------------------------------------------------------
    // Tab controls.  A tab control's width/height in the structure file
    // describe its CONTENT area; the strip of tabs is added outside of
    // that.  Three strip positions are shown because each one reserves a
    // different edge.
    // ---------------------------------------------------------------
    tab_top_title "widget/label"
	{
	x=8; y=58; width=320; height=18;
	text="tab_location=top"; font_size=12; align=left; valign=middle;
	}
    tab_top "widget/tab"
	{
	x=8; y=80; width=320; height=180;
	tab_location=top; tab_height=24; fl_height=30;
	bgcolor="#b0b0b0"; inactive_bgcolor="#8c8c8c"; textcolor="black";

	tt_p1 "widget/tabpage"
	    {
	    title="One";
	    tt1_tl "widget/pane" { x=0;   y=0;   width=12; height=12; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	    tt1_tr "widget/pane" { x=306; y=0;   width=12; height=12; style=flat; bgcolor="#e0e000"; fl_width=0; fl_height=0; }
	    tt1_bl "widget/pane" { x=0;   y=166; width=12; height=12; style=flat; bgcolor="#00c000"; fl_width=0; fl_height=0; }
	    tt1_br "widget/pane" { x=306; y=166; width=12; height=12; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }
	    tt1_fill "widget/pane" { x=20; y=20; width=278; height=138; style=flat; bgcolor="#5080c0"; }
	    }
	tt_p2 "widget/tabpage"
	    {
	    title="Two";
	    tt2_fill "widget/pane" { x=0; y=0; width=318; height=178; style=flat; bgcolor="#50a050"; }
	    }
	tt_p3 "widget/tabpage"
	    {
	    title="Three";
	    tt3_fill "widget/pane" { x=0; y=0; width=318; height=178; style=flat; bgcolor="#c07040"; }
	    }
	}

    tab_bot_title "widget/label"
	{
	x=340; y=58; width=300; height=18;
	text="tab_location=bottom"; font_size=12; align=left; valign=middle;
	}
    tab_bot "widget/tab"
	{
	x=340; y=80; width=300; height=180;
	tab_location=bottom; tab_height=24; fl_height=30;
	bgcolor="#b0b0b0"; inactive_bgcolor="#8c8c8c"; textcolor="black";

	tb_p1 "widget/tabpage"
	    {
	    title="One";
	    tb1_tl "widget/pane" { x=0;   y=0;   width=12; height=12; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	    tb1_tr "widget/pane" { x=286; y=0;   width=12; height=12; style=flat; bgcolor="#e0e000"; fl_width=0; fl_height=0; }
	    tb1_bl "widget/pane" { x=0;   y=166; width=12; height=12; style=flat; bgcolor="#00c000"; fl_width=0; fl_height=0; }
	    tb1_br "widget/pane" { x=286; y=166; width=12; height=12; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }
	    tb1_fill "widget/pane" { x=20; y=20; width=258; height=138; style=flat; bgcolor="#5080c0"; }
	    }
	tb_p2 "widget/tabpage"
	    {
	    title="Two";
	    tb2_fill "widget/pane" { x=0; y=0; width=298; height=178; style=flat; bgcolor="#8040c0"; }
	    }
	}

    // A side-mounted strip needs tab_width; without it the widget refuses
    // to render at all.
    tab_left_title "widget/label"
	{
	x=652; y=58; width=340; height=18;
	text="tab_location=left (needs tab_width)"; font_size=12; align=left; valign=middle;
	}
    tab_left "widget/tab"
	{
	x=652; y=80; width=270; height=180;
	tab_location=left; tab_width=68; tab_height=24; fl_height=30;
	bgcolor="#b0b0b0"; inactive_bgcolor="#8c8c8c"; textcolor="black";

	tl_p1 "widget/tabpage"
	    {
	    title="One";
	    tl1_tl "widget/pane" { x=0;   y=0;   width=12; height=12; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	    tl1_tr "widget/pane" { x=256; y=0;   width=12; height=12; style=flat; bgcolor="#e0e000"; fl_width=0; fl_height=0; }
	    tl1_bl "widget/pane" { x=0;   y=166; width=12; height=12; style=flat; bgcolor="#00c000"; fl_width=0; fl_height=0; }
	    tl1_br "widget/pane" { x=256; y=166; width=12; height=12; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }
	    tl1_fill "widget/pane" { x=20; y=20; width=228; height=138; style=flat; bgcolor="#50a050"; }
	    }
	tl_p2 "widget/tabpage"
	    {
	    title="Two";
	    tl2_fill "widget/pane" { x=0; y=0; width=268; height=178; style=flat; bgcolor="#c05050"; }
	    }
	}

    // ---------------------------------------------------------------
    // Scrollpane.  It reserves 18px on the right for its scrollbar, and
    // its contents are deliberately taller than the visible area.  The
    // layout engine does not vertically auto-scale content inside a
    // scrollpane, so the rows below should keep their height and simply
    // become more or less visible as the window changes.
    // ---------------------------------------------------------------
    sp_title "widget/label"
	{
	x=8; y=300; width=240; height=18;
	text="scrollpane (18px gutter)"; font_size=12; align=left; valign=middle;
	}
    sp_frame "widget/pane"
	{
	x=8; y=320; width=240; height=190;
	style=lowered; bgcolor="#909090";
	fl_height=40;

	the_sp "widget/scrollpane"
	    {
	    x=4; y=4; width=230; height=182;
	    bgcolor="#d0d0d0";

	    sp_r01 "widget/label" { x=4; y=4;   width=200; height=18; text="ROW 01 0....5...10 END-01"; font_size=11; align=left; valign=middle; }
	    sp_r02 "widget/label" { x=4; y=26;  width=200; height=18; text="ROW 02 0....5...10 END-02"; font_size=11; align=left; valign=middle; }
	    sp_r03 "widget/label" { x=4; y=48;  width=200; height=18; text="ROW 03 0....5...10 END-03"; font_size=11; align=left; valign=middle; }
	    sp_r04 "widget/label" { x=4; y=70;  width=200; height=18; text="ROW 04 0....5...10 END-04"; font_size=11; align=left; valign=middle; }
	    sp_r05 "widget/label" { x=4; y=92;  width=200; height=18; text="ROW 05 0....5...10 END-05"; font_size=11; align=left; valign=middle; }
	    sp_r06 "widget/label" { x=4; y=114; width=200; height=18; text="ROW 06 0....5...10 END-06"; font_size=11; align=left; valign=middle; }
	    sp_r07 "widget/label" { x=4; y=136; width=200; height=18; text="ROW 07 0....5...10 END-07"; font_size=11; align=left; valign=middle; }
	    sp_r08 "widget/label" { x=4; y=158; width=200; height=18; text="ROW 08 0....5...10 END-08"; font_size=11; align=left; valign=middle; }
	    sp_r09 "widget/label" { x=4; y=180; width=200; height=18; text="ROW 09 0....5...10 END-09"; font_size=11; align=left; valign=middle; }
	    sp_r10 "widget/label" { x=4; y=202; width=200; height=18; text="ROW 10 0....5...10 END-10"; font_size=11; align=left; valign=middle; }
	    sp_r11 "widget/label" { x=4; y=224; width=200; height=18; text="ROW 11 0....5...10 END-11"; font_size=11; align=left; valign=middle; }
	    sp_r12 "widget/label" { x=4; y=246; width=200; height=18; text="ROW 12 0....5...10 END-12"; font_size=11; align=left; valign=middle; }
	    sp_r13 "widget/label" { x=4; y=268; width=200; height=18; text="ROW 13 0....5...10 END-13"; font_size=11; align=left; valign=middle; }
	    sp_r14 "widget/label" { x=4; y=290; width=200; height=18; text="ROW 14 0....5...10 END-14"; font_size=11; align=left; valign=middle; }
	    sp_r15 "widget/label" { x=4; y=312; width=200; height=18; text="ROW 15 LAST -- END"; font_size=11; align=left; valign=middle; }
	    }
	}

    // ---------------------------------------------------------------
    // Standalone scrollbars.  A scrollbar is fixed at 18px across its
    // short axis and refuses to render shorter than 54px along its long
    // axis, so the vertical one must keep its width and the horizontal
    // one must keep its height no matter how the window changes.
    // ---------------------------------------------------------------
    sb_title "widget/label"
	{
	x=256; y=300; width=300; height=18;
	text="scrollbars (18px across, min 54 along)"; font_size=12; align=left; valign=middle;
	}
    sb_frame "widget/pane"
	{
	x=256; y=320; width=300; height=190;
	style=lowered; bgcolor="#909090";
	fl_height=40;

	sb_vert "widget/scrollbar"
	    { x=12; y=12; height=160; direction=vertical; range=100; }
	sb_horiz "widget/scrollbar"
	    { x=48; y=12; width=232; direction=horizontal; range=100; }

	sb_note "widget/label"
	    {
	    x=48; y=40; width=232; height=48; allow_break=yes; font_size=11; align=left; valign=top;
	    text="Both bars keep their 18px thickness at every window size.";
	    }
	}

    // ---------------------------------------------------------------
    // Component instance.  A component is laid out on the server as part
    // of this page, so it should scale exactly like an inline subtree.
    // The nav strip at the top of every page is the same mechanism.
    // ---------------------------------------------------------------
    cmp_title "widget/label"
	{
	x=564; y=300; width=428; height=18;
	text="component instance"; font_size=12; align=left; valign=middle;
	}
    cmp_frame "widget/pane"
	{
	x=564; y=320; width=428; height=190;
	style=lowered; bgcolor="#909090";
	fl_height=40;

	cmp_tl "widget/pane" { x=0;   y=0;   width=12; height=12; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	cmp_br "widget/pane" { x=414; y=176; width=12; height=12; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }

	the_cmp "widget/component"
	    {
	    x=20; y=20; width=386; height=150;
	    path="/samples/button.cmp";
	    }
	}

    // ---------------------------------------------------------------
    // Child windows.  These float: the layout engine skips them in the
    // main pass and only clamps them inside their container at the end.
    // The three variants reserve different amounts of edge space, so
    // their corner markers sit in visibly different places.
    // ---------------------------------------------------------------
    win_title "widget/label"
	{
	x=8; y=520; width=984; height=18;
	text="childwindow variants (floating; markers show each client area)";
	font_size=12; align=left; valign=middle;
	}
    win_frame "widget/pane"
	{
	x=8; y=540; width=984; height=150;
	style=lowered; bgcolor="#8c8c8c";
	fl_height=50;

	win_bare "widget/childwindow"
	    {
	    x=660; y=12; width=300; height=124;
	    style=window; titlebar=no;
	    bgcolor="#c0c0c0";

	    wb_tl "widget/pane" { x=0;   y=0;   width=12; height=12; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	    wb_br "widget/pane" { x=285; y=110; width=12; height=12; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }
	    wb_note "widget/label"
		{ x=20; y=48; width=250; height=18; text="titlebar=no"; font_size=11; align=center; valign=middle; }
	    }

	win_dlg "widget/childwindow"
	    {
	    x=336; y=12; width=300; height=124;
	    style=dialog; titlebar=yes; title="style=dialog";
	    bgcolor="#c0c0c0"; hdr_bgcolor="#a04040"; textcolor="white";

	    wd_tl "widget/pane" { x=0;   y=0;  width=12; height=12; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	    wd_br "widget/pane" { x=286; y=87; width=12; height=12; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }
	    }

	win_std "widget/childwindow"
	    {
	    x=12; y=12; width=300; height=124;
	    style=window; titlebar=yes; title="style=window";
	    bgcolor="#c0c0c0"; hdr_bgcolor="#4060a0"; textcolor="white";

	    ws_tl "widget/pane" { x=0;   y=0;  width=12; height=12; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	    ws_br "widget/pane" { x=285; y=88; width=12; height=12; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }
	    }
	}
    }
