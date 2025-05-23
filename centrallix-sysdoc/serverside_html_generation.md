# Serverside HTML Generation
Author: Seth Bird (Thr4wn)

Date: August 2008

This file was created by Seth Bird (Thr4wn) to introduce people to
nuances of how centrallix handles javascript.

Please edit this file if there are any mistakes.

Also, online documentation can be found at http://www.centrallix.net/docs/docs.php

See javascript.txt for more information on how the javascript side of things work.

## Note about Unique Endings

You'll see the following notation:

       startup_*, build_wgtr_*, expinit_*, *Render, etc

Where * represents some unique set of letters/numbers. (explanation given below)

## Server-side App handling
Once the server Identifies that the requested URI is an app, there are three important steps that happen:

### 1) Conversion of App file to internal machine data
wgtrParseOpenObject traverses the app file by using the OSML and converts the widget information into an internal machine data (WgtrNode struct).

### 2) Auto Positioning
the WgtrNode instance will then end up in the **Apos unit**. (read below)

### 3) HTML generation
The WgtrNode instance gets passed to **htrRender** which generates the HTML. (NOTE: instead of directly returning HTML pages or using PHP, perl, or other language to generate pages, Centrallix generates every web page in C).

### Conversion to Internal Machine Data

### Apos Unit (Auto Positioning)
seriously, can someone please fill in this information :), including the reasoning behind _why_ it's even there (for simplicity for non-techy users)

### HTML generation
Seeing the source code of a returned web page while reading this will be extremely helpful. There's actually an example webpage that the server returns called 'output_for_default_index.app.html'.

#### Logical Division of html page

The returned web page is logically divided into different parts:

* `<script>` includes (added on server-side via htrAddScriptInclude)

* CSS information (added via htrAddStylesheetItem)

* The actual HTML (added via htrAddBodyItem)

* The initial javascript function where client-side control starts (body's onload attribute is startup_* (see "Unique Endings" section)) (added via htrAddScriptInit)

* the JSON-like description of the widget tree being returned and the dynamically generated invocation of the js function wgtrSetupTree which traverses that JSON information and uses it to initialize all the DOM elements so that the DOM elements have the necessary js functions/members. (added via htrBuildClientWgtr)

There are other parts as well, but the important thing here to note is that for every logical part (except the client widget tree (JSON-like description)), there is a corresponding htrAdd* function.

For each page generation, there is a HtPage instance which has members for each logical part.

#### Flow of server-side html generation
First, the server-side code traverses the WgtrNode instance.

For each widget, it runs the corresponding Render function (found in htmlgen/htdrv_*.c where * is the widget shorthand) which individually adds (to the HtPage instance) the needed information for each part for that particular widget. Thus, there are no html pages on the server, but just a bunch of *Render functions which have hardcoded in them what information to add.

Also, because of the async-hack (see javascript.txt), every HTML request will run through the same algorithm which always generates an entire web-page, including global variables, even though that is not necessary.

## Misc information

### Server-side "Classes"
Centrallix has future support for what are called "classes". These "clases" are really just different data formats the client can request data to be sent as. There is a file where different 'classes' can be defined (centrallix/etc/useragent.cfg)

First of all, the functionality for returning widget-tree data in different formats is not even currently (2008-07-11) implemented. Second, if it ever does get implemented, some portions of code will have to be moved (marked in htrRender() as '$$$ MARK: server-side classes $$$').
