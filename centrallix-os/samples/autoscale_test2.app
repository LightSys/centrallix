$Version=2$
MyPage "widget/page"
    {
    title = "Flex Testing Page";
    bgcolor = "black";
    textcolor = "#00f8ff";
    width = 1000;
    height = 1000;

    box "widget/pane"
        {
        x=25; y=25; width=975; height=975; bgcolor = "#111";

	// Note: fl_x and fl_y seem to be ignored.
	standard "widget/pane" { x=100; y=100; width=100; height=100; fl_x=100; fl_y=100; fl_width=100; fl_height=100; bgcolor = "orange"; }
	big      "widget/pane" { x=250; y=250; width=200; height=200; fl_x=10;  fl_y=50;  fl_width=10;  fl_height=10;  bgcolor = "purple"; }
	double   "widget/pane" { x=500; y=500; width=200; height=200; fl_x=50;  fl_y=10;  fl_width=50;  fl_height=50;  bgcolor = "green";  }
	}
    }