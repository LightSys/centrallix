$Version=2$
wholePage "widget/page" 
    {
    title = "XML & HTTP Test page";
    background="/sys/images/slate2.gif";
    textcolor = black;

    navWindow "widget/htmlwindow" 
	{
	title = "XML & HTTP Test page";
	height=210;
	width=722;
	x=20;y=20;
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
	    sql = "select :title,:link from /samples/freebsd.http/news/news.rdf/item";
	    readahead=3;
	    replicasize=25; //this is a really slow query -- get it all...

	    pane1 "widget/pane" {
		x=10; y=30; height=142; width=702;
		style="lowered";
		bgcolor="#b8b8b8";

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
		    title "widget/table-column" { title="Title";width=200; }
		    link "widget/table-column" { title="Link"; width=500; }
		    }
		}
	    }
	}
    }
