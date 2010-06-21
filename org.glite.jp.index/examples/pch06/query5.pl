#! /usr/bin/perl -W
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
# 5. query:
# 
# Find all Atlas Graphic images outputted from workflows where at least one of
# the input Anatomy Headers had an entry global maximum=4095. The contents of
# a header file can be extracted as text using the scanheader AIR utility.
#
# call:
#   ./query5.pl [PROGRAMS] [END_PROGRAMS] [HEADER] 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps=$pch::ps;
my $is=$pch::is;
my %program_names=(align_warp => 1);
my %end_program_names=(convert => 1);
my $header="GLOBAL_MAXIMUM=4095"; # test for exact equal (scripts already prepared it)

my @according_jobs = (); # sequencially jobid list
my %according_jobs = (); # hash jobid list


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
if ($#ARGV + 1 >= 3) {
	$header = $ARGV[2];
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
	["$pch::jplbtag:IPAW_HEADER", ['EQUAL', "<string>$header</string>"]],
], \@pch::view_attributes);
print STDERR Dumper(@jobs) if ($debug);
die "...so exit on error" if ($pch::err);

#
# collect all jobs (tree browsing down)
#
foreach my $job (@jobs) {
	my %job = %$job;
	my $jobid = $job{jobid};
	my @succs;
	my $pname;

	$pname = $job{attributes}{"$pch::jplbtag:IPAW_PROGRAM"}{value}[0];
	print "Handling $jobid ($pname)\n" if ($debug);

	if (exists $end_program_names{$pname}) {
		print "It's $pname\n" if $debug;
		if (!exists $according_jobs{$jobid}) {
			$according_jobs{$jobid} = \%job;
			push @according_jobs, $jobid;
			print "Added $jobid to $#according_jobs\n" if ($debug);
		}
		else {
			print "Already existing $jobid\n" if ($debug);
		}
		next;
	}

	@succs = pch::isquery($is, [["$pch::jpwf:ancestor", ['EQUAL', "<string>$jobid</string>"]]], \@pch::view_attributes);
	die "...so exit on error" if ($pch::err);
	push @jobs, @succs;
}


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
