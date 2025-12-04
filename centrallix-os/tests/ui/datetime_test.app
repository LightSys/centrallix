// David Hopkins May 2025

$Version=2$
editbox_test "widget/page"
    {
    title = "DateTime Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=100; height=480;

    vbox "widget/vbox"
    {
    x=10; y=10; height=200; width=260;
    spacing=20;
    bgcolor="#c0c0c0";
    title="DateTime Widget Test";

    dt "widget/datetime"
        {
        x=8; y=8; width=50; height=40;
        bgcolor="#ffffff";
        fgcolor="#000000";
        initialdate="01 Jan 2023";
        date_only="yes";
        default_time="00:00:00";
        search_by_range="no";
        }

    cn1 "widget/connector"
        {
        event="DataChange";
        action="SetValue";
        value=runclient(:dt:value);
        target="dt";
        source="dt";
        }

    eb "widget/editbox"
        {
        x=8; y=60; width=50; height=20;
        style=lowered;
        bgcolor="#e0e0e0";
        }

    cn2 "widget/connector"
        {
        event="Click";
        action="SetValue";
        value=runclient(:dt:value);
        target="eb";
        source="dt";
        }
     }
    }