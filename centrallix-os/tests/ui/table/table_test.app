// Minsik Lee 2025

$Version=2$
checkbox_test "widget/page"
{
  title = "Table Test Application";
  bgcolor = "#d2d3d6";
  x = 0; y = 0; height = 1200; width = 1600;
  
  osrc "widget/osrc"
  {
    sql = "SELECT :Firstname, :Lastname, :Role, :Pay, :Day  FROM /tests/ui/table/pay.csv/rows";
    baseobj = "/tests/ui/table/pay.csv/rows";
    readahead=80;
    replicasize=80;

    form "widget/form"
    {
      enter_mode = "save";

      // formstatus "widget/formstatus" { }
      
      table_pn "widget/pane"
      {
        x = 60; y = 30; width = 1240; height = 500;
        titlebar = "New"; bgcolor = "white";

        table "widget/table"
        {
          mode = static;
          x=20; y=20; width=1200; height=400;
          row1_bgcolor = "#ffffff";
          row2_bgcolor = "#e0e0e0";
          rowhighlight_bgcolor = "#000080";
          hdr_bgcolor = "#c0c0c0";
          textcolorhighlight = "#ffffff";
          textcolor = "black";
          colsep = 0;
          // data_mode = "properties";
          // cellvspacing = 6;
          // titlecolor = "blue";
          // outer_border = 13;     Not Working?
          // dragcols = 0;            Not working?
          // gridinemptyrows = 0;     Not Working?
          // inner_border = 10;       Not Working?
          // inner_padding = 10;
          // show_selection = no;
          rowheight = 20;
          windowsize = 17;

          // row_detail "widget/table-row-detail"
          // {
          //   height=68; width=340;

          //   row_form "widget/form"
          //   {
          //     row_vbox "widget/vbox"
          //     {
          //       x = 1100; y = 30; height = 50; width = 300;
                
          //       row_irstname_txt "widget/editbox"
          //       {
          //         x = 20; y = 40; width = 200; height = 30;
          //         maxchars = 30;
          //         fieldname = "Firstname";
          //         bgcolor = "#ffffff";
          //         form = row_form;
          //       }
          //     }
          //   }
          // }

          col_rpt "widget/repeat"
          {
            sql = runserver("SELECT :name FROM /tests/ui/table/pay.csv/columns");

            dynamic_col "widget/table-column"
            {
              fieldname = runserver(:col_rpt:name);
              title = runserver(:col_rpt:name);
              width = 100;
            }
          }

          click_cn "widget/connector" { event = Click; target = lbl; action = SetValue; Value = runclient(:osrc:Firstname); }
          doubleclick_cn "widget/connector" { event = DblClick; target = lbl; action = SetValue; Value = runclient(:osrc:Lastname); }
        }

        add_btn "widget/textbutton"
        {
          x = 1130; y = 450; width = 80; height = 30;
          bgcolor = "gray";
          text = "Add";
          enabled = runclient(:form:is_newable);

          add_cn "widget/connector" { target=form; action=New; event=Click; }
        }
      }

      window "widget/pane"
      {
        x = 60; y = 560; width = 630; height = 280;
        titlebar = "New"; bgcolor = "white";

        firstname_lbl "widget/label"
        {
          x = 20; y = 20; width = 120; height = 30;
          text = "First Name";
        }

        firstname_txt "widget/editbox"
        {
          x = 20; y = 40; width = 200; height = 30;
          maxchars = 30;
          fieldname = "Firstname";
          bgcolor = "#ffffff";
          form = form;
        }

        lastname_lbl "widget/label"
        {
          x = 250; y = 20; width = 120; height = 30;
          text = "Last Name";
        }

        lastname_txt "widget/editbox"
        {
          x = 250; y = 40; width = 200; height = 30;
          maxchars = 30;
          bgcolor = "#ffffff";
          fieldname = "Lastname";
        }

        role_lbl "widget/label"
        {
          x = 480; y = 20; width = 120; height = 30;
          text = "Role";
        }

        role_dd "widget/dropdown"
        {
          x = 480; y = 40; width = 120; height = 30;
          hilight="#b5b5b5";
          bgcolor="white";
          fieldname = "Role";

          role_ddi1 "widget/dropdownitem" { label = "Project Manager"; value = "Project Manager"; }
          role_ddi2 "widget/dropdownitem" { label = "DevOps"; value = "DevOps"; }
          role_ddi3 "widget/dropdownitem" { label = "Developer"; value = "Developer"; }
          role_ddi4 "widget/dropdownitem" { label = "Designer"; value = "Designer"; }
          role_ddi5 "widget/dropdownitem" { label = "Tester"; value = "Tester"; }
        }

        pay_lbl "widget/label"
        {
          x = 20; y = 100; width = 120; height = 30;
          text = "Pay";
        }

        pay_txt "widget/editbox"
        {
          x = 20; y = 120; width = 200; height = 30;
          bgcolor = "#ffffff";
          maxchars = 60;
          fieldname = "Pay";
        }

        date_lbl "widget/label"
        {
          x = 250; y = 100; width = 120; height = 30;
          text = "Date";
        }

        datetime "widget/datetime"
        {
          x = 250; y = 120; width = 100; height = 30;
          bgcolor = "#ffffff";
          fieldname = "Date";
        }

        delete_btn "widget/textbutton"
        {
          x = 20; y = 210; width = 80; height = 30;
          bgcolor = "#ad1133"; text = "Delete";
          enabled=runclient(not :form:is_discardable);


          delete_cn "widget/connector" { target=form; action=Delete; event=Click; }
        }

        cancel_btn "widget/textbutton"
        {
        x = 430; y = 210; width = 80; height = 30;
        bgcolor = "#9c7279"; disable_color = "#404040";
        text = "Cancel";
        enabled = runclient(:form:is_discardable);

        cancel_cn "widget/connector" { target=form; action=Discard; event=Click; }
        }

        save_btn "widget/textbutton"
        {
        x = 520; y = 210; width = 80; height = 30;
        bgcolor = "#330dba";
        text = "Save";
        enabled = runclient(:form:is_savable);

        save_cn "widget/connector" { target=form; action=Save; event=Click; }
        }

      }
    }
  } 

  lbl "widget/label"
  {
    x = 60; y = 900; width = 120; height = 30;
    font_size = 24;
    value = "Table Label";
  }
}