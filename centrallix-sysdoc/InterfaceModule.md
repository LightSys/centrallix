************************************************
*       Interface Module Documentation         *
*             Matt McGill, August 2004         *
************************************************

INTRODUCTION

    Please read the End-User documentation on Interfaces in centrallix-doc before
    continuing to read. This document assumes that you have done so, and are
    familiar with what an interface is, and how it is defined. 

OVERVIEW

    The IFC module includes a main sub-module on the server side, and
    additional sub-modules for each client-side deployment method that provide
    functionality that is roughly equivalent to that found on the server side.

    The server-side IFC sub-module is responsible for parsing interface
    definitions, for providing a means of accessing interface definitions on
    the server, and for conveying interface definitions to its client-side
    counterparts, either during application rendering time or dynamically.

    The client-side IFC sub-modules are meant to be roughly equivalent to
    each other in terms of the functionality they provide. They are
    responsible for storing the interface definitions provided by the
    server-side sub-module, and for providing a means of accessing those
    interface definitions on the client side.


SERVER-SIDE IFC SUB-MODULE
**************************

    EXTERNAL INTERFACE
	
	int ifcContains(IfcHandle h, int category, char* member)
	    * h:	    handle to the interface being polled
	    * category:	    IFC_CAT_XXX
	    * member:	    string representing the namem of the member being
			    checked

	    This function is meant to be used to check the validity of a
	    member against an interface. Returns 1 if the member is a valid
	    member of the given category, 0 if it isn't, and -1 on error.

	pObject ifcGetProperties(IfcHandle h, int category, char* member)
	    * h:	    handle to the interface being polled
	    * category:	    IFC_CAT_XXX
	    * member:	    string containing the name of the member being
			    checked.

	    This function provides a way to retrieve the properties of a
	    member of an interface definition as an open OSML object. Returns
	    an open OSML object on success, NULL on failure.

	int ifcIsSubset(IfcHandle h1, IfcHandle h2)
	    * h1:	    an interface handle
	    * h2:	    another interface handle

	    This function encapsulates some of the versioning rules of the
	    interface module. Based on the definition paths, and the major and
	    minor version numbers, this function determines if h2 describes a
	    subset of the functionality described by h1. In otherwords, this
	    function returns true if h1 and h2 reference the same interface
	    definition and have the same major version number, and if h2's
	    minor version is less than or equal to h1's version number.

	IfcHandle ifcGetHandle(pObjSession s, char* path)
	    * s:    	    the object session to use to open the interface
			    definition
	    * path:	    an OSML path to the interface definition,
			    including major and minor version numbers

	    This function returns a handle to the specified version of an
	    interface definition, or returns NULL on failure.

	void ifcReleaseHandle(IfcHandle handle)
	    * handle:	    the handle to be released

	    This function releases an interface handle, freeing the resources
	    associated with it if it is the last open handle to that
	    particular version of its interface definition.

	int ifcInitialize()
	    
	    This function initializes the interface module. This is one of the
	    places where additions must be made when adding a new interface
	    type - the names of interface types and their categories are
	    assigned here. This function also sets the base interface path,
	    attempting to pull it out of centrallix.conf, and setting it to a
	    default value if it cannot.

	int ifcToHtml(pFile file, pObjSession s, char* def_str)
	    * file:	    the file to output the recieve the htmlified 
			    interface definition to
	    * s:	    the object session to use to find the referenced
			    interface definition
	    * def_str:	    the path to the interface definition to convert

	    This function htmlifies an interface definition, generating HTML
	    output that is designed to be easily parsible by the DHTML IFC
	    sub-module. When a client DHTML app requests an interface
	    definition from the server dynamically, the net driver calls this
	    function to generate the requested output. This output is parsed
	    on the client-side into a javascript representation of the
	    interface definition.
	
	int ifcHtmlInit(pHtSession s, pWgtrNode tree)
	    * s:	    the current rendering session
	    * tree:	    the widget tree to use to determine which
			    interface definitions to include on the
			    client-side.

	    This function is meant to provide the client-side IFC sub-module
	    with all the interface definitions we think it might need. The
	    widget tree is walked, and any interface definition implemented by
	    a widget is included on the client-side. The goal is to make sure
	    that 99.5% of the interface definitions that will be required on
	    the client-side will be there when they're needed, to avoid having
	    to load an interface definition dynamically.
    
    OTHER FUNCTIONS 

	int ifc_internal_HtmlInit_r(pHtSession s, pXString func, 
				    pWgtrNode tree, pXArray AlreadyProcessed)
	    * s:		    Current rendering context
	    * func:		    The function being generated
	    * tree:		    The node in the tree being processed
	    * AlreadyProcessed:	    A list of interface definitions that have
				    already been included

	    This function recursively walks a widget tree, generating inline
	    object definitions for each interface definition referenced by a
	    widget in the tree. It's called by ifcHtmlInit().
	
	int ifc_internal_ObjToJS(pXString str, pObject obj)
	    * str:	    will contain the generated javascript code
	    * obj:	    the OSML object to javascriptify

	    This function takes an open OSML object, and recursively generates
	    a javascript inline object declaration, effectively building a
	    javascript representation of the OSML object and its children, and
	    their children, etc. The function is concious of the OSML object's
	    properties and children, and creates a javascript object with the
	    same properties, and sub-objects for each child.
	    NOTE: As currently implemented, it's possible for collisions if an
	    OSML object has a property with the same name as one of its
	    children.
	
	IfcHandle ifc_internal_BuildHandle(pIfcDefinition def, int major,
					    int minor)
	    * def:	    the definition to build the handle out of
	    * major:	    major version
	    * minor:	    minor version

	    This function builds an interface handle out of an interface
	    definition and a major and minor version number. An interface
	    handle refers to a specific version of an interface definition,
	    and only contains the information for that version; hence the
	    reason for this function.
	
	pIfcDefinition ifc_internal_NewIfcDef(pObjSession s, char* path)
	    * s:	    the OSML session to use to open the definition
	    * path:	    the OSML path to the definition

	    This function takes an OSML path, and parses the object it
	    referers to into an interface definition. The interface definition
	    includes the information for all versions of the interface. Users
	    of the module never interact with IfcDefinitions - they only
	    interact with the handles built out of them.
	
	pIfcMajorVersion ifc_internal_NewMajorVersion(pObject def, int type)
	    * def:	    the OSML object representing the major version
	    * type:	    the interface type for this major version

	    This function is arguably the most complicated in the module. It
	    takes an open OSML object and parses it into the IFC module's
	    internal representation of a major version of an interface
	    definition. 

	    After some initial set-up, the function makes two passes through
	    the object. The first path is mostly to check for errors,
	    malformed or duplicated minor version names. It also establishes
	    the highest minor version number present. The second pass sorts
	    the minor versions, makes sure no minor versions were skipped, and
	    actually fills out the IfcMajorVersion datastructure. See below
	    for more info on the datastructure. 
	    
	    This second pass does some verification of the contents of the 
	    minor versions, ensuring that they play by the versioning rules
	    and don't break backward-compatibility. One thing is lacking: the
	    ability to modify the properties of a member between minor
	    versions, and still be able to detect if the user is cheating and
	    breaking the versioning rules. Currently, it is illegal to include
	    the same member in the definition of two minor versions.

    INTERNAL DATA STRUCTURES

	* IfcMajorVersion

	    This is not a pretty data structure. On the surface, it looks
	    simple enough:

	    typedef struct
		{
		XArray*     Offsets;
		XArray*     Members;
		XArray*     Properties;
		int         NumMinorVersions;
		} IfcMajorVersion, *pIfcMajorVersion;

	    The most important thing to remember is that Offsets, Members, and
	    Properties are not just pointers to XArrays - they are *arrays* of
	    XArrays, one XArray for each category in the widget interface
	    type. Also keep in mind that newer minor versions inherit from
	    older minor versions.

	    NumMinorVersions is just that - the number of minor versions
	    contained in this major version.

	    Members and Properties are fairly straight-forward. Members is an
	    array of XArrays, where each XArray is a list of the member names
	    across all the minor versions, sorted in descending order by the
	    minor version they belong two. So the first items in
	    Members[IFC_CAT_WIDGET_PROP] are the members in the property
	    category that belong to the highest minor version number.
	    Properties is another array of XArrays, where each XArray is a
	    list of OSML paths to properties, matching up one-to-one with the 
	    list of members. So if the member name is retrieved with
	    xaGetItem(&(maj_v->Members[IFC_CAT_WIDGET_PROP]), 3), the path to
	    the properties object for that member can be obtained with
	    xaGetItem(&(maj_v->Properties[IFC_CAT_WIDGET_PROP]), 3).

	    Offsets is what makes it possible to locate the members that
	    belong to a minor version. Offsets is an array of XArrays, where
	    the XArray for a given category is a list of the offsets into
	    the Members and Properties XArrays for the same category that
	    correspond to the first member of a minor version, where the
	    index into the XArray for the given category of Offsets is the
	    minor version. An example will hopefully help that to make sense.
	    The following diagram attempts to illustrate what's going on:

	    v1 "iface/majorversion"
		{
		v0 "iface/minorversion"
		    {
		    x "iface/property" {}
		    y "iface/property" {}
		    width "iface/property" {}
		    height "iface/property" {}
		    enabled "iface/property" {}
		    Click "iface/event" {}
		    }
		v1 "iface/minorversion"
		    {
		    foo "iface/action" {}
		    bar "iface/action" {}
		    }
		}

	    NumMinorVersions = 2

			    Members XArray for each category
		   PROP            ACTION          EVENT
		  ******          ********        *******
	    0       x               foo		    Click
	    1       y		    bar
	    2     width
	    3     height
	    4    enabled

			    Offsets
		   PROP            ACTION          EVENT
		  ******          ********        *******
	    0       0		     2		    0
	    1	    0		     0		    0

	    Remember that each category has an XArray, and within that XArray
	    the members are sorted descending according to the minor version
	    they're introduced in. To determine what members of the properties
	    category belong to minor version 0, I do
	    xaGetItem(&(Offsets[IFC_CAT_WIDGET_PROP]), 0), and get 0 as my
	    result. This tells me that everything from index 0 to the end of
	    Members[IFC_CAT_WIDGET_PROP] belongs to minor version 0. Checking
	    for minor version 1 yeilds the same result, which is the desired
	    behavior - minor version 1 inherits everything from minor version
	    0.

	    To see what members belong to the action category in minor version
	    0, I check xaGetItem(&(Offsets[IFC_CAT_WIDGET_ACTION]), 0) and get
	    2. This means that the action members for minor version 0 start at
	    index 2 of Members[IFC_CAT_WIDGET_ACTION]. But there are only two
	    items in that XArray - this tells us that there are *no* members
	    of the action category for minor version 0. Checking for minor
	    version 1 yeilds index 0, telling us that both items in the
	    Members[IFC_CAT_WIDGET_ACTION] XArray belong to minor version 1
	    exclusively.

	 * IfcDefinition

	    typedef struct
		{
		char*       Path;
		pObjSession ObjSession;
		pObject     Obj;
		XArray      MajorVersions;
		int         Type;
		}
		IfcDefinition, *pIfcDefinition;

	    This data structure is straight-forward. Path is the OSML path to
	    the interface definition, without the cx__version parameter.
	    ObjSession is the object session the interface definition was
	    opened in. Obj is an open OSML object for the interface
	    definition. MajorVersions is an XArray of pIfcMajorVersion
	    structs, where the index into MajorVersions is the major version
	    number. Finally, Type is the interface type.

	* IfcHandle_t
	    
	    struct IfcHandle_t
		{
		pObjSession         ObjSession;
		char*               DefPath;
		char*               FullPath;
		int                 MinorVersion, MajorVersion;
		int                 LinkCount;
		int                 Type;
		int*                Offsets;
		pXArray             Members, Properties;
		};

	    This is the datastructure the user receives after a call to
	    ifcGetHandle. It contains information for a specific version of an
	    interface definition.

	    ObjSession is the OSML session of the related interface
	    definition. DefPath is the absolute path to the definition without 
	    the cx__version parameter. FullPath is the absolute path to the
	    definition including the cx__version parameter. MinorVersion and
	    MajorVersion are self-explanitory. LinkCount is the number of
	    references to this handle that are currently in use. Type is the
	    type of the interface definition. Members and Properties are
	    pointers to the Members and Properties arrays in the related
	    IfcDefinition struct, and Offsets is an array of the offsets into
	    the Members and Properties XArrays for the minor version of the
	    handle.

	    The user is not meant to touch any of the internals of this data
	    structure - the user should treat an interface handle like a magic
	    black box.

    CACHING INTERFACE DEFINITIONS AND HANDLES

	Since there are a limitted number of interface definitions that will
	most likely be referenced many times, the IFC module only parses each
	interface definition the first time it is referenced - the result is
	saved in a hash table, with the key being the absolute pathname to the 
	definition.

	Handles are also cached; however, unlike the definitions which are
	kept around until Centrallix exits, a handle is only cached while it
	is in use. If six widgets in a tree all request a handle to a version
	of an interface definition, only one data structure is actually
	allocated. When the last widget has released its handle, the data
	structure is destroyed. The handles are stored in a hash table with
	their absolute path, including cx__version parameter, as the key.

CLIENT-SIDE DHTML IFC SUB-MODULE
********************************
    :TODO:
