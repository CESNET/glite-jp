#! /usr/bin/perl

#
# 4. query:
# 
# Find all invocations of procedure align_warp using a twelfth order nonlinear
# 1365 parameter model (see model menu describing possible values of parameter
# "-m 12" of align_warp) that ran on a Monday.
#
# call:
#   ./query4.pl OUTPUT_FILE_NAME 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps='https://skurut1.cesnet.cz:8901';
my $is='https://skurut1.cesnet.cz:8902';
my $program_name='align_warp';
my $program_params='-m 12';
my $runday=1;
my @view_attributes = ("$pch::jplbtag:IPAW_STAGE", "$pch::jplbtag:IPAW_PROGRAM", "$pch::jplbtag:IPAW_PARAM", "$pch::jplbtag:IPAW_INPUT", "$pch::jplbtag:IPAW_OUTPUT", "$pch::lbattr:CE");
my @attributes = ("$pch::jpsys:jobId", "$pch::jpwf:ancestor", @view_attributes);

my @according_jobs = (); # sequencially jobid list
my %according_jobs = (); # hash jobid list
my $according_count = 0;


# debug calls
$pch::debug = 0;
my $debug = 0;

#
# find out processes with given name ant parameters
#
my @jobs = pch::isquery($is, [
	["$pch::jplbtag:IPAW_PROGRAM", ['EQUAL', "<string>$program_name</string>"]],
	["$pch::jplbtag:IPAW_PARAM", ['EQUAL', "<string>$program_params</string>"]],
], \@attributes);
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
		$according_jobs{$job{jobid}} = \%job;
	}
}
undef @jobs;


#
# collect all jobs (tree browsing)
#
$according_count = 0;
foreach my $jobid (@according_jobs) {
	my @ancs;

	print "Handling $jobid (position $according_count)\n" if ($debug);
	@ancs = pch::isquery($is, [["$pch::jpwf:successor", ['EQUAL', "<string>$jobid</string>"]]], \@attributes);
	die "...so exit on error" if ($pch::err);

	for my $anc (@ancs) {
		my %anc = %$anc;
		print "Considered: $anc{jobid}\n" if ($debug);
		if (!exists $according_jobs{$anc{jobid}}) {
			$according_jobs{$anc{jobid}} = \%anc;
			push @according_jobs, $anc{jobid};
			print "Added $anc{jobid} to $#according_jobs\n" if ($debug);
		}
		else {
			print "Already existing $anc{jobid}\n" if ($debug);
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
foreach my $jobid (sort { $according_jobs{$b}{attributes}{"$pch::jplbtag:IPAW_STAGE"}{value}[0] <=> $according_jobs{$a}{attributes}{"$pch::jplbtag:IPAW_STAGE"}{value}[0] } keys %according_jobs) {
	my %job = %{$according_jobs{$jobid}};
	my %attributes = %{$job{attributes}};

	my @time = pch::psquery($ps, $jobid, "$pch::jpsys:regtime");
	my @timesep = gmtime($time[0]);
	if ($timesep[6] == $runday) {
		print "jobid $jobid:\n";

		# query & output all desired atributes
		foreach my $attr (@view_attributes) {
			my $attr_name = $attr; $attr_name =~ s/.*://;

			print "  attr $attr_name: ";
			if (exists $attributes{$attr}) {
				my %attr = %{$attributes{$attr}};

				print join(", ", @{$attr{value}}); print "\n";
			} else {
				print "N/A\n";
			}
		}
		print "  attr REGTIME: ".gmtime($time[0])." (".join(", ", @time).")\n";

		print "\n";
	} else {
		print "Job $jobid ran at day $timesep[6] (0=Sun, ...): ".gmtime($time[0])."\n" if $debug;
	}
}
