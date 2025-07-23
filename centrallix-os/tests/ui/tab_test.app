// Minsik Lee May 2025 
// NOTE
// - selected_index displays both the first tab and indexth tab as selected
// - could not find any example of dynamic tab and get it to work
// - tab page with `visible=0` property is still visible

$Version=2$

tab_test "widget/page"
{
  title="tab test app";
  bgcolor="#c0c0c0"; 
  x=0; y=0; width=600; height=800;

  tab1 "widget/tab"
  {
    tab_location=top;
    x=10; y=10; width=300; height=100; 
    border_color="black"; border_radius=8; border_style="solid"; 
    shadow_angle=30; shadow_color="blue"; shadow_offset=6; shadow_radius=60;
    background="/sys/images/slate2.gif";
    inactive_background="/sys/images/slate2_dark.gif";


    tab1pg1 "widget/tabpage" 
    { 
      lbl11 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label One"; 
      } 
    }

    tab1pg2 "widget/tabpage" 
    { 
      lbl12 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Two"; 
      } 
    }

    tab1pg3 "widget/tabpage" 
    { 
      lbl13 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Three"; 
      } 
    }
  }

  tab2 "widget/tab"
  {
    tab_location=bottom;
    x=10; y=160; width=300; height=100; 
    background="/sys/images/slate2.gif";
    inactive_background="/sys/images/slate2_dark.gif";

    tab2pg1 "widget/tabpage" 
    { 
      lbl21 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label One"; 
      } 
    }

    tab2pg2 "widget/tabpage" 
    { 
      lbl22 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Two"; 
      } 
    }

    tab2pg3 "widget/tabpage" 
    { 
      lbl23 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Three"; 
      } 
    }
  }

  tab3 "widget/tab"
  {
    x=10; y=310; width=300; height=100; 
    tab_location=left; tab_width = 80;
    background="/sys/images/slate2.gif";
    inactive_background="/sys/images/slate2_dark.gif";

    tab3pg1 "widget/tabpage" 
    { 
      lbl31 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label One"; 
      } 
    }

    tab3pg2 "widget/tabpage" 
    { 
      lbl32 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Two"; 
      } 
    }

    tab3pg3 "widget/tabpage" 
    { 
      lbl33 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Three"; 
      } 
    }
  }

  tab4 "widget/tab"
  {
    x=10; y=460; width=300; height=100; 
    tab_location=right; tab_width = 80;
    background="/sys/images/slate2.gif";
    inactive_background="/sys/images/slate2_dark.gif";
    selected_index=2;

    tab4pg1 "widget/tabpage" 
    { 
      lbl41 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label One"; 
      } 
    }

    tab4pg2 "widget/tabpage" 
    { 
      lbl42 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Two"; 
      } 
    }

    tab4pg3 "widget/tabpage" 
    { 
      lbl43 "widget/label" 
      { 
        x=10; y=10; width=100; height=32; text="Label Three"; 
      } 
    }
  }

  osrc "widget/osrc"
  {
    sql="SELECT :Index,:Name FROM /tests/ui/tab_test.csv/rows";

    tab5 "widget/tab"
    {
      x=10; y=610; width=300; height=100; 
      background="/sys/images/slate2.gif";
      inactive_background="/sys/images/slate2_dark.gif";

      tab5pg1 "widget/tabpage" 
      { 
        type="dynamic"; fieldname="Name";
        lbl51 "widget/label" 
        { 
          x=10; y=10; width=100; height=32; text="Label One"; 
        } 
      }
    }
  }
}