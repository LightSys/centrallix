//David Hopkins May 2025

$Version=2$
scrollbar_only_test "widget/page"
{
    title = "Scrollbar Only Test";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=300; height=120;

    vbox "widget/vbox"
    {
        x=20; y=20; height=60; width=50;
        spacing=30;
        bgcolor="#c0c0c0";
        title="Scrollbar Test";

        sb1 "widget/scrollbar"
        {
            x=10; y=10; width=128; height=30;
            direction="horizontal";
            bgcolor="#d0d0d0";
            range=100;
        }
    }
}
