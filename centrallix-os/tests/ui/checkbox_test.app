//David Hopkins May 2025

$Version=2$
checkbox_test "widget/page"
{
    title = "Checkbox Test Application";
    background = "/sys/images/slate2.gif";
    x = 0; y = 0; width = 300; height = 480;

    vbox "widget/vbox"
    {
        x = 10; y = 10; height = 80; width = 80;
        spacing = 20;
        bgcolor = "#c0c0c0";
        title = "Checkbox Only";

        checkbox1 "widget/checkbox"
        {
            x = 8; y = 20; width = 20; height = 20;
            fieldname = "mycheckbox1";
            checked = no;
        }

        checkbox2 "widget/checkbox"
        {
            x = 8; y = 50; width = 20; height = 20;
            fieldname = "mycheckbox2";
            checked = yes;
        }
    }
}
