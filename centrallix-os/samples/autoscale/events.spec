$Version=2$
autoscale_events "application/filespec"
    {
    // General parameters.
    filetype = csv;
    header_row = yes;
    header_has_titles = yes;
    annotation = "Autoscale harness -- calendar events";
    row_annot_exp = ":title";
    key_is_rowid = yes;

    // 'offset' is a number of days relative to today rather than a real
    // date.  A checked-in file cannot know when it will be opened, and a
    // calendar showing the current month would be empty if these were
    // hard-coded dates, so the osrc turns the offset into a date with
    // dateadd('day', :offset, getdate()).
    offset "filespec/column" { type=integer; id=1; }
    title  "filespec/column" { type=string;  id=2; }
    descr  "filespec/column" { type=string;  id=3; }
    prio   "filespec/column" { type=integer; id=4; }
    }
