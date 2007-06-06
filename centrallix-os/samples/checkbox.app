// This indicates that we're using a v2 structure file.
$Version=2$

// Main page widget is at top-level.
editbox_test "widget/page"
    {
    background="/sys/images/slate2.gif";
    x=0; y=0; width=640; height=480;
    
    testcheck2 "widget/checkbox"
	{
	    x = 20; y = 40; width = 12; height = 12;
	}
    testcheck "widget/checkbox"
	{
	    x = 20; y = 20; width = 12; height = 12;
	}

    label1 "widget/label"
    	{
	    x = 40; y = 17; width = 150; height = 12;
	    text = "First Checkbox";
	}
    label2 "widget/label"
    	{
	    x = 40; y = 37; width = 150; height = 12;
	    text = "Second Checkbox";
	}
    }
