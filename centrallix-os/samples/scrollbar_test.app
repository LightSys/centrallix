$Version=2$
scrollbar_test "widget/page"
    {
    title = "Scrollbar Test";
    bgcolor="#a0a0a0";
    width=640; height=480;

    title_label "widget/label"
	{
	x=0;y=0;width=640;height=32;
	text = "Test of Scrollbar Widgets:";
	fontsize = 5;
	align=center;
	}

    vscroll "widget/scrollbar"
	{
	x=10;y=60;height=128;
	direction=vertical;
	background = "/samples/tube_vert.png";
	range=runclient(:ebVrange:content);
	}
    pane1 "widget/pane"
	{
	bgcolor="#404040";
	x=28;y=60;height=128;width=192;

	ebVrange "widget/editbox"
	    {
	    x=80;y=10;height=20;width=100;
	    bgcolor="#e0e0e0";
	    content=runclient(:vscroll:range);
	    }
	lbVrange "widget/label"
	    {
	    x=10;y=10;height=20;width=65;
	    align=right;
	    text="Range:";
	    fgcolor=white;
	    }
	ebVpos "widget/editbox"
	    {
	    x=80;y=40;height=20;width=100;
	    bgcolor="#e0e0e0";
	    content=runclient(:vscroll:value);
	    }
	lbVpos "widget/label"
	    {
	    x=10;y=40;height=20;width=65;
	    align=right;
	    text="Position:";
	    fgcolor=white;
	    }
	}

    hscroll "widget/scrollbar"
	{
	x=256;y=188;width=192;
	direction=horizontal;
	background = "/samples/tube_horiz.png";
	range=runclient(:ebHrange:content);
	}
    pane2 "widget/pane"
	{
	bgcolor="#404040";
	x=256;y=60;height=128;width=192;

	ebHrange "widget/editbox"
	    {
	    x=80;y=10;height=20;width=100;
	    bgcolor="#e0e0e0";
	    content=runclient(:hscroll:range);
	    }
	lbHrange "widget/label"
	    {
	    x=10;y=10;height=20;width=65;
	    align=right;
	    text="Range:";
	    fgcolor=white;
	    }
	ebHpos "widget/editbox"
	    {
	    x=80;y=40;height=20;width=100;
	    bgcolor="#e0e0e0";
	    content=runclient(:hscroll:value);
	    }
	lbHpos "widget/label"
	    {
	    x=10;y=40;height=20;width=65;
	    align=right;
	    text="Position:";
	    fgcolor=white;
	    }
	}
    }
