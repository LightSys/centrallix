$Version=2$
wholePage "widget/page" 
    {
    alerter "widget/alerter" {}
    title = "Form, osrc, and widget test page";
    bgcolor = "#1f1f1f";
    textcolor = black;
    x=0; y=0; width=640; height=480;
    
    _3bConfirmWindow "widget/childwindow"
	{
	title = "Confirm something please";
	titlebar = yes;
	bgcolor= "#c0c0c0";
	visible = false;
	x=200;y=200;width=300;height=80;

	_3bConfirmDiscard "widget/textbutton"
	    {
	    x=10;y=15;width=80;height=30;
	    text = "Discard";
	    }
	_3bConfirmSave "widget/textbutton"
	    {
	    x=110;y=15;width=80;height=30;
	    text = "Save";
	    }
	_3bConfirmCancel "widget/textbutton"
	    {
	    x=210;y=15;width=80;height=30;
	    text = "Cancel";
	    }
	}

    navWindow "widget/childwindow" 
	{
	title = "Left Nav Window";
	bgcolor = "#b0b0b0";
	titlebar = yes;
	style = "floating";
	x = 0; y = 0; width = 95; height = 300;

	navBtn1 "widget/textbutton" 
	    {
	    x = 5; y = 5; height = 60; width = 80;
	    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
	    text = "Show self DOM";
	    tristate = "yes";
	    cn5 "widget/connector"
		{
		event="Click";
		target="alerter";
		action="ViewTreeDOM";
		param="(navBtn1)";
		}
	    }
	showedit1 "widget/textbutton" 
	    {
	    x = 5; y = 125; height = 60; width = 80;
	    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
	    text = "Display above text";
	    tristate = "yes";
	    cn6 "widget/connector"
		{
		event="Click";
		target="alerter";
		action="Confirm";
		param="editshow1.getvalue()";
		}
	    }
	editshow1 "widget/editbox"
	    {
		x=4;y=100;width=80;height=15;
	    }
	}
    mainWindow "widget/childwindow" 
	{
	title = "Main";
	bgcolor = "#b0b0b0";
	titlebar = yes;
	style = "floating";
	x = 95; y = 0; width = 600; height = 300;
	osrc1 "widget/osrc"
	    {
	    sql = "SELECT :name, :full_name, :num_days FROM /samples/Months.csv/rows";
	    bigTabFrame "widget/tab" 
		{
		x = 5; y = 5; width = 590; height = 200;
		bgcolor = "#c0c0c0";
	    
		First "widget/tabpage"
		    {
		    form1 "widget/form"
			{
			basequery = "SELECT objname = :name, :full_name, :num_days FROM /samples/Months.csv/rows";
			//ReadOnly = no;

			_3bconfirmwindow = "_3bConfirmWindow";
			//basequery = "SELECT a,b,c,d,e FROM data WHERE b=true";
			//basewhere = "a=true";
			formstatus "widget/formstatus"
			    {
			    x=5;y=170;
			    }
			btnFirst "widget/imagebutton"
			    {
			    x=50;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16aa.gif";
			    pointimage = "/sys/images/ico16ab.gif";
			    clickimage = "/sys/images/ico16ac.gif";
			    cn1 "widget/connector"
				{
				event="Click";
				target="form1";
				action="First";
				}
			    }
			btnPrev "widget/imagebutton"
			    {
			    x=70;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16ba.gif";
			    pointimage = "/sys/images/ico16bb.gif";
			    clickimage = "/sys/images/ico16bc.gif";
			    cn2 "widget/connector"
				{
				event="Click";
				target="form1";
				action="Prev";
				}
			    }
			btnNext "widget/imagebutton"
			    {
			    x=90;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16ca.gif";
			    pointimage = "/sys/images/ico16cb.gif";
			    clickimage = "/sys/images/ico16cc.gif";
			    cn3 "widget/connector"
				{
				event="Click";
				target="form1";
				action="Next";
				}
			    }
			btnLast "widget/imagebutton"
			    {
			    x=110;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16da.gif";
			    pointimage = "/sys/images/ico16db.gif";
			    clickimage = "/sys/images/ico16dc.gif";
			    cn4 "widget/connector"
				{
				event="Click";
				target="form1";
				action="Last";
				}
			    }


			testcheck "widget/checkbox"
			    {
				x = 20; y = 20; width = 12; height = 12;
				fieldname = "fieldcheck1";
			    }
//			testspin "widget/spinner"
//			    {
//				x=10;y=80;width=100;height=15;
//				fieldname="objname";
//			    }
			testspin "widget/editbox"
			    {
				x=20;y=100;width=100;height=15;
				fieldname="objname";
			    }
			testedit "widget/editbox"
			    {
				x=20;y=130;width=100;height=15;
				fieldname="num_days";
			    }
			formchangebtn1 "widget/textbutton" 
			    {
			    x = 50; y = 10; height = 30; width = 80;
			    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
			    text = "Form status to query";
			    cn25 "widget/connector"
				{
				event="Click";
				target="form1";
				action="Query";
				}
			    }
			formchangebtn2 "widget/textbutton" 
			    {
			    x = 150; y = 10; height = 30; width = 80;
			    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
			    text = "Execute Query";
			    cn26 "widget/connector"
				{
				event="Click";
				target="form1";
				action="QueryExec";
				}
			    }
			testradio "widget/radiobuttonpanel" 
			    {
			    x=350;
			    y=30;
			    width=150;
			    height=180;
			    title="test";
			    fieldname="radiofield1";

			    label1 "widget/radiobutton" 
				{
				label="basketball";
				selected="true";
				}
			    label2 "widget/radiobutton" 
				{
				label="is fun";
				}
			    }
			testdrop "widget/dropdown"
			    {
			    x=200;y=130;
			    width=100;
			    bgcolor="#CFCFCF";
			    hilight="green";
			    fieldname="full_name";
			    d1 "widget/dropdownitem"
				{
				value="January";
				label="January";
				}
			    d2 "widget/dropdownitem"
				{
				value="February";
				label="February";
				}
			    d3 "widget/dropdownitem"
				{
				value="March";
				label="March";
				}
			    d4 "widget/dropdownitem"
				{
				value="April";
				label="April";
				}
			    d5 "widget/dropdownitem"
				{
				value="May";
				label="May";
				}
			    d6 "widget/dropdownitem"
				{
				value="June";
				label="June";
				}
			    d7 "widget/dropdownitem"
				{
				value="July";
				label="July";
				}
			    d8 "widget/dropdownitem"
				{
				value="August";
				label="August";
				}
			    d9 "widget/dropdownitem"
				{
				value="September";
				label="September";
				}
			    d10 "widget/dropdownitem"
				{
				value="October";
				label="October";
				}
			    d11 "widget/dropdownitem"
				{
				value="November";
				label="November";
				}
			    d12 "widget/dropdownitem"
				{
				value="December";
				label="December";
				}
			    }
//			testspin "widget/spinner"
//			    {
//			    x=100;y=80;
//			    width=100;height=15;
//			    fieldname="spin1";
//			    }
			}
		    }
		Second "widget/tabpage" 
		    {
		    form2 "widget/form"
			{
			basequery = "SELECT objname = :name, :full_name, :num_days FROM /samples/Months.csv/rows";

			_3bconfirmwindow = "_3bConfirmWindow";
			formstatus1 "widget/formstatus"
			    {
			    x=105;y=170;
			    }
			btnFirst1 "widget/imagebutton"
			    {
			    x=150;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16aa.gif";
			    pointimage = "/sys/images/ico16ab.gif";
			    clickimage = "/sys/images/ico16ac.gif";
			    cn7 "widget/connector"
				{
				event="Click";
				target="form1";
				action="First";
				}
			    }
			btnPrev1 "widget/imagebutton"
			    {
			    x=170;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16ba.gif";
			    pointimage = "/sys/images/ico16bb.gif";
			    clickimage = "/sys/images/ico16bc.gif";
			    cn28 "widget/connector"
				{
				event="Click";
				target="form1";
				action="Prev";
				}
			    }
			btnNext1 "widget/imagebutton"
			    {
			    x=190;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16ca.gif";
			    pointimage = "/sys/images/ico16cb.gif";
			    clickimage = "/sys/images/ico16cc.gif";
			    cn8 "widget/connector"
				{
				event="Click";
				target="form1";
				action="Next";
				}
			    }
			btnLast1 "widget/imagebutton"
			    {
			    x=210;y=170;
			    width=20; height=20;
			    image = "/sys/images/ico16da.gif";
			    pointimage = "/sys/images/ico16db.gif";
			    clickimage = "/sys/images/ico16dc.gif";
			    cn9 "widget/connector"
				{
				event="Click";
				target="form1";
				action="Last";
				}
			    }


			testcheck1 "widget/checkbox"
			    {
				x = 120; y = 20; width = 12; height = 12;
				fieldname = "fieldcheck1";
			    }
//			testspin "widget/spinner"
//			    {
//				x=10;y=80;width=100;height=15;
//				fieldname="objname";
//			    }
			testspin1 "widget/editbox"
			    {
				x=120;y=100;width=100;height=15;
				fieldname="objname";
			    }
			testedit1 "widget/editbox"
			    {
				x=120;y=130;width=100;height=15;
				fieldname="num_days";
			    }
			formchangebtn11 "widget/textbutton" 
			    {
			    x = 150; y = 10; height = 30; width = 80;
			    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
			    text = "Form status to query";
			    cn10 "widget/connector"
				{
				event="Click";
				target="form2";
				action="Query";
				}
			    }
			formchangebtn12 "widget/textbutton" 
			    {
			    x = 250; y = 10; height = 30; width = 80;
			    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
			    text = "Execute Query";
			    cn11 "widget/connector"
				{
				event="Click";
				target="form2";
				action="QueryExec";
				}
			    }
			testradio1 "widget/radiobuttonpanel" 
			    {
			    x=450;
			    y=30;
			    width=150;
			    height=180;
			    title="test";
			    fieldname="radiofield1";

			    label3 "widget/radiobutton" 
				{
				label="basketball";
				selected="true";
				}
			    label4 "widget/radiobutton" 
				{
				label="is fun";
				}
			    }
			testdrop1 "widget/dropdown"
			    {
			    x=300;y=130;
			    width=100;
			    bgcolor="#CFCFCF";
			    hilight="green";
			    fieldname="full_name";
			    d1_1 "widget/dropdownitem"
				{
				value="January";
				label="January";
				}
			    d2_1 "widget/dropdownitem"
				{
				value="February";
				label="February";
				}
			    d3_1 "widget/dropdownitem"
				{
				value="March";
				label="March";
				}
			    d4_1 "widget/dropdownitem"
				{
				value="April";
				label="April";
				}
			    d5_1 "widget/dropdownitem"
				{
				value="May";
				label="May";
				}
			    d6_1 "widget/dropdownitem"
				{
				value="June";
				label="June";
				}
			    d7_1 "widget/dropdownitem"
				{
				value="July";
				label="July";
				}
			    d8_1 "widget/dropdownitem"
				{
				value="August";
				label="August";
				}
			    d9_1 "widget/dropdownitem"
				{
				value="September";
				label="September";
				}
			    d10_1 "widget/dropdownitem"
				{
				value="October";
				label="October";
				}
			    d11_1 "widget/dropdownitem"
				{
				value="November";
				label="November";
				}
			    d12_1 "widget/dropdownitem"
				{
				value="December";
				label="December";
				}
			    }
//			testspin "widget/spinner"
//			    {
//			    x=100;y=80;
//			    width=100;height=15;
//			    fieldname="spin1";
//			    }
			}


		    }
	        }
	    }	//end of osrc
	}
    rightNavWindow "widget/childwindow" 
	{
	title = "Right Nav Window";
	bgcolor = "#b0b0b0";
	titlebar = yes;
	style = "floating";
	x = 695; y = 0; width = 135; height = 300;

	rightTabFrame "widget/tab" 
	    {
	    x = 0; y = 0; width = 130; height = 280;
	    bgcolor = "#c0c0c0";
	    rFirst "widget/tabpage"
		{
		title="Buttons";
		formchangebtn13 "widget/textbutton" 
		    {
		    x = 5; y = 5; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "Form to Query";
		    cn12 "widget/connector"
			{
			event="Click";
			target="form1";
			action="Query";
			}
		    }
		formchangebtn14 "widget/textbutton" 
		    {
		    x = 5; y = 35; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "Form to New";
		    cn13 "widget/connector"
			{
			event="Click";
			target="form1";
			action="New";
			}
		    }
		formchangebtn15 "widget/textbutton" 
		    {
		    x = 5; y = 65; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "Form Clear";
		    cn14 "widget/connector"
			{
			event="Click";
			target="form1";
			action="Clear";
			}
		    }
		formchangebtn16 "widget/textbutton" 
		    {
		    x = 5; y = 95; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "Form to Edit";
		    cn15 "widget/connector"
			{
			event="Click";
			target="form1";
			action="Edit";
			}
		    }
		formchangebtn17 "widget/textbutton" 
		    {
		    x = 5; y = 125; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "Form Discard";
		    cn16 "widget/connector"
			{
			event="Click";
			target="form1";
			action="Discard";
			}
		    }
		formchangebtn18 "widget/textbutton" 
		    {
		    x = 5; y = 155; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "Test 3-button confirm";
		    cn17 "widget/connector"
			{
			event="Click";
			target="form1";
			action="test3bconfirm";
			}
		    }
		formchangebtn19 "widget/textbutton" 
		    {
		    x = 5; y = 185; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "";
		    cn18 "widget/connector"
			{
			event="Click";
			target="form1";
			action="";
			}
		    }
		formchangebtn20 "widget/textbutton" 
		    {
		    x = 5; y = 215; height = 30; width = 118;
		    fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
		    text = "";
		    cn19 "widget/connector"
			{
			event="Click";
			target="form1";
			action="";
			}
		    }
		}
	    }
	}
    Treeview_pane "widget/pane"
        {
	x=0; y=300; width=800; height=300;
	bgcolor="#e0e0e0";
	style=lowered;
	Tree_scroll "widget/scrollpane"
	    {
	    x=0; y=0; width=798; height=298;
	    Tree "widget/treeview"
	        {
		x=0; y=1; width=778;
		source = "javascript:form1";
		}
	    }
	}
    }
