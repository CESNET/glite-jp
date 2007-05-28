#!/bin/sh

. ./functions.sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1/$2-resliced.img file://$PWD/resliced1.img
globus-url-copy $1/$2-resliced.hdr file://$PWD/resliced1.hdr
globus-url-copy $1/$3-resliced.img file://$PWD/resliced2.img
globus-url-copy $1/$3-resliced.hdr file://$PWD/resliced2.hdr
globus-url-copy $1/$4-resliced.img file://$PWD/resliced3.img
globus-url-copy $1/$4-resliced.hdr file://$PWD/resliced3.hdr
globus-url-copy $1/$5-resliced.img file://$PWD/resliced4.img
globus-url-copy $1/$5-resliced.hdr file://$PWD/resliced4.hdr

chmod +x softmean
./softmean atlas.hdr y null resliced1.img resliced2.img resliced3.img resliced4.img

globus-url-copy file://$PWD/atlas.img $1/$6.img
globus-url-copy file://$PWD/atlas.hdr $1/$6.hdr


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "3"
log_event "IPAW_PROGRAM" "softmean"
log_file_event "IPAW_INPUT" "$2-resliced" "$1/$2-resliced.img" "$1/$2-resliced.hdr"
log_file_event "IPAW_INPUT" "$3-resliced" "$1/$3-resliced.img" "$1/$3-resliced.hdr"
log_file_event "IPAW_INPUT" "$4-resliced" "$1/$4-resliced.img" "$1/$4-resliced.hdr"
log_file_event "IPAW_INPUT" "$5-resliced" "$1/$5-resliced.img" "$1/$5-resliced.hdr"
log_file_event "IPAW_OUTPUT" "$6" "$1/$6.img" "$1/$6.hdr"
