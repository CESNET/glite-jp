#! /bin/bash

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
