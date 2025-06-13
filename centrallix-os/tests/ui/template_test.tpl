// Minsik Lee 2025
$Version=2$

template_test "widget/template"
{
  imgBtn "widget/imagebutton"
  {
    widget_class = "Prevbtn";
    width=18;
    height=18;
    image="/sys/images/ico16aa.gif";
    pointimage="/sys/images/ico16ab.gif";
    clickimage="/sys/images/ico16ac.gif";
    disabledimage="/sys/images/ico16ad.gif";
  }
  imgBtn2 "widget/imagebutton"
  {
    widget_class = "Nextbtn";
    width=18;
    height=18;
    image="/sys/images/ico16da.gif";
    pointimage="/sys/images/ico16db.gif";
    clickimage="/sys/images/ico16dc.gif";
    disabledimage="/sys/images/ico16dd.gif";
  }

    closebtn "widget/imagebutton"
  {
    widget_class = "Closebtn";
    width=18;
    height=18;
    image="/sys/images/01bigclose.gif";
    clickimage="/sys/images/02bigclose.gif";
    disabledimage="/sys/images/ico16ad.gif";
  }
}
