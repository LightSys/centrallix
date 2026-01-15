// David Hopkins June 2025
// Code adapted from /samples/basicform2.app with modifications
// See Documentation for details, status, ongoing work, and bugs

$Version=2$
basicform2 "widget/page" {
    title = "David's User and Computer Management";
    bgcolor = "#121212";
    textcolor = "#E0E0E0";
    x = 0; y = 0; width = 700; height = 480;


    osrc1 "widget/osrc" {
        sql = "SELECT :first_name, :last_name, :email from /tests/ui/form/people2.csv/rows";
        baseobj = "/tests/ui/form/people2.csv/rows";
        readahead = 1;
        replicasize = 8;

        peoplePane "widget/pane" {
            x = 15; y = 15; width = 260; height = 450;
            style = flat; bgcolor = "#1E1E1E"; border_color = "#2d2d2d";

            peopleLabel "widget/label" {
                x = 15; y = 15; width = 100; height = 20;
                text = "People";
                font_size = 18;
            }

            searchbtn "widget/textbutton" {
                x = 175; y = 12; height = 28; width = 70;
                text = "Search";
                bgcolor = "#424242";
                textcolor = "#E0E0E0";
                border_color = "#616161";
                enabled = runclient(:form1:is_queryable or :form1:is_queryexecutable);
                cn5 "widget/connector" {
                    event = "Click"; target = "form1"; action = "QueryToggle";
                }
            }

            myTable "widget/table" {
                mode = dynamicrow;
                x = 10; y = 50; width = 240; height = 390;
                row1_bgcolor = "#1E1E1E";
                row2_bgcolor = "#252525";
                rowhighlight_bgcolor = "#FFB74D";
                rowheight = 24;
                windowsize = 15;
                hdr_bgcolor = "#252525";
                textcolorhighlight = "#121212";
                textcolor = "#E0E0E0";
                gridinemptyrows = 0;

                first_name2 "widget/table-column" {
                    fieldname = "first_name";
                    title = "First Name";
                    width = 110;
                }
                last_name2 "widget/table-column" {
                    fieldname = "last_name";
                    title = "Last Name";
                    width = 120;
                }

                email2 "widget/table-column" {
                    fieldname = "email";
                    title = "Email";
                    width = 150;
                }
            }
        }

        form1 "widget/form" {

            detailsPane "widget/pane" {
                x = 290; y = 15; width = 395; height = 240;
                style = flat; bgcolor = "#1E1E1E";
                border_width = 1; border_color = "#2d2d2d";

                detailsLabel "widget/label" {
                    x = 15; y = 15; width = 200; height = 20;
                    text = "Person Details";
                    font_size = 18;
                    font_weight = bold;
                }

                first_name_label "widget/label" {
                    x = 15; y = 60; width = 85; height = 25;
                    text = "First Name:";
                    align = right;
                }
                first_name "widget/editbox" {
                    style = flat; border_width = 1; border_color = "#424242";
                    bgcolor = "#ffffff"; textcolor = "#121212";
                    x = 110; y = 60; width = 270; height = 25;
                    fieldname = "first_name";
                }

                last_name_label "widget/label" {
                    x = 15; y = 95; width = 85; height = 25;
                    text = "Last Name:";
                    align = right;
                }
                last_name "widget/editbox" {
                    style = flat; border_width = 1; border_color = "#424242";
                    bgcolor = "#ffffff"; textcolor = "#121212";
                    x = 110; y = 95; width = 270; height = 25;
                    fieldname = "last_name";
                }

                email_label "widget/label" {
                    x = 15; y = 130; width = 85; height = 25;
                    text = "Email:";
                    align = right;
                }
                email "widget/editbox" {
                    style = flat; border_width = 1; border_color = "#424242";
                    bgcolor = "#ffffff"; textcolor = "#121212";
                    x = 110; y = 130; width = 270; height = 25;
                    fieldname = "email";
                }

                // --- Action bar ---
                newbtn "widget/textbutton" {
                    x = 110; y = 200; height = 30; width = 65;
                    text = "New";
                    bgcolor = "#424242";
                    textcolor = "#E0E0E0";
                    border_width = 1; border_color = "#616161";
                    enabled = runclient(:form1:is_newable);
                    cn6 "widget/connector" {
                        event = "Click"; target = "form1"; action = "New";
                    }
                }
                editbtn "widget/textbutton" {
                    x = 180; y = 200; height = 30; width = 65;
                    text = "Edit";
                    bgcolor = "#424242";
                    textcolor = "#E0E0E0";
                    border_width = 1; border_color = "#616161";
                    enabled = runclient(:form1:is_editable);
                    cn7 "widget/connector" {
                        event = "Click"; target = "form1"; action = "Edit";
                    }
                }
                savebtn "widget/textbutton" {
                    x = 250; y = 200; height = 30; width = 65;
                    text = "Save";
                    bgcolor = "#FFB74D";
                    textcolor = "#121212";
                    font_weight = bold;
                    border_width = 0;
                    enabled = runclient(:form1:is_savable);
                    cn8 "widget/connector" {
                        event = "Click"; target = "form1"; action = "Save";
                    }
                    cn8_refresh "widget/connector" {
                        event = "Click"; target = "osrc1"; action = "Refresh";
                    }
                }
                cancelbtn "widget/textbutton" {
                    x = 320; y = 200; height = 30; width = 65;
                    text = "Cancel";
                    bgcolor = "#424242";
                    textcolor = "#E0E0E0";
                    border_width = 1; border_color = "#616161";
                    enabled = runclient(:form1:is_discardable);
                    cn9 "widget/connector" {
                        event = "Click"; target = "form1"; action = "Discard";
                    }
                }
                deletebtn "widget/textbutton" {
                    x = 40; y = 200; height = 30; width = 65;
                    text = "Delete";
                    bgcolor = "#424242";
                    textcolor = "#F97583";
                    border_width = 1; border_color = "#F97583";
                    enabled = runclient(:form1:is_editable);
                    cn_delete "widget/connector" {
                        event = "Click"; target = "form1"; action = "Delete";
                    }
                    cn_delete_refresh "widget/connector" {
                        event = "Click"; target = "osrc1"; action = "Refresh";
                    }
                }
            }
        }
    }

    osrc2 "widget/osrc" {
        sql = "SELECT :first_name, :computer_name, :memory from /tests/ui/form/computers2.csv/rows";
        baseobj = "/tests/ui/form/computers2.csv/rows";
        readahead = 1;
        replicasize = 8;
        autoquery = never;

        // probably not doing anything 
        relationship "widget/rule" {
            ruletype = osrc_relationship;
            target = osrc1;
            key_1 = "first_name";
            target_key_1 = "first_name";
            autoquery = true;
        }

        computersPane "widget/pane" {
            x = 290; y = 265; width = 395; height = 200;
            style = flat; bgcolor = "#1E1E1E";
            border_width = 1; border_color = "#2d2d2d";

            computersLabel "widget/label" {
                x = 15; y = 15; width = 250; height = 20;
                text = "Associated Computers";
                font_size = 18;
                font_weight = bold;
            }

            computerTable "widget/table" {
                mode = dynamicrow;
                x = 10; y = 45; width = 375; height = 145;
                row1_bgcolor = "#1E1E1E";
                row2_bgcolor = "#252525";
                rowhighlight_bgcolor = "#FFB74D";
                rowheight = 24;
                windowsize = 8;
                hdr_bgcolor = "#252525";
                textcolorhighlight = "#121212";
                textcolor = "#E0E0E0";
                gridinemptyrows = 1;

                first_name3 "widget/table-column" {
                    fieldname = "first_name";
                    title = "First Name";
                    width = 120;
                }
                computer_name "widget/table-column" {
                    fieldname = "computer_name";
                    title = "Computer Name";
                    width = 150;
                }
                memory "widget/table-column" {
                    fieldname = "memory";
                    title = "MB Memory";
                    width = 85;
                }
            }
        }
    }
}
