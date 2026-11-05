// This indicates that we're using a v2 structure file.
$Version=2$

// Main page widget is at top-level.
radiobutton_test "widget/page" {
    background="/sys/images/slate2.gif";
    x=0; y=0; width=200; height=120;
    
    testradio "widget/radiobuttonpanel" {
	x=20;
	y=20;
	width=150;
	height=80;
	spacing=10;
	title="test";
	bgcolor="#e0e0e0";
	outline_bgcolor="black";

	label1 "widget/radiobutton" {
	    label="basketball";
	    selected="true";
	}
	label2 "widget/radiobutton" {
	    label="is fun";
	}
	label3 "widget/radiobutton" {
	    label="and stuff";
	}
	label4 "widget/radiobutton" {
	    label="yeah!";
	}
    }
}
