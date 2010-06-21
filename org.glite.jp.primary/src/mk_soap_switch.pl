#!/usr/bin/perl
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
