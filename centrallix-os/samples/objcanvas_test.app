$Version=2$

objcanvas_test "widget/page"
    {
    title = "ObjectCanvas Test Application - Collegiate Peaks, Colorado, USA";
    background="/sys/images/slate2.gif";
    //datafocus1="white";
    //datafocus2="white";
    x=0; y=0; width=640; height=480;

    widget_template = "/samples/objcanvas_test.tpl";
    
    osrc1 "widget/osrc"
	{
	sql = "select :x, :y, :width, :height, :color, :image, :type, :description from /samples/CanvasObjects.csv/rows";
	baseobj = "/samples/CanvasObjects.csv/rows";
	replicasize = 32;
	readahead = 1;
	autoquery = onload;

	win1 "widget/childwindow"
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
		    widget_class="FirstRecord";
		    x=11; y=11;
		    template_form = form1;
		    }
		btnBack "widget/imagebutton"
		    {
		    widget_class="PrevRecord";
		    x=31; y=11;
		    template_form = form1;
		    }
		btnNext "widget/imagebutton"
		    {
		    widget_class="NextRecord";
		    x=147; y=11; 
		    template_form = form1;
		    }
		btnLast "widget/imagebutton"
		    {
		    widget_class="LastRecord";
		    x=169; y=11; 
		    template_form = form1;
		    }
		btnSearch "widget/textbutton"
		    {
		    x=250; y=10;
		    text="Search";
		    enabled = runclient(:form1:is_queryable or :form1:is_queryexecutable);
		    cnSearch "widget/connector" { event="Click"; target="form1"; action="QueryToggle"; }
		    }
		btnEdit "widget/textbutton"
		    {
		    x=314; y=10;
		    text="Edit";
		    enabled = runclient(:form1:is_editable);
		    cnEdit "widget/connector" { event="Click"; target="form1"; action="Edit"; }
		    }
		btnSave "widget/textbutton"
		    {
		    x=378; y=10;
		    text="Save";
		    enabled = runclient(:form1:is_savable);
		    cnSave "widget/connector" { event="Click"; target="form1"; action="Save"; }
		    }
		btnCancel "widget/textbutton"
		    {
		    x=442; y=10;
		    text="Cancel";
		    enabled = runclient(:form1:is_discardable);
		    cnCancel "widget/connector" { event="Click"; target="form1"; action="Discard"; }
		    }

		lblMapX "widget/label" { x=4; y=40; text="Map X:"; }
		ebMapX "widget/editbox" { x=90; y=40; width=60; fieldname="x"; }

		lblMapY "widget/label" { x=4; y=64; text="Map Y:"; }
		ebMapY "widget/editbox" { x=90; y=64; width=60; fieldname="y"; }

		lblMapWidth "widget/label" { x=4; y=88; text="Map Width:"; }
		ebMapWidth "widget/editbox" { x=90; y=88; width=60; fieldname="width"; }

		lblMapHeight "widget/label" { x=4; y=112; text="Map Height:"; }
		ebMapHeight "widget/editbox" { x=90; y=112; width=60; fieldname="height"; }

		lblMapImage "widget/label" { x=160; y=40; text="Map Image:"; }
		ebMapImage "widget/editbox" { x=250; y=40; width=250; fieldname="image"; }

		lblMapColor "widget/label" { x=160; y=64; text="Map Color:"; }
		ebMapColor "widget/editbox" { x=250; y=64; width=250; fieldname="color"; }

		lblObjType "widget/label" { x=160; y=88; text="Type:"; }
		//ebObjType "widget/editbox" { x=250; y=88; width=250; fieldname="type"; }
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

		lblObjDesc "widget/label" { x=160; y=112; text="Description:"; }
		ebObjDesc "widget/editbox" { x=250; y=112; width=250; fieldname="description"; }
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
