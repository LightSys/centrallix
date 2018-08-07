$Version=2$
main "widget/page"
    {
    bgcolor="#213e87";
    x=0; y=0; width=740; height=740;

    window1 "widget/childwindow"
    	{
	bgcolor="#c0c0c0";
	x=20; y=20; width=600; height=600;
	hdr_bgcolor="#ffcc00";
	title="richTextarea Demo";
	style=window;

	textarea1 "widget/richtextedit"
	    {
            fieldname=helloworld;
            x=15; y=15; width=500; height=500;
            bgcolor="white";
	    }
	}
    }
