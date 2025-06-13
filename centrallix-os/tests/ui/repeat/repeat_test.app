// Minsik Lee 2025

$Version=2$
in_main "widget/page" 
  {
    bgcolor="#1A0180";
    width=1600; height=1200;

    menu "widget/menu" 
    {
      x=0;y=0;
      width=1600;
      bgcolor="#E0EAF9";
      highlight_bgcolor="#c5d0e0";
      active_bgcolor="#c5d0e0";

      submenu "widget/menu" 
      { 
        label="Dummy Menu"; 
        bgcolor="#F2F2F2";
        highlight_bgcolor="#cfcfcf";
        active_bgcolor="#efefef";
        row_height=18;

        mi_rpt "widget/repeat"
        {
          sql = "SELECT :name FROM /tests/ui/repeat WHERE right(:name, 4) = '.csv'";

          file1 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label=runserver(:mi_rpt:name); }
        }
      }
    }

    label "widget/label"
    {
      x=30;y=20;
      width=300;height=16;
      value="repeat test application";
      font_size=24; style=bold; 
      fgcolor="white";
    }

    table_cp "widget/component"
    {
      x=30; y=80; width=400; height=100;
      path = "/tests/ui/repeat/table.cmp";
      mode = static;
      multiple_instantiation = yes;
    }


    widget_rpt "widget/repeat"
    {
      sql = runserver("select :cx__rowid, :name, display=replace(:name, '.csv', '') from /tests/ui/repeat where right(:name, 4) = '.csv' order by :name");

        btn "widget/button"
        {
          x=30;y=runserver(:widget_rpt:cx__rowid * 40 + 450);
          height=30; width=90;
          type="text";
          text=runserver(:widget_rpt:display);
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
            title=runclient(runserver(:widget_rpt:name));
          }
        }

        checkbox "widget/checkbox"
        {
            x = 130; y=runserver(:widget_rpt:cx__rowid * 40 + 450);
            fieldname = "mycheckbox1";
        }

        clock "widget/clock"
        {
          ampm="yes";
          x = 150; y=runserver(:widget_rpt:cx__rowid * 40 + 440); width=80; height=40;
          bgcolor="white";
          shadowed="true";
          fgcolor1="#dddddd";
          fgcolor2="black";
          shadowx = 2;
          shadowy = 2;
          size=3;
        }

        dropdown "widget/dropdown"
        {
          x = 320; y=runserver(:widget_rpt:cx__rowid * 40 + 450); width=120; height=20;
          fieldname="value";
          hilight="#b5b5b5";
          bgcolor="#c0c0c0";

          dditem "widget/dropdownitem"
          {
            label = runserver(:widget_rpt:name);
            value = "0";
          }
        }

        editbox "widget/editbox" 
        {
          x = 460; y=runserver(:widget_rpt:cx__rowid * 40 + 450); 
          width=120; height=15;  
          bgcolor="#ffffff";
        }

        html "widget/html"
        {
          x = 600; y=runserver(:widget_rpt:cx__rowid * 40 + 450); 
          width = 120; height = 30;
          mode = dynamic;
          source = "/tests/ui/repeat/index.html";
        }

        image "widget/image"
        {
          x = 730; y=runserver(:widget_rpt:cx__rowid * 40 + 450); 
          source = "/sys/images/centrallix_18x18.gif";
          width = 26; height = 26;
        }

        imagebutton "widget/imagebutton"
        {
          x = 770; y=runserver(:widget_rpt:cx__rowid * 40 + 450); 
            width = 30; height = 30;
            image="/sys/images/ico16aa.gif";
            pointimage="/sys/images/ico16ab.gif";
            clickimage="/sys/images/ico16ac.gif";
            tooltip = "click me";
            repeat = "no";
        }

        pane "widget/pane"
        {
          x = 820; y=runserver(:widget_rpt:cx__rowid * 40 + 450);
          bgcolor="green"; height=30; width=30;
        }

        radiobuttonpanel "widget/radiobuttonpanel"
        {
          x = 870; y=runserver(:widget_rpt:cx__rowid * 40 + 450);
          width = 110; height = 30;
          bgcolor = "white"; 
          title = "radiopanel";

          radiobutton "widget/radiobutton" { label = runserver(:widget_rpt:name); value = "0"; }
        }

        // scrollbar "widget/scrollbar"
        // {
        //   x = 990; y=runserver(:widget_rpt:cx__rowid * 40 + 450);
        //   width = 140; height = 30; 
        //   bgcolor = "white"; direction = "vertical"; range = 30;
        // }

        scrollpane "widget/scrollpane"
        {
          x = 990; y=runserver(:widget_rpt:cx__rowid * 40 + 450);
          width = 50; height = 60;
          bgcolor = "white";
        }

        textarea "widget/textarea"
        {
          x = 1050; y=runserver(:widget_rpt:cx__rowid * 40 + 450);
          width = 50; height = 30;
          bgcolor = "white";
        }

        textbutton "widget/textbutton"
        {
          x = 1110; y=runserver(:widget_rpt:cx__rowid * 40 + 450);
          width = 30; height = 30;
          text="button";
        }

        mypane "widget/pane"
        {
        x=1180; y=runserver(:widget_rpt:cx__rowid * 40 + 450); width=120; height=30;
        style = "lowered";
        bgcolor = "#c0c0c0";
        myscroll "widget/scrollpane"
          {
          x=0; y=0; width=120; height=30;
          mytreeview "widget/treeview"
            {
            x=1; y=1; width=120;
            source="/tests/ui";
            }
          }
        }
    }

    cmp "widget/component"
    {
      path = "/tests/ui/repeat/repeat_test.cmp";
      mode = dynamic;
      x = 100; y = 100; width=300; height=220;
      multiple_instantiation = no;
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
