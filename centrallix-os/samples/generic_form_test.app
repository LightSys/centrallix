$Version=2$
generic_form_test "widget/page"
    {
    height=600; width=800;
    title = "Generic form component - test";
    bgcolor = "#e0e0e0";

    csv_list_osrc "widget/osrc"
	{
	sql = "select path = '/lightsys/kardia_DB/' + :name from /lightsys/kardia_DB";
	//sql = "select path = '/samples/' + :name from /samples where right(:name, 4) == '.csv'";
	autoquery = onload;
	baseobj = "/lightsys/kardia_DB";
	replicasize = 25;
	readahead = 12;

	csv_list_form "widget/form" 
	    {
	    path_eb "widget/editbox"
		{
		x=10; y=180; width=200; height=20;
		bgcolor=white;
		fieldname=path;
		}
	    }

	csv_list_pane "widget/pane"
	    {
	    x=10; y=10; width=780; height=160;
	    style=lowered;
	    bgcolor="#f0f0f0";

	    csv_list_table "widget/table"
		{
		x=0;y=0;width=778;height=158;
		mode=dynamicrow;

		row1_bgcolor = "#ffffff";
		row2_bgcolor = "#e0e0e0";
		rowhighlight_bgcolor = "#000080";
		rowheight = 20;
		hdr_bgcolor = "#c0c0c0";
		textcolorhighlight = "#ffffff";
		textcolor = "#000000";
		gridinemptyrows = 1;

		pathname "widget/table-column"
		    {
		    fieldname=path;
		    title = "Path Name";
		    }

		sel_conn "widget/connector" 
		    {
		    event = "DblClick";
		    target = cmp;
		    action = "Instantiate";
		    table=runclient(:csv_list_form:path);
		    title=runclient(:csv_list_form:path);
		    }
		}
	    }
	}

    cmp "widget/component"
	{
	//table = "/samples/files.csv";
	path = "/sys/cmp/window_container.cmp";
	component = "/sys/cmp/generic_form.cmp";
	mode = dynamic;
	width=798; height=476; h=476;
	multiple_instantiation = yes;
	}
    }
