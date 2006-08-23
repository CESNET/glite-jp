#!/bin/sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1.warp file://$PWD/warp
globus-url-copy $1.img file://$PWD/anatomy.img
globus-url-copy $1.hdr file://$PWD/anatomy.hdr

./reslice warp resliced

globus-url-copy file://$PWD/resliced.img $1-resliced.img
globus-url-copy file://$PWD/resliced.hdr $1-resliced.hdr
