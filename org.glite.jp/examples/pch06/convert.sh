#!/bin/sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1-$2.pgm atlas.pgm

./convert atlas.pgm atlas.gif

globus-url-copy atlas.gif $1-$2.gif
