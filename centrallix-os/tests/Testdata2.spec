$Version=2$
Testdata2 "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "Test data file 2";
    //row_annot_exp = ":full_name";
    key_is_rowid = yes;
    new_row_padding = 8;

    // Column specifications.
    f_id "filespec/column" { type=integer; id=1; }
    f_desc "filespec/column" { type=string; id=2; }
    f_square "filespec/column" { type=integer; id=3; }
    }
