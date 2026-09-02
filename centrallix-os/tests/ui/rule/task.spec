$Version=2$
pay "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Task CSV Data";
    row_annot_exp = ":Name";
    key_is_rowid = no;

    // Column specifications.
    TaskID "filespec/column" { type=integer; id=1; }
    Project "filespec/column" { type=string; id=2; }
    Name "filespec/column" { type=string; id=3; }
    Description "filespec/column" { type=string; id=4; }
    Hours "filespec/column" { type=integer; id=5; }
    Percentage "filespec/column" { type=integer; id=6; }
    }