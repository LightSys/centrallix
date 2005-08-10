Filemanager "widget/page"
    {
    title = "DHTML Filemanager by Centrallix"
    bgcolor = "#c0c0c0"
    x=0 y=0 width=705 height=517
    
    Treeview_pane "widget/pane"
        {
	x=8 y=8 width=200 height=500
	bgcolor="#e0e0e0"
	style=lowered
	Tree_scroll "widget/scrollpane"
	    {
	    x=0 y=0 width=198 height=498
	    Tree "widget/treeview"
	        {
		x=0 y=1 width=178
		source = "/"
		}
	    }
	}
    Icons_pane "widget/pane"
        {
	bgcolor="#e0e0e0"
	x=216 y=8 width=480 height=500
	style=lowered
	Icons_scroll "widget/scrollpane"
	    {
	    x=0 y=0 width=478 height=498
	    }
	}
    }
