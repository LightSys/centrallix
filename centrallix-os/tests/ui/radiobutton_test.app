// David Hopkins June 2025

$Version=2$
RadioButtontest "widget/page"
{
    title = "Radiobutton Test Application";
    background = "/sys/images/slate2.gif";
    x = 0; y = 0; width = 300; height = 480;

	testradio "widget/radiobuttonpanel"
	{
		x = 20; y = 20;
		width = 250; height = 200;
		title = "Time complexity of Selection Sort";
		bgcolor = "#ffffff";
		querymode = "false"; // Prevents radiobuttons from becoming checkboxes in query mode

		rb1 "widget/radiobutton" { label = "O(n log n)"; selected = "true"; }
		rb2 "widget/radiobutton" { label = "O(n)"; }
		rb3 "widget/radiobutton" { label = "O(n^2)"; }
		rb4 "widget/radiobutton" { label = "O(log n)"; }
		rb5 "widget/radiobutton" { label = "O(1)"; }
		rb6 "widget/radiobutton" { label = "O(n^3)"; }
	}

	
}
