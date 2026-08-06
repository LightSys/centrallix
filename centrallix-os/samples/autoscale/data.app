$Version=2$

// Autoscale visual test harness -- data-driven widgets page.
//
// Everything here is fed from the CSV files in this directory, so no
// database is needed.  These widgets redraw their own contents in response
// to a resize rather than just being repositioned, which makes them the
// most likely place for a scaling bug to show up as wrong content instead
// of wrong geometry.
data "widget/page"
    {
    title = "Autoscale Harness -- Data";
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
	page_file="data.app";
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
    // Table.  Column widths sum to a little under the visible width so the
    // rightmost column sits against the scrollbar gutter, which is where
    // column clipping shows up first.  The ruler and filler columns are
    // wider than their content box on purpose.
    // ---------------------------------------------------------------
    table_title "widget/label"
	{
	x=8; y=58; width=600; height=18;
	text="table over rows.csv (48 rows; watch the right-hand columns)";
	font_size=12; align=left; valign=middle;
	}
    table_frame "widget/pane"
	{
	x=8; y=78; width=600; height=222;
	style=lowered; bgcolor="#909090";

	table_osrc "widget/osrc"
	    {
	    sql = "select :rid, :tag, :band, :level, :amount, :ruler, :filler from /samples/autoscale/rows.csv/rows";
	    baseobj = "/samples/autoscale/rows.csv/rows";
	    replicasize = 48;
	    readahead = 16;
	    autoquery = onload;

	    the_table "widget/table"
		{
		x=2; y=2; width=594; height=216;
		objectsource = table_osrc;
		rowheight = 18;
		hdr_bgcolor = "#d0d0d0";
		row1_bgcolor = "#ffffff";
		row2_bgcolor = "#e8e8e8";
		rowhighlight_bgcolor = "#000080";
		textcolor = "#000000";
		textcolorhighlight = "#ffffff";
		allow_selection = yes;
		demand_scrollbar = yes;
		gridinemptyrows = 1;
		colsep = 0;

		tc_tag    "widget/table-column" { fieldname=tag;    title="tag";    width=52;  }
		tc_band   "widget/table-column" { fieldname=band;   title="band";   width=74;  }
		tc_level  "widget/table-column" { fieldname=level;  title="level";  width=48;  align=right; }
		tc_amount "widget/table-column" { fieldname=amount; title="amount"; width=66;  align=right; }
		tc_ruler  "widget/table-column" { fieldname=ruler;  title="ruler";  width=170; }
		tc_filler "widget/table-column" { fieldname=filler; title="filler"; width=150; }
		}
	    }
	}

    // ---------------------------------------------------------------
    // Treeview.  Its labels come from rows.spec's row_annot_exp and are
    // deliberately too long, so they must truncate at the widget's right
    // edge and show a tooltip only when actually truncated.  The Depth
    // branch nests eight levels, shrinking the usable label width at each
    // level, so one screen exercises the clip maths at every indent.
    // ---------------------------------------------------------------
    tree_title "widget/label"
	{
	x=616; y=58; width=376; height=18;
	text="treeview (labels truncate; Depth branch nests 8 deep)";
	font_size=12; align=left; valign=middle;
	}
    tree_frame "widget/pane"
	{
	x=616; y=78; width=376; height=222;
	style=lowered; bgcolor="#ffffff";

	tree_scroll "widget/scrollpane"
	    {
	    x=2; y=2; width=372; height=218;
	    bgcolor="#ffffff";

	    the_tree "widget/treeview"
		{
		x=0; y=0; width=352;
		source="/samples/autoscale/tree.qyt/";
		show_root=no;
		show_root_branch=yes;
		show_branches=yes;
		use_3d_lines=no;
		}
	    }
	}

    // ---------------------------------------------------------------
    // Charts.  The three series have deliberately simple shapes -- a
    // straight rise, a straight fall and a single triangular bump -- so a
    // chart that redraws wrongly after a resize stops looking like those
    // shapes.  Both charts read the same objectsource.
    // ---------------------------------------------------------------
    chart_title "widget/label"
	{
	x=8; y=310; width=668; height=18;
	text="charts over series.csv (shapes: rise, fall, single bump)";
	font_size=12; align=left; valign=middle;
	}
    chart_frame "widget/pane"
	{
	x=8; y=330; width=668; height=358;
	style=lowered; bgcolor="#e8e8e8";

	chart_osrc "widget/osrc"
	    {
	    sql = "select :label, :rising, :falling, :bump from /samples/autoscale/series.csv/rows";
	    replicasize = 12;
	    readahead = 12;
	    autoquery = onload;

	    bar_chart "widget/chart"
		{
		x=2; y=2; width=328; height=352;
		chart_type="bar";
		title="bar: rise + fall"; titlecolor="black"; title_size=12;
		legend_position="bottom";
		start_at_zero=yes;

		bc_rise "widget/chart-series" { label="rising";  color="#4060b0"; x_column="label"; y_column="rising";  }
		bc_fall "widget/chart-series" { label="falling"; color="#b04040"; x_column="label"; y_column="falling"; }
		bc_x "widget/chart-axis" { axis="x"; label="point"; }
		bc_y "widget/chart-axis" { axis="y"; label="value"; }
		}

	    line_chart "widget/chart"
		{
		x=336; y=2; width=328; height=352;
		chart_type="line";
		title="line: single bump"; titlecolor="black"; title_size=12;
		legend_position="bottom";
		start_at_zero=yes;

		lc_bump "widget/chart-series" { label="bump"; color="#308030"; x_column="label"; y_column="bump"; fill=no; }
		lc_x "widget/chart-axis" { axis="x"; label="point"; }
		lc_y "widget/chart-axis" { axis="y"; label="value"; }
		}
	    }
	}

    // ---------------------------------------------------------------
    // Repeat.  This one is expanded on the SERVER while the widget tree
    // is being built, so its copies are ordinary widgets by the time the
    // layout engine runs and should scale like any hand-written set.
    // Field values therefore use runserver(), not runclient().
    // ---------------------------------------------------------------
    repeat_title "widget/label"
	{
	x=684; y=310; width=308; height=18;
	text="repeat (expanded server-side)";
	font_size=12; align=left; valign=middle;
	}
    repeat_frame "widget/pane"
	{
	x=684; y=330; width=308; height=358;
	style=lowered; bgcolor="#b0b0b0";

	repeat_box "widget/vbox"
	    {
	    x=8; y=8; width=290; height=342;
	    spacing=8; cellsize=28;

	    the_repeat "widget/repeat"
		{
		sql = "select :band from /samples/autoscale/bands.csv/rows";

		rp_btn "widget/textbutton"
		    {
		    width=290; height=28;
		    text = runserver(:the_repeat:band);
		    tristate=no;
		    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		    }
		}
	    }
	}

    }
