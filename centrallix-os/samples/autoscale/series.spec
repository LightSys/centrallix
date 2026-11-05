$Version=2$
autoscale_series "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "Autoscale harness -- chart series data";
    row_annot_exp = ":label";
    key_is_rowid = yes;

    // Three series with shapes that are obvious at a glance: a straight rise,
    // a straight fall, and a triangular bump.  A chart that mis-plots after a
    // resize stops looking like them.
    label   "filespec/column" { type=string;  id=1; }
    rising  "filespec/column" { type=integer; id=2; }
    falling "filespec/column" { type=integer; id=3; }
    bump    "filespec/column" { type=integer; id=4; }
    }
