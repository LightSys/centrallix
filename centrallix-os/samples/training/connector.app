$Version=2$
connector "widget/page"
    {
    // Application title
    title = "Example #3: Connectors";

    // Background color for the application
    bgcolor = "#c0c0c0";

    // Design geometry, in pixels
    width = 1000;
    height = 600;

    popup_window "widget/childwindow"
	{
	titlebar=no;
	bgcolor="#a0a0a0";
	border_radius=16;
	border_style=solid;
	border_color=black;
	shadow_radius=8;
	shadow_color=black;
	shadow_offset=0;
	width=300;
	height=300;
	x=350;
	y=150;
	visible=no;
	toplevel=yes;

	popup_label "widget/label"
	    {
	    x=10; y=10; width=280; height=280;
	    text = "Hello World!";
	    font_size=32;
	    }
	}

    // Here's our button
    button "widget/textbutton"
	{
	// Button geometry
	x=120; y=20; width=120; height=32;

	// Button settings
	text = "Click Me";
	bgcolor = "#808080";
	tristate = no;
	border_radius = 8;

	// When button is clicked...
	on_click "widget/connector"
	    {
	    event = Click;
	    target = button;
	    action = SetText;
	    Text = runclient("Clicked!");
	    }

	on_click2 "widget/connector"
	    {
	    event=Click;
	    target=popup_window;
	    action=Open;
	    PointAt=button;
	    PointSide=runclient("top");
	    }
	on_click3 "widget/connector"
	    {
	    event=Click;
	    target=connector;
	    action=Alert;
	    Message=runclient("Clicked!");
	    }
	}
    }
