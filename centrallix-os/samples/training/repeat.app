$Version=2$
repeat "widget/page"
    {
    // Application title
    title = "Example #9: Repeats";

    // Background color for the application
    bgcolor = "#c0c0c0";

    // Design geometry, in pixels
    width = 1000;
    height = 600;

    // Template
    widget_template = "/samples/training/repeat.tpl";

    // Vbox for autolayout
    my_vbox "widget/vbox"
	{
	x=10; y=10; width=200; height=580;
	spacing=10;

	my_repeat "widget/repeat"
	    {
	    sql = "select :name, :title, path=:cx__pathpart3 from object wildcard '/samples/training/*.app' order by :title";

	    // Label the thing
	    one_label "widget/label"
		{
		height=12;
		style=bold;
		text=runserver(:my_repeat:title);
		}

	    // Here's our button
	    button "widget/component"
		{
		path = "repeat.cmp";

		// Button geometry
		height=24;

		// Button settings
		emptytext = runserver(:my_repeat:name);

		on_click "widget/connector"
		    {
		    event = Click;
		    target = repeat;
		    action = Launch;
		    Source = runclient('/samples/training/' + runserver(:my_repeat:path));
		    Width = 800;
		    Height = 600;
		    }
		}

	    // Spacer
	    sep "widget/autolayoutspacer" { height=10; }
	    }
	}
    }
