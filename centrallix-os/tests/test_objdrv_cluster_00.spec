$Version=2$
test_objdrv_cluster_00 "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = no;
    annotation = "test_objdrv_cluster_00";
    key_is_rowid = yes;

    // Column specifications.
    key "filespec/column" { type=string; id=1; }
    data "filespec/column" { type=string; id=2; }
    }
