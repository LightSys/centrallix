$Version=2$
TabFeatures "widget/page"
    {
    x = 0; y = 0;
    width = 500; height = 500;
    
    title = "Tab Features Demonstrated";
    bgcolor = "#b0b0b0";
    
    tloc "widget/parameter" { type = "string"; default = "top"; }
    w    "widget/parameter" { type = "integer"; default = 220; }
    h    "widget/parameter" { type = "integer"; default = 70; }
    
    tloc_label "widget/label"
	{
	x = 2; y = 1;
	width = 50; height = 20;
	font_size = 16;
	
	text = "Tab Location:";
	}
    
    button_top "widget/textbutton"
	{
	x = 55; y = 1;
	width = 27; height = 18;
	font_size = 10;
	
	text = "Top";
	bgcolor="#0c0447ff";
	
	button_top_c "widget/connector"
	    {
	    event = Click;
	    target = TabFeatures;
	    action = LoadPage;
	    Source = "TabFeatures.app?tloc=top&w=220&h=70";
	    }
	}

    button_bottom "widget/textbutton"
	{
	x = 85; y = 1;
	width = 27; height = 18;
	font_size = 10;
	
	text = "Bottom";
	bgcolor="#0c0447ff";
	
	button_bottom_c "widget/connector"
	    {
	    event = Click;
	    target = TabFeatures;
	    action = LoadPage;
	    Source = "TabFeatures.app?tloc=bottom&w=220&h=70";
	    }
	}

    button_left "widget/textbutton"
	{
	x = 115; y = 1;
	width = 27; height = 18;
	font_size = 10;
	
	text = "Left";
	bgcolor="#0c0447ff";
	
	button_left_c "widget/connector"
	    {
	    event = Click;
	    target = TabFeatures;
	    action = LoadPage;
	    Source = "TabFeatures.app?tloc=left&w=140&h=90";
	    }
	}

    button_right "widget/textbutton"
	{
	x = 145; y = 1;
	width = 27; height = 18;
	font_size = 10;
	
	text = "Right";
	bgcolor="#0c0447ff";
	
	button_right_c "widget/connector"
	    {
	    event = Click;
	    target = TabFeatures;
	    action = LoadPage;
	    Source = "TabFeatures.app?tloc=right&w=140&h=90";
	    }
	}

    t1 "widget/tab"
	{
	x = 0; y = 20;
	height = 500; width = 500;
	
	bgcolor = "#c0c0c0";
	inactive_bgcolor = "#a8a8a8";
	selected_index = 2;
	tab_location = top;
	select_translate_out = 1;

	t11 "widget/tabpage"
	    {
	    title = "Spacing Rant";

	    t11l0 "widget/label" { x=20; y=20; width=200; height=40; font_size=24; text="Spacing Rant"; }
	    
	    t11t1 "widget/tab"
		{
		x = 20; y = 60;
		width = runserver(:this:w); height = 100;
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif"; // "/sys/images/4Color.png"
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;

		t11t11 "widget/tabpage" { title = "Tab 1"; t11t11l "widget/label" { x=10; y=10; width=100; height=32; style=bold; text="10px X 10px"; } }
		t11t12 "widget/tabpage" { title = "Tab 2"; t11t12l "widget/label" { x=20; y=30; width=100; height=32; style=bold; text="20px X 30px"; } }
		t11t13 "widget/tabpage" { title = "Tab 3"; t11t13l "widget/label" { x=30; y=50; width=100; height=32; style=bold; text="30px X 50px"; } }
		t11t14 "widget/tabpage" { title = "Tab 4"; t11t14l "widget/label" { x=40; y=70; width=100; height=32; style=bold; text="40px X 70px"; } }

		t11t1c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t11t2;
		    TabIndex = runclient(:t11t1:selected_index);
		    }
		}
	    
	    t11t2 "widget/tab"
		{
		x = 260; y = 60;
		width = runserver(:this:w); height = 100;
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		bgcolor = "#c0c0c0";
		inactive_bgcolor = "#b8b8b8";
		selected = t11t22;

		t11t21 "widget/tabpage" { title = "Tab 1"; t11t21l "widget/label" { x=10; y=10; width=100; height=32; style=bold; text="10px X 10px"; } }
		t11t22 "widget/tabpage" { title = "Tab 2"; t11t22l "widget/label" { x=20; y=30; width=100; height=32; style=bold; text="20px X 30px"; } }
		t11t23 "widget/tabpage" { title = "Tab 3"; t11t23l "widget/label" { x=30; y=50; width=100; height=32; style=bold; text="30px X 50px"; } }
		t11t24 "widget/tabpage" { title = "Tab 4"; t11t24l "widget/label" { x=40; y=70; width=100; height=32; style=bold; text="40px X 70px"; } }

		t11t2c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t11t1;
		    TabIndex = runclient(:t11t2:selected_index);
		    }
		}
	    
	    t11p "widget/pane"
	    	{
	    	x = 20; y = 180; width=runserver(:this:w); height=100;

	    	t11p1l "widget/label" { x=10; y=10; width=100; height=32; style=bold; text="10px X 10px"; }
	    	t11p2l "widget/label" { x=20; y=30; width=100; height=32; style=bold; text="20px X 30px"; }
	    	t11p3l "widget/label" { x=30; y=50; width=100; height=32; style=bold; text="30px X 50px"; }
	    	t11p4l "widget/label" { x=40; y=70; width=100; height=32; style=bold; text="40px X 70px"; }
	    	}
	    }

	t12 "widget/tabpage"
	    {
	    title = "Tab Spacing";

	    t12l0 "widget/label" { x=20; y=20; width=400; height=40; font_size=24; text="Tab Spacing"; }
	    
	    t12l1 "widget/label" { x=30; y=75; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="Default (2px)"; }
	    t12t1 "widget/tab"
		{
		x = 20; y = 100;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;

		t12t11 "widget/tabpage" { title = "First";      t12t11l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t12t12 "widget/tabpage" { title = "Looong Tab"; t12t12l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t12t13 "widget/tabpage" { title = "S";          t12t13l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t12t14 "widget/tabpage" { title = "Last Tab";   t12t14l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t12t1c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t12t2;
		    TabIndex = runclient(:t12t1:selected_index);
		    }
		}
	    
	    t12l2 "widget/label" { x=270; y=75; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="No Tab Spacing"; }
	    t12t2 "widget/tab"
		{
		x = 260; y = 100;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t12t22;
		tab_spacing = 0;

		t12t21 "widget/tabpage" { title = "First";      t12t21l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t12t22 "widget/tabpage" { title = "Looong Tab"; t12t22l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t12t23 "widget/tabpage" { title = "S";          t12t23l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t12t24 "widget/tabpage" { title = "Last Tab";   t12t24l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t12t2c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t12t3;
		    TabIndex = runclient(:t12t2:selected_index);
		    }
		}
	    
	    t12l3 "widget/label" { x=30; y=215; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="Tab Spacing 8px"; }
	    t12t3 "widget/tab"
		{
		x = 20; y = 240;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;
		tab_spacing = 8;

		t12t31 "widget/tabpage" { title = "First";      t12t31l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t12t32 "widget/tabpage" { title = "Looong Tab"; t12t32l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t12t33 "widget/tabpage" { title = "S";          t12t33l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t12t34 "widget/tabpage" { title = "Last Tab";   t12t34l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t12t3c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t12t4;
		    TabIndex = runclient(:t12t3:selected_index);
		    }
		}
	    
	    t12l4 "widget/label" { x=270; y=215; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="Tab Spacing 16px"; }
	    t12t4 "widget/tab"
		{
		x = 260; y = 240;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t12t42;
		tab_spacing = 16;

		t12t41 "widget/tabpage" { title = "First";      t12t41l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t12t42 "widget/tabpage" { title = "Looong Tab"; t12t42l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t12t43 "widget/tabpage" { title = "S";          t12t43l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t12t44 "widget/tabpage" { title = "Last Tab";   t12t44l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t12t4c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t12t5;
		    TabIndex = runclient(:t12t4:selected_index);
		    }
		}
	    
	    t12l5 "widget/label" { x=30; y=355; width=60; height=20; fl_width = 1; font_size=16; style="bold"; text="Tab Spacing -8px"; }
	    t12l5a "widget/label" { x=90; y=357; width=runserver(:this:w-80); height=15; font_size=12; style="bold"; fgcolor="red"; text="(Negative values not recomended)"; }
	    t12t5 "widget/tab"
		{
		x = 20; y = 380;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;
		tab_spacing = -8;

		t12t51 "widget/tabpage" { title = "First";      t12t51l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t12t52 "widget/tabpage" { title = "Looong Tab"; t12t52l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t12t53 "widget/tabpage" { title = "S";          t12t53l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t12t54 "widget/tabpage" { title = "Last Tab";   t12t54l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t12t5c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t12t6;
		    TabIndex = runclient(:t12t5:selected_index);
		    }
		}
	    
	    t12l6 "widget/label" { x=270; y=355; width=70; height=20; fl_width = 1; font_size=16; style="bold"; text="Tab Spacing -16px"; }
	    t12l6a "widget/label" { x=345; y=357; width=runserver(:this:w-70); height=15; font_size=12; style="bold"; fgcolor="red"; text="(Negative values not recomended)"; }
	    t12t6 "widget/tab"
		{
		x = 260; y = 380;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t12t62;
		tab_spacing = -16;

		t12t61 "widget/tabpage" { title = "First";      t12t61l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t12t62 "widget/tabpage" { title = "Looong Tab"; t12t62l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t12t63 "widget/tabpage" { title = "S";          t12t63l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t12t64 "widget/tabpage" { title = "Last Tab";   t12t64l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t12t6c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t12t2;
		    TabIndex = runclient(:t12t6:selected_index);
		    }
		}
	    }
	
	t13 "widget/tabpage"
	    {
	    title = "Tab Height";

	    t13l0 "widget/label" { x=20; y=20; width=400; height=40; font_size=24; text="Tab Height"; }
	    
	    t13l1 "widget/label" { x=30; y=75; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="Default (24px)"; }
	    t13t1 "widget/tab"
		{
		x = 20; y = 100;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;

		t13t11 "widget/tabpage" { title = "First";      t13t11l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t13t12 "widget/tabpage" { title = "Looong Tab"; t13t12l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t13t13 "widget/tabpage" { title = "S";          t13t13l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t13t14 "widget/tabpage" { title = "Last Tab";   t13t14l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t13t1c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t13t3;
		    TabIndex = runclient(:t13t1:selected_index);
		    }
		}
	    
	    t13l2 "widget/label" { x=270; y=75; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="No Tab Height"; }
	    t13l2a "widget/label"
	        {
		x = 270; y = 110;
		width = runserver(:this:w-20); height = runserver(:this:h - 20);
		font_size = 24; style = bold; fgcolor = "red";
		text = "Not allowed";
		}
		
	    t13l2b "widget/label"
	        {
		x = 270; y = 140;
		width = runserver(:this:w-20); height = 30;
		font_size = 18; style = italic; fgcolor = "red";
		text = "Because I hate fun.";
		}
	    
	    t13l3 "widget/label" { x=30; y=215; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="Tab Height 32px"; }
	    t13t3 "widget/tab"
		{
		x = 20; y = 240;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;
		tab_height = 32;

		t13t31 "widget/tabpage" { title = "First";      t13t31l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t13t32 "widget/tabpage" { title = "Looong Tab"; t13t32l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t13t33 "widget/tabpage" { title = "S";          t13t33l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t13t34 "widget/tabpage" { title = "Last Tab";   t13t34l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t13t3c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t13t4;
		    TabIndex = runclient(:t13t3:selected_index);
		    }
		}
	    
	    t13l4 "widget/label" { x=270; y=215; width=runserver(:this:w-20); height=40; font_size=16; style="bold"; text="Tab Height 48px"; }
	    t13t4 "widget/tab"
		{
		x = 260; y = 240;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t13t42;
		tab_height = 48;

		t13t41 "widget/tabpage" { title = "First";      t13t41l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t13t42 "widget/tabpage" { title = "Looong Tab"; t13t42l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t13t43 "widget/tabpage" { title = "S";          t13t43l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t13t44 "widget/tabpage" { title = "Last Tab";   t13t44l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t13t4c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t13t5;
		    TabIndex = runclient(:t13t4:selected_index);
		    }
		}
	    
	    t13l5 "widget/label" { x=30; y=355; width=60; height=20; font_size=16; style="bold"; text="Tab Height 16px"; }
	    t13l5a "widget/label" { x=90; y=357; width=runserver(:this:w-80); height=15; font_size=12; style="bold"; fgcolor="red"; text="(Negative values not supported)"; }
	    t13t5 "widget/tab"
		{
		x = 20; y = 380;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;
		tab_height = 16;

		t13t51 "widget/tabpage" { title = "First";      t13t51l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t13t52 "widget/tabpage" { title = "Looong Tab"; t13t52l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t13t53 "widget/tabpage" { title = "S";          t13t53l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t13t54 "widget/tabpage" { title = "Last Tab";   t13t54l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t13t5c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t13t6;
		    TabIndex = runclient(:t13t5:selected_index);
		    }
		}
	    
	    t13l6 "widget/label" { x=270; y=355; width=60; height=20; font_size=16; style="bold"; text="Tab Spacing 8px"; }
	    t13l6a "widget/label" { x=335; y=357; width=runserver(:this:w-80); height=15; font_size=12; style="bold"; fgcolor="red"; text="(Negative values not supported)"; }
	    t13t6 "widget/tab"
		{
		x = 260; y = 380;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t13t62;
		tab_height = 8;

		t13t61 "widget/tabpage" { title = "First";      t13t61l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t13t62 "widget/tabpage" { title = "Looong Tab"; t13t62l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t13t63 "widget/tabpage" { title = "S";          t13t63l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t13t64 "widget/tabpage" { title = "Last Tab";   t13t64l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t13t6c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t13t1;
		    TabIndex = runclient(:t13t6:selected_index);
		    }
		}
	    }

	    
	t14 "widget/tabpage"
	    {
	    title = "Selection Offsets";

	    t14l0 "widget/label" { x=20; y=20; width=400; height=40; font_size=24; text="Selection Offsets"; }
	    
	    t14l1 "widget/label" { x=30; y=75; width=220; height=40; font_size=16; style="bold"; text="Default"; }
	    t14t1 "widget/tab"
		{
		x = 20; y = 100;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;

		t14t11 "widget/tabpage" { title = "First";      t14t11l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t14t12 "widget/tabpage" { title = "Looong Tab"; t14t12l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t14t13 "widget/tabpage" { title = "S";          t14t13l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t14t14 "widget/tabpage" { title = "Last Tab";   t14t14l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t14t1c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t14t2;
		    TabIndex = runclient(:t14t1:selected_index);
		    }
		}
	    
	    t14l2 "widget/label" { x=270; y=75; width=220; height=40; font_size=16; style="bold"; text="No Translation"; }
	    t14t2 "widget/tab"
		{
		x = 260; y = 100;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t14t22;
		select_translate_along = 0;
		select_translate_out = 0;

		t14t21 "widget/tabpage" { title = "First";      t14t21l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t14t22 "widget/tabpage" { title = "Looong Tab"; t14t22l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t14t23 "widget/tabpage" { title = "S";          t14t23l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t14t24 "widget/tabpage" { title = "Last Tab";   t14t24l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t14t2c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t14t3;
		    TabIndex = runclient(:t14t2:selected_index);
		    }
		}
	    
	    t14l3 "widget/label" { x=30; y=215; width=220; height=40; font_size=16; style="bold"; text="Along 8"; }
	    t14t3 "widget/tab"
		{
		x = 20; y = 240;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;
		select_translate_along = 8;
		select_translate_out = 0;

		t14t31 "widget/tabpage" { title = "First";      t14t31l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t14t32 "widget/tabpage" { title = "Looong Tab"; t14t32l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t14t33 "widget/tabpage" { title = "S";          t14t33l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t14t34 "widget/tabpage" { title = "Last Tab";   t14t34l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t14t3c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t14t4;
		    TabIndex = runclient(:t14t3:selected_index);
		    }
		}
	    
	    t14l4 "widget/label" { x=270; y=215; width=220; height=40; font_size=16; style="bold"; text="Out 8"; }
	    t14t4 "widget/tab"
		{
		x = 260; y = 240;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t14t42;
		select_translate_along = 0;
		select_translate_out = 8;

		t14t41 "widget/tabpage" { title = "First";      t14t41l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t14t42 "widget/tabpage" { title = "Looong Tab"; t14t42l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t14t43 "widget/tabpage" { title = "S";          t14t43l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t14t44 "widget/tabpage" { title = "Last Tab";   t14t44l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t14t4c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t14t5;
		    TabIndex = runclient(:t14t4:selected_index);
		    }
		}
	    
	    t14l5 "widget/label" { x=30; y=355; width=30; height=40; font_size=16; style="bold"; text="Along -8"; }
	    t14l5a "widget/label" { x=67; y=357; width=100; height=40; font_size=12; style="bold"; fgcolor="red"; text="(Negative values not recomended)"; }
	    t14t5 "widget/tab"
		{
		x = 20; y = 380;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected_index = 2;
		select_translate_along = -8;
		select_translate_out = 0;

		t14t51 "widget/tabpage" { title = "First";      t14t51l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab One"; } }
		t14t52 "widget/tabpage" { title = "Looong Tab"; t14t52l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Two"; } }
		t14t53 "widget/tabpage" { title = "S";          t14t53l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Three"; } }
		t14t54 "widget/tabpage" { title = "Last Tab";   t14t54l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Tab Four"; } }

		t14t5c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t14t6;
		    TabIndex = runclient(:t14t5:selected_index);
		    }
		}
	    
	    t14l6 "widget/label" { x=270; y=355; width=30; height=40; font_size=16; style="bold"; text="Out -8"; }
	    t14l6a "widget/label" { x=307; y=357; width=100; height=40; font_size=12; style="bold"; fgcolor="red"; text="(Negative values not recomended)"; }
	    t14t6 "widget/tab"
		{
		x = 260; y = 380;
		width = runserver(:this:w); height = runserver(:this:h);
		tab_location = runserver(:this:tloc);
		tab_width = 80;
		
		background = "/sys/images/slate2.gif";
		inactive_background = "/sys/images/slate2_dark.gif";
		selected = t14t62;
		select_translate_along = 0;
		select_translate_out = -8;

		t14t61 "widget/tabpage" { title = "First";      t14t61l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label One"; } }
		t14t62 "widget/tabpage" { title = "Looong Tab"; t14t62l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Two"; } }
		t14t63 "widget/tabpage" { title = "S";          t14t63l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Three"; } }
		t14t64 "widget/tabpage" { title = "Last Tab";   t14t64l "widget/label" { x=10; y=10; width=100; height=32; font_size=18; fgcolor="#00065f"; text="Label Four"; } }

		t14t6c "widget/connector"
		    {
		    event = TabChanged;
		    action = SetTab;
		    target = t14t1;
		    TabIndex = runclient(:t14t6:selected_index);
		    }
		}
	    }
	}
    }