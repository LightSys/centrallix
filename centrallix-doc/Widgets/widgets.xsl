<?xml version='1.0'?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" indent="yes"/>
<xsl:preserve-space elements="pre,comment"/>
<xsl:strip-space elements="p"/>

<xsl:template match="/widgets">
	<html>
	<head>
	<title>Centrallix Application Server: Widget Documentation</title>
	<link rel='StyleSheet' href='style.css' type='text/css'/>
	</head>
	<body>
	<xsl:comment> PAGE BREAK: 'toc.html' </xsl:comment>
	<xsl:text>
</xsl:text>
	<h1>Centrallix Application<br/>Widget<br/>Reference</h1>
	<br/>
	<p>This guide contains a basic overview and specification of each of the widgets that Centrallix supports, including a handful of planned (but not yet implemented or not fully implemented) widgets.</p>
	<p>Each widget can have properties, child properties, events, and actions.  Properties define how the widget behaves, appears, and is constructed. Child properties usually define how a widget contained within the given widget relates to its parent. Such properties are normally managed by the containing (parent) widget rather than by the child, and many times the child widget cannot exist without its parent. Events enable a widget to have an "influence" on the other widgets on the page, and are utilized by placing a "connector" widget within the given widget.  The connector routes an event's firing to the activation of an "action" on some other widget on the page.  Actions are "methods" on widgets which cause the widget to do something or change its appearance in some way.</p>
	<p>The "overview" section for each widget describes the widget's purpose and how it operates.  A "usage" section is provided to show the appropriate contexts for the use of the widget in a real application.</p>
	<p>Some widget properties are 'dynamic'; that is, an expression can be provided which is then evaluated dynamically on the client so that the widget's property changes as the expression's value changes.</p>
	<p>"Client properties" are available for some widgets.  These properties may not be specified in the application file itself, but are available on the client during application operation.  They often relate to the status of a given widget.</p>
	<p>Where possible, sample code is provided for each widget to show how the widget can be used.  In most cases the widget's code is displayed in isolation, although in practice widgets can never be used outside of a "widget/page" container widget at some level. In a few cases, a more complete mini-application is shown.</p>
	<p>Sample code is generally given in "structure file" format, which is the normal format for the building of applications.  However, other suitable object-structured data formats can be used, including XML.</p>
	<p>Copyright (c)  1998-2024 LightSys Technology Services, Inc.</p>
	<br/>
	<p><b>Documentation on the following widgets is available:</b></p>
	<table>
	<xsl:for-each select="widget">
		<xsl:variable name="widgetname"><xsl:value-of select="@name"/></xsl:variable>
		<tr><td><a href="#{$widgetname}"><xsl:value-of select="@name"/></a></td><td><xsl:value-of select="@description"/></td></tr>
	</xsl:for-each>
	</table>

	<br/><br/>
	<xsl:text>
</xsl:text>

	<xsl:for-each select="widget">
		<!-- PAGE BREAK: '.html' -->
		<xsl:comment> PAGE BREAK: '<xsl:value-of select="@name"/>.html' </xsl:comment>
		<xsl:text>
</xsl:text>
		<xsl:variable name="widgetname"><xsl:value-of select="@name"/></xsl:variable>
		<h2 class="widgetname"><a name="{$widgetname}"><xsl:value-of select="@name"/></a></h2>
		<p class="widgettype">type: <span class="widgettypevalue"><xsl:value-of select="@type"/></span></p>
		<p class="widgettype">visual: <span class="widgettypevalue"><xsl:value-of select="@visual"/></span></p>
		<p class="widgettype">container: <span class="widgettypevalue"><xsl:value-of select="@container"/></span></p>
		<p class="widgettype">form element: <span class="widgettypevalue"><xsl:value-of select="@formelement"/></span></p>
		<xsl:apply-templates select="overview"/>
		<xsl:apply-templates select="usage"/>
		<xsl:apply-templates select="properties"/>
		<xsl:apply-templates select="children"/>
		<xsl:apply-templates select="actions"/>
		<xsl:apply-templates select="events"/>
		<xsl:apply-templates select="clientprops"/>
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

<xsl:template match="clientprops">
	<h3 class="properties">Client Properties:</h3>
	<xsl:if test="boolean(count(clientprop))">
		<table class="properties">
		<tr><th class="name">Property</th><th class="type">Type</th><th class="description">Description</th></tr>
		<xsl:for-each select="clientprop">
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
			<p class="childname">(of <xsl:value-of select="@type"/> child widgets)</p>
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

<xsl:template match="actions">
	<h3 class="actions">Actions:</h3>
	<xsl:if test="count(action)">
		<table class="actions">
		<tr><th class="name">Action</th><th class="description">Description</th></tr>
		<xsl:for-each select="action">
			<xsl:if test="boolean(count(ni))">
				<tr class="notimplimented">
					<td class="name"><xsl:value-of select="@name"/></td>
					<td class="description"><xsl:apply-templates /></td>
				</tr>
			</xsl:if>
			<xsl:if test="not(boolean(count(ni)))">
				<tr>
					<td class="name"><xsl:value-of select="@name"/></td>
					<td class="description"><xsl:apply-templates /></td>
				</tr>
			</xsl:if>
		</xsl:for-each>
		</table>
	</xsl:if>
	<xsl:if test="not(count(action))">
		<p class="none">none currently available</p>
	</xsl:if>
</xsl:template>

<xsl:template match="events">
	<h3 class="events">Events:</h3>
	<xsl:if test="count(event)">
		<table class="events">
		<tr><th class="name">Event</th><th class="description">Description</th></tr>
		<xsl:for-each select="event">
			<xsl:if test="boolean(count(ni))">
				<tr class="notimplimented">
					<td class="name"><xsl:value-of select="@name"/></td>
					<td class="description"><xsl:apply-templates /></td>
				</tr>
			</xsl:if>
			<xsl:if test="not(boolean(count(ni)))">
				<tr>
					<td class="name"><xsl:value-of select="@name"/></td>
					<td class="description"><xsl:apply-templates /></td>
				</tr>
			</xsl:if>
		</xsl:for-each>
		</table>
	</xsl:if>
	<xsl:if test="not(count(event))">
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
