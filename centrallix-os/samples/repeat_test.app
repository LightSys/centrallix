$Version=2$
in_main "widget/page" {
    x=0;y=0;width=1000;height=1000;
    vbox "widget/vbox"{
	x=10;y=10;width=100;height=800;
	cellsize=20;
	spacing=0; 
	rpt "widget/repeat"
	    {
	    sql="SELECT :name from /samples WHERE substring(:name,1,1)=='t'";
	    testbutton "widget/textbutton"
		{
		width=100;height=20;
		text=runserver(:rpt:name);
		}
	    }
	btn1 "widget/textbutton"
	    {
	    width=100;height=30;
	    text="hello";
	    }
	rpt2 "widget/repeat"
	    {
	    sql="SELECT :name from /samples WHERE substring(:name,1,1)=='d'";
	    othertestbutton "widget/textbutton"
		{
		width=100;height=20;
		text=runserver(:rpt2:name);
		}
	    }
    }
}
