# JavaScript in Centrallix

**Author**: Seth Bird (Thr4wn)

**Date**: August 2008

*With slight modification for clarity by Israel Fuller (Lightning) during June, 2025.*

This file was created by Seth Bird (Thr4wn) to introduce people to nuances of how centrallix handles javascript.

Please edit this file if there are any mistakes.

Also, online documentation can be found at http://www.centrallix.net/docs/docs.php.

See `serverside_html_generation.txt` for more information on how the server side generates javascript.

## Table of Contents
- [JavaScript in Centrallix](#javascript-in-centrallix)
  - [Table of Contents](#table-of-contents)
  - [The Async Hack](#the-async-hack)
  - [Client Interface Model](#client-interface-model)
  - [Widget Initializers](#widget-initializers)

## The Async Hack
Note that centrallix was origionally made for NetscapeNavigator 4 which did not support AJAX using xmlhttprequest. Thus, a hack was devised and much of the system is now based upon it. The hack works as follows: on the client side, a new iframe (or 'layer' if NS4 is the navigator) is generated which sends a request to the server. The server looks at the request, generates requested information, and returns it as normal. Note that everything HTML-related will be a full-fledged HTML page, so the client then has to take the desired parts out of the iframe and insert it into the main page where required.

**Note**: In NS4, a 'layer' element could serve the purposes of both a div and an iframe so the term 'layer' is seen a lot in the code. This documentation uses the terms iframe and layer interchangeably, even though the code will use layers for NS4 navigators and iframes elsewhere.

Much of the async request management is defined in `htdrv_page.js`.

## Client Interface Model
For example, say there is some javscript class C. One might expect the initializer for such a class to look something like the following:

```js
function C_init(params)
    {
    var i, c_instance;
    i = c_instance = new Object();

    i.color = params.color;
    i.text = params.text;
    i.changeText = params.changeText;
    i.changeColor = params.changeColor;
    }
```

However, Centrallix uses an alternative method called the *Client Interface model*, which makes use of the ClientInterface constructor.

Instead of ever directly accesing or using methods or members, Centrallix creates a "Client Interface" instance which contains wrappers to use said methods or members. All methods in class instances can be accessed indirectly via these client interfaces.

The point of a Client Interface essentially seems to be to "bind" the methods to the object instance. "Binding" can be simply done by using [Prototype's 'bind' function](http://prototypejs.org/doc/latest/language/Function/prototype/bind) which binds a provided context to a given function. When the returned function is called, its context (aka. the `this` pointer) will be the provided context. Thus, the Client Interface model essentially "implements" the class methods so that each instance's methods apply only to the instance itself.

However, Centrallix does not actually bind the class methods to the instance. Instead, it creates a hash of functions that the coder "wants" to be bound to said instance. Then a whole slew of wrapper functions have to be called every time so that it's ensured that these "want-to-be-bound" methods actually are being bound. But how do these wrapper functions know which object is the "bound" object of these member functions? via the 'obj' member (read below).

A Client Interface instance will contain the following:

* an 'obj' member which points to the object intended to be the
    context

* a container of all "want-to-be-bounded" methods/members

* a slew of wrapper functions wich interact with methods/members
    inside the container (including wrappers which will first add
    methods/members individually to the container)

Thus, an initializer for some class C (using this model) would look like the following:

```js
function C_init(params)
    {
    var obj;
    ifc_init_widget(obj);

    obj.color = params.color;
    obj.text = params.text;

    var iface = form.ifcProbeAdd(ifAction);
    iface.Add("changeText", c_changeText); //these are globally defined in the same file
    iface.Add("changeColor", c_changeColor); //^^

    return obj;
    }
```

So ultimately, instead of calling a function using the traditional syntax:

```js
c_instance.changeText("foo");
```

You invoke it using this syntax:

```js
c_instance.ifcProbe(ifAction).Invoke("changeText", "foo");
```

This is an example of an instance using the traditional approach:

```js
    c_instance                    
+------------+                   
|   member1 -+--> "value1"      
|   member2 -+--> "value2"      
|   method1 -+--> function(){} 
|   method2 -+--> function(){} 
| formMember-+--> ...
| formMethod-+--> ...
|            |                   
+------------+
```

This is an example of an instance using the Client Interface approach:

```js
  c_instance   <----------------------------------------------------------------<-+-<-----------------+
+------------+                                                                    |                   |
| member1 ---+-> ...                                             +------------+   |                   |
| member2 ---+-> ...                                             |   obj -----+---+                   |
|            |                                                   |            |                       |
|            |                                                   |  Actions --+-> [ method1, method2] |
|            |                                            +----> |  Add ------+--> function(){}       |
|            |     /* a list of interfaces */             |      |  Exists ---+--> function(){}       |
|   _ifc-----+--->  +---------------+                     |      |  Invoke ---+--> function(){}       |
+------------+      |    "ifAction"-+---------------------+      |SchedInvoke-+--> function(){}       |
                    |               |                            +------------+                       |
                    |               |                                                                 |
                    |    "ifValue" -+-->  +------------+                                              |
                    |               |     |   obj -----+----------------------------------------------+-----------------------------------------------------------------------------------------------------------------+
              +-----+"ifFormElement"|     |            |                                              |                                                                                                                 |
              |     |               |     |_Attributes-+---> [ {name:"member1", exists:true, obj: ----+ , instance: -+ , watchlist:[], propname: "member1", get:null , set:null} , {name:"member2", exists:true, obj: --+ , instance: -+ , watchlist:[], propname: "member2", get:null, set:null}]
              |   +-+--- "ifEvent"  |     |            |                                                             |                                                                                                                 |
              |   | +---------------+     |            | <---------------------------------------------------------<-+-<---------------------------------------------------------------------------------------------------------------+
              |   |                       |/* other    |
              |   |                       |  wrapper   |
              |   |                       | functions*/|
              |   |                       +------------+
              |   |          
              |   |          
              |   +------> /* an entire "Client Interface" which implements events (perhaps use YUI?) */
              |
              |
              +---> /* yet another interface which only does members/methods for _form elements_ */
```

## Widget Initializers

All widget initializers are ment to be applied directly to the appropriate DOM element.
