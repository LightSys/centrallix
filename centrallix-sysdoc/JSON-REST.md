# RESTful JSON API to access Centrallix OSML data
Author: Greg Beeley (GRB)

Date: 05-Jun-2014

## Overview
The use of JSON for data exchange over HTTP connections (GET, POST, PUT) has become the defacto standard for web services APIs in recent years. This document describes the implementation of a RESTful JSON-based API in Centrallix, for access to data stored in the ObjectSystem.

## Table of Contents
- [RESTful JSON API to access Centrallix OSML data](#restful-json-api-to-access-centrallix-osml-data)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [About REST](#about-rest)
  - [About JSON](#about-json)
  - [OSML JSON Data Format](#osml-json-data-format)
    - [A.	Basic Format:](#abasic-format)
    - [B.	Full Format](#bfull-format)
    - [C.	Encoding entire objects in JSON](#cencoding-entire-objects-in-json)
    - [D.	Encoding a whole collection in JSON](#dencoding-a-whole-collection-in-json)
  - [REST Interface URI Parameters](#rest-interface-uri-parameters)
    - [A.	Identifying the use of the REST interface](#aidentifying-the-use-of-the-rest-interface)
    - [B.	Collections vs Elements](#bcollections-vs-elements)
    - [C.	Content vs. Attributes](#ccontent-vs-attributes)
    - [D.	Selecting Basic vs Full attribute format](#dselecting-basic-vs-full-attribute-format)
    - [E.	Recursively retrieving data](#erecursively-retrieving-data)
  - [Collection Search Criteria](#collection-search-criteria)
  - [Authentication](#authentication)
  - [Session Cookies](#session-cookies)
  - [Enhanced Access Token](#enhanced-access-token)
  - [Keeping Sessions and Tokens Alive](#keeping-sessions-and-tokens-alive)
  - [Response Mime Types](#response-mime-types)
  - [Example Requests](#example-requests)
    - [A.	Retrieve a list of modules installed in Kardia](#aretrieve-a-list-of-modules-installed-in-kardia)
    - [B.	Get the module information data about one module:](#bget-the-module-information-data-about-one-module)
    - [C.	Update a value on the server:](#cupdate-a-value-on-the-server)
    - [D.	Create a new object (when the name of the new object is not yet known):](#dcreate-a-new-object-when-the-name-of-the-new-object-is-not-yet-known)
      - [Request](#request)
      - [Response](#response)

## About REST
REST stands for REpresentational State Transfer.  It is not a protocol specification, but is an approach to implementing protocols over HTTP and HTTPS.  There are a few key concepts to understand:

- A.	Collections.  A Collection contains a set of REST elements.  This maps to a list of Subobjects of an Object within the OSML.

- B.	Elements.  An Element is an object or document.  This maps to an Object within the OSML.

- C.	Stateless Operation.  REST is a stateless architectural element, which means that the concept of "opening" and "closing" resources or queries is nonexistent in REST.

- D.	Methods Used.  REST makes use of GET, POST, PUT, PATCH, and DELETE. These methods map into the standard object access categories below:

    1.	GET = Read information from an element or a collection

    2.	PUT = Replace (i.e., update) or create information if the exact URL of the element is known in advance (i.e., the name or ID of the element is known rather than depending on the server and database backend to create the name).  When updating an element, the entire element is overwritten.

    3.	POST = Create information, by POSTing to a collection URL to create a new entity.  POST is used to create when the ID or name of the new entity is not known by the client in advance (where the server needs to automatically generate the name, such as in autonumber or autoincrement situations).

    4.	DELETE = Delete information, by DELETING the element URL.

    5.	PATCH = Update part of an element, by providing only the information about the new element that is different from the old element.  A PATCH request contains a request body in JSON format, where the JSON object contains one attribute for each attribute that needs to be updated, in basic attribute format (see below for basic vs. full attribute format).  PATCH will respond with the modified object, just like a GET request had been placed.  PATCH does not work on collections.

REST does not require the use of a particular data format for requests or responses.  Data can be in JSON, XML, or any other format of the API designer's choosing.

## About JSON
JSON, or JavaScript Object Notation, is the use of what is essentially a subset of JavaScript to represent an object's attributes and data (there are some subtle differences from normal JavaScript however).  It requires the use of quoted attribute names, and allows certain unescaped characters that would not parse properly in normal JavaScript.

JSON defines six fundamental data types:  Objects, Arrays, Strings, Numbers, Booleans, and the special untyped null value.

Data from the Centrallix OSML will be represented in JSON in a format described below in this document.

An additional recommended standard, called JSON-LD (for "JSON Linked Data") is used to specify the links required by the REST architecture.  JSON-LD properties can be identified by starting with an "@" character.

## OSML JSON Data Format
Data will be represented in one of two possible formats, a basic format, and an full format containing metadata.

### A.	Basic Format:

In the Basic Format, only attribute names and values are supplied. This is appropriate for simple API integrations, and represents a traditional JSON data format.  For an Object, there will be one top- level JSON object container, with a list of attributes inside of it:

    { "first_name":"John", "last_name":"Smith" }

Centrallix has five fundamental data types: Integer, Double, String, Money, and Datetime.  These will be mapped to JSON data as follows:

Integer:

    "age":32

Double:

    "percent":95.5

String:

    "first_name":"John"

Money:

    { "wholepart":100, "fractionpart":0 }

Datetime:

    { "year":2013, "month":1, "day":1, "hour":12, "minute":10,
    "second":10 }

And, finally, for null values:

    "org_name":null

### B.	Full Format

The Full Format will be used to transfer metadata about each attribute in addition to the attribute's name and value:

    { "a":"age", "e":null, "v":32, "t":"integer", "h":"d=0" }

In the above representation, the property names are abbreviated for space savings, and mean the following:

- a = name of attribute
- e = error status, null (or not present) if no error.  If set, it will either be the string "error" or a string containing an error message.
- v = attribute's value (same representation as in the Basic Format)
- t = data type (integer, double, string, money, datetime)
- h = presentation hints, as encoded by hntEncodeHints() and as decoded by cx_parse_hints in JavaScript.  If no hints are available, the h property may be omitted entirely.

The 'a' property above will be omitted if the name has already been specified outside of the full format attribute, for instance if a list of attributes is contained in a JSON Object.

### C.	Encoding entire objects in JSON
The encoding of entire objects in JSON will be done by placing the attributes inside a JSON Object.  Below are examples of this being done with both the basic and full attribute formats:

Basic:

    {
    "@id":"/people/001?cx__mode=rest&cx__res_format=attrs",
    "first_name":"John",
    "last_name":"Smith"
    }

Full:

    {
    "@id":"/people/001?cx__mode=rest&cx__res_format=attrs&cx__res_attrs=full",
    "first_name": { "v":"John", "t":"string", "h":"l=64" }
    "last_name": { "v":"Smith", "t":"string", "h":"l=64" }
    }

### D.	Encoding a whole collection in JSON
A collection will be encoded as a list of objects, but will default to not including any attributes at all.  Here is an example collection listing the modules installed in Kardia:

    {
        "@id":"/apps/kardia/modules?cx__mode=rest&cx__res_type=collection",
        "base": { "@id":"/apps/kardia/modules/base?cx__mode=rest" },
        "crm": { "@id":"/apps/kardia/modules/crm?cx__mode=rest" },
        "disb": { "@id":"/apps/kardia/modules/disb?cx__mode=rest" },
        "gl": { "@id":"/apps/kardia/modules/gl?cx__mode=rest" },
        "payroll": { "@id":"/apps/kardia/modules/payroll?cx__mode=rest" },
        "rcpt": { "@id":"/apps/kardia/modules/rcpt?cx__mode=rest" }
    }

URI parameters will be able to be supplied (see below) to control whether attributes are included in collection lists.

If additional attributes are included in the collection listing, then they will be included at the same level as the @id JSON-LD property, such as in the following snippet:

    "base":
    {
    "@id":"/apps/kardia/modules/base?cx__mode=rest",
    "someattr": "attrvalue"
    }

## REST Interface URI Parameters
### A.	Identifying the use of the REST interface
The Centrallix HTTP/HTTPS interface currently has multiple existing API's that it supports.  In order to additionally support REST, the following URI parameter is required:

    cx__mode=rest

### B.	Collections vs Elements
In REST, either a URI is a collection URI or an element URI.  However, in the OSML, an object can have both attributes and subobjects.  Thus, the OSML pathname alone is not sufficient to determine whether something is a collection or an element.  In order to differentiate the type of resource, the cx__res_type URI parameter is required on all requests for a collection, since it defaults to Element mode.  It can be specified as follows:

    cx__res_type=collection
    cx__res_type=element	(default)
    cx__res_type=both

("res" in the URI parameter above stands for "Resource", not for "REST".)

When requesting "both" collection and element data from a URI, the response will contain two JSON objects at the top level, named cx__element and cx__collection.  Within each of those objects will be the element and collection data, respectively, had the resource type not been set to "both".

### C.	Content vs. Attributes
Since an OSML object can have both attributes and content, and it is normal in REST to return the content in an element request, the following URI parameter can be used to force the server to respond one way or the other:

    cx__res_format=attrs	(default for Collection)
    cx__res_format=auto	
    cx__res_format=content	(default for Element)
    cx__res_format=both

The 'both' option allows for any content to be encoded into the 'cx__objcontent' property, thus providing a complete list of attributes as well as the content of the object.  Note that cx__objcontent is included in the JSON data as a simple (basic) property; it does not use the 'full' attribute format.

The 'auto' option causes the system to return content if the object can have content, or to return an attributes document if the object cannot have content.  If the object can have content but does not, an empty response will be received (zero length).

If retrieving a collection, then 'content' is disallowed and 'auto' works like 'attrs'.  The 'both' option can be used when retrieving a collection, however.

### D.	Selecting Basic vs Full attribute format
To select the Basic or Full attribute format, include the following URI parameter.  This URI parameter can also be used to control whether attributes are shown in a Collection document.

    cx__res_attrs=basic		(default for Element)
    cx__res_attrs=full	
    cx__res_attrs=none		(omit attrs: default for Collection)

In a Collection document, the default is "none", which means to only include the names of the elements in the collection, but not any added information about them.  If the entire set of attributes is desired, then cx__attr_format can be used on a collection request to include them.

If attributes are not being returned, then cx__res_attrs is ignored.

### E.	Recursively retrieving data
Sometimes it is useful to obtain a multi-level JSON document in order to reduce the number of requests placed with the server.  Using the "cx__res_levels" attribute, you can request multiple levels of data. This can only be done when cx__res_type is set to either "collection" or "both".  The number of levels defaults to 1.

## Collection Search Criteria
When requesting a collection resource, you can supply one or more criteria to restrict the results in the collection.  To do so, simply include the criteria name as a URI parameter.  For example, to search only for items with a 'status' property of 'Active', include the following URI param:

    status=Active

You can also use other comparison operators, including not equal, greater, less, and substring matching.  To do so, include the operator followed by a colon (:) before the parameter value.  Here is a list of supported operators:

    =	Exactly equal to
    !=	Not equal to
    >	Greater than
    >=	Greater than or equal to
    <	Less than
    <=	Less than or equal to
    *	Contains substring
    _*	Begins with substring
    *_	Ends with substring

For example, to search for items whose 'haystack' property contains the string 'needle', use the following URI parameter:

    haystack=*:needle

If your parameter value might begin with one of the above operators, and you don't want it interpreted that way, it is always "safe" to put =: at the beginning of the value.  For example:

    status=%3D:Active

(in this case %3D is the URI code for the equals sign, which needs to be escaped so it is not interpreted as a URI parameter delimiter.)

## Authentication
Although future authentication mechanisms may be supported later, the authentication mechanism currently used is HTTP Basic Auth.  This involves setting a WWW-Authenticate header with the username and password appended separated by a colon and then encoded with Base 64 encoding. Most HTTP clients should allow setting the username and password for HTTP Basic auth without having to manually construct the header, but if the header must be manually constructed, see the HTTP RFC for details.

## Session Cookies
The Centrallix server will issue a session cookie via the Set-Cookie: HTTP header.  This cookie, whenever present, should be retrieved by the web services client and sent with all successive requests.  It is possible for the server to send a replacement cookie, so whenever Set-Cookie: is present, the web services client must save that cookie and send it on all subsequent requests (until a new cookie is sent by the server or until the web services end-user, if any, is finished with the current data viewing session).  Consult your web client's documentation for how to save and send these cookies; your web client may do this automatically.

## Enhanced Access Token
Simple authentication can be used for reading data, but not for creating, updating, or deleting data.  For those operations (including POST, PUT, PATCH, and DELETE), an additional token must be first retrieved from the server.

This token can be retrieved by placing a GET request to the below URL:

    http://servername:port/?cx__mode=appinit&cx__groupname=GRP&cx__appname=APP

You should replace GRP and APP with strings of your choice which will become the name of your application group and application, respectively, for the active session.  cx__groupname and cx__appname may be omitted entirely, but at least cx__appname should ideally be set to a short but descriptive string regarding what kind of client is connecting to the server.

The response from the server will be a JSON document containing the following attributes:

| Attribute        | Description
| ---------------- | ------------
| akey             | The enhanced access token
| watchdogtimer    | An integer value expressing how long (in seconds) the session and token will remain valid in the absence of any network requests
| servertime       | The current time on the server, including the server's timezone.
| servertime_notz  | The current time on the server without the timezone included.

The token MUST be supplied on any requests that involve modification of data, via the cx__akey URL parameter.

The client MUST NOT disclose the token to any server except the server from which the key originated.  The token serves an important security purpose of guarding against "Cross Site Request Forgery" attacks.

The server's time and timezone can be used by the client to determine the timezone offset between the client and server as well as the amount of clock skew between the client and the server.  These times are supplied in a format compatible with Javascript's Date() function.

## Keeping Sessions and Tokens Alive
The session cookie and enhanced access token (or akey) will expire after a short (server configured) time of unuse.  After that expiry time, a new session will be created on a subsequent HTTP request, and the old token will no longer be valid.

To prevent the session and token from expiring, the client should send "ping" or "heartbeat" messages to the server at an interval of half the session watchdog timer interval (retrieved when obtaining the akey token). The ping message should be as follows:

    http://servername:port/INTERNAL/ping?cx__akey=akeygoeshere

For example, if the value of "watchdogtimer" retrieved from the server is 180 (a typical default), then the client should ping the server every 90 seconds to ensure the session is kept alive.

The server will reply with an HTML document containing the server's current time if the session and token are still valid.  The time is supplied in a format compatible with Javascript's Date() funtion.

## Response Mime Types
If a response contains content (for an element), then the content type of the response will be the content type of the object's content, such as a PNG image or an HTML file.

If a response contains attributes or a list of objects (collection), then the response content type will be "application/json".

## Example Requests

### A.	Retrieve a list of modules installed in Kardia

- Method: GET
- URI: `http://server/apps/kardia/modules?cx__mode=rest&cx__res_type=collection`

### B.	Get the module information data about one module:

- Method: GET
- URI: `http://server/apps/kardia/modules/crm/kardia_modinfo.struct?cx__mode=rest&cx__res_format=attrs`

### C.	Update a value on the server:

- Method: PATCH
- URI: `http://server/apps/kardia/api/thingy/objectname?cx__mode=rest&cx__res_format=attrs`
- Content-Type: `application/json`
- Request Body: `{"attrname":"newattributestringvalue","attrname2":1000}`

### D.	Create a new object (when the name of the new object is not yet known):
#### Request
- Method: POST
- URI: `http://server/apps/kardia/api/thingy?cx__mode=rest&cx__res_format=attrs&cx__res_attrs=basic&cx__res_type=collection`
- Content-Type: `application/json`
- Request Body: `{"attr1":"value1", "attr2":1000, "attr3":"value3"}`

#### Response
- Response: 201 Created
- Location: `http://server/apps/kardia/api/thingy/newobjectname?cx__mode=rest&cx__res_format=attrs&cx__res_attrs=basic`
- Content-Type: `application/json`
- Response Body: `{"attr1":"value1", "attr2":1000, "attr3":"value3"}`
