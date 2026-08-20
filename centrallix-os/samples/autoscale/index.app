$Version=2$

// Autoscale visual test harness -- overview page.
//
// See README.md in this directory for what the harness is for and how to
// drive it.  This page holds the two pure-geometry references: if anything
// here looks wrong, the problem is in the layout engine itself rather than
// in any particular widget, so check this page before trusting the others.
index "widget/page"
    {
    title = "Autoscale Harness -- Overview";
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
	page_file="index.app";
	}

    // Server render geometry.  Every page carries this same note in the
    // same place: the strip to the right of the nav component.
    //
    // The value is decoded out of the cx__geom URL parameter, which is
    // the geometry the server was handed for this render.  cx__geom is
    // stripped from the address bar once the page loads, so this note is
    // the only way to see which size a page was rendered at after the
    // fact.  It does NOT track window resizing -- that is the point:
    // compare it against the current window size.
    //
    // A page's own ':index:width' will NOT work here: runserver()
    // expressions are evaluated as the widget tree is parsed, before
    // wgtrVerify() runs the layout, and the server-side evaluation
    // context holds only 'this' (the widget/parameter list), so a
    // reference to another widget silently binds to NULL.
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
    // Concentric rings.  Every ring is inset exactly 16px from its
    // parent on all four sides, and every ring is style=flat so it
    // reserves no border of its own and the nesting math stays exact.
    //
    // Correct scaling keeps all four margins of every ring equal to
    // each other.  A ring that goes lopsided, or whose margin collapses
    // to nothing, localizes the bug to a specific nesting depth.
    // ---------------------------------------------------------------
    rings_title "widget/label"
	{
	x=8; y=58; width=330; height=18;
	text="Concentric rings (margins should stay equal)";
	font_size=12; align=left; valign=middle;
	}
    rings "widget/pane"
	{
	x=8; y=78; width=330; height=610;
	style=lowered; bgcolor="#909090";

	ring1 "widget/pane"
	    {
	    x=16; y=16; width=296; height=576; style=flat; bgcolor="#c04040";
	    ring2 "widget/pane"
		{
		x=16; y=16; width=264; height=544; style=flat; bgcolor="#c08040";
		ring3 "widget/pane"
		    {
		    x=16; y=16; width=232; height=512; style=flat; bgcolor="#c0c040";
		    ring4 "widget/pane"
			{
			x=16; y=16; width=200; height=480; style=flat; bgcolor="#40c040";
			ring5 "widget/pane"
			    {
			    x=16; y=16; width=168; height=448; style=flat; bgcolor="#4080c0";
			    ring6 "widget/pane"
				{
				x=16; y=16; width=136; height=416; style=flat; bgcolor="#8040c0";
				}
			    }
			}
		    }
		}
	    }
	}

    // ---------------------------------------------------------------
    // Fixed-size markers and a centering cross.
    //
    // The corner and edge markers have fl_width=0 and fl_height=0, so
    // they must stay exactly the same pixel size at every window size
    // while staying pinned to their corner or edge.  The two bars must
    // stay centered.  A marker that grows, drifts inward, or gets
    // clipped is a scaling bug.
    // ---------------------------------------------------------------
    markers_title "widget/label"
	{
	x=346; y=58; width=330; height=18;
	text="Markers (fixed size, pinned to edges, bars stay centered)";
	font_size=12; align=left; valign=middle;
	}
    markers "widget/pane"
	{
	x=346; y=78; width=330; height=610;
	style=lowered; bgcolor="#909090";

	// Centering cross.  Each bar keeps its thickness (flex 0 across)
	// but spans the full width or height (flex 100 along).
	h_bar "widget/pane"
	    { x=0; y=302; width=328; height=4; style=flat; bgcolor="#303030"; fl_width=100; fl_height=0; }
	v_bar "widget/pane"
	    { x=162; y=0; width=4; height=608; style=flat; bgcolor="#303030"; fl_width=0; fl_height=100; }

	// Corners.
	c_tl "widget/pane"
	    { x=0;   y=0;   width=16; height=16; style=flat; bgcolor="#e00000"; fl_width=0; fl_height=0; }
	c_tr "widget/pane"
	    { x=312; y=0;   width=16; height=16; style=flat; bgcolor="#e0e000"; fl_width=0; fl_height=0; }
	c_bl "widget/pane"
	    { x=0;   y=592; width=16; height=16; style=flat; bgcolor="#00c000"; fl_width=0; fl_height=0; }
	c_br "widget/pane"
	    { x=312; y=592; width=16; height=16; style=flat; bgcolor="#0000e0"; fl_width=0; fl_height=0; }

	// Edge midpoints.
	e_top "widget/pane"
	    { x=157; y=0;   width=14; height=10; style=flat; bgcolor="#ffffff"; fl_width=0; fl_height=0; }
	e_bot "widget/pane"
	    { x=157; y=598; width=14; height=10; style=flat; bgcolor="#ffffff"; fl_width=0; fl_height=0; }
	e_lft "widget/pane"
	    { x=0;   y=297; width=10; height=14; style=flat; bgcolor="#ffffff"; fl_width=0; fl_height=0; }
	e_rgt "widget/pane"
	    { x=318; y=297; width=10; height=14; style=flat; bgcolor="#ffffff"; fl_width=0; fl_height=0; }

	// Dead centre, on top of the cross.
	centre "widget/pane"
	    { x=144; y=284; width=40; height=40; style=raised; bgcolor="#ff8000"; fl_width=0; fl_height=0; }
	}

    // ---------------------------------------------------------------
    // Links to existing sample apps that are worth eyeballing.  These
    // open in a separate window so the harness itself is never lost.
    // All of them run without a database.
    // ---------------------------------------------------------------
    links_title "widget/label"
	{
	x=684; y=58; width=310; height=18;
	text="Other resizable apps";
	font_size=12; align=left; valign=middle;
	}
    links "widget/pane"
	{
	x=684; y=78; width=310; height=610;
	style=lowered; bgcolor="#b0b0b0";

	links_box "widget/vbox"
	    {
	    x=8; y=8; width=292; cellsize=20; spacing=5;

	    lnk_samples "widget/textbutton"
		{
		text="samples index  (treeview + html)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_samples_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=800; Height=600; Source="/samples/index.app"; }
		}
	    lnk_autoscale "widget/textbutton"
		{
		text="autoscale_test  (flex grid)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_autoscale_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=900; Height=700; Source="/samples/autoscale_test.app"; }
		}
	    lnk_autoscale2 "widget/textbutton"
		{
		text="autoscale_test2  (minimal flex)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_autoscale2_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=900; Height=700; Source="/samples/autoscale_test2.app"; }
		}
	    lnk_scrollpane "widget/textbutton"
		{
		text="scrollpane_test  (scroll + resize)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_scrollpane_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=800; Height=600; Source="/samples/scrollpane_test.app"; }
		}
	    lnk_tabfeat "widget/textbutton"
		{
		text="tab_features  (tab layout by URL param)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_tabfeat_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=800; Height=600; Source="/samples/tab_features.app"; }
		}
	    lnk_fourtabs "widget/textbutton"
		{
		text="FourTabs  (linked tab controls)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_fourtabs_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=800; Height=600; Source="/samples/FourTabs.app"; }
		}
	    lnk_charttable "widget/textbutton"
		{
		text="chart_table_test  (chart + table)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_charttable_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=1000; Height=760; Source="/samples/chart_table_test.app"; }
		}
	    lnk_window "widget/textbutton"
		{
		text="window_test  (overlapping windows)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_window_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=900; Height=700; Source="/samples/window_test.app"; }
		}
	    lnk_connector "widget/textbutton"
		{
		text="connector_test  (densest page in repo)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_connector_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=1000; Height=760; Source="/tests/ui/connector/connector_test.app"; }
		}
	    lnk_repeat "widget/textbutton"
		{
		text="repeat_test  (widget gallery)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_repeat_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=900; Height=700; Source="/tests/ui/repeat/repeat_test.app"; }
		}
	    lnk_ors "widget/textbutton"
		{
		text="ors  (tabs, tree, forms, scrollpane)"; tristate=no;
		background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		lnk_ors_c "widget/connector"
		    { event=Click; target=index; action=Launch; Width=1000; Height=760; Source="/samples/ors.app"; }
		}
	    }
	}
    }
