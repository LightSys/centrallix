$Version=2$
computers "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Files CSV Data";
    row_annot_exp = ":file_name";
    key_is_rowid = yes;

    // Column specifications.
    fileid "filespec/column"
	{
	type=integer;
	id=1;
	}

    computer_name "filespec/column"
	{
	type=string;
	id=2;
	}

    parentid "filespec/column"
	{
	type=integer;
	id=3;
	}
    file_name "filespec/column"
	{
	type=string;
	id=4;
	}
    }
