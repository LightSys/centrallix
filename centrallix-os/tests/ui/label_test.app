//Minsik Lee May 2025 

$Version=2$
label_test "widget/page"
{
    title = "Label Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=600; height=800;

    vbox "widget/vbox"
    {
        x=10; y=10; height=300; width=250;
        spacing=10;

      title_label "widget/label"
        {
          width = 400;
          align = "center";
          font_size = 24;
          fgcolor = "blue";
          style = "bold";
          text = "Label Test Application";
        }

      title_label_left "widget/label"
        {
          width = 400;
          align = "left";
          font_size = 24;
          style = "bold";
          text = "Left Aligned";
        }

      title_label_right "widget/label"
        {
          width = 400;
          align = "right";
          font_size = 24;
          style = "bold";
          text = "Right Aligned";
        }
      link_label "widget/label"
        {
          width = 80;
          font_size = 18;
          fgcolor = "blue";
          point_fgcolor = "blue";
          click_fgcolor = "grey";
          text = "link label";
        }

      form1 "widget/form"
        {
          enter_mode = "save";

          checkbox_label "widget/label"
            {
              width = 400;
              form = "form1";
              text = "checkbox label";
            }
          checkbox "widget/checkbox"
            {
              form = "form1";
            }
          textarea_label "widget/label"
            {
              width = 400;
              form = "form1";
              text = "textarea label";
            }
          textarea "widget/textarea"
            {
              width = 400; height = 200;
            }
        }
    }
}