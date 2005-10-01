$Version=2$
Page "widget/page"
    {
    title = "Month CSV Test Application";
    bgcolor = "#7f7f7f";
    textcolor = "#000000";
    x=0; y=0; width=640; height=480;
    
    objsrc1 "widget/osrc"
	{
	replicasize=12;
	readahead=12;
	sql = "SELECT :id,:full_name,:short_name,:num_days FROM /samples/Months.csv/rows ORDER BY :id";

	Form1 "widget/form"
	    {
	    AllowQuery = 1;
	    AllowNew = 0;
	    AllowModify = 0;
	    AllowView = 1;
	    AllowNoData = 1;
	    ReadOnly = 1;

	    Window1 "widget/childwindow"
		{
		x=100; y=100;
		width=220; height=200;
		hdr_bgcolor="#880000";
		bgcolor="#cfcfcf";
		title="Months";
		style="dialog";

		TextButton1 "widget/textbutton"
		    {
		    x=10; y=10; height=25; width=90;
		    fgcolor="#000000";
		    bgcolor="#bfbfbf";
		    fgcolor1="#000000";
		    fgcolor2="#cfcfcf";
		    tristate="no";
		    text="Query";
		    cn1 "widget/connector" { event="Click"; target="Form1"; action="Query"; }
		    }

		TextButton2 "widget/textbutton"
		    {
		    x=110; y=10; height=25; width=90;
		    fgcolor="#000000";
		    bgcolor="#bfbfbf";
		    fgcolor1="#000000";
		    fgcolor2="#cfcfcf";
		    tristate="no";
		    text="QueryExec";
		    cn2 "widget/connector" { event="Click"; target="Form1"; action="QueryExec"; }
		    }

		Label1 "widget/label"
		    {
		    x=10; y=50; width=80; height=15;
		    text="Month ID:";
		    align="right";
		    }
		EditBox1 "widget/editbox"
		    {
		    x=90; y=50; width=110; height=15;
		    fieldname="id";
		    bgcolor="#ffffff";
		    style="lowered";
		    readonly="no";
		    }

		Label2 "widget/label"
		    {
		    x=10; y=75; width=80; height=15;
		    text="Name:";
		    align="right";
		    }
		EditBox2 "widget/editbox"
		    {
		    x=90; y=75; width=110; height=15;
		    fieldname="full_name";
		    bgcolor="#ffffff";
		    style="lowered";
		    readonly="no";
		    }
		}
	    }
	}
    }
