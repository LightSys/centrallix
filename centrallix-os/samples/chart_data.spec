$Version=2$
UTF8_valid "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "Chart test data";
    //row_annot_exp = ":full_name";
    key_is_rowid = yes;

    // Column specifications.
    key "filespec/column" { type=string; id=1; }
    x "filespec/column" { type=string; id=2; }
    y "filespec/column" { type=double; id=2; }
    }
