$Version=2$
in_main "widget/page" {
    bgcolor="#195173";

    form1 "widget/form" {
	dda "widget/dropdown" {
	    x=15;y=15;
	    width=120;
	    bgcolor="#afafaf";
	    hilight="#cfcfcf";

	    ddb "widget/dropdownitem" {
	       label="Trees";
	       value="1";
	    }
	    ddc "widget/dropdownitem" {
	       label="Flowers";
	       value="2";
	    }
	    ddd "widget/dropdownitem" {
	       label="Apples";
	       value="2";
	    }
	    dde "widget/dropdownitem" {
	       label="Oranges";
	       value="2";
	    }
	}

	dd "widget/dropdown" {
	    x=180;y=15;
	    width=75;
	    bgcolor="#afafaf";
	    hilight="#cfcfcf";

	    dd1 "widget/dropdownitem" {
	       label="Male";
	       value="1";
	    }
	    dd2 "widget/dropdownitem" {
	       label="Female";
	       value="2";
	    }
	}
    }
}
