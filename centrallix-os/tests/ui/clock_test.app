// Minsik Lee May 2025 
// NOTE
// - moveable, bold property found on sample app but missing in document.

$Version=2$
clock_test "widget/page"
{
  title = "Dropdown Test Application";
  background = "/sys/images/slate2.gif";
  x=0; y=0; width=600; height=800;

  clock_label "widget/label"
  {
    width = 400;
    align = "center";
    font_size = 24;
    fgcolor = "blue";
    style = "bold";
    value = "Clock Label";
  }

  clock "widget/clock"
  {
    ampm="yes";
    x=65; y=15; width=80; height=40;
  	bgcolor="white";
    shadowed="true";
    fgcolor1="#dddddd";
    fgcolor2="black";
    shadowx = 2;
    shadowy = 2;
    size=5;
    // moveable="true";
    // bold="true";
    cn_mouse_down "widget/connector"
    {
      action="SetValue";
      event="MouseDown";
      target="clock_label";
      Value = "Mouse Down";
    }
    cn_mouse_up "widget/connector"
    {
      action="SetValue";
      event="MouseUp";
      target="clock_label";
      Value = "Mouse Up";
    }
    cn_mouse_over "widget/connector"
    {
      action="SetValue";
      event="MouseOver";
      target="clock_label";
      Value = "Mouse Over";
    }
    cn_mouse_move "widget/connector"
    {
      action="SetValue";
      event="MouseMove";
      target="clock_label";
      Value = "Mouse Move";
    }
	}
}
