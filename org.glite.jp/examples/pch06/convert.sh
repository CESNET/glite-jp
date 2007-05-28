#!/bin/sh

. ./functions.sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1/$2-$3.pgm file://$PWD/atlas.pgm

chmod +x convert
./convert atlas.pgm atlas.gif

globus-url-copy file://$PWD/atlas.gif $1/$2-$3.gif


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "5"
log_event "IPAW_PROGRAM" "convert"
log_file_event "IPAW_INPUT" "$2-$3" "$1/$2-$3.pgm"
log_file_event "IPAW_OUTPUT" "$2-$3" "$1/$2-$3.gif"
