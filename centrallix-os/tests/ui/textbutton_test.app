$Version=2$

window_test "widget/page"
    {
    alerter "widget/alerter" {}
    background="/sys/images/slate2.gif";
    x=0; y=0; width=640; height=480;
    
	vbox "widget/vbox"
		{
		x=5; y=5; height=160; width=160;
		spacing=20;
		bgcolor="#c0c0c0";
		title="File Listing";

    	tristate_button "widget/textbutton"
			{
			x=8; y=8; width=100; height=32;
			bgcolor="#ffffff";
			fgcolor1="#000000";
			text="Button 1";
			tristate=yes;
			cn1 "widget/connector"
				{
				event="Click";
				target="alerter";
				action="Alert";
				param=runclient('You clicked button 1');
				}
			}

		bistate_button "widget/textbutton"
			{
			x=8; y=68; width=100; height=32;
			bgcolor="#cccccc";
			fgcolor1="#ffffff";
			text="Button 2";
			tristate=no;
			cn2 "widget/connector"
				{
				event="Click";
				target="alerter";
				action="Alert";
				param=runclient('You clicked button 2');
				}
			}
    	}
	}
