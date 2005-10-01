$Version=2$
tabs_and_tree "widget/page"
    {
    // my first app
    title = "A plain ol' app";
    bgcolor="#c0c0c0";
    x=0; y=0; width=700; height=600;
    
    myWin "widget/childwindow"
        {
	x = 100; y=100; width=500; height=400;
	style=dialog;
	title="Test Window";
	visible=false;
	bgcolor="#a0a0a0";
	hdr_bgcolor="#a0a0a0";

	pn "widget/pane"
	    {
	    x = 5;y=5;width=488;height=362;
	    style=lowered;

	    sp "widget/scrollpane"
		{
		x=0;y=0;width=486;height=360;

		ht "widget/html"
		    {
		    mode = dynamic;
		    x=0;y=0;width=464;
		    source="http://localhost:800/INSTALL.txt";
		    }
		}
	    }
	}
    
    myTab "widget/tab"
        {
	x = 20; y = 20; width=600; height=400;
	//bgcolor="#e0e0e0";
	background="/sys/images/slate2.gif";

	TabOne "widget/tabpage"
	    {
	    title = "&nbsp;<b>TabOne</b>&nbsp;";

	    tbOne "widget/textbutton"
	        {
		x = 20; y=20; width=80; height=30;
		text="Click Me";
		tristate=no;
		fgcolor1=black;fgcolor2=white;
		background="/sys/images/grey_gradient.png";

		cn "widget/connector"
		    {
		    event="Click";
		    target="myWin";
		    action="SetVisibility";
		    IsVisible=1;
		    }
		}
	    }
	TabTwo "widget/tabpage"
	    {
	    title = "&nbsp;<b>TabTwo</b>&nbsp;";

	    sp2 "widget/scrollpane"
		{
		x=0;y=0;width=598;height=398;
		tv "widget/treeview"
		    {
		    x=0;y=2;width=578;
		    source="/samples/mbox/";

		    ccn "widget/connector"
			{
			event="ClickItem";
			target="myWin";
			action="SetVisibility";
			IsVisible=1;
			}
		    ccn2 "widget/connector"
			{
			event="ClickItem";
			target="ht";
			action="LoadPage";
			Source=Pathname;
			}
		    }
		}
	    }
	}
    }
