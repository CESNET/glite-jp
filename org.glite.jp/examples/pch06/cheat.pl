#!/usr/bin/perl 

use Getopt::Std;

getopt('g:s:');
$glite = $opt_g ? $opt_g : "$ENV{HOME}/glite/stage";
$dagids = "$glite/examples/glite-lb-dagids";
$regjob = "$glite/examples/glite-lb-job_reg";
$logevent = "$glite/bin/glite-lb-logevent -I";

die "usage: $0 -s bkserver [ -g glite_install_dir ] dump \n" unless $opt_s && $#ARGV == 0;
$server = $opt_s;

%omap = 
(
# 	common => {
		'DG.SOURCE' => 'source',
		'DG.SRC_INSTANCE' => 'source-instance',
		'DG.SEQCODE' => 'sequence',
#		
#	},
#	UserTag => {
#		'DG.USERTAG.NAME' => '',
#		'DG.USERTAG.VALUE' => '',
#	},
#	Accepted => {
#		'DG.ACCEPTED.FROM' => ''
##		DG.ACCEPTED.FROM_HOST
#	},
#	EnQueued => {
#		DG.ENQUEUED.QUEUE
#		DG.ENQUEUED.JOB
#		DG.ENQUEUED.RESULT
#	},
);

%smap =
(
	'HOST' => 1,
	'DATE' => 1,
	'LVL' => 1,
	'PROG' => 1,
	'DG.ARRIVED' => 1,
	'DG.PRIORITY' => 1,
	'DG.EVNT' => 1,
	'DG.JOBID' => 1,
	'DG.USER' => 1,
);

while ($_ = <>) {
	next if /^\s*$/;
	
	chomp;
	@F = split / /;
	
	undef $prev;
	undef %f;
	
	for $f (@F) {
		if ($f =~ /^[.A-Z_]+="/) {
	#		print $prev,"\n" if $prev;
			@P = split /=/,$prev,2;
			$P[0] =~ s/^\s*//; $P[0] =~ s/\s*$//;
			$P[1] =~ s/^\s*\"//; $P[1] =~ s/\"\s*$//;
			$f{$P[0]} = $P[1];
			$prev = $f;
		}
		else { $prev .= ' '.$f; }
	}
	
	# print $prev,"\n";
	@P = split /=/,$prev,2;
	$P[0] =~ s/^\s*//; $P[0] =~ s/\s*$//;
	$P[1] =~ s/^\s*\"//; $P[1] =~ s/\"\s*$//;
	$f{$P[0]} = $P[1];

	push @events,{%f};
}

for (@events) {
#	print $_->{'DG.JOBID'},"\n";
	push @{$jobs{$_->{'DG.JOBID'}}},$_;

	if ($_->{'DG.EVNT'} eq 'UserTag' &&
		$_->{'DG.USERTAG.NAME'} eq 'ipaw_stage')
	{
		my $s = $_->{'DG.USERTAG.VALUE'};
		push @{$stage{$s}},$_->{'DG.JOBID'};
	}
}

$odag = $events[0]->{'DG.JOBID'};
print "dag: $odag\n";

for (keys %jobs) {
	next if $_ eq $odag;
	push @onode,$_;
	$index{$_} = $#onode;
}

for (sort { $a <=> $b } keys %stage) {
	local $"="\n\t";
	print "stage $_\n\t@{$stage{$_}}\n";
}

$nodes = $#onode + 1;
open REG,"$dagids -m $server -n $nodes -s $$|" or die "$dagids: $!\n";

while ($_ = <REG>) {
#	print $_;
	eval "\$$_";
}
close REG;

$jdl = $jobs{$odag}->[0]->{'DG.REGJOB.JDL'};

print "substituting edg_jobid ...\n";
print "$odag -> $dag\n";
$jdl =~ s|edg_jobid = \\"$odag\\"|edg_jobid = \\"$dag\\"|;
for (0..$nodes-1) {
	print "$onode[$_] -> $node[$_]\n";
	$jdl =~ s|edg_jobid = \\"$onode[$_]\\"|edg_jobid = \\"$node[$_]\\"|;
}

$jdlf="/tmp/cheat.$$";
open JDL,">$jdlf" or die "$jdlf: $!";
print JDL $jdl;
close JDL;

print "$regjob -j $dag -s NetworkServer -e $seed -n $nodes -l $jdlf\n";
system "$regjob -j $dag -s NetworkServer -e $seed -n $nodes -l $jdlf"; # XXX: or die "$regjob\n";

sub logit {
	local $_ = shift;
	my $job = shift;
	my @opt = ();
	my $ev = $_->{'DG.EVNT'};

	for my $k (keys %$_) {
		next if $smap{$k};

		if ($omap{$k}) {
			push @opt,"--$omap{$k}";
			push @opt,"'$_->{$k}'";
		}
		else {
			$k =~ /^DG\.([A-Z]+)\.([A-Z_]+)$/;
			die "$k: unexpected in $ev\n" unless $1 eq uc $ev;
			push @opt,'--'.lc $2;
			push @opt,"'$_->{$k}'";
		}

	}
	print "\n#####\n$logevent -j $job -e $_->{'DG.EVNT'} @opt\n\n#####\n";
	system "$logevent -j $job -e $_->{'DG.EVNT'} @opt\n";
}

print "DAG events up to Running ...\n";
for (sort { $a->{'DG.SEQCODE'} cmp $b->{'DG.SEQCODE'} } @{$jobs{$odag}}) {
	my $ev = $_->{'DG.EVNT'};
	next if $ev eq 'RegJob';
	logit $_,$dag;
	last if $ev eq 'Running';
}


for my $s (sort { $a <=> $b } keys %stage) {
	print "\n### stage $s ###\n";
	for my $ojob (@{$stage{$s}}) {
		for (sort { $a->{'DG.SEQCODE'} cmp $b->{'DG.SEQCODE'} } @{$jobs{$ojob}}) {
			my $ev = $_->{'DG.EVNT'};
			next if $ev eq 'RegJob';

			logit $_,$node[$index{$ojob}];
		}
	}
}


print "Final DAG events ...\n";
undef $gotrun;
for (sort { $a->{'DG.SEQCODE'} cmp $b->{'DG.SEQCODE'} } @{$jobs{$odag}}) {
	my $ev = $_->{'DG.EVNT'};
	next unless $gotrun || $ev eq 'Running';
	$gotrun = 1;
	next if $ev eq 'Running';

	logit $_,$dag;
}


