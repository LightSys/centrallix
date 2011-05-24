$Version=2$
index "widget/page"
    {
    title = "Welcome to Centrallix 0.9.1";
    bgcolor = "#ffffff";
    height = 500;
    width = 620;

    cximg "widget/image"
	{
	x=120; y=10; height=66; width=374; 
	source="/sys/images/centrallix_374x66.png";
	}

    cxlabel "widget/label"
	{
	x=16; y=80; height=66; width=588;
	align=center;
	fontsize=4;
	text = "Welcome to Centrallix 0.9.1, released in September 2010.  If you're seeing this page for the first time after an installation, congratulations - you've just successfully finished the install!  Below are a few links to get you started.";
	}

    count_em "widget/repeat"
	{
	sql = "select cnt = count(1) from object wildcard '/apps/*/app_info.struct'";
	pnOptions "widget/pane"
	    {
	    x=16;y=150;width=588;
	    height=runserver(2 + 10 + 30 + (50 * :count_em:cnt));
	    bgcolor="#c0c0c0";
	    style=raised;

	    apps_vbox "widget/vbox"
		{
		x=10; y=10; width=566;
		height = runserver((50 * :count_em:cnt) - 16 + 36);
		spacing = 10;

		apps_label "widget/label"
		    {
		    height = 18;
		    font_size = 16;
		    style = bold;
		    align = center;
		    text = "Installed Applications:";
		    }

		apps_rpt "widget/repeat"
		    {
		    sql = "select file = :cx__pathpart2, :app_name, :app_info from object wildcard '/apps/*/app_info.struct'";

		    oneapp_hbox "widget/hbox"
			{
			height = 40;
			spacing = 10;

			btn "widget/textbutton"
			    {
			    width=140;
			    fl_width = 0;
			    tristate=no;
			    background="/sys/images/grey_gradient.png";
			    fgcolor1=black; fgcolor2=white;
			    text = runserver(:apps_rpt:app_name);

			    onClick "widget/connector"
				{
				event = "Click";
				target = index;
				action = "LoadPage";
				Source = runserver("/apps/" + :apps_rpt:file + "/");
				}
			    }

			lblSamples "widget/label"
			    {
			    width=400;
			    font_size=13;
			    text = runserver(:apps_rpt:app_info);
			    }
			}
		    }
		}
	    }
	}

    lblLicense "widget/label"
	{
	x=16; y=440; width=588; height=50;
	text="Centrallix 0.9.1 is Free Software, released under the GNU GPL version 2, or at your option any later version published by the Free Software Foundation.  Centrallix 0.9.1 is provided with ABSOLUTELY NO WARRANTY.  See the file COPYING, in the accompanying documentation, for details.";
	fontsize=2;
	}
    }

