#! /usr/bin/perl

#
# 5. query:
# 
# Find all Atlas Graphic images outputted from workflows where at least one of
# the input Anatomy Headers had an entry global maximum=4095. The contents of
# a header file can be extracted as text using the scanheader AIR utility.
#
# call:
#   ./query5.pl 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps='https://skurut1.cesnet.cz:8901';
my $is='https://skurut1.cesnet.cz:8902';
my $program_name='align_warp';
my $end_program_name='convert';
my $atlas_image_program_name='convert';
my $header="GLOBAL_MAXIMUM=4095"; # test for exact equal (scripts already prepared it)

my @according_jobs = (); # sequencially jobid list
my %according_jobs = (); # hash jobid list
my $according_count = 0;


# debug calls
$pch::debug = 1;
my $debug = 1;

#
# find out processes with given name and parameters
#
my @jobs = pch::isquery($is, [
	["$pch::jplbtag:IPAW_PROGRAM", ['EQUAL', "<string>$program_name</string>"]],
	["$pch::jplbtag:IPAW_HEADER", ['EQUAL', "<string>$header</string>"]],
], \@pch::view_attributes);
print Dumper(@jobs) if ($debug);
die "...so exit on error" if ($pch::err);

#
# initial set of DAGs from index server
#
foreach my $job (@jobs) {
	my %job = %$job;
	my %attributes = %{$job{attributes}};
	my $dagjobid = $attributes{"$pch::lbattr:parent"}{value}[0];

	if (!exists $according_jobs{$dagjobid}) {
		%job = ();
		push @according_jobs, $dagjobid;
		# query to primary storage when searching by jobid
		$job{jobid} = $dagjobid;
		foreach my $attr (@pch::view_attributes) {
			my @value;

			@value = pch::psquery($ps, $dagjobid, $attr);
			if (defined @value) { $job{attributes}{$attr} = \@value; }
		}
		$according_jobs{$dagjobid} = \%job;
	}
}
undef @jobs;


#
# collect all jobs (tree browsing down)
#
$according_count = 0;
foreach my $jobid (@according_jobs) {
	my @succs;

	print "Handling $jobid (position $according_count)\n" if ($debug);

	if ($according_jobs{$jobid}{attributes}{"$pch::IPAW_PROGRAM"}[0] eq $end_program_name) {
		print "It's $end_program_name\n" if $debug;
		next;
	}

	@succs = pch::isquery($is, [["$pch::jpwf:ancestor", ['EQUAL', "<string>$jobid</string>"]]], \@pch::view_attributes);
	die "...so exit on error" if ($pch::err);

	for my $succ (@succs) {
		my %succ = %$succ;
		print "Considered: $succ{jobid}\n" if ($debug);
		if (!exists $according_jobs{$succ{jobid}}) {
			$according_jobs{$succ{jobid}} = \%succ;
			push @according_jobs, $succ{jobid};
			print "Added $succ{jobid} to $#according_jobs\n" if ($debug);
		}
		else {
			print "Already existing $succ{jobid}\n" if ($debug);
		}
	}
	$according_count++;
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
