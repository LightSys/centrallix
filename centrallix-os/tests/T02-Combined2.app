//
//  Centrallix Test Suite
//
//  "T02-Combined2.app"
//  By Luke Ehresman - August 23, 2002
//
//  This app is designed to test the querying capabilites of the
//  form widget and its interaction with the objectsource widget.
//  The data is read from States.csv.
//
//  Widgets used:
//    page, textbutton, osrc, connector, form, dropdown,
//    editbox, formstatus, label
//

$Version=2$
Page1 "widget/page"
    {
    title="Test #02 - 2nd Combined Page";
    bgcolor="#546140";
    loadstatus="true";

    TextButton1 "widget/textbutton"
	{
	x=10; y=10; height=25; width=60;
	fgcolor1="#000000";
	fgcolor2="#c0c0c0";
	bgcolor="#c0c0c0";
	text="INDEX";
	tristate="no";

	Connector1 "widget/connector" { event="Click"; target="Page1"; action="LoadPage"; Source="'index.app'"; }
	}

    OSRC1 "widget/osrc"
	{
	replicasize=25;
	readahead=15;
	sql="SELECT :id,:full_name,:abbrev FROM /tests/States.csv/rows ORDER BY :full_name";

	Form1 "widget/form"
	    {
	    AllowQuery=1;
	    AllowNew=0;
	    AllowModify=0;
	    AllowView=1;
	    AllowNoData=1;
	    ReadOnly=1;

	    Tab1 "widget/tab"
		{
		x=10; y=40; width=280; height=150;
		bgcolor="#c0c0c0";

		TabPage1 "widget/tabpage"
		    {
		    TextButton2 "widget/textbutton"
			{
			x=180; y=10; height=25; width=90;
			fgcolor1="#000000";
			fgcolor2="#c0c0c0";
			tristate="no";
			text="Query";
			Connector2 "widget/connector" { event="Click"; target="Form1"; action="Query"; }
			}
		    TextButton3 "widget/textbutton"
			{
			x=180; y=40; height=25; width=90;
			fgcolor1="#000000";
			fgcolor2="#c0c0c0";
			tristate="no";
			text="QueryExec";
			Connector3 "widget/connector" { event="Click"; target="Form1"; action="QueryExec"; }
			}
		    FormStatus1 "widget/formstatus"
			{
			x=240; y=120;
			}

		    Dropdown1 "widget/dropdown"
			{
			mode="dynamic_server";
			x=10; y=10; width=160; height=20;
			hilight="#b5b5b5";
			bgcolor="#c0c0c0";
			fieldname="id";
			numdisplay=10;
			sql="SELECT :full_name,:id FROM /tests/States.csv/rows ORDER BY :full_name";
			}

		    EditBox1 "widget/editbox"
			{
			x=10; y=40; width=160; height=15;
			fieldname="id";
			bgcolor="#ffffff";
			style="lowered";
			readonly="yes";
			}
		    EditBox2 "widget/editbox"
			{
			x=10; y=60; width=160; height=15;
			fieldname="full_name";
			bgcolor="#ffffff";
			style="lowered";
			readonly="yes";
			}
		    EditBox3 "widget/editbox"
			{
			x=10; y=80; width=160; height=15;
			fieldname="abbrev";
			bgcolor="#ffffff";
			style="lowered";
			readonly="yes";
			}


		    }
		TabPage2 "widget/tabpage"
		    {
		    Label1 "widget/label"
			{
			x=10; y=10; width=100; height=15;
			text="<b>Current State:</b>";
			align="right";
			}
		    Editbox4 "widget/editbox"
			{
			x=110; y=12; width=155; height=15;
			fieldname="full_name";
			bgcolor="#ffffff";
			style="lowered";
			readonly="yes";
			}
		    }
		}
	    }
	}
    }
