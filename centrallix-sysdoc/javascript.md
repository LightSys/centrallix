
*********************************

This file was created by Seth Bird (Thr4wn) to introduce people to
nuances of how centrallix handles javascript.

Please edit this file if there are any mistakes.

Also, online documentation can be found at http://www.centrallix.net/docs/docs.php

*********************************

 See serverside_html_generation.txt for more information on how the
 server side generates javascript.

= The Async Hack =

Note that centrallix was origionally made for NetscapeNavigator 4
which did not support AJAX using xmlhttprequest. Thus, a hack was
devised upon which a lot of the system is based upon. The hack works
as follows: on the client side, a new iframe (or 'layer' if NS4 is the
navigator) is generated which sends a request to the server. The
server looks at the request, generates requested information, and
returns it as normal. Note that everything HTML-related will be a
full-fledged HTML page, so the client then has to take the desired
parts out of the iframe and insert it into the main page where
required.

     note: that in NS4, a 'layer' element could served the purposes of
           both a div and an iframe. Thus, the term 'layer' is seen a
           lot in the code. So when I use the term iframe here (and
           elsewhere), I also mean a 'layer' for NS4 navigators.

a lot of async request management is defined in htdrv_page.js.


= Client Interface model =

Let's say that there exists some javscript class C. An initializer for
that class could look like the following:

function C_init(params)
    {
    var i, c_instance;
    i = c_instance = new Object();

    i.color = params.color;
    i.text = params.text;
    i.changeText = params.changeText;
    i.changeColor = params.changeColor;
    }

However, Centrallix does not work that way; instead, it uses the
Client Interface model, which makes use of the ClientInterface
constructor.

Instead of ever directly accesing/using methods/members, Centrallix
will instead make an entire "Client Interface" instance which will
contain wrappers to use said methods/members. All methods (in class
instances) are supposed to be accessed indirectly via these client
interfaces instead of used directly.

The point of a Client Interface essentially seems to be to "bind" the
methods to the object instance. "Binding" can be simply done by using
Prototype's 'bind' function (http://prototypejs.org/api/function/bind)
which returns a function whose sole purpose is to call the wrapped
function by passing a specific object as 'this'. Thus, whenever the
bounded method is called, it will always be called via a wrapper that
ensures that the correct 'this' pointer is being passed to the
member. The Client Interface model is essentially trying to
"implement" the class methods so that the instance's methods applies
only to the instance itself.

However, Centrallix does not actually ever bind the class methods to
the instance. Instead, it creates a hash of functions that the coder
"wants" to be bound to said instance. Then a whole slew of wrapper
functions have to be called every time so that it's ensured that these
"want-to-be-bound" methods actually are being bound. But how do these
wrapper functions know which object is the "bounded" object of these
member functions? via the 'obj' member (read below).

A Client Interface instance will contain the following:

    * an 'obj' member which points to the object intended to be the
      boundee

    * a container of all "want-to-be-bounded" methods/members

    * a slew of wrapper functions wich interact with methods/members
      inside the container (including wrappers which will first add
      methods/members individually to the container)

Thus, an initializer for some class C (using this model) would look
like the following:

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

So ultimately, instead of typing:

    c_instance.changeText("foo");

you have to type:

    c_instance.ifcProbe(ifAction).Invoke("changeText", "foo");




This is what the instance would look like using the traditional
approach:

   c_instance                    
+------------+                   
|   member1 -+----> "value1"      
|   member2 -+----> "value2"      
|   method1 -+----> function(){} 
|   method2 -+----> function(){} 
| formMember-+--> ...
| formMethod-+--> ...
|            |                   
+------------+                   























This is what the instance would look like using the Client Interface
approach:

                                                                                                                             
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






== Widget Initializers ==

All widget initializers are ment to be applied directly to the
appropriate DOM element.


