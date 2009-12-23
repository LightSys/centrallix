<?xml version='1.0'?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" indent="yes"/>
<xsl:preserve-space elements="pre,comment"/>
<xsl:strip-space elements="p"/>

<xsl:template match="/components">
	<html>
	<head>
	<title>Centrallix Application Server: Report Documentation</title>
	<link rel='StyleSheet' href='style.css' type='text/css'/>
	</head>
	<body>
	<xsl:comment> PAGE BREAK: 'toc.html' </xsl:comment>
	<xsl:text>
</xsl:text>
	<h1>Centrallix<br/>Reporting System<br/>Reference</h1>
	<br/>
	<p>The Centrallix reporting system provides the ability to generate reports in a variety of formats from ObjectSystem data and a report object.  This document describes the report object structure in detail and how to use the various report components to build a report.</p>
	<p>Reports can have parameters - see the 'report' object and 'report/parameter' object for more details.  Parameters are passed to a report via HTTP URL parameters, and are defined in the report by 'report/parameter' objects (or, though deprecated, by simple top level attributes other than those already defined for 'system/report').</p>
	<p>Sample code is generally given in "structure file" format, which is the normal format for the building of applications.  However, other suitable object-structured data formats can be used, including XML.</p>
	<p>Where a 'moneyformat' is specified, it is a text string in which the following special characters are recognized (the default money format is "$0.00"):</p>
	<table>
	    <tr><td><tt>#</tt></td><td> - Optional digit unless after the decimal point or the digit immediately before the decimal point.</td></tr>
	    <tr><td><tt>0</tt></td><td> - Mandatory digit, no leading zero suppression.</td></tr>
	    <tr><td><tt>,</tt></td><td> - Insert a comma, suppressed if no digits around it.</td></tr>
	    <tr><td><tt>.</tt></td><td> - Decimal point (only one allowed).</td></tr>
	    <tr><td><tt>$</tt></td><td> - Dollar sign.</td></tr>
	    <tr><td><tt>+</tt></td><td> - Sign, + if positive or zero, - if negative.</td></tr>
	    <tr><td><tt>-</tt></td><td> - Sign, blank if positive or zero, - if negative.</td></tr>
	    <tr><td><tt>()</tt></td><td> - Surround number with () if it is negative.</td></tr>
	    <tr><td><tt>[]</tt></td><td> - Surround number with () if it is positive or zero.</td></tr>
	    <tr><td><tt> </tt></td><td> - (space) optional digit, but put a space in its place if suppressing leading 0's.</td></tr>
	</table>
	<p>Where a 'dateformat' is specified, it is a text string with the following character sequences recognized (default is "dd MMM yyyy HH:mm"):</p>
	<table>
	    <tr><td><tt>dd</tt></td><td> - Two digit day of month (01 through 31).</td></tr>
	    <tr><td><tt>ddd</tt></td><td> - Day of month plus cardinality (1st through 31st).</td></tr>
	    <tr><td><tt>MMMM</tt></td><td> - Full (long) month name (January through December).</td></tr>
	    <tr><td><tt>MMM</tt></td><td> - Three-letter month abbreviation (Jan through Dec).</td></tr>
	    <tr><td><tt>MM</tt></td><td> - Two digit month (01 through 12).</td></tr>
	    <tr><td><tt>yy</tt></td><td> - Two digit year (00 through 99).</td></tr>
	    <tr><td><tt>yyyy</tt></td><td> - Four digit year (0000 through 9999).</td></tr>
	    <tr><td><tt>HH</tt></td><td> - Hour in 24-hour format (00 through 23).</td></tr>
	    <tr><td><tt>hh</tt></td><td> - Hour in 12-hour format (00 AM through 11 PM).</td></tr>
	    <tr><td><tt>mm</tt></td><td> - Minutes (00 through 59).</td></tr>
	    <tr><td><tt>ss</tt></td><td> - Seconds (00 through 59).</td></tr>
	    <tr><td><tt>I</tt></td><td> - At the beginning of the format, indicates that the date is in international (dd/mm/yy) order rather than U.S. (mm/dd/yy) order.  Used mainly when a date is being input rather than when it is being generated.</td></tr>
	</table>
	<p>Copyright (c)  1998-2009 LightSys Technology Services, Inc.</p>
	<br/>
	<p><b>Documentation on the following report components is available:</b></p>
	<table>
	<xsl:for-each select="component">
		<xsl:variable name="componentname"><xsl:value-of select="@name"/></xsl:variable>
		<tr><td><a href="#{$componentname}"><xsl:value-of select="@name"/></a></td><td><xsl:value-of select="@description"/></td></tr>
	</xsl:for-each>
	</table>

	<br/><br/>
	<xsl:text>
</xsl:text>

	<xsl:for-each select="component">
		<!-- PAGE BREAK: '.html' -->
		<xsl:comment> PAGE BREAK: '<xsl:value-of select="@name"/>.html' </xsl:comment>
		<xsl:text>
</xsl:text>
		<xsl:variable name="componentname"><xsl:value-of select="@name"/></xsl:variable>
		<h2 class="componentname"><a name="{$componentname}"><xsl:value-of select="@name"/></a></h2>
		<p class="componenttype">type: <span class="componenttypevalue"><xsl:value-of select="@type"/></span></p>
		<p class="componenttype">visual: <span class="componenttypevalue"><xsl:value-of select="@visual"/></span></p>
		<p class="componenttype">container: <span class="componenttypevalue"><xsl:value-of select="@container"/></span></p>
		<xsl:apply-templates select="overview"/>
		<xsl:apply-templates select="usage"/>
		<xsl:apply-templates select="properties"/>
		<xsl:apply-templates select="children"/>
		<xsl:apply-templates select="sample"/>
	</xsl:for-each>
	<xsl:comment> PAGE BREAK: 'END' </xsl:comment>
	<xsl:text>
</xsl:text>
	</body>
	</html>
</xsl:template>

<xsl:template match="overview">
	<h3 class="overview">Overview:</h3>
	<span class="overview"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="usage">
	<h3 class="usage">Usage:</h3>
	<span class="usage"><xsl:apply-templates/></span>
</xsl:template>


<xsl:template match="properties">
	<h3 class="properties">Properties:</h3>
	<xsl:if test="boolean(count(property))">
		<table class="properties">
		<tr><th class="name">Property</th><th class="type">Type</th><th class="description">Description</th></tr>
		<xsl:for-each select="property">
			<xsl:if test="boolean(count(ni))">
				<tr class="notimplimented">
					<td class="name"><xsl:value-of select="@name"/></td>
					<td class="type"><xsl:value-of select="@type"/></td>
					<td class="description"><xsl:apply-templates /></td>
				</tr>
			</xsl:if>
			<xsl:if test="not(count(ni))">
				<tr>
					<td class="name"><xsl:value-of select="@name"/></td>
					<td class="type"><xsl:value-of select="@type"/></td>
					<td class="description"><xsl:apply-templates /></td>
				</tr>
			</xsl:if>
		</xsl:for-each>
		</table>
	</xsl:if>
	<xsl:if test="not(count(property))">
		<p class="none">none currently available</p>
	</xsl:if>
</xsl:template>

<xsl:template match="children">
	<h3 class="childproperties">Child Properties:</h3>
	<xsl:if test="count(child)">
		<xsl:for-each select="child">
			<p class="childname">(of <xsl:value-of select="@type"/> child components)</p>
			<xsl:if test="count(childproperty)">
				<table class="childproperties">
				<tr><th class="name">Property</th><th class="type">Type</th><th class="description">Description</th></tr>
				<xsl:for-each select="childproperty">
					<xsl:if test="boolean(count(ni))">
						<tr class="notimplimented">
							<td class="name"><xsl:value-of select="@name"/></td>
							<td class="type"><xsl:value-of select="@type"/></td>
							<td class="description"><xsl:apply-templates /></td>
						</tr>
					</xsl:if>
					<xsl:if test="not(count(ni))">
						<tr>
							<td class="name"><xsl:value-of select="@name"/></td>
							<td class="type"><xsl:value-of select="@type"/></td>
							<td class="description"><xsl:apply-templates /></td>
						</tr>
					</xsl:if>
				</xsl:for-each>
				</table>
			</xsl:if>
			<xsl:if test="not(count(childproperty))">
				<p class="none">none currently available</p>
			</xsl:if>
		</xsl:for-each>
	</xsl:if>
	<xsl:if test="not(count(child))">
		<p class="none">none currently available</p>
	</xsl:if>
</xsl:template>

<xsl:template match="sample">
	<h3 class="sample">Sample Code:</h3>
	<xsl:if test="string-length(normalize-space(string()))">
		<pre>
		<xsl:apply-templates />
		</pre>
	</xsl:if>
	<xsl:if test="not(string-length(normalize-space(string())))">
		<p class="none">none currently available</p>
	</xsl:if>
</xsl:template>


<!--HTML elements allowed in other stuff-->
<xsl:template match="p"><p><xsl:apply-templates/></p></xsl:template>
<xsl:template match="ol"><ol><xsl:apply-templates/></ol></xsl:template>
<xsl:template match="ul"><ul><xsl:apply-templates/></ul></xsl:template>
<xsl:template match="li"><li><xsl:apply-templates/></li></xsl:template>
<xsl:template match="b"><b><xsl:apply-templates/></b></xsl:template>
<xsl:template match="i"><i><xsl:apply-templates/></i></xsl:template>
<xsl:template match="table"><table><xsl:apply-templates/></table></xsl:template>
<xsl:template match="tr"><tr><xsl:apply-templates/></tr></xsl:template>
<xsl:template match="td"><td><xsl:apply-templates/></td></xsl:template>
<xsl:template match="th"><th><xsl:apply-templates/></th></xsl:template>

</xsl:stylesheet>
