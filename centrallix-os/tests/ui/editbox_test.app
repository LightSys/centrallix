$Version=2$
editbox_test "widget/page"
    {
    title = "Editbox Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=300; height=480;

	vbox "widget/vbox"
	{
	x=10; y=10; height=80; width=160;
	spacing=20;
	bgcolor="#c0c0c0";
	title="File Listing";

	
	testtext "widget/variable"
		{
		fieldname = 'testtext';
		value = 'From Widget';
		}

    eb "widget/editbox"
		{
		x=8; y=8; width=128; height=20;
		style=lowered;
		bgcolor="#e0e0e0";
		}

	cn1 "widget/connector"
		{
		event="Click"; 
		action="SetValue";
		Value=runclient(:testtext:value); 
		target="eb";        
		source="eb";
		}

	eb2 "widget/editbox"
		{
		x=8; y=40; width=128; height=20;
		style=lowered;
		bgcolor="#e0e0e0";
		}



// need to make this call a variable then modify it in JS



	//  eb2 "widget/editbox"
	// {
	// x=8; y=72; width=128; height=20;
	// style=lowered;
	// bgcolor="#e0e0e0";
	// }

	// testcheck2 "widget/checkbox"
	// {
	//     x = 20; y = 40; width = 12; height = 12;
	// }
    // testcheck "widget/checkbox"
	// {
	//     x = 20; y = 20; width = 12; height = 12;
	// }

    // label1 "widget/label"
    // 	{
	//     x = 40; y = 17; width = 150; height = 12;
	//     text = "First Checkbox";
	// }
    // label2 "widget/label"
    // 	{
	//     x = 40; y = 37; width = 150; height = 12;
	//     text = "Second Checkbox";
	// }

    // eb_toosmall "widget/editbox"
	// {
	// x=8; y=8 + 64; width=128; height=1;
	// style=lowered;
	// bgcolor="#e0e0e0";
	// }

    // eb_toobig "widget/editbox"
	// {
	// x=8; y=8 + 128; width=128; height=40;
	// style=lowered;
	// bgcolor="#e0e0e0";
	// }

    // eb_raised "widget/editbox"
	// {
	// x=8; y=8 + 192; width=128; height=20;
	// style=raised;
	// bgcolor="#e0e0e0";
	// }

    // eb_readonly "widget/editbox"
	// {
	// x=8; y=8 + 256; width=128; height=20;
	// style=lowered;
	// readonly=yes;
	// bgcolor="#e0e0e0";
	// }
	
	}
	
    }
