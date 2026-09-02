$Version=2$
test "widget/page"
    {
    title = "Test App";
    bgcolor = "#ffffff";
    
    x = 0; y = 0;
    width = 500; height = 500;
    
    // Description:
    // This page is intended for testing scroll pane functionality, including
    // the associated events, actions, etc. It specifically tests the scroll
    // pane when used on an HTML widget, allowing us to load long HTML pages
    // to test scrolling large amounts of content.
    
    pane "widget/pane"
	{
	x = 10; y = 10;
	width = 480; height = 480;
	
	scroll "widget/scrollpane"
	    {
	    x = 0; y = 20;
	    width = 300; height = 300;
	    bgcolor = "#c0f1ba";
	    
	    // Content.
	//     html "widget/html"
	// 	{
	// 	x = 0; y = 0;
	// 	width = 300; height = 300;
		
	// 	mode = "dynamic";
	// 	source = "/samples/html_example2_long.html";
	// 	}
	
	    // Vertical content.
	    spacer "widget/pane"
		{
		x = 0; y = 0;
		width = 0; height = 2000;
		fl_height = 0;
		style = flat;
		}
	    a1 "widget/pane"
		{
		x = 10; y = 0;
		width = 250; height = 500;
		
		bgcolor = "#e6b2c4";
		}
	    a2 "widget/pane"
		{
		x = 20; y = 500;
		width = 200; height = 500;
		
		bgcolor = "#e6d2c4";
		}
	    a3 "widget/pane"
		{
		x = 30; y = 1000;
		width = 150; height = 500;
		
		bgcolor = "#e6b2c4";
		}
	    a4 "widget/pane"
		{
		x = 40; y = 1500;
		width = 100; height = 500;
		
		bgcolor = "#e6d2c4";
		}
	
	    // Horizontal Content.
	    // Unused because there is no horizontal scrollpane.
	//     spacer "widget/pane"
	// 	{
	// 	x = 0; y = 0;
	// 	width = 2000; height = 0;
	// 	fl_height = 0;
	// 	style = flat;
	// 	}
	//     a1 "widget/pane"
	// 	{
	// 	x = 0; y = 10;
	// 	width = 500; height = 250;
		
	// 	bgcolor = "#e6b2c4";
	// 	}
	//     a2 "widget/pane"
	// 	{
	// 	x = 500; y = 20;
	// 	width = 500; height = 200;
		
	// 	bgcolor = "#e6d2c4";
	// 	}
	//     a3 "widget/pane"
	// 	{
	// 	x = 1000; y = 30;
	// 	width = 500; height = 150;
		
	// 	bgcolor = "#e6b2c4";
	// 	}
	//     a4 "widget/pane"
	// 	{
	// 	x = 1500; y = 40;
	// 	width = 500; height = 100;
		
	// 	bgcolor = "#e6d2c4";
	// 	}
	    
	    // Ads, testing the Scroll event.
	    adv "widget/variable" { type = integer; value = runclient(0); }
	    ad1v "widget/variable" { type = integer; value = runclient(:adv:value); }
	    ad1c "widget/connector"
		{
		event = Scroll;
		target = test;
		action = Alert;
		event_condition = runclient(:ad1v:value == 0 and :Percent > 30 and :Change > 0);
		Message = runclient("Advertisement!!");
		}
	    ad1d "widget/connector"
		{
		event = Scroll;
		target = ad1v;
		action = SetValue;
		event_condition = runclient(:ad1v:value == 0 and :Percent > 30 and :Change > 0);
		Value = runclient(1);
		}
	    
	    ad2v "widget/variable" { type = integer; value = runclient(:adv:value); }
	    ad2c "widget/connector"
		{
		event = Scroll;
		target = test;
		action = Alert;
		event_condition = runclient(:ad2v:value == 0 and :Percent > 40 and :Change > 0);
		Message = runclient("Advertisement 2!!");
		}
	    ad2d "widget/connector"
		{
		event = Scroll;
		target = ad2v;
		action = SetValue;
		event_condition = runclient(:ad2v:value == 0 and :Percent > 40 and :Change > 0);
		Value = runclient(1);
		}
	    
	    ad3v "widget/variable" { type = integer; value = runclient(:adv:value); }
	    ad3c "widget/connector"
		{
		event = Scroll;
		target = test;
		action = Alert;
		event_condition = runclient(:ad3v:value == 0 and :Percent > 50 and :Change > 0);
		Message = runclient("Advertisement 3!!");
		}
	    ad3d "widget/connector"
		{
		event = Scroll;
		target = ad3v;
		action = SetValue;
		event_condition = runclient(:ad3v:value == 0 and :Percent > 50 and :Change > 0);
		Value = runclient(1);
		}
	    
	    // Log events for debugging.
	    debug_Scroll "widget/connector"
		{
		event = Scroll;
		target = test;
		action = Log;
		Message = runclient("Scroll " + :Percent + " " + :Change);
		}
	    debug_Wheel "widget/connector"
		{
		event = Wheel;
		target = test;
		action = Log;
		Message = runclient("Wheel:     ctrlKey=" + :ctrlKey + " shiftKey=" + :shiftKey + " altKey=" + :altKey + " metaKey=" + :metaKey + " button=" + :button);
		}
	    debug_Click "widget/connector"
		{
		event = Click;
		target = test;
		action = Log;
		Message = runclient("Click:     ctrlKey=" + :ctrlKey + " shiftKey=" + :shiftKey + " altKey=" + :altKey + " metaKey=" + :metaKey + " button=" + :button);
		}
	    debug_MouseDown "widget/connector"
		{
		event = MouseDown;
		target = test;
		action = Log;
		Message = runclient("MouseDown: ctrlKey=" + :ctrlKey + " shiftKey=" + :shiftKey + " altKey=" + :altKey + " metaKey=" + :metaKey + " button=" + :button);
		}
	    debug_MouseUp "widget/connector"
		{
		event = MouseUp;
		target = test;
		action = Log;
		Message = runclient("MouseUp:   ctrlKey=" + :ctrlKey + " shiftKey=" + :shiftKey + " altKey=" + :altKey + " metaKey=" + :metaKey + " button=" + :button);
		}
	    debug_MouseOver "widget/connector"
		{
		event = MouseOver;
		target = test;
		action = Log;
		Message = runclient("MouseOver");
		}
	    debug_MouseOut "widget/connector"
		{
		event = MouseOut;
		target = test;
		action = Log;
		Message = runclient("MouseOut");
		}
	    debug_MouseMove "widget/connector"
		{
		event = MouseMove;
		target = test;
		action = Log;
		Message = runclient("MouseMove");
		}
	    }
	
	button1 "widget/textbutton"
	    {
	    x = 5; y = 5;
	    width = 75; height = 30;
	    font_size = 18;
	    
	    bgcolor="#0c0447";
	    text = "To 45%";
	    
	    button1c "widget/connector"
		{
		event = Click;
		target = scroll;
		action = ScrollTo;
		Percent = 45;
		}
	    }
	button2 "widget/textbutton"
	    {
	    x = 5; y = 40;
	    width = 75; height = 30;
	    font_size = 18;
	    
	    bgcolor="#241672";
	    text = "To 100px";
	    
	    button2c "widget/connector"
		{
		event = Click;
		target = scroll;
		action = ScrollTo;
		Offset = 100;
		}
	    }
	}
    }