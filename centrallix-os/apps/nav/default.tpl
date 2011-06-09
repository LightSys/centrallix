$Version=2$
default "widget/template"
    {
    tplPage "widget/page"
	{
	bgcolor="#e0e0e0";
	linkcolor="#0000ff";
	font_name = "Arial";
	font_size = 12;
	icon = "/favicon.ico";
	}
    tplMenuBar "widget/menu"
	{
	widget_class="bar";
	bgcolor = "#e0e0e0";
	fgcolor="#000000";
	highlight_bgcolor = "#808080";
	active_bgcolor = "#ffffff";
	direction = "horizontal";
	popup = false;
	height = 25;
	}
    tplMenuPopup "widget/menu"
	{
	widget_class="popup";
	bgcolor = "#e0e0e0";
	fgcolor="#000000";
	highlight_bgcolor = "#808080";
	active_bgcolor = "#ffffff";
	direction = "vertical";
	popup = true;
	}
    tplWin "widget/childwindow"
	{
	hdr_bgcolor="#c0c0c0";
	textcolor="#000000";
	bgcolor="#e0e0e0";
	style=dialog;
	}
    tplPopupWin "widget/childwindow"
	{
	widget_class = "popup";
	bgcolor="#e0e0e0";
	style = dialog;
	visible = false;
	}
    tpEb "widget/editbox"
	{
	bgcolor="white";
	description_fgcolor="#a0a0a0";
	}
    tplDt "widget/datetime"
	{
	bgcolor="white";
	}
    tplTab "widget/tab"
	{
	bgcolor="#e0e0e0";
	inactive_bgcolor="#c0c0c0";
	}
    tplTable "widget/table"
	{
	row1_bgcolor = "#ffffff";
	row2_bgcolor = "#f0f0f0";
	rowhighlight_bgcolor = "#0000a0";
	hdr_bgcolor = "#e0e0e0";
	textcolorhighlight = "#ffffff";
	textcolor = "#000000";
	newrow_bgcolor = "#ffff80";
	textcolornew = "black";
	rowheight = 18;
	mode = dynamicrow;
	colsep = 1;
	colsep_bgcolor = "#ffffff";
	}
    tplTablePane "widget/pane"
	{
	widget_class = "table_bgnd";
	style=lowered;
	bgcolor="#ffffff";
	}
    tplDataPane "widget/pane"
	{
	widget_class = "data";
	style=lowered;
	bgcolor="#ffffff";
	}
    tplLink "widget/label"
	{
	widget_class = "link";
	fgcolor = "#000080";
	point_fgcolor = "#0000ff";
	click_fgcolor = "#ffffff";
	}
    tplLabelPaneLabel "widget/label"
	{
	widget_class = "label";
	align=center;
	style=bold;
	fgcolor=white;
	y = -2;
	}
    tplLabelPane "widget/pane"
	{
	widget_class = "label";
	width=100;
	height=18;
	style=bordered;
	//border_color="white";
	border_color=black;
	bgcolor = "#a0a0a0";
	}
    tplGroupPane "widget/pane"
	{
	widget_class = "group";
	bgcolor = "#e0e0e0";
	style=raised;
	}
    tplButton "widget/textbutton"
	{
	bgcolor="#d0d0d0";
	fgcolor2="white";
	fgcolor1="black";
	tristate=no;
	width=100;
	height=24;
	}
    tplDropDown "widget/dropdown"
	{
	hilight = "#e0e0e0";
	bgcolor = white;
	}
    tplFormControls "widget/component"
	{
	path = "/sys/cmp/form_controls.cmp";
	//background=runserver("/apps/nav/formbg.png");
	fgcolor=white;
	}
    tplRadiobuttonpanel "widget/radiobuttonpanel"
	{
	bgcolor="#e0e0e0";
	outline_bgcolor="black";
	}
    tplOsrc "widget/osrc"
	{
	}
    tplForm "widget/form"
	{
	}
    }
