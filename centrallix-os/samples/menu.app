$Version=2$
in_main "widget/page" {
    bgcolor="#195173";

    alerter "widget/alerter" {}
    form1 "widget/form" {
	mn "widget/menu" {
	    x=15;y=15;
	    width=400;
	    bgcolor="#afafaf";
	    hilight="#cfcfcf";

	    mna "widget/menuitem" {
	        label="File";
	        value="1";
	        width="120";
		}
	    mnb "widget/menuitem" {
	        label="Edit";
	        value="2";
	        width="120";
		level="0";
	    }
	    mnc "widget/menuitem" {
	        label="View";
	        value="3";
	        width="120";
		level="0";
	    }
	}
    }
}



