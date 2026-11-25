$Version=2$
// Large result set table test with fixed row heights and a row detail.
table_large_rowdetail_test "widget/page"
    {
    title = "Table Large Result Set Row Detail Test";
    bgcolor = "#d2d3d6";
    x = 0; y = 0; width = 1000; height = 700;

    osrc "widget/osrc"
	{
	sql = "SELECT :Id, :Name, :Grp, :Amount, :Notes FROM /tests/ui/table_large/table_large.csv/rows ORDER BY :Id";
	baseobj = "/tests/ui/table_large/table_large.csv/rows";
	replicasize = 50;
	readahead = 25;
	scrollahead = 25;

	form "widget/form"
	    {
	    table_pn "widget/pane"
		{
		x = 20; y = 20; width = 620; height = 440;
		bgcolor = "white";

		table "widget/table"
		    {
		    x = 10; y = 10; width = 598; height = 418;
		    min_rowheight = 20;
		    max_rowheight = 20;
		    row1_bgcolor = "#ffffff";
		    row2_bgcolor = "#e0e0e0";
		    rowhighlight_bgcolor = "#000080";
		    newrow_bgcolor = "#80ff80";
		    hdr_bgcolor = "#c0c0c0";
		    textcolorhighlight = "#ffffff";
		    textcolor = "black";
		    colsep = 0;

		    col_id "widget/table-column" { fieldname = "Id"; title = "Id"; width = 60; }
		    col_name "widget/table-column" { fieldname = "Name"; title = "Name"; width = 120; }
		    col_grp "widget/table-column" { fieldname = "Grp"; title = "Group"; width = 100; }
		    col_amount "widget/table-column" { fieldname = "Amount"; title = "Amount"; width = 80; }
		    col_notes "widget/table-column" { fieldname = "Notes"; title = "Notes"; width = 220; }

		    detail "widget/table-row-detail"
			{
			height = 200;
			detail_lbl "widget/label" { x = 10; y = 10; width = 400; height = 24; value = runclient("Detail for " + :osrc:Name); }
			}
		    }
		}

	    ctl_pn "widget/pane"
		{
		x = 660; y = 20; width = 320; height = 440;
		bgcolor = "white";

		name_eb "widget/editbox" { x = 10; y = 10; width = 200; height = 24; fieldname = "Name"; bgcolor = "white"; }
		amount_eb "widget/editbox" { x = 10; y = 44; width = 200; height = 24; fieldname = "Amount"; bgcolor = "white"; }

		first_btn "widget/textbutton" { x = 10; y = 90; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "First"; first_cn "widget/connector" { event = Click; target = form; action = First; } }
		prev_btn "widget/textbutton" { x = 110; y = 90; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "Prev"; prev_cn "widget/connector" { event = Click; target = form; action = Prev; } }
		next_btn "widget/textbutton" { x = 210; y = 90; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "Next"; next_cn "widget/connector" { event = Click; target = form; action = Next; } }
		last_btn "widget/textbutton" { x = 10; y = 128; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "Last"; last_cn "widget/connector" { event = Click; target = form; action = Last; } }
		refresh_btn "widget/textbutton" { x = 110; y = 128; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "Refresh"; refresh_cn "widget/connector" { event = Click; target = osrc; action = Refresh; } }
		new_btn "widget/textbutton" { x = 10; y = 166; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "New"; new_cn "widget/connector" { event = Click; target = form; action = New; } }
		save_btn "widget/textbutton" { x = 110; y = 166; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "Save"; save_cn "widget/connector" { event = Click; target = form; action = Save; } }
		cancel_btn "widget/textbutton" { x = 210; y = 166; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "Cancel"; cancel_cn "widget/connector" { event = Click; target = form; action = Discard; } }
		delete_btn "widget/textbutton" { x = 10; y = 204; width = 90; height = 28; bgcolor = "#c0c0c0"; text = "Delete"; delete_cn "widget/connector" { event = Click; target = form; action = Delete; } }

		id_eb "widget/editbox" { x = 10; y = 290; width = 200; height = 24; fieldname = "Id"; bgcolor = "white"; }
		grp_eb "widget/editbox" { x = 10; y = 324; width = 200; height = 24; fieldname = "Grp"; bgcolor = "white"; }
		notes_eb "widget/editbox" { x = 10; y = 358; width = 300; height = 24; fieldname = "Notes"; bgcolor = "white"; }

		cur_lbl "widget/label" { x = 10; y = 250; width = 300; height = 24; value = runclient(:osrc:Name); }
		}
	    }
	}
    }
