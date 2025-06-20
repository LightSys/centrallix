$Version=2$
pay "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Pays CSV Data";
    row_annot_exp = ":Firstname";
    key_is_rowid = yes;

    // Column specifications.
    Firstname "filespec/column" { type=string; id=1; }
    Lastname "filespec/column" { type=string; id=2; }
    Role "filespec/column" { type=string; id=3; }
    Pay "filespec/column" { type=integer; id=4; }
    Day "filespec/column" { type=datetime; id=5; }
    }