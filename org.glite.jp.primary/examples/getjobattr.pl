#!/usr/bin/perl

use SOAP::Lite;
use Data::Dumper;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$ENV{HTTPS_CERT_FILE}="$ENV{HOME}/.globus/usercert.pem";
$ENV{HTTPS_KEY_FILE}="$ENV{HOME}/.globus/userkey.pem";

$proxy = shift;
$job = shift;

die "usage: $0 https://jp.primary.storage.org:8901/jpps https://some.nice.job/id attr attr ...\n"
	unless $ARGV[0];

$c = SOAP::Lite
	-> proxy($proxy)
	-> uri('http://glite.org/wsdl/services/jp');

service $c 'http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/3.1/JobProvenancePS.wsdl' or die "service: $1\n";

ns $c 'http://glite.org/wsdl/elements/jp';

print "WSDL OK\n";

push @attr,SOAP::Data->name(attributes => $_) for (@ARGV);

$req = SOAP::Data->value(
	SOAP::Data->name(jobid => $job),
	@attr
#	SOAP::Data->name(attributes => 'http://egee.cesnet.cz/en/Schema/LB/Attributes:CE'),
#	SOAP::Data->name(attributes => 'http://egee.cesnet.cz/en/Schema/JP/System:owner')
);



on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

$resp = GetJobAttributes $c $req;

print Dumper $resp->body unless $fault;

