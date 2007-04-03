$Version=2$
labeled_editbox_test "widget/page"
    {
    title = "Test for Labeled Editbox Component";
    bgcolor="#e8e8e8";
    width=640;
    height=480;

    months_osrc "widget/osrc"
	{
	sql = "select :short_name, :full_name, :num_days, :num_leapyear_days from /samples/Months.csv/rows";
	baseobj = "/samples/Months.csv/rows";
	replicasize=12;
	readahead=12;
	autoquery = onload;

	months_form "widget/form"
	    {
	    months_ctl "widget/component"
		{
		x=0;y=0;width=640;height=24;
		path="/sys/cmp/form_controls.cmp";
		bgcolor="#d0d0d0";
		}
	    months_vbox "widget/vbox"
		{
		x=10;y=40;width=200;cellsize=20; spacing=5;

		// These two have 'form=' unspecified; component automatically
		// sniffs out the widget/form container of the component.
		//
		short_cmp "widget/component" { path="/samples/labeled_editbox.cmp"; field=short_name; text="Abbrev:"; }
		long_cmp "widget/component" { path="/samples/labeled_editbox.cmp"; field=full_name; text="Name:"; }

		// These two have 'form=' specified explicitly.
		//
		n_days "widget/component" { path="/samples/labeled_editbox.cmp"; form=months_form; field=num_days; text="Days:"; }
		n_l_days "widget/component" { path="/samples/labeled_editbox.cmp"; form=months_form; field=num_leapyear_days; text="Leap Days:"; }
		}
	    }
	}
    }
