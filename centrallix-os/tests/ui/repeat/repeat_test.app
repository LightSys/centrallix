// Minsik Lee 2025

$Version=2$
in_main "widget/page" 
  {
    bgcolor="#1A0180";
    width=1280; height=720;

    label "widget/label"
    {
      x=10;y=10;
      width=300;height=16;
      value="repeat test application";
      font_size=16; 
      fgcolor="white";
    }
    
    btn_rpt "widget/repeat"
    {
      sql = runserver("select :cx__rowid, :name, display=replace(:name, '.csv', '') from /tests/ui/repeat where right(:name, 4) = '.csv' order by :name");

      btn "widget/button"
      {
        x=10;y=runserver(:btn_rpt:cx__rowid * 50 + 40);
        height=30; width=90;
        type="text";
        text=runserver(:btn_rpt:display);
        bgcolor="white";
        fgcolor1="black";
        fgcolor2="grey";
        disable_color="grey";
        enabled=yes;

        btnLink "widget/connector"
        {
			    event = Click;
			    target = cmp;
			    action = "Instantiate";
			    title=runclient(runserver(:btn_rpt:name));
        }
      }

    }

    table_cp "widget/component"
    {
      path = "/tests/ui/repeat/table.cmp";
      mode = static;
      width=300; height=70;
      multiple_instantiation = yes;
    }

    cmp "widget/component"
    {
      path = "/tests/ui/repeat/repeat_test.cmp";
      mode = dynamic;
      width=798; height=476;
      multiple_instantiation = yes;
      icon = "/sys/images/ico26a.gif";

      cmp_update "widget/connector"
      {
        event = Update;
        target = label;
        action = SetValue;
        Value = runclient(:value);
      }
    }
  }
