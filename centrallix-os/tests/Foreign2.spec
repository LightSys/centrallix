$Version=2$
Foreign2 "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "Foreign language strings and substrings";
    //row_annot_exp = ":full_name";
    key_is_rowid = yes;
    new_row_padding = 8;

    // Column specifications.
    Full "filespec/column" { type=string; id=1; }
    Part "filespec/column" { type=string; id=1; }
    }
