// David Hopkins June 2025

$Version=2$
hard_reset_test_page "widget/page"
{
    // --- Page Style ---
    title = "Timed Event with HTML Display";
    bgcolor = "#f0f2f5"; 
    font_name = "Arial";
    width = 400; height = 550;
    show_diagnostics = "yes";

    // --- NON-VISUAL TIMER LOGIC ---
    // This timer chain is stable and includes the 'Reload' action.
    timer1_set_wait_text "widget/timer"
    {
        msec = 1; auto_start = 0; auto_reset = 0;
        cn1a_set_text "widget/connector" { event=Expire; target=the_button; action=SetText; Text="Waiting for 5 seconds..."; }
        cn1b_start_wait "widget/connector" { event=Expire; target=timer2_the_wait; action=SetTimer; Time=5000; }
    }
    timer2_the_wait "widget/timer"
    {
        msec = 5000; auto_start = 0; auto_reset = 0;
        cn2a_start_reset "widget/connector" { event=Expire; target=timer3_reset_text; action=SetTimer; Time=1; }
    }
    timer3_reset_text "widget/timer"
    {
        msec = 1; auto_start = 0; auto_reset = 0;
        // Action 1: Reset the button's text
        cn3a_reset_text "widget/connector" { event=Expire; target=the_button; action=SetText; Text="Click Me Again"; }
    }

    // --- VISUAL WIDGETS ---

    // 1. The Logo
    my_logo "widget/image"
    {
        source = "/sys/images/centrallix_374x66.png";
        x = 63; y = 20; 
        width = 274; height = 48; 
    }

    // 2. The Text Area
    info_textarea "widget/textarea"
    {
        x = 20; y = 90;
        width = 360; height = 100;
        bgcolor = "#ffffff";
        style = lowered;
    }

    // 3. The Button
    the_button "widget/textbutton"
    {
        x = 100; y = 200; // Positioned below the textarea
        width = 200; height = 40;
        text = "Start Timer";
        bgcolor = "#2563eb"; 
        fgcolor1 = "white";
        
        cn_master_click "widget/connector"
        {
            event = Click;
            target = timer1_set_wait_text;
            action = SetTimer;
            Time = 1;
        }
    }

    // 4. The HTML Area 
    htmlarea "widget/html"
    {
        mode = "dynamic";
        x = 95; y = 270; // Positioned below the button
        width = 360; height = 260; // Fills the remaining space
        source = "/tests/ui/index2.html"; 
        style = lowered;
    }
}