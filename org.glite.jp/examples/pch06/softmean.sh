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

globus-url-copy $1-resliced.img file://$PWD/resliced1.img
globus-url-copy $1-resliced.hdr file://$PWD/resliced1.hdr
globus-url-copy $2-resliced.img file://$PWD/resliced2.img
globus-url-copy $2-resliced.hdr file://$PWD/resliced2.hdr
globus-url-copy $3-resliced.img file://$PWD/resliced3.img
globus-url-copy $3-resliced.hdr file://$PWD/resliced3.hdr
globus-url-copy $4-resliced.img file://$PWD/resliced4.img
globus-url-copy $4-resliced.hdr file://$PWD/resliced4.hdr

chmod +x softmean
./softmean atlas.hdr y null resliced1.img resliced2.img resliced3.img resliced4.img

globus-url-copy file://$PWD/atlas.img $5.img
globus-url-copy file://$PWD/atlas.hdr $5.hdr


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "3"
log_event "IPAW_PROGRAM" "softmean"
log_event "IPAW_INPUT" "$1-resliced.img"
log_event "IPAW_INPUT" "$1-resliced.hdr"
log_event "IPAW_INPUT" "$2-resliced.img"
log_event "IPAW_INPUT" "$2-resliced.hdr"
log_event "IPAW_INPUT" "$3-resliced.img"
log_event "IPAW_INPUT" "$3-resliced.hdr"
log_event "IPAW_INPUT" "$4-resliced.img"
log_event "IPAW_INPUT" "$4-resliced.hdr"
log_event "IPAW_OUTPUT" "$5.img"
log_event "IPAW_OUTPUT" "$5.hdr"

