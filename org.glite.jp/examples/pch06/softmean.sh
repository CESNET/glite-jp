#!/bin/sh

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

./softmean atlas.hdr y null resliced1.img resliced2.img resliced3.img resliced4.img

globus-url-copy file://$PWD/atlas.img $5.img
globus-url-copy file://$PWD/atlas.hdr $5.hdr
