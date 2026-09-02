//David Hopkins May 2025 

$Version=2$
textarea_test "widget/page"
    {
    title = "Textarea Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=300; height=480;

    vbox "widget/vbox"
    {
    x=10; y=10; height=80; width=160;
    spacing=20;
    bgcolor="#c0c0c0";
    title="File Listing";

    ta "widget/textarea"
        {
        x=8; y=8; width=128; height=60;
        style=lowered;
        bgcolor="#e0e0e0";
        }

    ta_readonly "widget/textarea"
        {
        x=8; y=80; width=128; height=60;
        style=lowered;
        bgcolor="#f0f0f0";
        readonly=yes;
        value="This textarea is read-only.";
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
