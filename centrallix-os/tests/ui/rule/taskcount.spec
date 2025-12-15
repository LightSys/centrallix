$Version=2$
pay "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = no;
    header_has_titles = no;
    annotation = "Task Count";
    row_annot_exp = ":NextTaskID";
    key_is_rowid = no;

    // Column specifications.
    NextTaskID "filespec/column" { type=integer; id=1; }
    }