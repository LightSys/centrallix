$Version=2$
component "widget/page"
    {
    // Application title
    title = "Example #5: Components";

    // Background color for the application
    bgcolor = "#c0c0c0";

    // Design geometry, in pixels
    width = 1000;
    height = 600;

    // Instantiating a component
    cmp1 "widget/component"
	{
	x=10; y=10; width=200; height=32;
	path="component.cmp";
	emptytext="Click Me!";
	}

    // Instantiating a component a second time
    cmp2 "widget/component"
	{
	x=10; y=52; width=400; height=32;
	path="component.cmp";
	emptytext="Click Me Too!";
	}
    }
