#!/bin/sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1.hdr file://$PWD/atlas.hdr
globus-url-copy $1.img file://$PWD/atlas.img

chmod +x slicer
FSLOUTPUTTYPE=ANALYZE
export FSLOUTPUTTYPE
./slicer atlas.hdr -$2 .5 atlas-$2.pgm

globus-url-copy file://$PWD/atlas-$2.pgm $1-$2.pgm
