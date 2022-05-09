Title:    Beeley's Coding Style
Author:   Greg Beeley
Date:     03-Jul-2001
License:  Copyright (C) 2001 LightSys Technology Services.  See LICENSE.txt.
-------------------------------------------------------------------------------

BEELEY'S CODING STYLE...

    For reference, here is some documentation on the core coding style used
    in Centrallix.  Again, don't feel you must use it for your modules or
    files.

    1.  Braces are always indented to align with the text they surround, and
        almost never come on the same line as the control structure which 
	initiated them.  Indents are always four spaces.

    2.  Every function must have a return line at the end, even if the function
        never reaches that return.  In such a case, include a comment with the
	return to indicate that it is never reached.

    3.  The return data type for a function declaration (not header) is placed
        on a separate line from the function name itself.  This makes it easy
	to locate function declarations for certain function names (via a grep
	with the beginning-of-line '^' character).

    4.  Normally, the entire function declaration (excepting the return type)
        is placed on one line.

    5.  Lines need not be manually wrapped at 80 characters.  In today's world
        it is plenty easy to use an xterm with a width much larger than 80
	characters.  You can wrap the lines if you want to, though.  It is 
	helpful, though, to write control structures in such a way that the
	80 character line length is rarely exceeded.

    6.  Local variable declarations are indented the same as the brace that
        begins the function body.  The return statement at the end is done
	the same way.  The code body of the function itself, however, is
	indented four *more* spaces for clarity.  This is the only instance
	(that I know of) where a single block of code requires two indentation
	steps.  There should be a blank line after the local declarations as
	well as before the return statement.

    7.  Single line comments that are not to the right of a block of code
        use double asterisks, as in /** this is a comment **/.  Single line
	comments to the right of code or declarations use single asterisks as
	in /* this is to the right of code */.  Multiline comments for function
	declarations use three asterisks that continue on each line, as in

	    /*** myFunFunction - this is a function
	     *** that does some fun stuff with its 
	     *** parameters.
	     ***/

    8.  The return statement is not treated as a function call.  Type it in as
        "return value;" instead of "return(value);".  Return isn't a function
	and doesn't return.  Well, I guess that depends on how you look at it,
	but I think you get the point :)

    9.  If an "if" statement's "if" or "else" blocks contain more than one
        statement, then BOTH MUST be enclosed in braces for clarity.  Moreover,
	if the "if" statement has other "if"s embedded in it, then all but the
	innermost "if" MUST have its statements enclosed in braces.  If you 
	desire, you MAY enclose ALL if/else blocks in braces.

    10. Here is an example function declaration (it is indented from the left
        margin a bit to look nice in this document file, but in real life
	don't indent from the left margin for the start of the function
	declaration itself).

	/*** myFunFunction - this function does some fun stuff with its
	 *** parameters.  Returns 0 on success, -1 on error.  Makes a 
	 *** strange weighted total of the characters in the two string
	 *** params and places that total in the integer passed by ref.
	 ***/
	int
	myFunFunction(char* param1, char* param2, int* total)
	    {
	    int cnt1, cnt2;
	    int cnt3;
	    char* tmpptr;
	    char* ptr;

	        /** Do some error checking - params must be valid! **/
		if (!param1 || !param2 || !total)
		    {
		    return -1; /* error */
		    }

		/** Ok, here's the work for param #1. **/
		*total = 0;
		while(*(param1++)) (*total)++;

		/** And, for the second parameter. **/
		while(*param2)
		    {
		    if (*param2 == ' ')
		        (*total) += 3;
		    else
		        (*total) += 2;

		    /** This is a really long comment, so I'm going to break it
		     ** up into more than one line.  The following operation 
		     ** takes the second parameter pointer and increments it 
		     ** by one element.  In this case, it means param2 points 
		     ** to the next character in its string.  We need to do 
		     ** this or else the loop will keep going indefinitely 
		     ** unless param2 was empty in which case we'd never get 
		     ** to this location.  I know that wasn't particularly 
		     ** obscure, but I needed a long comment for illustration 
		     ** here :)
		     **/
		    param2++;
		    }

	    return 0; /* success */
	    }

    11. Local variables and function parameters are all in lower-case with
        embedded underscores where needed to separate words.  All global
	variables are stored in a module-wide global structure.  That structure
	name is always in all caps for ease of identification.  Typedef names
	are always in mixed case with both the structure "XxxxYyy" and a 
	pointer to the structure "pXxxxYyy" defined in the typedef declaration.
	Structure and union elements are in mixed case.

    12. Flags.  Flags are always stored in a full-length integer, and never as
        single-bit bitfields (i.e., Flag:1;).  This allows the flags to be 
	passed as parameters easily and to be bulk-edited.  The component flag
	values are of the general form MOD_STRUCT_F_XXX where "MOD" is the
	module prefix, "STRUCT" is a short abbreviation for the structure that
	the flags serve, and "XXX" is the name of the individual flag value 
	itself.  The values are #define'd.

    13. For structures that represent objects that can have different kinds or
        types (not data types, this is a conceptual thing), #define's are used
	to list those types, in the general form MOD_STRUCT_T_XXX (see above
	for an explanation of that form).

    14. All modules have a prefix assigned to them.  The prefix is normally a
        two to four letter abbreviation for the module.  In the Centrallix
	project, a list of prefixes can be found in the sysdoc documentation
	package.  All functions in the module contain this prefix.  For
	example, a module with the prefix "XYZ" would have externally 
	accessible functions named "xyzFunctionOne()" or "xyzDoSomething()".
	For functions that are not accessible to the outside, the "_internal_"
	or "_i_" tag is added, as in "xyz_internal_HiddenStuff()" or for
	brevity "xyz_i_HiddenStuff()".  The prefix is used in nearly everything
	associated with the module, including any global structure, which may
	be named "XYZ_INF" or "XYZ_GLOBALS" or something like that.


-------------------------------------------------------------------------------
