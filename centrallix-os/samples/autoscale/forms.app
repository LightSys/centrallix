$Version=2$

// Autoscale visual test harness -- form widgets page.
//
// The bound half of this page runs against rows.csv through an osrc, so the
// form navigation actually works: step through records with the formbar and
// watch the fields refill.  The ruler and filler fields are long on purpose
// so you can count characters to see exactly where an edit box clips.
//
// The unbound half holds the widgets that need no data source at all.
forms "widget/page"
    {
    title = "Autoscale Harness -- Forms";
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
	page_file="forms.app";
	}

    // Server render geometry.  Every page carries this same note in the
    // same place: the strip to the right of the nav component.
    //
    // ':forms:width' and ':forms:height' are the page's COMPUTED
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
    // Data-bound form over rows.csv.
    // ---------------------------------------------------------------
    bound_title "widget/label"
	{
	x=8; y=58; width=600; height=18;
	text="Bound form over rows.csv (formbar steps through 48 records)";
	font_size=12; align=left; valign=middle;
	}
    bound_frame "widget/pane"
	{
	x=8; y=78; width=600; height=250;
	style=raised; bgcolor="#b8b8b8";

	form_osrc "widget/osrc"
	    {
	    sql = "select :rid, :tag, :band, :level, :amount, :ruler, :filler from /samples/autoscale/rows.csv/rows";
	    baseobj = "/samples/autoscale/rows.csv/rows";
	    replicasize = 48;
	    readahead = 12;
	    autoquery = onload;

	    the_form "widget/form"
		{
		f_tag_l "widget/label"   { x=8; y=10; width=70; height=20; text="tag"; font_size=12; align=left; valign=middle; }
		f_tag "widget/editbox"   { x=82; y=8; width=180; height=22; fieldname="tag"; style=lowered; bgcolor="white"; }

		f_band_l "widget/label"  { x=8; y=38; width=70; height=20; text="band"; font_size=12; align=left; valign=middle; }
		f_band "widget/editbox"  { x=82; y=36; width=180; height=22; fieldname="band"; style=lowered; bgcolor="white"; }

		f_lvl_l "widget/label"   { x=8; y=66; width=70; height=20; text="level"; font_size=12; align=left; valign=middle; }
		f_lvl "widget/editbox"   { x=82; y=64; width=180; height=22; fieldname="level"; style=lowered; bgcolor="white"; }

		f_amt_l "widget/label"   { x=8; y=94; width=70; height=20; text="amount"; font_size=12; align=left; valign=middle; }
		f_amt "widget/editbox"   { x=82; y=92; width=180; height=22; fieldname="amount"; style=lowered; bgcolor="white"; }

		// Wide fields: the content is longer than the box, so the
		// text must clip cleanly at the right edge at every size.
		f_rul_l "widget/label"   { x=8; y=122; width=70; height=20; text="ruler"; font_size=12; align=left; valign=middle; }
		f_rul "widget/editbox"   { x=82; y=120; width=500; height=22; fieldname="ruler"; style=lowered; bgcolor="white"; }

		f_fil_l "widget/label"   { x=8; y=150; width=70; height=20; text="filler"; font_size=12; align=left; valign=middle; }
		f_fil "widget/editbox"   { x=82; y=148; width=500; height=22; fieldname="filler"; style=lowered; bgcolor="white"; }

		// Current mode of the form.
		f_status "widget/formstatus" { x=290; y=8; style=large; form=the_form; }

		// Record navigation.  This widget is hardcoded to 240x30 in
		// its driver, so it must NOT change size when the window does.
		f_bar "widget/formbar" { x=290; y=40; target=the_form; }

		// Bound dropdown fed by the same objectsource.
		f_band_dd "widget/dropdown"
		    {
		    x=290; y=88; width=290; height=22;
		    mode=objectsource; fieldname="band";
		    hilight="#b5b5b5"; bgcolor="#c0c0c0";
		    }

		f_new "widget/textbutton"
		    {
		    x=8; y=182; width=64; height=20; text="New"; tristate=no;
		    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		    f_new_c "widget/connector" { event=Click; target=the_form; action=New; }
		    }
		f_save "widget/textbutton"
		    {
		    x=78; y=182; width=64; height=20; text="Save"; tristate=no;
		    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		    f_save_c "widget/connector" { event=Click; target=the_form; action=Save; }
		    }
		f_discard "widget/textbutton"
		    {
		    x=148; y=182; width=74; height=20; text="Discard"; tristate=no;
		    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		    f_discard_c "widget/connector" { event=Click; target=the_form; action=Discard; }
		    }
		f_query "widget/textbutton"
		    {
		    x=228; y=182; width=64; height=20; text="Query"; tristate=no;
		    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		    f_query_c "widget/connector" { event=Click; target=the_form; action=Query; }
		    }
		f_qexec "widget/textbutton"
		    {
		    x=298; y=182; width=84; height=20; text="Run query"; tristate=no;
		    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
		    f_qexec_c "widget/connector" { event=Click; target=the_form; action=QueryExec; }
		    }
		}
	    }
	}

    // ---------------------------------------------------------------
    // Widgets that need no data source.
    // ---------------------------------------------------------------
    unbound_title "widget/label"
	{
	x=616; y=58; width=376; height=18;
	text="Unbound: static dropdown, checkboxes, datetime";
	font_size=12; align=left; valign=middle;
	}
    // The captions below carry fl_height=1.  widget/label defaults to 0, and
    // a rigid widget pins every row it crosses, which left this pane with too
    // little flexible height to grow at all -- freezing bound_frame too, since
    // they share a page row.  The checkboxes and datetime stay rigid.
    unbound_frame "widget/pane"
	{
	x=616; y=78; width=376; height=250;
	style=raised; bgcolor="#b8b8b8";

	u_dd_l "widget/label" { x=8; y=10; width=110; height=20; text="static dropdown"; font_size=12; align=left; valign=middle; fl_height=1; }
	u_dd "widget/dropdown"
	    {
	    x=8; y=32; width=356; height=22;
	    hilight="#b5b5b5"; bgcolor="#c0c0c0"; numdisplay=4;

	    u_dd1 "widget/dropdownitem" { label="ALPHA -- short"; value="1"; }
	    u_dd2 "widget/dropdownitem" { label="BRAVO -- a much longer entry 0....5...10...15...20"; value="2"; }
	    u_dd3 "widget/dropdownitem" { label="CHARLIE"; value="3"; }
	    u_dd4 "widget/dropdownitem" { label="DELTA"; value="4"; }
	    u_dd5 "widget/dropdownitem" { label="ECHO"; value="5"; }
	    }

	// Checkboxes are a fixed 13x13 image in the driver, so these must
	// stay exactly the same size however the window is resized.
	u_cb_l "widget/label" { x=8; y=64; width=200; height=20; text="checkboxes (fixed 13x13)"; font_size=12; align=left; valign=middle; fl_height=1; }
	u_cb_on "widget/checkbox"    { x=10; y=88; checked=yes; }
	u_cb_on_l "widget/label"     { x=30; y=86; width=90; height=18; text="checked"; font_size=11; align=left; valign=middle; }
	u_cb_off "widget/checkbox"   { x=130; y=88; checked=no; }
	u_cb_off_l "widget/label"    { x=150; y=86; width=90; height=18; text="unchecked"; font_size=11; align=left; valign=middle; }
	u_cb_null "widget/checkbox"  { x=250; y=88; }
	u_cb_null_l "widget/label"   { x=270; y=86; width=90; height=18; text="null"; font_size=11; align=left; valign=middle; }

	u_dt_l "widget/label" { x=8; y=116; width=200; height=20; text="datetime"; font_size=12; align=left; valign=middle; fl_height=1; }
	u_dt "widget/datetime"
	    {
	    x=8; y=138; width=356; height=44;
	    bgcolor="#ffffff"; fgcolor="#000000";
	    initialdate="15 Mar 2026";
	    date_only=no; default_time="09:00:00"; search_by_range=no;
	    }

	// Presentation hints attach to the widget they sit inside.
	u_hint_l "widget/label" { x=8; y=190; width=200; height=20; text="editbox with hints (max 10 chars)"; font_size=12; align=left; valign=middle; fl_height=1; }
	u_hint_eb "widget/editbox"
	    {
	    x=8; y=212; width=356; height=22;
	    style=lowered; bgcolor="white";
	    empty_description="type here -- hints cap this at 10";
	    u_hints "widget/hints" { length=10; }
	    }
	}

    // ---------------------------------------------------------------
    // Text entry variants.
    // ---------------------------------------------------------------
    text_title "widget/label"
	{
	x=8; y=336; width=600; height=18;
	text="Text entry: textarea and editbox variants";
	font_size=12; align=left; valign=middle;
	}
    text_frame "widget/pane"
	{
	x=8; y=356; width=600; height=180;
	style=raised; bgcolor="#b8b8b8";

	t_ta_l "widget/label" { x=8; y=8; width=280; height=18; text="textarea (scrolls)"; font_size=12; align=left; valign=middle; }
	t_ta "widget/textarea"
	    {
	    x=8; y=28; width=284; height=140;
	    style=lowered; mode=text;
	    value="LINE 01 0....5...10...15...20...25...30 END-01
LINE 02 0....5...10...15...20...25...30 END-02
LINE 03 0....5...10...15...20...25...30 END-03
LINE 04 0....5...10...15...20...25...30 END-04
LINE 05 0....5...10...15...20...25...30 END-05
LINE 06 0....5...10...15...20...25...30 END-06
LINE 07 0....5...10...15...20...25...30 END-07
LINE 08 LAST LINE -- END OF TEXTAREA";
	    }

	t_ro_l "widget/label" { x=300; y=8; width=290; height=18; text="readonly"; font_size=12; align=left; valign=middle; }
	t_ro "widget/editbox"
	    {
	    x=300; y=28; width=290; height=22;
	    style=lowered; bgcolor="#d8d8d8"; readonly=yes;
	    value="READONLY 0....5...10...15...20...25 END";
	    }

	t_max_l "widget/label" { x=300; y=58; width=290; height=18; text="max_chars=12"; font_size=12; align=left; valign=middle; }
	t_max "widget/editbox"
	    {
	    x=300; y=78; width=290; height=22;
	    style=lowered; bgcolor="white"; max_chars=12;
	    empty_description="at most 12 characters";
	    }

	t_desc_l "widget/label" { x=300; y=108; width=290; height=18; text="empty_description + raised style"; font_size=12; align=left; valign=middle; }
	t_desc "widget/editbox"
	    {
	    x=300; y=128; width=290; height=22;
	    style=raised; bgcolor="white";
	    empty_description="placeholder text shown while empty";
	    description_fgcolor="#808080";
	    }
	}

    // ---------------------------------------------------------------
    // Radio button panel.  Its driver refuses to render without a title.
    // ---------------------------------------------------------------
    radio_title "widget/label"
	{
	x=616; y=336; width=376; height=18;
	text="radiobuttonpanel (title is mandatory)";
	font_size=12; align=left; valign=middle;
	}
    radio_frame "widget/pane"
	{
	x=616; y=356; width=376; height=180;
	style=raised; bgcolor="#b8b8b8";

	r_panel "widget/radiobuttonpanel"
	    {
	    x=8; y=8; width=356; height=160;
	    title="Pick a band";
	    bgcolor="#ffffff"; textcolor="black"; spacing=10;

	    r_a "widget/radiobutton" { label="ALPHA"; value="1"; selected="true"; }
	    r_b "widget/radiobutton" { label="BRAVO"; value="2"; }
	    r_c "widget/radiobutton" { label="CHARLIE -- longer label 0....5...10...15"; value="3"; }
	    r_d "widget/radiobutton" { label="DELTA"; value="4"; }
	    }
	}

    // ---------------------------------------------------------------
    // File upload.  Its driver reads no geometry at all, so it lands
    // wherever the browser puts it -- that is expected, not a bug.
    // Uploads are written into the ObjectSystem's own tmp directory.
    // ---------------------------------------------------------------
    upload_title "widget/label"
	{
	x=8; y=544; width=984; height=18;
	text="fileupload (driver reads no x/y/width/height, so it flows inline)";
	font_size=12; align=left; valign=middle;
	}
    upload_frame "widget/pane"
	{
	x=8; y=564; width=984; height=126;
	style=raised; bgcolor="#b8b8b8";

	up_input "widget/fileupload"
	    {
	    multiselect=yes;
	    fieldname="file";
	    target="/tmp//";
	    }

	up_reset "widget/textbutton"
	    {
	    x=8; y=60; width=90; height=20; text="Clear"; tristate=no;
	    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;

	    // The client dispatches "Clear"; the server-registered "Reset"
	    // name is vestigial and does nothing.
	    up_reset_c "widget/connector" { event=Click; target=up_input; action=Clear; }
	    }
	up_submit "widget/textbutton"
	    {
	    x=104; y=60; width=90; height=20; text="Submit"; tristate=no;
	    background="/sys/images/grey_gradient.png"; fgcolor1=black; fgcolor2=white;
	    up_submit_c "widget/connector" { event=Click; target=up_input; action=Submit; }
	    }
	up_note "widget/label"
	    {
	    x=204; y=58; width=770; height=36; allow_break=yes; font_size=11; align=left; valign=top;
	    text="Uploads land in /tmp inside the ObjectSystem tree. This widget is here to confirm it still renders and does not disturb the widgets around it.";
	    }
	}
    }
