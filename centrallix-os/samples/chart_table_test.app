$Version=2$
index "widget/page"
    {
    width=1000;
    height=700;
    title = "Table and Chart Test App";
    bgcolor = "#e0e0e0";

    test_vbox "widget/vbox"
	{
	x=10; y=10;
	width=980;
	height=680;
	spacing=10;

	hdr_label "widget/label"
	    {
	    height=18;
	    font_size=18;
	    style=bold;
	    align=center;
	    text = "Table and Chart Test App";
	    }

	test_osrc "widget/osrc"
	    {
	    sql = "select :size, :permissions, :name from /";
	    readahead=100;
	    replicasize=100;

	    test_hbox "widget/hbox"
		{
		height=652;
		spacing=10;

		table_pane "widget/pane"
		    {
		    width=485;
		    style=lowered;

		    table "widget/table"
			{
			x=0; y=0;
			width=483;
			height=650;
			hdr_bgcolor="#d0d0d0";
			allow_selection = yes;
			show_selection = yes;
			demand_scrollbar = yes;
			overlap_scrollbar = yes;
			colsep = 0;
			inner_padding = 2;
			cellvspacing = 2;
			row1_bgcolor = "#ffffff";
			row2_bgcolor = "#f0f0f0";
			textcolor = black;
			rowhighlight_bgcolor = "#0000a0";
			textcolorhighlight = white;
			row_minheight=16;
			row_maxheight=128;
			rowheight = null;

			tc_name "widget/table-column" { title="Name"; fieldname=name; }
			tc_size "widget/table-column" { title="Size"; fieldname=size; }
			}
		    }

		chart_pane "widget/pane"
		    {
		    width=485;
		    style=lowered;
		    
		    chart "widget/chart"
			{
			x=0; y=0;
			width=483;
			height=650;
			chart_type="bar";
			title="Sample Chart";
			titlecolor="orange";
			legend_position="right";
			start_at_zero=no;
			title_size=16;

			series2 "widget/chart-series" {color="orange"; y_column="size";}
			
			y_axis "widget/chart-axis" {axis="y"; label="Y";}
			x_axis "widget/chart-axis" {label="X";}

			}

		    }
		}
	    }
	}
    }
