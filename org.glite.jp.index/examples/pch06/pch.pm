#
# Job Provenance queries wrapper (Primary and Index queries)
#
# $debug - trace calls
# $err - error status from last query
#

package pch;

use strict;
use warnings;
use XML::Twig;
use Data::Dumper;

our $lbattr='http://egee.cesnet.cz/en/Schema/LB/Attributes';
our $jpsys='http://egee.cesnet.cz/en/Schema/JP/System';
our $jpwf='http://egee.cesnet.cz/en/Schema/JP/Workflow';
our $jplbtag='http://egee.cesnet.cz/en/WSDL/jp-lbtag';

our @view_attributes=("$pch::jplbtag:IPAW_STAGE", "$pch::jplbtag:IPAW_PROGRAM", "$pch::jplbtag:IPAW_PARAM", "$pch::jplbtag:IPAW_INPUT", "$pch::jplbtag:IPAW_OUTPUT", "$pch::lbattr:CE", "$pch::lbattr:parent", "$pch::lbattr:host", "$pch::jpsys:regtime");


our $debug = 0;
our $err = 0;

my $jpis_client_program = "./glite-jpis-client";
my $jpps_client_program = "./glite-jp-primary-test";
my @default_is_attributes = (
	"http://egee.cesnet.cz/en/Schema/JP/System:owner",
	"http://egee.cesnet.cz/en/Schema/JP/System:jobId",
	"http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus",
	"http://egee.cesnet.cz/en/Schema/LB/Attributes:user",
	"http://egee.cesnet.cz/en/WSDL/jp-lbtag:IPAW_PROGRAM",
	"http://egee.cesnet.cz/en/Schema/JP/Workflow:ancestor"
);
my @isquery = (
'<?xml version="1.0" encoding="UTF-8"?>
<jpelem:QueryJobs xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:jptype="http://glite.org/wsdl/types/jp" xmlns:jpsrv="http://glite.org/wsdl/services/jp" xmlns:jpelem="http://glite.org/wsdl/elements/jp">
',
'</jpelem:QueryJobs>
'
);


my @jobs;


#
# query to Job Provenance Index Server
#
sub isquery {
	my ($server, $queries, $attributes, $origin) = @_;
	my ($s, @jobs);
	my $args = '';
	my @attributes;
	my $fh;

	$err = 0;
	if ($attributes) { @attributes = @$attributes; }
	else { @attributes = @default_is_attributes; }

	$s = $isquery[0];
	foreach my $query (@$queries) {
		my @query = @$query;
		my $i = 1;
		$s .= "<conditions>\n";
		$s .= "\t<attr>$query[0]</attr>\n";
		while ($i <= $#query) {
			my @record = @{$query[$i]};
			$s .= "\t<origin>$origin</origin>\n" if $origin;
			$s .= "\t<record>\n";
			$s .= "\t\t<op>$record[0]</op>\n";
			$s .= "\t\t<value>$record[1]</value>\n";
			$s .= "\t\t<value2>$record[2]</value2>\n" if ($record[2]);
			$s .= "\t</record>\n";
			$i++;
		}
		$s .= "</conditions>\n";
	}

	foreach my $attribute (@attributes) {
		$s .= "<attributes>$attribute</attributes>\n";
	}
	$s .= $isquery[1];

	$args .= "-i $server " if ($server);
	$args .= '-q -';

	if ($debug) {
		print STDERR "calling 'echo '$s' | $jpis_client_program $args |'\n";
	}
	if (!open($fh, "echo '$s' | $jpis_client_program $args |")) {
		print STDERR "Can't execute '$jpis_client_program $args'\n";
		$err = 1;
		return ();
	}
	@jobs = parse_is($fh);
#	print STDERR <$fh>; print STDERR "\n";
	close $fh;
	if ($?) {
		print STDERR "Error returned from $jpis_client_program $args\n";
		$err = 1;
		return ();
	}

	return @jobs;
}


sub parse_is {
	my ($fh) = @_;
	my $twig;

	@jobs = ();
	
	$twig = new XML::Twig(TwigHandlers => { jobs => \&jobs_handler });
	if (!$twig->safe_parse($fh)) { $err = 1; return (); }
	else { return @jobs; }
}


sub jobs_handler {
	my($twig, $xmljobs)= @_;
	my (%attributes, $xmljobid, $xmlattribute, %job);
	%attributes = ();

	$xmljobid = $xmljobs->first_child('jobid');
	die "No jobid on '".$xmljobs->text."'" if (!$xmljobid);
	$job{jobid} = $xmljobid->text;

	$xmlattribute = $xmljobs->first_child('attributes');
	while ($xmlattribute) {
		my ($xmlname, $xmlvalue);
		my @values = ();
		my %attribute = ();

		$xmlname = $xmlattribute->first_child('name');
		die "No name on '".$xmlattribute->text."'" if (!$xmlname);
#print $xmljobid->text.": ".$xmlname->text.":\n";
		if (exists $attributes{$xmlname->text}) {
			%attribute = %{$attributes{$xmlname->text}};
		}
#print "  prev attr: ".Dumper(%attribute)."\n";
		if (exists $attribute{value}) {
			@values = @{$attribute{value}};
		}
#print "  prev values: ".Dumper(@values)."\n";
		$xmlvalue = $xmlattribute->first_child('value');
		while ($xmlvalue) {
#print "  to add: ".$xmlvalue->text."\n";
			push @values, $xmlvalue->text;
			$xmlvalue = $xmlvalue->next_sibling('value');
		}
		@{$attribute{value}} = @values;
#print "  new values: ".Dumper($attribute{value})."\n";
		$attribute{timestamp} = $xmlattribute->first_child('timestamp')->text;
		$xmlattribute = $xmlattribute->next_sibling('attributes');

		$attributes{$xmlname->text} = \%attribute;
	}
	$job{attributes} = \%attributes;

	push @jobs, \%job;
}

#
# query to Job Provenance Primary Storage
# ==> array of string
#
sub psquery {
	my ($server, $jobid, $attribute) = @_;
	my $args = '';
	my @attrs = ();
	my $fh;

	$err = 0;
	$args .= "-s $server " if ($server);
	$args .= "GetJobAttr $jobid $attribute";
	if ($debug) {
		print STDERR "calling '$jpps_client_program $args |'\n";
	}
	if (!open($fh, "$jpps_client_program $args |")) {
		print STDERR "Can't execute '$jpps_client_program $args'\n";
		$err = 1;
		return ();
	}
	@attrs = parse_ps($fh);
	close $fh;
	if ($?) {
		print STDERR "Error returned from $jpps_client_program $args\n";
		$err = 1;
		return ();
	}

	return @attrs;
}


sub parse_ps {
	my ($fh) = @_;
	my @attrs = ();
	my $attr;

	while (<$fh>) {
		chomp;
		next if (!$_);
		next if (/^OK$/);
		next if (/^Attribute values:$/);
#		print STDERR "$_\n";
		$attr = $_;
		$attr =~ s/\t*//;
		$attr =~ s/\t.*//;
		push @attrs, $attr;
	}

	return @attrs;
}


1;
