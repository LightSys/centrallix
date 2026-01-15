$Version=2$
pay "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "Connector Data";
    row_annot_exp = ":Label";
    key_is_rowid = yes;

    // Column specifications.
    // Label,Value,Selected,Grp,Hidden
    Label "filespec/column" { type=string; id=1; }
    Value "filespec/column" { type=integer; id=2; }
    Selected "filespec/column" { type=integer; id=3; }
    Grp "filespec/column" { type=integer; id=4; }
    }