$Version=2$
fade_test "widget/page"
    {
    title = "Fader Test";
    bgcolor=black;

    pane "widget/pane"
        {
	x=100; y=30; width=400; height=400;
	style=raised;
	//bgcolor=white;

	//sp "widget/scrollpane"
	//    {
	//    width=398; height=398; x=0; y=0;
	htmlarea "widget/html"
	    {
	    width=398;
	    height=398;
	    x=0; y=0;
	    mode=dynamic;
	    source="http://greg:800/test/index.html";
	    }
	//    }
	}
	b1 "widget/textbutton"
	    {
	    x=0; y=500; width=100; height=64; text="Push Me"; tristate=no;
	    c1 "widget/connector"
	        {
		event = "Click";
		action = "LoadPage";
		Source = "'http://greg:800/test/team.html'";
		Transition = "'pixelate'";
		target = htmlarea;
		}
	    }
    }
