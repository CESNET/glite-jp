<service name="JobProvenance"
	ns="http://glite.org/wsdl/services/jp"
	prefix="jp"
	typeNs="http://glite.org/wsdl/types/jp"
	typePrefix="jpt"
	elemNs="http://glite.org/wsdl/elements/jp"
	elemPrefix="jpe">

	<version>CVS revision: <![CDATA[ $Header$: ]]></version>

	<import namespace="http://glite.org/wsdl/services/jp" location="JobProvenanceTypes.wsdl"/>

	<doc>
${DOC}
	</doc>

	<fault name="genericFault"/>

	<operations>
${OPERATIONS}
	</operations>
</service>
