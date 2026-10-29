$Version=2$
popup_scroll_test "widget/page"
    {
    title = "Popups in a Scrollpane";
    bgcolor = "#ffffff";
    x = 0; y = 0;
    width = 640; height = 480;

    // Description:
    // Each widget in the scrollpane draws a popup at page level. Open one,
    // then scroll (wheel, scrollbar, or the auto-scroll timer) and check that
    // the popup stays attached to its widget. The buttons on the right open
    // popups that point at widgets inside the scrollpane.

    title_lbl "widget/label"
	{
	x = 10; y = 10; width = 380; height = 20;
	font_size = 14; style = bold;
	text = "Open a popup, then scroll the green pane.";
	}

    scroll "widget/scrollpane"
	{
	x = 10; y = 40;
	width = 380; height = 300;
	bgcolor = "#c0f1ba";

	// Stretches the content so it can scroll.
	spacer "widget/pane"
	    {
	    x = 0; y = 0;
	    width = 0; height = 1200;
	    fl_height = 0;
	    style = flat;
	    }

	form1 "widget/form"
	    {
	    dd_top_lbl "widget/label" { x = 10; y = 10; width = 200; height = 20; text = "Dropdown (top):"; }
	    dd_top "widget/dropdown"
		{
		x = 10; y = 30; width = 150; height = 20;
		bgcolor = "#ffffff"; highlight = "#cfcfcf";
		mode = static;
		dd_top_1 "widget/dropdownitem" { label = "Red"; value = "red"; }
		dd_top_2 "widget/dropdownitem" { label = "Green"; value = "green"; }
		dd_top_3 "widget/dropdownitem" { label = "Blue"; value = "blue"; }
		dd_top_4 "widget/dropdownitem" { label = "White"; value = "white"; }
		}

	    dt_top_lbl "widget/label" { x = 190; y = 10; width = 180; height = 20; text = "Datetime (top):"; }
	    dt_top "widget/datetime"
		{
		x = 190; y = 30; width = 170; height = 20;
		bgcolor = "#ffffff"; fgcolor = "#000000";
		}

	    eb_lbl "widget/label" { x = 10; y = 70; width = 350; height = 20; text = "Editbox (shows the focus box when clicked):"; }
	    eb "widget/editbox"
		{
		x = 10; y = 90; width = 200; height = 20;
		bgcolor = "#ffffff";
		}

	    // Near the bottom edge of the visible area, so the popups overhang it.
	    dd_edge_lbl "widget/label" { x = 10; y = 255; width = 200; height = 20; text = "Dropdown (bottom edge):"; }
	    dd_edge "widget/dropdown"
		{
		x = 10; y = 275; width = 150; height = 20;
		bgcolor = "#ffffff"; highlight = "#cfcfcf";
		mode = static;
		numdisplay = 5;
		dd_edge_1 "widget/dropdownitem" { label = "One"; value = "1"; }
		dd_edge_2 "widget/dropdownitem" { label = "Two"; value = "2"; }
		dd_edge_3 "widget/dropdownitem" { label = "Three"; value = "3"; }
		dd_edge_4 "widget/dropdownitem" { label = "Four"; value = "4"; }
		dd_edge_5 "widget/dropdownitem" { label = "Five"; value = "5"; }
		dd_edge_6 "widget/dropdownitem" { label = "Six"; value = "6"; }
		dd_edge_7 "widget/dropdownitem" { label = "Seven"; value = "7"; }
		dd_edge_8 "widget/dropdownitem" { label = "Eight"; value = "8"; }
		}

	    dt_edge_lbl "widget/label" { x = 190; y = 255; width = 180; height = 20; text = "Datetime (bottom edge):"; }
	    dt_edge "widget/datetime"
		{
		x = 190; y = 275; width = 170; height = 20;
		bgcolor = "#ffffff"; fgcolor = "#000000";
		}

	    // Only reachable by scrolling.
	    dd_mid_lbl "widget/label" { x = 10; y = 580; width = 200; height = 20; text = "Dropdown (middle):"; }
	    dd_mid "widget/dropdown"
		{
		x = 10; y = 600; width = 150; height = 20;
		bgcolor = "#ffffff"; highlight = "#cfcfcf";
		mode = static;
		dd_mid_1 "widget/dropdownitem" { label = "Alpha"; value = "a"; }
		dd_mid_2 "widget/dropdownitem" { label = "Beta"; value = "b"; }
		dd_mid_3 "widget/dropdownitem" { label = "Gamma"; value = "c"; }
		}

	    dt_mid_lbl "widget/label" { x = 190; y = 580; width = 180; height = 20; text = "Datetime (middle):"; }
	    dt_mid "widget/datetime"
		{
		x = 190; y = 600; width = 170; height = 20;
		bgcolor = "#ffffff"; fgcolor = "#000000";
		}

	    dd_bot_lbl "widget/label" { x = 10; y = 1120; width = 200; height = 20; text = "Dropdown (end of content):"; }
	    dd_bot "widget/dropdown"
		{
		x = 10; y = 1140; width = 150; height = 20;
		bgcolor = "#ffffff"; highlight = "#cfcfcf";
		mode = static;
		dd_bot_1 "widget/dropdownitem" { label = "North"; value = "n"; }
		dd_bot_2 "widget/dropdownitem" { label = "South"; value = "s"; }
		dd_bot_3 "widget/dropdownitem" { label = "East"; value = "e"; }
		dd_bot_4 "widget/dropdownitem" { label = "West"; value = "w"; }
		}
	    }

	mn_lbl "widget/label" { x = 10; y = 400; width = 350; height = 20; text = "Menu bar (click File or Edit):"; }
	mn "widget/menu"
	    {
	    x = 10; y = 420; width = 250;
	    bgcolor = "#afafaf";
	    highlight_bgcolor = "#cfcfcf";
	    active_bgcolor = "#efefef";

	    mn_file "widget/menu"
		{
		label = "File";
		bgcolor = "#afafaf";
		highlight_bgcolor = "#cfcfcf";
		active_bgcolor = "#efefef";
		row_height = 18;
		mn_file_new "widget/menuitem" { label = "New..."; }
		mn_file_open "widget/menuitem" { label = "Open..."; }
		mn_file_recent "widget/menu"
		    {
		    label = "Recent Files";
		    bgcolor = "#afafaf";
		    highlight_bgcolor = "#cfcfcf";
		    active_bgcolor = "#efefef";
		    row_height = 18;
		    mn_file_r1 "widget/menuitem" { label = "SomeFile.txt"; }
		    mn_file_r2 "widget/menuitem" { label = "A_Document.sdw"; }
		    }
		}
	    mn_edit "widget/menu"
		{
		label = "Edit";
		bgcolor = "#afafaf";
		highlight_bgcolor = "#cfcfcf";
		active_bgcolor = "#efefef";
		row_height = 18;
		mn_edit_cut "widget/menuitem" { label = "Cut"; }
		mn_edit_copy "widget/menuitem" { label = "Copy"; }
		mn_edit_paste "widget/menuitem" { label = "Paste"; }
		}
	    }

	// Targets for the pointing buttons on the right.
	target_a "widget/pane"
	    {
	    x = 200; y = 140; width = 120; height = 40;
	    bgcolor = "#e6b2c4";
	    target_a_lbl "widget/label" { x = 5; y = 10; width = 100; height = 20; text = "Target A"; }
	    }
	target_b "widget/pane"
	    {
	    x = 200; y = 800; width = 120; height = 40;
	    bgcolor = "#e6d2c4";
	    target_b_lbl "widget/label" { x = 5; y = 10; width = 100; height = 20; text = "Target B"; }
	    }
	}

    // Right side: controls.
    ctl_lbl "widget/label"
	{
	x = 410; y = 40; width = 220; height = 20;
	font_size = 14; style = bold;
	text = "Controls";
	}

    // Scrolls between two offsets while the timer runs, so a popup can stay
    // open during a programmatic scroll.
    flip "widget/variable" { type = integer; value = runclient(0); }
    auto_tm "widget/timer"
	{
	msec = 1000;
	auto_reset = yes;
	auto_start = no;
	auto_tm_scroll "widget/connector"
	    {
	    event = Expire;
	    target = scroll;
	    action = ScrollTo;
	    Offset = runclient(condition(:flip:value == 0, 120, 0));
	    }
	auto_tm_flip "widget/connector"
	    {
	    event = Expire;
	    target = flip;
	    action = SetValue;
	    Value = runclient(condition(:flip:value == 0, 1, 0));
	    }
	}
    btn_auto_on "widget/textbutton"
	{
	x = 410; y = 70; width = 105; height = 30;
	text = "Auto-scroll";
	bgcolor = "#0c0447";
	btn_auto_on_c "widget/connector" { event = Click; target = auto_tm; action = SetTimer; Time = 1000; AutoReset = 1; }
	}
    btn_auto_off "widget/textbutton"
	{
	x = 525; y = 70; width = 105; height = 30;
	text = "Stop";
	bgcolor = "#0c0447";
	btn_auto_off_c "widget/connector" { event = Click; target = auto_tm; action = CancelTimer; }
	}

    btn_top "widget/textbutton"
	{
	x = 410; y = 110; width = 105; height = 30;
	text = "To top";
	bgcolor = "#241672";
	btn_top_c "widget/connector" { event = Click; target = scroll; action = ScrollTo; Offset = 0; }
	}
    btn_100 "widget/textbutton"
	{
	x = 525; y = 110; width = 105; height = 30;
	text = "To 100px";
	bgcolor = "#241672";
	btn_100_c "widget/connector" { event = Click; target = scroll; action = ScrollTo; Offset = 100; }
	}

    btn_win_a "widget/textbutton"
	{
	x = 410; y = 160; width = 220; height = 30;
	text = "Window: point at A";
	bgcolor = "#3a2a8a";
	btn_win_a_c "widget/connector" { event = Click; target = point_win; action = Open; PointAt = "target_a"; PointSide = runclient("right"); }
	}
    btn_win_b "widget/textbutton"
	{
	x = 410; y = 200; width = 220; height = 30;
	text = "Window: point at B";
	bgcolor = "#3a2a8a";
	btn_win_b_c "widget/connector" { event = Click; target = point_win; action = Open; PointAt = "target_b"; PointSide = runclient("right"); }
	}
    btn_pop_a "widget/textbutton"
	{
	x = 410; y = 240; width = 220; height = 30;
	text = "Window: pop to A";
	bgcolor = "#3a2a8a";
	btn_pop_a_c "widget/connector" { event = Click; target = pop_win; action = Popup; PopTo = "target_a"; }
	}
    btn_pane_a "widget/textbutton"
	{
	x = 410; y = 280; width = 220; height = 30;
	text = "Pane: point at A";
	bgcolor = "#3a2a8a";
	btn_pane_a_c "widget/connector" { event = Click; target = point_pane; action = Point; AtWidget = "target_a"; BorderColor = "#000000"; FillColor = "#ffffc0"; }
	}

    // Over the scrollpane, just below Target A, so it is within reach of the point.
    point_pane "widget/pane"
	{
	x = 120; y = 225; width = 250; height = 50;
	bgcolor = "#ffffc0";
	point_pane_lbl "widget/label" { x = 5; y = 5; width = 240; height = 40; text = "This pane points at Target A after its button is clicked."; }
	}

    point_win "widget/childwindow"
	{
	x = 410; y = 100; width = 180; height = 100;
	style = dialog;
	title = "Pointing Window";
	bgcolor = "#e0e0e0";
	visible = no;
	toplevel = yes;
	point_win_lbl "widget/label" { x = 10; y = 10; width = 160; height = 40; text = "Scroll the pane; I should follow my target."; }
	}

    pop_win "widget/childwindow"
	{
	x = 410; y = 100; width = 180; height = 100;
	style = dialog;
	title = "Popup Window";
	bgcolor = "#e0e0e0";
	visible = no;
	toplevel = yes;
	pop_win_lbl "widget/label" { x = 10; y = 10; width = 160; height = 40; text = "Opened with Popup PopTo."; }
	}
    }
