// Minsik Lee 2025

$Version=2$
rule_test "widget/page"
{
  title = "Rule Test Application";
  bgcolor = "#d2d3d6";
  x = 0; y = 0; width = 1600; height = 1200; 

  project_osrc "widget/osrc"
  {
    sql = "SELECT :Name, :Start, :End, :CurrentWeek, :Owner  FROM /tests/ui/rule/project.csv/rows";
    baseobj = "/tests/ui/rule/project.csv/rows";
    readahead=10;
    replicasize=10;

    project_pn "widget/pane"
    {
      x = 40; y = 30; width = 820; height = 300; bgcolor = "white";

      project_lbl "widget/label"
      {
        x = 20; y = 20; width = 120; height = 30; font_size = 24; style = "bold";
        text = "Project";
      }

      project_table "widget/table"
      {
        x=20; y=60; width=770; height=200;
        row1_bgcolor = "#ffffff";
        row2_bgcolor = "#e0e0e0";
        rowhighlight_bgcolor = "#000080";
        hdr_bgcolor = "#c0c0c0";
        textcolorhighlight = "#ffffff";
        textcolor = "black";
        colsep = 0;
        rowheight = 26;
        windowsize = 10;

        Name "widget/table-column" { fieldname = "Name"; title = "Name"; width = 100; }
        Start "widget/table-column" { fieldname = "Start"; title = "Start Date"; width = 100;}
        End "widget/table-column" { fieldname = "End"; title = "End Date"; width = 100;}
        Currentweek "widget/table-column" { fieldname = "CurrentWeek"; title = "Current Week"; width = 100;}
        Owner "widget/table-column" { fieldname = "Owner"; title = "Project Owner"; width = 100;}

      }
    }
  }

  count_osrc "widget/osrc"
{
  sql = "SELECT :NextTaskID FROM /tests/ui/rule/taskcount.csv/rows";
  baseobj = "/tests/ui/rule/taskcount.csv/rows";
  replicasize = 1;
  readahead = 1;
}

  task_osrc "widget/osrc"
  {
    sql = "SELECT :TaskID, :Project, :Name, :Description, :Hours, :Percentage  FROM /tests/ui/rule/task.csv/rows";
    baseobj = "/tests/ui/rule/task.csv/rows";
    readahead=20;
    replicasize=20;

    sync_rule "widget/rule"
    {
      ruletype = osrc_relationship;
      target = project_osrc;
      key_1 = "Project";
      target_key_1 = "Name";
      // autoquery = true;
      master_norecs_action=norecs;
      master_null_action=norecs;
    }

    task_id_rule "widget/rule"
    {
      ruletype = "osrc_key";
      keying_method = "counterosrc";
      key_fieldname = "TaskID";
      osrc = count_osrc;
      counter_attribute = "NextTaskID";
    }

    task_pn "widget/pane"
    {
      x = 40; y = 430; width = 820; height = 580; bgcolor = "white";

      task_lbl "widget/label"
      {
        x = 20; y = 30; width = 120; height = 30; font_size = 24; style = "bold";
        value = "Task";
      }

      add_btn "widget/textbutton"
      {
        x = 720; y = 30; width = 80; height = 30;
        bgcolor = "gray";
        text = "+ New Task";
        enabled = runclient(:form:is_newable);

        add_cn "widget/connector" { target=form; action=New; event=Click; }
      }

      task_table "widget/table"
      {
        mode = static;
        x=20; y=70; width=780; height=500;
        row1_bgcolor = "#ffffff";
        row2_bgcolor = "#e0e0e0";
        rowhighlight_bgcolor = "#000080";
        hdr_bgcolor = "#c0c0c0";
        textcolorhighlight = "#ffffff";
        textcolor = "black";
        colsep = 0;
        rowheight = 26;
        windowsize = 16;

        Task "widget/table-column" { fieldname = "Name"; title = "Task"; width = 100;}
        Description "widget/table-column" { fieldname = "Description"; title = "Description"; width = 100;}
        Hours "widget/table-column" { fieldname = "Hours"; title = "# Hours"; width = 100;}
        Percentage "widget/table-column" { fieldname = "Percentage"; title = "% Done"; width = 100;}
      }
    }

    add_task_pn "widget/pane"
    {
      x = 890; y = 30; width = 480; height = 440; bgcolor = "#f2f2f2";

      add_lbl "widget/label"
      {
        x = 20; y = 30; width = 120; height = 30; font_size = 24; style = "bold";
        value = "Add Task";
      }

      form "widget/form"
      {
        enter_mode = "save";

        project_input_lbl "widget/label"
        {
          x = 20; y = 70; width = 120; height = 30;
          text = "Project";
        }

        taskid_editbox "widget/editbox"
        {
          x = 0; y = 0; width = 40; height = 40;
          maxchars = 30;
          bgcolor = "#ffffff";
          fieldname = "TaskID";
        }

        project_dd "widget/dropdown"
        {
          x = 80; y = 70; width = 140; height = 20;
          hilight="#b5b5b5";
          bgcolor="white";
          fieldname = "Project";
          sql = "SELECT :Name FROM /tests/ui/rule/project.csv/rows";

          project_ddi1 "widget/dropdownitem" { label = "Alpha"; value = "Alpha"; }
          project_ddi2 "widget/dropdownitem" { label = "Beta"; value = "Beta"; }
          project_ddi3 "widget/dropdownitem" { label = "Gamma"; value = "Gamma"; }
          project_ddi4 "widget/dropdownitem" { label = "Delta"; value = "Delta"; }
          project_ddi5 "widget/dropdownitem" { label = "Epsilon"; value = "Epsilon"; }

        }

        percentage_lbl "widget/label"
        {
          x = 300; y = 70; width = 120; height = 30;
          text = "% Done";
        }

        percentage_lbl_editbox "widget/editbox"
        {
          x = 360; y = 70; width = 80; height = 20;
          maxchars = 30;
          bgcolor = "#ffffff";
          fieldname = "Percentage";
        }

        task_input_lbl "widget/label"
        {
          x = 20; y = 130; width = 120; height = 30;
          text = "Task Name";
        }

        task_editbox "widget/editbox"
        {
          x = 20; y = 150; width = 290; height = 20;
          maxchars = 30;
          bgcolor = "#ffffff";
          fieldname = "Name";
        }

        hours_lbl "widget/label"
        {
          x = 360; y = 130; width = 60; height = 30;
          text = "# Hours";
        }

        hours_editbox "widget/editbox"
        {
          x = 360; y = 150; width = 80; height = 20;
          maxchars = 30;
          bgcolor = "#ffffff";
          fieldname = "Hours";
        }

        description_lbl "widget/label"
        {
          x = 20; y = 220; width = 90; height = 30;
          text = "description";
        }

        description_lbl_editbox "widget/editbox"
        {
          x = 20; y = 240; width = 420; height = 20;
          maxchars = 30;
          bgcolor = "#ffffff";
          fieldname = "Description";
        }

        // buttons
        delete_btn "widget/textbutton"
        {
          x = 20; y = 330; width = 80; height = 30;
          bgcolor = "#ad1133"; text = "Delete";
          enabled=runclient(not :form:is_discardable);

          delete_cn "widget/connector" { target=form; action=Delete; event=Click; }
        }

        cancel_btn "widget/textbutton"
        {
        x = 280; y = 330; width = 80; height = 30;
        bgcolor = "#9c7279"; disable_color = "#404040";
        text = "Cancel";
        enabled = runclient(:form:is_discardable);

        cancel_cn "widget/connector" { target=form; action=Discard; event=Click; }
        }

        save_btn "widget/textbutton"
        {
        x = 360; y = 330; width = 80; height = 30;
        bgcolor = "#330dba";
        text = "Save";
        enabled = runclient(:form:is_savable);

        save_cn "widget/connector" { target=form; action=Save; event=Click; }
        }
      }

    }

  }

  // addr_sync "widget/rule"
  //   {
  //     ruletype=osrc_relationship;
  //     target=osrc;
  //     key_objname=p;
  //     key_1=p_partner_key;
  //     target_key_1=p_partner_key;
  //     master_norecs_action=norecs;
  //     master_null_action=norecs;
  //   }

  // set_batch "widget/rule"
  // {
  //   ruletype = "osrc_key";
  //   keying_method = "counterosrc";
  //   key_fieldname = "a_batch_number";
  //   osrc = ledger_osrc;
  //   counter_attribute = "a_next_batch_number";
  // }

  // linkage "widget/rule" 
  // { 
  //   ruletype=osrc_filter; 
  //   fieldname=runserver(:this:preview_field); 
  //   value=runclient(:ctl:content); 
  //   min_chars=1; 
  //   trailing_wildcard=no; 
  //   query_delay=499;
  // }

}