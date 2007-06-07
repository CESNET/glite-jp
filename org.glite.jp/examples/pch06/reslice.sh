#!/bin/sh

. ./functions.sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1/$2-$3.warp file://$PWD/warp
globus-url-copy $1/$2.img file://$PWD/anatomy.img
globus-url-copy $1/$2.hdr file://$PWD/anatomy.hdr

chmod +x reslice
./reslice warp resliced

globus-url-copy file://$PWD/resliced.img $1/$2-$3-resliced.img
globus-url-copy file://$PWD/resliced.hdr $1/$2-$3-resliced.hdr


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "2"
log_event "IPAW_PROGRAM" "reslice"
log_file_event "IPAW_INPUT" "$2-$3.warp"
log_file_event "IPAW_INPUT" "$2"
log_file_event "IPAW_OUTPUT" "$2-$3-resliced"

