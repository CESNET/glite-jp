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


set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1.warp file://$PWD/warp
globus-url-copy $1.img file://$PWD/anatomy.img
globus-url-copy $1.hdr file://$PWD/anatomy.hdr

chmod +x reslice
./reslice warp resliced

globus-url-copy file://$PWD/resliced.img $1-resliced.img
globus-url-copy file://$PWD/resliced.hdr $1-resliced.hdr


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "2"
log_event "IPAW_PROGRAM" "reslice"
log_event "IPAW_INPUT" "$1.warp"
log_event "IPAW_INPUT" "$1.img"
log_event "IPAW_INPUT" "$1.hdr"
log_event "IPAW_OUTPUT" "$1-resliced.img"
log_event "IPAW_OUTPUT" "$1-resliced.img"

