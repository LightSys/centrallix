$Version=2$
people "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "People CSV Data";
    row_annot_exp = ":first_name + ' ' + :last_name";
    key_is_rowid = yes;

    // Column specifications.
    first_name "filespec/column"
	{
	type=string;
	id=1;
	}

    last_name "filespec/column"
	{
	type=string;
	id=2;
	}

    email "filespec/column"
	{
	type=string;
	id=3;
	}
    }
