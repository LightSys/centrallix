// Minsik Lee 2025
$Version=2$

template_test "widget/page"
{
  title="template test app";
  bgcolor="#c0c0c0"; 
  x=0; y=0; width=600; height=800;
  widget_template = "/tests/ui/template_test.tpl";

  prevbtn "widget/imagebutton"
  {
    x=10;y=10;
    widget_class = "Prevbtn";
  }

  nextbtn "widget/imagebutton"
  {
    x=40;y=10;
    widget_class = "Nextbtn";
  }

  closebtn "widget/imagebutton"
  {
    x=70;y=10;
    widget_class = "Closebtn";
  }
}
