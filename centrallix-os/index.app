$Version=2$
index "widget/page"
    {
    title = "Welcome to Centrallix 0.7.5";
    bgcolor = "#ffffff";
    height = 400;
    width = 620;

    cximg "widget/image"
	{
	x=120; y=10; height=66; width=374; 
	source="/sys/images/centrallix_374x66.png";
	}

    cxlabel "widget/label"
	{
	x=16; y=80; height=66; width=588;
	align=center;
	fontsize=4;
	text = "Welcome to Centrallix 0.7.5, released in June 2006.  If you're seeing this page for the first time after an installation, congratulations - you've just successfully finished the install!  Below are a few links to get you started.";
	}

    pnOptions "widget/pane"
	{
	x=16;y=160;width=588;height=130;
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
	    text = "Click here to browse some Centrallix sample applications, reports, and more.";
	    }

	btnMgmt "widget/textbutton"
	    {
	    x=16; y=72; width=96; height=40;
	    tristate=no;
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black; fgcolor2=white;
	    text = "System Mgmt";

	    onClick2 "widget/connector"
		{
		event = "Click";
		target = index;
		action = "LoadPage";
		Source = runclient("/apps/cxmanage/");
		}
	    }
	lblMgmt "widget/label"
	    {
	    x=120; y=72; width=440; height=40;
	    fontsize=4;
	    text = "Click here for the Centrallix system management page.";
	    }
	}

    lblLicense "widget/label"
	{
	x=16; y=340; width=588; height=50;
	text="Centrallix 0.7.5 is Free Software, released under the GNU GPL version 2, or at your option any later version published by the Free Software Foundation.  Centrallix 0.7.5 is provided with ABSOLUTELY NO WARRANTY.  See the file COPYING, in the accompanying documentation, for details.";
	fontsize=2;
	}
    }

