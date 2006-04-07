$Version=2$
editbox_test2 "widget/page"
    {
    title = "Editbox Test Application";
    background = "/sys/images/test_background.png";
    x=0; y=0; width=300; height=480;
	
    eb "widget/editbox"
	{
	x=8; y=8; width=128; height=20;
	style=lowered;
	bgcolor="#e0e0e0";
	}

    eb_toosmall "widget/editbox"
	{
	x=8; y=8 + 64; width=128; height=1;
	style=lowered;
	bgcolor="#e0e0e0";
	}

    eb_toobig "widget/editbox"
	{
	x=8; y=8 + 128; width=128; height=40;
	style=lowered;
	bgcolor="#e0e0e0";
	}

    eb_raised "widget/editbox"
	{
	x=8; y=8 + 192; width=128; height=20;
	style=raised;
	bgcolor="#e0e0e0";
	}

    eb_readonly "widget/editbox"
	{
	x=8; y=8 + 256; width=128; height=20;
	style=lowered;
	readonly=yes;
	bgcolor="#e0e0e0";
	}

    //ebautoh "widget/editbox"
//	{
//	x=8; y=8 + 320; width=128;
//	style=lowered;
//	bgcolor="#e0e0e0";
//	}
    }
