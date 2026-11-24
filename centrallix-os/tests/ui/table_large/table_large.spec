$Version=2$
table_large "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Large result set table test data";
    row_annot_exp = ":Name";
    key_is_rowid = yes;

    // Column specifications.
    Id "filespec/column" { type=integer; id=1; }
    Name "filespec/column" { type=string; id=2; }
    Grp "filespec/column" { type=string; id=3; }
    Amount "filespec/column" { type=integer; id=4; }
    Notes "filespec/column" { type=string; id=5; }
    }
