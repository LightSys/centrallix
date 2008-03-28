$Version=2$
simplerpt "widget/page"
    {
    title = "Launch a Simple Report";
    bgcolor = "#d0d0d0";
    width=640; height=480;

    size_form "widget/form"
	{
	size_lbl "widget/label"
	    {
	    x=8; y=8; width=120; height=20;
	    text = "Minimum File Size:";
	    align=right;
	    }
	size_eb "widget/editbox"
	    {
	    x=140; y=8; width=90; height=20;
	    bgcolor=white;
	    fieldname="min_size";
	    }
	}

    btn_print "widget/textbutton"
	{
	x=240; y=5; width=90; height=26;
	text="Print";
	fgcolor1 = black;
	fgcolor2 = white;
	bgcolor = "#c0c0c0";
	tristate = no;
	cn_print "widget/connector" 
	    { 
	    event="Click"; 
	    target="size_form"; 
	    action="Submit"; 
	    Target=runclient("simplerpt"); 
	    NewPage=runclient(1); 
	    Source=runclient("/samples/simple.rpt"); 
	    Width=runclient(800); 
	    Height=runclient(600);
	    }
	}
    }
