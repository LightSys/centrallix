$Version=2$
autoscale_bands "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "Autoscale harness -- distinct band names";
    row_annot_exp = ":band";
    key_is_rowid = yes;

    // Column specifications.
    band "filespec/column" { type=string; id=1; }
    }
