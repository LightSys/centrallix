$Version=2$
CsvTableMaintenance "widget/page"
    {
    height=600; width=800;
    title = "CSV Sample Files - Table Maintenance";
    bgcolor = "#e0e0e0";

    mnMain "widget/menu"
	{
	popup=false;
	direction=horizontal;
	bgcolor="#c0c0c0";
	highlight_bgcolor="#f0f0f0";
	active_bgcolor="#ffffff";
	x=0; y=0; width=800; 

	type_rpt "widget/repeat"
	    {
	    sql = "select description='All', prefix=''";

	    mnKardia "widget/menu"
		{
		//condition=runserver(char_length(:type_rpt:prefix) > 0);
		label = runserver(:type_rpt:description + ' Tables');
		popup=yes;
		direction=vertical;
		highlight_bgcolor="#f0f0f0";
		active_bgcolor="#ffffff";
		bgcolor="#c0c0c0";

		tbl_rpt "widget/repeat"
		    {
		    sql = runserver("select :name from /samples where right(:name, 4) = '.csv' order by :name");

		    mnItem "widget/menuitem"
			{
			label = runserver(:tbl_rpt:name);

			mnLink "widget/connector"
			    {
			    event = Select;
			    target = cmp;
			    action = "Instantiate";
			    table=runclient(runserver('/samples/' + :tbl_rpt:name));
			    title=runclient(runserver('/samples/' + :tbl_rpt:name));
			    }
			}
		    }
		}
	    }
	mnReturn "widget/menuitem"
	    {
	    label = "Back to Samples Index";
	    cnMenu "widget/connector" { target="CsvTableMaintenance"; action="LoadPage"; event="Select"; Source="/samples/"; }
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
	icon = "/sys/images/ico26a.gif";
	}
    }
