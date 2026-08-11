$Version=2$

// Autoscale visual test harness -- layout engine page.
//
// Everything here is built from plain panes, so nothing on this page depends
// on any individual widget driver.  If a test here fails, the bug is in
// apos.c or in the responsive CSS that ht_render.c emits, not in a widget.
//
// The thresholds probed below come from centrallix/include/apos.h:
//   APOS_MINSPACE 20  -- a gap of 20px or less between two widgets is a
//                        "spacer" and is pinned at flex 0, so it must NOT
//                        grow when the window grows.
//   APOS_MINWIDTH 30  -- the smallest width a widget is allowed to shrink to.
layout "widget/page"
    {
    title = "Autoscale Harness -- Layout";
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
	page_file="layout.app";
	}

    // Server render geometry.  Every page carries this same note in the
    // same place: the strip to the right of the nav component.
    //
    // ':layout:width' and ':layout:height' are the page's COMPUTED
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
    // Equal grid.  Twelve identical cells with identical gaps.  Under
    // correct scaling every cell stays the same size as every other
    // cell and every gap stays equal.  Uneven cells are the single
    // clearest symptom of a distribution bug in aposSpaceOutLines().
    // ---------------------------------------------------------------
    grid_title "widget/label"
	{
	x=8; y=58; width=486; height=18;
	text="Equal grid: all 12 cells and all gaps stay equal";
	font_size=12; align=left; valign=middle;
	}
    grid "widget/pane"
	{
	x=8; y=78; width=486; height=202;
	style=lowered; bgcolor="#909090";

	g11 "widget/pane" { x=10;  y=10;  width=98; height=44; style=flat; bgcolor="#5080c0"; }
	g12 "widget/pane" { x=132; y=10;  width=98; height=44; style=flat; bgcolor="#5080c0"; }
	g13 "widget/pane" { x=254; y=10;  width=98; height=44; style=flat; bgcolor="#5080c0"; }
	g14 "widget/pane" { x=376; y=10;  width=98; height=44; style=flat; bgcolor="#5080c0"; }
	g21 "widget/pane" { x=10;  y=78;  width=98; height=44; style=flat; bgcolor="#50a050"; }
	g22 "widget/pane" { x=132; y=78;  width=98; height=44; style=flat; bgcolor="#50a050"; }
	g23 "widget/pane" { x=254; y=78;  width=98; height=44; style=flat; bgcolor="#50a050"; }
	g24 "widget/pane" { x=376; y=78;  width=98; height=44; style=flat; bgcolor="#50a050"; }
	g31 "widget/pane" { x=10;  y=146; width=98; height=44; style=flat; bgcolor="#c07040"; }
	g32 "widget/pane" { x=132; y=146; width=98; height=44; style=flat; bgcolor="#c07040"; }
	g33 "widget/pane" { x=254; y=146; width=98; height=44; style=flat; bgcolor="#c07040"; }
	g34 "widget/pane" { x=376; y=146; width=98; height=44; style=flat; bgcolor="#c07040"; }
	}

    // ---------------------------------------------------------------
    // Flex ladder.  Eight bars, identical at the design size, differing
    // only in fl_width.  The gaps between them are 8px, which is under
    // APOS_MINSPACE, so the gaps stay fixed and all the extra width goes
    // into the bars themselves.
    //
    // Widen the window: growth must increase strictly left to right.
    // The leftmost bar (fl_width=0) must never change width at all.
    // ---------------------------------------------------------------
    ladder_title "widget/label"
	{
	x=502; y=58; width=490; height=18;
	text="Flex ladder: growth increases left to right; '0' never grows";
	font_size=12; align=left; valign=middle;
	}
    ladder "widget/pane"
	{
	x=502; y=78; width=490; height=202;
	style=lowered; bgcolor="#909090";

	fl_l0   "widget/label" { x=24;  y=8; width=48; height=16; text="0";    font_size=11; align=center; valign=middle; fl_width=0; }
	fl_l1   "widget/label" { x=80;  y=8; width=48; height=16; text="1";    font_size=11; align=center; valign=middle; fl_width=1; }
	fl_l2   "widget/label" { x=136; y=8; width=48; height=16; text="2";    font_size=11; align=center; valign=middle; fl_width=2; }
	fl_l5   "widget/label" { x=192; y=8; width=48; height=16; text="5";    font_size=11; align=center; valign=middle; fl_width=5; }
	fl_l10  "widget/label" { x=248; y=8; width=48; height=16; text="10";   font_size=11; align=center; valign=middle; fl_width=10; }
	fl_l25  "widget/label" { x=304; y=8; width=48; height=16; text="25";   font_size=11; align=center; valign=middle; fl_width=25; }
	fl_l50  "widget/label" { x=360; y=8; width=48; height=16; text="50";   font_size=11; align=center; valign=middle; fl_width=50; }
	fl_l100 "widget/label" { x=416; y=8; width=48; height=16; text="100";  font_size=11; align=center; valign=middle; fl_width=100; }

	fl_b0   "widget/pane" { x=24;  y=34; width=48; height=152; style=flat; bgcolor="#803030"; fl_width=0;   fl_height=100; }
	fl_b1   "widget/pane" { x=80;  y=34; width=48; height=152; style=flat; bgcolor="#904040"; fl_width=1;   fl_height=100; }
	fl_b2   "widget/pane" { x=136; y=34; width=48; height=152; style=flat; bgcolor="#a05040"; fl_width=2;   fl_height=100; }
	fl_b5   "widget/pane" { x=192; y=34; width=48; height=152; style=flat; bgcolor="#b06040"; fl_width=5;   fl_height=100; }
	fl_b10  "widget/pane" { x=248; y=34; width=48; height=152; style=flat; bgcolor="#c07840"; fl_width=10;  fl_height=100; }
	fl_b25  "widget/pane" { x=304; y=34; width=48; height=152; style=flat; bgcolor="#d09040"; fl_width=25;  fl_height=100; }
	fl_b50  "widget/pane" { x=360; y=34; width=48; height=152; style=flat; bgcolor="#e0a840"; fl_width=50;  fl_height=100; }
	fl_b100 "widget/pane" { x=416; y=34; width=48; height=152; style=flat; bgcolor="#f0c040"; fl_width=100; fl_height=100; }
	}

    // ---------------------------------------------------------------
    // Spacer threshold.  Six pairs of identical blocks whose only
    // difference is the gap between them: 10, 19, 20, 21, 30 and 40px.
    //
    // Gaps of 20 or less are spacers and must stay exactly the same
    // width forever.  The 21px gap is the first one allowed to grow.
    // Watch the 20 and 21 pairs side by side as you widen the window:
    // they should visibly diverge, and nothing at or below 20 should move.
    // ---------------------------------------------------------------
    spacer_title "widget/label"
	{
	x=8; y=288; width=486; height=18;
	text="Spacer threshold: gaps <=20 must never grow; 21 must";
	font_size=12; align=left; valign=middle;
	}
    spacers "widget/pane"
	{
	x=8; y=308; width=486; height=152;
	style=lowered; bgcolor="#909090";

	sp_t10 "widget/label" { x=8;   y=8; width=58; height=16; text="10"; font_size=11; align=center; valign=middle; fl_width=100; }
	sp_t19 "widget/label" { x=74;  y=8; width=67; height=16; text="19"; font_size=11; align=center; valign=middle; fl_width=100; }
	sp_t20 "widget/label" { x=149; y=8; width=68; height=16; text="20"; font_size=11; align=center; valign=middle; fl_width=100; }
	sp_t21 "widget/label" { x=225; y=8; width=69; height=16; text="21"; font_size=11; align=center; valign=middle; fl_width=100; }
	sp_t30 "widget/label" { x=302; y=8; width=78; height=16; text="30"; font_size=11; align=center; valign=middle; fl_width=100; }
	sp_t40 "widget/label" { x=388; y=8; width=88; height=16; text="40"; font_size=11; align=center; valign=middle; fl_width=100; }

	// gap 10
	sp_a10 "widget/pane" { x=8;   y=32; width=24; height=88; style=flat; bgcolor="#3060a0"; }
	sp_b10 "widget/pane" { x=42;  y=32; width=24; height=88; style=flat; bgcolor="#3060a0"; }
	// gap 19
	sp_a19 "widget/pane" { x=74;  y=32; width=24; height=88; style=flat; bgcolor="#3060a0"; }
	sp_b19 "widget/pane" { x=117; y=32; width=24; height=88; style=flat; bgcolor="#3060a0"; }
	// gap 20 -- last non-growing gap
	sp_a20 "widget/pane" { x=149; y=32; width=24; height=88; style=flat; bgcolor="#3060a0"; }
	sp_b20 "widget/pane" { x=193; y=32; width=24; height=88; style=flat; bgcolor="#3060a0"; }
	// gap 21 -- first growing gap
	sp_a21 "widget/pane" { x=225; y=32; width=24; height=88; style=flat; bgcolor="#a03030"; }
	sp_b21 "widget/pane" { x=270; y=32; width=24; height=88; style=flat; bgcolor="#a03030"; }
	// gap 30
	sp_a30 "widget/pane" { x=302; y=32; width=24; height=88; style=flat; bgcolor="#a03030"; }
	sp_b30 "widget/pane" { x=356; y=32; width=24; height=88; style=flat; bgcolor="#a03030"; }
	// gap 40
	sp_a40 "widget/pane" { x=388; y=32; width=24; height=88; style=flat; bgcolor="#a03030"; }
	sp_b40 "widget/pane" { x=452; y=32; width=24; height=88; style=flat; bgcolor="#a03030"; }

	sp_note "widget/label"
	    {
	    x=8; y=126; width=468; height=18;
	    text="blue = must stay put, red = may spread"; font_size=11; align=left; valign=middle;
	    fl_width=100;
	    }
	}

    // ---------------------------------------------------------------
    // Minimum width.  Blocks narrower than, at, and above APOS_MINWIDTH
    // (30px).  Shrink the window as far as it will go: nothing here
    // should collapse to zero width or overlap its neighbour.
    // ---------------------------------------------------------------
    minw_title "widget/label"
	{
	x=502; y=288; width=490; height=18;
	text="Minimum width (should never vanish or overlap)";
	font_size=12; align=left; valign=middle;
	}
    minw "widget/pane"
	{
	x=502; y=308; width=490; height=152;
	style=lowered; bgcolor="#909090";

	mw_t24 "widget/label" { x=62;  y=8; width=24; height=16; text="24"; font_size=11; align=center; valign=middle; fl_width=100; }
	mw_t28 "widget/label" { x=116; y=8; width=28; height=16; text="28"; font_size=11; align=center; valign=middle; fl_width=100; }
	mw_t30 "widget/label" { x=174; y=8; width=30; height=16; text="30"; font_size=11; align=center; valign=middle; fl_width=100; }
	mw_t32 "widget/label" { x=234; y=8; width=32; height=16; text="32"; font_size=11; align=center; valign=middle; fl_width=100; }
	mw_t40 "widget/label" { x=296; y=8; width=40; height=16; text="40"; font_size=11; align=center; valign=middle; fl_width=100; }
	mw_t60 "widget/label" { x=366; y=8; width=60; height=16; text="60"; font_size=11; align=center; valign=middle; fl_width=100; }

	mw_24 "widget/pane" { x=62;  y=32; width=24; height=88; style=flat; bgcolor="#7040a0"; }
	mw_28 "widget/pane" { x=116; y=32; width=28; height=88; style=flat; bgcolor="#7040a0"; }
	mw_30 "widget/pane" { x=174; y=32; width=30; height=88; style=flat; bgcolor="#4060b0"; }
	mw_32 "widget/pane" { x=234; y=32; width=32; height=88; style=flat; bgcolor="#4060b0"; }
	mw_40 "widget/pane" { x=296; y=32; width=40; height=88; style=flat; bgcolor="#4060b0"; }
	mw_60 "widget/pane" { x=366; y=32; width=60; height=88; style=flat; bgcolor="#4060b0"; }

	mw_note "widget/label"
	    {
	    x=8; y=126; width=474; height=18;
	    text="purple = narrower than the 30px floor"; font_size=11; align=left; valign=middle;
	    fl_width=100;
	    }
	}

    // ---------------------------------------------------------------
    // The automatic layout containers.  These position their own
    // children instead of using x/y, so they scale by a different code
    // path than everything above.
    // ---------------------------------------------------------------
    hbox_title "widget/label"
	{
	x=8; y=468; width=320; height=18;
	text="hbox (wraps at row_height)"; font_size=12; align=left; valign=middle;
	}
    hbox_frame "widget/pane"
	{
	x=8; y=488; width=320; height=200;
	style=lowered; bgcolor="#909090";

	the_hbox "widget/hbox"
	    {
	    x=8; y=8; width=302; height=182;
	    spacing=12; row_height=56;

	    hb1 "widget/pane" { width=56; height=48; style=flat; bgcolor="#c05050"; }
	    hb2 "widget/pane" { width=72; height=48; style=flat; bgcolor="#c08050"; }
	    hb3 "widget/pane" { width=48; height=48; style=flat; bgcolor="#c0b050"; }
	    hb4 "widget/pane" { width=64; height=48; style=flat; bgcolor="#70b050"; }
	    hb5 "widget/pane" { width=56; height=48; style=flat; bgcolor="#5090c0"; }
	    hb6 "widget/pane" { width=80; height=48; style=flat; bgcolor="#8060c0"; }
	    }
	}

    vbox_title "widget/label"
	{
	x=336; y=468; width=200; height=18;
	text="vbox (stacked)"; font_size=12; align=left; valign=middle;
	}
    vbox_frame "widget/pane"
	{
	x=336; y=488; width=200; height=200;
	style=lowered; bgcolor="#909090";

	the_vbox "widget/vbox"
	    {
	    x=8; y=8; width=182; height=182;
	    spacing=8; cellsize=28;

	    vb1 "widget/pane" { height=28; style=flat; bgcolor="#c05050"; }
	    vb2 "widget/pane" { height=28; style=flat; bgcolor="#c08050"; }
	    vb3 "widget/pane" { height=28; style=flat; bgcolor="#c0b050"; }
	    vb4 "widget/pane" { height=28; style=flat; bgcolor="#70b050"; }
	    }
	}

    al_title "widget/label"
	{
	x=544; y=468; width=220; height=18;
	text="autolayout + spacer"; font_size=12; align=left; valign=middle;
	}
    al_frame "widget/pane"
	{
	x=544; y=488; width=220; height=200;
	style=lowered; bgcolor="#909090";

	the_al "widget/autolayout"
	    {
	    style=vbox;
	    x=8; y=8; width=202; height=182;
	    spacing=6;

	    al1 "widget/pane" { height=32; style=flat; bgcolor="#5090c0"; }
	    al2 "widget/pane" { height=32; style=flat; bgcolor="#5090c0"; }

	    // Reserves empty space; never rendered itself.  The visible
	    // gap here should stay proportional to the others.
	    al_gap "widget/autolayoutspacer" { height=28; }

	    al3 "widget/pane" { height=32; style=flat; bgcolor="#8060c0"; }
	    al4 "widget/pane" { height=32; style=flat; bgcolor="#8060c0"; }
	    }
	}

    // Pane styles differ in how much border they reserve: everything
    // except "flat" takes 1px on each side, which nests.
    style_title "widget/label"
	{
	x=772; y=468; width=220; height=18;
	text="pane styles (border insets)"; font_size=12; align=left; valign=middle;
	}
    style_frame "widget/pane"
	{
	x=772; y=488; width=220; height=200;
	style=lowered; bgcolor="#909090";

	st_raised "widget/pane"
	    {
	    x=8; y=8; width=202; height=40; style=raised; bgcolor="#b0b0b0";
	    st_raised_l "widget/label" { x=4; y=4; width=194; height=16; text="raised"; font_size=11; align=left; valign=middle; fl_width=100; fl_height=100; }
	    }
	st_lowered "widget/pane"
	    {
	    x=8; y=54; width=202; height=40; style=lowered; bgcolor="#b0b0b0";
	    st_lowered_l "widget/label" { x=4; y=4; width=194; height=16; text="lowered"; font_size=11; align=left; valign=middle; fl_width=100; fl_height=100; }
	    }
	st_bordered "widget/pane"
	    {
	    x=8; y=100; width=202; height=40; style=bordered; bgcolor="#b0b0b0";
	    st_bordered_l "widget/label" { x=4; y=4; width=194; height=16; text="bordered"; font_size=11; align=left; valign=middle; fl_width=100; fl_height=100; }
	    }
	st_flat "widget/pane"
	    {
	    x=8; y=146; width=202; height=40; style=flat; bgcolor="#b0b0b0";
	    st_flat_l "widget/label" { x=4; y=4; width=194; height=16; text="flat (no inset)"; font_size=11; align=left; valign=middle; fl_width=100; fl_height=100; }
	    }
	}
    }
