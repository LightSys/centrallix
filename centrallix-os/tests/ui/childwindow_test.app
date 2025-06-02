// David Hopkins June 2025 

$Version=2$
// Top-level childwindow test application
editbox_test "widget/page"
{
    title = "Child Window Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=300; height=480;

    vbox "widget/vbox"
    {
        x=10; y=10; height=80; width=160;
        spacing=20;
        bgcolor="#c0c0c0";
        title="File Listing";
    }

    // Top-level childwindow, not inside vbox
    MyWindow "widget/childwindow"
    {
        x = 10; y = 100;
        width = 200; height = 200;
        style = "dialog";
        bgcolor = "#c0c0c0";
        hdr_background = "/sys/images/grey_gradient2.png";
        title = "David's Child Window";
        textcolor = "black";
        toplevel = "yes";      // Ensures floating above all widgets
        visible = true;        // Window is initially visible
        modal = yes;
        titlebar = "yes";  // Show title bar
    }
}
