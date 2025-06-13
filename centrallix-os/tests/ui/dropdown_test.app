//Minsik Lee May 2025 

$Version=2$
dropdown_test "widget/page"
{
    title = "Dropdown Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=800; height=800;
    
		ddlbl "widget/label"
		{
			width=160; x=10; y=10;
			font_size = 24;
			style = "bold";
			value = "dropdown label";
		}

    Dropdown1 "widget/dropdown"
		{
			name="Dropdown1";
			x=10; y=50; width=160; height=20;
			hilight="#b5b5b5";
			bgcolor="#c0c0c0";
			fieldname="value";

			dd1item1 "widget/dropdownitem"
			{
				label="Male";
				value="0";
			}
			dd1item2 "widget/dropdownitem"
			{
				label="Female";
				value="1";
			}

			dd1cnt "widget/connector"
			{
				action=SetValue;
				event=DataChange;
				target=ddlbl;
				Value=runclient(:Dropdown1:value);
			}
		}

	dd_osrc "widget/osrc"
	{
		replicasize=20;
		readahead=20;
		sql="SELECT :State,:Population FROM /tests/ui/dropdown_test.csv/rows";

		Dropdown2 "widget/dropdown"
		{
			mode="objectsource";
			fieldname="State";
			x=200; y=50; width=160; height=20;
			hilight="#b5b5b5";
			bgcolor="#c0c0c0";

			dd2cnt "widget/connector"
			{
				action=SetValue;
				event=DataChange;
				target=dd2lbl;
				Value=runclient(:Dropdown2:value);
			}
		}
	}

	dd2lbl "widget/label"
	{
		width = 160; x=200; y=10;
		font_size = 24;
		style = "bold";
		value=runclient(:dd_osrc:Population);
	}
}