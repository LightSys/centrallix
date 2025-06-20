// Minsik Lee 2025
$Version=2$

formstatus_test "widget/page"
{
  title = "Formstatus Test Application";
  bgcolor = "#d2d3d6";
  x = 0; y = 0; width = 1600; height = 1200; 

  // osrc "widget/osrc"
  // {
  //   replicasize=20;
	// 	readahead=20;
  //   sql = "SELECT :State, :Population FROM /tests/ui/dropdown_test.csv/rows";

  //   dropdown "widget/dropdown"
	// 	{
	// 		mode="objectsource";
	// 		fieldname="State";
	// 		x=200; y=50; width=160; height=20;
	// 		hilight="#b5b5b5";
	// 		bgcolor="#c0c0c0";
	// 	}
  // }

  form "widget/form"
  {
    sm_formstatus "widget/formstatus"
    {
      x = 0; y = 0;
      style = "small";
    }

    lg_formstatus "widget/formstatus"
    {
      x = 60; y = 0;
      style = "large";
    }

    lgft_formstatus "widget/formstatus"
    {
      x = 200; y = 0;
      style = "largeflat";
    }

    editbox "widget/editbox"
    {
      x = 30; y = 70; width = 80; height = 20;
      maxchars = 30;
      bgcolor = "#ffffff";
      fieldname = "Percentage";
    }
    save_btn "widget/textbutton"
    {
      x = 30; y = 130; width = 80; height = 30;
      bgcolor = "#330dba";
      text = "Save";
      enabled = runclient(:form:is_savable);
      save_cn "widget/connector" {
        event = "Click"; target = "form"; action = "Save";
      }
    }
    searchbtn "widget/textbutton" 
    {
      x = 30; y = 160; width = 80; height = 30;
      text = "Search";
      bgcolor = "#330dba";
      enabled = runclient(:form:is_queryable or :form:is_queryexecutable);
      cn5 "widget/connector" {
          event = "Click"; target = "form"; action = "QueryToggle";
      }
    }
  } 
}