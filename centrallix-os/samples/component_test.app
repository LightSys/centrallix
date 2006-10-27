$Version=2$
component_test "widget/page"
    {
    title = "Component System Test";
    bgcolor = "black";
    width=640; height=480;

    btn "widget/textbutton"
	{
	x=10;y=10;width=80;height=30;
	text = "Instantiate";
	fgcolor1=black; fgcolor2=white;
	background="/sys/images/grey_gradient.png";
	tristate = no;

	cn1 "widget/connector" { event="Click"; action="Instantiate"; target=win; }
	}
    btn2 "widget/textbutton"
	{
	x=10;y=50;width=80;height=30;
	text = "Destroy";
	fgcolor1=black; fgcolor2=white;
	background="/sys/images/grey_gradient.png";
	tristate = no;

	cn2 "widget/connector" { event="Click"; action="Destroy"; target=win; }
	}

    win "widget/component"
	{
	mode=dynamic;
	x=100;y=10;
	path="/samples/debugwin.cmp";
	}
    }
