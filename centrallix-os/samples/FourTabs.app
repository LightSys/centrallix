$Version=2$
FourTabs "widget/page"
    {
    title = "Four Tabs demonstration app";
    bgcolor = "#c0c0c0";

    TabCtlOne "widget/tab"
	{
	tab_location = top;
	//tab_width = 80;
	x = 10; y = 10; width=300; height=100;
	background = "/sys/images/slate2.gif";
	inactive_background = "/sys/images/slate2_dark.gif";
	selected = runclient(:TabCtlTwo:selected);

	TabOne "widget/tabpage" { lbl1 "widget/label" { x=10; y=10; width=100; height=32; text="Label One"; } }
	TabTwo "widget/tabpage" { lbl2 "widget/label" { x=30; y=30; width=100; height=32; text="Label Two"; } }
	TabThree "widget/tabpage" { lbl3 "widget/label" { x=50; y=50; width=100; height=32; text="Label Three"; } }
	}
    TabCtlTwo "widget/tab"
	{
	tab_location = bottom;
	tab_width = 90;
	x = 10; y = 144; width=300; height=100;
	bgcolor = "#e0e0e0";
	inactive_bgcolor = "#d0d0d0";
	selected = runclient(:TabCtlOne:selected);

	TabOne "widget/tabpage" { }
	TabTwo "widget/tabpage" { }
	TabThree "widget/tabpage" { }
	}
    TabCtlThree "widget/tab"
	{
	tab_location = left;
	tab_width = 80;
	x = 320; y = 34; width=300; height=100;
	bgcolor = "#c0c0e0";
	inactive_bgcolor = "#b0b0d0";

	TabOne "widget/tabpage" { }
	TabTwo "widget/tabpage" { }
	TabThree "widget/tabpage" { }
	}
    TabCtlFour "widget/tab"
	{
	tab_location = right;
	tab_width = 80;
	x = 320; y = 168; width=300; height=100;
	bgcolor = "#c0e0c0";
	inactive_bgcolor = "#b0d0b0";

	TabOne "widget/tabpage" { }
	TabTwo "widget/tabpage" { }
	TabThree "widget/tabpage" { }
	}
    }
