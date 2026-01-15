$Version=2$
computers "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Computers CSV Data";
    row_annot_exp = ":computer_name + ' with ' + :memory + 'MB'";
    key_is_rowid = yes;

    // Column specifications.
    first_name "filespec/column"
	{
	type=string;
	id=1;
	}

    computer_name "filespec/column"
	{
	type=string;
	id=2;
	}

    memory "filespec/column"
	{
	type=integer;
	id=3;
	}
    }