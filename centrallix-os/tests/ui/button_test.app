// Minsik Lee May 2025
//    NOTE
// -  textoverImgButton seems to be not working and 
//    other buttons stop working when added on the page.
//
// -  Button with text type shows the clickimage only when clicked,
//    but the rest of the buttons show both image and clickimage when clicked.

$Version=2$
checkbox_test "widget/page"
{
    title = "Button Test Application";
    background = "/sys/images/slate2.gif";
    x = 0; y = 0; height = 800; width = 900;

    vbox "widget/vbox"
    {
        x = 10; y = 10; height = 600;
        spacing = 20;
        title = "Buttons";

        statusLabel "widget/label"
        {
          value = "status";
          font_size = 16;
          style = bold;
        }

        hbox0 "widget/hbox"
        {
          height = 30;
          spacing = 20;
          title = "Image Buttons";

          textButton "widget/button"
          {
            height = 20; width = 60;
            type = "text";
            text = "text button";
            bgcolor = "blue";
            fgcolor1 = "white";
            fgcolor2 = "red";
            disable_color = "grey";
            enabled = yes;

            cn1 "widget/connector"
              {
                action = "SetValue";
                event = "Click";
                target = "statusLabel";
                Value = "text button clicked";
              }
          }

          textButton2 "widget/button"
          {
            height = 20; width = 60;
            type = "text";
            text = "disabled text button";
            bgcolor = "blue";
            fgcolor1 = "white";
            fgcolor2 = "red";
            disable_color = "grey";
            enabled = no;
          }
        }

        hbox1 "widget/hbox"
        {
          height = 70;
          spacing = 10;
          title = "Image Buttons";

          imgButton "widget/button"
          {
            height = 80; width = 80;
            type = "image";
            enabled = yes;
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";

            cn2 "widget/connector"
            {
              action = "SetValue";
              event = "Click";
              target = "statusLabel";
              Value = "image button clicked";
            }
          }

          imgButton2 "widget/button"
          {
            height = 80; width = 80;
            type = "image";
            enabled = no;
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";
          }
        }
        
        hbox2 "widget/hbox"
        {
          height = 70;
          spacing = 10;
          title = "Top Image Buttons";

          topImgButton "widget/button"
          {
            height = 80; width = 80;
            type = "topimage";
            enabled = yes;
            text = "top image button";
            fgcolor1 = "white";
            fgcolor2 = "black";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";

            cn3 "widget/connector"
            {
              action = "SetValue";
              event = "Click";
              target = "statusLabel";
              Value = "top image button clicked";
            }
          }

          topImgButton2 "widget/button"
          {
            height = 80; width = 80;
            type = "topimage";
            tristate = no;
            enabled = no;
            text = "disabled top image button";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";
          }
        }

        hbox3 "widget/hbox"
        {
          height = 70;
          spacing = 10;
          title = "Right Image Buttons";
        
          rightImgButton "widget/button"
          {
            height = 80; width = 80;
            type = "rightimage";
            enabled = yes;
            tristate = yes;
            text = "right image button";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";

            cn4 "widget/connector"
            {
              action = "SetValue";
              event = "Click";
              target = "statusLabel";
              Value = "right image button clicked";
            }
          }
          
          rightImgButton2 "widget/button"
          { 
            height = 80; width = 80;
            type = "rightimage";
            enabled = no;
            tristate = no;
            text = "disabled right image button";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";
          }
        }

        hbox4 "widget/hbox"
        {
          height = 70;
          spacing = 10;
          title = "Left Image Buttons";

          leftImgButton "widget/button"
          {
            height = 80; width = 80;
            type = "leftimage";
            text = "left image button";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";

            cn5 "widget/connector"
            {
              action = "SetValue";
              event = "Click";
              target = "statusLabel";
              Value = "left image button clicked";
            }
          }

          leftImgButton2 "widget/button"
          {
            height = 80; width = 80;
            type = "leftimage";
            enabled = no;
            tristate = no;
            text = "disabled left image button";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";
          }
        }

        hbox5 "widget/hbox"
        {
          height = 70;
          spacing = 10;
          title = "Bottom Image Buttons";

          bottomImgButton "widget/button"
          {
            height = 80; width = 80;
            type = "bottomimage";
            text = "bottom image button";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";

          cn6 "widget/connector"
            {
              action = "SetValue";
              event = "Click";
              target = "statusLabel";
              Value = "bottom image button clicked";
            }
          }

          bottomImgButton2 "widget/button"
          {
            height = 80; width = 80;
            type = "bottomimage";
            enabled = no;
            tristate = no;
            text = "disabled bottom image button";
            clickimage = "/sys/images/wait_spinner.gif";
            disabledimage = "/sys/images/dotted_check.gif";
            image = "/sys/images/grey_check.gif";
            pointimage = "/sys/images/green_check.gif";
          }
        }

        // hbox6 "widget/hbox"
        // {
        //   height = 70;
        //   spacing = 10;
        //   title = "Text Over Image Buttons";

        //   textoverImgButton "widget/button"
        //   {
        //     height = 80; width = 80;
        //     type = "textoverimage";
        //     text = "text over image button";
        //     clickimage = "/sys/images/wait_spinner.gif";
        //     disabledimage = "/sys/images/dotted_check.gif";
        //     image = "/sys/images/grey_check.gif";
        //     pointimage = "/sys/images/green_check.gif";
        //   }
        // }
    }
}
