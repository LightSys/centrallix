# HTML Generation Subsystem Form/FormElement API

Author: Greg Beeley (GRB)

Date: October 30, 2001

## Table of Contents
- [HTML Generation Subsystem Form/FormElement API](#html-generation-subsystem-formformelement-api)
  - [Table of Contents](#table-of-contents)
  - [I Introduction](#i-introduction)
  - [II Form Elements](#ii-form-elements)
    - [A.  Types of Focus](#a--types-of-focus)
    - [B.  Interaction with the Page widget.](#b--interaction-with-the-page-widget)
    - [C.	Interaction with the Form Widget.](#cinteraction-with-the-form-widget)
  - [IV The Form Widget](#iv-the-form-widget)
  - [V Objectsource Widgets](#v-objectsource-widgets)
    - [A.	Model of Operation](#amodel-of-operation)
    - [B.  Interaction with the Centrallix Server](#b--interaction-with-the-centrallix-server)
    - [C.	Interaction with the Form Widget.](#cinteraction-with-the-form-widget-1)
  - [VI Summary of Form Element, Form, and Objectsource Widgets](#vi-summary-of-form-element-form-and-objectsource-widgets)
    - [A.	ObjectSource Widget](#aobjectsource-widget)
      - [Methods:](#methods)
      - [Callbacks From the Form Widget:](#callbacks-from-the-form-widget)
    - [B.	Form Widget](#bform-widget)
      - [Methods:](#methods-1)
      - [Modes:](#modes)
      - [Status/Control Flags:](#statuscontrol-flags)
      - [Other Properties Used By ObjectSource Widget:](#other-properties-used-by-objectsource-widget)
      - [Callbacks From ObjectSource Widget:](#callbacks-from-objectsource-widget)
      - [Callbacks/Methods From Form Element Widgets:](#callbacksmethods-from-form-element-widgets)
    - [C.	Form Element Widget](#cform-element-widget)
      - [Methods/Actions:](#methodsactions)
      - [Properties Used by the Form Widget:](#properties-used-by-the-form-widget)
      - [Callbacks Used by the Page Widget:](#callbacks-used-by-the-page-widget)

## I Introduction
This document covers the details of how forms, form elements, data sources, and reloadable containers interoperate on a generated DHTML page.

## II Form Elements
Form elements, such as checkboxes and editboxes, contain single pieces of scalar data that the user can view, modify, query, etc.  These form elements will implement a standard API to interact with the Form widget and the Page widget.

### A.  Types of Focus
Centrallix applications contain widgets which deal with three different kinds of focus on the page: keyboard focus, mouse focus, and data focus.  Keyboard focus visually indicates which widget will receive keypresses when the user begins typing.  Mouse focus visually indicates which widget is being pointed to by the mouse.  Typically the user can transform mouse focus into keyboard focus by clicking.  Finally, data focus indicates which data record(s) are "selected" or "current" in the controls on screen.  While only one widget can have mouse focus or keyboard focus at a time, more than one record can have data focus at a time, as long as those records are from different data sources. Thus, clicking on a multi-record control could not only give a record keyboard focus, but also data focus.  Data focus determines what is potentially shown on other parts of the screen, as well as what record is affected when the user performs some operation, such as deleting or viewing detail.

### B.  Interaction with the Page widget.
The Page widget controls much of the top-level handling of events, including which controls have focus and where keypresses are directed. Widgets which wish to receive keyboard, mouse, or data focus.  In order to properly interact with the widgets on the page, the page widget requires that other widgets that need to receive focus and/or keystrokes implement up to three callback functions:

| Function              | Description
| --------------------- | -----------
| keyhandler()          | this callback function on a widget is called when the page sends a keypress to the widget. If the widget uses the keypress, it should return true to prevent the keypress from being used by other widgets.
| getfocushandler()     | This function is called when a widget receives keyboard focus.  It should return the logical OR of zero or more of: 1 - set the keyboard focus to this area, and 2 - set the data focus to this area.
| losefocushandler()    | This function is called when a widget is about to lose keyboard focus.

Finally, in order to be able to receive focus, widgets must "register" "focusable areas" with the page.  This is done with the pg_addarea() function:

pg_addarea(l,x,y,w,h,n,c,f)

- l: The layer that will receive focus.  This layer should also implement most or all of the above callback functions as properties of the layer.
- x,y: The x,y location relative to the layer where the focus rectangle(s) will be placed.  Generally speaking, most widget layers specify the x,y as 0,0 or -1,-1.  The latter case is used when the focus rectangle should appear one pixel "outside" of the widget.
- w,h: The width/height of the focus rectangle.  Often this is specified as "layer.clip.width" and "layer.clip. height" or these values plus one.
- n: The "name" of the area.  Primarily used by the above callback functions.
- c: The "context" of the area.  Primarily used by the above callback functions.
- f: The "flags".  This is the logical OR of zero or more of the below flags:
    - 1: Allow the area to receive keyboard focus.
    - 2: Allow the area to receive data focus.

### C.	Interaction with the Form Widget.
A form widget, of which there can be more than one on a page, controls data at the record level, as well as what happens when data is modified, saved, submitted, queried, etc.

Forms can have one of several modes of operation, which can be shown onscreen via a "form-status" widget.

| Mode       | Description
| ---------- | ------------
| "View"     | Data is being viewed.  The data from the objectsource widget may contain several records, in which case the form widget is free to move among the records.
| "Modify"   | Data is being modified for an existing object.  If the objectsource widget contains multiple object records, the form may NOT browse among them without going back to "View" mode by either 1) cancelling the changes it is making, or 2) saving the changes it has made.
| "New"      | A new unsaved object is being created.  The record is not yet in the objectsource nonvisual widget, and is thus not in the server's objectsystem either.  A form in this mode can transition to "View" by saving the record.
| "Query"    | The form is being used as a query form.  That is, the data elements on the form can be filled in to provide query criteria.  When the form performs the query, it loads the objectsource widget with resulting records and then transitions to "View" mode for the first of those records.  A forced transition from the "View" state to "New" after a query has been performed will automatically load the form fields with query criteria data, where possible.
| "No Data"  | The form does not have any data loaded, and has not been told to create a new record.  This can be the result of no activity being performed on the form, or if no objects were returned by a query.  A trans- ition from this state to "New" will automatically fill in fields according to the state of the most recent query (if the query returned empty).  The "No Data" state can also result if the user just deleted the last record that was contained in an objectsource that the form is currently using.

The form element widget (such as an editbox, checkbox, etc.) should have several callback methods that the form widget can use to manage the form element.

| Method          | Description
| --------------- | ------------
| getvalue()      | should return the current value of the form element.
| setvalue()      | is used to set the current value of the form element, triggering the 'Modified' and 'Changed' events on the widget.  A 'flags' parameter passed to this method can however override the triggering of one or both of the 'Modified' and 'Changed' events.  The 'Modified' event is supposed to indicate that the user changed the value, whereas the 'Changed' event indicates that the value simply changed, no matter what caused it.
| clearvalue()    | is used to clear the current values.  In the case of an edit box, this would set it to the "" string. For a radio button, it would make no radio buttons selected.  Note that this is different than the resetvalue() function.
| resetvalue()    | is used to reset the value of a widget to its original state (if any).  If no original state has been defined, this will have the same effect as clearvalue().
| setoptions()    | is used to set the possible values for the widget. This is used, for instance, on dynamically-populated drop-down list boxes.
| enable()        | Enables the widget so that it can receive focus.
| readonly()      | Sets the widget so that it does not appear disabled, but cannot receive keyboard focus.
| disable()       | Disables the widget so that it cannot receive focus, and thus looks disabled.

Finally, the form element widget needs to notify the form widget when changes occur in its value.  This is done via calls placed to form widget functions.

| Function        | Description
| --------------- | ------------
| register()      | Tells the form widget that the form element widget is a part of the form.
| datanotify()    | Notifies the form widget that the value of a form element has changed.
| focusnotify()   | Tells the form widget that one of the form elements just received focus.

When a form element widget initializes, it can determine the form widget to which it is attached by examining the global javascript variable "fm_current".  

## IV The Form Widget
Form widgets handle a collection of form elements, which collectively represent a single object's attributes (a record) on screen.

Form widgets can have one of five different modes of operation, and can transition from one mode to another based on programmatic or end-user commands.  The modes, briefly described earlier, are as follows:

- "View" -    Viewing an object's data (a record)
- "Modify" -  Modifying an object's data
- "New" -	    Creating a new (unsaved) object
- "Query" -   Preparing to query for objects, using a QBF (query by form) approach
- "No Data" - The form does not contain any data, and has not been commanded to create a new record.

Forms can be commanded to transition from one mode to another either programmatically or as initiated by the end-user.  The following events or calls can occur to cause this to happen.

| Event      | Description
| ---------- | ------------
| Save       | Saving causes a form in Modify or New mode to send data to the server and then transition to View mode.
| Edit       | Requesting an Edit operation causes a form to transition from No Data to New or from View to Modify.  A form in View mode can have an implicit Edit command performed if the user clicks on a form element, causing it to receive focus. This implicit Edit can be disabled in the form's config- uration.  Forms in No Data mode need an explicit Edit command.  
| New        | Causes a form in View, Query, or No Data mode to clear its contents and then transition to New mode.  If the form was in Modify mode, the user will be prompted to either do a Save or Discard operation, or to Cancel the New request. If the form was in No Data or View mode as the result of a query, the fields can be auto-populated with the relevant query criteria.
| Discard    | Causes a form in Modify mode to cancel its changes and then transition back to View mode.  Causes a form in New mode to cancel its changes and transition back to No Data mode. Causes a form in Query mode to be cleared and then remain in query mode.
| Query      | Causes a form in View, Query, or No Data mode to clear its contents and then transition to Query mode.  A form in Modify mode will prompt the user for Save or Discard first. If the form was in View or No Data mode as the result of a query, the query fields will be auto-populated with the original query criteria.
| QueryExec  | Causes a form in Query mode to send a data request to the objectsource, and then transition to View mode if data was returned, or to No Data mode if the query returned no objects.  If more than one form is attached to the given objectsource, the form in Query mode will stay in query mode if View mode is not allowed for the form.
| Delete     | Causes a form in New mode to clear and remain in New mode. Causes a form in View mode to delete the record and then move to View mode on the next record or to No Data mode. Causes a form in Query mode to return to Query mode with no data in the query fields.
| Clear      | Causes a form to transition to No Data mode.  If the form had unsaved data resulting from a New or Modify mode, then the user is prompted to Save or Discard first.  This operation also clears any rows in the objectsource widget. (it does not delete them from the server, but rather just disconnects the replica).

All of the above can be accessed as Actions on the form widget, either via simple connectors or via scripts.  The below events that cause status changes are not generally initiated by the user.

| Event                 | Description
| --------------------- | ------------
| ObjectNoLongerExists  | This happens when for some reason the object- source nonvisual widget loses the record being viewed or edited in the form widget.  This can be because the record was deleted via another form or table attached to the objectsource, or because the objectsource received a data replication message from the server indicating that the object/record was deleted.  The form widget will respond by returning to View mode if other data is present in the objectsource, or to No Data, as appropriate.
| ObjectChanged         | This happens when some other form, table, or replication message causes the objectsource's copy of the data to be changed.  It will transition a form in Modify mode back to View mode with the updated information, or simply update a form already in View mode.
| ObjectCreated         | This happens when a form in No Data mode as a result of an empty query result set can transition to View mode because a replication message was received that inserted a row matching the criteria, or because some other form or table on the screen created a record in the object- source.

There will be a special "form status" widget which can reflect the mode the form is in.  The form status widget is a special case in that it cannot receive focus, but the form will use the standard setvalue() callback in order to change the visual representation of the widget.  It is also possible to have more than one kind of form status widget; these widgets only need to have a property "isFormStatusWidget" that is set to true.

Form status widgets can reflect 1) the mode of the form, and 2) whether the form has an operation pending with the objectsource widget or not.  For example, the form status widget can display a clock or hourglass to indicate that an operation is pending.

## V Objectsource Widgets
The objectsource widget supplies a connection with the Centrallix server as far as on-page data is concerned.  Eventually, the objectsource widget will implement replication-to-the-client, although this probably will not be implemented in the initial versions.

There are three possible ways for the objectsource widget to communicate with the Centrallix server.  First, it can use the standard OSML-over-HTTP approach that is implemented in the net_http network driver.  Second, it can use the BDQS-over-HTTP approach that will be available pending the implementation of the BDQS.  Finally, it can possibly use a Java applet to facilitate the implementation of a TCP sockets version of the BDQS protocol for direct and fast communication with the server.

This document will, for now, examine the use of the simple OSML-over-HTTP approach to obtaining data.

### A.	Model of Operation
The objectsource widget is designed to be a replica of a certain selected segment of data from the Centrallix server.  As a result, the entire mechanism is somewhat asynchronous:  due to the nature of the DHTML environment and because of the nature of maintaining a replica, when the data is sent to the server the form(s) are left with a pending status; once the results come back from the server, the appropriate controls on the DHTML page are then (and only then) notified of the results.

The replica of objects sends and receives updates to and from the server as changes occur.  The form and/or table widgets involved are the widgets which cache the changes until changes are ready to be saved; once changes are saved into the objectsource, the objectsource then immediately communicates with the server.

Whether the server sends unsolicited updates to the objectsource widget in order to maintain its replica depends on that event/replica functionality being present in the server.  At this point it is not, so the replica is not maintained automatically.

### B.  Interaction with the Centrallix Server
The objectsource widget will communicate with the Centrallix server via one or more hidden layers.  When a request needs to be placed to the server, the layer will be reloaded with the appropriate url-encoded arguments.  When the layer finishes loading, a callback is triggered on the objectsource widget, telling it to process the returned data in the layer.

Once data has been obtained, the objectsource widget will then trigger the loading of any related forms/tables onscreen with the returned data.

### C.	Interaction with the Form Widget.
The interaction with the Form widget is bi-directional.  When the form widget desires to save changes, create objects, delete objects, or issue queries, it will issue commands thus to the objectsource widget. In response, when data becomes available, the objectsource widget then notifies the form widget of such.

Furthermore, if multiple form or table widgets are attached to a single objectsource widget, then changes that one form or table widget make to the objectsource can cause the objectsource to interact with the other form and table widgets to update their contents.

Below are the basic events that occur with the objectsource widget that cause it to interact with the server and/or the form widget.

1.  Query Is Issued

    The first item of business is for the objectsource widget to check for any attached form or table widgets which are in a New or Modified state and allow those widgets to possibly cancel the query operation, or continue it later once changes have been saved.  This requires the form/table in question to place a completion callback to the objectsource once that operation has completed.

    Alternately, for a simpler implementation, the objectsource could simply notify the user to either save or cancel changes before issuing a new query, and cancel the query outright.

    All attached forms and tables then receive a 'Pending' status, and possibly a 'No Data' mode.

    When a form widget, table widget, or other outside source issues a query through the objectsource widget, the objectsource widget then allocates a hidden layer (if needed) and sends the query to the server as a GET request on that hidden layer.  The objectsource widget sets the layer's onLoad event to one of its internal completion routines.  The query call then completes as far as the DHTML application is concerned and nothing happens until the layer finishes loading and the onLoad completion event hander is called.

    Once onLoad is called, the objectsource then scans the loaded layer for data and loads the data into its internal object replica data structures.  It then notifies all attached forms/tables of the arrival of new data (or the lack of new data, if the query's result set was empty).  From that point, it is the responsibility of those tables/forms to set result set cursors and load data, as may be appropriate.

    The objectsource widget may choose not to load the entire result set to the client.  As a result, when a table or form requests data from the objectsource, a callback function must be supplied so that the objectsource has the chance to load new records from the query result set before displaying them.

    In summary, the process is as follows:

    - a.  A query is placed on the objectsource.
    - b.  Objectsource places notification to any new/modified tables and forms
    - c.  Tables/forms issue callback to objectsource indicating that they are ready for the objectsource data to be cleared.
    - d.  Objectsource clears internal object replica.
    - e.  Objectsource issues query to server.
    - f.  Server returns completion of query start.
    - g.  Objectsource then issues first fetch to server.
    - h.  Server returns completion of fetch, along with data.
    - i.  Objectsource notifies tables/forms that data is becoming available.
    - j.  Tables/forms request object info from objectsource.
    - k.  If objectsource has the objects in its replica, it returns the object data immediately (via placing callbacks with the tables/forms), otherwise issues more fetches to server.
    - l.  When data comes in from fetches, and is what the tables/ forms requested, the objectsource places callbacks with the tables/forms letting them know the data has arrived and providing them the data.
    - m.  When the objectsource senses end-of-query-results from the server, and tables/forms were still waiting on data, the objectsource does a callback to the tables/forms telling them that end of data has been reached.

2.  An Object is Modified

    When a table or form widget modifies data in the objectsource, the objectsource then must perform updates to the server.  It does this by using a hidden layer to place a GET request to the server which encodes the object update.  When the update completes, the layer's onLoad event handler will trigger, causing the objectsource to let the form/table know that its pending status may be cleared.

    At that point, the objectsource also notifies other forms/tables that the data they are displaying may have changed, and providing the new data for those objects.

    It may be possible that the objectsource should notify other tables or forms of the modification before sending the modification to the server.

3.  An Object is Deleted

    If a table or form deletes an object, two things must be done, as in the case of the modification of the object.  One, the object- source must issue a delete operation to the server and wait for the completion onLoad event from that operation, and two, the objectsource must notify any tables/forms that an object has been removed from the replica.

4.  An Object is Created

    This case is handled in a similar manner to the deleted/modified cases already examined.

## VI Summary of Form Element, Form, and Objectsource Widgets
This section presents a summary of the interaction methods, callbacks, and properties that the form element widgets, form widgets, and objectsource widgets will have.

### A.	ObjectSource Widget

#### Methods:

1.  Clear() method - equivalent to a query returning no records.
2.  Query() method - issues a query to the server and loads new objects
    into the objectsource's object replica.
3.  Delete() method - delete an object.
4.  Create() method - create an object.
5.  Modify() method - modify an object.

#### Callbacks From the Form Widget:

1.  QueryContinue() callback - when a query was issued but the form had
    unsaved data, this is called when the form has saved or discarded
    the data.
2.  QueryCancel() callback - same as above, except in this case the
    user chose to cancel the query instead of saving/discarding the
    form contents.
3.  RequestObject() callback - the form widget needs the data for one
    or more specific object(s).

### B.	Form Widget

#### Methods: 

1.  Save() method/action - saves the form's contents.
2.  Edit() method/action - allows editing of form's contents.
3.  Discard() method/action - cancels an edit of form contents.
4.  New() method/action - allows creation of new form contents.
5.  Query() method/action - allows entering of query criteria.
6.  QueryExec() method/action - execs the query and returns data.
7.  Delete() method/action - deletes the current object displayed.
8.  Clear() method/action - clears the form to a 'no data' state.
9.  Next() method/action - moves the form to the next object in the
    objectsource.
10. Prev() method/action - moves the form to the previous object in
    the objectsource.
11. First() method/action - moves the form to the first object in 
    the objectsource.
12. Last() method/action - moves the form to the last object in the
    objectsource.

#### Modes:

1.  No Data - form inactive/disabled, no data viewed/edited.
2.  View - data being viewed readonly.
3.  Modify - data being modified.
4.  Query - query criteria being entered.
5.  New - new object being entered/created.

#### Status/Control Flags:

1.  Pending - results or confirmation of an operation are pending from the server.
2.  AllowQuery - whether the form allows query mode or not.
3.  AllowNew - whether the form allows new record creation.
4.  AllowModify - whether the form allows data modification mode.
5.  AllowView - whether the form can be used for browsing data.
6.  AllowNoData - if this is set to false, and a NoData mode is about to be entered, the form will select either New or Query mode, according to what other flags are set to true or false.
7.  MultiEnter - whether, when saving a New object, the form auto transitions to the New mode again with an empty form.  This automatically happens if AllowView is false.

#### Other Properties Used By ObjectSource Widget:

1.  IsUnsaved - set to True if there is unsaved data in the form. This will be set true for a form in New or Modify mode where a form element has issued a DataNotify() callback.

#### Callbacks From ObjectSource Widget:

1.  IsDiscardReady() callback - this is called if the UnsavedData property is true and the objectsource's Query() method was called.
2.  DataAvailable() callback - this is called after a QueryExec when data first starts to be returned from the query.
3.  ObjectAvailable() callback - this is called when the objectsource has the requested object(s) for the form.
4.  OperationComplete() callback - this is called when an insert, delete, or modify operation has completed (on success or error). This is also called when an end-of-query is reached (with or without returned rows).
5.  ObjectDeleted() callback - this is called asynchronously when an object no longer exists.
6.  ObjectCreated() callback - this is called async. on obj creation.
7.  ObjectModified() callback - this is called when an object is modified.

#### Callbacks/Methods From Form Element Widgets:

1.  Register() method - when a form element initializes, it registers itself with the form that contains it.
2.  DataNotify() method - when the value of a form element changes, it lets the Form widget know about the change.
3.  FocusNotify() method - when a form element receives or loses focus, it lets the Form widget know about the focus change.

### C.	Form Element Widget

#### Methods/Actions:

1.  SetValue() method/action - sets the value of a form element.
2.  GetValue() method - gets the value of a form element.
3.  SetOptions() method - sets the possible values of a form element such as a drop-down list.
4.  SetStatus() method/action - sets the status of the form element to one of Enabled, Readonly, or Disabled.

#### Properties Used by the Form Widget:

1.  IsUnsaved - indicates that the form element has unsaved data.

#### Callbacks Used by the Page Widget:

1.  keyhandler() callback - called when a keypress is received and the widget has keyboard focus.
2.  getfocushandler() callback - called when the widget is receiving the keyboard focus.
3.  losefocushandler() callback - called when the widget is losing the keyboard focus.
