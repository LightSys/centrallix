// Demonstrates that widget/menu does not respond to a window resize.
//
// mn_init() in htdrv_menu.js measures the menu once, at load: act_w, act_h,
// and the per-item positions in coords[].  The menu's geometry is now flexible
// CSS, so it changes size when the window does, but nothing re-measures it.
//
// To see it, load this page, then make the browser window WIDER and hover the
// items called out below.  The highlight bar is drawn from the stale numbers.

$Version=2$
in_main "widget/page"
  {
    bgcolor="#195173";
    width=640; height=480;

    // Case 1: onright items in a full width horizontal menu.
    // The right hand group is drawn in a right aligned cell, so widening the
    // window moves those items but not their entry in coords[].  Hover "Help"
    // or "About" and the highlight bar sits where they used to be.
    bar "widget/menu"
      {
        x=0; y=0;
        width=640;
        bgcolor="#afafaf";
        highlight_bgcolor="#ff0000";
        active_bgcolor="#ff8080";

        bar_file "widget/menuitem" { label="File"; }
        bar_edit "widget/menuitem" { label="Edit"; }
        bar_view "widget/menuitem" { label="View"; }
        bar_help "widget/menuitem" { label="Help"; onright=yes; }
        bar_about "widget/menuitem" { label="About"; onright=yes; }
      }

    l1 "widget/label"
      {
        x=10; y=40;
        width=600;
        value="1) Widen the window, then hover Help or About in the bar above.";
        font_size=14;
        fgcolor="white";
      }

    // Case 2: a vertical menu whose width flexes.
    // mn_additem() sizes the highlight bar from act_w, so the bar keeps the
    // width the menu had at load.  Widen the window and the bar no longer
    // reaches the right edge of the menu.
    side "widget/menu"
      {
        x=10; y=90;
        width=400;
        row_height=18;
        direction=vertical;
        popup=no;
        bgcolor="#afafaf";
        highlight_bgcolor="#ff0000";
        active_bgcolor="#ff8080";

        side_a "widget/menuitem" { label="First item"; }
        side_b "widget/menuitem" { label="Second item"; }
        side_c "widget/menuitem" { label="Third item"; }
        side_d "widget/menuitem" { label="Fourth item"; }
      }

    l2 "widget/label"
      {
        x=10; y=210;
        width=600;
        value="2) Widen the window, then hover an item in the menu above.";
        font_size=14;
        fgcolor="white";
      }

    l3 "widget/label"
      {
        x=10; y=240;
        width=600;
        value="Both bars are red so the offset is easy to see. Reload to reset.";
        font_size=14;
        fgcolor="white";
      }
  }
