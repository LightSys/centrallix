$Version=2$
sales "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "Report test sales data";
    key_is_rowid = no;
    two_quote_escape = yes;

    // Column specifications.
    id "filespec/column" { type=integer; id=1; key=yes; }
    region "filespec/column" { type=string; id=2; }
    product "filespec/column" { type=string; id=3; }
    qty "filespec/column" { type=integer; id=4; }
    price "filespec/column" { type=money; id=5; }
    sold "filespec/column" { type=datetime; id=6; }
    rating "filespec/column" { type=double; id=7; }
    note "filespec/column" { type=string; id=8; }
    }
