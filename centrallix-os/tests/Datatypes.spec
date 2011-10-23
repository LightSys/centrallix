$Version=2$
Datatypes "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "Datatypes test CSV Data";
    //row_annot_exp = ":full_name";
    key_is_rowid = yes;

    // Column specifications.
    f_integer "filespec/column" { type=integer; id=1; }
    f_string "filespec/column" { type=string; id=2; }
    f_money "filespec/column" { type=money; id=3; }
    f_datetime "filespec/column" { type=datetime; id=4; }
    f_double "filespec/column" { type=double; id=5; }
    }
