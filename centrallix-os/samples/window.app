$Version=2$

window_test "widget/page"
    {
    background="/sys/images/slate2.gif";

    window1 "widget/htmlwindow"
	{
	bgcolor="#c0c0c0";
	x=32; y=32; width=640; height=480;
	hdr_bgcolor="white";
	title="Window One";
	style=window;

	window2 "widget/htmlwindow"
	    {
	    bgcolor="#e0e0e0";
	    hdr_bgcolor="white";
	    title = "Window Two";
	    x=32; y=32; width=320; height=240;
	    style=dialog;
	    }
	}
    }
