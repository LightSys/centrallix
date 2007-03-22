$Version=2$
component_test "widget/page"
    {
    title = "Component System Test";
    bgcolor = "black";
    width=640; height=480;

    dbg_win_label "widget/label"
	{
	x=10;y=10;width=100;height=20;
	fgcolor=white;
	text="Debug Window:";
	}
    btn "widget/textbutton"
	{
	x=10;y=30;width=100;height=30;
	text = "Instantiate DBG";
	fgcolor1=black; fgcolor2=white;
	background="/sys/images/grey_gradient.png";
	tristate = no;

	cn1 "widget/connector" { event="Click"; action="Instantiate"; target=win; }
	}
    btn2 "widget/textbutton"
	{
	x=10;y=70;width=100;height=30;
	text = "Destroy DBG";
	fgcolor1=black; fgcolor2=white;
	background="/sys/images/grey_gradient.png";
	tristate = no;

	cn2 "widget/connector" { event="Click"; action="Destroy"; target=win; }
	}

    win "widget/component"
	{
	mode=dynamic;
	x=0;y=0;width=640;height=480;
	multiple_instantiation=no;
	path="/samples/debugwin.cmp";
	}

    win2_form "widget/form"
	{
	win2_vbox "widget/vbox"
	    {
	    x=10; y=140; width=100;
	    cellsize=20;
	    spacing=0;

	    eb_title_label "widget/label"
		{
		fgcolor=white;
		text="Window Title:";
		}
	    eb_title "widget/editbox"
		{
		bgcolor=white;
		fieldname="win_title";
		}
	    sp1 "widget/autolayoutspacer" { height=10; }

	    dd_color_label "widget/label"
		{
		fgcolor=white;
		text="Window Color:";
		}
	    dd_color "widget/dropdown"
		{
		bgcolor=white;
		hilight="#c0c0c0";
		mode=static;
		fieldname="backcolor";

		dd_color_1 "widget/dropdownitem" { label="Black"; value="#000000"; }
		dd_color_2 "widget/dropdownitem" { label="Red"; value="#c00000"; }
		dd_color_3 "widget/dropdownitem" { label="Teal"; value="#40c0c0"; }
		}
	    sp2 "widget/autolayoutspacer" { height=10; }

	    dd_widget_label "widget/label"
		{
		fgcolor=white;
		text="Include Widget:";
		}
	    dd_widget "widget/dropdown"
		{
		bgcolor=white;
		hilight="#c0c0c0";
		mode=static;
		fieldname="whatwidget";

		dd_widget_1 "widget/dropdownitem" { label="Tab Control"; value="tab"; }
		dd_widget_2 "widget/dropdownitem" { label="Pane"; value="pane"; }
		dd_widget_3 "widget/dropdownitem" { label="Image"; value="image"; }
		dd_widget_4 "widget/dropdownitem" { label="Bordered Image"; value="imagebdr"; }
		}
	    sp3 "widget/autolayoutspacer" { height=10; }

	    submit_btn "widget/textbutton"
		{
		height=30;
		text = "Create Window";
		fgcolor1=black; fgcolor2=white;
		background="/sys/images/grey_gradient.png";
		tristate = no;

		cn3 "widget/connector" { event="Click"; action="Submit"; target=win2_form; Target=runclient("win2"); }
		}
	    }
	}

    win2 "widget/component"
	{
	mode=dynamic;
	x=0;y=0;width=640;height=480;
	multiple_instantiation=yes;
	path="/samples/colorwin.cmp";
	}
    }

