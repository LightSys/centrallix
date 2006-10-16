$Version=2$
fade_test "widget/page"
    {
    title = "Fader Test";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=600; height=560;
    
    pane "widget/pane"
        {
	x=100; y=30; width=400; height=400;
	style=raised;

	htmlarea "widget/html"
	    {
	    width=398;
	    height=398;
	    x=0; y=0;
	    mode=dynamic;
	    }
	}

	b1 "widget/textbutton"
	    {
	    x=100; y=500; width=100; height=32; text="Push Me (1)"; tristate=no;
	    bgcolor="#a0a0a0";
	    fgcolor1="#000000";
	    fgcolor2="#a0a0a0";
	    c1 "widget/connector"
	        {
		event = "Click";
		action = "LoadPage";
		Mode = runclient('static');
		Source = runclient('/samples/fade_test1.html');
		Transition = runclient('pixelate');
		target = htmlarea;
		}
	    }
	b2 "widget/textbutton"
	    {
	    x=210; y=500; width=100; height=32; text="Push Me (2)"; tristate=no;
	    bgcolor="#a0a0a0";
	    fgcolor1="#000000";
	    fgcolor2="#a0a0a0";
	    c2 "widget/connector"
	        {
		event = "Click";
		action = "LoadPage";
		Mode = runclient('static');
	    	Source=runclient('/samples/fade_test2.html');
		Transition = runclient('pixelate');
		target = htmlarea;
		}
	    }
    }
