//
//  NOTE:
//    If this doesn't work for you, you probably need to change
//    the URLs that link to the reports.  Do a search for "http"
//    in this file and change the relevant lines to match your
//    server configuration.
//
$Version=2$
ReportingSystem "widget/page"
    {
    // Page-level settings
    title = "Reporting System";
    background="/sys/images/slate_blue2.png";
    //bgcolor='#c0c0c0';
    textcolor=black;
    //kbdfocus1=black;
    //kbdfocus2=black;
    datafocus1='#2020a0';
    datafocus2='#2020a0';

    // Here's a dialog box to select a report
    dlgSelectReport "widget/htmlwindow"
        {
	x=100; y=100; width=440; height=240;
	title="&nbsp;<B>Select Report</B>";
	style=dialog;
	bgcolor="#e0e0e0";
	hdr_background="/sys/images/grey_gradient2.png";
	visible=false;

	// The list of report files...
	pnFileScrollPane "widget/pane"
	    {
	    x=4; y=4; width=430; height=170;
	    style=lowered;
	    scrFileScroll "widget/scrollpane"
	        {
		x=0; y=0; width=428; height=168;
		tblFileList "widget/table"
		    {
		    sql = "select :name,annotation=condition(:annotation=='','-none-',:annotation) 
		             from /samples";
		    //sql = "select :name, annotation=isnull(:annotation,'-none-')
		    //	    from /samples"
		    mode=static;
		    width=408;
		    inner_border=2;
		    inner_padding=1;
		    bgcolor="#c0c0c0";
		    row_bgcolor1="#e0e0e0";
		    //row_bgcolor2="#d0d0d0";
		    hdr_bgcolor="white";
		    textcolor=black;
		    name1 "widget/table-column" { fieldname="name"; title="Object Name"; width=20; }
		    annotation "widget/table-column" { fieldname="annotation"; title="Annotation"; width=25; }
		    }
		}
	    }

	// Command buttons...
	btnOpenReport "widget/textbutton"
	    {
	    x=70; y=180; width=100; height=30;
	    text = "Open Report";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnDoOpen "widget/connector" { event=Click; target=dlgSelectReport; action=SetVisibility; IsVisible=0; }
	    }
	btnCancelReport "widget/textbutton"
	    {
	    x=260; y=180; width=100; height=30;
	    text = "Cancel Open";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnCancelOpen "widget/connector" { event=Click; target=dlgSelectReport; action=SetVisibility; IsVisible=0; }
	    }
	}

    // This dialog is used for selecting a printer for the report
    dlgSelectPrinter "widget/htmlwindow"
        {
	x=120; y=120; width=440; height=240;
	title="&nbsp;<B>Print Report...</B>";
	style=dialog;
	bgcolor="#e0e0e0";
	hdr_background="/sys/images/grey_gradient2.png";
	visible=false;

	// The list of available printers...
	pnPrinterScrollPane "widget/pane"
	    {
	    x=4; y=4; width=430; height=170;
	    style=lowered;
	    scrPrinterScroll "widget/scrollpane"
	        {
		x=0; y=0; width=428; height=168;
		tblPrinterList "widget/table"
		    {
		    sql = "select :name,annotation=condition(:annotation=='','-none-',:annotation) 
		             from /";
		    mode=static;
		    width=408;
		    inner_border=2;
		    inner_padding=1;
		    bgcolor="#c0c0c0";
		    row_bgcolor1="#e0e0e0";
		    //row_bgcolor2="#d0d0d0";
		    hdr_bgcolor="white";
		    textcolor=black;
		    name2 "widget/table-column" { fieldname="name"; title = "Printer"; width=10; }
		    annotation2 "widget/table-column" { fieldname="annotation"; title = "Description"; width=40; }
		    }
		}
	    }

	// Command buttons...
	btnPrintIt "widget/textbutton"
	    {
	    x=70; y=180; width=100; height=30;
	    text = "Print Report";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnDoPrint "widget/connector" { event=Click; target=dlgSelectPrinter; action=SetVisibility; IsVisible=0; }
	    }
	btnCancelPrint "widget/textbutton"
	    {
	    x=260; y=180; width=100; height=30;
	    text = "Cancel Print";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnCancelPrint "widget/connector" { event=Click; target=dlgSelectPrinter; action=SetVisibility; IsVisible=0; }
	    }
	}

    // This window views the report.
    dlgViewReport "widget/htmlwindow"
        {
	x=20; y=100; width=600; height=360;
	style=window;
	title="&nbsp;<b>View Report</b>";
	bgcolor="#e0e0e0";
	hdr_background="/sys/images/grey_gradient2.png";
	visible=false;
	spViewReport "widget/scrollpane"
	    {
	    x=0; y=0; width=596; height=334;
	    htmlSourceCode "widget/html"
		{
		mode=dynamic;
		x=0; y=0; width=576;
		source = "http://localhost:800/samples/Samples.rpt";
		//source="http://localhost:800/test/index.html";
		}
	    }
	}

    // This window is for editing the source.
    dlgEditSource "widget/htmlwindow"
        {
	x=20; y=100; width=600; height=360;
	style=window;
	title="&nbsp;<B>Edit Source</B>";
	bgcolor="#e0e0e0";
	hdr_background="/sys/images/grey_gradient2.png";
	visible=false;

	spEditSource "widget/scrollpane"
	    {
	    x=0; y=0; width=596; height=334;
	    }
	}

    // This window is for browsing the source code and structure
    dlgViewSource "widget/htmlwindow"
        {
	x=20; y=100; width=600; height=360;
	style=window;
	title="&nbsp;<B>View Source</B>";
	bgcolor="#c0c0c0";
	hdr_background="/sys/images/grey_gradient2.png";
	visible=false;

	// Tabs for source code and structure
	tbViewSource "widget/tab"
	    {
	    x=1; y=1; width=594; height=308;
	    bgcolor="#e0e0e0";
	    tpSourceCode "widget/tabpage"
	        {
		title = "&nbsp;<B>Source&nbsp;Code</B>&nbsp;";
		spSourceCode "widget/scrollpane"
		    {
		    x=0; y=0; width=592; height=306;
		    htmlSourceCode2 "widget/html"
		        {
			mode=dynamic;
			x=0; y=0; width=576;
			source = "http://localhost:800/samples/Samples.rpt?ls__type=text%2fplain";
			}
		    }
		}
	    tpStructure "widget/tabpage"
	        {
		spStructure "widget/scrollpane"
		    {
		    x=0; y=0; width=592; height=306;
		    tvStructure "widget/treeview"
		        {
			x=0; y=0; width=574;
			source = "/samples/Samples.rpt?ls__type=system%2fstructure/";
			}
		    }
		title = "&nbsp;<B>Structure</B>&nbsp;";
		}
	    }
	}

    // This part is the righthand control "pane"...
    pnControlPane "widget/pane"
        {
	x=0; y=0; width=1024; height=80;
	style=raised;
	bgcolor="#e0e0e0";

	// Command buttons
	btnOpen "widget/textbutton"
	    {
	    x=104; y=4; width=100; height=30;
	    text = "Select...";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnOpen "widget/connector" { event=Click; target=dlgSelectReport; action=SetVisibility; IsVisible=1; }
	    }
	btnEnterParams "widget/textbutton"
	    {
	    x=104; y=43; width=100; height=30;
	    text = "Parameters...";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    }
	btnRunReport "widget/textbutton"
	    {
	    x=224; y=4; width=100; height=30;
	    text = "Run Report";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnRun "widget/connector" { event=Click; target=dlgViewReport; action=SetVisibility; IsVisible=1; }
	    }
	btnPrintReport "widget/textbutton"
	    {
	    x=224; y=43; width=100; height=30;
	    text = "Print Report";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnPrint "widget/connector" { event=Click; target=dlgSelectPrinter; action=SetVisibility; IsVisible=1; }
	    }
	btnViewSource "widget/textbutton"
	    {
	    x=344; y=4; width=100; height=30;
	    text = "View Source";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnViewSource "widget/connector" { event=Click; target=dlgViewSource; action=SetVisibility; IsVisible=1; }
	    }
	btnEditSource1 "widget/textbutton"
	    {
	    x=344; y=43; width=100; height=30;
	    text = "Edit Source";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnEditSource "widget/connector" { event=Click; target=dlgEditSource; action=SetVisibility; IsVisible=1; }
	    }
	btnEditSource2 "widget/textbutton"
	    {
	    x=464; y=4; width=100; height=30;
	    text = "Debug Window";
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black;
	    fgcolor2=white;
	    tristate=no;
	    cnEditSource2 "widget/connector" { event=Click; target=debugwin; action=SetVisibility; IsVisible=1; }
	    }

	pnLogoPanel "widget/pane"
	    {
	    x=6; y=4; width=78; height=68;
	    htImageArea "widget/html"
	        {
		mode=static;
		width=76;
		content = "<IMG SRC=/sys/images/lightsys_logo2d.png width=76 height=66>";
		}
	    style=raised;
	    }
	}
    debugwin "widget/htmlwindow"
	{
	x=100;y=100;width=800;height=530;
	visible=false;
	bgcolor="#c0c0c0";
	style=dialog;
	Tree_scroll "widget/scrollpane"
	    {
	    x=0; y=0; width=798; height=504;
	    Tree "widget/treeview"
		{
		x=0; y=1; width=20000;
		source = "javascript:window";
		}
	    }
	}
    }

