$Version=2$
in_main "widget/page" {
    bgcolor="#195173";
    x=0; y=0; width=400; height=200;
    
    form1 "widget/form" {
	dda "widget/dropdown" {
	    x=15;y=15;
	    width=120;
	    bgcolor="#afafaf";
	    hilight="#cfcfcf";
	    mode='static';

	    ddb "widget/dropdownitem" {
	       label="Red";
	       value="#ff0000";
	    }
	    ddc "widget/dropdownitem" {
	       label="Green";
	       value="#00ff00";
	    }
	    ddd "widget/dropdownitem" {
	       label="Blue";
	       value="#0000ff";
	    }
	    dde "widget/dropdownitem" {
	       label="White";
	       value="#ffffff";
	    }
	    c1 "widget/connector" {target = colorwin; event = RightClick; action="Instantiate"; backcolor = runclient(:Value);}
	}
    }
    
    colorwin "widget/component"
	{
	wintitle = "color window";
	backcolor = "000000";
	path = "/samples/colorwin.cmp";
	mode = dynamic;
	}
    
}
