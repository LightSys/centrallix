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
      sql = runserver("select :cx__rowid, :name, display=replace(:name, '.csv', '') from /tests/ui where right(:name, 4) = '.csv' order by :name");

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

    TabCtlOne "widget/tab"
    {
      tab_location = top;
      x = 300; y = 40; width=500; height=300;
      background = "/sys/images/slate2.gif";
      inactive_background = "/sys/images/slate2_dark.gif";

       tp_rpt "widget/repeat"
      {
        sql = runserver("select :cx__rowid, :name from /tests/ui where right(:name, 4) = '.csv' order by :name");
        
        tp "widget/tabpage" 
        { 
          title=runserver(:tp_rpt:name);
          x=10;y=0;width=300;height=300;
          
          osrc "widget/osrc"
          {
            sql = "SELECT :index, :key, :value from /tests/ui/repeat_test2.csv/rows";
            baseobj = "/tests/ui/repeat_test2.csv/rows";
            readahead=5;
            
            table "widget/table"
            {
              mode = dynamicrow;
              x=0;y=0;width=248;height=168;

              row1_bgcolor = "#ffffff";
              row2_bgcolor = "#e0e0e0";
              rowhighlight_bgcolor = "#000080";
              rowheight = 20;
              windowsize = 5;
              hdr_bgcolor = "#c0c0c0";
              textcolorhighlight = "#ffffff";
              textcolor = "black";
              gridinemptyrows = 0;

              index "widget/table-column" { title="index"; fieldname = "index"; width = 200; }
              key "widget/table-column" { title="name"; fieldname = "key"; width = 200; }
              value "widget/table-column" { title="score"; fieldname = "value"; width = 200; }
 
            }
          }
        }
      }
    }

    cmp "widget/component"
    {
      path = "/tests/ui/repeat_test.cmp";
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
