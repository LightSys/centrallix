Document:   Web Services Client Design
Date:	    27-Oct-2014
Author:	    Greg Beeley, with research by Tumbler Terrall and Remington Wells
===============================================================================

OVERVIEW...

    Centrallix needs to be able to act as a highly flexible web services client
    in order to interact with a wide variety of contemporary web services.
    This document describes the design for the web services client objectsystem
    driver.


HISTORY...

    Up until this point, the mechanism available to interact with a web service
    was to use a combination of the HTTP driver and the XML or JSON drivers.
    The web service, then, had to be REST-ish, and readonly access was the only
    possibility.  The HTTP driver requested the document, and the XML or JSON
    driver parsed the response and allowed the OSML to access the data therein.

    Modern web services, however, take multiple different forms, and often
    require more than one request in order to accomplish a given task.  For
    instance, a web client may be required to first log in, and then secondly
    to retrieve a collection or to execute a JSON-RPC operation.


SCOPE...

    The following technologies and approaches will need to be encompassed by
    this driver:

	REST/JSON
	REST/XML
	XML-RPC
	JSON-RPC
	WebSockets
	HTTP/HTTPS:  GET, PUT, POST, DELETE, and PATCH
	HTTP Authentication: Basic, oAuth
	Operations requiring multiple requests
	Single auth to remote resource vs per-user auths to remote resource

    The following will possibly also need to be handled someday.  These
    technologies are considered obsolete now by many, but are still used by
    some legacy services and software:

	SOAP
	WSDL


DESIGN...

    Object:  An "object" is a remote web service endpoint or object that can
    be involved in one or more types of operations.  The "object" is viewed by
    the OSML as one path location.  Various operations can then be done on that
    object to manage it and/or its subobjects.  An object referring to a
    RESTful remote URL can have many operations automatically defined for it,
    which could be overridden by the programmer if necessary.  Objects will
    have the following characteristics:

	1.  Any of the characteristics of a Request, below, excepting of course
	    the response data.

	2.  RESTfulness.  If this is enabled for a remote object using URL
	    framing, then the entire set of RESTful operations will be 
	    automatically made available.

	3.  Remote URL entity vs collection.  For a RESTful remote URL, this
	    indicates whether it points to an entity or to a collection.  For
	    an entity, the driver will allow read and update type operations
	    on the object itself, which will translate into GET and (PUT or
	    PATCH) operations on the remote resource.

	4.  Use of PATCH for updates.  This controls whether PUT or PATCH will
	    be used for updating remote resource data.

	5.  Use of PUT vs POST for creates.  For a collection resource, this
	    controls whether the new resource will be POSTED to the URL or
	    whether it will be PUT as a subentity of the URL.

    Operations:  An "operation" will refer to one or more requests in a
    sequence in order to obtain certain data or to accomplish a certain task
    (such as obtaining search results or deleting a document).

    Requests:  This driver will use the concept of a "request" to mean a single
    request placed with a remote web service.  A request (and its corresponding
    response) will have the following characteristics:

	1.  Transport Type

	    The Transport Type will be HTTP, HTTPS, or WebSockets.  These
	    correspond to http:, https:, and ws: in URL's.

	2.  URL

	    The URL is the location of the remote resource, and can be
	    contructed from data from other requests or objects.

	3.  Framing

	    The Framing can be one of the following:

	    URL:  A URL is provided and the request data is encoded as
	    parameters on the URL or as a request body.

	    RPC:  The request data is encoded into an RPC packet, which is
	    then passed either on the URL (GET operations) or in the request
	    body (other operations).

	    OSML:  The request is submitted in OSML-over-HTTP form.

	4.  Data Encoding

	    The Data Encoding can be XML, JSON, or URL Encoding.

	5.  Request Method
	
	    GET, POST, PUT, DELETE, or PATCH.

	6.  Scope

	    The scope of the request will control the availability of the data
	    received in the response from the remote web service:

	    Global - the data is available to all other requests made by the
	    web services client, regardless of what user initiated the request.

	    User - the data is available to all other requests made by the
	    current user.

	    Session - the data is available only within the current OSML
	    session context for a specific user.

	    Operation - the data is available for the current operation only,
	    and will not be remembered for future operations.

	7.  Requirements

	    The request may require that data be available from another
	    request.  For example, some RPC operations involve a separate login
	    step and data request step, or a search operation may require the
	    creation of a "search result set" and then retrieving the results,
	    as two separate requests.

	8.  Request Data
	
	    Request data will be specified as values and/or expressions, which
	    can reference other requests/objects and can reference parameters
	    supplied by the user.

	9.  Response Data

	    Response data will be stored for the duration of the intended scope
	    (see above), and will be available for other requests.

	10. Authentication Method

	    This can be None, Basic, or OAuth.  If a web service requires a
	    manual signin (via a normal Request), then None is used.


CONFIGURATION...

    The web services client will use a node object in structure file format,
    as most other Centrallix drivers do.  The structure file will contain one
    group for each object, operation, and request; these groups will be nested
    at those levels as well.


EXAMPLES...

    A.	Interface with the Kardia Partner REST API on a Centrallix server:

	$Version=2$
	kardia_partner "system/wsclient"
	    {
	    url = "http://localhost:800/apps/kardia/api/partner";

	    ptnrs "wsclient/object"
		{
		// Below url has no http/https/ws, so it inherits the above
		// url as a prefix, with a / separating them:
		url = "Partners";
		restful = yes;
		resource_type = collection;
		framing = url;
		encoding = json;
		scope = global;

		// No operations or requests need to be specified, since that
		// is automatic with an ordinary RESTful web service.

		addrs "wsclient/object"
		    {
		    // Specifying two URL parts causes them to be separated by
		    // a slash and then each encoded properly as a path
		    // element.
		    url = runserver(:partners:partner_id), 'Addresses';
		    }
		}
	    }

