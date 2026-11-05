$Version=2$
regions "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "Report test region data";
    key_is_rowid = no;
    two_quote_escape = yes;

    // Column specifications.
    region "filespec/column" { type=string; id=1; key=yes; }
    manager "filespec/column" { type=string; id=2; }
    target "filespec/column" { type=money; id=3; }
    }
