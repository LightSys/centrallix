$Version=2$

timer_test "widget/page"
    {
    title = "Timer Test";
    bgcolor=gray;
    x=0; y=0; width=640; height=480;
    
    tmr "widget/timer"
        {
	msec=1000;
	auto_reset=0;
	c1 "widget/connector" { target=dlg1; event=Expire; action=SetVisibility; IsVisible=1; }
	c2 "widget/connector" { target=tmr2; event=Expire; action=SetTimer; Time=500; }
	}
    tmr2 "widget/timer"
        {
	msec=500;
	auto_reset=0;
	auto_start=0;
	c3 "widget/connector" { target=dlg1; event=Expire; action=SetVisibility; IsVisible=0; }
	c4 "widget/connector" { target=tmr; event=Expire; action=SetTimer; Time=500; }
	}
    dlg1 "widget/childwindow"
	{
	x = 40; y=40; width=200; height=200;
	visible=false;
	title = "Blinkin' Window";
	bgcolor='#404040';
	style=dialog;
	}
    tb1 "widget/textbutton"
	{
	x = 40; y=300; width=200; height=48;
	text="Stop Blinking!";
	tristate=no;
	bgcolor='#c0c0c0';
	c5 "widget/connector" { target=tmr2; event=Click; action=CancelTimer; }
	c6 "widget/connector" { target=tmr; event=Click; action=CancelTimer; }
	}
    tb2 "widget/textbutton"
	{
	x = 40; y=370; width=200; height=48;
	text="Start Blinking!";
	tristate=no;
	bgcolor='#c0c0c0';
	c7 "widget/connector" { target=tmr; event=Click; action=SetTimer; Time=500; }
	}
    }
