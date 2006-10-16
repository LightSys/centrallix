$Version=2$
window_test "widget/page"
    {
    alerter "widget/alerter" {}
    background="/sys/images/slate2.gif";
    x=0; y=0; width=640; height=480;
    
    button1 "widget/textbutton"
        {
	x=8; y=8; width=100; height=32;
	text="Button 1";
	tristate=no;
	cn1 "widget/connector"
	    {
	    event="Click";
	    target="alerter";
	    action="Alert";
	    param=runclient('you clicked button 1');
	    }
	}

    button2 "widget/textbutton"
	{
	x=8; y=108; width=100; height=32;
	text="Button 2";
	tristate=no;
	cn2 "widget/connector"
	    {
	    event="Click";
	    target="alerter";
	    action="Alert";
	    param=runclient('you clicked button 2');
	    }
	}

    btnFirst "widget/imagebutton"
	{
	x=250;y=5;
	width=18; height=18;
	image = "/sys/images/ico16aa.gif";
	pointimage = "/sys/images/ico16ab.gif";
	clickimage = "/sys/images/ico16ac.gif";
	cn3 "widget/connector"
	    {
	    event="Click";
	    target="alerter";
	    action="Alert";
	    param=runclient('\'first\' button');
	    }
	}

    }
