$Version=2$
Foreign3 "application/filespec"
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
    English "filespec/column" { type=string; id=1; }
    Translation "filespec/column" { type=string; id=1; }
    Language "filespec/column" { type=string; id=1; }
    }
