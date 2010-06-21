#! /bin/bash
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

xmlcut() {
	echo -e "\t<!-- $1 -->"
	echo
	grep "<$2>" $(dirname $0)/JobProvenance$1.xml -A 1000 | grep "</$2>" -B 1000 | grep -v "<$2>\|</$2>"
}

xmlmerge() {
	xmlcut PS $1
	echo
	xmlcut IS $1
}

DOC="$(xmlmerge doc)"
OPERATIONS="$(xmlmerge operations)"

XML_TMPL=$(sed $(dirname $0)/$1 -e 's/"/\\"/g')
eval "XML_RESULT=\"${XML_TMPL}\""
echo "$XML_RESULT"
