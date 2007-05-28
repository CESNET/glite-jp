#!/bin/sh

. ./functions.sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1/$2.warp file://$PWD/warp
globus-url-copy $1/$2.img file://$PWD/anatomy.img
globus-url-copy $1/$2.hdr file://$PWD/anatomy.hdr

chmod +x reslice
./reslice warp resliced

globus-url-copy file://$PWD/resliced.img $1/$2-resliced.img
globus-url-copy file://$PWD/resliced.hdr $1/$2-resliced.hdr


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "2"
log_event "IPAW_PROGRAM" "reslice"
log_file_event "IPAW_INPUT" "$2" "$1/$2.warp"
log_file_event "IPAW_INPUT" "$2" "$1/$2.img" "$1/$2.hdr"
log_file_event "IPAW_OUTPUT" "$2-resliced" "$1/$2-resliced.img" "$1/$2-resliced.hdr"

