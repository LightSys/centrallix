States "application/filespec"
    {
    // General parameters.
    filetype = csv
    header_row = no
    header_has_titles = no
    annotation = "States CSV Data"
    row_annot_exp = ":full_name"
    key_is_rowid = yes

    // Column specifications.
    id "filespec/column" { type = integer id=1 format='10' }
    abbrev "filespec/column" { type = string id=2 }
    full_name "filespec/column" { type = string id=3 }
    }
