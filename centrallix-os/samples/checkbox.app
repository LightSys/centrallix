// This indicates that we're using a v2 structure file.
$Version=2$

// Main page widget is at top-level.
editbox_test "widget/page"
    {
    background="/sys/images/slate2.gif";

    testcheck2 "widget/checkbox"
	{
	    x = 20; y = 40; width = 12; height = 12;
	}
    testcheck "widget/checkbox"
	{
	    x = 20; y = 20; width = 12; height = 12;
	}
    }
