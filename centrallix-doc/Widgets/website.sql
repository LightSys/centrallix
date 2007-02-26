
insert into topic values(2,1,"II. Application Components",null,
"       [b]Centrallix Application Widget Reference[/b]
	
	This guide contains a basic overview and specification of each of the widgets that Centrallix supports, including a handful of planned (but not yet implemented or not fully implemented) widgets.
	
	Each widget can have properties, child properties, events, and actions.  Properties define how the widget behaves, appears, and is constructed. Child properties usually define how a widget contained within the given widget relates to its parent. Such properties are normally managed by the containing (parent) widget rather than by the child, and many times the child widget cannot exist without its parent. Events enable a widget to have an \"influence\" on the other widgets on the page, and are utilized by placing a \"connector\" widget within the given widget.  The connector routes an event's firing to the activation of an \"action\" on some other widget on the page.  Actions are \"methods\" on widgets which cause the widget to do something or change its appearance in some way.
	
	The \"overview\" section for each widget describes the widget's purpose and how it operates.  A \"usage\" section is provided to show the appropriate contexts for the use of the widget in a real application.
	
	Some widget properties are 'dynamic'; that is, an expression can be provided which is then evaluated dynamically on the client so that the widget's property changes as the expression's value changes.
	
	\"Client properties\" are available for some widgets.  These properties may not be specified in the application file itself, but are available on the client during application operation.  They often relate to the status of a given widget.
	
	Where possible, sample code is provided for each widget to show how the widget can be used.  In most cases the widget's code is displayed in isolation, although in practice widgets can never be used outside of a \"widget/page\" container widget at some level. In a few cases, a more complete mini-application is shown.
	
	Sample code is generally given in \"structure file\" format, which is the normal format for the building of applications.  However, other suitable object-structured data formats can be used, including XML.

	Copyright (c)  1998-2006 LightSys Technology Services, Inc.
	
	[b]Documentation on the following widgets is available:[/b]

	[table]
	
		[tr][td][widget/calendar][/td][td]Calendar-type view of event/schedule type data in an objectsource[/td][/tr]
	
		[tr][td][widget/checkbox][/td][td]Form element capable of selecting an on/off, yes/no, true/false, etc., type of value, via a visual 'check mark'.[/td][/tr]
	
		[tr][td][widget/clock][/td][td]A simple clock widget which displays the current time on the client.[/td][/tr]
	
		[tr][td][widget/connector][/td][td]A nonvisual widget used to trigger an Action on a widget when an Event on another widget fires.[/td][/tr]
	
		[tr][td][widget/datetime][/td][td]A visual widget for displaying and editing a date/time type value.[/td][/tr]
	
		[tr][td][widget/dropdown][/td][td]A visual widget which allows the user to select one from a number of options in a list which appears when the user clicks on the widget.[/td][/tr]
	
		[tr][td][widget/editbox][/td][td]A visual widget which allows the entry of one line of textual data.[/td][/tr]
	
		[tr][td][widget/execmethod][/td][td]A nonvisual widget which can call ObjectSystem methods on server objects.[/td][/tr]
	
		[tr][td][widget/form][/td][td]A nonvisual container used to group a set of form element widgets into a single record or object[/td][/tr]
	
		[tr][td][widget/formstatus][/td][td]A specialized visual widget used to display the current mode of a form widget.[/td][/tr]
	
		[tr][td][widget/frameset][/td][td]A visual container used to create a DHTML frameset within which page widgets can be placed[/td][/tr]
	
		[tr][td][widget/html][/td][td]A miniature HTML browser control, capable of viewing and navigating simple web documents.[/td][/tr]
	
		[tr][td][widget/htmlwindow][/td][td]A dialog or application window which is a lightweight movable container.[/td][/tr]
	
		[tr][td][widget/imagebutton][/td][td]A button widget which uses a set of images to control its appearance.[/td][/tr]
	
		[tr][td][widget/menu][/td][td]A visual pop-up or drop-down menu widget.[/td][/tr]
	
		[tr][td][widget/objectsource][/td][td]A nonvisual widget which handles communication between forms/tables and the Centrallix server.[/td][/tr]
	
		[tr][td][widget/page][/td][td]The top-level container that represents a Centrallix application.[/td][/tr]
	
		[tr][td][widget/pane][/td][td]A visible rectangular container widget with a border[/td][/tr]
	
		[tr][td][widget/radiobuttonpanel][/td][td]A visual widget displaying a set of 'radio buttons'[/td][/tr]
	
		[tr][td][widget/remotectl][/td][td]Nonvisual widget permitting events to be passed in from a remote Centrallix application[/td][/tr]
	
		[tr][td][widget/remotemgr][/td][td]Nonvisual widget permitting the application to send events to a remote Centrallix application[/td][/tr]
	
		[tr][td][widget/scrollpane][/td][td]A visual container with a scrollbar allowing for content height that exceeds the display area[/td][/tr]
	
		[tr][td][widget/tab][/td][td]A tab (or notebook) widget allowing multiple pages to be layered and individually selected for viewing[/td][/tr]
	
		[tr][td][widget/table][/td][td]A visual widget providing a columnar presentation of multiple objects of the same type (such as query result records).[/td][/tr]
	
		[tr][td][widget/textarea][/td][td]Visual multi-line text data entry widget.[/td][/tr]
	
		[tr][td][widget/textbutton][/td][td]A simple visual button widget built not from images but from a simple text string.[/td][/tr]
	
		[tr][td][widget/timer][/td][td]A nonvisual widget which is used to fire an event when a period of time has elapsed.[/td][/tr]
	
		[tr][td][widget/treeview][/td][td]A visual widget used to display tree-structured data (such as a directory tree or similar).[/td][/tr]
	
		[tr][td][widget/variable][/td][td]A scalar variable object[/td][/tr]
	
	[/table]
");

	
insert into topic values(null, 2, "widget/calendar", null,
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
			
			    Year - presents twelve months at a time in a low-detail type of setting; when events occur on a day, the day is highlighted, but no more data than that is displayed.  If the user points the mouse at a given day, it will display in a popup \"tooltip\" type of manner the events associated with that day.  If more than one year's worth of data is in the objectsource's replica, then multiple years are displayed one after the other.  Each year is displayed as four rows of three months each, in traditional calendar format.
			    Month - presents one month's worth of data in a medium-detail display.  For each month with data in the objectsource, the calendar will display the month as multiple rows, each row containing one week's worth of days.  The calendar will attempt to display the various entries for each day, in order of priority, but will limit the amount of data displayed to keep the boxes for the days approximately visually square.  Pointing at a day will show the events for the day in a popup, and pointing at an event in the day will show the details for that event.
			    Week - presents one week's worth of data in a high-detail display.  For each week with data in the objectsource, the calendar will display the week as seven days with a large vertical extent capable of displaying all events for the day.  The left edge of the display will contain times during the day for the day's events to line up with.
			    Day - presents an entire day's worth of data in a high-detail display.  For each day with data in the objectsource, the calendar will display the day in full-width, with the day's events listed chronologically from top to bottom.
			
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
					[td]width[/td]
					[td]integer[/td]
					[td]The width of the calendar, in pixels.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the calendar, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the calendar, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
		[i]none currently available[/i]
	
	[b]Client Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, 2, "widget/checkbox", null,
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
					[td]whether this checkbox should be initially checked or not.  Default 'no'.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]name of object source field that should be associated with this checkbox.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the checkbox, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the checkbox, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the checkbox. No parameters are available from this event.[/td]
				[/tr]
			
				[tr]
					[td]MouseUp[/td]
					[td]This event occurs when the user releases the mouse button on the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseDown[/td]
					[td]This event occurs when the user presses the mouse button on the checkbox.  This differs from the 'Click' event in that the user must actually press and release the mouse button on the checkbox for a Click event to fire, whereas simply pressing the mouse button down will cause the MouseDown event to fire.[/td]
				[/tr]
			
				[tr]
					[td]MouseOver[/td]
					[td]This event occurs when the user first moves the mouse pointer over the checkbox.  It will not occur again until the user moves the mouse off of the checkbox and then back over it again.[/td]
				[/tr]
			
				[tr]
					[td]MouseOut[/td]
					[td]This event occurs when the user moves the mouse pointer off of the checkbox.[/td]
				[/tr]
			
				[tr]
					[td]MouseMove[/td]
					[td]This event occurs when the user moves the mouse pointer while it is over the checkbox.  The event will repeatedly fire each time the pointer moves.[/td]
				[/tr]
			
				[tr]
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the checkbox (clicked or unclicked it).[/td]
				[/tr]
			
		[/table]
	
	[b]Client Properties:[/b]
	
		[i]none currently available[/i]
	
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
	
insert into topic values(null, 2, "widget/clock", null,
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
			
		[/table]
	
");
	
insert into topic values(null, 2, "widget/connector", null,
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
					[td]target[/td]
					[td]string[/td]
					[td]The name of another widget on the page whose action will be called by this connector.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		
		
$Version=2$
// Here's our connector.Imagine that this is embedded within a treeview
// and references an 'html' control called 'ht1' somewhere else on the page.
//
cn1 \"widget/connector\"
	{
	// Triggered by ClickItem from the treeview.
	event = \"ClickItem\";
	// Causes the LoadPage action on html area 'ht1'
	target = \"ht1\";
	action = \"LoadPage\";
	// The Source for LoadPage is the Pathname fronm ClickItem.
	Source = \"Pathname\";
	}
		
		
		[/code]
	
");
	
insert into topic values(null, 2, "widget/datetime", null,
"		[b]datetime[/b] :: A visual widget for displaying and editing a date/time type value.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/datetime[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
		
");
	
insert into topic values(null, 2, "widget/dropdown", null,
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
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the editbox, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the editbox, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]highlight[/td]
					[td]integer[/td]
					[td]The color of the highlighted option when the mouse goes over the item.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]integer[/td]
					[td]The background color for the widget.[/td]
				[/tr]
			
				[tr]
					[td]fieldname[/td]
					[td]string[/td]
					[td]Fieldname (from the dataset) to bind this element to[/td]
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
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
		[i]none currently available[/i]
	
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
	
insert into topic values(null, 2, "widget/editbox", null,
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
		
	[b]Usage:[/b]
	
			The editbox can be placed within any visual widget (or within a form widget).  Editboxes automatically attach themselves to the form which contains them (whether directly or indirectly).  Editboxes cannot contain visual widgets.
		
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
					[td]A background image for the contents of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the editbox contents. If neither bgcolor nor background is specified, the editbox is transparent.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]height, in pixels, of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]maxchars[/td]
					[td]integer[/td]
					[td]number of characters to accept from the user.[/td]
				[/tr]
			
				[tr]
					[td]readonly[/td]
					[td]yes/no[/td]
					[td]set to 'yes' if the user cannot modify the value of the edit box (default 'no').[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]Either \"raised\" or \"lowered\", and determines the style of the border drawn around the editbox.  Default is \"lowered\".[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the editbox.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the editbox, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the editbox, default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetValue[/td]
					[td]The SetValue action modifies the contents of an editbox, and takes a single parameter, 'Value' (string).[/td]
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
					[td]DataChange[/td]
					[td]This event occurs when the user has modified the data value of the widget (clicked or unclicked it).[/td]
				[/tr]
			
				[tr]
					[td]LoseFocus[/td]
					[td]This event occurs when the editbox loses keyboard focus (if the user tabs off of it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]GetFocus[/td]
					[td]This event occurs when the editbox receives keyboard focus (if the user tabs on to it or clicks on it, for instance).[/td]
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
	
insert into topic values(null, 2, "widget/execmethod", null,
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
					[td]action causes the widget to execute the method on the server. It can take three parameters, which default to those provided in this widget's properties: \"Objname\", the object path, \"Method\", the method to invoke, and \"Parameter\", the parameter to pass to the method being invoked.[/td]
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
	
insert into topic values(null, 2, "widget/form", null,
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
		
	[b]Usage:[/b]
	
			Although the form widget itself is nonvisual in nature, it can contain visual widgets, including other containers, which may then in turn contain form elements as well.  The form widget may be contained within any widget with a visual container (otherwise, the form elements would not show up on the page).
		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]AllowQuery[/td]
					[td]integer[/td]
					[td]Allow query by form[/td]
				[/tr]
			
				[tr]
					[td]AllowNew[/td]
					[td]integer[/td]
					[td]Allow creation of new records[/td]
				[/tr]
			
				[tr]
					[td]AllowModify[/td]
					[td]integer[/td]
					[td]Allow modification of existing records[/td]
				[/tr]
			
				[tr]
					[td]AllowView[/td]
					[td]integer[/td]
					[td]Allow viewing of existing data[/td]
				[/tr]
			
				[tr]
					[td]AllowNoData[/td]
					[td]integer[/td]
					[td]Allow the 'no data' state[/td]
				[/tr]
			
				[tr]
					[td]MultiEnter[/td]
					[td]integer[/td]
					[td]Enable MultiEnter[/td]
				[/tr]
			
				[tr]
					[td]TabMode[/td]
					[td]string[/td]
					[td]How to react to a tab in a control[/td]
				[/tr]
			
				[tr]
					[td]ReadOnly[/td]
					[td]integer[/td]
					[td]Allow changes[/td]
				[/tr]
			
				[tr]
					[td]_3bconfirmwindow[/td]
					[td]string[/td]
					[td]The name of the window to use for all 3-way confirm operations (save/discard/cancel)[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of formelement child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]fieldname[/td]
							[td]string[/td]
							[td]Fieldname (from the dataset) to bind this element to[/td]
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
					[td]Edit[/td]
					[td]Allows editing of form's contents.[/td]
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
					[td]Save[/td]
					[td]Saves the form's contents.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[i]none currently available[/i]
	
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
	
insert into topic values(null, 2, "widget/formstatus", null,
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
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the widget. No parameters are available from this event.[/td]
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
	
insert into topic values(null, 2, "widget/frameset", null,
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
					[td]title[/td]
					[td]string[/td]
					[td]Title of the frameset for an html page.[/td]
				[/tr]
			
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
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of any child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]framesize[/td]
							[td]integer/string[/td]
							[td]large this frame will be.Can either be expressed as an integer, which represents \"50%\".[/td]
						[/tr]
					
						[tr]
							[td]marginwidth[/td]
							[td]integer[/td]
							[td]width, in pixels, of the margins of the frame.[/td]
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
	
insert into topic values(null, 2, "widget/html", null,
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
					[td]width, in pixels, of the HTML area as a whole.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]dynamic HTML areas, the x coordinate on the container of its upper left corner.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]dynamic HTML areas, the y coordinate on the container of its upper left corner.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]LoadPage[/td]
					[td]loadpage action takes two parameters. \"Source\" contains the URL for the new page to be loaded into the HTML area.The optional parameter \"Transition\" indicates the type of fade to be used between one page and the next.Currently supported values are \"pixelate\", \"rlwipe\", and \"lrwipe\".[/td]
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
	
insert into topic values(null, 2, "widget/htmlwindow", null,
"		[b]htmlwindow[/b] :: A dialog or application window which is a lightweight movable container.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/htmlwindow[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The htmlwindow provides the capability of creating a popup dialog or application window within a web page.  The window is not actually a separate browser window, but rather a movable container that can appear \"floating\" above the rest of the Centrallix application.They can take on one of two styles;\"dialog\" and \"window\", which currently relate only to how the window border and titlebar are drawn.  The windows support windowshading (double-clicking on the titlebar to \"rollup\" or \"rolldown\" the window contents).
			These \"windows\" currently do not support multiple instantiation.
		
	[b]Usage:[/b]
	
			Htmlwindows are normally coded at the top-level of an application, since if they are placed within containers, the container limits where the window can be moved.  These windows can contain other visual and nonvisual widgets.
		
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
					[td]height, in pixels, of the window.[/td]
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
					[td]titlebar[/td]
					[td]yes/no[/td]
					[td]whether the window will have a titlebar. Default \"yes\".[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]The window's title.[/td]
				[/tr]
			
				[tr]
					[td]visible[/td]
					[td]boolean[/td]
					[td]the window is initially visible on screen. The window has an action which can \"true\".[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the window.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the window, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the window, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetVisibility[/td]
					[td]one parameter, \"is_visible\", which is set to 0 or 1 to hide or show the window, respectively.[/td]
				[/tr]
			
		[/table]
	
	[b]Sample Code:[/b]
	
		[code]
		
		
$Version=2$
MyButton \"widget/htmlwindow\"
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
	
insert into topic values(null, 2, "widget/imagebutton", null,
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
					[td]enabled[/td]
					[td]yes/no or expr[/td]
					[td]Whether or not the imagebutton should be enabled.  Defaults to 'yes'.  Can be an expression that if uses runclient() will cause the enabled status of the imagebutton to 'follow' the value of that expression while the application is running.[/td]
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
					[td]height[/td]
					[td]integer[/td]
					[td]height, in pixels, of the image button.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the image button.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Enable[/td]
					[td]This action causes the image button to enter its 'enabled' state, possibly changing its appearance if a 'disabledimage' was explicitly specified.[/td]
				[/tr]
			
				[tr]
					[td]Disable[/td]
					[td]This action causes the image button to enter its 'disabled' state, displaying the 'disabledimage' if specified.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the button. No parameters are available from this event.[/td]
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
	
insert into topic values(null, 2, "widget/menu", null,
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
					[td]orientation[/td]
					[td]string[/td]
					[td]Either \"horizontal\" or \"vertical\" (default), and determines whether the menu is a drop-down/popup (vertical) or a menubar (horizontal).[/td]
				[/tr]
			
				[tr]
					[td]type[/td]
					[td]string[/td]
					[td]Either \"popup\" (default) or \"fixed\".  Popup menus disappear after an item on them is selected, whereas fixed menus remain visible (such as for menubars).[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the menu.  The height is dynamically determined based on the contents.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the menu, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the menu, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of widget/menuitem child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]label[/td]
							[td]string[/td]
							[td]The text to appear on the menu item.[/td]
						[/tr]
					
						[tr]
							[td]value[/td]
							[td]string[/td]
							[td]The 'value' of the menu item, passed to the Selected event, below.  If not specified, it defaults to the name of the widget (not its label).[/td]
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
					[td]Selected[/td]
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
	orientation = \"vertical\"; type = \"popup\";
	bgcolor=\"#808080\";
	
	m1 \"widget/menuitem\" { label=\"One\"; }
	m2 \"widget/menuitem\" { label=\"Two\"; }
	m3 \"widget/menuitem\" { label=\"Three\"; }
	}
		
		
		[/code]
	
");
	
insert into topic values(null, 2, "widget/objectsource", null,
"		[b]objectsource[/b] :: A nonvisual widget which handles communication between forms/tables and the Centrallix server.

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/objectsource[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[tr][td]form element:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The objectsource widget lies at the core of Centrallix's ability to dynamically exchange data between the server and client. This widget implements a form of \"replication\" by maintaining a replica of a small segment of data in the user agent.
			Both form and dynamic table widgets interact with the objectsource nonvisual widget to acquire data, update data, create data, and delete data. In fact, it is possible for more than one form and/or table to be connected with a given objectsource, to perform a variety of functions.
			ObjectSources offer synchronization with other objectsources via the Sync and DoubleSync actions (see below).  These actions allow the application to contain multiple objectsources with primary key / foreign key relationships, and to have those objectsources automatically stay in synchronization with each other based on those relationships.
		
	[b]Usage:[/b]
	
			Objectsource widgets are normally used at the top-level of an application, within a \"widget/page\" object, but can be used anywhere.  The forms/tables that use an objectsource are generally placed within that objectsource.  A page may have more than one objectsource widget.
			Even though the objectsource is nonvisual, it is, like the form widget, a container, and can (should!) contain other visual and nonvisual widgets.
		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]baseobj[/td]
					[td]string[/td]
					[td]If inserts and deletes are to function, the ObjectSystem pathname in which those inserts and deletes should occur.  This should be one of the objects specified in the FROM clause of the SQL statement.[/td]
				[/tr]
			
				[tr]
					[td]replicasize[/td]
					[td]integer[/td]
					[td]Represents the number of records to store in its replica.[/td]
				[/tr]
			
				[tr]
					[td]readahead[/td]
					[td]integer[/td]
					[td]Represents the number to read-a-head when replica is over stepped.[/td]
				[/tr]
			
				[tr]
					[td]scrollahead[/td]
					[td]integer[/td]
					[td]Represents the number to step forward when viewing records (Currently set to readahead).[/td]
				[/tr]
			
				[tr]
					[td]sql[/td]
					[td]string[/td]
					[td]The SQL statement used to retrieve the data from the server.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]DoubleSync[/td]
					[td]Performs a double synchronization with two other objectsources, known as the Parent and the Child, in two steps.  The first step is like Sync (see below), with a ParentOSRC and ParentKey[1-9]/ParentSelfKey[1-9].  Next, a Sync is performed between the current objectsource and the ChildOSRC in the same way the first step performed a sync between the ParentOSRC and the current objectsource, respectively, using SelfChildKey[1-9]/ChildKey[1-9].[/td]
				[/tr]
			
				[tr]
					[td]Sync[/td]
					[td]Performs a synchronization operation with another objectsource by re-running the query for this objectsource based on another objectsource's data.  Used for implementing relationships between objectsources.  The ParentOSRC (string) parameter specifies the name of the objectsource to sync with.  Up to nine synchronization keys can be specified, as ParentKey1 and ChildKey1 through ParentKey9 and ChildKey9.  The ParentKey indicates the name of the field in the ParentOSRC to sync with (probably a primary key), and ChildKey indicates the name of the field (the foreign key) in the current objectsource to match with the parent objectsource.[/td]
				[/tr]
			
				[tr]
					[td]Clear[/td]
					[td]Clears the replica of data.[/td]
				[/tr]
			
				[tr]
					[td]Query[/td]
					[td]Query[/td]
				[/tr]
			
				[tr]
					[td]QueryObject[/td]
					[td]Query Object[/td]
				[/tr]
			
				[tr]
					[td]OrderObject[/td]
					[td]Order Object[/td]
				[/tr]
			
				[tr]
					[td]Delete[/td]
					[td]Deletes an object through OSML[/td]
				[/tr]
			
				[tr]
					[td]Create[/td]
					[td]Creates an object though OSML[/td]
				[/tr]
			
				[tr]
					[td]Modify[/td]
					[td]Modifies an object through OSML[/td]
				[/tr]
			
				[tr]
					[td]First[/td]
					[td]Returns first record in the replica[/td]
				[/tr]
			
				[tr]
					[td]Next[/td]
					[td]Returns next record in the replica[/td]
				[/tr]
			
				[tr]
					[td]Prev[/td]
					[td]Returns previous record in the replica[/td]
				[/tr]
			
				[tr]
					[td]Last[/td]
					[td]Returns last record in the replica[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[i]none currently available[/i]
	
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
	}
		
		
		[/code]
	
");
	
insert into topic values(null, 2, "widget/page", null,
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
					[td]textcolor[/td]
					[td]string[/td]
					[td]The default color for text which appears on the page.  Can either be a named color or RGB (numeric) color.[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]The title which will appear in the browser's window title bar.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
		[i]none currently available[/i]
	
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
	
insert into topic values(null, 2, "widget/pane", null,
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
					[td]height[/td]
					[td]integer[/td]
					[td]height, in pixels, of the pane.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]string[/td]
					[td]Either \"raised\" or \"lowered\", and determines the style of the pane's border.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the pane.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the pane, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the pane, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
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
	
insert into topic values(null, 2, "widget/radiobuttonpanel", null,
"		[b]radiobuttonpanel[/b] :: A visual widget displaying a set of 'radio buttons'

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]widget/radiobuttonpanel[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[tr][td]form element:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			A radio button panel widget is a form element used to create a set of radio buttons on screen.  Only one radio button may be selected at a time.
		
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
					[td]height, in pixels, of the panel.[/td]
				[/tr]
			
				[tr]
					[td]outlinecolor[/td]
					[td]string[/td]
					[td]The color, RGB or named, of the rectangular border drawn around the radio buttons.[/td]
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
					[td]width, in pixels, of the panel.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the panel, default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the panel, default is 0.[/td]
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
							[td]The value of the selected item[/td]
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
	
insert into topic values(null, 2, "widget/remotectl", null,
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
	
insert into topic values(null, 2, "widget/remotemgr", null,
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
	
insert into topic values(null, 2, "widget/scrollpane", null,
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
			
				[tr]
					[td]visible[/td]
					[td]boolean[/td]
					[td]allows user to set scroll pane to visible (true) or not (false).[/td]
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
	
insert into topic values(null, 2, "widget/tab", null,
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
		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]background[/td]
					[td]string[/td]
					[td]An image to be used as the background of the tab control.This is useful for adding control.[/td]
				[/tr]
			
				[tr]
					[td]bgcolor[/td]
					[td]string[/td]
					[td]As an alternate to \"background\", \"bgcolor\" can specify a color, either named or RGB, control.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]integer[/td]
					[td]the height, in pixels, of the entire tab control, including the tabs and pages.[/td]
				[/tr]
			
				[tr]
					[td]selected[/td]
					[td]string[/td]
					[td]The name of the tab page that should be initially selected.[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]The color of the text to be used on the tabs to identify them.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the entire tab control.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the tab control, relative to the container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the control, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of any child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]title[/td]
							[td]string[/td]
							[td]The name of the tab page, which appears in the tab at the top of the control.[/td]
						[/tr]
					
				[/table]
			
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
	
insert into topic values(null, 2, "widget/table", null,
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
		Table widgets come in three different flavors: static, dynamicpage, and dynamicrow.Static table widgets are built on the server and write their data directly into the container in which they reside, which is usually a scrollpane widget.  Dynamicpage table widgets load their data once they initialize on the client, by activating a query through an ObjectSource nonvisual widget.Dynamicpage table widgets do not support modification, but can be reloaded through an ObjectSource at will.Dynamicrow table widgets, on the other hand, display each row as an individual layer, and thus are modifiable on the client. Dynamicrow table widgets also load their contents through an ObjectSource widget query.As of the time of writing of this document, only static mode and dynamicrow mode were supported.
			Table widgets allow the selection (keyboard, mouse, and data focus) of individual rows.
		
	[b]Usage:[/b]
	
			Table widgets are normally placed inside of a scrollpane so that any rows which don't fit int the container can still be viewed. Table columns are created via \"widget/table-column\" child widgets within the table widget.
		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
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
					[td]mode[/td]
					[td]string[/td]
					[td]The mode of operation of the table, either \"static\" to be generated on the server when the application is generated, \"dynamicpage\" to be generated on the client a page at a time, or \"dynamicrow\", to be generated on the client a row at a time.  Default is \"static\".[/td]
				[/tr]
			
				[tr]
					[td]outer_border[/td]
					[td]integer[/td]
					[td]width of the outer spacing around the outside of the table, in pixels.  Default0.[/td]
				[/tr]
			
				[tr]
					[td]row_background1[/td]
					[td]string[/td]
					[td]A background image for the table row cells.[/td]
				[/tr]
			
				[tr]
					[td]row_bgcolor1[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the table row cells.[/td]
				[/tr]
			
				[tr]
					[td]row_background2[/td]
					[td]string[/td]
					[td]A background image for the table row cells.  If this is specified, rows will alternate in backgrounds between \"row_background1\" and \"row_background2\".[/td]
				[/tr]
			
				[tr]
					[td]row_bgcolor2[/td]
					[td]string[/td]
					[td]A color, RGB or named, for the table row cells.  If this is specified, rows will alternate in colors between \"row_bgcolor1\" and \"row_bgcolor2\".[/td]
				[/tr]
			
				[tr]
					[td]rowheight[/td]
					[td]integer[/td]
					[td]The height of the individual rows in pixels.  Default is 15 pixels.[/td]
				[/tr]
			
				[tr]
					[td]sql[/td]
					[td]string[/td]
					[td]The SQL query to be used for obtaining the data for the table.This query's result set child widgets.[/td]
				[/tr]
			
				[tr]
					[td]textcolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the text in the normal data rows.[/td]
				[/tr]
			
				[tr]
					[td]textcolorhighlight[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the text in the highlighted data row.[/td]
				[/tr]
			
				[tr]
					[td]titlecolor[/td]
					[td]string[/td]
					[td]A color, RGB or named, of the text in the header row.  If unset, defaults to textcolor.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the table.The height is determined dynamically.[/td]
				[/tr]
			
				[tr]
					[td]windowsize[/td]
					[td]integer[/td]
					[td]The maximum number of rows to show at any given time, for dynamic tables.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the table. Default is 0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the table. Default is 0.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of widget/table-column child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]title[/td]
							[td]string[/td]
							[td]The title of the column to be displayed in the header row.[/td]
						[/tr]
					
						[tr]
							[td]width[/td]
							[td]integer[/td]
							[td]width of the column.[/td]
						[/tr]
					
				[/table]
			
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
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
	
insert into topic values(null, 2, "widget/textarea", null,
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
					[td]readonly[/td]
					[td]yes/no[/td]
					[td]Set to 'yes' if the data in the text area should be viewed only and not be modified[/td]
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
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the widget. No parameters are available from this event.[/td]
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
					[td]DataChange[/td]
					[td]This event occurs when the data value of the widget has changed.[/td]
				[/tr]
			
				[tr]
					[td]Modified[/td]
					[td]This event occurs when the user modifies the data value of the widget (the distinction from 'DataChange' is that in this case the user modified the data value, whereas DataChange can occur even when the form containing the textarea views a new object, but also by user modification).[/td]
				[/tr]
			
				[tr]
					[td]LoseFocus[/td]
					[td]This event occurs when the textarea loses keyboard focus (if the user tabs off of it, for instance).[/td]
				[/tr]
			
				[tr]
					[td]GetFocus[/td]
					[td]This event occurs when the textarea receives keyboard focus (if the user tabs on to it or clicks on it, for instance).[/td]
				[/tr]
			
		[/table]
	
	[b]Client Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, 2, "widget/textbutton", null,
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
					[td]height, in pixels, of the text button.[/td]
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
					[td]width[/td]
					[td]integer[/td]
					[td]The width, in pixels, of the text button.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the button, relative to its container.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Actions:[/b]
	
		[i]none currently available[/i]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]Click[/td]
					[td]This event occurs when the user clicks the button. No parameters are available from this event.[/td]
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
	
insert into topic values(null, 2, "widget/timer", null,
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
					[td]the timer starts counting down again immediately after it expires.[/td]
				[/tr]
			
				[tr]
					[td]auto_start[/td]
					[td]boolean[/td]
					[td]the timer starts counting down immediately after the page loads.[/td]
				[/tr]
			
				[tr]
					[td]msec[/td]
					[td]integer[/td]
					[td]of milliseconds (1/1000th of a second) before the timer expires.[/td]
				[/tr]
			
		[/table]
	
	[b]Actions:[/b]
	
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		
				[tr]
					[td]SetTimer[/td]
					[td]action takes two parameters: \"Time\" (integer in milliseconds) and \"AutoReset\" (integer 0 or 1). It causes a timer to begin counting down towards an Expire event.[/td]
				[/tr]
			
				[tr]
					[td]CancelTimer[/td]
					[td]actions causes a timer to stop counting down, and thus no Expire event will occur until another countdown sequence is initiated by a SetTimer action.[/td]
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
	
insert into topic values(null, 2, "widget/treeview", null,
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
					[td]source[/td]
					[td]string[/td]
					[td]The ObjectSystem path of the root of the treeview.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]integer[/td]
					[td]width, in pixels, of the treeview.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]integer[/td]
					[td]x-coordinate of the upper left corner of the treeview's root object.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]integer[/td]
					[td]y-coordinate of the upper left corner of the treeview's root object.[/td]
				[/tr]
			
		[/table]
	
	[b]Events:[/b]
	
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		
				[tr]
					[td]ClickItem[/td]
					[td]occurs when the user clicks on the clickable link for an item in the treeview. Its one parameter is \"Pathname\", or the ObjectSystem path to the object which was selected.[/td]
				[/tr]
			
				[tr]
					[td]RightClickItem[/td]
					[td]to the above, but when the user right-clicks on an item in the treeview.[/td]
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
	
insert into topic values(null, 2, "widget/variable", null,
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
					[td]value[/td]
					[td]integer/string[/td]
					[td]initial value of the variable.[/td]
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
	