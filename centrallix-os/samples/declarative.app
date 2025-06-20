$Version=2$
declarative_test "widget/page"
    {
    width=800;
    height=600;
    bgcolor="#c0c0c0";
    title = "Test of Declarative Expression";

    btn "widget/textbutton"
	{
	x=10; y=10; width=200; height=24;
	bgcolor=white;
	text = "Push Me";
	tristate = no;
	enabled = runclient(substring(:eb:content,1,3) = "Yes");
	}

    eb "widget/editbox"
	{
	x=10; y=40; width=200; height=24;
	bgcolor=white;
	}
    }
