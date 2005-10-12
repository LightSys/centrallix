$Version=2$
in_main "widget/page" 
    {
    bgcolor="#195173";
    width=640; height=480;
    
    mn "widget/menu" 
	{
	x=0;y=0;
	width=640;
	//height=30;
	bgcolor="#afafaf";
	highlight_bgcolor="#cfcfcf";
	active_bgcolor="#efefef";

	mna "widget/menu" 
	    { 
	    icon="/sys/images/ico02a.gif"; 
	    label="File";
	    bgcolor="#afafaf";
	    highlight_bgcolor="#cfcfcf";
	    active_bgcolor="#efefef";
	    row_height=18;
	    mnaa "widget/menuitem" { label="New..."; }
	    mnab "widget/menuitem" { label="Open..."; }
	    mnac "widget/menuitem" { label="Quit"; }
	    mnad "widget/menu" 
		{ 
		label="Recent Files"; 
		bgcolor="#afafaf";
		highlight_bgcolor="#cfcfcf";
		active_bgcolor="#efefef";
		row_height=18;
		file1 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="SomeFile.txt"; }
		file2 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="A_Document.sdw"; }
		file3 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="A_Presentation.sdd"; }
		}
	    }
	mnb "widget/menu" 
	    { 
	    icon="/sys/images/ico01a.gif"; 
	    label="Edit";
	    bgcolor="#afafaf";
	    highlight_bgcolor="#cfcfcf";
	    active_bgcolor="#efefef";
	    row_height=18;
	    mnba "widget/menuitem" { label="Cut"; }
	    mnbb "widget/menuitem" { label="Copy"; }
	    mnbc "widget/menuitem" { label="Paste"; }
	    }
	mnc "widget/menuitem" { value=runclient(:mn1c:value); checked=yes; label="View"; }
	mnd "widget/menuitem" { label="Help"; onright=yes; }
	}

    mn2 "widget/menu"
	{
	x=100;y=100;
	width=100;
	row_height=18;
	direction=vertical;
	popup=no;
	bgcolor="#afafaf";
	highlight_bgcolor="#cfcfcf";
	active_bgcolor="#efefef";
	mn1a "widget/menuitem" { icon="/sys/images/ico02a.gif"; label="File"; }
	mn1b "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="Edit"; }
	mn1c "widget/menuitem" { value=runclient(:mnc:value); checked=yes; label="View"; }
	mn1d "widget/menuitem" { label="Help"; onright=yes; }
	}

    cn1 "widget/connector"
	{
	event="RightClick";
	target=mna;
	action="Popup";
	X=runclient(:X);
	Y=runclient(:Y);
	}
    }
