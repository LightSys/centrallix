// This indicates that we're using a v2 structure file.
$Version=2$

// Main page widget is at top-level.
editbox_test "widget/page"
    {
    background="/sys/images/slate2.gif";

    // Test putting em in a tab control.
    tabctl "widget/tab"
        {
	x = 20; y=20; width=300; height=200;
	background="/sys/images/slate2.gif";
	TabOne "widget/tabpage"
	    {
	    // New testing editbox widget.
	    my_editbox "widget/editbox"
		{
		style = lowered;
		bgcolor=white;
		x = 10; y = 10; width=100; height=19;
		}
	    my_editbox2 "widget/editbox"
		{
		style = lowered;
		//bgcolor='#e0e0e0';
		bgcolor=white;
		x = 10; y = 40; width=100; height=19;
		}
	    }
	TabTwo "widget/tabpage"
	    {
	    // New testing editbox widget.
	    my_editbox "widget/editbox"
		{
		style = lowered;
		//bgcolor=white;
		background='/sys/images/wood.png';
		x = 100; y = 10; width=100; height=19;
		}
	    my_editbox2 "widget/editbox"
		{
		style = lowered;
		//bgcolor='#e0e0e0';
		bgcolor=white;
		x = 100; y = 40; width=100; height=19;
		}
	    }
	}

    // Now for a table.
    tablepane "widget/pane"
        {
	x = 10; y=300; width=300; height=300;
	style=raised;
	MyTable "widget/table"
	    {
	    sql = "select :name, :size, :owner from /";
	    width=298;
	    inner_border=1;
	    hdr_bgcolor=white;
	    name "widget/table-column" { width=20; title="Filename"; }
	    size "widget/table-column" { width=5; title="Size"; }
	    owner "widget/table-column" { width=10; title="Owner"; }
	    }
	}
    }

