#! /usr/bin/perl

#
# 4. query:
# 
# Find all invocations of procedure align_warp using a twelfth order nonlinear
# 1365 parameter model (see model menu describing possible values of parameter
# "-m 12" of align_warp) that ran on a Monday.
#
# call:
#   ./query4.pl 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps=$pch::ps;
my $is=$pch::is;
my %program_names=(align_warp => 1);
my $program_params='-m 12';
my $runday=1;
#my $runday=4;
my @attributes = ("$pch::jpsys:jobId", @pch::view_attributes);

my %according_jobs = (); # hash jobid list
my $according_count = 0;


# debug calls
$pch::debug = 0;
my $debug = 0;

if ($#ARGV + 1 > 1) {
	%program_names = ();
	foreach (split(/  */, $ARGV[1])) {
		$program_names{$_} = 1;
	}
}


#
# find out processes with given name ant parameters
#
my @query_programs = ();
foreach (keys %program_names) {
	my @qitem = ['EQUAL', "<string>$_</string>"];
	push @query_programs, @qitem;
}
my @jobs = pch::isquery($is, [
	["$pch::jplbtag:IPAW_PROGRAM", @query_programs],
	["$pch::jplbtag:IPAW_PARAM", ['EQUAL', "<string>$program_params</string>"]],
], \@attributes);
print Dumper(@jobs) if ($debug);
die "...so exit on error" if ($pch::err);


#
# check found all jobs
#
$according_count = 0;
foreach my $job (@jobs) {
	my %job = %$job;
	my (@time, @timesep, @origin);
	my $itime;

	print "Handling $job{jobid} ($according_count.)\n" if ($debug);

	# search first regtime with origin USER,
	# be satisfied with first regime too if no value has USER origin
	@time = @{$job{attributes}{"$pch::jpsys:regtime"}{value}};
	@origin = @{$job{attributes}{"$pch::jpsys:regtime"}{origin}};
	@timesep = ();
	foreach $itime (0..$#time) {
#print "check ".$time[$itime]." ".$origin[$itime]."\n";
		if ($origin[$itime] eq 'USER') {
			@timesep = gmtime($time[$itime]);
			last;
		}
	}
	if ($#timesep == -1) { @timesep = gmtime($time[0]); }
#print join(',', @timesep)."\n";

	if ($timesep[6] == $runday) {
		if (!exists $according_jobs{$job{jobid}}) {
			$according_jobs{$job{jobid}} = \%job;
			print "Added $job{jobid}\n" if $debug;
		} else {
			print "Already existing $job{jobid}\n" if $debug;
		}
	} else {
		print "Job $job{jobid} ran at day $timesep[6] (0=Sun, ...): ".gmtime($time[0])."\n" if $debug;
	}

	$according_count++;
}
undef @jobs;


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
