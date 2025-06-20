$Version=2$
autolayout "widget/page"
    {
    // Application title
    title = "Example #8: Autolayout (vbox)";

    // Background color for the application
    bgcolor = "#c0c0c0";

    // Design geometry, in pixels
    width = 1000;
    height = 600;

    // Vbox for autolayout
    my_vbox "widget/vbox"
	{
	x=10; y=10; width=120; height=580;
	spacing=10;

	// Here's our button
	button "widget/textbutton"
	    {
	    // Button geometry
	    height=32;

	    // Button settings
	    text = runclient(isnull(:editbox:content, "Click Me"));
	    bgcolor = "#808080";
	    tristate = no;
	    border_radius = 8;
	    }

	editbox "widget/editbox"
	    {
	    height=32;
	    bgcolor = white;
	    eb_hints "widget/hints" { style=strnull; length=10; }
	    }
	}
    }
