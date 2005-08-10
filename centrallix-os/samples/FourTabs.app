$Version=2$
FourTabs "widget/page"
    {
    title = "Four Tabs demonstration app";
    bgcolor = "#c0c0c0";
    x=0; y=0; width=630; height=275;
    
    TabCtlOne "widget/tab"
	{
	tab_location = top;
	//tab_width = 80;
	x = 10; y = 10; width=300; height=100;
	background = "/sys/images/slate2.gif";
	inactive_background = "/sys/images/slate2_dark.gif";
	selected_index = runclient(:TabCtlTwo:selected_index);

	TabOne1 "widget/tabpage" { lbl1 "widget/label" { x=10; y=10; width=100; height=32; text="Label One"; } }
	TabTwo1 "widget/tabpage" { lbl2 "widget/label" { x=30; y=30; width=100; height=32; text="Label Two"; } }
	TabThree1 "widget/tabpage" { lbl3 "widget/label" { x=50; y=50; width=100; height=32; text="Label Three"; } }
	}
    TabCtlTwo "widget/tab"
	{
	tab_location = bottom;
	tab_width = 90;
	x = 10; y = 144; width=300; height=100;
	bgcolor = "#e0e0e0";
	inactive_bgcolor = "#d0d0d0";
	selected_index = runclient(:TabCtlOne:selected_index);

	TabOne2 "widget/tabpage" { }
	TabTwo2 "widget/tabpage" { }
	TabThree2 "widget/tabpage" { }
	}
    TabWindow "widget/htmlwindow"
	{
	x = 320; y = 10; width=300; height=124;
	title="Tab Number Three";
	bgcolor="#808080";
	hdr_bgcolor="#a0a0a0";
	style=dialog;
	TabCtlThree "widget/tab"
	    {
	    tab_location = left;
	    tab_width = 80;
	    x=10;y=5;width=200;height=90;
	    bgcolor = "#c0c0e0";
	    inactive_bgcolor = "#b0b0d0";

	    TabOne3 "widget/tabpage" { }
	    TabTwo3 "widget/tabpage" { }
	    TabThree3 "widget/tabpage" { }
	    }
	}
    TabPane "widget/pane"
	{
	x = 320; y = 144; width=300; height=124;
	style=raised;
	bgcolor="#e0e0e0";
	TabCtlFour "widget/tab"
	    {
	    x=10;y=10;width=200;height=100;
	    tab_location = right;
	    tab_width = 80;
	    bgcolor = "#c0e0c0";
	    inactive_bgcolor = "#b0d0b0";

	    TabOne4 "widget/tabpage" { }
	    TabTwo4 "widget/tabpage" { }
	    TabThree4 "widget/tabpage" { }
	    }
	}
    }
