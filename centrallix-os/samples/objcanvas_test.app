$Version=2$

objcanvas_test "widget/page"
    {
    title = "ObjectCanvas Test Application - Collegiate Peaks, Colorado, USA";
    background="/sys/images/slate2.gif";
    //datafocus1="white";
    //datafocus2="white";

    osrc1 "widget/osrc"
	{
	sql = "select :x, :y, :width, :height, :color, :image, :type, :description from /samples/CanvasObjects.csv/rows";
	baseobj = "/samples/CanvasObjects.csv/rows";
	replicasize = 32;
	readahead = 1;
	autoquery = onload;

	win1 "widget/htmlwindow"
	    {
	    x=60; y=280; height=170; width=520;
	    visible=yes;
	    style=dialog;
	    hdr_bgcolor="#e0e0e0";
	    background="/samples/e0_2x2.gif";
	    title="Map Object Browser";

	    form1 "widget/form"
		{
		fs "widget/formstatus"
		    {
		    x=53; y=10; style=largeflat;
		    }
		btnFirst "widget/imagebutton"
		    {
		    x=11; y=11; width=18; height=18;
		    image="/sys/images/ico16aa.gif";
		    pointimage="/sys/images/ico16ab.gif";
		    clickimage="/sys/images/ico16ac.gif";
		    disabledimage="/sys/images/ico16ad.gif";
		    enabled = runclient(:form1:recid > 1);
		    cnFirst "widget/connector" { event="Click"; target=form1; action="First"; }
		    }
		btnBack "widget/imagebutton"
		    {
		    x=31; y=11; width=18; height=18;
		    image="/sys/images/ico16ba.gif";
		    pointimage="/sys/images/ico16bb.gif";
		    clickimage="/sys/images/ico16bc.gif";
		    disabledimage="/sys/images/ico16bd.gif";
		    enabled = runclient(:form1:recid > 1);
		    cnBack "widget/connector" { event="Click"; target=form1; action="Prev"; }
		    }
		btnNext "widget/imagebutton"
		    {
		    x=147; y=11; width=18; height=18;
		    image="/sys/images/ico16ca.gif";
		    pointimage="/sys/images/ico16cb.gif";
		    clickimage="/sys/images/ico16cc.gif";
		    disabledimage="/sys/images/ico16cd.gif";
		    enabled = runclient(not(:form1:recid == :form1:lastrecid));
		    cnNext "widget/connector" { event="Click"; target=form1; action="Next"; }
		    }
		btnLast "widget/imagebutton"
		    {
		    x=169; y=11; width=18; height=18;
		    image="/sys/images/ico16da.gif";
		    pointimage="/sys/images/ico16db.gif";
		    clickimage="/sys/images/ico16dc.gif";
		    disabledimage="/sys/images/ico16dd.gif";
		    enabled = runclient(not(:form1:recid == :form1:lastrecid));
		    cnLast "widget/connector" { event="Click"; target=form1; action="Last"; }
		    }
		btnSearch "widget/textbutton"
		    {
		    x=250; y=10; width=60; height=21;
		    text="Search";
		    tristate=no;
		    fgcolor1=black;fgcolor2=white;
		    background="/sys/images/grey_gradient.png";
		    enabled = runclient(:form1:is_queryable or :form1:is_queryexecutable);
		    cnSearch "widget/connector" { event="Click"; target="form1"; action="QueryToggle"; }
		    }
		btnEdit "widget/textbutton"
		    {
		    x=314; y=10; width=60; height=21;
		    text="Edit";
		    tristate=no;
		    fgcolor1=black;fgcolor2=white;
		    background="/sys/images/grey_gradient.png";
		    enabled = runclient(:form1:is_editable);
		    cnEdit "widget/connector" { event="Click"; target="form1"; action="Edit"; }
		    }
		btnSave "widget/textbutton"
		    {
		    x=378; y=10; width=60; height=21;
		    text="Save";
		    tristate=no;
		    fgcolor1=black;fgcolor2=white;
		    background="/sys/images/grey_gradient.png";
		    enabled = runclient(:form1:is_savable);
		    cnSave "widget/connector" { event="Click"; target="form1"; action="Save"; }
		    }
		btnCancel "widget/textbutton"
		    {
		    x=442; y=10; width=60; height=21;
		    text="Cancel";
		    tristate=no;
		    fgcolor1=black;fgcolor2=white;
		    background="/sys/images/grey_gradient.png";
		    enabled = runclient(:form1:is_discardable);
		    cnCancel "widget/connector" { event="Click"; target="form1"; action="Discard"; }
		    }

		lblMapX "widget/label" { x=4; y=40; width=80; height=20; text="Map X:"; align=right; }
		ebMapX "widget/editbox" { x=90; y=40; width=60; height=20; bgcolor=white; fieldname="x"; }

		lblMapY "widget/label" { x=4; y=64; width=80; height=20; text="Map Y:"; align=right; }
		ebMapY "widget/editbox" { x=90; y=64; width=60; height=20; bgcolor=white; fieldname="y"; }

		lblMapWidth "widget/label" { x=4; y=88; width=80; height=20; text="Map Width:"; align=right; }
		ebMapWidth "widget/editbox" { x=90; y=88; width=60; height=20; bgcolor=white; fieldname="width"; }

		lblMapHeight "widget/label" { x=4; y=112; width=80; height=20; text="Map Height:"; align=right; }
		ebMapHeight "widget/editbox" { x=90; y=112; width=60; height=20; bgcolor=white; fieldname="height"; }

		lblMapImage "widget/label" { x=160; y=40; width=80; height=20; text="Map Image:"; align=right; }
		ebMapImage "widget/editbox" { x=250; y=40; width=250; height=20; bgcolor=white; fieldname="image"; }

		lblMapColor "widget/label" { x=160; y=64; width=80; height=20; text="Map Color:"; align=right; }
		ebMapColor "widget/editbox" { x=250; y=64; width=250; height=20; bgcolor=white; fieldname="color"; }

		lblObjType "widget/label" { x=160; y=88; width=80; height=20; text="Type:"; align=right; }
		//ebObjType "widget/editbox" { x=250; y=88; width=250; height=20; bgcolor=white; fieldname="type"; }
		ddObjType "widget/dropdown"
		    {
		    x=250;y=88;width=250;height=20;
		    bgcolor=white;
		    hilight="#e0e0e0";
		    mode=static;
		    fieldname="type";

		    dd_city "widget/dropdownitem" { label = "City, Town, or Settlement"; value = "city"; }
		    dd_peak "widget/dropdownitem" { label = "Mountain Peak"; value = "peak"; }
		    dd_road "widget/dropdownitem" { label = "Road or Highway"; value = "road"; }
		    }

		lblObjDesc "widget/label" { x=160; y=112; width=80; height=20; text="Description:"; align=right; }
		ebObjDesc "widget/editbox" { x=250; y=112; width=250; height=20; bgcolor=white; fieldname="description"; }
		}
	    }

	pn1 "widget/pane"
	    {
	    x=10; y=10; width=620; height=420;
	    style=raised;
	    bgcolor="#a0a0a0";

	    pn2 "widget/pane"
		{
		x=9; y=9; width=602; height=402;
		style=lowered;
		oc1 "widget/objcanvas"
		    {
		    x=0; y=0; width=600; height=400;
		    background="/samples/collegiate_peaks.jpg";
		    source = osrc1;
		    allow_selection = yes;
		    show_selection = yes;
		    }
		}
	    }
	}
    }
