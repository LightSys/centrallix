$Version=2$

// Scratch test app: verify that a tab control's background image tiles
// CONTINUOUSLY across the tab strip and the tab page body, with no visible
// seam or phase shift at the tab/body boundary. One tab control per
// tab_location value (top, bottom, left, right).

tab_bg_scratch "widget/page"
    {
    x = 0; y = 0;
    width = 800; height = 600;
    bgcolor = "#e8e8e8";

    TabTop "widget/tab"
	{
	x = 20; y = 20; width = 320; height = 220;
	tab_location = top;
	background = "/sys/images/test_background.png";
	inactive_bgcolor = "#204060";

	TabTopP1 "widget/tabpage" { title = "Page 1"; TabTopL1 "widget/label" { x=10; y=10; width=200; height=24; text="Top: Page One"; } }
	TabTopP2 "widget/tabpage" { title = "Page 2"; TabTopL2 "widget/label" { x=10; y=10; width=200; height=24; text="Top: Page Two"; } }
	TabTopP3 "widget/tabpage" { title = "Page 3"; TabTopL3 "widget/label" { x=10; y=10; width=200; height=24; text="Top: Page Three"; } }
	}

    TabBottom "widget/tab"
	{
	x = 420; y = 20; width = 320; height = 220;
	tab_location = bottom;
	background = "/sys/images/test_background.png";
	inactive_bgcolor = "#204060";

	TabBottomP1 "widget/tabpage" { title = "Page 1"; TabBottomL1 "widget/label" { x=10; y=10; width=200; height=24; text="Bottom: Page One"; } }
	TabBottomP2 "widget/tabpage" { title = "Page 2"; TabBottomL2 "widget/label" { x=10; y=10; width=200; height=24; text="Bottom: Page Two"; } }
	TabBottomP3 "widget/tabpage" { title = "Page 3"; TabBottomL3 "widget/label" { x=10; y=10; width=200; height=24; text="Bottom: Page Three"; } }
	}

    TabLeft "widget/tab"
	{
	x = 20; y = 280; width = 320; height = 220;
	tab_location = left;
	tab_width = 70;
	background = "/sys/images/test_background.png";
	inactive_bgcolor = "#204060";

	TabLeftP1 "widget/tabpage" { title = "Page 1"; TabLeftL1 "widget/label" { x=10; y=10; width=200; height=24; text="Left: Page One"; } }
	TabLeftP2 "widget/tabpage" { title = "Page 2"; TabLeftL2 "widget/label" { x=10; y=10; width=200; height=24; text="Left: Page Two"; } }
	TabLeftP3 "widget/tabpage" { title = "Page 3"; TabLeftL3 "widget/label" { x=10; y=10; width=200; height=24; text="Left: Page Three"; } }
	}

    TabRight "widget/tab"
	{
	x = 420; y = 280; width = 320; height = 220;
	tab_location = right;
	tab_width = 70;
	background = "/sys/images/test_background.png";
	inactive_bgcolor = "#204060";

	TabRightP1 "widget/tabpage" { title = "Page 1"; TabRightL1 "widget/label" { x=10; y=10; width=200; height=24; text="Right: Page One"; } }
	TabRightP2 "widget/tabpage" { title = "Page 2"; TabRightL2 "widget/label" { x=10; y=10; width=200; height=24; text="Right: Page Two"; } }
	TabRightP3 "widget/tabpage" { title = "Page 3"; TabRightL3 "widget/label" { x=10; y=10; width=200; height=24; text="Right: Page Three"; } }
	}
    }
