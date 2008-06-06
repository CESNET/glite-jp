#!/usr/bin/perl

use SOAP::Lite;
use Data::Dumper;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$cred = $ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";
$ENV{HTTPS_CERT_FILE}= $ENV{HTTPS_KEY_FILE} = $ENV{HTTPS_CA_FILE} = $cred;

$proxy = shift;

die "usage: $0 https://jp.primary.storage.org:8901/jpps https://some.nice.job/id attr=value ...\n\t\thttps://another.nice.job/id attr=value ...\n"
	unless $ARGV[0];

$c = SOAP::Lite
	-> proxy($proxy)
	-> uri('http://glite.org/wsdl/services/jp');

service $c 'http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/HEAD/JobProvenancePS.wsdl' or die "service: $1\n";

ns $c 'http://glite.org/wsdl/elements/jp';

print "WSDL OK\n";

push @ARGV,'__KONEC__';
$job = shift;
while ($_ = shift) {
	if (! /(.*)=(.*)/) { 
		push @j,SOAP::Data->name(jobs => \SOAP::Data->value(
			SOAP::Data->name(jobid=>$job),
			@a
		));

		break if $_ eq '__KONEC__';

		$job = $_;
		@a = ();
	}
	else {
		$name = $1; $value = $2;
		print "$job: $name = $value\n";

		push @a, SOAP::Data->name(attributes=>\SOAP::Data->value(
			SOAP::Data->name(name=>$name),
			SOAP::Data->name(value=> \SOAP::Data->value(SOAP::Data->name(string=>$value)))
		))
	}
}


$req = SOAP::Data->value(@j);
print Dumper($req);

on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

$resp = RecordMultiTags $c $req;

print Dumper $resp->body unless $fault;

