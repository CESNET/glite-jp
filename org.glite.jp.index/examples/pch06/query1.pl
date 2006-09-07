#! /usr/bin/perl

#
# first query implementation
# call:
#   ./query1.pl OUTPUT_FILE_NAME 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps='https://skurut1.cesnet.cz:8901';
my $is='https://skurut1.cesnet.cz:8902';

my @according_jobs = (); # sequencially jobid list
my %according_jobs = (); # hash jobid list
my $according_count = 0;
my $output;


if ($#ARGV + 1 != 1) {
	print STDERR "Usage: $0 OUTPUT_FILE\n";
	exit 1
}
$output = $ARGV[0];

# debug calls
$pch::debug = 1;
my $debug = 0;

#
# find out processes with given output
#
my @jobs = pch::isquery($is, [
	["$pch::jplbtag:IPAW_OUTPUT", ['EQUAL', "<string>$output</string>"]],
], ["$pch::jpsys:jobId", "$pch::jpwf:ancestor"]);
print Dumper(@jobs) if ($debug);
die "...so exit on error" if ($pch::err);

#
# initial set from index server
#
foreach my $job (@jobs) {
	my %job = %$job;
	my %attributes = %{$job{attributes}};

	if (!exists $according_jobs{$job{jobid}}) {
		push @according_jobs, $job{jobid};
		$according_jobs{$job{jobid}} = 1;
	}
}
undef @jobs;


#
# collect all jobids (tree browsing)
#
# better implementation will be: using children attribute on LB:parent
#
$according_count = 0;
foreach my $jobid (@according_jobs) {
	my @attrs;

	print "Handling $jobid (position $according_count)\n" if ($debug);
	@attrs = pch::psquery($ps, $jobid, "$pch::jpwf:ancestor");

	for my $anc_jobid (@attrs) {
		print "Considered: $anc_jobid\n" if ($debug);
		if (!exists $according_jobs{$anc_jobid}) {
			$according_jobs{$anc_jobid} = 1;
			push @according_jobs, $anc_jobid;
			print "Added $anc_jobid to $#according_jobs\n" if ($debug);
		}
		else {
			print "Already existing $anc_jobid\n" if ($debug);
		}
	}
	$according_count++;
}


#
# queries on result set
#
print "Results\n";
print "=======\n";
print "\n";
foreach my $jobid (keys %according_jobs) {
	print "jobid $jobid:\n";

	# query & output all desired atributes
	foreach my $attr ("$pch::jplbtag:IPAW_STAGE", "$pch::jplbtag:IPAW_PROGRAM", "$pch::jplbtag:IPAW_PARAM", "$pch::jplbtag:IPAW_INPUT", "$pch::jplbtag:IPAW_OUTPUT", "$pch::lbattr:CE", "$pch::lbattr:lastStatusHistory") {
		my @attrs;
		my $attr_name = $attr; $attr_name =~ s/.*://;

		@attrs = pch::psquery($ps, $jobid, $attr);
		print "  attr $attr_name: "; print join(", ", @attrs); print "\n";
	}

	print "\n";
}
