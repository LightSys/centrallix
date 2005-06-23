// Copyright (C) 1998-2001 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function term_get_sid(e)
    {
    var frame = e.currentTarget;
    var term = frame.term;
    frame.removeEventListener("load",term_get_sid,false);
    var a = frame.contentDocument.getElementsByTagName("a");
    if(a.length!=1)
	{
	alert("protocol error in communication with centrallix");
	return false;
	}
    term.sid = a[0].target;

    frame.addEventListener("load",term_get_oid,false);
    term.reader.src = term.source+"?ls__mode=osml&ls__req=open&ls__sid="+term.sid+"&ls__usrtype=system%2fobject&ls__objmode=0&ls__objmask=0";

    return true;
    }

function term_get_oid(e)
    {
    var frame = e.currentTarget;
    var term = frame.term;
    frame.removeEventListener("load",term_get_oid,false);
    var a = frame.contentDocument.getElementsByTagName("a");
    if(a.length<1)
	{
	alert("protocol error in communication with centrallix");
	return false;
	}
    term.oid = a[0].target;

    term.connected = true;

    term.reader.addEventListener("load",term_read,false);
    term.readString = term.baseURL+"?ls__mode=osml&ls__req=read&ls__oid="+term.oid+"&ls__sid="+term.sid+"&ls__bytecount="+term.blocksize+"&ls__flags=1"; // FD_NO_BLOCK
    term.reader.src = term.readString;

    return true;
    }

/** sets up the passed term to be a vt100-compatible terminal **/
function term_vt100(term)
    {
    /** \x1b is ASCII ESC **/
    term.chunkifier = /(?:\x1b\[([0-9;]*)([^0-9]))|\x1b([^[])|(\r|\n|\t)|([^\x1b\r\n\t]+)/g;
    term.processchunk = term_vt100_process_chunk;
    term.linewrap = false;
    }

function term_vt100_process_chunk(chunk)
    {
    var term = this;
    if(chunk[2])
	{
	/** normal escape sequence: \x1b[1g **/
	if(chunk[2]=="H" || chunk[2]=="f")
	    {
	    /** move to m,n (both m and n are optional -- use 0 for any missing params) **/
	    var d = chunk[1].match(/^([0-9]*)\(,([0-9]*)\)?$/);
	    if(!d) d=new Array();
	    if(!d[1]) d[1]=0;
	    if(!d[2]) d[2]=0;
	    term.nextrow=d[1];
	    term.nextcol=d[2];
	    }
	else if(chunk[2]=="K")
	    {
	    /** erase to EOL **/
	    term_clear_end_line(term,term.nextrow,term.nextcol);
	    }
	else if(chunk[2]=="m")
	    {
	    /** set character attributes **/
	    }
	else if(chunk[2]=="J")
	    {
	    /** erase to end of page **/
	    // not impliemented right now 
	    }
	else
	    {
	    /** unimplimented escape sequence **/
	    alert("unimplimented escape sequence: "+chunk[0]);
	    }
	}
    else if(chunk[3])
	{
	/** short escape sequence: \x1bg **/
	}
    else if(chunk[4])
	{
	/** \r or \n **/
	if(chunk[4]=="\r")
	    {
	    /** carriage return **/
	    term.nextcol = 0;
	    }
	else if(chunk[4]=="\n")
	    {
	    /** line feed **/
	    term.nextrow++;
	    }
	else if(chunk[4]=="\t")
	    {
	    /** tab **/
	    var numspaces = (Math.floor(term.nextcol/term.tabstop)+1)*term.tabstop-term.nextcol;
	    var str = "";
	    for(var i=0;i<numspaces;i++)
		{
		str+=" ";
		}
	    term.processchunk(new Array(str,null,null,null,null,str));
	    }
	else
	    {
	    alert("regex error: chunk[4] == "+escape(chunk[4]));
	    }
	}
    else
	{
	/** plain data in chunk[5] **/
	var str = chunk[5];
	/** should strip all characters between 0x00 and 0x1f **/

	/** make sure we're on a valid column **/
	if(term.nextcol>=term.cols)
	    {
	    if(term.linewrap)
		{
		term.nextrow++;
		term.nextcol=0;
		}
	    else
		{
		/** we're passed the edge of the terminal and not wrapping... **/
		return true;
		}
	    }
	/** make sure we're on a valid row **/
	if(term.nextrow>=term.rows)
	    {
	    return false;
	    }

	var span = term.firstChild;
	while(span.nextSibling)
	    {
	    if(span.row<term.nextrow)
		{
		break;
		}
	    if(span.row==term.nextrow && span.col<=term.nextcol)
		{
		break;
		}
	    span = span.nextSibling;
	    }
	var skiprows = term.nextrow-span.row;
	var skipcols = term.nextcol-span.col;
	var skipdist = skiprows*(term.cols+1)+skipcols;

	var numthisrow = term.cols-term.nextcol;
	if(str.length > numthisrow)
	    {
	    /** process the part that fits on this row **/
	    var left = span.data.slice(0,skipdist);
	    var right = span.data.slice(skipdist+numthisrow);
	    span.data=left+str.substr(0,numthisrow)+right;

	    term.nextcol+=numthisrow;

	    /** process the leftovers **/
	    term.processchunk(new Array(str.substring(numthisrow),null,null,null,null,str.substring(numthisrow)));
	    }
	else
	    {
	    /** process all the data -- it all fits on the current row **/
	    var left = span.data.slice(0,skipdist);
	    var right = span.data.slice(skipdist+str.length);
	    span.data=left+str+right;
	    term.nextcol+=str.length;
	    }

	span.dirty = true;

	}

    }

function term_reload_frame(frame)
    {
    var src = frame.src + " ";
    src.replace(/ $/);
    frame.src = src;
    }

function term_read(e)
    {
    var frame = e.currentTarget;
    var term = frame.term;
    var links = frame.contentDocument.getElementsByTagName("a");
    if(links.length!=1)
	{
	alert("protocol error in communication with centrallix -- "+links.length+" where 1 expected");
	return false;
	}

    var data = new String(htutil_unpack(links[0].text));
    if(data.length>0)
	{
	data = term.readbuffer+data;
	term.readbuffer = "";

	var regex = term.chunkifier;
	var chunks = new Array();
	var start = regex.lastIndex = 0;
	for(var a; a = regex.exec(data); )
	    {
	    if(a[0].length != regex.lastIndex-start)
		{
		term.readbuffer=data.substring(start);
		break;
		}
	    start = regex.lastIndex;
	    term.processchunk(a);
	    }
	if(start != data.length)
	    {
	    term.readbuffer=data.substring(start);
	    }
	
	term_write_spans(term);

	/** the 1 millisecond delay is to allow Mozilla to refresh the display **/
	setTimeout("term_reload_frame(document.getElementById('term"+term.id+"reader'));",1);
	}
    else
	{
	/** wait for reload **/
	setTimeout("term_reload_frame(document.getElementById('term"+term.id+"reader'));",term.refreshrate);
	}
    return false;
    }

function term_write_callback(e)
    {
    var frame = e.currentTarget;
    var term = frame.term;
    term.writer.removeEventListener("load",term_write_callback,false);
    frame.inprogress = false;
    term.curwrite = "";
    }

function term_write()
    {
    var frame = this;
    var term = frame.term;
    if(frame.inprogress)
	{
	return false;
	}

    term.curwrite = term.writebuffer;
    term.writebuffer = "";
    frame.inprogress = true;
    term.writer.addEventListener("load",term_write_callback,false);
    /** now we need to write the data ..... **/

    }

function term_send_data(data)
    {
    var term = this;
    term.writebuffer+=data;
    }

function term_clear_end_line(term,line,start)
    {
    if(line<term.rows)
	{
	/** store the current write location **/
	var oldrow = term.nextrow;
	var oldcol = term.nextcol;
	term.nextrow=line;
	term.nextcol=start;
	var str = "";
	for(var i=start;i<term.cols;i++)
	    {
	    str+=" ";
	    }
	/** build and process a fake chunk :) **/
	term.processchunk(new Array(str,null,null,null,null,str));
	/** restore the original write location **/
	term.nextrow = oldrow;
	term.nextcol = oldcol;
	}
    }

function term_write_spans(term)
    {
    for(var i=0;i<term.childNodes.length;i++)
	{
	term_write_span(term.childNodes[i])
	}
    }

function term_write_span(s)
    {
    if(!s.dirty) return false;
    s.dirty = false;
    var data = s.data.replace(/ /g,"&nbsp;");
    data = data.replace(/\\n/g,"<br/>\n");
    s.innerHTML = data;
    }


/** Connect terminal **/
function term_connect()
    {
    if(this.connected)
	return false;
    this.reader.addEventListener("load",term_get_sid,false);
    this.reader.src = this.baseURL+"?ls__mode=osml&ls__req=opensession";
    }

/** Disconnect terminal **/
function term_disconnect()
    {
    if(!this.connected)
	return false;
    alert("disconnect() is not implimented");
    }

function term_keyhandler(term, event, key)
    {
    if(!term_current) return;
    alert("key: "+k);
    }

function term_select(x,y,term,c,n)
    {
    alert("selected!");
    term_current = term;
    }

function term_deselect()
    {
    alert("deselected!");
    term_current = null;
    }

/** Terminal initializer **/
function terminal_init(param)
    {
    var parent = param.parent;
    var term = parent.getElementById('term'+param.id+'base');
    term.id = param.id;
    term.reader = parent.getElementById('term'+param.id+'reader');
    term.reader.term = term;
    term.writer = parent.getElementById('term'+param.id+'writer');
    term.writer.term = term;
    term.kind = 'term';

    term.source = param.source;
    term.rows = param.rows;
    term.cols = param.cols;
    term.colors = param.colors;

    term.blocksize = 2048;
    term.tabstop = 8;
    term.refreshrate = 2000;

    term.innerHTML="<span class='fixed"+term.id+"'></span>";
    var s = "";
    for(var i=0;i<term.rows;i++)
	{
	for(var j=0;j<term.cols;j++)
	    {
	    s+=" ";
	    }
	s+="\n";
	}
    term.firstChild.row = 0;
    term.firstChild.col = 0;
    term.firstChild.data=s;
    term.firstChild.dirty = true;
    term_write_span(term.firstChild);

    term.readbuffer = "";
    term.writebuffer = "";

    term.nextrow = 0;
    term.nextcol = 0;
    term.currentcolor = 7;

    term_vt100(term);

    term.keyhandler = term_keyhandler;
    term.getfocushandler = term_select;
    term.losefocushandler = term_deselect;
    
    /** no idea what I'm doing here .....
    var compStyle = document.defaultView.getComputedStyle(term,"");
    var height = compStyle.getPropertyCSSValue("height");
    var width = compStyle.getPropertyCSSValue("width");
    height = height.getFloatValue(height.primitiveType);
    width = width.getFloatValue(width.primitiveType);
    **/

    /** I don't know what that last constant means... **/
    //pg_addarea(term,0,0,height,width,'term','term',3);

    term.ActionConnect = term_connect;
    term.ActionDisconnect = term_disconnect;

    /** this is needed because if the document is loaded from /samples/test.app,
      XSS protections prohibit the access of content from / (/samples/ is ok) **/
    term.baseURL = document.URL.match(/[^?]*/)[0];
    term.baseURL = term.baseURL.match(/(.*\/)[^\/]*/)[1];

    /** term.baseURL should now be a valid path that will not violate XSS protections **/

    term.oid = null;
    term.sid = null;

    
    term.connected = false;
    term.ActionConnect();

    return term;
    }
