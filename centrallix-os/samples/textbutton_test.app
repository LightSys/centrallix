$Version=2$
textbutton_test "widget/page"
    {
    background="/sys/images/test_background.png";

    enabbutton "widget/textbutton"
        {
	x=8; y=8; width=100; height=32;
	text="Button 1";
	tristate=no;
	enabled=yes;
	}
    disabbutton "widget/textbutton"
        {
	x=8; y=64+ 8; width=100; height=32;
	text="Button 2";
	tristate=no;
	enabled=no;
	}
    colorbutton "widget/textbutton"
	{
	x=8; y=128+ 8; width=100; height=32;
	text="Button 3";
	tristate=no;
	enabled=yes;
	bgcolor="#c0c0c0";
	}
    revcolorbutton "widget/textbutton"
	{
	x=8; y=192 + 8; width=100; height=32;
	text="Button 4";
	tristate=no;
	enabled=yes;
	fgcolor1=black; fgcolor2=white;
	bgcolor="#c0c0c0";
	}
    imgbutton "widget/textbutton"
	{
	x=8; y=256 + 8; width=100; height=32;
	text="Button 5";
	tristate=no;
	enabled=yes;
	fgcolor1=black; fgcolor2=white;
	background="/sys/images/test_background_light.png";
	}
    tributton "widget/textbutton"
	{
	x=128; y=8; width=100; height=32;
	text="Button 6";
	tristate=yes;
	enabled=yes;
	}
    disabtributton "widget/textbutton"
	{
	x=128; y=64 + 8; width=100; height=32;
	text="Button 7";
	tristate=yes;
	enabled=no;
	}
    colortributton "widget/textbutton"
	{
	x=128; y=128+ 8; width=100; height=32;
	text="Button 8";
	tristate=yes;
	enabled=yes;
	bgcolor="#e0e0e0";
	}
    imgtributton "widget/textbutton"
	{
	x=128; y=192 + 8; width=100; height=32;
	text="Button 9";
	tristate=yes;
	enabled=yes;
	fgcolor1=black; fgcolor2=white;
	background="/sys/images/test_background_light.png";
	}
    lotsalinesbutton "widget/textbutton"
	{
	x=128; y=256 + 8; width=100; height=32;
	text="This is a lot of text for a text button";
	tristate=no;
	enabled=yes;
	fgcolor1=black; fgcolor2=white;
	background="/sys/images/test_background_light.png";
	}
    autoheight "widget/textbutton"
	{
	x=248; y=8; width=100;
	text = "AutoHeight1";
	tristate=no;
	enabled=yes;
	bgcolor="#c0c0c0";
	}
    autoheightdis "widget/textbutton"
	{
	x=248; y=8 + 64; width=100;
	text = "AutoHeightD";
	tristate=no;
	enabled=no;
	bgcolor="#c0c0c0";
	}
    autoheighttri "widget/textbutton"
	{
	x=248; y=8 + 128; width=100;
	text = "AutoHeightT";
	tristate=yes;
	enabled=yes;
	bgcolor="#c0c0c0";
	}
    autoheightbig "widget/textbutton"
	{
	x=248; y=8 + 192; width=100;
	text = "AutoHeight Big Button With Lots Of Text To Fit In It";
	tristate=no;
	enabled=yes;
	bgcolor="#c0c0c0";
	}
    }
