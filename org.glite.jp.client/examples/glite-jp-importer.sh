#! /bin/bash

#
# example script for purging LB and importing the dumps to JP
#

# job provenance server
if [ -z "$JPSERVER" ]; then
  echo "Please specify the Job Provanance Primary Storage server."
  exit 1
fi
# bookkeeping server
if [ -z "$BKSERVER" ]; then
  echo "Please specify the Book Keeping server."
  exit 1
fi
# certificates
if [ -z "$X509_USER_CERT" -o -z "$X509_USER_KEY" ]; then
  echo "Please set X509_USER_CERT and X509_USER_KEY."
  exit 1
fi
# dump directory of bkserver
if [ -z "$LB_DUMPDIR" ]; then
  LB_DUMPDIR=/tmp/LB/dump
  echo "LB_DUMPDIR not specified (-D arguent of the bkserver), used $LB_DUMPDIR"
fi
# LB maildir for job registration
if [ -z "$LB_JPREG_MAILDIR" ]; then
  LB_JPREG_MAILDIR=/tmp/LB/lb_server_jpreg
  echo "LB_JPREG_MAILDIR not specified (-J arguent of the bkserver), used $LB_JPREG_MAILDIR"
fi

CERT_ARGS="-c $X509_USER_CERT -k $X509_USER_KEY"
LB_JPDUMP_MAILDIR=${LB_JPDUMP_MAILDIR:-/tmp/LB/lb_server_jpdump}
LB_DUMPDIR_OLD=${LB_DUMPDIR_OLD:-$LB_DUMPDIR.old}
LB_EXPORTDIR=${LB_EXPORTDIR:-/tmp/LB/lb_export}
PREFIX=${PREFIX:-`dirname $0`/..}
LOGDIR=${LOGDIR:-/tmp/LB/log}
GLITE_LB_PURGE_ARGS=${GLITE_LB_PURGE_ARGS:--a 1h -c 1h -n 1h -o 1d}


[ -d $LB_JPDUMP_MAILDIR ] || mkdir -p $LB_JPDUMP_MAILDIR
[ -d $LB_DUMPDIR ] || mkdir -p $LB_DUMPDIR
[ -d $LB_DUMPDIR_OLD ] || mkdir -p $LB_DUMPDIR_OLD
[ -d $LB_EXPORTDIR ] || mkdir -p $LB_EXPORTDIR
[ -d $LOGDIR ] || mkdir -p $LOGDIR

echo "Using cert args $CERT_ARGS"

$PREFIX/bin/glite-jp-importer -r $LB_JPREG_MAILDIR -d $LB_JPDUMP_MAILDIR $CERT_ARGS -g -p $JPSERVER > $LOGDIR/jp-importer.log 2>&1 &
JP_PID=$!
trap "kill $JP_PID; exit 0" SIGINT

while [ 1 ]; do
  $PREFIX/sbin/glite-lb-purge $GLITE_LB_PURGE_ARGS -l -m $BKSERVER

  for file in $LB_DUMPDIR/*; do
    if [ -s $file ]; then
      $PREFIX/sbin/glite-lb-lb_dump_exporter -d $file -s $LB_EXPORTDIR -m $LB_JPDUMP_MAILDIR
      mv $file $LB_DUMPDIR_OLD
    else
      rm $file
    fi
  done

  sleep 30
done
