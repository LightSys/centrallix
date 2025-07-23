$Version=2$
pay "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Project CSV Data";
    row_annot_exp = ":Name";
    key_is_rowid = yes;

    // Column specifications.
    Name "filespec/column" { type=string; id=1; }
    Start "filespec/column" { type=datetime; id=2; }
    End "filespec/column" { type=datetime; id=3; }
    CurrentWeek "filespec/column" { type=integer; id=4; }
    Owner "filespec/column" { type=string; id=5; }
    }