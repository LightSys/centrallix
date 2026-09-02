// Minsik Lee 2025
$Version=2$

connector_test "widget/page"
{
  width = 1200; height = 800;
  bgcolor = "#303833";
  title = "connector test app";

  padding "widget/parameter" { type=integer; default=20; }

  // WINDOW
  window_label "widget/label"
  {
    x = 30; y = 10;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Widget Test";
  }

  window_pane "widget/pane"
  {
    x = 30; y = 40;
    width = 310; height = 230;
    bgcolor = "#6f7872";
    
    window_status "widget/label"
    {
      x = runserver(:this:padding); y = runserver(:this:padding);
      width = 200;
      font_size = 18;
      value = "Loading window ...";
    }

    closeWindow "widget/textbutton"
    {
      x = 160; y = 55;
      width = 100;
      height = 30;
      fgcolor1 = "black";
      fgcolor2 = "grey";  
      bgcolor = "#FFFFFF";
      text = "Close Window";
      enabled = runclient(:childwindow:is_visible);

      close_cn "widget/connector"
      {
        event = "Click";
        target = childwindow;
        action = "Close";
      }
    }

    toggleWindow "widget/textbutton"
    {
      x = runserver(:this:padding); y = 55;
      width = 100;
      height = 30;
      fgcolor1 = "black";
      fgcolor2 = "grey";  
      bgcolor = "#FFFFFF";
      text = "Toggle Window";

      toggle_cn "widget/connector"
      {
        event = "Click";
        target = childwindow;
        action = "ToggleVisibility";
      }
    }

    childwindow "widget/childwindow"
    {
      x = 30; y = 100; width = 240; height = 140;
      bgcolor = "white";
      modal = "no";
      title = "child window";

      wn_load_cn "widget/connector"
      {
        event = Load;
        target = window_status;
        action = SetValue;
        Value = "Window loaded";
      }

      wn_open_cn "widget/connector"
      {
        event = Open;
        target = window_status;
        action = SetValue;
        Value = "Window opened";
      }

      wn_close_cn "widget/connector"
      {
        event = Close;
        target = window_status;
        action = SetValue;
        Value = "Window closed";
      }
    }
  }

  // COMPONENT
  component_label "widget/label"
  {
    x = 30; y = 290;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Component Test";
  }

  component_pane "widget/pane"
  {
    x = 30; y = 320;
    width = 310; height = 180;
    bgcolor = "#6f7872";

    component_hbox "widget/hbox"
    {
      x = 20; y = 20; spacing = 15;

      instantiate_button "widget/textbutton"
      {
        width = 100;
        height = 30;
        fgcolor1 = "black";
        fgcolor2 = "grey";  
        bgcolor = "#FFFFFF";
        text = "Instantiate";

        instantiate_cn "widget/connector"
        {
          event = "Click";
          target = component;
          action = "Instantiate";
        }
      }

      destroy_button "widget/textbutton"
      {
        width = 100;
        height = 30;
        fgcolor1 = "black";
        fgcolor2 = "grey";  
        bgcolor = "#FFFFFF";
        text = "Destroy";

        destroy_cn "widget/connector"
        {
          event = "Click";
          target = component;
          action = "Destroy";
        }
      }
    }

    component "widget/component"
    {
      width = 260; height = 160;
      mode=dynamic;
      multiple_instantiation = yes;
      path = "/tests/ui/connector/connector_test.cmp";
    }
  }

  // DATE TIME
  datetime_label "widget/label"
  {
    x = 30; y = 520;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Datetime Test";
  }

  datetime_pane "widget/pane"
  {
    x = 30; y = 550;
    width = 310; height = 90;
    bgcolor = "#6f7872";

    dtstatus_label "widget/label"
    {
      x = runserver(:this:padding); y = runserver(:this:padding); width = 200; height = 30;
      font_size = 18;
      value = "Datetime status";
    }
    dt_form "widget/form"
    {
      datetime "widget/datetime"
      {
        x = runserver(:this:padding); y = 50; width=160; height=20;
        hilight="#b5b5b5";
        bgcolor="#c0c0c0";
        fieldname="value";

        datachange_cn "widget/connector"
        {
          event = DataChange;
          target = dtstatus_label;
          action = SetValue;
          Value = "Data change";
        }

        losefocus_cn "widget/connector"
        {
          event = LoseFocus;
          target = dtstatus_label;
          action = SetValue;
          Value = "Lost focus";
        }

        getfocus_cn "widget/connector"
        {
          event = GetFocus;
          target = dtstatus_label;
          action = SetValue;
          Value = "Got focus";
        }
      }
    }
  }

  // DROP DOWN
  dropdown_label "widget/label"
  {
    x = 30; y = 650;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Dropdown Test";
  }

  dropdown_pane "widget/pane"
  {
    x = 30; y = 680;
    width = 310; height = 100;
    bgcolor = "#6f7872";

    ddstatus_label "widget/label"
    {
      x = runserver(:this:padding); y = runserver(:this:padding); width = 200; height = 30;
      font_size = 18;
      value = "Dropdown status";
    }
    dd_osrc "widget/osrc"
    {
      sql = runserver("SELECT :Label, :Value, :Selected, :Grp, :Hidden FROM /tests/ui/connector/connector_test.csv/rows");
    
      dd_form "widget/form"
      {
        dropdown "widget/dropdown"
        {
          x = runserver(:this:padding); y = 50; width=160; height=20;
          hilight="#b5b5b5"; bgcolor="#c0c0c0";
          field="Label";

          dditem "widget/dropdownitem"
          {
            label="test";
            value="0";
          }

          // Only triggered on keyboard enter
          dd_datachange_cn "widget/connector"
          {
            event = DataChange;
            target = ddstatus_label;
            action = SetValue;
            Value = "Data change";
          }

          dd_losefocus_cn "widget/connector"
          {
            event = LoseFocus;
            target = ddstatus_label;
            action = SetValue;
            Value = "Lost focus";
          }

          dd_getfocus_cn "widget/connector"
          {
            event = GetFocus;
            target = ddstatus_label;
            action = SetValue;
            Value = "Got focus";
          }
        }

        setitems_btn "widget/textbutton"
        {
          x = 190; y = 50; width = 80; height = 30;
          text = "Set Item";

          setitems_cn "widget/connector" 
          {
            event = "Click"; 
            target = dropdown; 
            action = SetItems; 
            SQL = runserver("SELECT label = :Label, value = :Value, selected = :Selected, grp = :Grp, hidden = :Hidden FROM /tests/ui/connector/connector_test.csv/rows");
          }
        }

        setgroup_btn "widget/textbutton"
        {
          x = 190; y = 30; width = 80; height = 30;
          text = "Set Group";

          setgroup_cn "widget/connector" 
          {
            event = "Click"; 
            target = dropdown; 
            action = SetGroup; 
            Group = 2;
          }
        }
      }
    }
  }

  // EDITBOX
  editbox_label "widget/label"
  {
    x = 370; y = 10;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Editbox Test";
  }

  editbox_pane "widget/pane"
  {
    x = 370; y = 40;
    width = 160; height = 100;
    bgcolor = "#6f7872";

    editbox_status_label "widget/label"
    {
      x = runserver(:this:padding); y = runserver(:this:padding); width = 140; height = 30;
      font_size = 18;
      value = "Editbox status";
    }
    eb_form "widget/form"
    {
      editbox "widget/editbox"
      {
        x = runserver(:this:padding); y = 50; width=120; height=20;
        bgcolor="#c0c0c0"; empty_description="Enter here...";

        // Is not triggered when typing into the editbox, but is triggered by SetValue action
        eb_datachange_cn "widget/connector"
        {
          event = DataChange;
          target = editbox_status_label;
          action = SetValue;
          Value = "Data change";
        }

        eb_losefocus_cn "widget/connector"
        {
          event = LoseFocus;
          target = editbox_status_label;
          action = SetValue;
          Value = "Lost focus";
        }

        eb_getfocus_cn "widget/connector"
        {
          event = GetFocus;
          target = editbox_status_label;
          action = SetValue;
          Value = "Got focus";
        }
      }

      setvalue_btn "widget/textbutton"
      {
        x = 10; y = 70; width = 60; height = 30;
        text = "Set Value";

        setvalue_cn "widget/connector" 
        {
          event = "Click"; 
          target = editbox; 
          action = SetValue; 
          Value = "New Value";
        }
      }

      setvaluedescription_btn "widget/textbutton"
      {
        x = 80; y = 70; width = 60; height = 30;
        text = "Set Desc";

        setdesc_cn "widget/connector" 
        {
          event = "Click"; 
          target = editbox; 
          action = SetValueDescription; 
          Description = "New Descrip";
        }
      }
    }
  }

  // FORM
  form_label "widget/label"
  {
    x = 370; y = 150;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Form Test";
  }

  form_pane "widget/pane"
  {
    x = 370; y = 180;
    width = 160; height = 130;
    bgcolor = "#6f7872";

    form_osrc "widget/osrc"
    {
      sql="SELECT :Label, :Value, :Selected, :Grp From /tests/ui/connector/connector_test.csv/rows";
      baseobj="/tests/ui/connector/connector_test.csv/rows";

      form_form "widget/form"
      {
        form_selected_editbox "widget/editbox"
        {
          x = runserver(:this:padding); y = 0; width = 50; height = 10;
          fieldname="Selected";
          bgcolor="#c0c0c0"; empty_description="selected";
        }

        form_group_editbox "widget/editbox"
        {
          x = 80; y = 0; width = 50; height = 10;
          fieldname="Grp";
          bgcolor="#c0c0c0"; empty_description="Group";
        }

        form_label_editbox "widget/editbox"
        {
          x = runserver(:this:padding); y = 20; width = 120; height = 20;
          fieldname="Label";
          bgcolor="#c0c0c0"; empty_description="Enter here...";
        }

        form_value_editbox "widget/editbox"
        {
          x = runserver(:this:padding); y = 60; width=120; height=20;
          fieldname="Value";
          bgcolor="#c0c0c0"; empty_description="Enter here...";
        }

        form_firstbtn "widget/imagebutton"
        {
          x = 10; y = 85; width = 20; height = 20;
          image = "/sys/images/ico16ab.gif";

          form_first_cn "widget/connector" { event=Click; action=First; target=form_form; }
        }

        form_prevbtn "widget/imagebutton"
        {
          x = 30; y = 85; width = 20; height = 20;
          image = "/sys/images/ico16bb.gif";

          form_prev_cn "widget/connector" { event=Click; action=Prev; target=form_form; }
        }

        form_nextbtn "widget/imagebutton"
        {
          x = 50; y = 85; width = 20; height = 20;
          image = "/sys/images/ico16cb.gif";

          form_next_cn "widget/connector" { event=Click; action=Next; target=form_form; }
        }

        form_lastbtn "widget/imagebutton"
        {
          x = 70; y = 85; width = 20; height = 20;
          image = "/sys/images/ico16db.gif";

          form_last_cn "widget/connector" { event=Click; action=Last; target=form_form; }
        }

        form_searchbtn "widget/textbutton"
        {
          x = 90; y = 85; width = 40; height = 20;
          text = "Search";

          form_search_cn "widget/connector" { event=Click; action=QueryToggle; target=form_form; }
        }

        form_newbtn "widget/textbutton"
        {
          x = 10; y = 110; width = 20; height = 20;
          text = "New";

          form_new_cn "widget/connector" { event=Click; action=New; target=form_form; }
        }

        form_clearbtn "widget/textbutton"
        {
          x = 40; y = 110; width = 20; height = 20;
          text = "Clear";

          form_clear_cn "widget/connector" { event=Click; action=Clear; target=form_form; }
        }

        form_cnclbtn "widget/textbutton"
        {
          x = 40; y = 110; width = 20; height = 20;
          text = "Discard";

          form_cancl_cn "widget/connector" { event=Click; action=Discard; target=form_form; }
        }

        form_editbtn "widget/textbutton"
        {
          x = 90; y = 110; width = 20; height = 20;
          text = "Edit";

          form_edit_cn "widget/connector" { event=Click; action=Edit; target=form_form; }
        }

        form_savebtn "widget/textbutton"
        {
          x = 120; y = 110; width = 20; height = 20;
          text = "Save";

          form_save_cn "widget/connector" { event=Click; action=Save; target=form_form; }
        }
      }
    }
  }

  // HTML
  html_label "widget/label"
  {
    x = 370; y = 320;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "HTML Test";
  }

  html_pane "widget/pane"
  {
    x = 370; y = 350;
    width = 160; height = 130;
    bgcolor = "#6f7872";

    html_html "widget/html"
    {
      x = 5; y = 5; width = 50; height = 50;
      mode = "dynamic";
      source = "/tests/ui/connector/index.html";
    }

    html_loadpage "widget/textbutton"
    {
      x = 5; y = 60; width = 20; height = 20;
      text = "Load_pixelate";

      html_loadp_cn "widget/connector" { event=Click; action=LoadPage; target=html_html; Source=runserver("/tests/ui/connector/home.html"); }
      // Transition = runclient('rlwipe'); Not working
    }

  }

  // TEXT AREA
  textarea_label "widget/label"
  {
    x = 370; y = 430;
    width = 200; height = 30;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Textarea Test";
  }

  textarea_pane "widget/pane"
  {
    x = 370; y = 460;
    width = 160; height = 60;
    bgcolor = "#6f7872";

    textarea_status_label "widget/label"
    {
      x = 10; y = 10; width = 150; height = 30;
      font_size = 18;
      value = "Textarea status";
    }

    textarea_textarea "widget/textarea"
    {
      x = 10; y = 30; width=100; height=20;
      bgcolor="#c0c0c0";

      // Not triggered
      ta_datachange_cn "widget/connector"
      {
        event = DataChange;
        target = textarea_status_label;
        action = SetValue;
        Value = "Data change";
      }

      ta_losefocus_cn "widget/connector"
      {
        event = LoseFocus;
        target = textarea_status_label;
        action = SetValue;
        Value = "Lost focus";
      }

      ta_getfocus_cn "widget/connector"
      {
        event = GetFocus;
        target = textarea_status_label;
        action = SetValue;
        Value = "Got focus";
      }
    }
  }

  // TEXT BUTTON
  textbutton_label "widget/label"
  {
    x = 370; y = 530;
    width = 200; height = 50;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "Textbutton Test";
  }

  textbutton_pane "widget/pane"
  {
    x = 370; y = 560;
    width = 160; height = 70;
    bgcolor = "#6f7872";

    textbutton_status_label "widget/label"
    {
      x = 10; y = 10; width = 150; height = 30;
      font_size = 18;
      value = "Textbutton status";
    }

    textbutton_textbutton "widget/textbutton"
    {
      x = 10; y = 40; width = 60; height = 20;
      text = "TxTButton";

      click_cn "widget/connector" { event=Click; action=SetValue; target=textbutton_status_label; Value=runclient("clicked"); }
    }

    textbutton_setTextButton "widget/textbutton"
    {
      x = 80; y = 40; width = 50; height = 20;
      text = "Set Text";

      settext_cn "widget/connector" { event=Click; action=SetText; target=textbutton_textbutton; Text=runclient("new text"); }
    }
  }

  // TREE VIEW
  treeview_label "widget/label"
  {
    x = 370; y = 630;
    width = 200; height = 40;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "treeview Test";
  }

  treeview_pane "widget/pane"
  {
    x = 370; y = 660;
    width = 160; height = 120;
    bgcolor = "#6f7872";

    treeview_status_label "widget/label"
    {
      x = 5; y = 5; width = 150; height = 30;
      font_size = 18;
      value = "Treeview status";
    }

    treeview_scrollpane "widget/scrollpane"
    {
      x = 5; y = 20; width = 150; height = 80;

      treeview_treeview "widget/treeview"
      {
        x = 5; y = 20; width = 150; height = 80;
        source = "/tests/ui";

        treeview_click_cn "widget/connector"
        {
          event=ClickItem; target=treeview_status_label; action=SetValue; Value=runclient("clicked");
        }
        
        // Opens browser right click menu
        treeview_rclick_cn "widget/connector"
        {
          event=RightClickItem; target=treeview_status_label; action=SetValue; Value=runclient("R click");
        }
      }
    }
  }

  // VARIABLE
  variable_label "widget/label"
  {
    x = 560; y = 10;
    width = 200; height = 40;
    font_size = 24; fgcolor = "white"; style = bold;
    value = "variable Test";
  }

  variable_pane "widget/pane"
  {
    x = 560; y = 40;
    width = 140; height = 80;
    bgcolor = "#6f7872";

    variable_variable "widget/variable"
    {
      type=string; value="inital value";
    }

    variable_status_label "widget/label"
    {
      x = 5; y = 5; width = 150; height = 30;
      font_size = 18;
      value = runclient(:variable_variable:value);
    }

    variable_setvaluebtn "widget/textbutton"
    {
      x = 5; y = 30; width = 50; height = 20;
      text = "Set Var";

      setvarvalue_cn "widget/connector" { event=Click; action=SetValue; target=variable_variable; Value=runclient("new val"); }
    }

  }
  //obsrc?, variable
}