$Version=2$
Filemanager "widget/page"
    {
    title = "DHTML DOM viewer by Centrallix";
    bgcolor = "#c0c0c0";
    Treeview_pane "widget/pane"
        {
	x=8; y=8; width=800; height=500;
	bgcolor="#e0e0e0";
	style=lowered;
	Tree_scroll "widget/scrollpane"
	    {
	    x=0; y=0; width=798; height=498;
	    Tree "widget/treeview"
	        {
		x=0; y=1; width=778;
		//if there is a javascript: in front, the DOM Viewer mode is used
		//  if not, it's a normal treeview widget
		//directly after the javascript:, put the object you want the root to be
		//   it must be globally accessible -- default is window
		source = "javascript:";
		}
	    }
	}
    }
