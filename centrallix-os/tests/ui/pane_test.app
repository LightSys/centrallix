//David Hopkins June 2025

$Version=2$
panetest "widget/page"
{
    title      = "Pane Test";
    background = "/sys/images/slate2.gif";
    x          = 0;   y = 0;   width  = 300; height = 120;

    // First row
    loweredmypane "widget/pane"
    {
        x      = 10;  y = 10;  width  = 30;  height = 30;
        style  = "lowered";
        bgcolor= "#c0c0c0";
    }

    raisedmypane "widget/pane"
    {
        x      = 50;  y = 10;  width  = 30;  height = 30;
        style  = "raised";
        bgcolor= "#c0c0c0";
    }

    borderedmypane "widget/pane"
    {
        x      = 90;  y = 10;  width  = 30;  height = 30;
        style  = "bordered";
        bgcolor= "#c0c0c0";
    }

    flatmypane "widget/pane"
    {
        x      = 130; y = 10;  width  = 30;  height = 30;
        style  = "flat";
        bgcolor= "#c0c0c0";
    }

    // Second row
    transparentmypane "widget/pane"
    {
        x      = 10;  y = 50;  width  = 30;  height = 30;
        style  = "bordered";
        bgcolor= "";
    }
}
