<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
	xmlns="http://schemas.xmlsoap.org/wsdl/"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"
	xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
	xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"

	xmlns:jp="http://glite.org/wsdl/services/jp"
	xmlns:jpe="http://glite.org/wsdl/elements/jp"
	xmlns:jpt="http://glite.org/wsdl/types/jp">

<xsl:output indent="yes"/>

<xsl:template match="/service">
	<definitions
		xmlns="http://schemas.xmlsoap.org/wsdl/"
		name="{@name}"
		targetNamespace="{@ns}">
  	<documentation> <xsl:value-of select="text()"/> </documentation>

		<xsl:apply-templates select="import"/>

		<xsl:apply-templates select="types"/>
		
		<xsl:apply-templates select="fault"/>

		<xsl:apply-templates select="operations"/>
		
	</definitions>
</xsl:template>

<xsl:template match="types">
	<wsdl:types>
		<schema targetNamespace="{@ns}"
			xmlns="http://www.w3.org/2001/XMLSchema"
			elementFormDefault="unqualified"
			attributeFormDefault="unqualified">

			<xsl:apply-templates/>
		</schema>
	</wsdl:types>
	<!-- <xsl:apply-templates select="struct[@fault='yes']" mode="message"/> -->
</xsl:template>

<!--
<xsl:template match="simple">
	<xsd:element name="{@name}" type="xsd:{@name}"/>
	<xsd:complexType name="{@name}List">
		<xsd:sequence>
			<xsd:element name="{@name}" type="xsd:{@name}" minOccurs="0" maxOccurs="unbounded"></xsd:element>
		</xsd:sequence>
	</xsd:complexType>
	<xsd:element name="{@name}List" type="{/service/@typePrefix}:{@name}List"/>
</xsl:template>
-->

<xsl:template match="list">
	<xsd:complexType name="{@name}List">
		<xsd:sequence>
			<xsd:element name="{@name}" type="xsd:{@name}" minOccurs="0" maxOccurs="unbounded"></xsd:element>
		</xsd:sequence>
	</xsd:complexType>
</xsl:template>


<xsl:template match="enum">
	<xsd:simpleType name="{@name}">
		<xsd:restriction base="xsd:string">
			<xsl:for-each select="val"><xsd:enumeration value="{@name}"/></xsl:for-each>
		</xsd:restriction>
	</xsd:simpleType>
	<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/>
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
	<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/>
</xsl:template>

<xsl:template match="struct">
	<xsd:complexType name="{@name}">
		<xsd:sequence>
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
				<xsd:element name="{@name}" type="{$type}" minOccurs="{$min}" maxOccurs="{$max}"/>
			</xsl:for-each>
		</xsd:sequence>
	</xsd:complexType>
	<xsd:complexType name="{@name}List">
		<xsd:sequence>
			<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}" minOccurs="0" maxOccurs="unbounded"></xsd:element>
		</xsd:sequence>
	</xsd:complexType>
	<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/>
	<xsd:element name="{@name}List" type="{/service/@typePrefix}:{@name}List"/>
</xsl:template>

<xsl:template match="op" mode="message">
	<wsdl:message name="{@name}Request">
		<xsl:for-each select="input">
<!--
			<xsl:variable name="suffix">
				<xsl:choose>
					<xsl:when test="@list='yes'">List</xsl:when>
				</xsl:choose>
			</xsl:variable>
-->
			<wsdl:part name="{@name}" element="{/service/@elemPrefix}:{@name}{../@name}">
				<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
			</wsdl:part>
		</xsl:for-each>
	</wsdl:message>
	<wsdl:message name="{@name}Response">
		<xsl:for-each select="output">
<!--
			<xsl:variable name="suffix">
				<xsl:choose>
					<xsl:when test="@list='yes'">List</xsl:when>
				</xsl:choose>
			</xsl:variable>
-->
			<wsdl:part name="{@name}" element="{/service/@elemPrefix}:{@name}{../@name}">
				<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
			</wsdl:part>
		</xsl:for-each>
	</wsdl:message>
</xsl:template>

<xsl:template match="op" mode="element">
			<xsl:for-each select="input|output">
				<xsl:variable name="prefix">
					<xsl:choose>
						<xsl:when test="starts-with(@type,'xsd:')"/>
						<xsl:otherwise><xsl:value-of select="/service/@typePrefix"/>:</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<xsl:variable name="suffix">
					<xsl:choose>
						<xsl:when test="@list='yes'">List</xsl:when>
					</xsl:choose>
				</xsl:variable>
				<xsd:element name="{@name}{../@name}" type="{$prefix}{@type}{$suffix}"/>
			</xsl:for-each>
</xsl:template>

<xsl:template match="struct[@fault='yes']" mode="message">
	<wsdl:message name="{@name}">
		<wsdl:part name="{@name}" element="{/service/@typePrefix}:{@name}">
			<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
		</wsdl:part>
	</wsdl:message>
</xsl:template>

<xsl:template match="op" mode="port-type">
	<wsdl:operation name="{@name}">
		<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
		<wsdl:input name="i" message="{/service/@prefix}:{@name}Request"/>
		<wsdl:output name="o" message="{/service/@prefix}:{@name}Response"/>
		<wsdl:fault name="f" message="{/service/@prefix}:{fault/@name}"/>
	</wsdl:operation>
</xsl:template>

<xsl:template match="op" mode="binding">
	<wsdl:operation name="{@name}">
		<soap:operation style="rpc"/>
		<wsdl:input name="i">
			<soap:body use="literal" namespace="{/service/@ns}"/>
		</wsdl:input>
		<wsdl:output name="o">
			<soap:body use="literal" namespace="{/service/@ns}"/>
		</wsdl:output>
		<wsdl:fault name="f">
			<soap:fault use="literal"/>
		</wsdl:fault>
	</wsdl:operation>
</xsl:template>

<xsl:template match="import">
	<wsdl:import namepace="{@namespace}" location="{@location}"/>
</xsl:template>

<xsl:template match="operations">
	<wsdl:types>
		<schema targetNamespace="{/service/@elemNs}"
			xmlns="http://www.w3.org/2001/XMLSchema"
			elementFormDefault="unqualified"
			attributeFormDefault="unqualified">

			<xsl:apply-templates select="op" mode="element"/>
		</schema>
	</wsdl:types>

		<xsl:apply-templates select="op" mode="message"/>

		<wsdl:portType name="{/service/@name}PortType">
			<xsl:apply-templates select="op" mode="port-type"/>
		</wsdl:portType>

		<binding name="{/service/@name}" type="{/service/@prefix}:{/service/@name}PortType">
			<soap:binding style="rpc" transport="http://schemas.xmlsoap.org/soap/http"/>
			<xsl:apply-templates select="op" mode="binding"/>
		</binding>

		<service name="{/service/@name}">
			<documentation><xsl:value-of select="text()"/></documentation>
			<port name="{/service/@name}" binding="{/service/@prefix}:{/service/@name}">
				<soap:address location="http://test.glite.org/{/service/@prefix}:8080"/>
			</port>

		</service>

</xsl:template>

<xsl:template match="fault">
	<wsdl:message name="{@name}">
		<wsdl:part name="{@name}" element="{/service/@typePrefix}:{@name}" />
	</wsdl:message>
</xsl:template>


</xsl:stylesheet>

