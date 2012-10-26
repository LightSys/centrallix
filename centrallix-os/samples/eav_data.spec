$Version=2$
eav_data "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "Test EAV-Schema Data";
    key_is_rowid = yes;

    // Column specifications.
    id "filespec/column" { type=integer; id=1; }
    attr_name "filespec/column" { type=string; id=2; }
    attr_int_val "filespec/column" { type=integer; id=3; }
    attr_string_val "filespec/column" { type=string; id=4; }
    }
