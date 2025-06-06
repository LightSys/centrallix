//David Hopkins June 2025
// CMP DECL test #2: Main Declaration Component Test
// .app file

$Version=2$
NestedComponentTest "widget/page"
{
    title = "David's Nested Component Test";
    background = "/sys/images/slate2.gif";
    x = 0; y = 0; width = 350; height = 480;

    the_nested_component "widget/component"
    {
        x = 20; y = 20; width = 300; height = 250;
        // This assumes l2cmpdecl_test.cmp is in the same folder as this .app file.
        path = "l2cmpdecl_test.cmp";
        initial_choice = 3;

        // The event name is "selection_changed", which we will define in the components.
        on_selection_changed "widget/connector"
        {
            event = "selection_changed";
            action = "set";
            target = result_label;
            property = "text";
            // ':source' refers to the widget that fired the event (the radio button).
            // ':source:value' gets the 'value' property from that radio button.
            Value = runserver("Selected value is: " + :source:value);
        }
    }

    // This button is no longer needed, as the label updates instantly.
    // get_value_btn "widget/textbutton"
    // {
    //     x = 50; y = 300; width = 100; height = 30;
    //     text = "Get Value";
    //     fgcolor = "#000000";
    //     bgcolor = "#cccccc";
    // }

    // result_label "widget/label"
    // {
    //     x = 50; y = 340; width = 200; height = 30;
    //     fgcolor = "white";
    //     bgcolor = "black";
    //     text = "Click a radio button";
    // }
}


