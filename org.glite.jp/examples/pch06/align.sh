#!/bin/sh

. ./functions.sh

exec 2>$$.err >&2
set -x


hostname -f
date
echo $0 $*

chmod +x align_warp scanheader

globus-url-copy $1/$2.img file://$PWD/anatomy.img
globus-url-copy $1/$2.hdr file://$PWD/anatomy.hdr
globus-url-copy $1/$3.img file://$PWD/reference.img
globus-url-copy $1/$3.hdr file://$PWD/reference.hdr

./align_warp reference.img anatomy.img warp -m 12 -q
GLOBAL_MAXIMUM=`./scanheader anatomy.img | grep '^global maximum=' | sed 's/global maximum=//'`
echo $GLOBAL_MAXIMUM

globus-url-copy file://$PWD/warp $1/$2-$4.warp


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "1"
log_event "IPAW_PROGRAM" "align_warp"
log_file_event "IPAW_INPUT" "$2" 
log_file_event "IPAW_INPUT" "$3"
log_file_event "IPAW_OUTPUT" "$2-$4.warp"
log_event "IPAW_PARAM" "-m 12"
log_event "IPAW_PARAM" "-q"
log_event "IPAW_HEADER" "GLOBAL_MAXIMUM=$GLOBAL_MAXIMUM"


globus-url-copy file://$PWD/$$.err $1/$2-$4.align-err
