$Version=2$
index "widget/page"
    {
    title = "Welcome to Centrallix 0.7.4";
    bgcolor = "#ffffff";

    cximg "widget/image"
	{
	x=120; y=0; height=66; width=374; 
	source="/sys/images/centrallix_374x66.png";
	}

    cxlabel "widget/label"
	{
	x=0; y=80; height=66; width=620;
	align=center;
	fontsize=4;
	text = "Welcome to Centrallix 0.7.4, released in February 2005.  If you're seeing this page for the first time after an installation, congratulations - you've just successfully finished the install!  Below are a few links to get you started.";
	}

    pnOptions "widget/pane"
	{
	x=16;y=160;width=580;height=74;
	bgcolor="#c0c0c0";
	style=raised;

	btnSamples "widget/textbutton"
	    {
	    x=16; y=16; width=96; height=40;
	    tristate=no;
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black; fgcolor2=white;
	    text = "Samples";

	    onClick "widget/connector"
		{
		event = "Click";
		target = index;
		action = "LoadPage";
		Source = runclient("/samples/");
		}
	    }
	lblSamples "widget/label"
	    {
	    x=120; y=16; width=440; height=40;
	    fontsize=4;
	    text = "An application for browsing some Centrallix sample applications, reports, and more.";
	    }
	}
    }

