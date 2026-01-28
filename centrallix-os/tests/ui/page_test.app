// Minsik Lee
$Version=2$

page_test "widget/page"
{
  width=1200; height=800;
  title="page test app";
  background="/sys/images/slate2.gif";
  datafocus1="purple"; datafocus2="red"; mousefocus1="blue"; mousefocus2="yellow";
  textcolor="oragne";

  vbox "widget/vbox"
  {
    x=30; y=30; height=500;
    spacing=10; column_width = 400;

    clock "widget/clock"
    {
      ampm="yes";
      width=60; height=40;
      bgcolor="white";
      shadowed="true";
      fgcolor1="#dddddd";
      fgcolor2="black";
      shadowx = 2;
      shadowy = 2;
      size=3;
    }

    button "widget/button"
    {
      height=20; width=60;
      type="text";
      text="text button";
      bgcolor="blue";
      disable_color="grey";
    }

    label "widget/label"
    {
      width = 60;
      align = "center";
      font_size = 24;
      style = "bold";
      text = "Label";
    }

    osrc "widget/osrc"
    {
      sql = "SELECT :Date,:Event,:Description FROM /tests/ui/page_test.app/rows";

      calendar "widget/calendar"
      {
        height=100;width=100; bgcolor="white";
        displaymode="month";
        eventdatefield="Date"; eventnamefield="Event"; evetndescfield="Description";
      }
    }

    checkbox "widget/checkbox"
    {
      fieldname="checkbox"; height=30; width=30;
    }

    childwindow "widget/childwindow"
    {
      bgcolor="white"; height=100; width=200; style="window"; title="child window";
    }

    // component "widget/component"
    // {
    //   path = "/tests/ui/repeat/table.cmp";
    //   mode = static;
    //   width=100; height=50;
    //   multiple_instantiation = yes;
    // }

    datetime "widget/datetime"
    {
      width=160;height=30;
      bgcolor="white";
    }

    dropdown "widget/dropdown"
		{
			width=160; height=30;
			hilight="#b5b5b5";
			bgcolor="#c0c0c0";

			dd1item1 "widget/dropdownitem"
			{
				label="Male";
				value="0";
			}
			dd1item2 "widget/dropdownitem"
			{
				label="Female";
				value="1";
			}
		}

    editbox "widget/editbox"
    {
      bgcolor="white"; empty_description="this is an editbox";
      width=160; height=60; maxchars=300;
    }

    html "widget/html"
    {
      mode="dynamic";
      width=478; height=200;
      source="/tests/ui/index.html"; 
    }

    image "widget/image"
    {
      height=66; width=374; 
      source="/sys/images/centrallix_374x66.png";
    }

    imagebutton "widget/imagebutton"
    {
      width=18;
      height=18;
      image="/sys/images/ico16da.gif";
      pointimage="/sys/images/ico16db.gif";
      clickimage="/sys/images/ico16dc.gif";
      disabledimage="/sys/images/ico16dd.gif";  
    }

    pane "widget/pane"
    {
        width=30;  height=30;
        style="lowered";
        bgcolor="#c0c0c0";
    }

    radiobutton "widget/radiobuttonpanel"
    {
      width = 250; height = 200;
      title = "Time complexity of Selection Sort";
      bgcolor = "#ffffff";
      querymode = "false";

      rb1 "widget/radiobutton" { label = "O(n log n)"; selected = "true"; }
      rb2 "widget/radiobutton" { label = "O(n)"; }
      rb3 "widget/radiobutton" { label = "O(n^2)"; }
      rb4 "widget/radiobutton" { label = "O(log n)"; }
      rb5 "widget/radiobutton" { label = "O(1)"; }
      rb6 "widget/radiobutton" { label = "O(n^3)"; }
    }

    MyScrollPane "widget/scrollpane"
    {
      width=160; height=60;
      bgcolor="white";
    }

    tab "widget/tab"
    {
      tab_location=bottom;
      width=160; height=60; 
      bgcolor="white";

      tp1 "widget/tabpage" { }
      tp2 "widget/tabpage" { }
    }

  }
}