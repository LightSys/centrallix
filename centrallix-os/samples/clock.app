$Version=2$
main "widget/page"
    {
    bgcolor="#213e87";

    clock1 "widget/clock"
	{
	x=65; y=15; width=80; height=20;
//	bgcolor="white";
	shadowed="true";
	fgcolor1="#dddddd";
	fgcolor2="black";
	shadowx = 2;
	shadowy = 2;
	size=1;
	moveable="true";
	bold="true";
	}
    clock2 "widget/clock"
	{
	background="/sys/images/fade_pixelate_01.gif";
	x=15; y=55; width=80; height=20;
	fgcolor1="white";
	moveable="true";
	bold="true";
	}
    clock3 "widget/clock"
	{
	x=115; y=255; width=80; height=20;
	bgcolor="#cccccc";
	shadowed="true";
	fgcolor1="orange";
	fgcolor2="#666666";
//	size=3;
	moveable="true";
	bold="true";
	}
    }
