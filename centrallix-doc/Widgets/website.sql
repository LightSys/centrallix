
select (@oldid := id) from topic where name = "3. Application Components";
delete from topic where parent_id = @oldid;
delete from topic where id = @oldid;

insert into topic values(null,1,"3. Application Components",null,
"       [b]Centrallix Application Widget Reference[/b]
	
	This guide contains a basic overview and specification of each of the widgets that Centrallix supports, including a handful of planned (but not yet implemented or not fully implemented) widgets.
	
	Each widget can have properties, child properties, events, and actions.  Properties define how the widget behaves, appears, and is constructed. Child properties usually define how a widget contained within the given widget relates to its parent. Such properties are normally managed by the containing (parent) widget rather than by the child, and many times the child widget cannot exist without its parent. Events enable a widget to have an \"influence\" on the other widgets on the page, and are utilized by placing a \"connector\" widget within the given widget.  The connector routes an event's firing to the activation of an \"action\" on some other widget on the page.  Actions are \"methods\" on widgets which cause the widget to do something or change its appearance in some way.
	
	The \"overview\" section for each widget describes the widget's purpose and how it operates.  A \"usage\" section is provided to show the appropriate contexts for the use of the widget in a real application.
	
	Some widget properties are 'dynamic'; that is, an expression can be provided which is then evaluated dynamically on the client so that the widget's property changes as the expression's value changes.
	
	\"Client properties\" are available for some widgets.  These properties may not be specified in the application file itself, but are available on the client during application operation.  They often relate to the status of a given widget.
	
	Where possible, sample code is provided for each widget to show how the widget can be used.  In most cases the widget's code is displayed in isolation, although in practice widgets can never be used outside of a \"widget/page\" container widget at some level. In a few cases, a more complete mini-application is shown.
	
	Sample code is generally given in \"structure file\" format, which is the normal format for the building of applications.  However, other suitable object-structured data formats can be used, including XML.

	Copyright (c)  1998-2024 LightSys Technology Services, Inc.
	
	[b]Documentation on the following widgets is available:[/b]

	[table]
	
		[tr][td][widget/autolayout][/td][td]Container which automatically positions its children[/td][/tr]
	
		[tr][td][widget/button][/td][td]A multipurpose button[/td][/tr]
	
		[tr][td][widget/calendar][/td][td]Calendar-type view of event/schedule type data in an objectsource[/td][/tr]
	
		[tr][td][widget/chart][/td][td]Chart widget for graphing data from an ObjectSource, using the ChartJS library.[/td][/tr]
	
		[tr][td][widget/checkbox][/td][td]Form element capable of selecting an on/off, yes/no, true/false, etc., type of value, via a visual 'check mark'.[/td][/tr]
	
		[tr][td][widget/childwindow][/td][td]A dialog or application window which is a lightweight movable container.[/td][/tr]
	
		[tr][td][widget/clock][/td][td]A simple clock widget which displays the current time on the client.[/td][/tr]
	
		[tr][td][widget/component][/td][td]A widget which instantiates a custom component[/td][/tr]
	
		[tr][td][widget/component-decl][/td][td]A widget which defines a custom component[/td][/tr]
	
		[tr][td][widget/connector][/td][td]A nonvisual widget used to trigger an Action on a widget when an Event on another widget fires.[/td][/tr]
	
		[tr][td][widget/datetime][/td][td]A visual widget for displaying and editing a date/time type value.[/td][/tr]
	
		[tr][td][widget/dropdown][/td][td]A visual widget which allows the user to select one from a number of options in a list which appears when the user clicks on the widget.[/td][/tr]
	
		[tr][td][widget/editbox][/td][td]A visual widget which allows the entry of one line of textual data.[/td][/tr]
	
		[tr][td][widget/execmethod][/td][td]A nonvisual widget which can call ObjectSystem methods on server objects.[/td][/tr]
	
		[tr][td][widget/form][/td][td]A nonvisual container used to group a set of form element widgets into a single record or object[/td][/tr]
	
		[tr][td][widget/formstatus][/td][td]A specialized visual widget used to display the current mode of a form widget.[/td][/tr]
	
		[tr][td][widget/frameset][/td][td]A visual container used to create a DHTML frameset within which page widgets can be placed[/td][/tr]
	
		[tr][td][widget/hbox][/td][td]Container which automatically positions its children[/td][/tr]
	
		[tr][td][widget/hints][/td][td]Contains default values and modifier settings for various components[/td][/tr]
	
		[tr][td][widget/html][/td][td]A miniature HTML browser control, capable of viewing and navigating simple web documents.[/td][/tr]
	
		[tr][td][widget/image][/td][td]A picture (image).[/td][/tr]
	
		[tr][td][widget/imagebutton][/td][td]A button widget which uses a set of images to control its appearance.[/td][/tr]
	
		[tr][td][widget/label][/td][td]Form element capable of displaying text in a child window[/td][/tr]
	
		[tr][td][widget/menu][/td][td]A visual pop-up or drop-down menu widget.[/td][/tr]
	
		[tr][td][widget/osrc][/td][td]A nonvisual widget which handles data communication between forms/tables and the Centrallix server.[/td][/tr]
	
		[tr][td][widget/page][/td][td]The top-level container that represents a Centrallix application.[/td][/tr]
	
		[tr][td][widget/pane][/td][td]A visible rectangular container widget with a border[/td][/tr]
	
		[tr][td][widget/parameter][/td][td]A declaration of a parameter that can be passed to an application or component[/td][/tr]
	
		[tr][td][widget/radiobuttonpanel][/td][td]A visual widget displaying a set of 'radio buttons'[/td][/tr]
	
		[tr][td][widget/remotectl][/td][td]Nonvisual widget permitting events to be passed in from a remote Centrallix application[/td][/tr]
	
		[tr][td][widget/remotemgr][/td][td]Nonvisual widget permitting the application to send events to a remote Centrallix application[/td][/tr]
	
		[tr][td][widget/repeat][/td][td]Repeat a subtree of widgets[/td][/tr]
	
		[tr][td][widget/rule][/td][td]Declares a rule - a behavior expected from a widget.[/td][/tr]
	
		[tr][td][widget/scrollbar][/td][td]A scrollbar with a thumb[/td][/tr]
	
		[tr][td][widget/scrollpane][/td][td]A visual container with a scrollbar allowing for content height that exceeds the display area[/td][/tr]
	
		[tr][td][widget/tab][/td][td]A tab (or notebook) widget allowing multiple pages to be layered and individually selected for viewing[/td][/tr]
	
		[tr][td][widget/table][/td][td]A visual widget providing a columnar presentation of multiple objects of the same type (such as query result records).[/td][/tr]
	
		[tr][td][widget/template][/td][td]templates are a way to provide default values (and default children) of different widget_class's[/td][/tr]
	
		[tr][td][widget/textarea][/td][td]Visual multi-line text data entry widget.[/td][/tr]
	
		[tr][td][widget/textbutton][/td][td]A simple visual button widget built not from images but from a simple text string.[/td][/tr]
	
		[tr][td][widget/timer][/td][td]A nonvisual widget which is used to fire an event when a period of time has elapsed.[/td][/tr]
	
		[tr][td][widget/treeview][/td][td]A visual widget used to display tree-structured data (such as a directory tree or similar).[/td][/tr]
	
		[tr][td][widget/variable][/td][td]A scalar variable object[/td][/tr]
	
		[tr][td][widget/vbox][/td][td]Container which automatically positions its children[/td][/tr]
	
	[/table]
");
select (@newid := @@last_insert_id);

	
insert into topic values(null, @newid, "widget/autolayout", null,
"		[b]autolayout[/b] :: Container which automatically positions its children

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/autolayout[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The autolayout widget is used to automatically position widgets without having to explicitly provide x, y, width, and height values for those widgets.  Most of the layout in Centrallix is done on a coordinate basis (although the automatic resize logic adjusts those coordinates to fit the target application or component render size).  The autolayout widget provides the capability to do nested gui layout in Centrallix, where the coordinate positions of the widgets are computed automatically.


			Normally, the autolayout widget is used not as \"widget/autolayout\", but as \"widget/hbox\" or \"widget/vbox\", two synonyms for this widget which also set the layout direction (horizontal or vertical).


			A \"hbox\" widget stacks its children horizontally one after the other, while a \"vbox\" widget stacks its children vertically from the top down.  The spacing between children can be set, and the size of each child can be either independently set per-child, or it can be set by virtue of the geometry of the autolayout widget.


			The autolayout widget supports having multiple rows or columns of widgets.  That is, if a \"vbox\" widget fills up vertically, it can resume adding children to a second vertical column should there be space to the right of the original column of widgets.  For this to work, the width of the columns (for a vbox) or the height of the rows (for an hbox) must be specified.


		
	[b]Usage:[/b]
	

			The autolayout widget can occur any place that a visual widget can be placed.  As it is a container, it can also contain both visual and nonvisual widgets and containers.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]align[/td]
					[td]string[/td]
					[td]Sets the alignment of text  can have left (default) right or center.[/td]
				[/tr]
			
				[tr]
					[td]cellsize[/td]
					[td]integer[/td]
					[td]For an hbox, this is the width to use for the child widgets (unless otherwise specified by the child widget).  For a vbox, this refers to the height to use for child widgets unless otherwise specified.[/td]
				[/tr]
			
				[tr]
					[td]column_width[/td]
					[td]integer[/td]
					[td]This specifies the width of columns for a vbox -- should the first column of widgets fill up, a second column will be started at this offset from the start of the first column.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the autolayout area.  If omitted, it defaults to the maximum available height for the given width, without overlapping other visible widgets.  If both width and height are unspecified, Centrallix will chose a width and height that maximize the available autolayout area.[/td]
				[/tr]
			
				[tr]
					[td]justify[/td]
					[td]string[/td]
					[td]Makes the text fill page (justifies text - google it if you need).[/td]
				[/tr]
			
				[tr]
					[td]row_height[/td]
					[td]integer[/td]
					[td]This specifies the height of rows for an hbox -- should the first row of widgets fill up horizontally, a second row will be started beneath the first one, at this vertical offset from the start of the first row.[/td]
				[/tr]
			
				[tr]
					[td]spacing[/td]
					[td]integer[/td]
					[td]The spacing, in pixels, between widgets.  For an hbox, this refers to the horizontal spacing between children.  For a vbox, this refers to the vertical spacing.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]Either \"hbox\" or \"vbox\".  Not needed if the widget is created via \"widget/hbox\" or \"widget/vbox\".[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the autolayout area.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the autolayout area.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the autolayout area.  If omitted, it defaults to the maximum available width for the given height, without overlapping other visible widgets.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of any child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]autolayout_order[/td]
							[td]integer[/td]
							[td]An optional property, this allows the layout sequence of child widgets to be controlled, and the children are placed in ascending order.  If unspecified, this defaults to a value of 100 for the first child encountered, or to N+1 where N is the autolayout sequence of the most recent child widget encountered.[/td]
						[/tr]
					
						[tr]
							[td]height[/td]
							[td]integer[/td]
							[td]Height, in pixels, of the child.[/td]
						[/tr]
					
						[tr]
							[td]width[/td]
							[td]integer[/td]
							[td]Width, in pixels, of the child.[/td]
						[/tr]
					
				[/table]
			
	[b]Sample Code:[/b]
	
		[code]
		

		

// Here is a vbox which can have up to three

// vertical columns of widgets; the columns will

// fill up one at a time.

vbox1 \"widget/vbox\"

	{

	x=0; y=0; width=600; height=200;

	spacing=5; column_width=200;

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/button", null,
"		[b]button[/b] :: A multipurpose button

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/button[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

		Combines the functionality of the textbutton and imagebutton, as well as adding the ability to have both text and image displayed in a button.


	    
	[b]Usage:[/b]
	

		A Button can be placed inside any visible container, but only nonvisual widgets can be placed within it.  Properties that don't apply to the button's type are ignored.


	    
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the button's background.If neither bgcolor nor background are specified, the button is transparent.[/td]
				[/tr]
			
				[tr]
					[td]clickimage[/td]
					[td]string[/td]
					[td]The ObjectSystem pathname of the image to be shown when the user clicks the imagebutton.  Defaults to 'image' if not specified.[/td]
				[/tr]
			
				[tr]
					[td]disable_color[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used for the button's text when it is disabled.[/td]
				[/tr]
			
				[tr]
					[td]disabledimage[/td]
					[td]string[/td]
					[td]The ObjectSystem pathname of the image to be shown when the imagebutton is disabled.  Defaults to 'image' if not specified.[/td]
				[/tr]
			
				[tr]
					[td]enabled[/td]
					[td]yes/no or expr[/td]
					[td]Whether the button is enabled (can be clicked).  Default is 'yes'.  Also supports dynamic runclient() expressions allowing the enabled status of the button to follow the value of an expression.[/td]
				[/tr]
			
				[tr]
					[td]fgcolor1[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the text on the button.  Default \"white\".[/td]
				[/tr]
			
				[tr]
					[td]fgcolor2[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the text's 1-pixel drop-shadow.  Default \"black\".[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the text button.[/td]
				[/tr]
			
				[tr]
					[td]image[/td]
					[td]string[/td]
					[td]The pathname of the image to be shown when the button is \"idle\".[/td]
				[/tr]
			
				[tr]
					[td]pointimage[/td]
					[td]string[/td]
					[td]The pathname of the image to be shown when the button is pointed-to.  Defaults to  the 'image' if not specified.[/td]
				[/tr]
			
				[tr]
					[td]spacing[/td]
					[td]integer[/td]
					[td]The distance between the image and text if applicable.[/td]
				[/tr]
			
				[tr]
					[td]text[/td]
					[td]string[/td]
					[td]The text to appear on the button.[/td]
				[/tr]
			
				[tr]
					[td]tristate[/td]
					[td]yes/no[/td]
					[td]Whether or not the button is tri-state (does not display a raised border until the user points at it). Default is yes.[/td]
				[/tr]
			
				[tr]
					[td]type[/td]
					[td]string[/td]
					[td]There are currently 7 different types.  1) text 2) image 3) topimage (image above text) 4) rightimage 5) leftimage 6) bottomimage 7) textoverimage (text over image background).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]The width, in pixels, of the text button.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the widget. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, @newid, "widget/calendar", null,
"		[b]calendar[/b] :: Calendar-type view of event/schedule type data in an objectsource

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/calendar[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The calendar widget is used to present event/schedule types of data in a year, month, week, or day format.  The data for the events on the calendar is obtained from an objectsource's query results, and as such, can be controlled via queries and other forms/tables associated with the same objectsource.  The four formats are listed below and can be selected during app run-time.


			

			    [b]Year[/b] - Presents twelve months at a time in a low-detail type of setting; when events occur on a day, the day is highlighted, but no more data than that is displayed.  If the user points the mouse at a given day, it will display in a popup \"tooltip\" type of manner the events associated with that day.  If more than one year's worth of data is in the objectsource's replica, then multiple years are displayed one after the other.  Each year is displayed as four rows of three months each, in traditional calendar format.


			    [b]Month[/b] - Presents one month's worth of data in a medium-detail display.  For each month with data in the objectsource, the calendar will display the month as multiple rows, each row containing one week's worth of days.  The calendar will attempt to display the various entries for each day, in order of priority, but will limit the amount of data displayed to keep the boxes for the days approximately visually square.  Pointing at a day will show the events for the day in a popup, and pointing at an event in the day will show the details for that event.


			    [b]Week[/b] - Presents one week's worth of data in a high-detail display.  For each week with data in the objectsource, the calendar will display the week as seven days with a large vertical extent capable of displaying all events for the day.  The left edge of the display will contain times during the day for the day's events to line up with.


			    [b]Day[/b] - Presents an entire day's worth of data in a high-detail display.  For each day with data in the objectsource, the calendar will display the day in full-width, with the day's events listed chronologically from top to bottom.


			

			It should be noted that all of the data for the calendar must fit into the objectsource, or else the calendar will not display all of the available data.  In this manner the calendar's display must be controlled via the objectsource's data.


		
	[b]Usage:[/b]
	

			The calendar widget can be placed inside of any visual container, but because its height can change, it is often placed inside of a scrollpane widget.  This widget may not contain other visual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]An image for the background of the calendar widget.  Should be an ObjectSystem path.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, either named or numeric (RGB), for the background of the calendar.  If neither bgcolor nor background are specified, the calendar's background is transparent.[/td]
				[/tr]
			
				[tr]
					[td]displaymode[/td]
					[td]string[/td]
					[td]The visual display mode of the calendar - can be 'year', 'month', 'week', or 'day' (see the overview, above, for details on these modes).  Defaults to 'year'.[/td]
				[/tr]
			
				[tr]
					[td]eventdatefield[/td]
					[td]string[/td]
					[td]The name of the date/time field in the datasource containing the event date/time.  A mandatory field.[/td]
				[/tr]
			
				[tr]
					[td]eventdescfield[/td]
					[td]string[/td]
					[td]The name of the string field in the datasource containing the event's full description.  If not supplied, descriptions will be omitted.[/td]
				[/tr]
			
				[tr]
					[td]eventnamefield[/td]
					[td]string[/td]
					[td]The name of the string field in the datasource containing the event's short name.  A mandatory field.[/td]
				[/tr]
			
				[tr]
					[td]eventpriofield[/td]
					[td]string[/td]
					[td]The name of the integer field in the datasource containing the event's priority.  If not supplied, priority checking is disabled.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]The height of the calendar, in pixels.  If not specified, the calendar may grow vertically an indefinite amount in order to display its given data.  If specified, however, the calendar will be limited to that size.[/td]
				[/tr]
			
				[tr]
					[td]minpriority[/td]
					[td]integer[/td]
					[td]The minimum priority level for events that will be displayed on the calendar; the values for this integer depend on the nature of the 'priority' field in the data source; only events with priority values numerically higher than or equal to this minimum priority will be displayed.  Defaults to 0.[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]The color of the words in the calendar.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]The width of the calendar, in pixels.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the calendar, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the calendar, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, @newid, "widget/chart", null,
"		[b]chart[/b] :: Chart widget for graphing data from an ObjectSource, using the ChartJS library.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/chart[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			A chart widget is used to display charts and graphs of data. It uses the Chart.js library.

		
	[b]Usage:[/b]
	
			The chart widget can be placed inside of any visual container, and will attach itself to any objectsource widget that contains it (whether directly or indirectly).  Charts may not contain visual widgets.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]chart_type[/td]
					[td]string[/td]
					[td]Currently supported types are \"bar\", \"line\", \"scatter\", and \"pie\". Defaults to \"bar\". Chart.js supports much more than this, so new types could be added with relative ease.[/td]
				[/tr]
			
				[tr]
					[td]legend_position[/td]
					[td]string[/td]
					[td]The position of the legend on the chart. May be \"top\", \"botton\", \"left\", or \"right\".[/td]
				[/tr]
			
				[tr]
					[td]objectsource[/td]
					[td]string[/td]
					[td]The name of the ObjectSource widget which will supply data for the chart. We recommend that you do not specify this directly but instead embed the chart widget within a parent ObjectSource, directly or indirectly. If not specified, the chart will look for an ObjectSource in its parents.[/td]
				[/tr]
			
				[tr]
					[td]start_at_zero[/td]
					[td]boolean[/td]
					[td]Set to false if you want the y axis to start at the lowest data value. Default is true.[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]The title of the chart. Default is none.[/td]
				[/tr]
			
				[tr]
					[td]titlecolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the chart title.[/td]
				[/tr]
			
				[tr]
					[td]title_size[/td]
					[td]integer[/td]
					[td]The size, in points, of the title font.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of widget/chart-series child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]chart_type[/td]
							[td]string[/td]
							[td]Chart.js has limited support for having multiple chart types on the same axes. The outer chart type must still be specified if this is set.[/td]
						[/tr]
					
						[tr]
							[td]color[/td]
							[td]string[/td]
							[td]A color, RGB or named, for the chart element (ex: line) which represents this series.[/td]
						[/tr]
					
						[tr]
							[td]fill[/td]
							[td]boolean[/td]
							[td]Set to false to remove the color fill beneath a line chart. Default is true.[/td]
						[/tr]
					
						[tr]
							[td]label[/td]
							[td]string[/td]
							[td]A label for the data series.[/td]
						[/tr]
					
						[tr]
							[td]x_column[/td]
							[td]string[/td]
							[td]Which column in the data should be used for the x axis? By default, the chart will pick one for you.[/td]
						[/tr]
					
						[tr]
							[td]y_column[/td]
							[td]string[/td]
							[td]Which column in the data should be used for the y axis? By default, the chart will pick one for you.[/td]
						[/tr]
					
				[/table]
			
			(of widget/chart-axis child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]axis[/td]
							[td]string[/td]
							[td]Which axis is this? Possible values are \"x\" and \"y\". Default is \"x\".[/td]
						[/tr]
					
						[tr]
							[td]label[/td]
							[td]string[/td]
							[td]A label for the axis. Default is none.[/td]
						[/tr]
					
				[/table]
			
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$
// Here is a simple chart.
chart \"widget/chart\"
	{
	x=0; y=0;
	width=483;
	height=650;
	chart_type=\"bar\";
	title=\"Sample Chart\";
	titlecolor=\"black\";
	legend_position=\"right\";

	series1 \"widget/chart-series\" { color=\"blue\"; }

	series2 \"widget/chart-series\"
		{
		fill=no;
		chart_type=\"line\";
		x_column=\"name\";
		y_column=\"permissions\";
		}

	x_axis \"widget/chart-axis\" { axis=\"x\"; label=\"X\"; }
	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/checkbox", null,
"		[b]checkbox[/b] :: Form element capable of selecting an on/off, yes/no, true/false, etc., type of value, via a visual 'check mark'.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/checkbox[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The checkbox widget is used to display a value that can be either true or false (or on/off, yes/no, etc.).It displays as a simple clickable check box.


		
	[b]Usage:[/b]
	

			The checkbox widget can be placed inside of any visual container, and will attach itself to any form widget that contains it (whether directly or indirectly).  Checkboxes may not contain visual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]checked[/td]
					[td]yes/no[/td]
					[td]Whether this checkbox should be initially checked or not.  Default 'no'.[/td]
				[/tr]
			
				[tr]
					[td]enabled[/td]
					[td]boolean[/td]
					[td]This allows the checkbox to be changed.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]Name of objectsource field that should be associated with this checkbox.[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]The name of the form that this checkbox is associated with.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the checkbox, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the checkbox, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetValue[/td]
					[td]This takes a given value and sets the check box value to it.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the checkbox. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the checkbox (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here are two checkboxes.

checkbox_test \"widget/page\"

	{

	background=\"/sys/images/slate2.gif\";

	testcheck2 \"widget/checkbox\"

		{

		x = 20; y = 40;

		}

	testcheck \"widget/checkbox\"

		{

		x = 20; y = 20;

		}

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/childwindow", null,
"		[b]childwindow[/b] :: A dialog or application window which is a lightweight movable container.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/childwindow[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The childwindow provides the capability of creating a popup dialog or application window within a web page.  The window is not actually a separate browser window, but rather a movable container that can appear \"floating\" above the rest of the Centrallix application.They can take on one of two styles;\"dialog\" and \"window\", which currently relate only to how the window border and titlebar are drawn.  The windows support windowshading (double-clicking on the titlebar to \"rollup\" or \"rolldown\" the window contents).


			These \"windows\" currently do not support multiple instantiation.


		
	[b]Usage:[/b]
	

			Childwindows are normally coded at the top-level of an application, since if they are placed within containers, the container limits where the window can be moved (the window is clipped by its container).  However, if the \"toplevel\" property is enabled, then the window floats at the top level of other widgets regardless of the size and location of its container in the application or component (thus the window is not clipped by its container).


			These windows can contain other visual and nonvisual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the body of the window.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the window body's background.If neither transparent.[/td]
				[/tr]
			
				[tr]
					[td]border_color[/td]
					[td]string[/td]
					[td]A color that outlines window.[/td]
				[/tr]
			
				[tr]
					[td]border_radius[/td]
					[td]integer[/td]
					[td]A radius that describes the sharpness of the corners of the window (smaller means sharper).[/td]
				[/tr]
			
				[tr]
					[td]border_style[/td]
					[td]string[/td]
					[td]Determines the look of the outline.[/td]
				[/tr]
			
				[tr]
					[td]closetype[/td]
					[td]string[/td]
					[td]Decides wither the screen closes widthwise (1) or heightwise (2).[/td]
				[/tr]
			
				[tr]
					[td]gshade[/td]
					[td]string[/td]
					[td]Acts as a boolean to declare if the window is shaded or not (all but title bar minimized).[/td]
				[/tr]
			
				[tr]
					[td]hdr_background[/td]
					[td]string[/td]
					[td]A background image for the titlebar of the window.[/td]
				[/tr]
			
				[tr]
					[td]hdr_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the titlebar of the window.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the window.[/td]
				[/tr]
			
				[tr]
					[td]icon[/td]
					[td]string[/td]
					[td]A pathname to an icon to be used in the upper left corner of the window.  If unspecified, the default \"CX\" ichthus icon is used instead.[/td]
				[/tr]
			
				[tr]
					[td]modal[/td]
					[td]yes/no[/td]
					[td]If \"yes\", the window is modal (and IsModal need not be passed to \"Open\").  A modal window will force the user to close the window before anything else in the application can be accessed.[/td]
				[/tr]
			
				[tr]
					[td]shadow_angle[/td]
					[td]integer[/td]
					[td]The placement of the shadow described as a rotational transformation with respect to the window.[/td]
				[/tr]
			
				[tr]
					[td]shadow_color[/td]
					[td]string[/td]
					[td]The color of the shadow.[/td]
				[/tr]
			
				[tr]
					[td]shadow_radius[/td]
					[td]integer[/td]
					[td]A radius that describes the sharpness of the corners of the shadow (smaller means sharper).[/td]
				[/tr]
			
				[tr]
					[td]shadow_offset[/td]
					[td]integer[/td]
					[td]The placement of the shadow with respect to the window.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]Either \"dialog\" or \"window\", and determines the style of the window's border.[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]The color for the titlebar's text (window title).  Default \"black\".[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]The window's title.[/td]
				[/tr]
			
				[tr]
					[td]titlebar[/td]
					[td]yes/no[/td]
					[td]Whether the window will have a titlebar (and the close \"X\" in the upper right corner of the window). Default \"yes\".[/td]
				[/tr]
			
				[tr]
					[td]toplevel[/td]
					[td]yes/no[/td]
					[td]If \"yes\", the window will float above all other widgets in the application, otherwise it will be clipped by its own container.[/td]
				[/tr]
			
				[tr]
					[td]visible[/td]
					[td]boolean[/td]
					[td]The window is initially visible on screen. The window has an action which can \"true\".[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the window.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the window, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the window, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Close[/td]
					[td]Closes the window.  Note that widgets inside the window may choose to veto the Close operation: for example, if there is unsaved data in a form.[/td]
				[/tr]
			
				[tr]
					[td]Open[/td]
					[td]Opens the window.  If the parameter IsModal is set to 1, then the window becomes modal (only the window's contents are accessible to the user until the window is closed).  If the parameter NoClose is set to 1, then the close button in the upper right corner of the window becomes inactive and the window will only close via the Close, SetVisibility, and ToggleVisibility actions.[/td]
				[/tr]
			
				[tr]
					[td]Point[/td]
					[td]Makes the window relocate to a side using a triangle (pop over).[/td]
				[/tr]
			
				[tr]
					[td]Popup[/td]
					[td]Opens a window like a pop-up.[/td]
				[/tr]
			
				[tr]
					[td]SetVisibility[/td]
					[td]One parameter, \"is_visible\", which is set to 0 or 1 to hide or show the window, respectively.[/td]
				[/tr]
			
				[tr]
					[td]Shade[/td]
					[td]Turns the gshade property(see above) on.[/td]
				[/tr]
			
				[tr]
					[td]ToggleVisibility[/td]
					[td]If the window is visible, this closes it.  If it is closed, this opens it.[/td]
				[/tr]
			
				[tr]
					[td]Unshade[/td]
					[td]Turns the gshade property(see above) off.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Close[/td]
					[td]This event is triggered each time the window is closed (becomes invisible).[/td]
				[/tr]
			
				[tr]
					[td]Load[/td]
					[td]This event is triggered the first time the window is opened.  If the window is visible by default, then this event is triggered when the application loads.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]Open[/td]
					[td]This event is triggered each time the window is opened (becomes visible).[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

MyWindow \"widget/childwindow\"

	{

	x = 10; y = 10;

	width = 200; height = 200;

	style = \"dialog\";

	bgcolor = \"#c0c0c0\";

	hdr_background = \"/sys/images/grey_gradient2.png\";

	title = \"An HTML Window\";

	textcolor = \"black\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/clock", null,
"		[b]clock[/b] :: A simple clock widget which displays the current time on the client.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/clock[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The clock widget displays the current time on the client computer, and is very configurable in terms of appearance.


		
	[b]Usage:[/b]
	

			The clock widget can be used inside of any container capable of having visual subwidgets.  It may contain no widgets other than any applicable connectors.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]ampm[/td]
					[td]yes/no[/td]
					[td]Whether to show \"AM\" and \"PM\", default is 'yes' if 'hrtype' is 12, and cannot be set to 'yes' if 'hrtype' is 24.[/td]
				[/tr]
			
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]An image for the background of the clock widget.  Should be an ObjectSystem path.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, either named or numeric, for the background of the clock.  If neither bgcolor nor background are specified, the clock is transparent.[/td]
				[/tr]
			
				[tr]
					[td]bold[/td]
					[td]string[/td]
					[td]Acts as a boolean to declare if the type will be bold (larger and thicker).[/td]
				[/tr]
			
				[tr]
					[td]fgcolor1[/td]
					[td]string[/td]
					[td]A color, either named or numeric, for the foreground text of the clock.[/td]
				[/tr]
			
				[tr]
					[td]fgcolor2[/td]
					[td]string[/td]
					[td]A color, either named or numeric, for the foreground text's shadow, if 'shadowed' is enabled.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]Stores the field name and memory.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]The height of the clock, in pixels.[/td]
				[/tr]
			
				[tr]
					[td]hrtype[/td]
					[td]integer[/td]
					[td]Set to 12 for a 12-hour clock, or to 24 for military (24-hour) time.[/td]
				[/tr]
			
				[tr]
					[td]moveable[/td]
					[td]string[/td]
					[td]Acts as a boolean to control if the entire clock can be moved.[/td]
				[/tr]
			
				[tr]
					[td]seconds[/td]
					[td]integer[/td]
					[td]Whether or not the seconds should be displayed.  Default is 'yes'.[/td]
				[/tr]
			
				[tr]
					[td]shadowed[/td]
					[td]yes/no[/td]
					[td]Whether or not the clock's text has a shadow.  Default is 'no'.[/td]
				[/tr]
			
				[tr]
					[td]shadowx[/td]
					[td]integer[/td]
					[td]The horizontal offset, in pixels, of the shadow.[/td]
				[/tr]
			
				[tr]
					[td]shadowy[/td]
					[td]integer[/td]
					[td]The vertical offset, in pixels, of the shadow.[/td]
				[/tr]
			
				[tr]
					[td]size[/td]
					[td]integer[/td]
					[td]The size, in points, clock's text.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]The width of the clock, in pixels.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]The X location of the left edge of the widget.  Default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]The Y location of the top edge of the widget.  Default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
");
	
insert into topic values(null, @newid, "widget/component", null,
"		[b]component[/b] :: A widget which instantiates a custom component

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/component[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			This widget is used to instantiate a custom component that has already been defined using a widget/component-decl widget, typically inside a \".cmp\" file.  The instantiation can be either static or dynamic:  a static component is rendered along with the component or application that it resides inside, whereas a dynamic component is loaded as needed from the client.  Components may also allow multiple instantiation when dynamic, which is especially beneficial with components whose top-level widget happens to be a widget/childwindow.


		
	[b]Usage:[/b]
	

			A component can be either visual or non-visual, and can be placed at almost any point in the application.


			At the time of writing, only connectors may be placed inside a widget/component.  It is planned that it be possible to place widgets inside a widget/component which then appear inside specially designated containers within the component itself.  However that capability is not available at present.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]auto_destroy[/td]
					[td]yes/no[/td]
					[td]If enabled (dynamic single-instantiation components only), when a component is instantiated a second time, the original component is automatically destroyed so there is at most one instance at a time in existence.  Defaults to 'yes'.[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]integer[/td]
					[td]Can specify another form to be a child of (if different than the implied form in which this control is nested).[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the component.  If unset, this defaults to the height of the component's containing widget.[/td]
				[/tr]
			
				[tr]
					[td]mode[/td]
					[td]string[/td]
					[td]Either \"static\" or \"dynamic\".  A static component is rendered with the application, whereas a dynamic one is loaded as needed from the client.  Defaults to 'static'.[/td]
				[/tr]
			
				[tr]
					[td]multiple_instantiation[/td]
					[td]yes/no[/td]
					[td]If enabled (dynamic components only), the component can be instantiated more than once on the client.  Defaults to 'no'.[/td]
				[/tr]
			
				[tr]
					[td]path[/td]
					[td]string[/td]
					[td]The path, in the OSML, to the component's definition (.cmp) file. (e.g. /sys/cmp/smart_field.cmp, /sys/cmp/form_controls.cmp, /samples/button.cmp).[/td]
				[/tr]
			
				[tr]
					[td]toplevel[/td]
					[td]string[/td]
					[td]Acts as a boolean to indicate if there are any components on a higher level.[/td]
				[/tr]
			
				[tr]
					[td]type[/td]
					[td]string[/td]
					[td]A string that represents the data type.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the component.  If unset, this defaults to the width of the component's containing widget.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the component.  If unset, defaults to 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the component.  If unset, defaults to 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of /sys/cmp/smart_field.cmp child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]ctl_type[/td]
							[td]string[/td]
							[td] Type of smart field (e.g. label, editbox, checkbox, datetime).[/td]
						[/tr]
					
						[tr]
							[td]type[/td]
							[td]string[/td]
							[td](e.g. readonly). Must have field be set in order to be \"readonly\".[/td]
						[/tr]
					
						[tr]
							[td]value[/td]
							[td]string[/td]
							[td]This is something which can accessed as :this_widget:value (returns the value of text which can't be directly accessed?), but can not be set directly through this property -- see SetValue listed under the Action section.[/td]
						[/tr]
					
				[/table]
			
			(of /sys/cmp/form_controls.cmp child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]multienter[/td]
							[td]integer[/td]
							[td](e.g. 1)[/td]
						[/tr]
					
						[tr]
							[td]object_name[/td]
							[td]string[/td]
							[td]Title to put across the form control bar (Note: the form_controls.cmp allows the user to perform row operations on a given record set).[/td]
						[/tr]
					
				[/table]
			
			(of /apps/kardia/modules/base/editbox_tree.cmp child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]content[/td]
							[td]string[/td]
							[td]This is something which can accessed as :this_widget:content (returns the value of text which can't be directly accessed?), but can not be set directly through this property -- see SetValue listed under the Action section.[/td]
						[/tr]
					
				[/table]
			
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Destroy[/td]
					[td]Destroys the component.  If multiple instances exist, then all instances are destroyed.[/td]
				[/tr]
			
				[tr]
					[td]Instantiate[/td]
					[td]Instantiates the component.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]LoadComplete[/td]
					[td]This event is triggered when a dynamic component completes loading from the server.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

			

// Instantiate the form_controls.cmp component

formctl \"widget/component\"

	{

	x=0; y=0; width=800; height=26;

	mode=static;

	path=\"/sys/cmp/form_controls.cmp\";

	form=my_form;

	}

			

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/component-decl", null,
"		[b]component-decl[/b] :: A widget which defines a custom component

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/component-decl[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The component-decl widget is used to define a new component which can be used and reused in other applications or components.  Creating a component is very similar to creating an application - except that the top-level widget in a component is \"widget/component-decl\" instead of \"widget/page\".


		
	[b]Usage:[/b]
	

			A widget/component-decl widget must occur at the top level of a component (.cmp) file.


			Other visual and nonvisual widgets may be placed inside a component-decl, in addition to parameters and declarations of Events and Actions that the component generates and handles.


			To declare that a component generates an Event, place a \"widget/component-decl-event\" inside the component at the top level.  No parameters are needed for that Event.  To cause the component to generate the Event, trigger an Action with the same name on the component-decl from inside, and the event will be activated for the containing application or component.


			Similarly, to declare that a component can receive an Action, place a \"widget/component-decl-action\" inside the component at the top level.  Again, no parameters are needed.  The containing application or component can then trigger the Action, which will cause an event to occur inside the component.  The event occurs on the component-decl widget (top level of the component), and can be caught with a connector widget.


			Components can take parameters just like applications can.  See the \"widget/parameter\" widget for details on how to declare parameters on applications and components.


			Several of the properties of a component-decl are used to make it easy to wrap another widget or component.  The expose_actions_for, expose_events_for, and expose_properties_for properties cause the actions, events, and properties of a widget or component inside the component-decl to be exposed to the \"outside world\".  The apply_hints_to property causes any presentation hints (\"widget/hints\") that are applied in the instantiation of a component-decl (inside the \"widget/component\") to be applied to the specified widget or component inside the component-decl.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]apply_hints_to[/td]
					[td]string[/td]
					[td]Specifies a widget inside the component-decl that will receive any presentation hints constraints (\"widget/hints\") that are applied to the instantiation of this component-decl.[/td]
				[/tr]
			
				[tr]
					[td]expose_actions_for[/td]
					[td]string[/td]
					[td]Specifies a widget inside the component-decl whose actions (methods) we want to expose on the external interface of this component-decl.[/td]
				[/tr]
			
				[tr]
					[td]expose_events_for[/td]
					[td]string[/td]
					[td]Specifies a widget inside the component-decl whose events we want to expose on the external interface of this component-decl.[/td]
				[/tr]
			
				[tr]
					[td]expose_properties_for[/td]
					[td]string[/td]
					[td]Specifies a widget inside the component-decl whose client-side properties we want to expose on the external interface of this component-decl.[/td]
				[/tr]
			
				[tr]
					[td]visual[/td]
					[td]string[/td]
					[td]Indicates if the component is seen (not = -1).[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Alert[/td]
					[td]Sends an alert to the screen.[/td]
				[/tr]
			
				[tr]
					[td]Launch[/td]
					[td]Loads the widget.[/td]
				[/tr]
			
				[tr]
					[td]TriggerEvent[/td]
					[td]Allows events to happen.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]LoadComplete[/td]
					[td]Occurs when the widget has finished launching.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

			

// Declare a component having a button that triggers an event

my_button \"widget/component-decl\"

	{

	width=100; height=32;

	button_txt \"widget/parameter\" { type=string; default=\"OK\"; }

	button_clk \"widget/component-decl-event\" { }

	btn \"widget/textbutton\"

		{

		x=0; y=0; width=100; height=32;

		text = runserver(:this:button_txt);

		fgcolor1=\"white\"; fgcolor2=\"black\";

		tristate=no;

		bgcolor=\"#c0c0c0\";

		onclk \"widget/connector\"

			{

			event=\"Click\";

			target=my_button;

			action=\"button_clk\";

			}

		}

	}

			

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/connector", null,
"		[b]connector[/b] :: A nonvisual widget used to trigger an Action on a widget when an Event on another widget fires.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/connector[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			Each widget can have events and actions associated with it. The events occur when certain things occur via the user interface, via timers, or even as a result of data being loaded from the server.  Actions cause certain things to happen to or within a certain widget, for example causing an HTML layer to reload with a new page, or causing a scrollable area to scroll up or down.


			The connector widget allows an event to be linked with an action without actually writing any JavaScript code to do so -- the connector object is created, and given an event to trigger it and an action to perform when it is triggered.


			Events and actions can have parameters, which specify more information about the occurrence of an event, or which specify more information about how to perform the action.  Such parameters from events can be linked into parameters for actions via the connector widget as well.


			When supplying a parameter to a connector (other than \"action\", \"event\", and \"target\"), there are several ways that parameter can be specified.  First the parameter can be a constant value, defined explicitly in the application.  Second, the parameter can be an expression which is evaluated on the client (using the runclient() pseudo-function) and which can change based on the conditions under which the connector is triggered.  In this latter case, the runclient() expression can contain parameters supplied by the event, or values supplied by other widgets.


		
	[b]Usage:[/b]
	

			The connector object should be a sub-object of the widget which will trigger the event.  Any widget with events can contain connectors as subwidgets.More than one connector may be attached to another widget's event.


			An example connector would link the click of a URL in a treeview with the loading of a new page in an HTML area.  See the Example source code at the end of this document to see how this is done.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]action[/td]
					[td]string[/td]
					[td]The name of the action which will be activated on another widget on the page.[/td]
				[/tr]
			
				[tr]
					[td]event[/td]
					[td]string[/td]
					[td]The name of the event that this connector will act on.[/td]
				[/tr]
			
				[tr]
					[td]source[/td]
					[td]string[/td]
					[td]The name of the widget generating the event (defaults to the widget the connector is placed inside of).[/td]
				[/tr]
			
				[tr]
					[td]target[/td]
					[td]string[/td]
					[td]The name of another widget on the page whose action will be called by this connector (if unspecified, defaults to the widget the connector is placed inside of).[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here's our connector. Imagine that this is embedded within a treeview

// and references an 'html' control called 'ht1' somewhere else on the page.

//

cn1 \"widget/connector\"

	{

	// Triggered by ClickItem from the treeview.

	event = \"ClickItem\";

	// Causes the LoadPage action on html area 'ht1'

	target = \"ht1\";

	action = \"LoadPage\";

	// The Source for LoadPage is the Pathname from ClickItem.

	Source = runclient(:Pathname);

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/datetime", null,
"		[b]datetime[/b] :: A visual widget for displaying and editing a date/time type value.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/datetime[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

		    The datetime widget displays a calendar and clock.  Input is done by typing into the bar or by selecting a date in display pane.  When the form goes into query mode two panes will appear, one for the start and one for the end time.  Note that the osml format for searching for dates is dd Mon yyyy, whereas the format dates are displayed in the widget is Mon dd, yyyy.


		
	[b]Usage:[/b]
	

		    This widget can be placed within any visual widget.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]The background color of the bar and pane (named or numeric).[/td]
				[/tr]
			
				[tr]
					[td]date_only[/td]
					[td]yes/no[/td]
					[td]Default \"no\".  If set to \"yes\" indicates that the datetime widget should only allow select and display a date, not a date and time.  It may be useful to set default_time when using this option.[/td]
				[/tr]
			
				[tr]
					[td]default_time[/td]
					[td]string[/td]
					[td]When the user sets the date without selecting a time value (or if date_only is set), use this time as the \"default\" time.  This can be set to \"00:00:00\" or \"23:59:59\", for example, to specify a time at the start or end of a day.[/td]
				[/tr]
			
				[tr]
					[td]fgcolor[/td]
					[td]string[/td]
					[td]The foreground color of the bar and pane (named or numeric).[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]Not used currently to my knowledge.[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]Name of a form object that is a parent of the datetime widget (not required).[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the bar.[/td]
				[/tr]
			
				[tr]
					[td]initialdate[/td]
					[td]string[/td]
					[td]Used to set the initialdate by a string.[/td]
				[/tr]
			
				[tr]
					[td]search_by_range[/td]
					[td]yes/no[/td]
					[td]Default \"yes\".  Specifies that when the datetime widget is in a form that is in search (QBF) mode, the datetime should display both a \"start\" and an \"end\" calendar so the user can search for records/objects matching a range of dates.[/td]
				[/tr]
			
				[tr]
					[td]sql[/td]
					[td]string[/td]
					[td]Used to set the initial date by a query.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the bar.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the bar.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the bar.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetValue[/td]
					[td]Generates an internal data object using the data specified or the current time.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the widget. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the widget (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]GetFocus[/td]
					[td]This event occurs when the datetime widget receives keyboard focus (if the user tabs on to it or clicks on it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]LoseFocus[/td]
					[td]This event occurs when the datetime widget loses keyboard focus (if the user tabs off of it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

	

	dda \"widget/datetime\" {

	    x=15;y=15;

	    width=200;

	    height=20;

	    fgcolor=\"#000000\";

	    bgcolor=\"#cfcfcf\";

	    //initialdate=\"12:34:56 May 3, 2002\";

	}

	

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/dropdown", null,
"		[b]dropdown[/b] :: A visual widget which allows the user to select one from a number of options in a list which appears when the user clicks on the widget.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/dropdown[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

		  A dropdown form element widget that allows one of several options to be selected in a visual manner.  The options are filled in using one of the child widgets, or via an SQL query to a database defined below.


		
	[b]Usage:[/b]
	

		  This widget can be placed within any visual widget (or within a form widget).  It may only contain 'dropdownitem' child objects, although it may of course also contain connectors as needed.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]bgcolor[/td]
					[td]integer[/td]
					[td]The background color for the widget.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]Fieldname (from the dataset) to bind this element to.[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]Represents the widget form (groups many widgets into one widget).[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the dropdown.[/td]
				[/tr]
			
				[tr]
					[td]hilight[/td]
					[td]integer[/td]
					[td]The color of the highlighted option when the mouse goes over the item.[/td]
				[/tr]
			
				[tr]
					[td]invalid_select_default[/td]
					[td]integer[/td]
					[td]The default widget that has focus when the dropdown is first activated.[/td]
				[/tr]
			
				[tr]
					[td]mode[/td]
					[td]string[/td]
					[td]If this is set to objectsource then the dropdown acts an objectsource client.[/td]
				[/tr]
			
				[tr]
					[td]num_disp[/td]
					[td]integer[/td]
					[td]Number of widgets displayed at once in the dropdown box.[/td]
				[/tr]
			
				[tr]
					[td]objectsource[/td]
					[td]string[/td]
					[td]Represents the widget object source (transfers data to and from server).[/td]
				[/tr]
			
				[tr]
					[td]popup_width[/td]
					[td]integer[/td]
					[td]Width of popup dropdown list.[/td]
				[/tr]
			
				[tr]
					[td]query_multiselect[/td]
					[td]yes/no[/td]
					[td]If set to yes, this indicates that when the dropdown's form is in search (QBF) mode, the dropdown will allow multiple items to be selected, so the user can search for records/objects that match one of several values.  The values will be listed in the dropdown, separated by commas.[/td]
				[/tr]
			
				[tr]
					[td]sql[/td]
					[td]string[/td]
					[td]The SQL used to retrieve the list of items for the dropdown.  It should have between two and five columns, in this order:  label, value, selected (0 or 1, whether the item is selected by default), grp (group name), hidden (0 or 1 to hide the item from the dropdown list but still allow it to be a valid value).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the dropdown.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the dropdown, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the dropdown, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of widget/dropdownitem child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]label[/td]
							[td]string[/td]
							[td]The text label to appear in the dropdown listing.[/td]
						[/tr]
					
						[tr]
							[td]value[/td]
							[td]string[/td]
							[td]The actual data that is stored with this item.[/td]
						[/tr]
					
				[/table]
			
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]ClearItems[/td]
					[td]Deletes all the widgets in the dropdown menu.[/td]
				[/tr]
			
				[tr]
					[td]SetGroup[/td]
					[td]This causes the dropdown to display a different Group of items (use the Group parameter).  It can also restrict what items are displayed based on a minimum or maximum value (use Min and Max parameters).[/td]
				[/tr]
			
				[tr]
					[td]SetItems[/td]
					[td]This specifies a SQL (use \"SQL\" parameter) query to use to re-load the contents of the dropdown.  It should have between two and five columns, in this order: label, value, selected (0 or 1, whether the item is selected by default), grp (group name), hidden (0 or 1 to hide the item from the dropdown list but still allow it to be a valid value).[/td]
				[/tr]
			
				[tr]
					[td]SetValue[/td]
					[td]Changes the child property \"value\".[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the widget (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]DataModify[/td]
					[td]This event occurs when the data is changed (occurs when key press or button changes things).[/td]
				[/tr]
			
				[tr]
					[td]GetFocus[/td]
					[td]This event occurs when the edit bar receives keyboard focus (if the user tabs on to it or clicks on it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]LoseFocus[/td]
					[td]This event occurs when the edit bar of the dropdown loses keyboard focus (if the user tabs off of it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
				[tr]
					[td]RightClick[/td]
					[td]This event occurs when right click is used on the mouse.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Example using child widgets for options

myDropDown1 \"widget/dropdown\"

	{

	x=10;

	y=10;

	width=120;

	bgcolor=\"#dcdcdc\";

	highlight=\"#c0c0c0\";

	fieldname=\"gender\";

	myDropDownChild1 \"widget/dropdownitem\"

		{

		label=\"Male\";

		value=\"0\";

		}

	myDropDownChild2 \"widget/dropdownitem\"

		{

		label=\"Female\";

		value=\"1\";

		}

	}

$Version=2$

//  Example getting options from datasource using a query

myDropDown2 \"widget/dropdown\"

	{

	x=10;

	y=10;

	width=120;

	bgcolor=\"#dcdcdc\";

	highlight=\"#c0c0c0\";

	fieldname=\"gender\";

	sql=\"SELECT :gender_label, :gender_value FROM /my_DB/infotable/rows\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/editbox", null,
"		[b]editbox[/b] :: A visual widget which allows the entry of one line of textual data.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/editbox[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			An editbox form element widget allows the display and data entry of a single line of text (or a numeric value).  When the editbox is clicked (and thus receives keyboard focus), the user can type and erase data inside the widget.  Data which does not fit will cause the text to scroll.  When it receives keyboard focus, the editbox displays a flashing I-beam cursor.  The cursor color uses the data focus color for the page (default is '#000080', or dark blue).


			  When an editbox is part of a form and the form goes into query mode the editbox has several additional features.  First, you can use the * operator as a wildcard (e.g. *ber matches September, October, November, December).  Second you can enter a range if the field is numeric (e.g. 1-100).  Thirdly, a list of criteria can be specified, separated by commas.


		
	[b]Usage:[/b]
	

			The editbox can be placed within any visual widget (or within a form widget).  Editboxes automatically attach themselves to the form which contains them (whether directly or indirectly).  Editboxes cannot contain visual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the contents of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the editbox contents. If neither bgcolor nor background is specified, the editbox is transparent.[/td]
				[/tr]
			
				[tr]
					[td]description_fgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, editbox's value description.  The value description is displayed to the right of the value in the editbox, in parentheses, and in a different color.[/td]
				[/tr]
			
				[tr]
					[td]disabled[/td]
					[td]boolean[/td]
					[td]Specifies wither the edit box can be written in, default is true meaning the edit box cannot be written in.[/td]
				[/tr]
			
				[tr]
					[td]empty_description[/td]
					[td]string[/td]
					[td]The value description to display in the editbox when the editbox is empty.  For instance, \"optional\" or \"required\" or \"type search and press ENTER\" might be commonly used empty_description settings.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]Name of the column in the datasource you want to reference.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]maxchars[/td]
					[td]integer[/td]
					[td]Number of characters to accept from the user.[/td]
				[/tr]
			
				[tr]
					[td]readonly[/td]
					[td]yes/no[/td]
					[td]Set to 'yes' if the user cannot modify the value of the edit box (default 'no').[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]Either \"raised\" or \"lowered\", and determines the style of the border drawn around the editbox.  Default is \"lowered\".[/td]
				[/tr]
			
				[tr]
					[td]tooltip[/td]
					[td]string[/td]
					[td]A string to display in a tooltip when the user rests the mouse pointer over the editbox.  By default, the editbox will display a tooltip of its content if the content is too long to fit in the visible part of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the editbox, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the editbox, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Enable[/td]
					[td]The Enable action allows an edit box to be written in, and takes no parameters x from being written in, and takes no parameters.[/td]
				[/tr]
			
				[tr]
					[td]SetFocus[/td]
					[td]The SetFocus action selects and gives control to an edit box.[/td]
				[/tr]
			
				[tr]
					[td]SetValue[/td]
					[td]The SetValue action modifies the contents of an editbox, and takes a single parameter, 'Value' (string).[/td]
				[/tr]
			
				[tr]
					[td]SetValueDescription[/td]
					[td]This action modifies the \"value description\" of the editbox, and takes a single parameter, 'Description' (string).[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]BeforeDataChange[/td]
					[td]This event occurs before the data changes and can stop the data change event.[/td]
				[/tr]
			
				[tr]
					[td]BeforeKeyPress[/td]
					[td]This event occurs before the key press event is fired and can stop the key press event.[/td]
				[/tr]
			
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the widget. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the widget (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]DataModify[/td]
					[td]This event occurs when the data is changed (occurs when key press or button changes things).[/td]
				[/tr]
			
				[tr]
					[td]EscapePressed[/td]
					[td]This event occurs when the user presses the escape key.[/td]
				[/tr]
			
				[tr]
					[td]GetFocus[/td]
					[td]This event occurs when the editbox receives keyboard focus (if the user tabs on to it or clicks on it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]KeyPress[/td]
					[td]This event occurs when the user presses any key.[/td]
				[/tr]
			
				[tr]
					[td]LoseFocus[/td]
					[td]This event occurs when the editbox loses keyboard focus (if the user tabs off of it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]ReturnPressed[/td]
					[td]This event occurs when the user presses the return key.[/td]
				[/tr]
			
				[tr]
					[td]TabPressed[/td]
					[td]This event occurs when the user presses the tab key.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here is a single lonely edit box.

my_editbox \"widget/editbox\" 

	{

	x=180; 

	y=40; 

	width=120; 

	height=15; 

	style=\"lowered\"; 

	fieldname=\"datasource_fieldname\";

	bgcolor=\"#ffffff\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/execmethod", null,
"		[b]execmethod[/b] :: A nonvisual widget which can call ObjectSystem methods on server objects.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/execmethod[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The execmethod widget is a nonvisual widget which is used to call ObjectSystem methods on objects on the server.This widget may become deprecated in the future when more advanced OSML API widgets become available.


			These widgets are used via the activation of their \"ExecuteMethod\" action.


		
	[b]Usage:[/b]
	

			The execmethod widget, since it is nonvisual, can be placed almost anywhere but is typically placed at the top-level (within an object of type \"widget/page\") for clarity's sake. It has no effect on its container.These widgets cannot contain visual widgets, and since they have no Events, normally contain no connector widgets either.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]method[/td]
					[td]string[/td]
					[td]The name of the method to invoke.[/td]
				[/tr]
			
				[tr]
					[td]object[/td]
					[td]string[/td]
					[td]The ObjectSystem pathname of the object on which the method is to be run.[/td]
				[/tr]
			
				[tr]
					[td]parameter[/td]
					[td]string[/td]
					[td]A string parameter to pass to the method being invoked.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]ExecuteMethod[/td]
					[td]Action causes the widget to execute the method on the server. It can take three parameters, which default to those provided in this widget's properties: \"Objname\", the object path, \"Method\", the method to invoke, and \"Parameter\", the parameter to pass to the method being invoked.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// This execmethod widget is set up to play a sound file (on the server).

mySoundPlayer \"widget/execmethod\"

	{

	object = \"/sys/ossdsp.aud\";

	method = \"Play\";

	parameter = \"/data/Welcome.wav\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/form", null,
"		[b]form[/b] :: A nonvisual container used to group a set of form element widgets into a single record or object

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/form[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The form widget is used as a high-level container for form elements.  Essentially, a form widget represents a single record of data, or the attributes of a single object in the objectsystem (or of a single query result set object). Form widgets must be used in conjunction with an ObjectSource widget, which does the actual transferring of data to and from the server.


			Forms have five different \"modes\"of operation, each of which can be specifically allowed or disallowed for the form.


		

		 No Data - form inactive/disabled, no data viewed/edited.


		 View - data being viewed readonly.


		 Modify - existing data being modified.


		 Query - query criteria being entered (used for query-by-form applications)


		 New - new object being entered/created.


		

			Occasionally, the user may perform an operation which inherently disregards that the form may contain unsaved data.  When this occurs and there is newly created or modified data in the form, the application must ask the user whether the data in the form should be saved or discarded, or whether to simply not even perform the operation in question.  Since DHTML does not inherently have a \"three-way confirm\" message box (with save, discard, and cancel buttons), Centrallix allows a form to specify a \"three-way confirm\" window.  This should be a hidden (visible=no) \"widget/htmlwindow\" object which may contain any content, but should at least contain three buttons named \"_3bConfirmSave\", \"_3bConfirmDiscard\", and \"_3bConfirmCancel\" directly in the htmlwindow.  During a confirm operation, this window will become \"application-modal\"; that is, no other widgets in the application may be accessed by the user until one of the three buttons is pushed.


			Several settings on the form widget control what state, or \"mode\", the form can be in: allow_query, allow_new, allow_modify, allow_view, and allow_nodata.  These can beused to constrain a form to perform a specific task, such as only searching, or only creating new records.  For example, a form with only allow_search enabled will always return to the search (QBF) mode and will never display the searched-for data that is returned in the objectsource.


		
	[b]Usage:[/b]
	

			Although the form widget itself is nonvisual in nature, it can contain visual widgets, including other containers, which may then in turn contain form elements as well.  The form widget may be contained within any widget with a visual container (otherwise, the form elements would not show up on the page).


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]allow_delete[/td]
					[td]yes/no[/td]
					[td]Allow deletion of the displayed record.[/td]
				[/tr]
			
				[tr]
					[td]allow_modify[/td]
					[td]yes/no[/td]
					[td]Allow modification of existing records.[/td]
				[/tr]
			
				[tr]
					[td]allow_new[/td]
					[td]yes/no[/td]
					[td]Allow creation of new records.[/td]
				[/tr]
			
				[tr]
					[td]allow_nodata[/td]
					[td]yes/no[/td]
					[td]Allow the 'no data' state.[/td]
				[/tr]
			
				[tr]
					[td]allow_obscure[/td]
					[td]yes/no[/td]
					[td]Default \"no\".  If this is set to \"yes\", then the form will permit its data to be obscured (hidden from the user, via a window closure or tab page switch) and unsaved at the same time.  If set to \"no\", the form will require the user to save or cancel any data modifications before the form may be obscured.[/td]
				[/tr]
			
				[tr]
					[td]allow_query[/td]
					[td]yes/no[/td]
					[td]Allow query by form.[/td]
				[/tr]
			
				[tr]
					[td]allow_view[/td]
					[td]yes/no[/td]
					[td]Allow viewing of existing data.[/td]
				[/tr]
			
				[tr]
					[td]auto_focus[/td]
					[td]yes/no[/td]
					[td]Default \"yes\".  Whether to automatically place the focus on the first form element in the form when the form transitions to the New, Modify, or Search mode for any reason other than the user manually putting focus on one of the form elements.[/td]
				[/tr]
			
				[tr]
					[td]confirm_delete[/td]
					[td]yes/no[/td]
					[td]Whether to pop up an OK/Cancel message box asking the user whether he/she is sure the record should be deleted.[/td]
				[/tr]
			
				[tr]
					[td]comfirm_discard[/td]
					[td]yes/no[/td]
					[td]Set to true when the discard button of the 3 buttom confirm window (3bconfirmwindow) is pressed.[/td]
				[/tr]
			
				[tr]
					[td]enter_mode[/td]
					[td]string[/td]
					[td]Can be \"save\" (default), \"nextfield\", or \"lastsave\".  Controls what to do when the user presses ENTER while in a form element in this form.  \"save\" means to save the record immediately.  \"nextfield\" means to move to the next form field, as if TAB were pressed instead.  \"lastsave\" means to move to the next form field, but once ENTER is pressed on the very last field, to then save the record.  Most people prefer \"save\" for this, since that is consistent with how normal applications work, but people in finance or accounting work often prefer \"lastsave\" since it allows them to perform high-speed data entry using a numeric keypad.[/td]
				[/tr]
			
				[tr]
					[td]interlock_with[/td]
					[td]string[/td]
					[td]Stores wither this form can be grouped with other forms.[/td]
				[/tr]
			
				[tr]
					[td]MultiEnter[/td]
					[td]integer[/td]
					[td]Enable MultiEnter.[/td]
				[/tr]
			
				[tr]
					[td]next_form[/td]
					[td]string[/td]
					[td]Specifies another form to transfer focus to when the user presses TAB at the end of the form.[/td]
				[/tr]
			
				[tr]
					[td]next_form_within[/td]
					[td]string[/td]
					[td]Similar to \"next_form\", but searches for the next form after this one within a given widget or component.  Transitioning from one component context to another is permitted (you could specify a \"widget/page\" widget here, and cause focus to transfer to another form in the same application, regardless of level of component nesting; this may or may not be desired behavior).[/td]
				[/tr]
			
				[tr]
					[td]objectsource[/td]
					[td][/td]
					[td]Represents the widget object source (transfers data to and from server).[/td]
				[/tr]
			
				[tr]
					[td]ReadOnly[/td]
					[td]integer[/td]
					[td]Allow changes.[/td]
				[/tr]
			
				[tr]
					[td]TabMode[/td]
					[td]string[/td]
					[td]How to react to a tab in a control.[/td]
				[/tr]
			
				[tr]
					[td]tab_revealed_only[/td]
					[td]yes/no[/td]
					[td]When tabbing between form elements, if this is set to \"yes\", do not transfer focus to form elements that are not currently visible to the user.[/td]
				[/tr]
			
				[tr]
					[td]_3bconfirmwindow[/td]
					[td]string[/td]
					[td]The name of the window to use for all 3-way confirm operations (save/discard/cancel).[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of formelement child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]fieldname[/td]
							[td]string[/td]
							[td]Fieldname (from the dataset) to bind this element to.[/td]
						[/tr]
					
				[/table]
			
			(of formstatus child widgets)
			
				[i]none currently available[/i]
			
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Clear[/td]
					[td]Clears the form to a 'no data'state.[/td]
				[/tr]
			
				[tr]
					[td]Delete[/td]
					[td]Deletes the current object displayed.[/td]
				[/tr]
			
				[tr]
					[td]Discard[/td]
					[td]Cancels an edit of form contents.[/td]
				[/tr]
			
				[tr]
					[td]Disable[/td]
					[td]Prevents interaction with the entire form.[/td]
				[/tr]
			
				[tr]
					[td]Edit[/td]
					[td]Allows editing of form's contents.[/td]
				[/tr]
			
				[tr]
					[td]Enable[/td]
					[td]Allows interaction with the form.[/td]
				[/tr]
			
				[tr]
					[td]First[/td]
					[td]Moves the form to the first object in the objectsource.[/td]
				[/tr]
			
				[tr]
					[td]Last[/td]
					[td]Moves the form to the last object in the objectsource.[/td]
				[/tr]
			
				[tr]
					[td]New[/td]
					[td]Allows creation of new form contents.[/td]
				[/tr]
			
				[tr]
					[td]Next[/td]
					[td]Moves the form to the next object in the objectsource.[/td]
				[/tr]
			
				[tr]
					[td]Prev[/td]
					[td]Moves the form to the previous object in the objectsource.[/td]
				[/tr]
			
				[tr]
					[td]Query[/td]
					[td]Allows entering of query criteria.[/td]
				[/tr]
			
				[tr]
					[td]QueryExec[/td]
					[td]the query and returns data.[/td]
				[/tr]
			
				[tr]
					[td]QueryToggle[/td]
					[td]Either executes a query or allows one to be made depending on the previous state.[/td]
				[/tr]
			
				[tr]
					[td]Save[/td]
					[td]Saves the form's contents.[/td]
				[/tr]
			
				[tr]
					[td]SetValue[/td]
					[td]Modifies a field and puts the form into a new modify or search mode if appropriate.[/td]
				[/tr]
			
				[tr]
					[td]Submit[/td]
					[td]Attemps to save data, if there is an error it returns false.[/td]
				[/tr]
			
				[tr]
					[td]test3bconfirm[/td]
					[td]Debugging tool to test the 3bcomfirm method (only use if you know what this method is).[/td]
				[/tr]
			
				[tr]
					[td]View[/td]
					[td]Changes the form to view mode.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]BeforeSave[/td]
					[td]This event occurs just before the form is enabled after a save.[/td]
				[/tr]
			
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when a child controller changes its data.[/td]
				[/tr]
			
				[tr]
					[td]DataDeleted[/td]
					[td]This event occurs when the delete function is successfully completed.[/td]
				[/tr]
			
				[tr]
					[td]DataLoaded[/td]
					[td]This event occurs when it is confirmed that the method search for object is available.[/td]
				[/tr]
			
				[tr]
					[td]DataSaved[/td]
					[td]This event occurs when the save action was successful.[/td]
				[/tr]
			
				[tr]
					[td]Discard[/td]
					[td]This event occurs after the discard action is performed.[/td]
				[/tr]
			
				[tr]
					[td]ModeChange[/td]
					[td]This event occurs at the end of the change mode action (always with status change event).[/td]
				[/tr]
			
				[tr]
					[td]Modify[/td]
					[td]This event occurs if and only if there was a mode change and the new mode is the 'Modify' state (always occurs with both a statusChange event and a ModeChange Event).[/td]
				[/tr]
			
				[tr]
					[td]New[/td]
					[td]This event occurs if and only if there was a mode change and the new mode is the 'New' state (always occurs with both a statusChange event and a ModeChange Event).[/td]
				[/tr]
			
				[tr]
					[td]NoData[/td]
					[td]This event occurs if and only if there was a mode change and the new mode is the 'NoData' state (always occurs with both a statusChange event and a ModeChange Event.[/td]
				[/tr]
			
				[tr]
					[td]Query[/td]
					[td]This event occurs if and only if there was a mode change and the new mode is the 'Query' state (always occurs with both a statusChange event and a ModeChange Event.[/td]
				[/tr]
			
				[tr]
					[td]StatusChange[/td]
					[td]The even occurs when the form changes modes or when a child control changes its data.[/td]
				[/tr]
			
				[tr]
					[td]View[/td]
					[td]This event occurs if and only if there was a mode change and the new mode is the 'View' state (always occurs with both a statusChange event and a ModeChange Event.[/td]
				[/tr]
			
		[/table]
	
	[b]Client Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]is_discardable[/td]
					[td]true/false[/td]
					[td]Whether the form has a status or modifications that can be discarded / canceled.[/td]
				[/tr]
			
				[tr]
					[td]is_editable[/td]
					[td]true/false[/td]
					[td]True if the form is in a state where the data can be edited (but isn't currently being edited).[/td]
				[/tr]
			
				[tr]
					[td]is_newable[/td]
					[td]true/false[/td]
					[td]True if the form is in a state where a new record can be created.[/td]
				[/tr]
			
				[tr]
					[td]is_queryable[/td]
					[td]true/false[/td]
					[td]Whether the form can be placed into \"QBF\" (query by form) mode so that the user can enter query criteria.[/td]
				[/tr]
			
				[tr]
					[td]is_queryexecutable[/td]
					[td]true/false[/td]
					[td]Whether the form is in \"QBF\" (query by form) mode so that the query can actually be run.[/td]
				[/tr]
			
				[tr]
					[td]is_savable[/td]
					[td]true/false[/td]
					[td]Whether the form has data in it that can be saved with the Save action.[/td]
				[/tr]
			
				[tr]
					[td]lastrecid[/td]
					[td]integer[/td]
					[td]The number of the last record in the query results, starting with '1'.  Null if no data is available or if the last record has not yet been determined.[/td]
				[/tr]
			
				[tr]
					[td]recid[/td]
					[td]integer[/td]
					[td]The current record in the data source being viewed, starting with '1'.  Null if no data is available.[/td]
				[/tr]
			
		[/table]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

form1 \"widget/form\"

	{

	//Form elements here

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/formstatus", null,
"		[b]formstatus[/b] :: A specialized visual widget used to display the current mode of a form widget.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/formstatus[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			Many times with multi-mode forms like those offered by Centrallix, the end-user can become confused as to what the form is currently \"doing\" (for instance, is a blank form with a blinking cursor in a \"new\" state or \"enter query\" state?).  Centrallix helps to address this issue using the form status widget. A form status widget displays the current mode of operation that a form is in, as well as whether the form is busy processing a query, save, or delete operation.  This clear presentation of the form's mode is intended to clear up any confusion created by a multi-mode form.  This widget is a special-case form element.


		
	[b]Usage:[/b]
	

			The form status widget can only be used either directly or indirectly within a form widget.  It can contain no visual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]The name of the form that this instance of form status corresponds to.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]Sets the style of the form widget.  Can be \"small\", \"large\", or \"largeflat\".[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]Allows you to set x position in pixels relative to the window.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Allows you to set y position in pixels relative to the window.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

formstatus \"widget/formstatus\" 

    {

    x=5; y=450;

    style=\"largeflat\";

    }

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/frameset", null,
"		[b]frameset[/b] :: A visual container used to create a DHTML frameset within which page widgets can be placed

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/frameset[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The frameset widget provides the ability to construct a page consisting of multiple (potentially resizeable) frames.It is one of two possible top-level widgets (the page widget is the other one).  Framesets can consist of one or more frames, arranged either in rows or columns.


		
	[b]Usage:[/b]
	

			The frameset can either be a top-level widget, or can be contained within a frameset (for subframes).The frameset widget should not be used anywhere else in an application. The frameset should contain only other framesets and/or pages.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]borderwidth[/td]
					[td]integer[/td]
					[td]Number of pixels wide the border(s) between the frame(s) are. Can be set to zero.[/td]
				[/tr]
			
				[tr]
					[td]direction[/td]
					[td]string[/td]
					[td]Whether the frames are arranged in rows or columns.  Set this attribute to \"rows\" or \"columns\"respectively.[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]Title of the frameset for an html page.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of any child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]framesize[/td]
							[td]integer/string[/td]
							[td]Large this frame will be.Can either be expressed as an integer, which represents \"50%\".[/td]
						[/tr]
					
						[tr]
							[td]marginwidth[/td]
							[td]integer[/td]
							[td]Width, in pixels, of the margins of the frame.[/td]
						[/tr]
					
				[/table]
			
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Example of a page with three frames on it.

//

BigFrameset \"widget/frameset\"

	{

	title = \"MyTitle\";

	direction = \"rows\";

	borderwidth = 3;

	TopFrameset \"widget/frameset\"

		{

		framesize = \"40%\";

		direction = \"columns\";

		borderwidth = 3;

		TopLeftDocument \"widget/page\"

			{

			framesize = \"50%\";

			}

		TopRightDocument \"widget/page\"

			{

			framesize = \"50%\";

			}

		}

	BottomDocument \"widget/page\"

		{

		framesize = \"60%\";

		}

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/hbox", null,
"		[b]hbox[/b] :: Container which automatically positions its children

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/hbox[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			An autolayout widget with style set to \"hbox\".  See \"widget/autolayout\".


		
");
	
insert into topic values(null, @newid, "widget/hints", null,
"		[b]hints[/b] :: Contains default values and modifier settings for various components

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/hints[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The hints widget stores default values and other component modifying properties.


		
	[b]Usage:[/b]
	

			The hints widget can be placed inside of any visual component. Hints do not contain visual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]allowchars[/td]
					[td]string[/td]
					[td]Defines a set of acceptable characters (e.g. allowchars=\"0123456789\").[/td]
				[/tr]
			
				[tr]
					[td]badchars[/td]
					[td]string[/td]
					[td]Defines a set of unacceptable characters.[/td]
				[/tr]
			
				[tr]
					[td]constraint[/td]
					[td]object[/td]
					[td]Defines a condition that the parent value must meet to be valid.[/td]
				[/tr]
			
				[tr]
					[td]default[/td]
					[td]object[/td]
					[td]A string or integer that specifies the initial, default, starting value for the component which containing this widget/hints widget.[/td]
				[/tr]
			
				[tr]
					[td]enumlist[/td]
					[td]object[/td]
					[td]A list of string values that give all the possible values of th the parent.[/td]
				[/tr]
			
				[tr]
					[td]enumquery[/td]
					[td]string[/td]
					[td]A sql query that is used to get the required value list.[/td]
				[/tr]
			
				[tr]
					[td]format[/td]
					[td]string[/td]
					[td]The way in which data is presented to the user.[/td]
				[/tr]
			
				[tr]
					[td]groupid[/td]
					[td]integer[/td]
					[td]A numeric identification for a group of attributes.[/td]
				[/tr]
			
				[tr]
					[td]groupname[/td]
					[td]string[/td]
					[td]A string identification for a group of attributes.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Determines rows of the widget.[/td]
				[/tr]
			
				[tr]
					[td]length[/td]
					[td]integer[/td]
					[td]Length in characters that can be entered into this field.[/td]
				[/tr]
			
				[tr]
					[td]max[/td]
					[td]object[/td]
					[td]A string or integer that stores the maximum value of the attributes of its parent widget.[/td]
				[/tr]
			
				[tr]
					[td]min[/td]
					[td]object[/td]
					[td]A string or integer that stores the minimum value of the attributes of its parent widget.[/td]
				[/tr]
			
				[tr]
					[td]readonlybits[/td]
					[td]object[/td]
					[td]If the field is a bitmask, determines which ones are read only.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]Optional: contains a combination of 1 or more items following set {readonly, alwaysdef} separated by a comma if multiple items are chosen.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Number of characters in the field shown to the user at once.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here is a checkbox which uses widget/hints.

f_trx_mod \"widget/checkbox\"

{

	width=16;

	y=4;

	readonly=yes;

	fieldname=\"a_modified\";

	f_trx_mod_h \"widget/hints\"

	{

		style=readonly,alwaysdef;

		default=1;

	}

}		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/html", null,
"		[b]html[/b] :: A miniature HTML browser control, capable of viewing and navigating simple web documents.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/html[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The HTML area widget provides a way to insert a plain HTML document into a Centrallix generated page, either in-flow (static) or in its own separate layer that can be reloaded at will (dynamic). The HTML document can either be given in a property of the widget or can be referenced so that the HTML is read from an external document.


			The HTML area widget also can act as a mini-browser -- clicking on hyper-text links in the visible document will by default cause the link to be followed, and the new document to be displayed in the HTML area (if the HTML area is dynamic).


			Dynamic HTML areas do double-buffering and allow for transitions (fades) of various types.


		
	[b]Usage:[/b]
	

			The HTML area widget can be placed within any other widget that has a visible container (such as panes, pages, tab pages, etc.).


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]content[/td]
					[td]string[/td]
					[td]Static contents for the HTML area. Usually used in lieu of \"source\" (see below).[/td]
				[/tr]
			
				[tr]
					[td]epilogue[/td]
					[td]string[/td]
					[td]HTML source to be inserted at the end of the HTML document.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]height, in pixels, of the HTML area as a whole.[/td]
				[/tr]
			
				[tr]
					[td]mode[/td]
					[td]string[/td]
					[td]Either \"static\" or \"dynamic\", and determines whether the HTML area is in-flow (\"static\") can be positioned and reloaded at-will.[/td]
				[/tr]
			
				[tr]
					[td]prologue[/td]
					[td]string[/td]
					[td]HTML source to be inserted at the beginning of the HTML document.[/td]
				[/tr]
			
				[tr]
					[td]source[/td]
					[td]string[/td]
					[td]The objectsystem path or URL containing the document to be loaded into the HTML as  local server.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the HTML area as a whole.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]Dynamic HTML areas, the x coordinate on the container of its upper left corner.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Dynamic HTML areas, the y coordinate on the container of its upper left corner.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]LoadPage[/td]
					[td]Loadpage action takes two parameters. \"Source\" contains the URL for the new page to be loaded into the HTML area.The optional parameter \"Transition\" indicates the type of fade to be used between one page and the next.Currently supported values are \"pixelate\", \"rlwipe\", and \"lrwipe\".[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

HTMLArea \"widget/html\"

	{

	mode = \"dynamic\";

	x = 2; y = 2;

	width = 478; height = 396;

	source = \"http://localhost:800/index.html\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/image", null,
"		[b]image[/b] :: A picture (image).

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/image[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The image widget displays a picture (image).


			This widget can be a form element, in which case it will display an image as specified by the data from the form.


		
	[b]Usage:[/b]
	

			The image widget can be placed inside any container that allows for visual widgets.  Only connectors may be placed inside it.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]aspect[/td]
					[td]sting[/td]
					[td]Determines wither the image can be stretched or must stay its original aspect ration.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]If using a form for the image path, the name of the column in the datasource you want to reference.[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]If using a form for the image path, this is the name of the form.  If left unspecified, and fieldname is supplied, the default will be whatever form the image is inside.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the image.[/td]
				[/tr]
			
				[tr]
					[td]text[/td]
					[td]string[/td]
					[td]The words that come up if the image object cannot find its source and display an image.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the image.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the image, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the image, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]LoadImage[/td]
					[td]Displays the image to user.[/td]
				[/tr]
			
				[tr]
					[td]Offset[/td]
					[td]Reloads the image with new offset values.[/td]
				[/tr]
			
				[tr]
					[td]Scale[/td]
					[td]Reloads the image with a new scale value.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the checkbox. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
		[/table]
	
	[b]Client Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]scale[/td]
					[td]integer[/td]
					[td]Determines how much the image grows or shrinks compared to the original.[/td]
				[/tr]
			
				[tr]
					[td]source[/td]
					[td]string[/td]
					[td]An OSML pathname for the location of the image (such as a png or jpg file).[/td]
				[/tr]
			
				[tr]
					[td]xoffset[/td]
					[td]integer[/td]
					[td]Distance it is along the x axis from its parent widget.[/td]
				[/tr]
			
				[tr]
					[td]yoffset[/td]
					[td]integer[/td]
					[td]Distance it is along the y axis from its parent widget.[/td]
				[/tr]
			
		[/table]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, @newid, "widget/imagebutton", null,
"		[b]imagebutton[/b] :: A button widget which uses a set of images to control its appearance.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/imagebutton[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The ImageButton widget provides a clickable button that is comprised of a set of two or three images.The first image is shown normally when the button is idle, the second when the button is pointed-to, and the third image is shown when the button is actually clicked.  This provides a \"tri-state\" appearance much like that provided by buttons in modern user interfaces, although the button can be two-state, with just an \"unclicked\" and \"clicked\" version of the image.


			The images are automatically swapped out when the appropriate mouse events occur on the button.


			ImageButtons can also be disabled, and a \"disabled\" image is displayed at that time.


			Unlike the textbutton, there is no 'tristate' property for an imagebutton; to make an imagebutton tri-state (different image when idle vs. when pointed to), use a 'pointimage', otherwise do not specify 'pointimage'.


		
	[b]Usage:[/b]
	

			The ImageButton can be placed inside any visible container, but only nonvisual widgets can be placed within it.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]clickimage[/td]
					[td]string[/td]
					[td]The ObjectSystem pathname of the image to be shown when the user clicks the imagebutton.  Defaults to 'image' if not specified.[/td]
				[/tr]
			
				[tr]
					[td]disabledimage[/td]
					[td]string[/td]
					[td]The ObjectSystem pathname of the image to be shown when the imagebutton is disabled.  Defaults to 'image' if not specified.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the image button.[/td]
				[/tr]
			
				[tr]
					[td]image[/td]
					[td]string[/td]
					[td]The pathname of the image to be shown when the button is \"idle\".[/td]
				[/tr]
			
				[tr]
					[td]pointimage[/td]
					[td]string[/td]
					[td]The pathname of the image to be shown when the button is pointed-to.  Defaults to  the 'image' if not specified.[/td]
				[/tr]
			
				[tr]
					[td]repeat[/td]
					[td]yes/no[/td]
					[td]Whether to repeat the click event multiple times while the user holds down the button.[/td]
				[/tr]
			
				[tr]
					[td]tooltip[/td]
					[td]string[/td]
					[td]The text that appears when the courser hovers over the image button.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the image button.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Disable[/td]
					[td]This action causes the image button to enter its 'disabled' state, displaying the 'disabledimage' if specified.[/td]
				[/tr]
			
				[tr]
					[td]Enable[/td]
					[td]This action causes the image button to enter its 'enabled' state, possibly changing its appearance if a 'disabledimage' was explicitly specified.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the button. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

MyButton \"widget/imagebutton\"

	{

	x = 10; y = 10;

	width = 50;

	height = 20;

	// No pointimage for this one, thus not \"tristate\".

	image = \"/images/default.png\"

	clickimage = \"/images/clicked.png\"

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/label", null,
"		[b]label[/b] :: Form element capable of displaying text in a child window

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/label[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The label widget is used to display a non-editable string of text.  It displays as a simple label.


			A label can be used as a form element; to do this, specify a fieldname, and optionally a form.


			A label can behave like a \"link\": to do this, specify a point_fgcolor and a click_fgcolor.


		
	[b]Usage:[/b]
	

			The label widget can be placed inside of any visual container, and will attach itself to any form widget that contains it (whether directly or indirectly).  Labels may not contain visual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]align[/td]
					[td]string[/td]
					[td]Describes how the text should align in the label (e.g. right).[/td]
				[/tr]
			
				[tr]
					[td]allow_break[/td]
					[td]string[/td]
					[td]It determines if the text can wrap around something.[/td]
				[/tr]
			
				[tr]
					[td]click_fgcolor[/td]
					[td]string[/td]
					[td]The color (named or #numeric) of the text in the label when the user clicks on the label.[/td]
				[/tr]
			
				[tr]
					[td]fgcolor[/td]
					[td]string[/td]
					[td]The color (named or #numeric) of the text in the label.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]The name of the column in the datasource that will be used to supply the text of the label.[/td]
				[/tr]
			
				[tr]
					[td]font_size[/td]
					[td]integer[/td]
					[td](e.g. 16, default: 12).[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]The name of the form that this label is associated with.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]string[/td]
					[td]The height of the label.[/td]
				[/tr]
			
				[tr]
					[td]overflow_ellipsis[/td]
					[td]string[/td]
					[td]If text is no longer able to display shows an ellipsis.[/td]
				[/tr]
			
				[tr]
					[td]point_fgcolor[/td]
					[td]string[/td]
					[td]The color (named or #numeric) of the text in the label when the user hovers the mouse over the label.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td](e.g. bold).[/td]
				[/tr]
			
				[tr]
					[td]text[/td]
					[td]string[/td]
					[td]The text that the label is to display.[/td]
				[/tr]
			
				[tr]
					[td]tooltip[/td]
					[td]string[/td]
					[td]The text displayed when the courser hovers over the widget.[/td]
				[/tr]
			
				[tr]
					[td]valign[/td]
					[td]string[/td]
					[td]Describes how the text should align vertically within the label (e.g. top, middle, or bottom).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]The width of the label.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the checkbox, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the checkbox, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetValue[/td]
					[td]Sets the value property to the given parameter.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the checkbox. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the checkbox (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
		[/table]
	
	[b]Client Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]value[/td]
					[td]string[/td]
					[td]This property allows a runclient() expression to dynamically supply the text to display in the label.[/td]
				[/tr]
			
		[/table]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here is a label.

f_trx_mod_l \"widget/label\" { width=86; text=\"User Edit?\"; align=right; font_size=16; }



		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/menu", null,
"		[b]menu[/b] :: A visual pop-up or drop-down menu widget.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/menu[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The menu widget is used to create popup menus, drop-down menus, and menu bars.  Menu widgets consist of a series of menuitem widgets which compose a menu.


			Menu items are generally linked to actions on a page via the use of connectors.  Simply place the connector inside the menu item widget.


			Note: as of the time of writing, menu widgets were not yet properly functional.


		
	[b]Usage:[/b]
	

			Menus can be placed inside of any visual container. However, be aware that the menu will be clipped by its container, so placing them at the top-level can be of an advantage. Menu widgets contain menuitem widgets, which are also described in this section.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]active_background[/td]
					[td]string[/td]
					[td]A background image for a menu item that is selected (clicked on).[/td]
				[/tr]
			
				[tr]
					[td]active_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as a selected (clicked) item's background.  If neither active_bgcolor nor active_background are specified, the 'highlight' color or background is used instead.[/td]
				[/tr]
			
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the menu.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the menu's background.  If neither bgcolor nor background are specified, the menu is transparent.[/td]
				[/tr]
			
				[tr]
					[td]column_width[/td]
					[td]integer[/td]
					[td]The width of the data elements in the menu.[/td]
				[/tr]
			
				[tr]
					[td]direction[/td]
					[td]string[/td]
					[td]Either \"horizontal\" or \"vertical\" (default), and determines whether the menu is a drop-down/popup (vertical) or a menubar (horizontal).[/td]
				[/tr]
			
				[tr]
					[td]fgcolor[/td]
					[td]string[/td]
					[td]A color for the menu's text.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the menu, for menus with a direction of 'horizontal'.[/td]
				[/tr]
			
				[tr]
					[td]highlight_background[/td]
					[td]string[/td]
					[td]A background image for a menu item that is highlighted (pointed at).[/td]
				[/tr]
			
				[tr]
					[td]highlight_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as a highlighted (pointed-at) item's background.  If neither highlight_bgcolor nor highlight_background are specified, the standard color or background is used instead.[/td]
				[/tr]
			
				[tr]
					[td]popup[/td]
					[td]yes/no[/td]
					[td]Default \"no\".  Popup menus disappear after an item on them is selected, whereas fixed menus remain visible (such as for menubars).[/td]
				[/tr]
			
				[tr]
					[td]row_height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the menu items in a menu.[/td]
				[/tr]
			
				[tr]
					[td]shadow_offset[/td]
					[td]integer[/td]
					[td]Determines how far the shadow translates from the menu.[/td]
				[/tr]
			
				[tr]
					[td]shadow_radius[/td]
					[td]integer[/td]
					[td]Describes in a ratio how much larger or smaller the size of the shadow is compared to the window.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the menu.  For menus with a direction of 'vertical', an unspecified width is determined dynamically based on the menu contents.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the menu, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the menu, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of widget/menuitem child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]checked[/td]
							[td]yes/no[/td]
							[td]Optionally, a checkbox can be displayed to the left of the menu item when 'checked' is specified.  In this case, the 'value' can also be a runclient() expression that controls whether the menu item is checked.[/td]
						[/tr]
					
						[tr]
							[td]icon[/td]
							[td]string[/td]
							[td]Optionally, the pathname of an image file to display to the left of the menu item.[/td]
						[/tr]
					
						[tr]
							[td]label[/td]
							[td]string[/td]
							[td]The text to appear on the menu item.[/td]
						[/tr]
					
						[tr]
							[td]onright[/td]
							[td]yes/no[/td]
							[td]If set to \"yes\", then the menu item will be displayed on the righthand side of a horizontal menu bar (e.g., for having \"File\" \"Edit\" \"Tools\" on the left, and \"Help\" on the far right).[/td]
						[/tr]
					
						[tr]
							[td]value[/td]
							[td]string[/td]
							[td]The 'value' of the menu item, passed to the Selected event, below.  If not specified, it defaults to the name of the widget (not its label).[/td]
						[/tr]
					
				[/table]
			
			(of widget/menusep child widgets)
			
				[i]none currently available[/i]
			
			(of widget/menutitle child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]label[/td]
							[td]string[/td]
							[td]The text to appear on the menu title.[/td]
						[/tr]
					
				[/table]
			
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Activate[/td]
					[td]This action causes a popup-type menu to become visible and appear at a selected (x,y) position on the page.  When the user selects an item on the menu or clicks elsewhere on the page, the menu then disappears.  Takes two parameters - X and Y, the (integer) positions on the page for the menu to appear.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the an item in the menu which is not a submenu is changed.[/td]
				[/tr]
			
				[tr]
					[td]GetFocus[/td]
					[td]This event occurs when the menu which previously was not clicked is clicked.[/td]
				[/tr]
			
				[tr]
					[td]LoseFocus[/td]
					[td]This event occurs when the menu which previously was clicked is unselected.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]Select[/td]
					[td]This event fires when a menu item is selected.  It can be placed in the menu itself, or inside the menu item widget.  When on a menu item, it only fires when that item is selected.  When on a menu, it passes the selected item's value as a parameter named 'Item' (string).[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here's a sample menu with three buttons.

myMenu \"widget/menu\"

	{

	x = 10; y = 10; width = 72;

	direction = \"vertical\"; popup = yes;

	bgcolor=\"#808080\";

	

	m1 \"widget/menuitem\" { label=\"One\"; }

	m2 \"widget/menuitem\" { label=\"Two\"; }

	m3 \"widget/menuitem\" { label=\"Three\"; }

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/osrc", null,
"		[b]osrc[/b] :: A nonvisual widget which handles data communication between forms/tables and the Centrallix server.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/osrc[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The objectsource (osrc) widget lies at the core of Centrallix's ability to dynamically exchange data between the server and client. This widget implements a form of \"replication\" by maintaining a replica of a small segment of data in the user agent.


			Both form and dynamic table widgets interact with the objectsource nonvisual widget to acquire data, update data, create data, and delete data. In fact, it is possible for more than one form and/or table to be connected with a given objectsource, to perform a variety of functions.


			Objectsources offer synchronization with other objectsources via the Sync and DoubleSync actions (see below) or rule-based connnectivity (see widget/rule).  These actions allow the application to contain multiple objectsources with primary key / foreign key relationships, and to have those objectsources automatically stay in synchronization with each other based on those relationships.


			An objectsource may also be used to run a query which does not return any rows, such as an insert, update, or delete query.  Under normal data maintenance conditions such queries are not needed as the objectsource handles those operations internally, however.


		
	[b]Usage:[/b]
	

			Objectsource widgets are often used at the top-level of an application, within a \"widget/page\" object, but can be used anywhere.  The forms/tables that use an objectsource are generally placed within that objectsource.  A page may have more than one objectsource widget.


			Even though the objectsource is nonvisual, it is, like the form widget, a container, and can (should!) contain other visual and nonvisual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]autoquery[/td]
					[td]string[/td]
					[td]One of oneachreveal, never, onload, onfirstreveal.  (note: this autoquery setting is different from, but related to, the \"widget/rule\" \"autoquery\").  \"onload\" means that the osrc should run its query automatically when the .app containing this osrc is loaded by the user.  \"onfirstreveal\" means that the osrc should run its query automatically when the data (e.g., a table or form) is first displayed to the user (e.g., the containing childwindow becomes visible or tabpage is selected).  \"oneachreveal\" means to do so *each* time the data is displayed to the user.  \"never\" means that the osrc should never query automatically by itself, but it may be triggered by a connector (QueryParam, QueryText, etc.) or by a widget/rule of type osrc_relationship.  Important Note:  If you expect to normally trigger the osrc via a relationship or via QueryParam, it is often *best* to set autoquery to 'never'.  Otherwise, unexpected results can sometimes occur.[/td]
				[/tr]
			
				[tr]
					[td]baseobj[/td]
					[td]string[/td]
					[td]If inserts and deletes are to function, the ObjectSystem pathname in which those inserts and deletes should occur.  This should be one of the objects specified in the FROM clause of the SQL statement.[/td]
				[/tr]
			
				[tr]
					[td]filter[/td]
					[td]string[/td]
					[td]Set of search parameters.[/td]
				[/tr]
			
				[tr]
					[td]indicates_activity[/td]
					[td]yes/no[/td]
					[td]Whether normal query activity on this objectsource indicates to the server that the user is active.  Some objectsources (in conjunction with a widget/timer) regularly refresh their data; for those, indicates_activity can be set to \"no\" so that those refreshes don't cause a user's session to never time-out.[/td]
				[/tr]
			
				[tr]
					[td]key_objname[/td]
					[td]string[/td]
					[td]The name of the object in the SQL query that contains the primary key for the query.[/td]
				[/tr]
			
				[tr]
					[td]readahead[/td]
					[td]integer[/td]
					[td]Represents the number to fetch from the server when more records are needed from the server (such as when a form requests that the Current Record be a record beyond the end of the current replica contents).[/td]
				[/tr]
			
				[tr]
					[td]receive_updates[/td]
					[td]yes/no[/td]
					[td]** This feature currently disabled in Centrallix 0.9.1 **  Default \"no\".  If set to \"yes\", the objectsource will ask the server to send it updates on any changes that occur on the server side (i.e., if the changes were made by another objectsource or by another user, they would be automatically refreshed into this objectsource in near real-time).[/td]
				[/tr]
			
				[tr]
					[td]refresh_interval[/td]
					[td]integer[/td]
					[td]The time between the data refreshing, if set to 0 it does not automatically refresh.[/td]
				[/tr]
			
				[tr]
					[td]replicasize[/td]
					[td]integer[/td]
					[td]Represents the number of records to store in its replica.  This value should be larger than the maximum number of records that will be displayed at any one time.  At times, Centrallix may increase the number of records cached on the client beyond this number.[/td]
				[/tr]
			
				[tr]
					[td]revealed_only[/td]
					[td]string[/td]
					[td]Acts as a boolean and delays query until the osrc is visable (if \"true\").[/td]
				[/tr]
			
				[tr]
					[td]scrollahead[/td]
					[td]integer[/td]
					[td]Similar to readahead, but relates to how many records should be fetched when more records are needed from the server to fulfill a request to display more data, such as a table widget scrolling without changing the current record.  By default this is set to the value of 'readahead'.[/td]
				[/tr]
			
				[tr]
					[td]send_updates[/td]
					[td]yes/no[/td]
					[td]Default \"yes\".  If set to \"no\", the objectsource will not relay any updates (modifications, etc.) that the user makes to the server.[/td]
				[/tr]
			
				[tr]
					[td]sql[/td]
					[td]string[/td]
					[td]The SQL statement used to retrieve the data.[/td]
				[/tr]
			
				[tr]
					[td]use_having_clause[/td]
					[td]yes/no[/td]
					[td]Default \"no\".  If set to \"yes\", then queries will be performed using a HAVING clause instead of a WHERE clause.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of rule with ruletype=osrc_relationship (replaces the deprecated: sync and double sync) child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]autoquery[/td]
							[td]true/false[/td]
							[td]When autoquery is set to true, when the master changes, the slave automatially requeries (otherwise have to explicitly call requery or refresh on the slave osrc).  When autoquery is false, it causes relationships to be in enforced, but doesn't cause a re-query when the master's osrc refreshes / requeries.[/td]
						[/tr]
					
						[tr]
							[td]is_slave[/td]
							[td]yes/no[/td]
							[td]Defaults to 'yes'.  If it is set to 'no', then this osrc is set up to be a master of the target, otherwise the default is for it to be a slave of the target.[/td]
						[/tr]
					
						[tr]
							[td]key_#[/td]
							[td]string[/td]
							[td]The field names in this objectsource to be used as the key value for the relationship.  Keys can be key_1 through key_5.[/td]
						[/tr]
					
						[tr]
							[td]master_norecs_action[/td]
							[td]string[/td]
							[td]One of: allrecs, norecs, or sameasnull (the default).  If the master osrc has no data loaded in it (no records), this determines how the slave (child) osrc behaves.  'allrecs' means to query for all records that match the sql query for the osrc, without additional constraints.  'norecs' means the slave (child) will be empty.  'sameasnull' means to handle it as if the key value in the master were NULL (see master_null_action).[/td]
						[/tr]
					
						[tr]
							[td]master_null_action[/td]
							[td]string[/td]
							[td]One of: allrecs, norecs, or nullisvalue (the default).  If the master osrc has a NULL value for its key (as defined by key_#/target_key_#), this setting determines how the slave (child) osrc behaves.  'allrecs' and 'norecs' have the same meaning as for master_norecs_action.  'nullisvalue' means to treat the NULL as a value, and query for slave records that have a matching NULL values in their key (as defined by key_#/target_key_#).  If no slave records have null values in the key, then 'nullisvalue' and 'norecs' have the same result.[/td]
						[/tr]
					
						[tr]
							[td]target[/td]
							[td]string[/td]
							[td]The target objectsource that this objectsource will be related to.[/td]
						[/tr]
					
						[tr]
							[td]target_key_#[/td]
							[td]string[/td]
							[td]The field names in the target objectsource to be used for the key value for the relationship (where # is an integer 1 through 5).  These keys can be target_key_1 through target_key_5.[/td]
						[/tr]
					
				[/table]
			
			(of rule with ruletype=osrc_key child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]counter_attribute[/td]
							[td]string[/td]
							[td]The field in the specified 'osrc' in which to store the auto number.[/td]
						[/tr]
					
						[tr]
							[td]keying_method[/td]
							[td]string[/td]
							[td](e.g. counterosrc).[/td]
						[/tr]
					
						[tr]
							[td]key_fieldname[/td]
							[td]string[/td]
							[td]The field name in this objectsource to be used as the key.[/td]
						[/tr]
					
						[tr]
							[td]osrc[/td]
							[td]string[/td]
							[td]The osrc in which to store the next auto id / counting number.[/td]
						[/tr]
					
				[/table]
			
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]BeginCreateObject[/td]
					[td]Creates a base of an object and notifies all childeren that a child is creating an object.[/td]
				[/tr]
			
				[tr]
					[td]CancelCreateObject[/td]
					[td]If it is called while an object is being created it stops the creation and cleans up the mess.[/td]
				[/tr]
			
				[tr]
					[td]ChangeSource[/td]
					[td]Changes the data item that the object source points to in the server.[/td]
				[/tr]
			
				[tr]
					[td]Clear[/td]
					[td]Clears the replica of data.[/td]
				[/tr]
			
				[tr]
					[td]Create[/td]
					[td]Encapsulates data in an array and calls CreateObject.[/td]
				[/tr]
			
				[tr]
					[td]CreateObject[/td]
					[td]Creates an object though OSML[/td]
				[/tr]
			
				[tr]
					[td]Delete[/td]
					[td]Deletes an object through OSML.[/td]
				[/tr]
			
				[tr]
					[td]DeleteClients[/td]
					[td]Stops relations with all clients and forces them to resync.[/td]
				[/tr]
			
				[tr]
					[td]DoubleSync[/td]
					[td]DEPRECATED: Performs a double synchronization with two other objectsources, known as the Parent and the Child, in two steps.  The first step is like Sync (see below), with a ParentOSRC and ParentKey1-ParentKey9] / ParentSelfKey1-ParentSelfKey9.  Next, a Sync is performed between the current objectsource and the ChildOSRC in the same way the first step performed a sync between the ParentOSRC and the current objectsource, respectively, using SelfChildKey1-SelfChildKey9 / ChildKey1-ChildKey9.[/td]
				[/tr]
			
				[tr]
					[td]FindObject[/td]
					[td]Searches for a certain object in the replica (retrieved records), and makes it the current object.  Parameters:  To search by record number, set ID equal to the integer (1 = first record).  To search by object name (primary key), set Name equal to a string containing the primary key (note that concatenated keys use | as a separator).  To search by other abitrary field values, set those values in the parameters to this action.[/td]
				[/tr]
			
				[tr]
					[td]First[/td]
					[td]Returns first record in the replica.[/td]
				[/tr]
			
				[tr]
					[td]Last[/td]
					[td]Returns last record in the replica.[/td]
				[/tr]
			
				[tr]
					[td]Modify[/td]
					[td]Modifies an object through OSML.[/td]
				[/tr]
			
				[tr]
					[td]Next[/td]
					[td]Returns next record in the replica.[/td]
				[/tr]
			
				[tr]
					[td]OrderObject[/td]
					[td]Order Object.[/td]
				[/tr]
			
				[tr]
					[td]Prev[/td]
					[td]Returns previous record in the replica.[/td]
				[/tr]
			
				[tr]
					[td]Query[/td]
					[td]Query.[/td]
				[/tr]
			
				[tr]
					[td]QueryObject[/td]
					[td]Query Object.  Primarily for internal use only.[/td]
				[/tr]
			
				[tr]
					[td]QueryParam[/td]
					[td]Refreshes the query, and allows new parameter values to be passed in.  Previous comment: Re-runs the SQL query, but adds more constraints.  The additional constraints are provided as parameters to the connector which invokes this action.[/td]
				[/tr]
			
				[tr]
					[td]QueryText[/td]
					[td]Runs the query, searching for objects whose attributes *contain* a combination of string values.  'query' contains a space-separated list of strings that must be present in each returned record (typically the 'query' is typed by the user).  'field_list' is a comma-separated list of field names to search in.  Each field name (attribute) can be preceded by a * or followed by a *; the presence of these asterisks controls whether the matching is done on the entire attribute value or just as a substring match.  Examples:  'my_key,*my_description*' for field_list means to match exact values for my_key, and match anywhere in my_description.  cx__case_insensitive can be set to 1 to make the search case insensitive.[/td]
				[/tr]
			
				[tr]
					[td]Refresh[/td]
					[td]Re-runs the SQL query using the previously stored parameter values (if any).  This is used when the data on the server is believed to have changed, and the new data is desired to be displayed in the application.  This action does not change the current row.[/td]
				[/tr]
			
				[tr]
					[td]RefreshObject[/td]
					[td]Re-runs the SQL query for just the one current object (record).  This does not change the currently selected record in the osrc.  This should be used when the intent is to refresh the values of just the current object without affecting anything else.[/td]
				[/tr]
			
				[tr]
					[td]Sync[/td]
					[td]DEPRECATED: Performs a synchronization operation with another objectsource by re-running the query for this objectsource based on another objectsource's data.  Used for implementing relationships between objectsources.  The ParentOSRC (string) parameter specifies the name of the objectsource to sync with.  Up to nine synchronization keys can be specified, as ParentKey1 and ChildKey1 through ParentKey9 and ChildKey9.  The ParentKey indicates the name of the field in the ParentOSRC to sync with (probably a primary key), and ChildKey indicates the name of the field (the foreign key) in the current objectsource to match with the parent objectsource.[/td]
				[/tr]
			
				[tr]
					[td]SaveClients[/td]
					[td]Save the clients that connect and lets clients of clients know that orsc is conected to them.[/td]
				[/tr]
			
				[tr]
					[td]SeqBackward[/td]
					[td]Moves the current row backwards.[/td]
				[/tr]
			
				[tr]
					[td]SeqForward[/td]
					[td]Moves the current row forwards.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]BeginQuery[/td]
					[td]This event occurs when a query is opened and the query ID is not null.[/td]
				[/tr]
			
				[tr]
					[td]Created[/td]
					[td]This event occurs when data is created in the object source.[/td]
				[/tr]
			
				[tr]
					[td]DataFocusChanged[/td]
					[td]This event is invoked whenever the current record changes to a different record.[/td]
				[/tr]
			
				[tr]
					[td]EndQuery[/td]
					[td]This event is invoked when a query is completed and the last row(s) retrieved.[/td]
				[/tr]
			
				[tr]
					[td]Modified[/td]
					[td]This event occurs when data is modified in the object source.[/td]
				[/tr]
			
				[tr]
					[td]Sequenced[/td]
					[td]This event occurs when replicant entries are swapped.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

osrc1 \"widget/osrc\"

	{

	replicasize = 9;

	readahead = 3;

	sql = \"select :name, :size from /samples\";

	baseobj = \"/samples\";



	// form or table widgets go here

	// Notes: 

	// 1. WHERE clause may require '*=' instead of '='

	// 2. Be careful with string / integer parameters,

	//    strings requires quotes, but using :parameters:paramname

	//    in the sql query automatically adds quotes.

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/page", null,
"		[b]page[/b] :: The top-level container that represents a Centrallix application.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/page[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The page widget represents the HTML application (or subapplication) as a whole and serves as a top-level container for other widgets in the application.  The page widget also implements some important functionality regarding the management of keypresses, focus, widget resizing, and event management. When creating an application, the top-level object in the application must either be \"widget/page\" or \"widget/frameset\", where the latter is used to create a multi-framed application containing multiple page widgets.


			Page widgets specify the colors for mouse, keyboard, and data focus for the application.  Focus is usually indicated via the drawing of a rectangle around a widget or data item, and for a 3D-effect two colors are specified for each type of focus: a color for the top and left of the rectangle, and another color for the right and bottom of the rectangle.  Mouse focus gives feedback to the user as to which widget they are pointing at (and thus which one will receive keyboard and/or data focus if the user clicks the mouse).  Keyboard focus tells the user which widget will receive data entered via the keyboard.  Data focus tells the user which record or data item is selected in a widget.


		
	[b]Usage:[/b]
	

			The page widget cannot be embedded within other widgets on a page. There must only be one per page, unless a frameset is used, in which case page widgets may be added within a frameset widget.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]attract[/td]
					[td]integer[/td]
					[td]Determines how close the window is to the browser edge.[/td]
				[/tr]
			
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the page.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]The background color for the page. Can either be a recognized color (such as \"red\"), or an RGB color (such as \"#C0C0C0\").[/td]
				[/tr]
			
				[tr]
					[td]datafocus1[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the top and left edges of rectangles drawn to indicate data focus (default is dark blue).[/td]
				[/tr]
			
				[tr]
					[td]datafocus2[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the bottom and right edges of rectangles drawn to indicate data focus (default is dark blue).[/td]
				[/tr]
			
				[tr]
					[td]font_name[/td]
					[td]string[/td]
					[td]The default font to use in the application (such as \"Arial\").[/td]
				[/tr]
			
				[tr]
					[td]font_size[/td]
					[td]integer[/td]
					[td]The default font size, in pixels (same as points on a typical 72dpi display), for text in the application.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]The height of the page in pixels.[/td]
				[/tr]
			
				[tr]
					[td]http_frame_options[/td]
					[td]string[/td]
					[td]When the application is rendered over HTTP/HTML, this controls the X-Frame-Options anti-clickjacking HTTP response header.  Possible values are \"none\", \"sameorigin\", and \"deny\".  The default value is set by the x_frame_options setting in the \"net_http\" section of centrallix.conf (see configuration docs for more information).  The http_frame_options setting only applies to the page widget it is set in, not to any components that the page loads (it can be set in those components separately if needed).[/td]
				[/tr]
			
				[tr]
					[td]icon[/td]
					[td]string[/td]
					[td]The file path to the page icon.[/td]
				[/tr]
			
				[tr]
					[td]kbdfocus1[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used for the top and left edges of rectangles drawn to indicate keyboard focus (default is \"white\").[/td]
				[/tr]
			
				[tr]
					[td]kbdfocus2[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the bottom and right edges of rectangles drawn to indicate keyboard focus (default is dark grey).[/td]
				[/tr]
			
				[tr]
					[td]linkcolor[/td]
					[td]string[/td]
					[td]Color for hyper links.[/td]
				[/tr]
			
				[tr]
					[td]loadstatus[/td]
					[td]string[/td]
					[td]True if page is loaded, false otherwise.[/td]
				[/tr]
			
				[tr]
					[td]max_requests[/td]
					[td]integer[/td]
					[td]The maximum amount of server requests at one time.[/td]
				[/tr]
			
				[tr]
					[td]mousefocus1[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used for the top and left edges of rectangles drawn to indicate mouse focus (default is \"black\").[/td]
				[/tr]
			
				[tr]
					[td]mousefocus2[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the bottom and right edges of rectangles drawn to indicate mouse focus (default is \"black\").[/td]
				[/tr]
			
				[tr]
					[td]show_diagnostics[/td]
					[td]yes/no[/td]
					[td]Set this to \"yes\" to display message boxes when application errors occur, such as an undefined property reference.  The default is \"no\", but this is handy when debugging.[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]The title which will appear in the browser's window title bar.[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]The default color for text which appears on the page.  Can either be a named color or RGB (numeric) color.[/td]
				[/tr]
			
				[tr]
					[td]widget_template[/td]
					[td]string[/td]
					[td]One or more comma-separated pathnames for widget templates to use.  The specified files should contain \"widget/template\" objects (see \"widget/template for details).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]The width of the page in pixels.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Alert[/td]
					[td]Sends an alert widget.[/td]
				[/tr]
			
				[tr]
					[td]Close[/td]
					[td]Closes the page.[/td]
				[/tr]
			
				[tr]
					[td]Launch[/td]
					[td]Starts a new app in a new window.[/td]
				[/tr]
			
				[tr]
					[td]LoadPage[/td]
					[td]Loads the page.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Load[/td]
					[td]This occurs when the page initializes.[/td]
				[/tr]
			
				[tr]
					[td]RightClick[/td]
					[td]This occurs when the uses right clicks on a mouse or mouse pad.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// A really simple application.Just a blank page with a title.

//

MyPage \"widget/page\"

	{

	title = \"This is an example page.\";

	bgcolor = \"white\";

	textcolor = \"black\";



	// widgets for application go here

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/pane", null,
"		[b]pane[/b] :: A visible rectangular container widget with a border

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/pane[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The pane is Centrallix's simplest container. It consists only of a background and a border, which can either have a \"raised\" edge or \"lowered\" edge style.


		
	[b]Usage:[/b]
	

			This container can be placed inside any widget having a visible container.  Visual or nonvisual widgets may be placed inside a pane.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the pane.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the pane's background.  If neither bgcolor nor background is specified, the pane will be transparent.[/td]
				[/tr]
			
				[tr]
					[td]border_radius[/td]
					[td]integer[/td]
					[td] The radius (sharpness) of the pane, smaller is more sharp.[/td]
				[/tr]
			
				[tr]
					[td]border_color[/td]
					[td]string[/td]
					[td]For \"bordered\" pane styles, this is the color of the border, either named or a #number.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the pane.[/td]
				[/tr]
			
				[tr]
					[td]shadow_color[/td]
					[td]string[/td]
					[td]The color of the shadow.[/td]
				[/tr]
			
				[tr]
					[td]shadow_offset[/td]
					[td]integer[/td]
					[td]The placement of the shadow with respect to the window.[/td]
				[/tr]
			
				[tr]
					[td]shadow_radius[/td]
					[td]integer[/td]
					[td]A radius that describes the sharpness of the corners of the shadow (smaller means sharper).[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]\"raised\", \"lowered\", \"bordered\", or \"flat\".  Determines the style of the pane's border.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the pane.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the pane, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the pane, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Point[/td]
					[td]Creates a triangular pointer on the edge of a pane to point at a given (X,Y) coordinate.[/td]
				[/tr]
			
				[tr]
					[td]Resize[/td]
					[td]Changes the width and height of the pane to the given (Width,Height).[/td]
				[/tr]
			
				[tr]
					[td]SetBackground[/td]
					[td]Sets the backgound image or color of the pane, using the attribute Color or Image.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the checkbox. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
		[/table]
	
	[b]Client Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]enable[/td]
					[td]string[/td]
					[td]Indicates if the user can interact with the pane (if so \"true\").[/td]
				[/tr]
			
		[/table]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

mypane \"widget/pane\"

	{

	x=100; y=100; width=300; height=300;

	style = \"raised\";

	bgcolor = \"#c0c0c0\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/parameter", null,
"		[b]parameter[/b] :: A declaration of a parameter that can be passed to an application or component

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/parameter[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			Applications (.app) and components (.cmp) can accept parameters, which can then be used to control the way that the application or component is rendered or the way it functions.  The widget/parameter widget is used to declare a parameter on an application or component, and to specify data type, default values, and more.  ObjectSource widgets can also have parameters, which are used when running the ObjectSource's SQL query.


		
	[b]Usage:[/b]
	

			Parameters must be placed inside a widget/page, widget/component-decl, or widget/osrc.  They cannot be used anywhere else, and cannot contain other widgets.


			Parameters have a very similar set of properties to the widget/hints widget, which is used to attach \"presentation hints\" to a widget.


			To use a parameter value, just reference it in a runserver() expression with object \"this\", as in :this:parametername.  For \"object\" parameters, reference them by just using the parameter name as if you would use the name of the object itself if the object were physically present in the component instead of being passed as a parameter (see sample code).


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]deploy_to_client[/td]
					[td]yes/no[/td]
					[td]Parameter widgets are treated as other widgets and normally would appear as widgets in the namespace on the client, with the parameter values being available in runclient() expressions on the client.  For efficiency reasons, however, parameters to static components, other than parameters of type \"object\", are not deployed to the client.  To override this behavior, set this option to \"yes\".[/td]
				[/tr]
			
				[tr]
					[td]find_container[/td]
					[td]string[/td]
					[td]If this is a parameter to a component, and the parameter has type \"object\", this can be set to a type of widget that should be searched for in the containing application or component once the component is instantiated.  Note that the object being searched for must be a container of the component instance, either directly or indirectly.  This option is frequently used to link in with a form or objectsource in the contianing application or component, without that form or objectsource having to be explicitly passed to the component.[/td]
				[/tr]
			
				[tr]
					[td]param_name[/td]
					[td]string[/td]
					[td]Gives the name of the parameter being passed, thus allowing the parameter widget name (which must be unique within the application or component) to be different from the parameter name as referenced within expressions, SQL, or connectors.[/td]
				[/tr]
			
				[tr]
					[td]type[/td]
					[td]string[/td]
					[td]The data type of the parameter.  Can be \"integer\", \"string\", \"double\", \"datetime\", \"money\", or \"object\".[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetValue[/td]
					[td]This sets the value of the parameter.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the parameter being passed is changed.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

			

$Version=2$

// Here is a component with two parameters, one a string, and one to

// pass a form to the component.

my_cmp \"widget/component-decl\"

	{

	width=200; height=32;

	

	field_name \"widget/parameter\"

		{

		type = string;

		default = null;

		}

	form_to_use \"widget/parameter\"

		{

		type = object;

		find_container = \"widget/form\";

		}

	my_label \"widget/label\"

		{

		x=0;y=0;width=200;height=32;

		fieldname=runserver(:this:field_name);

		form = form_to_use;

		}

	}

			

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/radiobuttonpanel", null,
"		[b]radiobuttonpanel[/b] :: A visual widget displaying a set of 'radio buttons'

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/radiobuttonpanel[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			A radio button panel widget is a form element used to create a set of radio buttons on screen.  Only one radio button may be selected at a time.  When the form goes into query mode the radio buttons change into checkboxes and more than one can be selected at a time.


		
	[b]Usage:[/b]
	

			The radio button panel can be placed inside of any visual container, and will automatically attach itself to a form widget if it is inside of one (directly or indirectly).  The \"widget/radiobuttonpanel\" is the main widget, and can contain any number of \"widget/radiobutton\" widgets which specify the choices which will be present on the panel.  No other visual widgets can be contained within a radio button panel.


			Note: form widget interaction was not yet implemented as of the time of writing of this document.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]Name of the column in the datasource you want to reference.[/td]
				[/tr]
			
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the radio button panel.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the panel background. If neither bgcolor nor background transparent.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the panel.[/td]
				[/tr]
			
				[tr]
					[td]outline_background[/td]
					[td]string[/td]
					[td]An image to be used for the rectangular border drawn around the radio buttons.[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]The color, RGB or named, of the text within the panel.  Default: \"black\".[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]The title for the radio button panel, which appears superimposed on the rectangular   border around the radio buttons.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the panel.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the panel, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the panel, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of widget/radiobutton child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]label[/td]
							[td]string[/td]
							[td]The text label to appear beside the radio button.[/td]
						[/tr]
					
						[tr]
							[td]selected[/td]
							[td]boolean[/td]
							[td]the radio button is initially selected or not. Should only be set on one radio Default;\"false\".[/td]
						[/tr]
					
						[tr]
							[td]value[/td]
							[td]string[/td]
							[td]The value of the selected item.[/td]
						[/tr]
					
				[/table]
			
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here are some radio buttons...

testradio \"widget/radiobuttonpanel\" 

	{

	x=20; y=20;

	width=150; height=80;

	title=\"test\";

	bgcolor=\"#ffffff\";

	label1 \"widget/radiobutton\" { label=\"basketball\";selected=\"true\"; }

	label2 \"widget/radiobutton\" { label=\"is fun\"; }

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/remotectl", null,
"		[b]remotectl[/b] :: Nonvisual widget permitting events to be passed in from a remote Centrallix application

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/remotectl[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The remote control nonvisual widget allows for one application (or instance of an application) to activate Actions in another running application, even if those applications are on two separate client computer systems. This is done by passing the event/action information through a remote control channel on the server.


			Two remote control widgets are required: a master and slave. This widget is the slave widget, which receives remote control events via the Centrallix server. When a master widget (remotemgr) sends an event through the channel, this slave widget is automatically activated and can then trigger the appropriate action on another widget on the page.


			In order for the remote control event to be passed through Centrallix, the master and slave widgets must both be using the same channel id and be logged in with the same username.They need not be a part of the same session on the server.


			Note: at the time of this writing, the remotectl widget was not yet operational.


		
	[b]Usage:[/b]
	

			The remote control widget is a nonvisual widget and thus cannot contain visual widgets.  It is normally located at the top-level of an application, within a \"widget/page\" object.


		
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, @newid, "widget/remotemgr", null,
"		[b]remotemgr[/b] :: Nonvisual widget permitting the application to send events to a remote Centrallix application

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/remotemgr[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The remote control manager nonvisual widget allows for one application (or instance of an application) to activate Actions in another running application, even if those applications are on two separate client computer systems. This is done by passing the event/action information through a remote control channel on the server.


			Two remote control widgets are required: a master and slave. This widget is the master widget, which sends remote control events via the Centrallix server. When a this widget sends an event through the channel, the slave widget (remotectl) is automatically activated and can then trigger the appropriate action on another widget on the remote application's page.


			In order for the remote control event to be passed through Centrallix, the master and slave widgets must both be using the same channel id and be logged in with the same username.They need not be a part of the same session on the server.


			Note: at the time of this writing, the remotemgr widget was not yet written.


		
	[b]Usage:[/b]
	

			The remote control manager widget is a nonvisual widget and thus cannot contain visual widgets.It is normally located at the top-level of an application, within a \"widget/page\" object.


		
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, @newid, "widget/repeat", null,
"		[b]repeat[/b] :: Repeat a subtree of widgets

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/repeat[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

		The 'repeat' nonvisual widget is used to repeat its entire subtree of widgets for each record in an sql query.


	    
	[b]Usage:[/b]
	

		This widget has no content of its own, so it is only useful if it has widgets inside it.  For positioning of visual widgets inside a widget/repeat, an hbox or vbox (outside the widget/repeat) can be used, or the x and y can be set mathematically based on results from the SQL query.


		The widget/repeat can be useful in creating data-driven user interfaces, as well as in facilitating a plug-in architecture in your application.  For instance, the SQL query could retrieve a list of matching components to be included in an interface, and the repeat widget could create components, tabs, windows, table columns, buttons, etc., for each returned SQL query record.


	    
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]sql[/td]
					[td]string[/td]
					[td]The sql query that will be run on the server.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// This is a repeat widget inside a vbox

	vbox \"widget/vbox\"

	    {

	    x=10; y=10; cellsize=20; spacing=0;

	    rpt \"widget/repeat\"

		{

		sql=\"SELECT :name FROM /samples WHERE substring(:name,1,1)='t'\";

		button \"widget/button\"

		    {

		    type=\"text\";

		    width=100; height=20;

		    text=runserver(:rpt:name);

		    }

		}

	    }

		

	    
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/rule", null,
"		[b]rule[/b] :: Declares a rule - a behavior expected from a widget.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/rule[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The rule widget is used as an important part of Centrallix's declarative application development.  It specifies behavior expected from a widget or relationship between two widgets.  For instance, one type of rule, the osrc_relationship rule, ties two objectsources together to enforce a primary key / foreign key relationship between the two.


		
	[b]Usage:[/b]
	

			Various widgets can have rule widgets; the various types of rule widgets are described in the sections for the widgets that they relate to.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]ruletype[/td]
					[td]string[/td]
					[td]The type of rule.  For instance, a \"widget/osrc\" can have \"osrc_relationship\" and \"osrc_key\" rules.[/td]
				[/tr]
			
		[/table]
	
");
	
insert into topic values(null, @newid, "widget/scrollbar", null,
"		[b]scrollbar[/b] :: A scrollbar with a thumb

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/scrollbar[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The scrollbar is used to allow the uesr to control a numeric value; typically the scrollbar is tied to the scrolling behavior of another widget.


			Currently, both table and scrollpane widgets have their own scrollbars, so this widget is not used for either of those.


		
	[b]Usage:[/b]
	

			As a visual widget, the scrollbar can be placed anywhere a visual widget is permitted.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image to be placed behind the scrollbar.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the scrollbar background. If neither bgcolor nor background is supplied, the scrollbar is transparent.[/td]
				[/tr]
			
				[tr]
					[td]direction[/td]
					[td]string[/td]
					[td]Either \"horizontal\" or \"vertical\" indicating the visible direction that the scrollbar is oriented.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the scrollbar.[/td]
				[/tr]
			
				[tr]
					[td]range[/td]
					[td]integer[/td]
					[td]The upper limit of the range of the scrollbar's value.  The value will range from 0 to the number specified by 'range'.  This property can be set to a runclient() dynamic expression, in which case the range will change as the expression changes.[/td]
				[/tr]
			
				[tr]
					[td]visible[/td]
					[td]string[/td]
					[td] This acts as a boolean to denote if the scroll bar is visible (\"true\" is visible).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the scrollbar.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the scrollbar, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the scrollbar, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MoveTo[/td]
					[td]Sets the scroll bar to a specific location determined by the parameter.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
");
	
insert into topic values(null, @newid, "widget/scrollpane", null,
"		[b]scrollpane[/b] :: A visual container with a scrollbar allowing for content height that exceeds the display area

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/scrollpane[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The scrollpane widget provides a container and a scrollbar. The scrollbar can be used to move up and down in the container, so more content can be placed in the container than can be normally viewed at one time.


			The scrollbar includes a draggable thumb as well as up and down arrows at the top and bottom.  Clicking the arrows scrolls the content of the container up or down by a small amount, whereas clicking on the scrollbar itself above or below the thumb will scroll the area by a large amount.


		
	[b]Usage:[/b]
	

			Scrollpane widgets can be placed inside any other container, but are usually placed inside a pane or a tab page.  Almost any content can be placed inside a scrollpane, but most commonly tables, treeviews, and html areas appear there.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image to be placed behind the scrollpane.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the scrollpane background.If neither bgcolor transparent.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]height, in pixels, of the scrollpane's visible area. Due to the nature of the  can time.[/td]
				[/tr]
			
				[tr]
					[td]visible[/td]
					[td]boolean[/td]
					[td]allows user to set scroll pane to visible (true) or not (false).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the scrollpane, including the scrollbar area on the right side.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the scrollpane, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the scrollpane, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]ScrollTo[/td]
					[td]Scrolls to a specific location determined by the scroll bar.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

MyScrollPane \"widget/scrollpane\"

	{

	// Visible scrollpane geometries...

	x = 0; y = 0;

	width = 600; height = 300;

	// This treeview is inside the scrollpane.

	MyTreeView \"widget/treeview\"

		{

		x = 1; y = 1;

		// Leave room for the scrollbar, (600 - 20 = 580)

		width = 580;

		// The source for the treeview.

		source = \"/\";

		}

	visible = \"true\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/tab", null,
"		[b]tab[/b] :: A tab (or notebook) widget allowing multiple pages to be layered and individually selected for viewing

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/tab[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The TabControl widget provides a DHTML tab control within Centrallix. The widget behaves in the same way as tab controls in other GUI environments, providing a set of tab pages, layered one on top of the other, which can be selected (brought to the foreground) by clicking the mouse on the respective visible tab at the top of the tab control.


			To further distinguish which tab at the top of the tab control is active, this widget slightly modifies the X/Y position of the tab as well as changing a thumbnail image (on the left edge of the tab) to further enhance the distinction between selected and inactive tab pages.


		
	[b]Usage:[/b]
	

			The tab pages are containers, and as such, controls of various kinds, including other tab controls, can be placed inside the tab pages.


			Tab pages are added to a tab control by including widgets of type \"widget/tabpage\" within the \"widget/tab\" widget in the structure file that defines the application. Any controls to appear inside a particular tab page should be placed inside their respective \"widget/tabpage\" widgets in the structure file.Only widgets of type \"widget/tabpage\" should be placed inside a \"widget/tab\", with the exception of nonvisuals such as connectors.


			Tab pages also have a 'visible' property which allows them to be hidden and revealed.  This is used if the type is set to dynamic, but can be used manually as well.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]An image to be used as the background of the tab control.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]As an alternate to \"background\", \"bgcolor\" can specify a color, either named or RGB.[/td]
				[/tr]
			
				[tr]
					[td]border_color[/td]
					[td]string[/td]
					[td]A color that outlines window.[/td]
				[/tr]
			
				[tr]
					[td]border_radius[/td]
					[td]integer[/td]
					[td]A radius that describes the sharpness of the corners of the window (smaller means sharper).[/td]
				[/tr]
			
				[tr]
					[td]border_style[/td]
					[td]string[/td]
					[td]Determines the look of the outline.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]The height, in pixels, of the tab control, including the page height but not including the height of the tabs at the top.[/td]
				[/tr]
			
				[tr]
					[td]inactive_background[/td]
					[td]string[/td]
					[td]An image to be used as the background of the tabs which are inactive (in the background).[/td]
				[/tr]
			
				[tr]
					[td]inactive_bgcolor[/td]
					[td]string[/td]
					[td]As an alternate to \"inactive_background\", \"inactive_bgcolor\" can specify a color, either named or RGB.[/td]
				[/tr]
			
				[tr]
					[td]shadow_angle[/td]
					[td]integer[/td]
					[td]The placement of the shadow described as a rotational transformation with respect to the window.[/td]
				[/tr]
			
				[tr]
					[td]shadow_color[/td]
					[td]string[/td]
					[td]The color of the shadow.[/td]
				[/tr]
			
				[tr]
					[td]shadow_offset[/td]
					[td]integer[/td]
					[td]The placement of the shadow with respect to the window.[/td]
				[/tr]
			
				[tr]
					[td]shadow_radius[/td]
					[td]integer[/td]
					[td]A radius that describes the sharpness of the corners of the shadow (smaller means sharper).[/td]
				[/tr]
			
				[tr]
					[td]tab_location[/td]
					[td]string[/td]
					[td]The location of the tabs:  \"top\" (default), \"bottom\", \"left\", \"right\", or \"none\".[/td]
				[/tr]
			
				[tr]
					[td]tab_width[/td]
					[td]integer[/td]
					[td]The width of the tabs in pixels.  This is optional for tab_locations of \"top\", \"bottom\", and \"none\".[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]The color of the text to be used on the tabs to identify them.[/td]
				[/tr]
			
				[tr]
					[td]visible[/td]
					[td]boolean[/td]
					[td]allows user to set tab to visible (true) or not (false).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the tab control, including the page width but not the width of the tabs (if they are at the side).[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the tab control, relative to the container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the control, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of any child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]fieldname[/td]
							[td]string[/td]
							[td]This is the fieldname from the objectsource that the tabpage will use for its title.[/td]
						[/tr]
					
						[tr]
							[td]title[/td]
							[td]string[/td]
							[td]The name of the tab page, which appears in the tab at the top of the control.[/td]
						[/tr]
					
						[tr]
							[td]type[/td]
							[td]string[/td]
							[td]If this is set to dynamic then the tabpage will act as an objectsource client, displaying zero or more tabs: one tab for each item in the replica with title set to the value of the fieldname.  In this way, a tab control can have a mix of dynamic tabs and static ones.[/td]
						[/tr]
					
						[tr]
							[td]visible[/td]
							[td]integer[/td]
							[td]0 or 1.  Can contain a dynamic runclient() expression to control when the tab page is visible to the user.[/td]
						[/tr]
					
				[/table]
			
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetTab[/td]
					[td]Sets the selected tab according to the parameter given (the tab itself or its index).[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
				[tr]
					[td]TabChanged[/td]
					[td]This event occurs when the visible tab changes.[/td]
				[/tr]
			
		[/table]
	
	[b]Client Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]selected[/td]
					[td]string[/td]
					[td]The name of the tab page that should be initially selected.  This can also contain a dynamic runclient() expression controlling which tab page is selected.[/td]
				[/tr]
			
				[tr]
					[td]selected_index[/td]
					[td]integer[/td]
					[td]Similar to \"selected\", but selects the tab by numeric index.[/td]
				[/tr]
			
		[/table]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here's a tab control with two tab pages.

myTabControl \"widget/tab\"

	{

	x = 20; y = 100; width=360; height=200;

	bgcolor=\"#c0c0c0\";

	selected = \"FirstPage\";

	FirstPage \"widget/tabpage\" { title = \"TheFirstPage\";}

	SecondPage \"widget/tabpage\" { title = \"TheSecondTabPage\"; }

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/table", null,
"		[b]table[/b] :: A visual widget providing a columnar presentation of multiple objects of the same type (such as query result records).

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/table[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			A table widget is used to display data in a tabular format. It consists of a header row with column labels, followed by any number of rows containing data.The header may have a different color or image scheme than the rows, and the rows may or may not be configured to alternate between two colors or background images.


		Table widgets come in three different flavors: static, dynamicpage, and dynamicrow.Static table widgets are built on the server and write their data directly into the container in which they reside, which is usually a scrollpane widget.  Dynamicpage table widgets load their data once they initialize on the client, by activating a query through an ObjectSource nonvisual widget.Dynamicpage table widgets do not support modification, but can be reloaded through an ObjectSource at will.Dynamicrow table widgets, on the other hand, display each row as an individual layer, and thus are modifiable on the client. Dynamicrow table widgets also load their contents through an ObjectSource widget query.As of the time of writing of this document, only [i]static[/i] mode and [i]dynamicrow[/i] mode were supported.

			Table widgets allow the selection (keyboard, mouse, and data focus) of individual rows.


		
	[b]Usage:[/b]
	

			Table widgets are normally placed inside of a scrollpane so that any rows which don't fit int the container can still be viewed. Table columns are created via \"widget/table-column\" child widgets within the table widget.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]allow_selection[/td]
					[td]yes/no[/td]
					[td]Whether to permit the user to select rows in the table.  Default \"yes\".[/td]
				[/tr]
			
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the table. This \"shows through\" between table cells.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used between table cells and rows.If neither bgcolor nor background are specified, the table is transparent.[/td]
				[/tr]
			
				[tr]
					[td]cellhspacing[/td]
					[td]integer[/td]
					[td]The horizontal spacing between cells in the table, in pixels.  Default is 1.[/td]
				[/tr]
			
				[tr]
					[td]cellvspacing[/td]
					[td]integer[/td]
					[td]The vertical spacing between cells in the table, in pixels.  Default is 1.[/td]
				[/tr]
			
				[tr]
					[td]colsep[/td]
					[td]integer[/td]
					[td]The width of the column separation lines in pixels.  Default is 1.[/td]
				[/tr]
			
				[tr]
					[td]data_mode[/td]
					[td]string[/td]
					[td]Either \"rows\" (default) or \"properties\".  In \"properties\" mode, the table displays one row per attribute, and so only displays the current record in the objectsource.  In \"rows\" mode, the table displays one row per record in the objectsource.[/td]
				[/tr]
			
				[tr]
					[td]demand_scrollbar[/td]
					[td]integer[/td]
					[td]Acts as a boolean to only show the scrollbar when it is needed (activates when it is set to \"1\").[/td]
				[/tr]
			
				[tr]
					[td]dragcols[/td]
					[td]integer[/td]
					[td]Whether to allow dragging of column boundaries to resize columns; set to 1 to allow it and 0 to disallow.  Default is 1.[/td]
				[/tr]
			
				[tr]
					[td]followcurrent[/td]
					[td]yes/no[/td]
					[td]Set to 'yes' to cause the table's current row to follow the currently selected object in the ObjectSource, and 'no' to disable this behavior.  Default is 'yes'.[/td]
				[/tr]
			
				[tr]
					[td]gridinemptyrows[/td]
					[td]integer[/td]
					[td]Whether to show the table's grid in rows which do not hold data.  Set to 1 to show the grid or 0 to not show it; default is 1.[/td]
				[/tr]
			
				[tr]
					[td]hdr_background[/td]
					[td]string[/td]
					[td]A background image for the header row cells.[/td]
				[/tr]
			
				[tr]
					[td]hdr_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the header row cells.[/td]
				[/tr]
			
				[tr]
					[td]hide_scrollbar[/td]
					[td]integer[/td]
					[td]Acts as a boolean to determine if the scrollbar can be seen (hide is \"1\").[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]The height in pixels of the table.[/td]
				[/tr]
			
				[tr]
					[td]image_max_height[/td]
					[td]integer[/td]
					[td]The maximum height of the image from the child.[/td]
				[/tr]
			
				[tr]
					[td]image_max_width[/td]
					[td]integer[/td]
					[td]The maximum width of the image from the child.[/td]
				[/tr]
			
				[tr]
					[td]initial_selection[/td]
					[td]string[/td]
					[td]Sets which data element in the table has focus initially.[/td]
				[/tr]
			
				[tr]
					[td]inner_border[/td]
					[td]integer[/td]
					[td]width of the inner spacing between cells in a table. Default0.[/td]
				[/tr]
			
				[tr]
					[td]inner_padding[/td]
					[td]integer[/td]
					[td]margins within each cell, in pixels.  Default is 0 pixels.[/td]
				[/tr]
			
				[tr]
					[td]max_rowheight[/td]
					[td]integer[/td]
					[td]The maximum height of each row.[/td]
				[/tr]
			
				[tr]
					[td]min_rowheight[/td]
					[td]integer[/td]
					[td]The minimum height of each row.[/td]
				[/tr]
			
				[tr]
					[td]newrow_background[/td]
					[td]string[/td]
					[td]A background image for the \"new row\" placeholder that is visible when a new object is being created.[/td]
				[/tr]
			
				[tr]
					[td]newrow_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the \"new row\" placeholder that is visible when a new object is being created.[/td]
				[/tr]
			
				[tr]
					[td]objectsource[/td]
					[td]string[/td]
					[td]Identifies the OSRC that the table connects to.[/td]
				[/tr]
			
				[tr]
					[td]outer_border[/td]
					[td]integer[/td]
					[td]width of the outer spacing around the outside of the table, in pixels.  Default0.[/td]
				[/tr]
			
				[tr]
					[td]overlap_scrollbar[/td]
					[td]integer[/td]
					[td]Acts as a boolean to allow the scroll bar to overlab with the table (allow is \"1\").[/td]
				[/tr]
			
				[tr]
					[td]reverse_order[/td]
					[td]integer[/td]
					[td]Acts as a boolean to reverse to order of the table elements (reverses if set to \"1\").[/td]
				[/tr]
			
				[tr]
					[td]row1_background[/td]
					[td]string[/td]
					[td]A background image for the table row cells.[/td]
				[/tr]
			
				[tr]
					[td]row1_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the table row cells.[/td]
				[/tr]
			
				[tr]
					[td]row2_background[/td]
					[td]string[/td]
					[td]A background image for the table row cells.  If this is specified, rows will alternate in backgrounds between \"row_background1\" and \"row_background2\".[/td]
				[/tr]
			
				[tr]
					[td]row2_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the table row cells.  If this is specified, rows will alternate in colors between \"row_bgcolor1\" and \"row_bgcolor2\".[/td]
				[/tr]
			
				[tr]
					[td]row_border_color[/td]
					[td]string[/td]
					[td]Determines the color of the edge of the rows.[/td]
				[/tr]
			
				[tr]
					[td]row_border_radius[/td]
					[td]string[/td]
					[td]Determines the sharpness of the row corners, smaller is more sharp.[/td]
				[/tr]
			
				[tr]
					[td]rowcashe_size[/td]
					[td]integer[/td]
					[td]How many rows are shown in the table.[/td]
				[/tr]
			
				[tr]
					[td]rowheight[/td]
					[td]integer[/td]
					[td]The height of the individual rows in pixels.  Default is 15 pixels.[/td]
				[/tr]
			
				[tr]
					[td]rowhighlight_background[/td]
					[td]string[/td]
					[td]A background image for the current (selected) row cells.[/td]
				[/tr]
			
				[tr]
					[td]rowhighlight_bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the current (selected) row cells.[/td]
				[/tr]
			
				[tr]
					[td]row_shadow_color[/td]
					[td]string[/td]
					[td]The color of the shadow of the row.[/td]
				[/tr]
			
				[tr]
					[td]row_shadow_offset[/td]
					[td]integer[/td]
					[td]How far the shadow is from the row.[/td]
				[/tr]
			
				[tr]
					[td]row_shadow_radius[/td]
					[td]integer[/td]
					[td]The sharpness of the corners of the shadow, smaller means more sharp.[/td]
				[/tr]
			
				[tr]
					[td]show_selection[/td]
					[td]yes/no[/td]
					[td]Whether to highlight the currently selected row.  Default \"yes\".[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the text in the normal data rows.[/td]
				[/tr]
			
				[tr]
					[td]textcolornew[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the text in the \"new row\" placeholder.[/td]
				[/tr]
			
				[tr]
					[td]textcolorhighlight[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the text in the highlighted data row.[/td]
				[/tr]
			
				[tr]
					[td]titlebar[/td]
					[td]yes/no[/td]
					[td]Whether to show the title bar of the table.  Default \"yes\".[/td]
				[/tr]
			
				[tr]
					[td]titlecolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the text in the header row.  If unset, defaults to textcolor.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the table.The height is determined dynamically.[/td]
				[/tr]
			
				[tr]
					[td]windowsize[/td]
					[td]integer[/td]
					[td]The maximum number of rows to show at any given time, for dynamic tables.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the table. Default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the table. Default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of widget/table-column child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]align[/td]
							[td]string[/td]
							[td]The alignment of the column:  \"left\" or \"right\".[/td]
						[/tr]
					
						[tr]
							[td]caption_fieldname[/td]
							[td]string[/td]
							[td]It is a pseudonym for the fieldname.[/td]
						[/tr]
					
						[tr]
							[td]caption_textcolor[/td]
							[td]string[/td]
							[td]The color of the caption_fieldname.[/td]
						[/tr]
					
						[tr]
							[td]group_by[/td]
							[td]string[/td]
							[td]Acts as a boolean to determine if items are group able.[/td]
						[/tr]
					
						[tr]
							[td]height[/td]
							[td]integer[/td]
							[td]The height in pixels of the column.[/td]
						[/tr]
					
						[tr]
							[td]title[/td]
							[td]string[/td]
							[td]The title of the column to be displayed in the header row.[/td]
						[/tr]
					
						[tr]
							[td]type[/td]
							[td]string[/td]
							[td]The type of the column: \"text\", \"check\", or \"image\".  \"text\" is a normal column, and displays the textual value of the data element.  \"check\" displays a checkmark if the data is non-zero (integers) or for strings if the value is non-empty and not \"N\" or \"No\".  \"image\" displays the image referred to by the pathname contained in the data value.[/td]
						[/tr]
					
						[tr]
							[td]width[/td]
							[td]integer[/td]
							[td]width of the column.[/td]
						[/tr]
					
						[tr]
							[td]wrap[/td]
							[td]string[/td]
							[td]Determines if text can wrap around an obstacle.[/td]
						[/tr]
					
				[/table]
			
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Clear[/td]
					[td]Drops all elements from the table.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]The Click event fires when a user clicks on a row.[/td]
				[/tr]
			
				[tr]
					[td]DblClick[/td]
					[td]The DblClick event fires when the user double-clicks the mouse on a row.  Passes three values, 'Caller' which is the table's name, 'recnum' which is the sequential number of the record in the table, and 'data' which is an array of the data in the selected record.[/td]
				[/tr]
			
				[tr]
					[td]RightClick[/td]
					[td]The Click event fires when a user right clicks on a mouse or mouse pad.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Here is a simple static table.

tblFileList \"widget/table\"

	{

	sql = \"select :name,annotation=condition(:annotation=='','-none-',:annotation)

				from /samples\";

	mode=\"static\";

	width=408;

	inner_border=2;

	inner_padding=1;

	bgcolor=\"#c0c0c0\";

	row_bgcolor1=\"#e0e0e0\";

	hdr_bgcolor=\"white\";

	textcolor=\"black\";

	name \"widget/table-column\" { title=\"Object Name\";width=20; }

	annotation \"widget/table-column\" { title=\"Annotation\"; width=25; }

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/template", null,
"		[b]template[/b] :: templates are a way to provide default values (and default children) of different widget_class's

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/template[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

		
	[b]Usage:[/b]
	

		  A widget/template must be a root widget of a file (which normally is given a .tpl extension). 


		  Each child in a widget/template is a \"rule\". Each rule applies to widgets that both 1) have the same 'widget_class' property value (there can be only one widget_class per widget (and \"rule\")) and 2) match the widget type of the child (eg \"widget/imagebutton\").


		  Every other property of the \"rule\" are default values.


		  All children of the \"rule\" are automatically inserted into the matched widgets.


		
	[b]Child Properties:[/b]
	
			(of any child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]widget_class[/td]
							[td]string[/td]
							[td]The value of this property must match another widget before said widget will \"inherit\" this \"rule\".[/td]
						[/tr]
					
				[/table]
			
	[b]Sample Code:[/b]
	
		[code]
		

		

objcanvas_test \"widget/template\"

    {

    btnFirst \"widget/imagebutton\"

	{

	widget_class=\"FirstRecord\";



	width=18;

	height=18;

	image=\"/sys/images/ico16aa.gif\";

	pointimage=\"/sys/images/ico16ab.gif\";

	clickimage=\"/sys/images/ico16ac.gif\";

	disabledimage=\"/sys/images/ico16ad.gif\";

	enabled = runclient(:template_form:recid > 1);

	cnFirst \"widget/connector\" { event=\"Click\"; target=template_form; action=\"First\"; }

	}

    }

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/textarea", null,
"		[b]textarea[/b] :: Visual multi-line text data entry widget.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/textarea[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The textarea is a multi-line text edit widget, allowing the user to enter text data containing line endings, tabs, and more.  It also contains a scrollbar which allows more text to be edited than can fit in the displayable area.


		
	[b]Usage:[/b]
	

			The textarea is a visual form element widget, and can be contained inside any container capable of holding visual widgets.  It may only contain nonvisual widgets like the connector.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the textarea.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the background.  If neither bgcolor nor background are specified, the textarea is transparent.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]The field in the objectsource to associate this textarea with.[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]Links text area to a form.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]The height of the textarea, in pixels[/td]
				[/tr]
			
				[tr]
					[td]maxchars[/td]
					[td]integer[/td]
					[td]The maximum number of characters the textarea should accept from the user[/td]
				[/tr]
			
				[tr]
					[td]mode[/td]
					[td]string[/td]
					[td]Can hold text, html, or wiki and display.[/td]
				[/tr]
			
				[tr]
					[td]readonly[/td]
					[td]yes/no[/td]
					[td]Set to 'yes' if the data in the text area should be viewed only and not be modified.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]The border style of the text area, can be 'raised' or 'lowered'.  Default is 'raised'.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]The width of the textarea, in pixels[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]The horizontal coordinate of the left edge of the textarea, relative to the container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]The vertical coordinate of the top edge of the textarea, relative to the container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]InsertText[/td]
					[td]Checks to see if things need to be changed (new is different from old) then calls set value.[/td]
				[/tr]
			
				[tr]
					[td]SetFocus[/td]
					[td]Sets the focus on a selected widget.[/td]
				[/tr]
			
				[tr]
					[td]SetValue[/td]
					[td]Sets the content to the text parameter sent.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]BeforeKeyPress[/td]
					[td]This event occurs before the key press event is fired and can stop the key press event.[/td]
				[/tr]
			
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the widget (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]DataModify[/td]
					[td]This event occurs when the data is changed (occurs when key press or button changes things).[/td]
				[/tr]
			
				[tr]
					[td]EscapePressed[/td]
					[td]This event occurs when the user presses the escape key.[/td]
				[/tr]
			
				[tr]
					[td]GetFocus[/td]
					[td]This event occurs when the editbox receives keyboard focus (if the user tabs on to it or clicks on it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]KeyPress[/td]
					[td]This event occurs when the user presses any key.[/td]
				[/tr]
			
				[tr]
					[td]LoseFocus[/td]
					[td]This event occurs when the editbox loses keyboard focus (if the user tabs off of it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
				[tr]
					[td]TabPressed[/td]
					[td]This event occurs when the user presses the tab key.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, @newid, "widget/textbutton", null,
"		[b]textbutton[/b] :: A simple visual button widget built not from images but from a simple text string.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/textbutton[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			A textbutton provides similar functionality to the imagebutton. However, the programmer need not create two or three graphics images in order to use a textbutton; rather simply specifying the text to appear on the button is sufficient.


			Textbuttons, like imagebuttons, can either have two or three states. A three-state textbutton doesn't have a \"raised\" border until the user points to it, whereas a two-state textbutton retains its raised border whether pointed to or not.


		
	[b]Usage:[/b]
	

			The TextButton can be placed inside any visible container, but only nonvisual widgets can be placed within it.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]align[/td]
					[td]string[/td]
					[td]Sets the alignment of text in the button, can have left right or center (default).[/td]
				[/tr]
			
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]A background image for the button.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used as the button's background.If neither bgcolor nor background are specified, the button is transparent.[/td]
				[/tr]
			
				[tr]
					[td]border_color[/td]
					[td]string[/td]
					[td]A color that outlines the button.[/td]
				[/tr]
			
				[tr]
					[td]border_radius[/td]
					[td]integer[/td]
					[td]A radius that describes the sharpness of the corners of the button (smaller means sharper).[/td]
				[/tr]
			
				[tr]
					[td]border_style[/td]
					[td]string[/td]
					[td]Determines the look of the outline.[/td]
				[/tr]
			
				[tr]
					[td]disable_color[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used for the button's text when it is disabled.[/td]
				[/tr]
			
				[tr]
					[td]enabled[/td]
					[td]yes/no or expr[/td]
					[td]Whether the button is enabled (can be clicked).  Default is 'yes'.  Also supports dynamic runclient() expressions allowing the enabled status of the button to follow the value of an expression.[/td]
				[/tr]
			
				[tr]
					[td]fgcolor1[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the text on the button.  Default \"white\".[/td]
				[/tr]
			
				[tr]
					[td]fgcolor2[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the text's 1-pixel drop-shadow.  Default \"black\".[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]Height, in pixels, of the text button.[/td]
				[/tr]
			
				[tr]
					[td]image[/td]
					[td]string[/td]
					[td]File path to the source of the image.[/td]
				[/tr]
			
				[tr]
					[td]image_height[/td]
					[td]integer[/td]
					[td]Defines the height of image.[/td]
				[/tr]
			
				[tr]
					[td]image_margin[/td]
					[td]integer[/td]
					[td]Defines spacing between image and the border.[/td]
				[/tr]
			
				[tr]
					[td]image_width[/td]
					[td]integer[/td]
					[td]Defines the width of image.[/td]
				[/tr]
			
				[tr]
					[td]image_position[/td]
					[td]string[/td]
					[td]Describes where an image is in the text button (top-default, right, bottom, left).[/td]
				[/tr]
			
				[tr]
					[td]text[/td]
					[td]string[/td]
					[td]The text to appear on the button.  This may be a dynamic runclient() expression to dynamically change the button's text.[/td]
				[/tr]
			
				[tr]
					[td]tristate[/td]
					[td]yes/no[/td]
					[td]Whether or not the button is tri-state (does not display a raised border until the user points at it). Default is yes.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]The width, in pixels, of the text button.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetText[/td]
					[td]Called from a connector, this action sets the button's text to the value passed in through connector's \"Text\" parameter.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the button. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

MyButton \"widget/textbutton\"

	{

	x = 10; y = 10;

	width = 50;

	height = 20;

	tristate = \"no\";

	background = \"/sys/images/grey_gradient.png\";

	text = \"OK\";

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/timer", null,
"		[b]timer[/b] :: A nonvisual widget which is used to fire an event when a period of time has elapsed.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/timer[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			A timer widget is used to schedule an Event to occur after a specified amount of time.  Timers can be set to expire a set amount of time after the page is loaded, or they can be triggered into counting down via activating an Action on the timer. Timers can be used to create animations, delayed effects, and more.


		
	[b]Usage:[/b]
	

			Timers are nonvisual widgets which can be placed almost anywhere in an application.  They are most commonly found at the top-level of the application, however. Timers have no direct effects on the object in which they are placed.  Timers can only contain Connector widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]auto_reset[/td]
					[td]boolean[/td]
					[td]The timer starts counting down again immediately after it expires.[/td]
				[/tr]
			
				[tr]
					[td]auto_start[/td]
					[td]boolean[/td]
					[td]The timer starts counting down immediately after the page loads.[/td]
				[/tr]
			
				[tr]
					[td]msec[/td]
					[td]integer[/td]
					[td]Of milliseconds (1/1000th of a second) before the timer expires.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]CancelTimer[/td]
					[td]Actions causes a timer to stop counting down, and thus no Expire event will occur until another countdown sequence is initiated by a SetTimer action.[/td]
				[/tr]
			
				[tr]
					[td]SetTimer[/td]
					[td]Action takes two parameters: \"Time\" (integer in milliseconds) and \"AutoReset\" (integer 0 or 1). It causes a timer to begin counting down towards an Expire event.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]expire[/td]
					[td]This occurs when the timer hits its timeout and the auto reset is not enabled.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// These timers trigger each other! You can make this app do something

// more interesting by putting other connectors on the Expire events of

// the timers as well :)

//

mytimerOne \"widget/timer\"

	{

	msec = 500; // half a second

	auto_start = 1;

	auto_reset = 0;

	cnOne \"widget/connector\"

		{ 

		event=\"Expire\"; target=\"mytimerTwo\";action=\"SetTimer\";

		Time=\"500\"; AutoReset=\"0\";

		}  

	}

mytimerTwo \"widget/timer\"

	{

	msec = 500;

	auto_start = 0;

	auto_reset = 0;

	cnTwo \"widget/connector\"

		{ 

		event=\"Expire\"; target=\"mytimerOne\";action=\"SetTimer\";

		Time=\"500\"; AutoReset=\"0\";

		}

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/treeview", null,
"		[b]treeview[/b] :: A visual widget used to display tree-structured data (such as a directory tree or similar).

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/treeview[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			A treeview provides a way of viewing hierarchically-organized data via a \"traditional\" GUI point-and-click tree structure. A treeview has \"branches\" that can expand and collapse entire subtrees of data.


			Centrallix treeviews present a subtree of the ObjectSystem, with the data dynamically loaded, on demand, from the server as the widget is used. The treeview widget thus has no intelligence in and of itself in determining what kinds of objects are presented at each level of the tree. Many times, this is exactly what is desired because the treeview is being used to simply browse objects in the ObjectSystem, such as directories and files. In other cases, the treeview is teamed up with a special ObjectSystem object called a \"querytree\" (QYT) object. The querytree object creates a hierarchical view from other potentially non-hierarchical data in the ObjectSystem, such as that from different database tables and so forth.


		
	[b]Usage:[/b]
	

			Treeviews can be placed inside of any visual container, but are usually placed inside of a scrollpane, since scrollpanes can expand to allow the user to view data that would not normally fit inside the desired container. Treeviews can contain only nonvisual widgets such as connectors.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]fgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used for the text for the items in the treeview.[/td]
				[/tr]
			
				[tr]
					[td]highlight_background[/td]
					[td]string[/td]
					[td]A background image for the selected item in the treeview.[/td]
				[/tr]
			
				[tr]
					[td]highlight_fgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, to be used for the text for the selected item.[/td]
				[/tr]
			
				[tr]
					[td]icon[/td]
					[td]string[/td]
					[td]A pathname to an image to be used as icons for the items in the treeview.[/td]
				[/tr]
			
				[tr]
					[td]order[/td]
					[td]string[/td]
					[td]Orders treeview in descending (desc) and ascending (anything else) order.[/td]
				[/tr]
			
				[tr]
					[td]show_branches[/td]
					[td]yes/no[/td]
					[td]Whether to display the connections between items in the tree using lines.[/td]
				[/tr]
			
				[tr]
					[td]show_root[/td]
					[td]yes/no[/td]
					[td]Whether to display the root of the tree.  If \"no\", then the root is auto-expanded and only its children are visible.[/td]
				[/tr]
			
				[tr]
					[td]show_root_branch[/td]
					[td]yes/no[/td]
					[td]Whether to display the connection lines between the root and the root's immediate children.  If \"no\", the root's children are flush against the lefthand side of the treeview, otherwise the connecting lines show up.[/td]
				[/tr]
			
				[tr]
					[td]source[/td]
					[td]string[/td]
					[td]The ObjectSystem path of the root of the treeview.[/td]
				[/tr]
			
				[tr]
					[td]use_3d_lines[/td]
					[td]yes/no[/td]
					[td]If set to \"yes\", the branch lines are drawn in a 3D style, otherwise they are drawn in a single color.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]Width, in pixels, of the treeview.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]X-coordinate of the upper left corner of the treeview's root object.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]Y-coordinate of the upper left corner of the treeview's root object.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Search[/td]
					[td]Searches for a specific element.[/td]
				[/tr]
			
				[tr]
					[td]SearchNext[/td]
					[td]Returns the next element in the treeview.[/td]
				[/tr]
			
				[tr]
					[td]SetFocus[/td]
					[td]Transfers focus to the selected element.[/td]
				[/tr]
			
				[tr]
					[td]SetRoot[/td]
					[td]Sets the selected element as the root.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks while the treeview is in focus.[/td]
				[/tr]
			
				[tr]
					[td]ClickItem[/td]
					[td]Occurs when the user clicks on the clickable link for an item in the treeview. Its one parameter is \"Pathname\", or the ObjectSystem path to the object which was selected.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the widget.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the widget for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the widget.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the widget.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the widget.  It will not occur again until the user moves the mouse off of the widget and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the widget.[/td]
				[/tr]
			
				[tr]
					[td]SelectItem[/td]
					[td]This event occurs when one of the elements of the treeview is given focus.[/td]
				[/tr]
			
				[tr]
					[td]RightClickItem[/td]
					[td]To the above, but when the user right-clicks on an item in the treeview.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// Example of a pane containing a scrollpane containing a treeview.

mypane \"widget/pane\"

	{

	x=100; y=100; width=300; height=300;

	style = \"lowered\";

	bgcolor = \"#c0c0c0\";

	myscroll \"widget/scrollpane\"

		{

		x=0; y=0; width=198; height=198;

		mytreeview \"widget/treeview\"

			{

			x=1; y=1; width=175;

			source=\"/\";

			}

		}

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/variable", null,
"		[b]variable[/b] :: A scalar variable object

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/variable[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			The 'variable' nonvisual widget is used to create a global javascript scalar variable in the application.The variable can be an integer or string.


		
	[b]Usage:[/b]
	

			This nonvisual widget is used at the top-level (within a \"widget/page\").  Currently it has no events, and so shouldn't contain any visual or nonvisual widgets.


		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]The field with which to link / connect to the variable.[/td]
				[/tr]
			
				[tr]
					[td]form[/td]
					[td]string[/td]
					[td]The form (e.g. it's osrc) with which to associate the variable and fieldname (if different than the form in which variable is currently nested).[/td]
				[/tr]
			
				[tr]
					[td]value[/td]
					[td]string[/td]
					[td]Can be accessed as :var_name:value.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetValue[/td]
					[td]Called by a widget/connector: Sets the value of the variable to whatever is specified by the \"Value\" parameter.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the widget (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]DataModify[/td]
					[td]This event occurs when the data is changed (occurs when key press or button changes things).[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		

		

$Version=2$

// This creates a global variable.

counter \"widget/variable\"

	{

	value = 0;

	}

		

		
		[/code]
	
");
	
insert into topic values(null, @newid, "widget/vbox", null,
"		[b]vbox[/b] :: Container which automatically positions its children

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/vbox[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	

			An autolayout widget with style set to \"vbox\".  See \"widget/autolayout\".


		
");
	