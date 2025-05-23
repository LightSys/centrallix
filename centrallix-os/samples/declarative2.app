$Version=2$
declarative_test "widget/page"
    {
    width = 800;
    height = 600;
    bgcolor = "#c0c0c0";
    title = "Test of Declarative Expression";

    myvbox "widget/vbox"
	{
	x = 10;
	y = 10;
	width = 300;
	height = 580;
	spacing = 10;

	hello "widget/label"
	    {
	    height = 16;
	    value = runclient("Hello, " + :my_editbox:content + "!");
	    style = bold;
	    }

	my_editbox "widget/editbox"
	    {
	    height = 24;
	    bgcolor = white;
	    }

	btn "widget/textbutton"
	    {
	    height = 32;
	    width = 100;
	    bgcolor = white;
	    text = "Submit";
	    tristate = no;
	    enabled = runclient(char_length(:my_editbox:content) > 0);

	    on_btn_click "widget/connector"
		{
		event = Click;
		target = declarative_test;
		action = Alert;
		Message = runclient("Hello, " + :my_editbox:content + ".");
		}
	    }
	}
    }
