#! /usr/bin/perl

#
# 3. query:
# 
# Find the Stage 3, 4 and 5 details of the process that led to Atlas X Graphic.
#
# call:
#   ./query3.pl OUTPUT_FILE_NAME 2>/dev/null
#

use strict;
use pch;
use Data::Dumper;

my $ps='https://skurut1.cesnet.cz:8901';
my $is='https://skurut1.cesnet.cz:8902';
my @attributes = ("$pch::jpsys:jobId", "$pch::jpwf:ancestor", @pch::view_attributes);

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
$pch::debug = 0;
my $debug = 0;

#
# find out processes with given output
#
my @jobs = pch::isquery($is, [
	["$pch::jplbtag:IPAW_OUTPUT", ['EQUAL', "<string>$output</string>"]],
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
	my $stage = $attributes{"$pch::jplbtag:IPAW_STAGE"}{value}[0];

	if ( $stage == 3 || $stage == 4 || $stage == 5) {
		print "jobid $jobid:\n";

		# query & output all desired atributes
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
	} else {
		print "(ignored $jobid with stage $stage)\n" if $debug;
	}
}
