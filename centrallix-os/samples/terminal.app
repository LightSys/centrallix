// This indicates that we're using a v2 structure file.
$Version=2$

// Main page widget is at top-level.
editbox_test "widget/page"
    {
    background="/sys/images/slate2.gif";

    term1 "widget/terminal"
	{
	x=0; y=100; rows=24; cols=80;
	//source = "/samples/environ.shl";
	source = "/samples/top.shl";
	}
    }
