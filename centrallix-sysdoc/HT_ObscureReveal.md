# HTML Generation Subsystem Obscure/Reveal Notification API

Author: Greg Beeley (GRB)

Date: November 20, 2003

Status: Design Specification and API Reference

## 1. Introduction
Many times, an onscreen widget needs to know when it is being revealed (or shown) to the user, and also when it is being obscured (or hidden) from the user.  In addition, the widget may need the opportunity to cancel the reveal or obscure action, for instance when a window is being closed which contains a form with unsaved modifications.

User agents currently do not provide a good way to obtain this information in a 'natural' manner based on layer or "div" DOM object visibility changes.  Though the visibility property can be "watched", it still depends on the visibility of container(s), and so a better method is needed.  Furthermore, some widgets may cause an obscure operation without actually setting a layer's visibility to 'hidden'.

The purpose of this API is to define and describe a mechanism for providing obscure/reveal notification and hooks in a cross-browser manner within the Centrallix framework.

## 2. Mechanism
Much of this mechanism is required to be asynchronous, and thus callback- driven, as a result of the requirements of being able to cancel an obscure or reveal event.

### Events called on Listeners:

#### Reveal
A widget is 'revealing' a content area to the user, making it visible and thus accessible to the user.

#### Obscure
A widget is 'obscuring', or hiding, a content area from the user, making it potentially inaccessible.

#### RevealCheck
A widget desires to reveal a content area, and needs to give objects in that content area a chance to "veto" the reveal operation.

#### ObscureCheck
A widget desire to obscure a content area, and needs to give objects in that content area a chance to "veto" or cancel the obscure operation.

### Events for Triggerers:

#### RevealFailed
The reveal request has failed because some listener vetoed the operation.

#### ObscureFailed
The obscure request has failed because some listener vetoed the operation.

#### RevealOK
The reveal request has succeeded.  The triggerer should proceed with its reveal operation.

#### ObscureOK
The obscure request has been accepted, and the triggerer should proceed with the obscure operation.

### Entities:

#### Listener
A widget which is listening for new Reveal or Obscure events.  This widget will have the opportunity to cancel such events in the context of the RevealCheck or ObscureCheck events.  The listener must be a layer (or DIV).

#### Triggerer
A widget which has the capability of revealing or obscuring content areas.  Internally, a Triggerer is a Listener as well, though the Triggerer itself does not do any Listener processing.  The Triggerer must be a layer (or DIV) which physically contains the content that is being obscured or revealed.

#### Page
The page - top-level widget which has the ability to reveal or obscure the entire application - reveal on application load, and obscure on application termination or closure.  The Page is a Triggerer only without needing to listen for any events.

### Operation:

#### A.  Triggerer or Page registers:
When the triggerer or page registers with the obscure/reveal system, a set of properties is created on that DIV object which control the obscure/reveal operation.  These properties are as follows:

##### __pg_reveal:
An Array() of listeners contained within this triggering layer

##### __pg_reveal_visible:
A boolean value indicating whether or not the current triggerer is obscured (false) or revealed (true).  Initially set to false.

##### __pg_reveal_parent_visible:
A boolean value indicating whether or not the parent triggerer (or Page object) is obscured or revealed.  For a page object, this is always set to true since the page object has no parent.  Otherwise it is set to the __pg_reveal_visible property value for the parent triggerer (as adjusted by the __pg_reveal_parent_visible property of the parent).

##### __pg_reveal_listener_fn:
The function to be called when the parent triggerer is obscured or revealed.  This is the same type of function as the ones in a Listener.  This function will be null for the Page.

##### __pg_reveal_triggerer_fn:
This is normally set to the Reveal() function on a triggerer, and is called with triggerer events.  Note that for a listener, the Reveal() function is pointed to by the listener_fn, but for a triggerer, Reveal() is pointed to by the triggerer_fn.  They have different purposes, but Reveal() is always the name of the event callback in either case.

##### __pg_reveal_busy:
Indicates whether an operation is already in progress; if it is, the triggerer will not be able to invoke events.  This prevents an aggressive triggerer from 'getting ahead' of the notification mechanism.

Once these properties have been created, if the triggerer is not the Page, then a listener setup is also done so that the obscure/reveal mechanism can receive obscure/reveal events from the parent triggerer on behalf of the current triggerer.  The registration function returns the visibility status of the parent triggerer, which is then used to initialize the triggerer's __pg_reveal_parent_visible property (see above).

#### B.  Listener registers:
When a listener registers, first the following property is created in the listener:

__pg_reveal_listener_fn

which is the same as listed above for a Triggerer.  This function, if not already set up, will be set to the Reveal() method on the listener.  For a Triggerer, the function will have already been set up for internal reveal/obscure system operation.

Next, a search is done for the triggerer containing this listener, by following the parentLayer or parentNode chain until an object with a __pg_reveal array is found.  When it is, the listener is added to the triggerer's __pg_reveal array. 

The registration routine then returns the current value of the triggerer's __pg_reveal_visible property.

#### C.  Triggerer sends Obscure/Reveal/ObscureCheck/RevealCheck event.

##### For Check events:
If the new status matches the current triggerer's value of __pg_reveal_visible, then nothing is done and the call is ignored entirely.

If the parent triggerer is not visible, then only the __pg_reveal_visible value is updated and no other operation takes place.

Next, the RevealCheck or ObscureCheck sequences of events commences (see below).

##### For non-Check events:
The __pg_reveal_visible value is updated, and Reveal or Obscure events are sent to all listeners connected with this triggerer.

#### D.  Triggerer is itself receives Reveal/Obscure/RevealCheck/etc event
When the triggerer is obscured or revealed because the parent triggerer changed its obscure/reveal status and called its listeners' __pg_reveal_listener_fn() functions, the following events commence.

First, the value of __pg_reveal_parent_visible is checked, and if the reveal/obscure event is the same as the current status, nothing is done.

Next, the value of __pg_reveal_visible is checked, and if it is different than the requested status (i.e., the event is Reveal and visible is false, etc.), then the event is sent to all listeners registered with the current triggerer.  For sending of Check type events, see below.

#### E.  RevealCheck or ObscureCheck event needs to be sent to listeners on a triggerer.
The sending of the Check events is serialized.  To do this, the Check event is sent only to the first listener on the list.  That listener then has the responsibility of telling the reveal/obscure mechanism, via a callback, whether to allow the Reveal or Obscure event to proceed; the callbacks are:

pg_reveal_check_ok(e)

pg_reveal_check_veto(e)

Called with e = event object passed to Reveal().  The properties of 'e' will be:

- event name (reveal, obscure, revealcheck, etc.)
- triggerer
- parent triggerer's 'e' object if this triggerer
    did not originate the event.
- any other context info needed by the triggerer(s)

The callbacks can be called in the context of the current script, or at a later point, but one of the two MUST be called in response to the Check event.

#### F.  Check OK callback occurs from a listener
The triggerer then looks up the next listener in its list, and if there is another one, it sends the Check event to the next listener in the list.  It can keep a counter of how many listeners have been notified in the 'e' structure (see above).

If all listeners had been notified, then the Check event has passed all listeners.  If this triggerer caused the original event, then it should then respond with a RevealOK or ObscureOK event to the triggerer.

If the triggerer was responding to a Check event from the parent triggerer, then it should do an OK callback on the parent 'e' value (which should be kept in the triggerer's own 'e' structure).

#### G.  Check VETO callback occurs from a listener
The triggerer then stops looking up more listeners from its list. The check has failed.

If the triggerer initiated the event, its own Reveal function should be called with the event RevealFailed or ObscureFailed, as needed, event to let the triggerer know that its requested operation has failed.

If the triggerer did not initiate the event but was responding to a Check event from its parent triggerer, it should do a VETO callback on the parent 'e' value (see above).

## 3. API
This section describes the API for the obscure/reveal subsystem (I'll call it "ORS" from here forward just for brevity).

### ORS <-> Listener API

#### pg_reveal_register_listener(l)
This call is made when the Listener registers itself to receive Obscure/Reveal/Check events from the ORS.  'l' is the main object of the listener, and must have the following interface:

Reveal() - a function which is called when an event occurs.  It
will be called with an ORS event object 'e' which will
contain the following properties that will be of interest
to the Listener:

- eventName:
  - 'Reveal'
  - 'Obscure'
  - 'RevealCheck'
  - 'ObscureCheck'

#### pg_reveal_check_ok(e)
This call is a callback from the Listener back to the ORS indicating that the RevealCheck or ObscureCheck event has passed.  The 'e' parameter must be that which was passed to Reveal().  This should never be called in response to a plain 'Reveal' or 'Obscure' event!

#### pg_reveal_check_veto(e)
The complement to the above callback routine, to be called when RevealCheck or ObscureCheck must be cancelled (reasons might be unsaved data in a component that is about to be obscured, or unavailable/irrelevant data in a component that was going to be revealed).

Note that on a RevealCheck or ObscureCheck event, the Listener MUST ALWAYS call either of pg_reveal_check_ok or pg_reveal_check_veto! There are NO exceptions to this.  However, the callbacks don't have to be called in the same function call context as the Reveal() method was invoked - a dialog, for instance, can request that the user specify what must be done with unsaved data.

Until the Listener calls the OK or VETO calls listed above, the ORS will 'lock' the triggerer so that it cannot generate any more events.

### ORS <-> Triggerer API

#### pg_reveal_register_triggerer(l)
registers an object onscreen to generate events.  The triggerer must register *before* its child objects have a chance to register as listeners, or else operation is undefined.  The 'l' parameter is the main layer object of the triggerer, and should have a 'Reveal()' callback for delivery of triggerer events:

Reveal() - a function which is called when a triggerer event occurs.  Such events are the result of a failed or successful reveal or obscure operation.  It will be called with a context value 'e' which has the following properties:

- eventName:
  - 'RevealOK'
  - 'RevealFailed'
  - 'ObscureOK'
  - 'ObscureFailed'
- c: the context value passed to pg_reveal_event().

#### pg_reveal_event(l,c,e_name)
this is the routine a triggerer uses to initiate an event.  'l' is the triggerer's main object, and 'e_name' is 'Reveal', 'Obscure', 'RevealCheck', or 'ObscureCheck'. 'c' is the context argument; it may be anything the triggerer desires, and will be passed back to the triggerer's Reveal() event callback when the Obscure/Reveal Failed/OK events occur.

Note that OK/Failed callbacks are *only* sent for 'Check' type events.  The standard 'Reveal' and 'Obscure' types automatically succeed.

If the triggerer for some reason is not permitted to initiate an event at this point in time, pg_reveal_event() returns false, otherwise true.  The triggerer should respond by refusing to continue its reveal/obscure operation that was the reason for the event.

The triggerer will receive an OK or Failed event at a later point to inform it whether or not its requested event succeeded or not. On an OK event, it should continue with its reveal or obscure operation (such as opening/closing a window or tab page).  Thus the triggerer should embed enough information in 'c' to allow it to continue its action when 'c' is passed back to RevealOK or ObscureOK.

## 4. Nonvisual and Related Widgets: Form, Table, OSRC...
The form and objectsource in particular present a challenge to this obscure/reveal mechanism because they are nonvisual.  The table is included in this discussion since it is integrally a part of the form/osrc setup.

The objectsource in particular is an issue because it is nonvisual and thus cannot naturally be obscured or revealed.  Thus, for objectsource and form, the obscure/reveal mechanism requires some additional work.  In this case the visual presentation components for this data (form elements and the table widget) are used to 'pass on' that information to the osrc.

The osrc currently will use the information to handle auto-querying on 'first reveal' or on 'each reveal'.  The form will use the information to pop a squawk dialog should unsaved information be threatened with obscuring.

Form elements should pass on the obscure/reveal data to the form via the form's Reveal() method which conforms to the Reveal() method for a listener.  If the form element does not have a form associated with it, then it should handle such events itself (including doing veto/ok operations on check events).

The form and table should call the Reveal method on the osrc when any part of the form or table first is revealed (made visible).  They should call Obscure() on the osrc when the last part of the form or table is obscured from the user's reach.

## 5. Future Directions
This is an initial implementation of the obscure/reveal subsystem.  The
following future directions are foreseen:

### 1.  Triggerer and Listener functionality in one object
Sometimes an object may need to act as a Listener and Triggerer simultaneously, preventing the builtin pg_reveal_internal() routine from passing the events down the chain.  Some provision needs to be made for this.

### 2.  Generic messaging model
Perhaps even better would be a generic messaging model by which these kinds of events could be sent up or down the widget hierarchy, generically instead of specific to the obscure/reveal mechanism.  Different systems use different terminology for this type of mechanism such as 'messaging' or 'broadcasting'.

### 3.  Stronger parent/child widget association
Currently, widget parent/child associations in the generated DHTML page are indirect: the subwidgets are 'generally' rendered within a layer that belongs to the parent widget, but may not be the 'main layer' of the parent widget.  We may need to develop a stronger parent/child widget tree in the generated DHTML to help aid the functioning of a generic messaging model or even of the obscure/reveal system itself.
