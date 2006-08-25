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


exec 2>$$.err >&2
set -x


hostname -f
date
echo $0 $*

chmod +x align_warp scanheader

lb_logevent=${GLITE_WMS_LOCATION}/bin/glite-lb-logevent
if [ ! -x "$lb_logevent" ]; then
  lb_logevent="${EDG_WL_LOCATION}/bin/edg-wl-logev"
fi
host=`hostname -f`

log_event "IPAW_PROGRAM" "align_warp"

globus-url-copy $1.img file://$PWD/anatomy.img
globus-url-copy $1.hdr file://$PWD/anatomy.hdr
globus-url-copy $2.img file://$PWD/reference.img
globus-url-copy $2.hdr file://$PWD/reference.hdr

./align_warp reference.img anatomy.img warp -m 12 -q
echo GLOBAL_MAXIMUM=`./scanheader anatomy.img | grep '^global maximum=' | sed 's/global maximum=//'`

globus-url-copy file://$PWD/warp $1.warp

globus-url-copy file://$PWD/$$.err $1.align-err
