$Version=2$

window_test "widget/page"
    {
    //background="/sys/images/slate2.gif";
    background="/sys/images/test_background.png";
    x=0; y=0; width=640; height=480;
    
    button1 "widget/component"
        {
	x=8; y=8; width=100; height=32;
	path = "/samples/button.cmp";
	//text="MyButton";
	//tristate=no;
	}

    notitle_win1 "widget/childwindow"
	{
	style=window;
	titlebar=no;
	bgcolor="#c0c0c0";
	x=450; y=32; width=200; height=200;
	}

    notitle_win2 "widget/childwindow"
	{
	style=dialog;
	titlebar=no;
	bgcolor="#e0e0e0";
	x=450; y=240; width=200; height=200;
	}

    window1 "widget/childwindow"
	{
	//bgcolor="#c0c0c0";
	background="/sys/images/test_background_light.png";
	x=32; y=32; width=400; height=360;
	hdr_bgcolor="white";
	title="Window One";
	style=window;

	button2 "widget/textbutton"
	    {
	    x=8; y=8; width=100; height=32;
	    text="MyButton Two";
	    tristate=no;
	    }

	window2 "widget/childwindow"
	    {
	    bgcolor="#e0e0e0";
	    hdr_bgcolor="white";
	    title = "Window Two";
	    x=32; y=32; width=320; height=240;
	    style=dialog;
	    Treeview_pane "widget/pane"
		{
		x=4; y=4; width=310; height=206;
		bgcolor="#e0e0e0";
		style=lowered;
		Tree_scroll "widget/scrollpane"
		    {
		    x=0; y=0; width=308; height=204;
		    Tree "widget/treeview"
			{
			x=0; y=1; width=188;
			//if there is a javascript: in front, the DOM Viewer mode is used
			//  if not, it's a normal treeview widget
			//directly after the javascript:, put the object you want the root to be
			//   it must be globally accessible -- default is window
			source = "javascript:";
			}
		    }
		}
	    }
	}

    debugger "widget/component"
	{
	x=0;y=0;
	path = "/samples/debugwin.cmp";
	mode = static;
	}
    }
