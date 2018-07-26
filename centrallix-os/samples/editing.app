$Version=2$
editing "widget/page"
    {
    width=900;
    height=600;
    title="File Editing Test App";
    bgcolor="#334466";

    hbox "widget/hbox"
	{
	x=10; y=10;
	width=880; height=580;
	spacing=10;

	left_vbox "widget/vbox"
	    {
	    width=435;
	    spacing=10;

	    text_osrc "widget/osrc"
		{
		sql = "select file_content = :objcontent from object /samples/testfile.txt";
		autoquery=onload;

		text_form "widget/form"
		    {
		    textarea "widget/textarea"
			{
			height=538;
			bgcolor=white;
			
			}

		    textarea_ctls "widget/hbox"
			{
			height=32;
			align=center;

			save_btn "widget/textbutton"
			    {
			    height=32;
			    width=100;
			    bgcolor="#6080c0";
			    fgcolor1=white;
			    fgcolor2=black;
			    text="Save";
			    tristate=no;
			    border_radius=8;
			    enabled=runclient(:text_form:is_savable);

			    save_text_on_click "widget/connector"
				{
				event=Click;
				target=text_form;
				action=Save;
				}
			    }
			}
		    }
		}
	    }

	right_vbox "widget/vbox"
	    {
	    width=435;
	    spacing=10;

	    html_osrc "widget/osrc"
		{
		sql = "select file_content = :objcontent from object /samples/testfile.html";
		autoquery=onload;

		html_form "widget/form"
		    {
		    richtext "widget/richtextedit"
			{
			height=538;
			bgcolor=white;
			fieldname=file_content;
			}

		    richtext_ctls "widget/hbox"
			{
			height=32;
			align=center;

			save_html_btn "widget/textbutton"
			    {
			    height=32;
			    width=100;
			    bgcolor="#6080c0";
			    fgcolor1=white;
			    fgcolor2=black;
			    text="Save";
			    tristate=no;
			    border_radius=8;
			    enabled=runclient(:html_form:is_savable);

			    save_html_text_on_click "widget/connector"
				{
				event=Click;
				target=html_form;
				action=Save;
				}
			    }
			}
		    }
		}
	    }
	}
    }
