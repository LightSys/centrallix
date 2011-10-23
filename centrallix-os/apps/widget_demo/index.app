$Version=2$
widget_demo "widget/page"
    {
    title = "Centrallix Widget Demo";
    width=640; height=480;
    background="brushed1.png";

    main_hbox "widget/hbox"
	{
	x=16; y=16; width=608; height=448;
	spacing=16;
	widget_list_pane "widget/pane"
	    {
	    width = 200;
	    style = lowered;
	    bgcolor = "#e0e0e0";

	    list_osrc "widget/osrc"
		{
		sql = "select :name, wname = substring(:name, 1, char_length(:name) - 4) from /apps/widget_demo/widgets where right(:name, 4) = '.cmp'";
		autoquery=onload;
		replicasize=100;
		readahead=20;

		list_form "widget/form"
		    {
		    left_vbox "widget/vbox"
			{
			x=8;y=8;width=184;height=446;
			cellspacing=4;
			cellsize=20;

//			list_lbl_1 "widget/label" { text = "Name of Widget:"; }
//
//			widget_eb "widget/editbox"
//			    {
//			    fieldname = "wname";
//			    bgcolor="white";
//			    }
//
//			list_sp1 "widget/autolayoutspacer" { height=8; }

			list_lbl_2 "widget/label" { style=bold; text = "Select a Widget:"; }

			list_table "widget/table"
			    {
			    height=410;
			    mode=dynamicrow;
			    row1_bgcolor = "#ffffff";
			    row2_bgcolor = "#e8e8e8";
			    rowhighlight_bgcolor = "#000080";
			    rowheight=20;
			    hdr_bgcolor = "#d0d0d0";
			    textcolorhighlight = "#ffffff";
			    textcolor="#000000";
			    gridinemptyrows=1;
			    colsep = 0;
			    demand_scrollbar = yes;
			    overlap_scrollbar = yes;
			    wname "widget/table-column" { fieldname="wname"; title="Widget"; width=84; }
			    wname_cmp "widget/table-column" { fieldname="name"; title="Component"; width=100; }
			    }
			}
		    }

		sync_src "widget/connector"
		    {
		    event="DataFocusChanged";
		    target=info_html;
		    action="LoadPage";
		    Source=runclient("/apps/widget_demo/widgets/" + :list_osrc:wname + '.cmp?ls__type=text%2fplain');
		    }

		sync_widget "widget/connector"
		    {
		    event = "DataFocusChanged";
		    target = widget_cmp;
		    action = "Instantiate";

		    // param to instantiate widget
		    which = runclient(:list_osrc:wname);
		    }
		}
	    }
	widget_demo_pane "widget/pane"
	    {
	    width=392;
	    style=lowered;
	    bgcolor="#e0e0e0";

	    src_lbl "widget/label" { x=8;y=8;width=200;height=20;text = "Sample Source Code:"; style=bold; }

	    infopane "widget/pane"
		{
		x=8;y=28;width=374;height=120;
		style=lowered;
		bgcolor="#ffffff";

		info_scroll "widget/scrollpane"
		    {
		    x=0;y=0;height=118;width=372;

		    info_html "widget/html"
			{
			x=1;y=0;width = 354;
			mode=dynamic;
			source="/samples/welcome.html";
			}
		    }
		}

	    ren_lbl "widget/label" { x=8;y=156;width=200;height=20;text = "Widget Renders Here:"; style=bold; }
	    widget_cmp_pane "widget/pane"
		{
		x=8;y=176;width=374;height=334;
		style=flat;
		widget_cmp "widget/component"
		    {
		    x=0;y=0;width=372;height=372;
		    mode=dynamic;
		    path="/apps/widget_demo/wrapper.cmp";
		    }
		}
	    }
	}
    }
