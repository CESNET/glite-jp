#! /usr/bin/perl

#
# 6. query:
#
# Find all output averaged images of softmean (average) procedures, where the
# warped images taken as input were align_warped using a twelfth order
# nonlinear 1365 parameter model, i.e. "where softmean was preceded in the
# workflow, directly or indirectly, by an align_warp procedure with argument
# -m 12. 
#
# call:
#   ./query6.pl [PROGRAMS] [END_PROGRAMS] 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps=$pch::ps;
my $is=$pch::is;
my %program_names=(align_warp=>1);
my $program_param='-m 12';
my %end_program_names=(softmean=>1);

#my %jobs = ();          # just information cache
my @workflow_jobs = (); # sequencially jobid list
my %workflow_jobs = (); # hash jobid list
my @according_jobs = (); # sequencially jobid list
my %according_jobs = (); # hash jobid list
my $workflow_count = 0;


# debug calls
$pch::debug = 0;
my $debug = 0;

if ($#ARGV + 1 >= 1) {
	%program_names = ();
	foreach (split(/  */, $ARGV[0])) {
        	$program_names{$_} = 1;
	}
}
if ($#ARGV + 1 >= 2) {
	%end_program_names = ();
	foreach (split(/  */, $ARGV[1])) {
        	$end_program_names{$_} = 1;
	}
}

#
# find out processes with given name and parameters
#
my @query_programs = ();
foreach (keys %program_names) {
	my @qitem = ['EQUAL', "<string>$_</string>"];
	push @query_programs, @qitem;
}
my @jobs = pch::isquery($is, [
	["$pch::jplbtag:IPAW_PROGRAM", @query_programs],
	["$pch::jplbtag:IPAW_PARAM", ['EQUAL', "<string>$program_param</string>"]],
], ["$pch::jpwf:successor", @pch::view_attributes]);
print Dumper(@jobs) if ($debug);
die "...so exit on error" if ($pch::err);

#
# initial set of starting jobs from index server
# (root jobs)
#
foreach my $job (@jobs) {
	my %job = %$job;
	my %attributes = %{$job{attributes}};
	my $jobid = $job{jobid};

	if (!exists $workflow_jobs{$jobid}) {
		push @workflow_jobs, $jobid;
		$workflow_jobs{$jobid} = \%job;
	}
}
undef @jobs;


#
# collect all jobs (tree browsing down)
#
$workflow_count = 0;
foreach my $jobid (@workflow_jobs) {
	my @succs;
	my $pname;

	print "Handling $jobid (position $workflow_count)\n" if ($debug);
	print "  progname: ".$workflow_jobs{$jobid}{attributes}{"$pch::jplbtag:IPAW_PROGRAM"}{value}[0]."\n" if ($debug);

	$pname = $workflow_jobs{$jobid}{attributes}{"$pch::jplbtag:IPAW_PROGRAM"}{value}[0];
	if (exists $end_program_names{$pname}) {
		print "It's $pname, adding\n" if $debug;
		$according_jobs{$jobid} = \%{$workflow_jobs{$jobid}};
		next;
	}

	@succs = pch::isquery($is, [["$pch::jpwf:ancestor", ['EQUAL', "<string>$jobid</string>"]]], \@pch::view_attributes);
	die "...so exit on error" if ($pch::err);

	for my $succ (@succs) {
		my %succ = %$succ;
		print "Considered: $succ{jobid}\n" if ($debug);
		if (!exists $workflow_jobs{$succ{jobid}}) {
			$workflow_jobs{$succ{jobid}} = \%succ;
			push @workflow_jobs, $succ{jobid};
			print "Added $succ{jobid} to $#workflow_jobs\n" if ($debug);
		}
		else {
			print "Already existing $succ{jobid}\n" if ($debug);
		}
	}
	$workflow_count++;
}
undef @workflow_jobs;
undef %workflow_jobs;


#
# print the result set
#
print "Results\n";
print "=======\n";
print "\n";
foreach my $jobid (sort { $according_jobs{$b}{attributes}{"$pch::jplbtag:IPAW_STAGE"}{value}[0] <=> $according_jobs{$a}{attributes}{"$pch::jplbtag:IPAW_STAGE"}{value}[0] } keys %according_jobs) {
	my %job = %{$according_jobs{$jobid}};
	my %attributes = %{$job{attributes}};

	print "jobid $jobid:\n";

	# output all desired atributes
	foreach my $attr (@pch::view_attributes) {
		my $attr_name = $attr; $attr_name =~ s/.*://;

		print "  attr $attr_name: ";
		if (exists $attributes{$attr}) {
			my %attr = %{$attributes{$attr}};

			if ($attr eq "$pch::jpsys:regtime") {
				print gmtime($attr{value}[0])." (".join(", ", @{$attr{value}}).")\n";
			} else {
				print join(", ", @{$attr{value}})."\n";
			}
		} else {
			print "N/A\n";
		}
	}

	print "\n";
}
