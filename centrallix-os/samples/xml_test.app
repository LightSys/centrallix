$Version=2$
wholePage "widget/page" 
    {
    title = "XML & HTTP Test page";
    background="/sys/images/slate2.gif";
    textcolor = black;
    x=0; y=0; width=640; height=480;
    
    btnDebug "widget/textbutton" {
	x=710; y=10; width=90; height=25;
	fgcolor1 = "#000000"; fgcolor2 = "#cfcfcf";
	text="";
	cn3 "widget/connector" {
	    event="Click"; target="debugWindow"; action="SetVisibility";
	}
    }

    navWindow "widget/childwindow"
	{
	title="FreeBSD Page";
	height=500;
	width=700;
	x=50;y=20;
	hdr_bgcolor="#AA3333";
	bgcolor="#808080";
	style="dialog";

	sp "widget/scrollpane"
	    {
	    x=0;y=0;
	    width=680;
	    height=470;
	    pane1 "widget/pane" 
		{
		x=0; y=0; height=5000; width=680;
		style="lowered";
		bgcolor="#b8b8b8";
		htmlarea "widget/html"
		    {
		    width=680;
		    //height=5000;
		    x=0; y=0;
		    mode=dynamic;
		    }
		}
	    }
	}

    selectWindow "widget/childwindow" 
	{
	title = "XML & HTTP Test page";
	height=210;
	width=722;
	x=20;y=50;
	hdr_bgcolor="#880000";
	bgcolor="#c0c0c0";
	style="dialog";

	lbl1 "widget/label"
	    {
	    x=10;y=10;width=702;height=20;
	    text="<b>Current FreeBSD News Headlines (from http://freebsd.org):</b>";
	    }

	osrc1 "widget/osrc"
	    {
	    sql = "select :title,:link from /samples/freebsd.http/news/news.rdf where :internal_type='item'";
	    readahead=3;
	    replicasize=25; //this is a really slow query -- get it all...

	    pane2 "widget/pane" {
		x=10; y=30; height=142; width=702;
		style="lowered";
		bgcolor="#b8b8b8";

		alerter "widget/alerter" {}
		tblMonths "widget/table" 
		    {
		    mode="dynamicrow";
		    width=700;
		    height=140;
		    rowheight=20;
		    x=0;y=0;
		    cellhspacing=1;
		    cellvspacing=1;
		    inner_border=1;
		    inner_padding=1;
		    bgcolor="#c0c0c0";
		    row_bgcolor1="#c0c0c0";
		    row_bgcolor2="#a0a0a0";
		    row_bgcolorhighlight="black";
		    hdr_bgcolor="white";
		    textcolor="black";
		    textcolorhighlight="white";
		    titlecolor="red";
		    title "widget/table-column" { fieldname="title"; title="Title";width=200; }
		    link "widget/table-column" { fieldname="link"; title="Link"; width=500; }
		    cn4 "widget/connector"
			{
			//event="Click"; target="htmlarea"; action="LoadPage"; Source="new String(eparam.data.link).replace('http:..slashdot.org','/samples/slashdot.http')";
			event="Click"; target="htmlarea"; action="LoadPage"; Source="eparam.data.link";
			}
		    }
		}
	    }
	}
    debugWindow "widget/childwindow"
	{
	x=100;y=100;width=800;height=530;
	visible=false;
	Treeview_pane "widget/pane"
	    {
	    x=0; y=0; width=800; height=500;
	    bgcolor="#e0e0e0";
	    style=lowered;
	    Tree_scroll "widget/scrollpane"
		{
		x=0; y=0; width=798; height=498;
		Tree "widget/treeview"
		    {
		    x=0; y=1; width=20000;
		    source = "javascript:osrc1";
		    }
		}
	    }
	}
    }
