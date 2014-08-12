$Version=2$
page "widget/page" 
        {
        title = "Sample file upload app";
        width = 600;
        height = 800;
        
        file_name "widget/variable"
                {
                type = string;
                value = "";
                }
                
        input "widget/fileupload"
                {
                multiselect = yes;
                fieldname = "file";
                target = "/samples//";
                
                textchange "widget/connector"
                        {
                        target = file_name;
                        event = "DataChange";
                        action = "SetValue";
                        Value = runclient(:NewValue);
                        }
                }
                
        file_label "widget/label"
                {
                x = 5; y = 5;
                width = 100; height = 20;
                value = runclient(:file_name:value);
                
                file_label_click "widget/connector"
                        {
                        event = "Click";
                        target = input;
                        action = "Prompt";
                        }
                }
        
        set_button "widget/textbutton"
                {
                x = 110; y = 5;
                height = 20; width = 50;
                align = center;
                tristate = no;

                type = text;
                text = "Set";

                set_button_click "widget/connector"
                        {
                        event = "Click";
                        target = input;
                        action = "Prompt";
                        }
                }
        
        reset_button "widget/textbutton"
                {
                x=5; y=30;
                width = 50; height = 20;
                align = center;
                type = image;
                tristate = no;
                text = "Reset";

                //This is similar to the above connector.
                //Notice that I didn't set the source property.
                input_conn "widget/connector"
                        {
                        event = "Click";
                        target = input;
                        action = "Clear";
                        }
                }
            
        submit_button "widget/textbutton"
                {
                //In this case, I set the height.  Otherwise the button wouldn't fit.
                x=55; y=30;
                height = 20; width = 50;
                align = center;
                tristate = no; //Misc visual option.

                type = text;
                text = "Submit";

                submit_button_click "widget/connector"
                        {
                        event = "Click";
                        source = submit_button; //Defaults to parent widget if not set.
                        target = input; //The widget that gets the action
                        //Actions are predefined.  "SetValue" is the only valid action to send a variable widget.
                        action = "Submit";
                        }
                }
        }