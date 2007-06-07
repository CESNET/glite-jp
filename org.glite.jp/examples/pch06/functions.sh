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

log_file_event_cool_but_unused() #1 - attr.name #2 file.name #3 file.uri...
{
  attr="$1"
  str="<file name=\"urn:challenge:$2\">
"
  while [ -n "$3" ]; do
    str="$str  <url>$3</url>
"
    shift
  done
  str="$str</file>"
  log_event "$attr" "$str"
}

log_file_event() #1 - attr.name #2 file.name #3 file.uri...
{
  attr="$1"
  str="urn:challenge:$2"
  log_event "$attr" "$str"
}
