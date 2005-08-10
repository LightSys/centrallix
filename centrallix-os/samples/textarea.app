$Version=2$
main "widget/page"
    {
    bgcolor="#213e87";
    x=0; y=0; width=340; height=340;
    
    window1 "widget/htmlwindow"
    	{
	bgcolor="#c0c0c0";
	x=20; y=20; width=300; height=300;
	hdr_bgcolor="#ffcc00";
	title="Textarea Demo";
	style=window;

	textarea1 "widget/textarea"
	    {
	    x=15; y=15; width=150; height=200;
	    bgcolor="white";
	    }
	}

}
