//Davd Hopkins May 2025

$Version=2$
MyScrollablePage "widget/page"
{
    title = "Scrollable List Example";
    x = 0; y = 0;
    width = 640; height = 360;
    background = "/sys/images/slate2.gif";

    // Scrollable area
    mypane "widget/pane"
	{
	x=100; y=100; width=300; height=300;
	style = "lowered";
	bgcolor = "#c0c0c0";
	myscroll "widget/scrollpane"
		{
		x=0; y=0; width=198; height=198;
		mytreeview "widget/treeview"
			{
			x=1; y=1; width=175;
			source="/";
			}
		}
	}
}
