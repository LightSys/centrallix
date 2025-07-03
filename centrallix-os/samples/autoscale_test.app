$Version=2$
MyPage "widget/page"
    {
    title = "Responsive Testing Page";
    bgcolor = "black";
    textcolor = "#00f8ff";
    width = 1000;
    height = 1000;

    auto "widget/hbox"
	{
	x=100; y=50; width=900; height=750;
	spacing=20; row_height=300;
	fl_width=100; fl_height=100;

	pane0 "widget/pane" { fl_width=100; fl_height=100; width=190; height=180; bgcolor = "#9cf"; } // x=100; y=50;
	pane1 "widget/pane" { fl_width=100; fl_height=100; width=130; height=180; bgcolor = "#ccc"; } // x=305; y=50;
	pane2 "widget/pane" { fl_width=100; fl_height=100; width=80;  height=320; bgcolor = "#f99"; } // x=455; y=50;
	pane3 "widget/pane" { fl_width=100; fl_height=100; width=150; height=140; bgcolor = "#9f9"; } // x=555; y=50;
	pane4 "widget/pane" { fl_width=100; fl_height=100; width=90;  height=240; bgcolor = "#99f"; } // x=725; y=50;
	pane5 "widget/pane" { fl_width=100; fl_height=100; width=230; height=100; bgcolor = "#ff9"; } // x=80;  y=390;
	pane6 "widget/pane" { fl_width=100; fl_height=100; width=80;  height=200; bgcolor = "#f9f"; } // x=325; y=390;
	pane7 "widget/pane" { fl_width=100; fl_height=100; width=170; height=220; bgcolor = "#9ff"; } // x=425; y=390;
	pane8 "widget/pane" { fl_width=100; fl_height=100; width=110; height=200; bgcolor = "#fc9"; } // x=615; y=390;
	pane9 "widget/pane" { fl_width=100; fl_height=100; width=130; height=120; bgcolor = "#cf9"; } // x=745; y=390;
	}

    // pane0 "widget/pane" { x=100; y=50;  width=190; height=180; bgcolor = "#9cf"; }
    // pane1 "widget/pane" { x=305; y=50;  width=130; height=180; bgcolor = "#ccc"; }
    // pane2 "widget/pane" { x=455; y=50;  width=80;  height=320; bgcolor = "#f99"; }
    // pane3 "widget/pane" { x=555; y=50;  width=150; height=140; bgcolor = "#9f9"; }
    // pane4 "widget/pane" { x=725; y=50;  width= 90; height=240; bgcolor = "#99f"; }
    // pane5 "widget/pane" { x=80;  y=390; width=230; height=100; bgcolor = "#ff9"; }
    // pane6 "widget/pane" { x=325; y=390; width=80;  height=200; bgcolor = "#f9f"; }
    // pane7 "widget/pane" { x=425; y=390; width=170; height=220; bgcolor = "#9ff"; }
    // pane8 "widget/pane" { x=615; y=390; width=110; height=200; bgcolor = "#fc9"; }
    // pane9 "widget/pane" { x=745; y=390; width=130; height=120; bgcolor = "#cf9"; }

    paneA "widget/pane" { x=40; y=680;  width=890; height=220; bgcolor = "#620"; }

    // Outline the visible area. 
    top_left0     "widget/pane" { x=0;   y=0;   width=10; height=10; bgcolor = "#f00"; }
    top_right0    "widget/pane" { x=990; y=0;   width=10; height=10; bgcolor = "#ff0"; }
    bottom_left0  "widget/pane" { x=0;   y=990; width=10; height=10; bgcolor = "#0f0"; }
    bottom_right0 "widget/pane" { x=990; y=990; width=10; height=10; bgcolor = "#00f"; }

    // Advance markers.
    top_left1     "widget/pane" { x=100; y=100; width=10; height=10; fl_x=25; fl_y=25; fl_width=25; fl_height=25; bgcolor = "#a00"; }
    top_right1    "widget/pane" { x=890; y=100; width=10; height=10; fl_x=25; fl_y=25; fl_width=25; fl_height=25; bgcolor = "#aa0"; }
    bottom_left1  "widget/pane" { x=100; y=890; width=10; height=10; fl_x=25; fl_y=25; fl_width=25; fl_height=25; bgcolor = "#0a0"; }
    bottom_right1 "widget/pane" { x=890; y=890; width=10; height=10; fl_x=25; fl_y=25; fl_width=25; fl_height=25; bgcolor = "#00a"; }

    // Interior markers.
    top_left2     "widget/pane" { x=250; y=250; width=10; height=10; fl_x=100; fl_y=100; fl_width=25; fl_height=25; bgcolor = "#700"; }
    top_right2    "widget/pane" { x=740; y=250; width=10; height=10; fl_x=100; fl_y=100; fl_width=25; fl_height=25; bgcolor = "#770"; }
    bottom_left2  "widget/pane" { x=250; y=740; width=10; height=10; fl_x=100; fl_y=100; fl_width=25; fl_height=25; bgcolor = "#070"; }
    bottom_right2 "widget/pane" { x=740; y=740; width=10; height=10; fl_x=100; fl_y=100; fl_width=25; fl_height=25; bgcolor = "#007"; }

    // Deep interior markers.
    top_left3     "widget/pane" { x=400; y=400; width=10; height=10; fl_x=25; fl_y=25; fl_width=100; fl_height=100; bgcolor = "#500"; }
    top_right3    "widget/pane" { x=590; y=400; width=10; height=10; fl_x=25; fl_y=25; fl_width=100; fl_height=100; bgcolor = "#550"; }
    bottom_left3  "widget/pane" { x=400; y=590; width=10; height=10; fl_x=25; fl_y=25; fl_width=100; fl_height=100; bgcolor = "#050"; }
    bottom_right3 "widget/pane" { x=590; y=590; width=10; height=10; fl_x=25; fl_y=25; fl_width=100; fl_height=100; bgcolor = "#005"; }

    // Center marker.
    center "widget/pane"
	{
	x=450; y=450; width=100; height=100; bgcolor = "orange";
	centerer "widget/pane" { x=25; y=25; width=50; height=50; bgcolor = "purple"; } // Debug
    	}
    }