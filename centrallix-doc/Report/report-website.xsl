<?xml version='1.0'?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:exslt="http://exslt.org/common">
<xsl:output method="text" indent="no"/>
<xsl:preserve-space elements="xsl:text"/>

<xsl:template match="/components">
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
	<xsl:for-each select="component">
		<xsl:sort select="@name" order="ascending"/>
		<xsl:variable name="componentname"><xsl:value-of select="@name"/></xsl:variable>
		[tr][td][<xsl:value-of select="@name"/>][/td][td]<xsl:value-of select="@description"/>[/td][/tr]
	</xsl:for-each>
	[/table]
");
select (@newid := @@last_insert_id);

	<xsl:for-each select="component">
		<xsl:variable name="componentname"><xsl:value-of select="@name"/></xsl:variable>
insert into topic values(null, @newid, "<xsl:value-of select="@type"/>", null,
"		[b]<xsl:value-of select="@name"/>[/b] :: <xsl:value-of select="@description"/>

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]<xsl:value-of select="@type"/>[/td][/tr]
		[tr][td]visual:[/td][td] <xsl:value-of select="@visual"/>[/td][/tr]
		[tr][td]container:[/td][td] <xsl:value-of select="@container"/>[/td][/tr]
		[/table]
		<xsl:apply-templates select="overview"/>
		<xsl:apply-templates select="usage"/>
		<xsl:apply-templates select="properties"/>
		<xsl:apply-templates select="children"/>
		<xsl:apply-templates select="sample"/>
");
	</xsl:for-each>
</xsl:template>

<xsl:template match="overview">
	[b]Overview:[/b]
	<xsl:apply-templates/>
</xsl:template>

<xsl:template match="usage">
	[b]Usage:[/b]
	<xsl:apply-templates/>
</xsl:template>

<xsl:template match="p">
	<xsl:variable name="current">
		<xsl:apply-templates/>
	</xsl:variable>
	<xsl:call-template name="str_escape">
		<xsl:with-param name="search" select="'&quot;'"/>
		<xsl:with-param name="escchar" select="'&#092;'"/>
		<xsl:with-param name="subject" select="string($current)"/>
	</xsl:call-template>
<xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="li">
	<xsl:variable name="current">
		<xsl:apply-templates/>
	</xsl:variable>
	<xsl:call-template name="str_escape">
		<xsl:with-param name="search" select="'&quot;'"/>
		<xsl:with-param name="escchar" select="'&#092;'"/>
		<xsl:with-param name="subject" select="string($current)"/>
	</xsl:call-template>
<xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="properties">
	[b]Properties:[/b]
	<xsl:if test="boolean(count(property))">
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		<xsl:for-each select="property">
			<xsl:if test="boolean(count(ni))">
				[tr]
					[td]<xsl:value-of select="@name"/>[/td]
					[td]<xsl:value-of select="@type"/>[/td]
					[td]<xsl:call-template name="str_escape">
						<xsl:with-param name="search" select="'&quot;'"/>
						<xsl:with-param name="escchar" select="'&#092;'"/>
						<xsl:with-param name="subject" select="string()"/>
					</xsl:call-template>[/td]
				[/tr]
			</xsl:if>
			<xsl:if test="not(count(ni))">
				[tr]
					[td]<xsl:value-of select="@name"/>[/td]
					[td]<xsl:value-of select="@type"/>[/td]
					[td]<xsl:call-template name="str_escape">
						<xsl:with-param name="search" select="'&quot;'"/>
						<xsl:with-param name="escchar" select="'&#092;'"/>
						<xsl:with-param name="subject" select="string()"/>
					</xsl:call-template>[/td]
				[/tr]
			</xsl:if>
		</xsl:for-each>
		[/table]
	</xsl:if>
	<xsl:if test="not(count(property))">
		[i]none currently available[/i]
	</xsl:if>
</xsl:template>

<xsl:template match="children">
	[b]Child Properties:[/b]
	<xsl:if test="count(child)">
		<xsl:for-each select="child">
			(of <xsl:value-of select="@type"/> child widgets)
			<xsl:if test="count(childproperty)">
				[table]
				[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
				<xsl:for-each select="childproperty">
					<xsl:if test="boolean(count(ni))">
						[tr]
							[td]<xsl:value-of select="@name"/>[/td]
							[td]<xsl:value-of select="@type"/>[/td]
							[td]<xsl:call-template name="str_escape">
								<xsl:with-param name="search" select="'&quot;'"/>
								<xsl:with-param name="escchar" select="'&#092;'"/>
								<xsl:with-param name="subject" select="string()"/>
							</xsl:call-template>[/td]
						[/tr]
					</xsl:if>
					<xsl:if test="not(count(ni))">
						[tr]
							[td]<xsl:value-of select="@name"/>[/td]
							[td]<xsl:value-of select="@type"/>[/td]
							[td]<xsl:call-template name="str_escape">
								<xsl:with-param name="search" select="'&quot;'"/>
								<xsl:with-param name="escchar" select="'&#092;'"/>
								<xsl:with-param name="subject" select="string()"/>
							</xsl:call-template>[/td]
						[/tr]
					</xsl:if>
				</xsl:for-each>
				[/table]
			</xsl:if>
			<xsl:if test="not(count(childproperty))">
				[i]none currently available[/i]
			</xsl:if>
		</xsl:for-each>
	</xsl:if>
	<xsl:if test="not(count(child))">
		[i]none currently available[/i]
	</xsl:if>
</xsl:template>

<xsl:template match="sample">
	[b]Sample Code:[/b]
	<xsl:if test="string-length(normalize-space(string()))">
		[code]
		<xsl:call-template name="str_escape">
			<xsl:with-param name="search" select="'&quot;'"/>
			<xsl:with-param name="escchar" select="'&#092;'"/>
			<xsl:with-param name="subject" select="string()"/>
		</xsl:call-template>
		[/code]
	</xsl:if>
	<xsl:if test="not(string-length(normalize-space(string())))">
		[i]none currently available[/i]
	</xsl:if>
</xsl:template>

<!-- str_escape in XSLT.  -->
<xsl:template name="str_escape">
	<xsl:param name="search"/>
	<xsl:param name="escchar"/>
	<xsl:param name="subject"/>
	<xsl:choose>
		<xsl:when test="contains($subject, $search) and not(contains(substring-before($subject,$search), $escchar))">
			<xsl:value-of select="substring-before($subject,$search)"/>
			<xsl:value-of select="concat($escchar, $search)"/>
			<xsl:call-template name="str_escape">
				<xsl:with-param name="search" select="$search"/>
				<xsl:with-param name="escchar" select="$escchar"/>
				<xsl:with-param name="subject" select="substring-after($subject,$search)"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:when test="contains($subject, $escchar) and not(contains(substring-before($subject,$escchar), $search))">
			<xsl:value-of select="substring-before($subject,$escchar)"/>
			<xsl:value-of select="concat($escchar, $escchar)"/>
			<xsl:call-template name="str_escape">
				<xsl:with-param name="search" select="$search"/>
				<xsl:with-param name="escchar" select="$escchar"/>
				<xsl:with-param name="subject" select="substring-after($subject,$escchar)"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$subject"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>


<!--HTML elements allowed in other stuff-->
<xsl:template match="ol"><ol><xsl:apply-templates/></ol></xsl:template>
<xsl:template match="ul"><ul><xsl:apply-templates/></ul></xsl:template>
<xsl:template match="b">[b]<xsl:apply-templates/>[/b]</xsl:template>
<xsl:template match="i">[i]<xsl:apply-templates/>[/i]</xsl:template>
</xsl:stylesheet>
