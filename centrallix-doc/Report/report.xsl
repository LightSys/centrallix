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
	<p>Sample code is generally given in "structure file" format, which is the normal format for the building of applications.  However, other suitable object-structured data formats can be used, including XML.</p>
	<p>Copyright (c)  1998-2003 LightSys Technology Services, Inc.</p>
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

</xsl:stylesheet>
