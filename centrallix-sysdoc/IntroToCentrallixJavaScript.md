# Intro to Centrallix JavaScript

Author: mcancel

Date: August 2002

## Table of Contents
- [Intro to Centrallix JavaScript](#intro-to-centrallix-javascript)
  - [Table of Contents](#table-of-contents)
  - [I Introduction](#i-introduction)
  - [II Abbreviations](#ii-abbreviations)
    - [A. One letter abbreviations](#a-one-letter-abbreviations)
    - [B. Two letter abbreviations](#b-two-letter-abbreviations)
    - [C. Three or more letter abbreviations (okay some of these are obvious)](#c-three-or-more-letter-abbreviations-okay-some-of-these-are-obvious)
    - [D. Widget abbreviations](#d-widget-abbreviations)
  - [III Common Layer Attributes](#iii-common-layer-attributes)
  - [IV Events](#iv-events)
    - [A. List of Events](#a-list-of-events)
  - [V Centrallix Tools](#v-centrallix-tools)
    - [A. xstring](#a-xstring)
  - [VI Debugging Tips](#vi-debugging-tips)
    - [A. alert("You message goes here");](#a-alertyou-message-goes-here)
    - [B. javascript:](#b-javascript)
    - [C. TreeView in DOMViewer mode (only for debugging properties and such)](#c-treeview-in-domviewer-mode-only-for-debugging-properties-and-such)

## I Introduction
This document is just a little documentation on basic javascript code & centrallix. This is just to give a newbie some hints and a little bit of direction. I'm still getting use to JavaScript and Centrallix myself... so if you see anything wrong, please correct it. And if you can think of anything that would be usefull or helpful in any way, please add it.

## II Abbreviations
Each module will have different and specific abbreviations to it's own module and needs.  Here is a list of some abbreviations to help get you started. Once you start to dive into the code, you will start to recognize what the letters stand for. One way to learn the abbreviations of a specific module is to look at that module's init function in the js files (sometimes the init function lists out common abbreviations to that module). This list is not all-inclusive, it's just to help get you started and get you into the centrallix widget mind frame.  Also, not all modules use the same abbreviations (sorry there are different developers and different needs working for each module). 

### A. One letter abbreviations
- a - area
- b - bold
- c - channel
- c - character
- c - color
- d - display
- d - document
- d - down
- e - endcode
- e - event
- f - flags
- h - height
- i - index
- k - key
- k - kind
- l - layer (not the number one)
- m - method
- m - mode
- m - moveable
- o - object
- o - object name
- p - often used to hold layers or for a new layer (maybe stands for page)
- p - pane
- p - parameter
- p - parent
- q - query object
- s - server
- s - session
- s - shadowed
- s - sql
- s - start code
- s - string
- t - date
- t - kind
- t - seconds
- t - tab
- t - table
- t - the thumb part of the scroll bar
- t - time
- u - up
- v - value
- w - width
- x - position on the x axis
- y - position on the y axis
- z - z index

### B. Two letter abbreviations
- al - alternate layer
- ar - auto reset
- as - auto start
- bg - background
- bl - bottom layer
- fg - foreground
- fn - fieldname
- fs - fontsize
- hl - hilight
- ll - left layer
- ly - layer
- ml - mainlayer
- nm - name
- pn - pane
- po - parent object
- ra - read ahead
- rl - right layer
- rs - replica size
- sa - scroll ahead
- sc - scrollbar
- ti - target image
- tl - top layer
- ts - tristate
- zi - z index

### C. Three or more letter abbreviations (okay some of these are obvious)
- ary      - array
- bgnd     - background
- btm      - bottom
- cls      - class
- cols     - columns
- con      - content
- img      - image
- itm      - item
- klayer   - keyboard layer
- kbdlayer - keyboard layer
- lft      - left
- lnk      - link
- mlayer   - mouse layer
- pos      - position
- rgt      - right
- src      - source
- str      - string
- tgt      - target
- thum     - the thumb part of the scroll bar
- tmb      - the thumb part of the scroll bar
- tmp      - temp
- txt      - text
- val      - value


### D. Widget abbreviations
- alrt - alerter
- cl - clock
- cn - connector
- dd - drop down
- dt - date time
- eb - edit box
- ex - exec method
- fs - form status
- ht - html
- ib - image button
- lbl - label
- mn - menu
- pg - page
- pn - pane
- rb - radio button
- rc - remote control 
- spnr - spinner
- sp - scroll pane
- tbld - table
- tb - text button
- tc - tab control
- tm - timer
- tv - treeview
- tx - text area
- wn - window

## III Common Layer Attributes
In Netscape's implementation of Javascript, different objects are passed to the event handler in different ways.  The Event object in Javascript contains a property called "target" that is the target object that the current event was executed on.  For instance, if one were to do a MOUSEDOWN event on an image, Event.target (or more commonly referred to as e.target) would contain a reference to the Image object of that specific image. However, if a MOUSEDOWN event is triggered on a layer, e.target will contain a reference to the layer's document property (l.document).

This is not what we want, because most properties and attributes for layers and objects are set on the base of that object.  It would not be good to stick all properties on l.document.  To solve this inconsistancy and to standardize the events a little bit, we have created three attributes that should be on all visible objects.

| Attribute   | Description
| ----------- | ------------
| layer       | A reference to the containing layer of the current object. This is used to get access to all the attributes and the base object that an event occured on.
| mainlayer   | A reference to the base layer for a widget.  This is used so that we can reference a widget as a whole.  It is used in the page area functions to know what layer to draw the black box around.
| kind        | This is a tag used in the event handlers to check if a specific event is targeted at a specific widget.  This is explained in greater detail below.

The two most common objects that you will be attaching these properties to are the Image and the Layer objects.  Below are examples of how to properly assign these attributes for each of these objects.

Layer:
```
    l.document.layer = l;
    l.mainlayer = l;
    l.kind = "cb";
```

Image:
```
    l.document.images[0].layer = l;
    l.document.images[0].mainlayer = l;
    l.document.images[0].kind = "cb";
```

Notice the difference between the two.  In the Layer object, the ".layer" attribute is a child of the ".document" attribute.  However, on the Image, ".layer" is just part of the image.  In both of these cases, we are assuming that "l" is the base layer for the widget and are assigning that to ".mainlayer".

NOTE:  Strictly speaking, on the image example above, the .mainlayer and .kind properties that are set on the Image are not necessary.  Since the .layer property points back to "l", the .mainlayer and .kind properties from "l" will be used under standard event handling. However, we still recommend setting them, as they can be useful in other circumstances other than event handling.

The ".kind" attribute is used in a widget to check whether an event that has occurred belongs to that specific widget.  We have to do this because the event functions in Centrallix put all events of a certain kind together in one function.  For example, when you register some code with the MOUSEDOWN event (using htrAddEventHandler()), that chunk of code gets stuck into the function that handles all the MOUSEDOWN events including the MOUSEDOWN events for all other widgets.  Thus, a distinction must be made on which widget should get the event.  We will normally wrap all the event code inside a conditional that uses the ".kind" attribute.  Here is an example (note: the "ly" variable is explained in the next paragraph):

    if (ly.kind == "cb") 
        {
        ... do checkbox event code ...
        }

All of these parameters come together in the "ly" variable.  Each event handler (MOUSEDOWN, MOUSEOVER, etc) has its own function that gets called whenever that particular event type occurs.  This function is defined at the page level (not for individual widgets).  At the very top of that function is a conditional that checks to see if the ".layer" attribute is set for that object.  If it is, then ly is set to "e.target.layer" If it is not set, then it falls back and sets it to "e.target".  (Remember, e is a reference to the current Event object)

    if (e.target.layer != NULL)
        ly = e.target.layer;
    else
        ly = e.target;

As you can see, ly is then what we will use to reference the layer that an object occurred on.  We standardized on this to remove the need to do checks all over the place.  Now we can just use "ly".  Now that we have ly set, we can reference ly.mainlayer to get a reference to the top layer for the current widget.  We can also reference ly.kind to find out what kind of widget this is.  See how this can be handy?

## IV Events
### A. List of Events
- Mouse Move
- Mouse Out (a.k.a. leave)
- Mouse Over (a.k.a. enter)
- Mouse Down
- Mouse Up
- Key Down
- Key Up


## V Centrallix Tools

### A. xstring

There is a good explanation about the XString module in the OSDriver_Authoring.txt document in the centrallix-sysdoc directory under CVS. This is very helpful for building argument lists in c code to create javascript function calls.

## VI Debugging Tips

### A. alert("You message goes here");
The alert method pops up a dialog box to display the specified message. Remember that if you are using c to create javascript code that you have to escape the quotes:

`    htrAddScript(s,alert(\"Your message goes here\");\n`

Also... if while debugging you have a TON of pop up windows, you can select the topmost one (it should be mostly covered) and select okay. It should get rid almost all, if not all, of the pop up windows - this is better than trying to click okay on ALL of them. 
    

### B. javascript:
In your browser when running javascript dhtml: you may receive a message in the status bar that indicates you have a javascript error. In the url window type "javascript:" and hit enter. You will get another window that lists the javascript errors, the file and the line number.

### C. TreeView in DOMViewer mode (only for debugging properties and such)
