$Version=2$
CanvasObjects "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Canvas Objects CSV Data";
    row_annot_exp = ":x + ',' + :y";
    key_is_rowid = yes;

    // Column specifications.
    x "filespec/column"
	{
	type=integer;
	id=1;
	format='10';
	hints "column/hints" { min=0; max=600; }
	}

    y "filespec/column"
	{
	type=integer;
	id=2;
	format='10';
	hints "column/hints" { min=0; max=400; }
	}

    width "filespec/column"
	{
	type=integer;
	id=3;
	format='10';
	hints "column/hints" { min=0; max=600; }
	}

    height "filespec/column"
	{
	type=integer;
	id=4;
	format='10';
	hints "column/hints" { min=0; max=400; }
	}

    color "filespec/column"
	{
	type=string;
	id=5;
	hints "column/hints" { length=10; }
	}

    image "filespec/column"
	{
	type=string;
	id=5;
	hints "column/hints" { length=50; }
	}
    type "filespec/column"
	{
	type=string;
	id=6;
	hints "column/hints" { length=50; }
	}
    description "filespec/column"
	{
	type=string;
	id=7;
	hints "column/hints" { length=50; }
	}
    }
