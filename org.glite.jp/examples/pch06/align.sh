#!/bin/sh

log_event() #1 - attr.name #2 attr.value
{
  GLITE_WMS_SEQUENCE_CODE=`$lb_logevent\
    --jobid="$GLITE_WMS_JOBID"\
    --source="Application"\
    --sequence="$GLITE_WMS_SEQUENCE_CODE"\
    --event="UserTag"\
    --node=$host\
    --name="$1"\
    --value="$2"\
  || echo $GLITE_WMS_SEQUENCE_CODE`
}

init_log_event()
{
  lb_logevent=${GLITE_WMS_LOCATION}/bin/glite-lb-logevent
  if [ ! -x "$lb_logevent" ]; then
    lb_logevent="${EDG_WL_LOCATION}/bin/edg-wl-logev"
  fi
  host=`hostname -f`
}


exec 2>$$.err >&2
set -x


hostname -f
date
echo $0 $*

chmod +x align_warp scanheader

globus-url-copy $1.img file://$PWD/anatomy.img
globus-url-copy $1.hdr file://$PWD/anatomy.hdr
globus-url-copy $2.img file://$PWD/reference.img
globus-url-copy $2.hdr file://$PWD/reference.hdr

./align_warp reference.img anatomy.img warp -m 12 -q
GLOBAL_MAXIMUM=`./scanheader anatomy.img | grep '^global maximum=' | sed 's/global maximum=//'`
echo $GLOBAL_MAXIMUM

globus-url-copy file://$PWD/warp $1.warp


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "1"
log_event "IPAW_PROGRAM" "align_warp"
log_event "IPAW_INPUT" "$1.img"
log_event "IPAW_INPUT" "$2.img"
log_event "IPAW_OUTPUT" "$1.warp"
log_event "IPAW_PARAM" "-m 12"
log_event "IPAW_PARAM" "-q"
log_event "IPAW_HEADER" "GLOBAL_MAXIMUM=$GLOBAL_MAXIMUM"


globus-url-copy file://$PWD/$$.err $1.align-err
