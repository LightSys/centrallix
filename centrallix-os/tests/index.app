//
//  Centrallix Test Suite
//
//  "index.app"
//  By Luke Ehresman - August 23, 2002
//
//  This application provides a listing of all the test applications and
//  provides a simple interface with which to load and test those different
//  tests.
//

$Version=2$
Page1 "widget/page"
    {
    title="Centrallix HTML Generator Test Suite";
    background="/sys/images/slate2.gif";
    x=0; y=0; width=330; height=384;

    Label1 "widget/label"
	{
	x=10; y=10; width=500; height=20;
	text="<b>Centrallix Test Application Suite</b>";
	}

    Window1 "widget/childwindow"
	{
	x=10; y=40; height=334; width=310;
	hdr_bgcolor="#880000";
	textcolor="#ffffff";
	bgcolor="#c0c0c0";
	style="dialog";
	title="File Listing";

	OSRC1 "widget/osrc"
	    {
	    replicasize=25;
	    readahead=15;
	    sql="SELECT :name FROM /tests WHERE right(:name, 4) = '.app' AND :name != 'index.app'";

	    Pane1 "widget/pane"
		{
		x=3; y=3; width=302; height=302;
		style="lowered";
		bgcolor="#b0b0b0";

		Table1 "widget/table"
		    {
		    mode="dynamicrow";
		    x=0; y=0; width=300; height=300;
		    windowsize=15;
		    cellhspacing=0;
		    cellvspacing=0;
		    rowheight=20;
		    inner_border=0;
		    inner_padding=2;
		    bgcolor="#c0c0c0";
		    row_bgcolor1="#a0a0a0";
		    row_bgcolor2="#a0a0a0";
		    row_bgcolorhighlight="#909090";
		    hdr_bgcolor="#b0b0b0";
		    textcolor="black";
		    textcolorhighlight="black";
		    titlecolor="#000000";
		    fname "widget/table-column" { fieldname="name"; title="<b>File Name</b>"; width=300; }

		    Connector1 "widget/connector"
			{
			event="DblClick";
			target="Page1";
			action="LoadPage";
			Source="eparam.data.name";
			}
		    }
		}
	    }
	}
    }
