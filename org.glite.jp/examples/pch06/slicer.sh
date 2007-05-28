#!/bin/sh

. ./functions.sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1/$2.hdr file://$PWD/atlas.hdr
globus-url-copy $1/$2.img file://$PWD/atlas.img

chmod +x slicer
FSLOUTPUTTYPE=ANALYZE
export FSLOUTPUTTYPE
./slicer atlas.hdr -$3 .5 atlas-$3.pgm

globus-url-copy file://$PWD/atlas-$3.pgm $1/$2-$3.pgm


# Log LB user_tags
init_log_event
log_event "IPAW_STAGE" "4"
log_event "IPAW_PROGRAM" "slicer"
log_file_event "IPAW_INPUT" "$2" "$1/$2.hdr" "$1/$2.img"
log_file_event "IPAW_OUTPUT" "$2-$3" "$1/$2-$3.pgm"

