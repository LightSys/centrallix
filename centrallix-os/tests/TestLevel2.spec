$Version=2$
TestLevel2 "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "Nested test data file level 2";
    //row_annot_exp = ":full_name";
    key_is_rowid = yes;
    new_row_padding = 8;

    // Column specifications.
    Sheen "filespec/column" { type=string; id=1; }
    Description "filespec/column" { type=string; id=2; }
    }
