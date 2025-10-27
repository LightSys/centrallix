$Version=2$
qypivot_data "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "Query Pivot test CSV Data";
    key_is_rowid = yes;

    // Column specifications.
    ID1 "filespec/column" { type=integer; id=1; }
    ID2 "filespec/column" { type=string; id=2; }
    NAME "filespec/column" { type=string; id=3; }
    VALUE "filespec/column" { type=string; id=4; }
    CREATE_DATE "filespec/column" { type=datetime; id=5; }
    CREATE_USER "filespec/column" { type=string; id=6; }
    MODIFY_DATE "filespec/column" { type=datetime; id=7; }
    MODIFY_USER "filespec/column" { type=string; id=8; }
    }
