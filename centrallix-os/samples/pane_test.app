$Version=2$
pane_test "widget/page"
    {
    title = "Test of the pane widget";
    background = "/sys/images/test_background.png";

    raisedpane "widget/pane"
	{
	x=16; y=16; width=128; height=128;
	style=raised;
	}
    loweredpane "widget/pane"
	{
	x=160; y=16; width=128; height=128;
	style=lowered;
	bgcolor = "#c0c0c0";
	}
    containerpane "widget/pane"
	{
	x=16; y=160; width=128; height=128;
	style=raised;
	background="/sys/images/test_background_light.png";

	containedpane "widget/pane"
	    {
	    x=16; y=16; width=64; height=64;
	    style=lowered;
	    bgcolor = "#a0a0a0";
	    }
	}
    }
