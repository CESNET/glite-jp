#! /usr/bin/perl
#
# Copyright (c) Members of the EGEE Collaboration. 2004-2010.
# See http://www.eu-egee.org/partners/ for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# 2. query:
#
# Find the process that led to Atlas X Graphic, excluding everything prior to
# the averaging of images with softmean.
#
# call:
#   ./query2.pl OUTPUT_FILE_NAME [SOFTMEAN_PROGRAM] 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps=$pch::ps;
my $is=$pch::is;
my %program_names = (softmean => 1);

my @according_jobs = (); # sequencially jobid list
my %according_jobs = (); # hash jobid list
my $according_count = 0;
my $output;


if ($#ARGV + 1 < 1) {
	print STDERR "Usage: $0 OUTPUT_FILE [PROGRAM]\n";
	exit 1
}
$output = $ARGV[0];
if ($#ARGV + 1 > 1) {
	%program_names = ();
	foreach (split(/  */,$ARGV[1])) {
		$program_names{$_} = 1;
	}
}

# debug calls
$pch::debug = 0;
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
# collect all jobids (tree browsing), stop on softmean program
#
# note, the browsing tree is really needed here since we explore the workflow
#
$according_count = 0;
foreach my $jobid (@according_jobs) {
	my (@attrs, @program);

	print "Handling $jobid (position $according_count)\n" if ($debug);

	# stop on given program name
	@program = pch::psquery($ps, $jobid, "$pch::jplbtag:IPAW_PROGRAM");
	die "More program names of $jobid?" if ($#program > 0);
	if (exists $program_names{$program[0]}) { 
		print "$jobid is $program[0], stop here\n" if $debug;
		next;
	}

	# else browse up
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

foreach my $jobid (@according_jobs) {
	my @attrs2 = pch::psquery($ps, $jobid, "$pch::jplbtag:IPAW_STAGE");
	$according_jobs{$jobid} = $attrs2[0];
}

#
# queries on result set
#
print "Results\n";
print "=======\n";
print "\n";
foreach my $jobid (sort { $according_jobs{$b} <=> $according_jobs{$a} } keys %according_jobs) {
	print "jobid $jobid:\n";

	# query & output all desired atributes
	foreach my $attr (@pch::view_attributes) {
		my @attrs;
		my $attr_name = $attr; $attr_name =~ s/.*://;

		@attrs = pch::psquery($ps, $jobid, $attr);
		print "  attr $attr_name: ";
		if ($attr eq "$pch::jpsys:regtime") {
			print gmtime(@attrs[0])." (".join(", ", @attrs).")\n";
		} else {
			print join(", ", @attrs)."\n";
		}
	}

	print "\n";
}
