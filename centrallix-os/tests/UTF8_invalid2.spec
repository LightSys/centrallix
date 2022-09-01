$Version=2$
UTF8_valid "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "UTF-8ÿtest CSV Data";
    //row_annot_exp = ":full_name";
    key_is_rowid = yes;

    // Column specifications.
    descripton "filespec/column" { type=string; id=1; }
    UTF8 "filespec/column" { type=string; id=2; }
    }
