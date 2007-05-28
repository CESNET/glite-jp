#!/bin/sh

. ./functions.sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1-$2.pgm file://$PWD/atlas.pgm

chmod +x pgmtoppm
./pgmtoppm rgb:ffff/00/00 atlas.pgm > atlas.ppm

globus-url-copy file://$PWD/atlas.ppm $1-$2.ppm


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "5"
log_event "IPAW_PROGRAM" "pgmtoppm"
log_event "IPAW_INPUT" "$1-$2.pgm"
log_event "IPAW_OUTPUT" "$1-$2.ppm"

