$Version=2$

window_test "widget/page"
    {
    background="/sys/images/slate2.gif";

    button1 "widget/textbutton"
        {
	x=8; y=8; width=100; height=32;
	text="MyButton";
	tristate=no;
	}

    window1 "widget/htmlwindow"
	{
	bgcolor="#c0c0c0";
	x=32; y=32; width=640; height=480;
	hdr_bgcolor="white";
	title="Window One";
	style=window;

	button2 "widget/textbutton"
	    {
	    x=8; y=8; width=100; height=32;
	    text="MyButton Two";
	    tristate=no;
	    }

	window2 "widget/htmlwindow"
	    {
	    bgcolor="#e0e0e0";
	    hdr_bgcolor="white";
	    title = "Window Two";
	    x=32; y=32; width=320; height=240;
	    style=dialog;
	    Treeview_pane "widget/pane"
		{
		x=0; y=0; width=318; height=218;
		bgcolor="#e0e0e0";
		style=lowered;
		Tree_scroll "widget/scrollpane"
		    {
		    x=0; y=0; width=316; height=216;
		    Tree "widget/treeview"
			{
			x=0; y=1; width=296;
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
    }
