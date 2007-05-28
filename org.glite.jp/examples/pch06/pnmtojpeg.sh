#!/bin/sh

. ./functions.sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1-$2.ppm file://$PWD/atlas.ppm

chmod +x pnmtojpeg
./pnmtojpeg atlas.ppm >atlas.jpg


globus-url-copy file://$PWD/atlas.jpg $1-$2.jpg


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "6"
log_event "IPAW_PROGRAM" "pnmtojpeg"
log_event "IPAW_INPUT" "$1-$2.ppm"
log_event "IPAW_OUTPUT" "$1-$2.jpg"

