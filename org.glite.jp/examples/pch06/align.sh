#!/bin/sh

set -ex

hostname -f
date
echo $0 $*

globus-url-copy $1.img file://$PWD/anatomy.img
globus-url-copy $1.hdr file://$PWD/anatomy.hdr
globus-url-copy $2.img file://$PWD/reference.img
globus-url-copy $2.hdr file://$PWD/reference.hdr

./align_warp reference.img anatomy.img warp -m 12 -q
echo GLOBAL_MAXIMUM=`./scanheader anatomy.img | grep '^global maximum=' | sed 's/global maximum=//'`

globus-url-copy file://$PWD/warp $1.warp
