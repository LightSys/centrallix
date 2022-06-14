# "AutoForm" widget specification
Author: Greg Beeley (GRB)

Date: November 14th, 2006

## Overview
A key element of the Centrallix application server is simplicity and ease of application authoring.  To help meet this requirement, the "autoform" widget is being introduced.

An autoform is similar in some ways to the form widget, but it is designed to automatically fill in all needed form element widgets in a meaningful manner.  This is done basically by querying the type of object to be displayed in the form, determining what fields/attributes are present, and then using a set of heuristics, automatically do the layout for the widget.

In this way an autoform is a "composite widget", having a simple definition in the .app file but resulting in a potentially complex set of widgets in the actual widget tree to be deployed to the client.  At the time of writing, the only other composite widget is the formbar, which is more of a demonstration of what is possible with composite widgets than anything else.

## Heuristics
The autoform will use semi-intelligent heuristics to do the form element layout.  These heuristics are briefly itemized below.

### A.	Use Presentation Hints and Data Types for Widget Selection.
The autoform should query presentation hints for each field in order to determine what type of form element to use, in addition to considering the data type.

### B.	Create both Labels and Form Elements.
The autoform will create labels, where desirable, for each form element generated.

### C.	Use Field Length to Size Editboxes and Textareas.
Field lengths, from presentation hints, an be an important cue in the sizing of editboxes on screen.  However, this can't be followed exactly; a String(255) field would not necessitate an editbox that could display 255 characters all at once!

### D.	Keep Different Widget Sizes to a Minimum.
When laying out the form, choose a minimum set of different widget widths (e.g., editbox widths).  This keeps the visual layout looking cleaner.

### E.	Use a Columnar Layout.
Form Elements should be displayed in columns, with a list of labels on the left and a list of form elements on the right.  Labels should be right-justified, and elements aligned on the left side.

The .app creator may choose to specify the number of widget columns to use, or the autoform may determine that dynamically given the available width and height.

### F.	Use a Tab Control or "Advanced" Window if Needed.
Should the needed fields not fit in the required area, such devices as a tab control or a window can be used.  When dividing form elements between the display areas, attention should be given to the presentation hints data.  The "Show Container" API on the client will result in the window or tab automatically changing/opening should the user tab between fields not in the same display area.

A scrollpane widget can also be used when form elements will not fit in the given area.  Attention should be given to make sure that the scrollpane automatically scrolls to show the element of focus.

### G.	Choosing to Display Navigation Buttons.
Forms usually need navigation buttons, such as the formbar and/or Search/New/Edit/Save/Cancel/Delete buttons.  The .app file author should be able to specify which of these is needed.  If the form is marked non-editable, for example, the Edit and Save buttons may be omitted.  A search-only form would only require a Search button and a Cancel button.

### H.	Group Similar Widgets Together.
When there are several checkboxes, those should be placed together to neaten up the visual appearance.  It may or may not be advisable to do this with other widget types.

### I.	Follow Native Field Order.
When possible and reasonable, the autoform should lay out the form elements in a similar order to how they natively were listed in the object that came from the server.

### J.	Group Related Fields Near Each Other.
Some fields may have presentation hint data, such as constraints or defaults, which link them to other fields.  Such fields can, if possible, be displayed near each other rather than farther away.

Fields also might be grouped in the Presentation hints via the 'group name' hint.  Such fields should nearly always be grouped together.

### K.	Save Space by Grouping Short Editboxes.
If there are several editboxes which are shorter than usual, some space can be saved by putting more than one editbox in an area where ordinarily only one editbox might fit.  Examples of this include City/State/Zip editboxes.

### L.	Use "Pick" Buttons.
For fields that indicate a code list enumeration or SQL query, a pick button can be generated.
