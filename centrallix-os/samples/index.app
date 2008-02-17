$Version=2$
index "widget/page"
    {
    title = "Index of Sample Applications";
    bgcolor = "#ffffff";
    x=0; y=0; width=630; height=480;
    
    cximg "widget/image"
	{
	x=10; y=4; height=66; width=374; 
	source="/sys/images/centrallix_374x66.png";
	}
    cxlabel "widget/label"
	{
	x=414; y=10; height=66; width=196; 
	text="Sample Applications, Reports, etc."; 
	fontsize=5;
	align=center;
	}

    mainpane "widget/pane"
	{
	x=10;y=70;height=400;width=610; 
	style=raised; 
	//bgcolor="#ababfc";
	bgcolor="#e0e0e0";

	treepane "widget/pane"
	    {
	    x=10;y=10;height=378;width=200;
	    style=lowered;
	    bgcolor="#ffffff";

	    tree_scroll "widget/scrollpane"
		{
		x=0;y=0;height=376;width=198;

		samples_tree "widget/treeview"
		    {
		    x=0;y=0;width=178;
		    show_branches=yes;
		    show_root=no;
		    show_root_branch = yes;
		    use_3d_lines=no;
		    source="/samples/samples.qyt/";

		    tree_click "widget/connector"
		    	{
		    	event="ClickItem";
		    	target=info_html;
		    	action="LoadPage";
		    	Source=runclient(:Pathname + '?ls__type=text%2fplain');
		    	}
		    tree_click2 "widget/connector"
			{
		    	event="ClickItem";
		    	target=ebLaunch;
		    	action="SetValue";
		    	Value=runclient(:Pathname);
			}
		    }
		}
	    }

	infopane "widget/pane"
	    {
	    x=220;y=40;height=348;width=380;
	    style=lowered;
	    bgcolor="#ffffff";

	    info_scroll "widget/scrollpane"
		{
		x=0;y=0;height=346;width=378;

		info_html "widget/html"
		    {
		    x=1;y=0;width = 356;
		    mode=dynamic;
		    source="/samples/welcome.html";
		    }
		}
	    }

	btnLaunch "widget/textbutton"
	    {
	    x=520;y=10;height=20;width=80;
	    text = "Launch...";
	    tristate=no;
	    background="/sys/images/grey_gradient.png";
	    fgcolor1=black; fgcolor2=white;

	    btn_click "widget/connector"
		{
		event="Click";
		target=index;
		action="Launch";
		Width=runclient(640);
		Height=runclient(480);
		Source=runclient(:ebLaunch:content);
		}
	    }
	ebLaunch "widget/editbox"
	    {
	    x=220;y=10;height=20;width=290;
	    style=lowered;
	    bgcolor=white;
	    }
	}
    }

