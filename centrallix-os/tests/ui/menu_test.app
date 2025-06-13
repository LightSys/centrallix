// NOTE
// - Couldn't get Selected event work with menu
// - Selected event works with menu item

$Version=2$
in_main "widget/page" 
  {
    bgcolor="#9FBED7";
    width=640; height=480;
    
    mn "widget/menu" 
    {
      x=0;y=0;
      width=640;
      bgcolor="#E0EAF9";
      highlight_bgcolor="#c5d0e0";
      active_bgcolor="#c5d0e0";

      mna "widget/menu" 
      { 
        icon="/sys/images/ico02a.gif"; 
        label="File";
        bgcolor="#F2F2F2";
        highlight_bgcolor="#cfcfcf";
        active_bgcolor="#efefef";
        row_height=18;
        mnaa "widget/menuitem" 
        { 
          label="New..."; 
          value="New";
          mnaa_cn "widget/connector"
          {
            action = "Popup";
            event = "Select";
            target = child_window;
          }
        }
        mnab "widget/menuitem" { label="Open..."; }
        mnac "widget/menuitem" 
        { 
          label="Save";
          value="Saved";
        }
        mnad "widget/menu" 
        { 
          label="Recent Files"; 
          bgcolor="#F2F2F2";
          highlight_bgcolor="#cfcfcf";
          active_bgcolor="#efefef";
          row_height=18;
          file1 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="SomeFile.txt"; }
          file2 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="A_Document.sdw"; }
          file3 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="A_Presentation.sdd"; }
        }
        mna_cn "widget/connector"
        {
          action = SetValue;
          event = SelectItem;
          target = menu_label;
          Value = runclient(:Label);
        } 
      }
      mnb "widget/menu" 
      { 
        icon="/sys/images/ico01a.gif"; 
        label="Edit";
        bgcolor="#F2F2F2";
        highlight_bgcolor="#cfcfcf";
        active_bgcolor="#efefef";
        row_height=18;
        mnba "widget/menuitem" { label="Cut"; value="Cut";}
        mnbb "widget/menuitem" { label="Copy"; value="Copy";}
        mnbc "widget/menuitem" { label="Paste"; value="Paste";}

        mnb_cn "widget/connector"
        {
          action = SetValue;
          event = SelectItem;
          target = menu_label;
          Value = runclient(:Label);
        } 
      }
      mnc "widget/menuitem" 
      { 
        checked=yes; 
        label="View"; 

        mnc_cn "widget/connector"
        {
          action = "SetValue";
          event = "Select";
          target = menu_label;
          Value = runclient(:mnc:value);
        } 
      }
      mnd "widget/menuitem" 
      { 
        label="Help"; 
        onright=yes; 
        value="Help";
      }

      mn_cn "widget/connector"
      {
        action = SetValue;
        event = SelectItem;
        target = menu_label;
        Value = runclient(:Label);
      } 
    }

    mn2 "widget/menu"
	  {
      x=100;y=100;
      width=100;
      row_height=18;
      direction=vertical;
      popup=yes;
      bgcolor="#F2F2F2";
      highlight_bgcolor="#cfcfcf";
      active_bgcolor="#efefef";

      mn2a "widget/menuitem" { label="New..."; }
      mn2b "widget/menuitem" { label="Open..."; }
      mn2c "widget/menu" 
      {
        label="Recent Files"; 
        bgcolor="#F2F2F2";
        highlight_bgcolor="#cfcfcf";
        active_bgcolor="#efefef";
        row_height=18;
        file4 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="SomeFile.txt"; }
        file5 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="A_Document.sdw"; }
        file6 "widget/menuitem" { icon="/sys/images/ico01a.gif"; label="A_Presentation.sdd"; }
      }
      mn2d "widget/menuitem" { label="Help"; onright=yes; }
    }

    menu_label "widget/label"
    {
      x=30; y=50;
      width=200;
      value="menu label";
      font_size=16; 
    }

    child_window "widget/childwindow"
    {
      x = 10; y = 100;
      width = 200; height = 200;
      style = "dialog";
      bgcolor = "#c0c0c0";
      hdr_background = "/sys/images/grey_gradient2.png";
      title = "New";
      textcolor = "black";
      toplevel = "yes";
      visible = false;
      modal = yes;
      titlebar = "yes";
    }

    cn1 "widget/connector"
    {
      event="RightClick";
      target=mn2;
      action="Popup";
      X=runclient(:X);
      Y=runclient(:Y);
    }
  }

