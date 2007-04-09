$Version=2$
basicform2 "widget/page" 
    {
    title = "Basic Data-driven Maintenance Form";
    bgcolor = "#e0e0e0";
    textcolor = black;
    x=0; y=0; width=640; height=420;
    
    ConfirmWindow "widget/childwindow"
	{
	title = "Data Was Modified!";
	titlebar = yes;
	hdr_bgcolor="#c00000";
	bgcolor= "#e0e0e0";
	visible = false;
	style = dialog;
	x=200;y=200;width=300;height=140;

	warninglabel "widget/label"
	    {
	    x=10;y=10;width=276;height=30;
	    text="Some data was modified.  Do you want to save it first, discard your modifications, or simply cancel the operation?";
	    }

	_3bConfirmSave "widget/textbutton"
	    {
	    x=10;y=75;width=80;height=30;
	    tristate=no;
	    background="/sys/images/grey_gradient.png";
	    text = "Save";
	    fgcolor1=black;fgcolor2=white;
	    }
	_3bConfirmDiscard "widget/textbutton"
	    {
	    x=110;y=75;width=80;height=30;
	    tristate=no;
	    background="/sys/images/grey_gradient.png";
	    text = "Discard";
	    fgcolor1=black;fgcolor2=white;
	    }
	_3bConfirmCancel "widget/textbutton"
	    {
	    x=210;y=75;width=80;height=30;
	    tristate=no;
	    background="/sys/images/grey_gradient.png";
	    text = "Cancel";
	    fgcolor1=black;fgcolor2=white;
	    }
	}

    osrc1 "widget/osrc"
	{
	sql = "SELECT :first_name, :last_name, :email from /samples/people.csv/rows";
	baseobj = "/samples/people.csv/rows";
	readahead=1;
	replicasize=8;

	syncconn "widget/connector"
	    {
	    event = "DataFocusChanged";
	    target = osrc2;
	    action = "Sync";

	    // action parameters
	    ParentOSRC = osrc1;
	    ParentKey1 = runclient("first_name");
	    ChildKey1 = runclient("first_name");
	    }
	mainpane "widget/pane"
	    {
	    x = 20; y=20; width=610; height=190;
	    background="/sys/images/slate2.gif";

	    form1 "widget/form"
		{
		_3bconfirmwindow = "ConfirmWindow";
		//bgcolor="#c0c0c0";

		formctlpane "widget/pane"
		    {
		    x = 100; y=10; width=240; height=30;
		    style=raised;
		    //bgcolor="#c0c0c0";
		    background="/sys/images/grey_gradient3.png";

		    formstatus "widget/formstatus"
			{
			x=72;y=4;
			style=largeflat;
			}
		    btnFirst "widget/imagebutton"
			{
			x=8;y=5;
			width=18; height=18;
			image = "/sys/images/ico16aa.gif";
			pointimage = "/sys/images/ico16ab.gif";
			clickimage = "/sys/images/ico16ac.gif";
			disabledimage = "/sys/images/ico16ad.gif";
			enabled = runclient(:form1:recid > 1);
			cn1 "widget/connector"
			    {
			    event="Click";
			    target="form1";
			    action="First";
			    }
			}
		    btnPrev "widget/imagebutton"
			{
			x=28;y=5;
			width=18; height=18;
			image = "/sys/images/ico16ba.gif";
			pointimage = "/sys/images/ico16bb.gif";
			clickimage = "/sys/images/ico16bc.gif";
			disabledimage = "/sys/images/ico16bd.gif";
			cn2 "widget/connector"
			    {
			    event="Click";
			    target="form1";
			    action="Prev";
			    }
			enabled = runclient(:form1:recid > 1);
			}
		    btnNext "widget/imagebutton"
			{
			x=190;y=5;
			width=18; height=18;
			image = "/sys/images/ico16ca.gif";
			pointimage = "/sys/images/ico16cb.gif";
			clickimage = "/sys/images/ico16cc.gif";
			disabledimage = "/sys/images/ico16cd.gif";
			cn3 "widget/connector"
			    {
			    event="Click";
			    target="form1";
			    action="Next";
			    }
			enabled = runclient(not (:form1:recid == :form1:lastrecid));
			}
		    btnLast "widget/imagebutton"
			{
			x=210;y=5;
			width=18; height=18;
			image = "/sys/images/ico16da.gif";
			pointimage = "/sys/images/ico16db.gif";
			clickimage = "/sys/images/ico16dc.gif";
			disabledimage = "/sys/images/ico16dd.gif";
			cn4 "widget/connector"
			    {
			    event="Click";
			    target="form1";
			    action="Last";
			    }
			enabled = runclient(not (:form1:recid == :form1:lastrecid));
			}
		    }

		searchbtn "widget/textbutton"
		    {
		    x = 10; y=10; height=30; width=70;
		    tristate=no;
		    background="/sys/images/grey_gradient.png";
		    text="Search";
		    enabled = runclient(:form1:is_queryable or :form1:is_queryexecutable);
		    fgcolor1=black;fgcolor2=white;
		    cn5 "widget/connector" { event="Click"; target="form1"; action="QueryToggle"; }
		    }
		newbtn "widget/textbutton"
		    {
		    x = 10; y=45; height=30; width=70;
		    tristate=no;
		    background="/sys/images/grey_gradient.png";
		    text="New";
		    enabled = runclient(:form1:is_newable);
		    fgcolor1=black;fgcolor2=white;
		    cn6 "widget/connector" { event="Click"; target="form1"; action="New"; }
		    }
		editbtn "widget/textbutton"
		    {
		    x = 10; y=80; height=30; width=70;
		    tristate=no;
		    background="/sys/images/grey_gradient.png";
		    text="Edit";
		    enabled = runclient(:form1:is_editable);
		    fgcolor1=black;fgcolor2=white;
		    cn7 "widget/connector" { event="Click"; target="form1"; action="Edit"; }
		    }
		savebtn "widget/textbutton"
		    {
		    x = 10; y=115; height=30; width=70;
		    tristate=no;
		    background="/sys/images/grey_gradient.png";
		    text="Save";
		    enabled = runclient(:form1:is_savable);
		    fgcolor1=black;fgcolor2=white;
		    cn8 "widget/connector" { event="Click"; target="form1"; action="Save"; }
		    }
		cancelbtn "widget/textbutton"
		    {
		    x = 10; y=150; height=30; width=70;
		    tristate=no;
		    background="/sys/images/grey_gradient.png";
		    text="Cancel";
		    enabled = runclient(:form1:is_discardable);
		    fgcolor1=black;fgcolor2=white;
		    cn9 "widget/connector" { event="Click"; target="form1"; action="Discard"; }
		    }

		first_name_label "widget/label"
		    {
		    x=80;y=45;width=80; height=20;
		    text="First Name:";
		    align=right;
		    }
		first_name "widget/editbox"
		    {
		    style=lowered;
		    bgcolor=white;
		    x=160;y=45;width=180;height=20;
		    fieldname="first_name";
		    }

		last_name_label "widget/label"
		    {
		    x=80;y=73;width=80; height=20;
		    text="Last Name:";
		    align=right;
		    }
		last_name "widget/editbox"
		    {
		    style=lowered;
		    bgcolor=white;
		    x=160;y=73;width=180;height=20;
		    fieldname="last_name";
		    }

		email_label "widget/label"
		    {
		    x=80;y=101;width=80; height=20;
		    text="Email Addr.:";
		    align=right;
		    }
		email "widget/editbox"
		    {
		    style=lowered;
		    bgcolor=white;
		    x=160;y=101;width=180;height=20;
		    fieldname="email";
		    }
	        } // end form

	    tablePane "widget/pane"
		{
		style=lowered;
		x=350;y=10;width=250;height=170;
		background = "/sys/images/grey_gradient3.png";

		myTable "widget/table"
		    {
		    mode = dynamicrow;
		    x=0;y=0;width=248;height=168;

		    row1_bgcolor = "#ffffff";
		    row2_bgcolor = "#e0e0e0";
		    rowhighlight_bgcolor = "#000080";
		    rowheight = 20;
		    windowsize = 8;
		    hdr_bgcolor = "#c0c0c0";
		    textcolorhighlight = "#ffffff";
		    textcolor = "#000000";
		    gridinemptyrows = 0;

		    first_name2 "widget/table-column" { fieldname="first_name"; title = "First Name"; width = 110; }
		    last_name2 "widget/table-column" { fieldname="last_name"; title = "Last Name"; width = 110; }
		    } // end table
		}

	    } // end pane

	} // end osrc

    osrc2 "widget/osrc"
	{
	sql = "SELECT :first_name, :computer_name, :memory from /samples/computers.csv/rows";
	baseobj = "/samples/computers.csv/rows";
	readahead=1;
	replicasize=8;
	autoquery = never;

	mainpane2 "widget/pane"
	    {
	    x = 20; y=220; width=610; height=190;
	    background="/sys/images/slate2.gif";

	    computerPane "widget/pane"
		{
		style = lowered;
		x=10;y=10;width=590;height=170;
		background="/sys/images/grey_gradient3.png";

		computerTable "widget/table"
		    {
		    mode = dynamicrow;
		    x=0;y=0;width=588;height=168;

		    row1_bgcolor = "#ffffff";
		    row2_bgcolor = "#e0e0e0";
		    rowhighlight_bgcolor = "#000080";
		    rowheight = 18;
		    windowsize = 8;
		    hdr_bgcolor = "#c0c0c0";
		    textcolorhighlight = "#ffffff";
		    textcolor = "#000000";
		    gridinemptyrows = 1;

		    first_name3 "widget/table-column" { fieldname="first_name"; title = "First Name"; width = 150; }
		    computer_name "widget/table-column" { fieldname="computer_name"; title = "Computer Name"; width = 150; }
		    memory "widget/table-column" { fieldname="memory"; title = "MB Memory"; width = 100; }
		    } // end table
		}
	    }
	}

//    stupidwin "widget/childwindow"
//	{
//	x=40; y=40; width=700; height=450;
//	hdr_bgcolor="#a0a0a0";
//	bgcolor="#a0a0a0";
//	title="Stupid Window";
//	style = window;
//	stupidhtml "widget/html"
//	    {
//	    mode=dynamic;
//	    x=0;y=0;width=698;
//	    source = "http://localhost:800/samples/basicform.app";
//	    }
//	}
    }
