$Version=2$
osrc_rule_filter "widget/page"
    {
    bgcolor="#c0c0c0";
    title = "OSRC Rule test - 'filter' rule type";
    width=800; height=600;

    elbl "widget/label"
	{
	x=32;y=10;width=120;height=20;
	text = "Enter month name:";
	}
    eb "widget/editbox"
	{
	x=32;y=32;width=120;height=20;
	bgcolor=white;
	fgcolor=black;
	}

    osrc "widget/osrc"
	{
	sql = "select msg = 'Month: ' + :full_name + ' (' + :short_name + ')' from /samples/Months.csv/rows";
	replicasize=100;
	readahead=10;
	autoquery=never;

	rl "widget/rule" { ruletype="osrc_filter"; fieldname=full_name; value=runclient(:eb:content); min_chars=1; }

	form "widget/form"
	    {
	    lbl "widget/label"
		{
		x=32; y=64; width=200; height=20;
		fieldname = msg;
		}
	    }
	}
    }
