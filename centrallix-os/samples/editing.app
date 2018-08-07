$Version=2$
editing "widget/page"
    {
    width=900;
    height=800;
    title="File Editing Test App";
    bgcolor="F0F0F0";

    hbox "widget/hbox"
	{
	x=10; y=10;
	width=2000; height=400;
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
                    next_form= html_form;
		    textarea "widget/textarea"
			{

                        //width=400;
			//height=760;
                        height=600;
			bgcolor=white;
                        fieldname=file_content;
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
                    next_form = text_form;

		    richtext "widget/richtextedit"
			{
                        //width = 400;
			//height=760;
                        height=600;
                        bgcolor=white;
                        form=html_form;
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
