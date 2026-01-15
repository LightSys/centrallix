//David Hopkins May 2025 
//NOTE: There are 3 checkmarks
// 1. Idle state: When page firt started, text says ready. When this button is clicked, it will change the text to "TwoStateBtn clicked!".
// 2. Pointed state: When mouse pointer is over the button, it will change the text to "ThreeStateBtn clicked!".
// 3. Enable/Disable state: When the Enable button is clicked, it will enable the DisabledBtn and change the text to "Enabled DisabledBtn". 
// When the Disable button is clicked, it will disable the DisabledBtn and change the text to "Disabled DisabledBtn".
// The light greay is enable, while the dark blue is disable. 

$Version=2$
imagebutton_test "widget/page"
{
    title = "Imagebutton Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=300; height=480;

    vbox "widget/vbox"
    {
        x=10; y=10; height=300; width=250;
        spacing=20;
        bgcolor="#c0c0c0";
        title="ImageButton Examples";

        // Two-state ImageButton (idle/clicked) using text and border
        TwoStateBtn "widget/imagebutton"
        {
            x = 10; y = 10;
            width = 50;
            height = 50;
            image = "/sys/images/green_check.gif";                // idle state text
            clickimage = "/sys/images/green_check.gif";      // click image
            tooltip = "Two-state button";
            repeat = "no";                           // repeat property example
        }

        // Three-state ImageButton (idle/pointed/clicked)
        ThreeStateBtn "widget/imagebutton"
        {
            x = 70; y = 10;
            width = 50;
            height = 50;
            image = "/sys/images/green_check.gif";
            pointimage = "/sys/images/grey_check.gif";        // pointimage property
            clickimage = "/sys/images/green_check.gif";
            tooltip = "Three-state button";
            repeat = "no";
        }

        // Disabled ImageButton
        DisabledBtn "widget/imagebutton"
        {
            x = 130; y = 10;
            width = 50;
            height = 50;
            image = "/sys/images/green_check.gif";
            clickimage = "/sys/images/grey_check.gif";
            disabledimage = "/sys/images/green_check.gif";  // disabledimage property
            enabled = 0;                             // enabled property (disabled)
            tooltip = "Disabled button";
            repeat = "no";
        }

        // Example: Enable/Disable actions
        EnableBtn "widget/imagebutton"
        {
            x = 10; y = 70;
            width = 50;
            height = 30;
            image = "/sys/images/slate2_dark.gif";
            tooltip = "Enable DisabledBtn";
        }
        DisableBtn "widget/imagebutton"
        {
            x = 70; y = 70;
            width = 50;
            height = 30;
            image = "/sys/images/slate_blue.png";
            tooltip = "Disable DisabledBtn";
            
        }

        // Connectors for Enable/Disable actions
        cn_enable "widget/connector"
        {
            event = "Click";
            action = "Enable";
            target = "DisabledBtn";
            source = "EnableBtn";
        }
        cn_disable "widget/connector"
        {
            event = "Click";
            action = "Disable";
            target = "DisabledBtn";
            source = "DisableBtn";
        }

        // Status label for button actions
        status "widget/label"
        {
            x = 10; y = 110;
            width = 220;
            height = 28;
            value = "Ready";
            font = "Arial";
            fontsize = 20;
            fgcolor = "#2e7d32";
            bgcolor = "#f0f0f0";
            border = 1;
            align = "center";
        }

        // Connector: Show status when TwoStateBtn is clicked
        cn1 "widget/connector"
        {
            event = "Click";
            action = "SetValue";
            Value = "TwoStateBtn clicked!";
            target = "status";
            source = "TwoStateBtn";
        }

        // Connector: Show status when ThreeStateBtn is clicked
        cn2 "widget/connector"
        {
            event = "Click";
            action = "SetValue";
            Value = "ThreeStateBtn clicked!";
            target = "status";
            source = "ThreeStateBtn";
        }

        // Connector: Show status when EnableBtn is clicked
        cn3 "widget/connector"
        {
            event = "Click";
            action = "SetValue";
            Value = "Enabled DisabledBtn";
            target = "status";
            source = "EnableBtn";
        }

        // Connector: Show status when DisableBtn is clicked
        cn4 "widget/connector"
        {
            event = "Click";
            action = "SetValue";
            Value = "Disabled DisabledBtn";
            target = "status";
            source = "DisableBtn";
        }
    }
}
