$Version=2$
index "widget/page"
    {
    title = "App Navigator";
    width=800;
    height=600;
    widget_template = "/apps/nav/default.tpl";

    nav_cmp "widget/component"
	{
	path = "/apps/nav/nav.cmp";
	x=10; y=10; width=780; height=580;
	}
    }
