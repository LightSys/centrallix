// David Hopkins June 2025

$Version=2$
// Top-level HTML test application
HTMLtest "widget/page"
{
    title = "HTML Test Application";
    background = "/sys/images/slate2.gif";
    x = 0; y = 0; width = 300; height = 480;

        htmlarea "widget/html"
        {
            mode = "dynamic";
            x = 2; y = 2;
            width = 478; height = 396;
            source = "/tests/ui/index.html"; 
        }
}
