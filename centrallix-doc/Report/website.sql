
select (@oldid := id) from topic where name = "5. Report Components";
delete from topic where parent_id = @oldid;
delete from topic where id = @oldid;

insert into topic values(null,1,"5. Report Components",null,
"	[b]Centrallix Reporting System Reference[/b]

	The Centrallix reporting system provides the ability to generate reports in a variety of formats from ObjectSystem data and a report object.  This document describes the report object structure in detail and how to use the various report components to build a report.

	Reports can have parameters - see the 'report' object and 'report/parameter' object for more details.  Parameters are passed to a report via HTTP URL parameters, and are defined in the report by 'report/parameter' objects (or, though deprecated, by simple top level attributes other than those already defined for 'system/report').

	Sample code is generally given in \"structure file\" format, which is the normal format for the building of applications.  However, other suitable object-structured data formats can be used, including XML.

	Where 'intvec' is specified for a property type, it means a comma-separated list of numbers.  Similarly, 'stringvec' means a comma-separated list of string values.

	Where a 'moneyformat' is specified, it is a text string in which the following special characters are recognized (the default money format is \"$0.00\"):
	[table]
	    [tr][td]#[/td][td]Optional digit unless after the decimal point or the digit immediately before the decimal point.[/td][/tr]
	    [tr][td]0[/td][td]Mandatory digit, no leading zero suppression.[/td][/tr]
	    [tr][td],[/td][td]Insert a comma, suppressed if no digits around it.[/td][/tr]
	    [tr][td].[/td][td]Decimal point (only one allowed).[/td][/tr]
	    [tr][td]$[/td][td]Dollar sign.[/td][/tr]
	    [tr][td]+[/td][td]Sign, + if positive or zero, - if negative.[/td][/tr]
	    [tr][td]-[/td][td]Sign, blank if positive or zero, - if negative.[/td][/tr]
	    [tr][td]()[/td][td]Surround number with () if it is negative.[/td][/tr]
	    [tr][td][][/td][td]Surround number with () if it is positive or zero.[/td][/tr]
	    [tr][td] [/td][td](space) optional digit, but put a space in its place if suppressing leading 0's.[/td][/tr]
	[/table]
	[p]Where a 'dateformat' is specified, it is a text string with the following character sequences recognized (default is \"dd MMM yyyy HH:mm\"):[/p]
	[table]
	    [tr][td]dd[/td][td]Two digit day of month (01 through 31).[/td][/tr]
	    [tr][td]ddd[/td][td]Day of month plus cardinality (1st through 31st).[/td][/tr]
	    [tr][td]MMMM[/td][td]Full (long) month name (January through December).[/td][/tr]
	    [tr][td]MMM[/td][td]Three-letter month abbreviation (Jan through Dec).[/td][/tr]
	    [tr][td]MM[/td][td]Two digit month (01 through 12).[/td][/tr]
	    [tr][td]yy[/td][td]Two digit year (00 through 99).[/td][/tr]
	    [tr][td]yyyy[/td][td]Four digit year (0000 through 9999).[/td][/tr]
	    [tr][td]HH[/td][td]Hour in 24-hour format (00 through 23).[/td][/tr]
	    [tr][td]hh[/td][td]Hour in 12-hour format (00 AM through 11 PM).[/td][/tr]
	    [tr][td]mm[/td][td]Minutes (00 through 59).[/td][/tr]
	    [tr][td]ss[/td][td]Seconds (00 through 59).[/td][/tr]
	    [tr][td]I[/td][td]At the beginning of the format, indicates that the date is in international (dd/mm/yy) order rather than U.S. (mm/dd/yy) order.  Used mainly when a date is being input rather than when it is being generated.[/td][/tr]
	[/table]
	Copyright (c)  1998-2009 LightSys Technology Services, Inc.
	
	[b]Documentation on the following components is available:[/b]

	[table]
	
		[tr][td][Common_Properties][/td][td]A list of common properties used by many report components[/td][/tr]
	
		[tr][td][area][/td][td]A positionable rectangular container[/td][/tr]
	
		[tr][td][data][/td][td]An expression-based data value such as text, a number, or currency[/td][/tr]
	
		[tr][td][form][/td][td]A freeform layout container for displaying query results[/td][/tr]
	
		[tr][td][image][/td][td]An image (graphic/photo)[/td][/tr]
	
		[tr][td][parameter][/td][td]Defines a parameter that can be passed to a report[/td][/tr]
	
		[tr][td][query][/td][td]A definition of a SQL query to use elsewhere in the report[/td][/tr]
	
		[tr][td][report][/td][td]The top-level report object[/td][/tr]
	
		[tr][td][table][/td][td]A tabular presentation of report data[/td][/tr]
	
	[/table]
");
select (@newid := @@last_insert_id);

	
insert into topic values(null, @newid, "Common Properties", null,
"		[b]Common_Properties[/b] :: A list of common properties used by many report components

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]Common Properties[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			Below is a list of common properties that many report writer components have.

			Visual positioning properties such as x, y, width, height are shared by areas, images, and tables.

			The margin settings margintop, marginbottom, marginleft, and marginright are shared by the system/report object as well as areas, tables, table rows, and table cells.

			Formatting settings such as style, font, fontsize, fontcolor, align, and lineheight are shared by nearly all objects except for images and queries.

			Data output properties, including dateformat, moneyformat, and nullformat, are shared by all objects except for images and queries.

			The 'condition' property is available on all objects except for system/report and report/table-cell.

			Border properties are available on areas, tables, and table-rows.

		
	[b]Usage:[/b]
	
		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]align[/td]
					[td]string[/td]
					[td]The text alignment to use - either \"left\" (default), \"center\", \"right\", or \"full\".  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]bottomborder[/td]
					[td]double[/td]
					[td]Width of the outside border at the bottom of the table, table-row, or area.[/td]
				[/tr]
			
				[tr]
					[td]condition[/td]
					[td]integer[/td]
					[td]Typically an expression that evaluates to an integer/boolean type.  If true, the area, table, table-row, image, or data element is displayed, otherwise the object (and any contents) are ignored.[/td]
				[/tr]
			
				[tr]
					[td]dateformat[/td]
					[td]string[/td]
					[td]If the field has a date/time data type, this allows the format to be specified.  See the introduction to the reporting system documentation for date/time format information.  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]font[/td]
					[td]string[/td]
					[td]The font to use.  Available fonts are \"times\", \"helvetica\", and \"courier\".  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]fontsize[/td]
					[td]integer[/td]
					[td]The font size in points.  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]fontcolor[/td]
					[td]string[/td]
					[td]The color to use for the font, in HTML RGB format such as \"#FF0000\" for Red.  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]height[/td]
					[td]double[/td]
					[td]The height of the area, image, or table.  If \"fixedsize\" is set to \"no\" (false), then the object is allowed to grow beyond this height.[/td]
				[/tr]
			
				[tr]
					[td]leftborder[/td]
					[td]double[/td]
					[td]Width of the outside border at the left side of the table, table-row, or area.[/td]
				[/tr]
			
				[tr]
					[td]lineheight[/td]
					[td]double[/td]
					[td]The line height (and thus line spacing) to use.  Normally this is set when \"fontsize\" is set, but can be manually set.  Units are standard \"y\" units, so setting this to 2.0 will not result in double-spaced lines.  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]margintop[/td]
					[td]double[/td]
					[td]The height of the top margin of the object.  Default 0.  Available on report, area, table, table-row, and table-cell.[/td]
				[/tr]
			
				[tr]
					[td]marginbottom[/td]
					[td]double[/td]
					[td]The height of the bottom margin.  Default 0.  Available on report, area, table, table-row, and table-cell.[/td]
				[/tr]
			
				[tr]
					[td]marginleft[/td]
					[td]double[/td]
					[td]The width of the left margin.  Default 0.  Available on report, area, table, table-row, and table-cell.[/td]
				[/tr]
			
				[tr]
					[td]marginright[/td]
					[td]double[/td]
					[td]The width of the right margin.  Default 0.  Available on report, area, table, table-row, and table-cell.[/td]
				[/tr]
			
				[tr]
					[td]moneyformat[/td]
					[td]string[/td]
					[td]If the field has a money data type, this allows the format to be specified.  See the introduction to the reporting system documentation for money format information.  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]nullformat[/td]
					[td]string[/td]
					[td]Sets the text to use if the data is NULL.  Normally \"NULL\" is displayed.  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]rightborder[/td]
					[td]double[/td]
					[td]Width of the outside border at the right side of the table, table-row, or area.[/td]
				[/tr]
			
				[tr]
					[td]style[/td]
					[td]stringvec[/td]
					[td]A list of styles, separated by commas.  Values include \"bold\", \"italic\", \"underline\", or \"plain\".  Available on all objects except image, query, and parameter.[/td]
				[/tr]
			
				[tr]
					[td]topborder[/td]
					[td]double[/td]
					[td]Width of the outside border at the top of the table, table-row, or area.[/td]
				[/tr]
			
				[tr]
					[td]width[/td]
					[td]double[/td]
					[td]The width of the table, area, or image.[/td]
				[/tr]
			
				[tr]
					[td]x[/td]
					[td]double[/td]
					[td]The X coordinate of the upper left corner of the table, area, or image, relative to the container.  Defaults to 0.0.[/td]
				[/tr]
			
				[tr]
					[td]y[/td]
					[td]double[/td]
					[td]The Y coordinate of the upper left corner of the table, area, or image, relative to the container.  Defaults to 0.0.[/td]
				[/tr]
			
		[/table]
	
");
	
insert into topic values(null, @newid, "system/report", null,
"		[b]report[/b] :: The top-level report object

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]system/report[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The report object is the top-level container for a report, and is the first object that should be listed in the structure file.  It basically embodies the report itself.

		
	[b]Usage:[/b]
	
			This component should only be used at the top level, and should never be used inside another container.  It may contain other report components such as queries, forms, sections, tables, etc.  Typically, a set of \"report/query\" objects is specified first inside the \"system/report\", and then the content of the report (which uses the queries) comes next.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]dateformat[/td]
					[td]string[/td]
					[td]Sets the default date/time format for the report.  See the introduction to the reporting system documentation for date/time format information.[/td]
				[/tr]
			
				[tr]
					[td]document_format[/td]
					[td]string[/td]
					[td]The content type to use for the generated report.  Possible values typically include \"application/pdf\", \"application/postscript\", \"text/html\", \"text/plain\", \"text/x-epson-escp-24\", and \"text/x-hp-pcl\".  A list of supported types can be found in the OSML directory /sys/cx.sysinfo/prtmgmt/output_types, and this list can be joined (via SQL) with /sys/cx.sysinfo/osml/types to generate a user-friendly list of supported output formats for reports.[/td]
				[/tr]
			
				[tr]
					[td]margintop[/td]
					[td]double[/td]
					[td]The height of the top margin.  Default is 3 6ths of an inch (0.5 inches).[/td]
				[/tr]
			
				[tr]
					[td]marginbottom[/td]
					[td]double[/td]
					[td]The height of the bottom margin.  Default is 3 6ths of an inch (0.5 inches).[/td]
				[/tr]
			
				[tr]
					[td]marginleft[/td]
					[td]double[/td]
					[td]The width of the left margin.  Default is 0.[/td]
				[/tr]
			
				[tr]
					[td]marginright[/td]
					[td]double[/td]
					[td]The width of the right margin.  Default is 0.[/td]
				[/tr]
			
				[tr]
					[td]moneyformat[/td]
					[td]string[/td]
					[td]Sets the default currency format for the report.  See the introduction to the reporting system documentation for money format information.[/td]
				[/tr]
			
				[tr]
					[td]nullformat[/td]
					[td]string[/td]
					[td]Sets the default NULL data format for the report; this is used when data has a NULL value.  By default, the text \"NULL\" is displayed.[/td]
				[/tr]
			
				[tr]
					[td]pagewidth[/td]
					[td]double[/td]
					[td]The width of the physical page, in the specified units.  The default is 85 10ths of an inch (8.5 inches).[/td]
				[/tr]
			
				[tr]
					[td]pageheight[/td]
					[td]double[/td]
					[td]The height of the physical page, in the specified units.  The default is 66 6ths of an inch (11 inches).[/td]
				[/tr]
			
				[tr]
					[td]resolution[/td]
					[td]integer[/td]
					[td]Graphics resolution for the report, in dots per inch (DPI); typically set to 300, 600, or 1200.  Any images/photos in the report will be rendered at this resolution, and this resolution sets the minimum width for borders and lines around tables and areas.  This setting is NOT affected by the 'units' setting and is always in DPI.[/td]
				[/tr]
			
				[tr]
					[td]title[/td]
					[td]string[/td]
					[td]The title for the report.[/td]
				[/tr]
			
				[tr]
					[td]units[/td]
					[td]string[/td]
					[td]The units of measure to use for the report.  This setting affects nearly all positioning and sizing values, including x, y, width, height, margins, and borders.  Possible values are:  \"us_forms\" (the default), meaning 10ths of an inch for x/width and 6ths of an inch for y/height; \"inches\"; \"points\" (72nds of an inch); \"centimeters\"; and \"millimeters\".[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		
		
$Version=2$
myreport \"system/report\"
	{
	minsize \"report/parameter\" { type=integer; default = 1; }  // a parameter.
	title = \"This is a hello-world report\";

	comment \"report/data\"
		{
		value = \"Hello World\";
		}
	}
		
		
		[/code]
	
");
	
insert into topic values(null, @newid, "report/query", null,
"		[b]query[/b] :: A definition of a SQL query to use elsewhere in the report

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]report/query[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The query is used to define a SQL statement that will be used to obtain data from the ObjectSystem.  It in and of itself does not actually run the query, but must be invoked by a table or form elsewhere in the report.

			Queries can be linked to each other in order to depend on each others' data results.

			Queries can also contain aggregate definitions, which are similar to 'COMPUTE BY' statements in other SQL languages, and calculate aggregate values as a the report progresses.  These aggregates are normally used to provide summary data at the end of the query or at the end of a part of the query.

		
	[b]Usage:[/b]
	
			Queries can only be contained within a 'system/report'.  They can only contain 'report/aggregate' components.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]sql[/td]
					[td]string[/td]
					[td]A SQL statement.  It can be a plain string, or an expression if the SQL is to be constructed dynamically.  The expressions inside the SQL statement (such as inside the WHERE clause or the SELECT item list) can reference parameters of the report using :this:parametername, and can reference values from other queries using :queryname:fieldname.  See \"SQL Language\" for more details on Centrallix SQL.  Though deprecated, the SQL can also include report parameter substitution by embedding '&xyz' in the report to reference the parameter 'xyz' (do not use).[/td]
				[/tr]
			
				[tr]
					[td]link[/td]
					[td]stringvec[/td]
					[td]Deprecated.  Two strings, one specifying the query to link to, and the second specifying the name of the column/field to reference in that query.  In the SQL statement, to use this method to reference linked data from other queries, use '&1' and '&2' (and so forth) to reference the first and second links, respectively.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of report/aggregate child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]compute[/td]
							[td]string[/td]
							[td]An expression specifying the computation to be performed.  Normally this should include one or more aggregate functions (sum, count, avg, min, and max) in some form or fashion.[/td]
						[/tr]
					
						[tr]
							[td]where[/td]
							[td]string[/td]
							[td]A conditional expression specifying which rows should be included in the aggregate computation.[/td]
						[/tr]
					
						[tr]
							[td]reset[/td]
							[td]integer[/td]
							[td]Either 0 or 1 (the default).  Controls whether or not the aggregate (computed) value is reset when used.  Normally, whenever the value is referenced, it is automatically reset to 0 (count) or null (sum/avg/min/max/first/last).  Setting this value to 0 causes the value to accumulate during the entire time the report is running, and is useful for such things as numbering lines (when using count()) or generating a running total (with sum()).[/td]
						[/tr]
					
				[/table]
			
	[b]Sample Code:[/b]
	
		[code]
		
		
myQuery \"report/query\"
	{
	// standard SQL statement referencing parameter 'minsize'
	sql = \"select :name, :size from /samples where :size >= :this:minsize\";

	// a couple of aggregates for totaling things up
	totalsize \"report/aggregate\" { compute = \"sum(:size)\"; }
	bigfilesizetotal \"report/aggregate\" { compute = \"sum(:size)\"; where=\":size > 10000\"; }
	}

myQueryTwo \"report/query\"
	{
	// dynamically built SQL illustration
	sql = runserver(\"select :name, :size from /samples where :size >= :this:minsize\" + condition(:this:orderby_size == 1, \" order by :size\", \" order by :name\"));
	}
		
		
		[/code]
	
");
	
insert into topic values(null, @newid, "report/data", null,
"		[b]data[/b] :: An expression-based data value such as text, a number, or currency

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]report/data[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The data component allows the value of an arbitrary expression to be printed.  The expression can contain references to report parameters (via the object 'this') or to query fields.

		
	[b]Usage:[/b]
	
			This component can be used within an area, section, table row, table column, form, or at the top level within the system/report object.  It cannot contain any components.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]autonewline[/td]
					[td]yes/no[/td]
					[td]Whether to automatically emit a newline at the end of text.  Defaults to \"no\".[/td]
				[/tr]
			
				[tr]
					[td]value[/td]
					[td]expression[/td]
					[td]The text to be printed.  Can reference report/query objects using :queryname:fieldname syntax.  When the expression references queries, it should be enclosed in the runserver() domain-declaration function (see SQL Language / Functions and Operators for more details).[/td]
				[/tr]
			
				[tr]
					[td]xpos[/td]
					[td]integer[/td]
					[td]The horizontal position to offset the text to.  See \"units\" in \"system/report\".[/td]
				[/tr]
			
				[tr]
					[td]ypos[/td]
					[td]integer[/td]
					[td]The vertical position to offset the text to.  See \"units\" in \"system/report\".[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		
		
myData \"report/data\"
	{
	fontcolor = \"#0000ff\";
	fontsize = 15;
	style = \"bold\";
	value = runserver(:myQuery:string1 + ', ' + :myQuery:string2 + '.');
	}
		
		
		[/code]
	
");
	
insert into topic values(null, @newid, "report/form", null,
"		[b]form[/b] :: A freeform layout container for displaying query results

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]report/form[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The form provides a way to display query results in a non-tabular fashion.  Each individual piece of data has to be manually positioned, requiring more effort but giving more flexibility.  The form will essentially iterate through its contents for each record it retrieves from a query.

			Forms can have one of three modes: normal, outer, and inner.  A 'normal' form simply starts the query, retrieves all of the results from the query (up to the reclimit maximum), iterating over its content once for each record, and then ends the query, discarding any data remaining.  An 'outer' form starts the query and then iterates over its content while more results are available, but does not actually retrieve the results itself.  An 'inner' form works only (directly or indirectly) inside an 'outer' form, and retrieves records (up to the reclimit maximum) but does not start or end the query.

			The typical reason to use 'outer' and 'inner' mode forms is to group query results in chunks of a certain size, perhaps in a part of a multipage preprinted form such as a receipt or invoice.  The 'outer' mode form is used to surround the multipage preprinted forms as a whole, and the 'inner' mode form is used to generate the section of the report that contains the list that carries over from page to page but in an area not the size of the entire form (and possibly having other data above and/or below it).

			Forms can also be used with multiple queries simultaneously.  They support running these queries in one of several fashions: 'nested', 'parallel', and 'multinested'.  In the first, 'nested', if two queries were specified, the second query would be run once in its entirety for each record returned from the first - this is identical to nesting multiple forms inside of one another, each referencing only one query.

			    
				1,2,3a,b
				1a
				1b
				2a
				2b
				3a
				3b
			    
			The second, 'parallel', runs the queries simultaneously and independently of one another - the results are combined together on the output - and continues until no query has more records.

			    
				1,2,3a,b
				1a
				2b
				3NULL
			    
			The third, 'multinested', is more interesting - here the form iterates once through each query before the nested queries are run, giving each individual record its own unique line.

			    
				1,2,3a,b
				1NULL
				1a
				1b
				2NULL
				2a
				2b
				3NULL
				3a
				3b
			    
		
	[b]Usage:[/b]
	
			May be used inside any visual container, and may contain any visual component or container, including areas, tables, other forms, and data elements.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]ffsep[/td]
					[td]yes/no[/td]
					[td]Whether to do a page break between successive records.  Default 'no'.[/td]
				[/tr]
			
				[tr]
					[td]mode[/td]
					[td]string[/td]
					[td]The mode of the form (see overview).  'normal' is the default.[/td]
				[/tr]
			
				[tr]
					[td]multimode[/td]
					[td]string[/td]
					[td]How the form handles multiple queries (see overview).  'nested' is the default.[/td]
				[/tr]
			
				[tr]
					[td]page[/td]
					[td]integer[/td]
					[td]Sets the starting page number for the form.  Used sometimes if the page number has to be reset to 1, as in reports containing a series of multipage parts (such as multi-page invoices).[/td]
				[/tr]
			
				[tr]
					[td]reclimit[/td]
					[td]integer[/td]
					[td]Sets a limit on the number of records the form is to print.[/td]
				[/tr]
			
				[tr]
					[td]source[/td]
					[td]stringvec[/td]
					[td]A list of one or more query(ies) to run for this form.  If more than one is specified, use the 'multimode' setting to specify how they are combined.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		
		
normalForm \"report/form\"
	{
	source=myQuery;
	ffsep=yes;

	//
	// query data is printed here with report/data, etc.
	//
	}

outerForm \"report/form\"
	{
	// This inner/outer form pair displays six records per page.
	source=myQuery;
	mode=outer;
	ffsep=yes;
	comment \"report/data\" { value=\"Here are up to six of the records:\\n\"; }

	innerForm \"report/form\"
		{
		source=myQuery;
		mode=inner;
		reclimit=6;

		//
		// display query data here with report/data, etc.
		//
		}
	}
		
		
		[/code]
	
");
	
insert into topic values(null, @newid, "report/table", null,
"		[b]table[/b] :: A tabular presentation of report data

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]report/table[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The table component is used to present data in an orderly, tabular fashion, in rows and columns.  While less flexible than the form, it is much easier to use.

			Tables, like forms, can have more than one 'mode'.  However, tables only support 'normal' and 'inner'; 'outer' is not supported since tables do not contain other components.  See the form documentation for more on the mode.

			Tables also can handle multiple queries using a 'multimode' - see the form for more information.

		
	[b]Usage:[/b]
	
			Tables can be used inside any visual container or inside forms.  Tables can contain \"report/table-row\" objects.  Table rows can contain either a set of \"report/table-cell\" objects, or other objects such as areas, data elements, and so forth.  A row cannot contain both a table-cell and an object of another type.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]allowbreak[/td]
					[td]yes/no[/td]
					[td]Whether to allow the table to span multiple pages in the report.[/td]
				[/tr]
			
				[tr]
					[td]colsep[/td]
					[td]double[/td]
					[td]Sets the separation between the table's columns.[/td]
				[/tr]
			
				[tr]
					[td]columns[/td]
					[td]integer[/td]
					[td]The number of columns in the table.[/td]
				[/tr]
			
				[tr]
					[td]fixedsize[/td]
					[td]yes/no[/td]
					[td]If set to 'yes', then the 'height' setting (see Common Properties) is followed strictly, otherwise the table is allowed to grow vertically to fit its contents.[/td]
				[/tr]
			
				[tr]
					[td]innerborder[/td]
					[td]double[/td]
					[td]Width of the inside borders within the table.[/td]
				[/tr]
			
				[tr]
					[td]mode[/td]
					[td]string[/td]
					[td]The mode of the table (see overview).  'normal' is the default.[/td]
				[/tr]
			
				[tr]
					[td]multimode[/td]
					[td]string[/td]
					[td]How the table handles multiple queries (see overview).  'nested' is the default.[/td]
				[/tr]
			
				[tr]
					[td]nodatamsg[/td]
					[td]yes/no[/td]
					[td]Whether to display the '(no data returned)' message below the table if no records were returned from the query source.  Default is 'yes' to display the no-data message.[/td]
				[/tr]
			
				[tr]
					[td]outerborder[/td]
					[td]double[/td]
					[td]Width of the outside border around the table.  This sets topborder, leftborder, bottomborder, and rightborder simultaneously (see Common Properties).[/td]
				[/tr]
			
				[tr]
					[td]reclimit[/td]
					[td]integer[/td]
					[td]Sets a limit on the number of records the table is to print.[/td]
				[/tr]
			
				[tr]
					[td]shadow[/td]
					[td]double[/td]
					[td]Width of the drop-shadow to the right and bottom of the table.[/td]
				[/tr]
			
				[tr]
					[td]source[/td]
					[td]stringvec[/td]
					[td]A list of one or more query(ies) to run for this form.  If more than one is specified, use the 'multimode' to determine how they are combined.[/td]
				[/tr]
			
				[tr]
					[td]widths[/td]
					[td]intvec[/td]
					[td]A list of numbers giving the widths of the columns.  This MUST match the number of columns specified with the 'columns' setting.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
			(of report/table-row child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]allowbreak[/td]
							[td]yes/no[/td]
							[td]Whether to allow the row to span multiple pages in the report.[/td]
						[/tr]
					
						[tr]
							[td]fixedsize[/td]
							[td]yes/no[/td]
							[td]If set to 'yes', then the 'height' setting (see Common Properties) is followed strictly, otherwise the table row is allowed to grow vertically to fit its contents.[/td]
						[/tr]
					
						[tr]
							[td]header[/td]
							[td]yes/no[/td]
							[td]Whether the row is a header row (repeated on subsequent pages).[/td]
						[/tr]
					
						[tr]
							[td]innerborder[/td]
							[td]double[/td]
							[td]Width of the inside borders between cells in the row.[/td]
						[/tr]
					
						[tr]
							[td]outerborder[/td]
							[td]double[/td]
							[td]Width of the outside border around the row.  This sets topborder, leftborder, bottomborder, and rightborder simultaneously (see Common Properties).[/td]
						[/tr]
					
						[tr]
							[td]summary[/td]
							[td]yes/no[/td]
							[td]Whether the row is a summary row, emitted at the end of the table (no summarize_for) or at the end of each group of rows (if summarize_for is set).[/td]
						[/tr]
					
						[tr]
							[td]summarize_for[/td]
							[td]expression[/td]
							[td]An expression whose value uniquely defines the desired group of rows.  The summary row is printed at the end of each group of consecutive rows having the same value for this expression.[/td]
						[/tr]
					
				[/table]
			
			(of report/table-cell child widgets)
			
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				
						[tr]
							[td]value[/td]
							[td]expression[/td]
							[td]The text to be printed inside the cell.  Can reference report/query objects using :queryname:fieldname syntax.  When the expression references queries, it should be enclosed in the runserver() domain-declaration function (see SQL Language / Functions and Operators for more details).[/td]
						[/tr]
					
				[/table]
			
	[b]Sample Code:[/b]
	
		[code]
		
		
myTable \"report/table\"
	{
	source=myQuery;
	columns = 2;
	widths = 40,40;
	hdr \"report/table-row\"
	    {
	    header = yes;
	    h_name \"report/table-cell\" { align=center; style=bold; value=\"Name\"; }
	    h_size \"report/table-cell\" { align=center; style=bold; value=\"Size\"; }
	    }
	data \"report/table-row\"
	    {
	    t_name \"report/table-cell\" { align=center; style=bold; value=runserver(:myQuery:name); }
	    t_size \"report/table-cell\" { align=center; style=bold; value=runserver(:myQuery:size); }
	    }
	}
		
		
		[/code]
	
");
	
insert into topic values(null, @newid, "report/image", null,
"		[b]image[/b] :: An image (graphic/photo)

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]report/image[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The image object allows a graphics file or photo to be embedded in the report.  Currently only the PNG format is supported, so convert your image to PNG before using it in a report.

			The output quality is determined by the 'resolution' setting specified in the system/report object.  Lower resolutions result in a poorer quality but also result in faster-running reports.

		
	[b]Usage:[/b]
	
			The image object may be used inside the system/report, or inside areas, table rows, or table cells.

			See 'Common Properties' for x, y, width, and height, which are used for positioning the image.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]source[/td]
					[td]string[/td]
					[td]The ObjectSystem path to the PNG image file.[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[code]
		
		
img \"report/image\"
	{
	x=0; y=0; height=4.44; width=65;
	source = \"/images/DocumentHeader.png\";
	}
		
		
		[/code]
	
");
	
insert into topic values(null, @newid, "report/area", null,
"		[b]area[/b] :: A positionable rectangular container

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]report/area[/td][/tr]
		[tr][td]visual:[/td][td] yes[/td][/tr]
		[tr][td]container:[/td][td] yes[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			The area object is a generic positionable rectangular container that can also have a border around it.  Text added inside it with a 'report/data' element or a 'value' setting (as with table rows and table cells) can flow using text flow, justification, and wrapping semantics.

		
	[b]Usage:[/b]
	
			Areas may be placed inside the system/report, or inside other areas, table rows, or table cells.  Areas are containers, so they can contain other visual objects.

			See 'Common Properties' for x, y, width, and height, which are used for positioning the area.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]allowbreak[/td]
					[td]yes/no[/td]
					[td]Whether to allow the area to span multiple pages in the report if the text causes it to be larger than one page can hold.[/td]
				[/tr]
			
				[tr]
					[td]border[/td]
					[td]double[/td]
					[td]Width of the outside border around the area.[/td]
				[/tr]
			
				[tr]
					[td]fixedsize[/td]
					[td]yes/no[/td]
					[td]If set to 'yes', then the 'height' setting (see Common Properties) is followed strictly, otherwise the area is allowed to grow vertically to fit its contents.[/td]
				[/tr]
			
				[tr]
					[td]value[/td]
					[td]expression[/td]
					[td]The text to be printed in the area.  Can reference report/query objects using :queryname:fieldname syntax.  When the expression references queries, it should be enclosed in the runserver() domain-declaration function (see SQL Language / Functions and Operators for more details).[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	
insert into topic values(null, @newid, "report/parameter", null,
"		[b]parameter[/b] :: Defines a parameter that can be passed to a report

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]report/parameter[/td][/tr]
		[tr][td]visual:[/td][td] no[/td][/tr]
		[tr][td]container:[/td][td] no[/td][/tr]
		[/table]
		
	[b]Overview:[/b]
	
			A parameter defines a value that can be passed to a report, as well as various constraints and aspects of that value.

			A default value can be specified, which will be used if the user does not supply a value when running the report.

			A variety of constraint properties are also allowed, such as allowchars and badchars.

		
	[b]Usage:[/b]
	
			The parameter object can only be used inside a system/report object.

		
	[b]Properties:[/b]
	
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		
				[tr]
					[td]allowchars[/td]
					[td]string[/td]
					[td]The set of characters that are allowed. (e.g. allowchars=\"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\"; )[/td]
				[/tr]
			
				[tr]
					[td]badchars[/td]
					[td]string[/td]
					[td]The set of characters that are explicitly not allowed. (e.g. badchars=\" \\\"'/,;\"; )[/td]
				[/tr]
			
				[tr]
					[td]default[/td]
					[td]mixed[/td]
					[td]If a parameter is not passed in, then the (optional) default value is assigned to the parameter (e.g. default = null, or 2745).[/td]
				[/tr]
			
				[tr]
					[td]type[/td]
					[td]string[/td]
					[td]The data type of the parameter.  Can be \"integer\", \"string\", \"double\", \"datetime\", or \"money\".[/td]
				[/tr]
			
		[/table]
	
	[b]Child Properties:[/b]
	
		[i]none currently available[/i]
	
	[b]Sample Code:[/b]
	
		[i]none currently available[/i]
	
");
	