$Version=2$
window "widget/page"
    {
    width=1000;
    height=600;
    title="Test window app";
    bgcolor="#c0c0c0";

    window_controls "widget/hbox"
	{
	x=10; y=10;
	width=980; height=32;
	align=center;
	spacing=10;

	openbtn "widget/textbutton"
	    {
	    width=150;
	    text="Open";
	    tristate=no;
	    border_radius=10;
	    bgcolor="#f0f0f0";

	    open_con "widget/connector" { event=Click; target=my_window; action=Open; blah=runclient("Greg"); }
	    }

	closebtn "widget/textbutton"
	    {
	    width=150;
	    text="Close";
	    tristate=no;
	    border_radius=10;
	    bgcolor="#f0f0f0";

	    close_con "widget/connector" { event=Click; target=my_window; action=Close; }
	    }
	}

    my_window "widget/childwindow"
	{
	titlebar=no;
	visible=no;
	x=100; y=100; width=800; height=400;
	border_radius=10;
	bgcolor="white";

	label "widget/label"
	    {
	    x=10; width=778;
	    y=100; height=198;
	    text="A Popup Window";
	    align=center;
	    }
	}
    }
