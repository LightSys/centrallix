//David Hopkins June 2025
// CMP DECL Test #2: Main Declaration Component Test
// .app file

$Version=2$
NestedComponentTest "widget/page"
{
    title = "Component Test Page";
    background = "/sys/images/wood.png";
    x = 0; y = 0; width = 350; height = 480;

    the_nested_component "widget/component"
    {
        x = 20; y = 20; width = 300; height = 250;
        path = "l2cmpdecl_test.cmp";
        initial_choice = 3;
        bgcolor = "white";
        border = "solid 1 #e0e0e0"; 

        on_selection_changed "widget/connector"
        {
            event = "selection_changed";
            action = "set";
            target = result_label;
            property = "text";
            Value = runserver("Selected value is: " + :source:value);
        }
    }

    osrc1 "widget/osrc"
    {
        replicasize = 20;
        readahead = 5;
        // Assuming you might have changed the CSV, this still works.
        sql = "select * from /tests/ui/cmpdecl/denominations.csv/rows";
        baseobj = "/tests/ui/cmpdecl/denominations.csv";

        denominations_table "widget/table"
        {
            x = 20;
            y = 290;        
            width = 300;
            height = 135;   
            mode = "dynamicrow"; 
            
            inner_border = 1;
            inner_padding = 2;
            row1_bgcolor = "#e0e0e0";
            hdr_bgcolor = "#c0c0c0";
            textcolor = "#000000";
            rowhighlight_bgcolor = "#000080";
            textcolorhighlight = "#ffffff";

            column_repeater "widget/repeat"
            {
                sql = "select :name from /tests/ui/cmpdecl/denominations.csv/columns";

                dynamic_column "widget/table-column"
                {
                    fieldname = runserver(:column_repeater:name);
                    title = runserver(:column_repeater:name);
                    width = 125; 
                }
            }
        }
    }


    result_label "widget/label" 
    {
        x = 20;
        y = 440;      
        width = 200;
        text = "Selection from other component appears here";
    }
}