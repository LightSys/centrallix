$Version=2$
autoscale_rows "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "Autoscale harness -- table/treeview test rows";
    key_is_rowid = yes;

    // Deliberately long, so treeview labels truncate and can be checked
    // against the tooltip and the widget's right edge.
    row_annot_exp = ":tag + ' ' + :filler";

    // Column specifications.
    rid    "filespec/column" { type=integer; id=1; }
    tag    "filespec/column" { type=string;  id=2; }
    band   "filespec/column" { type=string;  id=3; }
    level  "filespec/column" { type=integer; id=4; }
    amount "filespec/column" { type=double;  id=5; }
    ruler  "filespec/column" { type=string;  id=6; }
    filler "filespec/column" { type=string;  id=7; }
    }
