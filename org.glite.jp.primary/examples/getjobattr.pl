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

use SOAP::Lite;
use Data::Dumper;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$cred = $ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";
$ENV{HTTPS_CERT_FILE}= $ENV{HTTPS_KEY_FILE} = $ENV{HTTPS_CA_FILE} = $cred;

$proxy = shift;
$job = shift;

die "usage: $0 https://jp.primary.storage.org:8901/jpps https://some.nice.job/id attr attr ...\n"
	unless $ARGV[0];

$c = SOAP::Lite
	-> proxy($proxy)
	-> uri('http://glite.org/wsdl/services/jp');

service $c 'http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/3.1/JobProvenancePS.wsdl' or die "service: $1\n";

ns $c 'http://glite.org/wsdl/elements/jp';

print "WSDL OK\n";

push @attr,SOAP::Data->name(attributes => $_) for (@ARGV);

$req = SOAP::Data->value(
	SOAP::Data->name(jobid => $job),
	@attr
#	SOAP::Data->name(attributes => 'http://egee.cesnet.cz/en/Schema/LB/Attributes:CE'),
#	SOAP::Data->name(attributes => 'http://egee.cesnet.cz/en/Schema/JP/System:owner')
);



on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

$resp = GetJobAttributes $c $req;

print Dumper $resp->body unless $fault;

