$Version=2$
expression "widget/page"
    {
    // Application title
    title = "Example #4: Expressions";

    // Background color for the application
    bgcolor = "#c0c0c0";

    // Design geometry, in pixels
    width = 1000;
    height = 600;

    // Here's our button
    button "widget/textbutton"
	{
	// Button geometry
	x=20; y=20; width=120; height=32;

	// Button settings
	text = runclient(condition(:editbox:content != "", :editbox:content, "Click Me"));
	bgcolor = "#808080";
	tristate = no;
	border_radius = 8;
	}

    editbox "widget/editbox"
	{
	x=20; y=60; width=120; height=32;
	bgcolor = white;
	}
    }
