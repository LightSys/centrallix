$Version=2$
index "widget/page"
    {
    title = "Welcome to Centrallix";
    bgcolor = "#ffffff";
    width = 900;
    height = 600;
    font_name = "Arial";

    cximg "widget/image"
	{
	x=263; y=10; height=66; width=374; 
	source="/sys/images/centrallix_374x66.png";
	}

    cxlabel "widget/label"
	{
	x=16; y=80; height=66; width=868;
	align=center;
	fontsize=4;
	text = "Welcome to Centrallix, a platform for open and community developed applications.  If you're seeing this page for the first time after an installation, congratulations - you've just successfully finished the install!  Below are the applications currently installed on this server.";
	}

    count_em "widget/repeat"
	{
	sql = "select cnt = count(1), defaultcnt=count(condition(:app_default and :app_enable, :app_path, null)), defaultapp=first(condition(:app_default and :app_enable, :app_path, null)) from '/applist.qy'";

	on_load "widget/connector"
	    {
	    condition=runserver(:count_em:defaultapp is not null and :count_em:defaultcnt == 1);
	    source=index;
	    event=Load;
	    target=index;
	    action=LoadPage;
	    Source = runserver(:count_em:defaultapp);
	    }

	pnOptions "widget/pane"
	    {
	    x=32;y=150;width=836;
	    height=runserver(2 + 10 + 32 + 10 + (50 * :count_em:cnt) + 10);
	    bgcolor="#f8f8f8";
	    style=bordered;
	    border_color="#e0e0e0";
	    border_radius=4;
	    shadow_radius=3;
	    shadow_color="#a0a0a0";
	    shadow_angle=135;
	    shadow_offset=2;

	    apps_vbox "widget/vbox"
		{
		x=32; y=10; width=770;
		height = runserver(32 + 10 + (50 * :count_em:cnt));
		spacing = 10;

		apps_label "widget/label"
		    {
		    height = 32;
		    font_size = 18;
		    style = bold;
		    align = center;
		    text = "Installed Applications:";
		    fl_height = 0;
		    }

		apps_rpt "widget/repeat"
		    {
		    sql = "select * from /applist.qy";

		    oneapp_hbox "widget/hbox"
			{
			height = 40;
			spacing = 32;

			btn "widget/textbutton"
			    {
			    width=140;
			    fl_width = 0;
			    tristate=no;
			    background="/sys/images/grey_gradient.png";
			    fgcolor1=black; fgcolor2=white;
			    text = runserver(:apps_rpt:app_name);
			    enabled = runserver(:apps_rpt:app_enable);
			    border_radius = 16;
			    fl_height = 0;

			    onClick "widget/connector"
				{
				event = "Click";
				target = index;
				action = "LoadPage";
				Source = runserver(:apps_rpt:app_path);
				}
			    }

			lblSamples "widget/label"
			    {
			    width=604;
			    font_size=13;
			    text = runserver(:apps_rpt:app_info);
			    fl_height = 0;
			    }
			}
		    }
		}
	    }
	}

    lblLicense "widget/label"
	{
	x=16; y=540; width=868; height=50;
	text="Open Source Licensing and Legal Notes:  Centrallix is Free Software, released under the GNU GPL version 2, or at your option any later version published by the Free Software Foundation.  Centrallix is provided with ABSOLUTELY NO WARRANTY.  See the file COPYING, in the accompanying documentation, for details.";
	fontsize=2;
	align=center;
	}
    }

