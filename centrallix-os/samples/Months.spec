Months "application/filespec"
    {
    // General parameters.
    filetype = csv
    header_row = no
    header_has_titles = no
    annotation = "Months CSV Data"
    row_annot_exp = ":full_name"
    key_is_rowid = yes

    // Column specifications.
    id "filespec/column" { type = integer id=1 format='10' }
    short_name "filespec/column" { type = string id=2 }
    full_name "filespec/column" { type = string id=3 }
    num_days "filespec/column" { type = integer id=4 format='10' }
    num_leapyear_days "filespec/column" { type= integer id=5 format='10' }
    }
