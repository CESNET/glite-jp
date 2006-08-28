#!/bin/sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1-$2.pgm file://$PWD/atlas.pgm

chmod +x convert
./convert atlas.pgm atlas.gif

globus-url-copy file://$PWD/atlas.gif $1-$2.gif
