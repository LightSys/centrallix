$Version=2$
basicform "widget/page" 
    {
    title = "Basic Data-driven Maintenance Form";
    bgcolor = "#e0e0e0";
    textcolor = black;
    x=0; y=0; width=650; height=370;
    
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
	sql = "SELECT :id, :full_name, :short_name, :num_days, :num_leapyear_days FROM /samples/Months.csv/rows";
	baseobj = "/samples/Months.csv/rows";
	readahead=1;
	replicasize=4;
	autoquery = oneachreveal;
	form1 "widget/form"
	    {
	    _3bconfirmwindow = "ConfirmWindow";

	    mainpane "widget/pane"
		{
		x = 20; y=20; width=610; height=225;
		//bgcolor="#c0c0c0";
		background="/sys/images/slate2.gif";

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
		delbtn "widget/textbutton"
		    {
		    x = 10; y=185; height=30; width=70;
		    tristate=no;
		    background="/sys/images/grey_gradient.png";
		    text="Delete";
		    enabled = runclient(:form1:is_editable);
		    fgcolor1=black;fgcolor2=white;
		    cn10 "widget/connector" { event="Click"; target="form1"; action="Delete"; }
		    }

		id_label "widget/label"
		    {
		    x=100;y=45;width=130; height=20;
		    text="ID:";
		    align=right;
		    }
		id "widget/editbox"
		    {
		    style=lowered;
		    bgcolor=white;
		    x=240;y=45;width=100;height=20;
		    fieldname="id";
		    }

		fullname_label "widget/label"
		    {
		    x=100;y=73;width=130; height=20;
		    text="Month Name:";
		    align=right;
		    }
		fullname "widget/editbox"
		    {
		    style=lowered;
		    bgcolor=white;
		    x=240;y=73;width=100;height=20;
		    fieldname="full_name";
		    }

		shortname_label "widget/label"
		    {
		    x=100;y=101;width=130; height=20;
		    text="Abbreviation:";
		    align=right;
		    }
		shortname "widget/editbox"
		    {
		    style=lowered;
		    bgcolor=white;
		    x=240;y=101;width=100;height=20;
		    fieldname="short_name";
		    }

		days_label "widget/label"
		    {
		    x=100;y=129;width=130; height=20;
		    text="Days:";
		    align=right;
		    }
//		days "widget/editbox"
//		    {
//		    style=lowered;
//		    bgcolor=white;
//		    x=240;y=129;width=100;height=20;
//		    fieldname="num_days";
//		    }
		days_dd "widget/dropdown"
		    {
		    mode=static;
		    bgcolor="#c0c0c0";
		    x=240;y=129;width=100;height=20;
		    hilight="#000080";
		    fieldname="num_days";
		    days_dd_1 "widget/dropdownitem" { label="28 Days"; value="28"; }
		    days_dd_2 "widget/dropdownitem" { label="30 Days"; value="30"; }
		    days_dd_3 "widget/dropdownitem" { label="31 Days"; value="31"; }
		    }

		ldays_label "widget/label"
		    {
		    x=100;y=157;width=130; height=20;
		    text="Leapyear Days:";
		    align=right;
		    }
		ldays "widget/editbox"
		    {
		    style=lowered;
		    bgcolor=white;
		    x=240;y=157;width=100;height=20;
		    fieldname="num_leapyear_days";
		    }
	        }
	    }
	}
    debugwin "widget/childwindow"
	{
	x=20;y=220;width=600;height=330;
	visible=true;
	Treeview_pane "widget/pane"
	    {
	    x=0; y=0; width=598; height=305;
	    bgcolor="#e0e0e0";
	    style=lowered;
	    Tree_scroll "widget/scrollpane"
		{
		x=0; y=0; width=596; height=303;
		Tree "widget/treeview"
		    {
		    x=0; y=1; width=576;
		    source = "javascript:osrc1";
		    }
		}
	    }
	}
    }
