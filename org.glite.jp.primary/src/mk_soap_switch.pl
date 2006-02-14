#!/usr/bin/perl

$formula = "/* Generated from @ARGV with the help of black magic.\n   Do not edit.\n*/\n\n";
print "${formula}#include \"soap_env_ctx.h\"\n\n";

# XXX: hardcoded
$prefix = 'ENV';

open EH,">soap_env_ctx.h" or die "soap_env_ctx.h: $!\n";
open EC,">soap_env_ctx.c" or die "soap_env_ctx.c: $!\n";

print EH "${formula}struct _glite_jp_soap_env_ctx_t {\n";

print EC "${formula}static struct _glite_jp_soap_env_ctx_t my_soap_env_ctx = {\n";


while ($_ = <>) {
	if (/^}$/) {
		print;
		$infunc = 0;
		undef @args;
		next;
	}

	next if $infunc;

	if (/^SOAP_FMAC3\s+(.+)\s+SOAP_FMAC4\s+([^(]+)\(([^)]*)\)/) {
		$type = $1;
		$func = $2;
		@a = split /,/,$3;
		for $a (@a) {
			$a =~ /.*\W(\w+)/;
			push @args,$1;
		}
		print;

		next if $func =~ "SOAP_$prefix";

		print EH "\t$type (*$func)();\n";
		print EC "\t$func,\n";
		next;
	}

	if (/^{/) {
		print;
		next if $func =~ "SOAP_$prefix";
		local $"=',';
		$infunc = 1;
		print "\t";
 		print 'return ' unless $type eq 'void';
		print "glite_jp_soap_env_ctx->$func(@args);\n";

		next;
	}

	print;
}

print EH "};\n\n";
print EH "extern struct _glite_jp_soap_env_ctx_t *glite_jp_soap_env_ctx;\n";
print EC "};\n";

print "struct _glite_jp_soap_env_ctx_t *glite_jp_soap_env_ctx;\n";
