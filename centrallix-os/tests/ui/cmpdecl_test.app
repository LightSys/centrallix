//David Hopkins June 2025

$Version=2$
CMPDECLtest "widget/page"
{
    title = "Component Decl Application";
    background = "/sys/images/slate2.gif";
    x = 0; y = 0; width = 340; height = 520;

    header "widget/label"
    {
        x=0; y=0; width=340; height=40;
        text="Component Declaration Test";
        fgcolor="#ffffff";
        bgcolor="#3a506b";
        align=center;
    }

    vbox "widget/vbox"
    {
        x=20; y=60; height=320; width=300;
        spacing=16;
        bgcolor="#f5f6fa";
        borderwidth=2;
        title="File Listing";
    
        ta "widget/textarea"
        {
            x=10; y=10; width=280; height=180;
            style=flat;
            bgcolor="#e0e0e0";
            fgcolor="#222";
            borderwidth=1;
        }

        button1 "widget/component"
        {
            x=90; y=200; width=120; height=36;
            mode=static;
            path="/tests/ui/cmpdecl_test.cmp";
        }

        cn1 "widget/connector"
        {
            event="Click"; 
            action="SetValue";
            Value=runclient(:testtext:value); 
            target="ta";        
            source="ta";
        }
    }

}