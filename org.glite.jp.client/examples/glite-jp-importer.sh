#! /bin/bash

#
# example script for purging LB and importing the dumps to JP
#

# job provenance server
JBSERVER=umbar.ics.muni.cz:8901
# bookkeeping server
BKSERVER=scientific.civ.zcu.cz:9000
# dump directory of bkserver (-D argument)
BKSERVER_DUMPDIR=/tmp/dump
# LB maildir for job registration (-J argument)
BKSERVER_JOBREG_MAILDIR=/tmp/lb_server_jpreg

if [ -z "$X509_USER_CERT" -o -z "$X509_USER_KEY" ]; then
  echo "Please set X509_USER_CERT and X509_USER_KEY."
  exit 1
fi

CERT_ARGS="-c $X509_USER_CERT -k $X509_USER_KEY"
LB_DUMPDIR=/tmp/lb_server_dump
BKSERVER_DUMPDIR_OLD=/tmp/dump.old
LB_EXPORTDIR=/tmp/lb_export
PREFIX=`dirname $0`/..
LOGDIR=/tmp/log


[ -d $LB_DUMPDIR ] || mkdir -p $LB_DUMPDIR
[ -d $BKSERVER_DUMPDIR ] || mkdir -p $BKSERVER_DUMPDIR
[ -d $BKSERVER_DUMPDIR_OLD ] || mkdir -p $BKSERVER_DUMPDIR_OLD
[ -d $LB_EXPORTDIR ] || mkdir -p $LB_EXPORTDIR
[ -d $LOGDIR ] || mkdir -p $LOGDIR

echo "Using cert args $CERT_ARGS"

$PREFIX/bin/glite-jp-importer -r $BKSERVER_JOBREG_MAILDIR -d $LB_DUMPDIR $CERT_ARGS -g -p $JBSERVER > $LOGDIR/jp-importer.log 2>&1 &
JP_PID=$!
trap "kill $JP_PID; exit 0" SIGINT

while [ 1 ]; do
  $PREFIX/sbin/glite-lb-purge -o 1 -l -m $BKSERVER

  for file in $BKSERVER_DUMPDIR/*; do
    rm -f $LB_EXPORTDIR/*
    if [ -s $file ]; then
      $PREFIX/sbin/glite-lb-lb_dump_exporter -d $file -s $LB_EXPORTDIR -m $LB_DUMPDIR
      mv $file $BKSERVER_DUMPDIR_OLD
    else
      rm $file
    fi
  done

  sleep 30
done
