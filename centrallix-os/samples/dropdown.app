$Version=2$
in_main "widget/page" {
    bgcolor="#195173";

    alerter "widget/alerter" {}
    form1 "widget/form" {
	dda "widget/dropdown" {
	    x=15;y=15;
	    width=120;
	    bgcolor="#afafaf";
	    hilight="#cfcfcf";
	    mode='static';

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
    }
}
