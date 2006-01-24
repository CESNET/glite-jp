<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"

	xmlns:jp="http://glite.org/wsdl/services/jp"
	xmlns:jpe="http://glite.org/wsdl/elements/jp"
	xmlns:jpt="http://glite.org/wsdl/types/jp">

<xsl:output indent="yes"/>

<xsl:template match="/service">

		<xsl:apply-templates select="types"/>
		
</xsl:template>

<xsl:template match="types">
		<xsd:schema targetNamespace="{@ns}"
			elementFormDefault="unqualified"
			attributeFormDefault="unqualified">

			<xsl:apply-templates/>
		</xsd:schema>
</xsl:template>

<xsl:template match="enum">
	<xsd:simpleType name="{@name}">
		<xsd:restriction base="xsd:string">
			<xsl:for-each select="val"><xsd:enumeration value="{@name}"/></xsl:for-each>
		</xsd:restriction>
	</xsd:simpleType>
</xsl:template>

<xsl:template match="flags">
	<xsd:simpleType name="{@name}Value">
		<xsd:restriction base="xsd:string">
			<xsl:for-each select="val"><xsd:enumeration value="{@name}"/></xsl:for-each>
		</xsd:restriction>
	</xsd:simpleType>
	<xsd:complexType name="{@name}">
		<xsd:sequence>
			<xsd:element name="flag" type="{/service/@typePrefix}:{@name}Value" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
	</xsd:complexType>
</xsl:template>

<xsl:template match="struct">
	<xsd:complexType name="{@name}">
		<xsd:sequence>
			<xsl:call-template name="inner-struct"/>
		</xsd:sequence>
	</xsd:complexType>
</xsl:template>

<xsl:template match="choice">
	<xsd:complexType name="{@name}">
		<xsd:choice>
			<xsl:call-template name="inner-struct"/>
		</xsd:choice>
	</xsd:complexType>
</xsl:template>


<xsl:template name="inner-struct">
	<xsl:variable name="nillable">
		<xsl:choose>
			<xsl:when test="local-name(.)='choice'">true</xsl:when>
			<xsl:otherwise>false</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
			<xsl:for-each select="elem">
				<xsl:variable name="type">
					<xsl:choose>
						<xsl:when test="contains(@type,':')">
							<xsl:value-of select="@type"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="/service/@typePrefix"/>:<xsl:value-of select="@type"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<xsl:variable name="min">
					<xsl:choose>
						<xsl:when test="@optional='yes'">0</xsl:when>
						<xsl:otherwise>1</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<xsl:variable name="max">
					<xsl:choose>
						<xsl:when test="@list='yes'">unbounded</xsl:when>
						<xsl:otherwise>1</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<xsd:element name="{@name}" type="{$type}" minOccurs="{$min}" maxOccurs="{$max}" nillable="{$nillable}"/>
			</xsl:for-each>
</xsl:template>

<xsl:template match="op" mode="element">
	<xsd:element name="{@name}">
		<xsd:complexType>
			<xsd:sequence>
				<xsl:for-each select="input">
					<xsl:variable name="prefix">
						<xsl:choose>
							<xsl:when test="starts-with(@type,'xsd:')"/>
							<xsl:otherwise><xsl:value-of select="/service/@typePrefix"/>:</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsl:variable name="max">
						<xsl:choose>
							<xsl:when test="@list='yes'">unbounded</xsl:when>
							<xsl:otherwise>1</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsd:element name="{@name}" type="{$prefix}{@type}" minOccurs="1" maxOccurs="{$max}"/>
				</xsl:for-each>
			</xsd:sequence>
		</xsd:complexType>
	</xsd:element>
	<xsd:element name="{@name}Response">
		<xsd:complexType>
			<xsd:sequence>
				<xsl:for-each select="output">
					<xsl:variable name="prefix">
						<xsl:choose>
							<xsl:when test="starts-with(@type,'xsd:')"/>
							<xsl:otherwise><xsl:value-of select="/service/@typePrefix"/>:</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsl:variable name="max">
						<xsl:choose>
							<xsl:when test="@list='yes'">unbounded</xsl:when>
							<xsl:otherwise>1</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsd:element name="{@name}" type="{$prefix}{@type}" minOccurs="1" maxOccurs="{$max}"/>
				</xsl:for-each>
			</xsd:sequence>
		</xsd:complexType>
	</xsd:element>
</xsl:template>


<xsl:template match="operations">
		<xsd:schema targetNamespace="{/service/@elemNs}"
			elementFormDefault="unqualified"
			attributeFormDefault="unqualified">

			<xsl:apply-templates select="op" mode="element"/>

			<xsl:for-each select="/service/fault">
				<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/>
			</xsl:for-each>
		</xsd:schema>

</xsl:template>

</xsl:stylesheet>

