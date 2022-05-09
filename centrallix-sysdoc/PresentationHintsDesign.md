Title:    Design for Presentation Hints use in Applications
Author:   Greg Beeley
Date:     12-Jun-2002
License:  Copyright (C) 2002 LightSys Technology Services.  See LICENSE.txt.
-------------------------------------------------------------------------------

OVERVIEW....

    The "presentation hints" mechanism in Centrallix allows for "extended"
    information to be specified for fields/attributes of objects.  This type of
    information allows for database-specific field information to be included
    in the Centrallix OSML API, and allows for presentation business rules to
    be easily implemented.

    The goal with the presentation hints mechanism is to provide a simple way
    for these design items to be specified without procedural coding.


PRESENTATION HINTS COVERAGE....

    The presentation hints mechanism includes a wide variety of attribute data,
    as listed below:

	MinValue	An expression which supplies the minimum value of the
			attribute.  This value *can* be a static value, or can
			reference other attributes of this object as well as 
			other items in the ObjectSystem, in any manner that is
			permissible in a standard Centrallix expression.

	MaxValue	An expression which specifies the maximum value of an
			attribute (see above).

	Default		An expression which specifies the default value of an
			attribute if the attribute has not already been set by
			the user.  (see above).

	Constraint	An expression specifying a condition that the attribute
			value must meet in order to be valid.  If only minima
			and maxima are needed, use those fields instead of this
			one.  Unlike the min/max, this expression is a boolean
			one - its answer should be TRUE or FALSE (1 or 0).

	EnumList	A list of string values which specify the possible 
			values for this attribute.  If the attribute is not a
			string, these string values will be converted to the
			appropriate type as needed behind-the-scenes.

	EnumQuery	A SQL query which will be used to obtain the required
			value list for this attribute.  Must return two columns
			the first of which is the data field and the second is
			the value to be presented to the user.  For bitmask
			type enumerations (multi-select), the data items must
			be numbered 0 through 31.

	Format		The format (mask) used in presentation of the data to
			the user.  Special formats apply for datetime and money
			values.  Undefined for other data types at present.

			Format specifications are contained in the Structure
			File document in the "centrallix-doc" package.

	AllowChars	A list of characters to be allowed in the string
			value.  If NULL, all characters are allowed, subject
			to the BadChars setting (if any), see below.

	BadChars	A list of characters that are NOT allowed in the
			string value.  If NULL, all characters permitted by
			AllowChars are allowed.  If both Allow and Bad are
			NULL, then everything is permitted in the string.
			These two settings can be used to restrict a string's
			content both for data validation and security
			purposes.

	Length		The length (in characters) of data that can be entered
			in this field.

	VisualLength	The length (in characters) of the field, as shown to
			the user.  Usually is the visual field width.

	VisualLength2	A secondary length indication, used mainly by textarea
			widgets to determine height (rows) of the widget.

	BitmaskRO	If the field is a bitmask, this determines which bits
			are readonly.

	Style		A bitmask of possible style indications.  Styles are
			as follows:

			1	Bitmask.  Enumeration is multi-select.
			2	Use a list-style presentation to the user.
			4	Use radiobuttons / checkboxes to present.
			8	Field allows NULL values.
			16	An empty string indicates a NULL value.
			32	Field is grouped - check GroupID.
			64	Field is readonly - user cannot modify it.
			128	Hide this field from the user.
			256	Field is "password" - hide string with
				asterisks as the user types it in.
			512	String value allows multiline editing
			1024	Highlight this attribute to draw special
				attention to it.
			2048	Attribute is lowercase-only
			4096	Attribute is uppercase-only
			8192	Prefer tab pages for groups over group boxes.
			16384	Prefer a separate window for this group.  Note
				that this is a BAD IDEA for required fields!

	GroupID		A numeric group identification for the attribute, used
			to group attributes together on screen.

	GroupName	A name for the group this attribute is in.  Can be
			presented to the user as a named groupbox or tab page.

	OrderID		The order the attribute should be visited by the user,
			usually indicating a tabbing order.

	FriendlyName	A descriptive name for this attribute.

    
    One special note about the expression fields (min, max, default, and
    constraint).  These are implemented dynamically in "normal" circumstances,
    which means that if an expression references another field, and that field
    value changes, then the current attribute's value will change.  One must
    take care, then, to avoid unsolvable 'looping' expressions which trigger
    off of one another.  The rules for this are below:

	1.  If the user has not entered a given field, then a change to a
	    different field which is referenced by the Default expression will
	    cause the given field's value to be updated.  If the user has
	    already entered data for that field, then the value will not
	    change.

	2.  If the user has entered a value for a given field, then a change to
	    a different field which is referenced by the Constraint, Max, or
	    Min expressions will cause the given field to be updated *IF* the
	    current value in the given field no longer satisfies the constraint
	    (or min or max) expression.  This is done via the following:

		a.  First, the Min is checked.  If it does not match, then the
		    field is updated to the new minimum value.

		b.  Second, the Max is checked.  If it does not match, then the
		    field is updated to the new maximum value.

		c.  Lastly, the Constraint is checked.  If it does not match,
		    then the Constraint is reverse-evaluated to yield a value.
		    If the constraint's reverse eval was indeterminate, then
		    the user is notified and is forced to edit that field.

	3.  If a given field is marked read only or hidden, and it has a 
	    constraint, min, or max that are no longer satisfied as a result
	    of a modification to another field, then the modification to the
	    other field will not succeed and the user will be notified of
	    the problem.

	4.  If the user enters data in a field, and its min/max/constraint
	    do not match the data, then the user is simply notified of the
	    problem and invited (required <grin>) to correct it.

    The idea behind these expression fields is to implement functionality in a
    rule-based approach to lessen an application's dependency on procedural
    client-side (user-interface) triggers.


HOW PRESENTATION HINTS WORK....

    Some presentation hints are not "hints" at all, but rather requirements on
    data to preserve data integrity and make data entry simpler for the user.
    The following is a description of how these presentation hints are handled
    internally and how they are transferred to the user interface for operation
    there.

    A.	Sources of Presentation Hints.

	Presentation hints information can come from several different sources,
	operating at different levels within Centrallix.

	    1.	A back-end server.  Hints may be derived from extended data
		information that a back-end server supplies, such as a RDBMS.
		This also includes information stored in local object content
		accessed via the objectsystem.

	    2.	Knowledge within an ObjectSystem Driver.  Some drivers may know
		things about the data that the user-interface should know.  For
		instance, an email driver will know that an address can only
		contain certain characters, and a filesystem driver will know
		that a filename can't contain a '/'.

	    3.	Knowledge within the OSML.  The OSML may know things about a
		data attribute that may be placed in the presentation hints
		information sent to the user interface.

	    4.	Application-specified.  The app itself may place further 
		information or constraints on the presentation hints that is
		not available at lower levels.

    B.	The Special "Presentation Data Overlay" objectsystem driver.

	A special objectsystem driver will be developed which will "overlay"
	presentation hints on top of a subtree which normally wouldn't be able
	to provide such hints by its very nature.  This driver will allow the
	specification of a full suite of presentation hints, and the user-
	interface will access data through that "overlay" just like it were the
	original location for the data in the objectsystem.

    C.	Triggers within the OSML.

	The OSML may have triggers installed to activate on the modification of
	certain values.  The OSML may be able to derive presentation hints from
	these triggers and send that information off to the user.

    D.	"Real" presentation hints sent to the user interface.

	These types of presentation hints, which are *not* specified by the
	application but inherent to the lower level data, are sent to the user
	interface as a part of a query result set.  In this way, they must be
	dynamically installed in the UI by the UI logic when the result set is
	returned from the server.  The server will know what format the UI will
	need the hints to be in because the UI will be able to specify this in
	its query request to the server.

	By their nature, 'real' presentation hints, when used in an app that is
	prebuilt, have a limited effect on the app at run time.  This is 
	because the app will have already been built.  Aspects of hints that
	will not influence the app in this way are:

	    - visual lengths,
	    - friendly name when used as a label,
	    - field grouping and group names,
	    - usually, a field highlight style indication,
	    - usually, a multiline edit style indication,
	    - usually, the tab order,
	    - usually, a hidden field style indication,
	    - the list style vs. button style for enum types.

    E.	Application-specified presentation hints sent to the UI.

	These hints are sent to the user interface as a part of the generation
	of the UI document when requested by the server.  They are "static"
	for a given form data entry widget.

    F.	Execution of the hints.  The various hints are applied at different
	points in the operation of the user interface.

	    1.	No matter what, the constraint, default, min, and max are 
		always applied at run-time when the user is actually using the
		application.  The OSML may also apply them as the data is
		passing back through the server in updates and inserts.  For
		a description of how these operate, see the earlier discussion
		in this document.

	    2.	Visual lengths, list vs. buttons, multiline vs. single line,
		hidden vs. visual, and so forth, are typically only applied
		when the user interface is being designed, or for dynamic
		maintenance screens, when the dynamic screen is being auto-
		generated.

	    3.	The format influences both how the data is shown to the user
		and how the data entry is interpreted.  This is especially
		true for date/time values whose component order changes from
		locale to locale.  Formats are never applied as constraints.

	    4.	Allow null, readonly, password, and empty-string-is-null are
		all applied dynamically as data is presented to the user and
		interpreted from the user's data entry.

    G.	Modification of Hints.

	Hints are NOT modifiable through the same API by which they are 
	obtained (objPresentationHints/objFreeHints).  Different approaches 
	are used to modify the hints, and these depend on the ultimate source
	of the given hint.

	For app designers, application-specified hints are provided at design
	time, using a standardized format.

	For overlay hints, the same standardized format applies, but the hints
	are provided in the overlay specification object(s).

	For RDBMS or backend supplied hints, sometimes the hints are not
	modifiable by the Centrallix user.  The ObjectSystem driver in question
	may provide for a way to change these, however, through the "columns"
	pseudo-object beneath a tabular data object.

	For ObjectSystem Driver 'knowledge' hints, no modification is typically
	possible - the driver knows best ;)  This also applies to hints
	provided by the OSML itself.
	

DELIVERY OF PRESENTATION HINTS VIA HTTP....

    Next we'll consider what it will take to deliver presentation hints to the
    browser via the current HTTP data-access interface.

    A.	Application-specified hints.

	App-specified hints are tied to particular form fields rather than 
	logically to objects retrieved via a query.  Thus, they are sent to the
	user interface as a part of the DHTML page generation process.

	1.  The page widget will offer a "pg_sethints()" function that can be
	    called at script init time to specify hints on a given form element
	    widget.  It will also offer "pg_getdefault", "pg_checkconstraints",
	    and other utility functions for widgets to use.

	    pg_sethints() will take parameters specifying the various hint
	    values, including javascript versions of the expressions.

	    The ht_render subsystem should provide a convenience function for
	    properly building the pg_sethints() initialization call, and will
	    include the functionality to properly reverse expressions and/or
	    itemize referenced fields in the expression.

	2.  A form element widget, whether inside a form or outside of a form,
	    can call "pg_getdefault()" to retrieve the appropriate default 
	    value for the form element widget, during initialization or at any
	    other needed time.  This call results in the setvalue() callback
	    being activated if needed - the form element need do nothing but
	    call the pg_getdefault() function.

	3.  A form element widget must call "pg_checkconstraints()" whenever it
	    is about to modify its value (other than being modified by data
	    loaded from the form widget / objectsource widget).  The function
	    will return false to indicate that the change is not permitted.  A
	    parameter to pg_checkconstraints will indicate whether the change 
	    is occurring while the widget has focus or not.  If it is while the
	    widget has focus, some constraints are ignored.

	4.  A form element must call pg_checkconstraints() whenever it loses
	    keyboard focus (typically, the losefocushandler() callback).

	5.  A form element's setvalue() call is used by the hints mechanism to
	    update values based on constraints firing.  During these calls, the
	    hints mechanism will indicate that the Modified event should not
	    fire, but Changed should.  Changed should NOT result in a 
	    pg_checkconstraints() occurring - only on user modification (which
	    triggers the Modified event).

    B.	Query-specified hints.

	These types of hints are delivered to the browser along with the query
	result set.  They are delivered with each and every object in the 
	result set, since result sets can be asymmetric (different objects
	having different attributes).  The hints data will be specified as a
	part of the "query" part of a DHTML "Link" object.  Currently the Link
	object is used as follows: Target = object id, Host = attribute name,
	Hash = column data type, and Text = column value.  The format of the
	link in general will be:

	    <A TARGET=oid HREF='http://attrname/?hintsdata#datatype>	\
		column value						\
	    </A>

	The hints data will take on the following format:

	    hintname=hintvalue&hintname=hintvalue&hintname=hintvalue ...

	where 'hintname' is the name of the hint being specified, and the
	'hintvalue' is its setting.  For expressions, the expression will
	be a valid JavaScript expression that should be evaluated in the
	context of a JavaScript object having top-level 'properties' which
	enumerate the various objects that are accessible (usually a list of
	forms) and each top-level property having sub-properties that can be
	read and written to, and when written to, trigger (via watches)
	real-world modifications.

	Reversed forms of the constraint expression (designed to yield a value
	for the field, not do a truth test) are supplied as "constraint_rev"
	(the normal constraint expression is "constraint").  If the reverse
	expression is indeterminate, then the reverse is not supplied.

	As an intermediate step, provisionally the reverse expressions may
	not be supplied at all, in which case the user interface will always
	force the user to re-edit the field when its constraint fails due to
	the edit of another field.

	The language an expression is supplied in may be changed by adding
	the query parameter "ls__explanguage=" to the OSML-over-HTTP query
	request.  Suppression of presentation hints altogether may be requested
	by specifying "ls__hints=no" in the query request.

	Expressions may reference other values.  To make it easy for the page
	to determine when an expression references something, an additional
	hintname will be included for each expression called "hintname_ref",
	i.e., "constraint_ref", which contains a comma-separated list of fields
	which are referenced by the expression.

	1.  When the hints arrive at the client.

	    The form widget should retrieve the hints from the objectsource and
	    apply those hints to the various controls in the form.  These 
	    should ADD TO, not REPLACE existing hints on a form element.  In
	    this manner, the form element may need two sets of hints to avoid
	    having to combine expressions.  Defaults specified by the app will
	    override those specified by the query.  Min, max, and constraints
	    will not override - both query and app expressions will be honored
	    for these.

	2.  Page-level processing of hint additions.

	    The page object, when implementing the hints via the utility 
	    function pg_sethints(), will need to determine and keep a record of
	    what expressions are referencing other form elements.  This should
	    be done as follows:

		a.  The pg_sethints operation will build a correlation list of
		    form element names when it receives each expression, based
		    on the expressionname_ref list received from the server.
		    The correlation list should include both the name of the
		    form element and the name of the objectsource.

		b.  When the page-level utility functions need to check for 
		    references to a (newly) modified widget, it will scan the
		    correlation list.

-------------------------------------------------------------------------------
