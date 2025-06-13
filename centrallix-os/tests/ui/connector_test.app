// Minsik Lee 2025
$Version=2$

page_test "widget/page"
{
  width = 1200; height = 800;
  bgcolor = "#303833";
  title = "connector test app";

  padding "widget/parameter" { type=integer; default=20; }

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

      load_cn "widget/connector"
      {
        event = "Load";
        target = window_label;
        action = SetValue;
        Value = "Window loaded";
      }
    }
  }

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
      path = "/tests/ui/connector_test.cmp";
    }
  }

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
    width = 310; height = 180;
    bgcolor = "#6f7872";

    dtstatus_label "widget/label"
    {
      x = runserver(:this:padding); y = runserver(:this:padding); width = 200; height = 30;
      font_size = 18;
      value = "Datetime status";
    }

    datetime "widget/datetime"
    {
      x = runserver(:this:padding); y = 50; width=160; height=40;
      bgcolor = "#ffffff";
      fgcolor = "#000000";
      initialdate = "01 Jan 2023";
      default_time = "00:00:00";

      // click_cn "widget/connector"
      // {
      //   event = DataChange;
      //   target = dtstatus_label;
      //   action = SetValue;
      //   Value = "changed";
      // }
    }
  }
}