<?xml version='1.0'?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:exslt="http://exslt.org/common">
<xsl:output method="text" indent="no"/>
<xsl:preserve-space elements="xsl:text"/>

<xsl:template match="/widgets">
select (@oldid := id) from topic where name = "3. Application Components";
delete from topic where parent_id = @oldid;
delete from topic where id = @oldid;

insert into topic values(null,1,"3. Application Components",null,
"       [b]Centrallix Application Widget Reference[/b]
	
	This guide contains a basic overview and specification of each of the widgets that Centrallix supports, including a handful of planned (but not yet implemented or not fully implemented) widgets.
	
	Each widget can have properties, child properties, events, and actions.  Properties define how the widget behaves, appears, and is constructed. Child properties usually define how a widget contained within the given widget relates to its parent. Such properties are normally managed by the containing (parent) widget rather than by the child, and many times the child widget cannot exist without its parent. Events enable a widget to have an \"influence\" on the other widgets on the page, and are utilized by placing a \"connector\" widget within the given widget.  The connector routes an event's firing to the activation of an \"action\" on some other widget on the page.  Actions are \"methods\" on widgets which cause the widget to do something or change its appearance in some way.
	
	The \"overview\" section for each widget describes the widget's purpose and how it operates.  A \"usage\" section is provided to show the appropriate contexts for the use of the widget in a real application.
	
	Some widget properties are 'dynamic'; that is, an expression can be provided which is then evaluated dynamically on the client so that the widget's property changes as the expression's value changes.
	
	\"Client properties\" are available for some widgets.  These properties may not be specified in the application file itself, but are available on the client during application operation.  They often relate to the status of a given widget.
	
	Where possible, sample code is provided for each widget to show how the widget can be used.  In most cases the widget's code is displayed in isolation, although in practice widgets can never be used outside of a \"widget/page\" container widget at some level. In a few cases, a more complete mini-application is shown.
	
	Sample code is generally given in \"structure file\" format, which is the normal format for the building of applications.  However, other suitable object-structured data formats can be used, including XML.

	Copyright (c)  1998-2010 LightSys Technology Services, Inc.
	
	[b]Documentation on the following widgets is available:[/b]

	[table]
	<xsl:for-each select="widget">
		<xsl:sort select="@name" order="ascending"/>
		<xsl:variable name="widgetname"><xsl:value-of select="@name"/></xsl:variable>
		[tr][td][widget/<xsl:value-of select="@name"/>][/td][td]<xsl:value-of select="@description"/>[/td][/tr]
	</xsl:for-each>
	[/table]
");
select (@newid := @@last_insert_id);

	<xsl:for-each select="widget">
		<xsl:variable name="widgetname"><xsl:value-of select="@name"/></xsl:variable>
insert into topic values(null, @newid, "widget/<xsl:value-of select="@name"/>", null,
"		[b]<xsl:value-of select="@name"/>[/b] :: <xsl:value-of select="@description"/>

		[b]Metadata:[/b]
		[table]
		[tr][td]type:[/td][td]<xsl:value-of select="@type"/>[/td][/tr]
		[tr][td]visual:[/td][td] <xsl:value-of select="@visual"/>[/td][/tr]
		[tr][td]container:[/td][td] <xsl:value-of select="@container"/>[/td][/tr]
		[tr][td]form element:[/td][td] <xsl:value-of select="@formelement"/>[/td][/tr]
		[/table]
		<xsl:apply-templates select="overview"/>
		<xsl:apply-templates select="usage"/>
		<xsl:apply-templates select="properties"/>
		<xsl:apply-templates select="children"/>
		<xsl:apply-templates select="actions"/>
		<xsl:apply-templates select="events"/>
		<xsl:apply-templates select="clientprops"/>
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

<xsl:template match="clientprops">
	[b]Client Properties:[/b]
	<xsl:if test="boolean(count(clientprop))">
		[table]
		[tr][th]Property[/th][th]Type[/th][th]Description[/th][/tr]
		<xsl:for-each select="clientprop">
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

<xsl:template match="actions">
	[b]Actions:[/b]
	<xsl:if test="count(action)">
		[table]
		[tr][th]Action[/th][th]Description[/th][/tr]
		<xsl:for-each select="action">
			<xsl:if test="boolean(count(ni))">
				[tr]
					[td]<xsl:value-of select="@name"/>[/td]
					[td]<xsl:call-template name="str_escape">
						<xsl:with-param name="search" select="'&quot;'"/>
						<xsl:with-param name="escchar" select="'&#092;'"/>
						<xsl:with-param name="subject" select="string()"/>
					</xsl:call-template>[/td]
				[/tr]
			</xsl:if>
			<xsl:if test="not(boolean(count(ni)))">
				[tr]
					[td]<xsl:value-of select="@name"/>[/td]
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
	<xsl:if test="not(count(action))">
		[i]none currently available[/i]
	</xsl:if>
</xsl:template>

<xsl:template match="events">
	[b]Events:[/b]
	<xsl:if test="count(event)">
		[table]
		[tr][th]Event[/th][th]Description[/th][/tr]
		<xsl:for-each select="event">
			<xsl:if test="boolean(count(ni))">
				[tr]
					[td]<xsl:value-of select="@name"/>[/td]
					[td]<xsl:call-template name="str_escape">
						<xsl:with-param name="search" select="'&quot;'"/>
						<xsl:with-param name="escchar" select="'&#092;'"/>
						<xsl:with-param name="subject" select="string()"/>
					</xsl:call-template>[/td]
				[/tr]
			</xsl:if>
			<xsl:if test="not(boolean(count(ni)))">
				[tr]
					[td]<xsl:value-of select="@name"/>[/td]
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
	<xsl:if test="not(count(event))">
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
