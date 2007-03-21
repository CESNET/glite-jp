#! /bin/bash

#
# all-in-one example script for purging LB and importing the dumps to JP
#

GLITE_LOCATION=${GLITE_LOCATION:-/opt/glite}
GLITE_LOCATION_VAR=${GLITE_LOCATION_VAR:-${GLITE_LOCATION}/var}

[ -f /etc/glite.conf ] && . /etc/glite.conf
[ -f $GLITE_LOCATION/etc/glite-wms.conf ] && . $GLITE_LOCATION/etc/glite-wms.conf

[ -f $GLITE_LOCATION/etc/jp.conf ] && . $GLITE_LOCATION/etc/jp.conf
[ -f $GLITE_LOCATION_VAR/etc/jp.conf ] && . $GLITE_LOCATION_VAR/etc/jp.conf

[ -f $HOME/.glite.conf ] && . $HOME/.glite.conf


# job provenance server
if [ -z "$GLITE_LB_EXPORT_JPPS" ]; then
  echo "Please specify the Job Provanance Primary Storage server."
  exit 1
fi
# book keeping server
GLITE_LB_SERVER_PORT=${GLITE_LB_SERVER_PORT:-9000}
GLITE_LB_EXPORT_BKSERVER=${GLITE_LB_EXPORT_BKSERVER:-localhost:$GLITE_LB_SERVER_PORT}
# certificates
if [ -z "$X509_USER_CERT" -o -z "$X509_USER_KEY" ]; then
  echo "Please set X509_USER_CERT and X509_USER_KEY."
  exit 1
fi
# dump directory of bkserver
if [ -z "$GLITE_LB_EXPORT_DUMPDIR" ]; then
  GLITE_LB_EXPORT_DUMPDIR=/tmp/dump
  echo "GLITE_LB_EXPORT_DUMPDIR not specified (-D arguent of the bkserver), used $GLITE_LB_EXPORT_DUMPDIR"
fi
# LB maildir for job registration
if [ -z "$GLITE_LB_EXPORT_JPREG_MAILDIR" ]; then
  GLITE_LB_EXPORT_JPREG_MAILDIR=/tmp/jpreg
  echo "GLITE_LB_EXPORT_JPREG_MAILDIR not specified (-J arguent of the bkserver), used $GLITE_LB_EXPORT_JPREG_MAILDIR"
fi
# pidfile
[ -n "$GLITE_JP_IMPORTER_PIDFILE" ] && pidfile="-i $GLITE_JP_IMPORTER_PIDFILE "

CERT_ARGS="-c $X509_USER_CERT -k $X509_USER_KEY"
GLITE_LB_EXPORT_JPDUMP_MAILDIR=${GLITE_LB_EXPORT_JPDUMP_MAILDIR:-/tmp/jpdump}
GLITE_LB_EXPORT_JOBSDIR=${GLITE_LB_EXPORT_JOBSDIR:-/tmp/lbexport}
PREFIX=${PREFIX:-`dirname $0`/..}
LOGDIR=$GLITE_LOCATION_VAR
GLITE_LB_EXPORT_PURGE_ARGS=${GLITE_LB_EXPORT_PURGE_ARGS:---cleared 2d --aborted 2w --cancelled 2w --other 2m}


[ -d $GLITE_LB_EXPORT_JPDUMP_MAILDIR ] || mkdir -p $GLITE_LB_EXPORT_JPDUMP_MAILDIR
[ -d $GLITE_LB_EXPORT_DUMPDIR ] || mkdir -p $GLITE_LB_EXPORT_DUMPDIR
[ -n "$GLITE_LB_EXPORT_DUMPDIR_KEEP" -a ! -d $GLITE_LB_EXPORT_DUMPDIR_KEEP ] && mkdir -p $GLITE_LB_EXPORT_DUMPDIR_KEEP
[ -d $GLITE_LB_EXPORT_JOBSDIR ] || mkdir -p $GLITE_LB_EXPORT_JOBSDIR
if [ -n "$GLITE_LB_EXPORT_JOBSDIR_KEEP" ]; then
  keep_jobs="--store ${GLITE_LB_EXPORT_JOBSDIR_KEEP} "
  [ -d $GLITE_LB_EXPORT_JOBSDIR_KEEP ] || mkdir -p $GLITE_LB_EXPORT_JOBSDIR_KEEP
fi
[ -d $LOGDIR ] || mkdir -p $LOGDIR

echo "Using cert args $CERT_ARGS"

$PREFIX/bin/glite-jp-importer --reg-mdir $GLITE_LB_EXPORT_JPREG_MAILDIR --dump-mdir $GLITE_LB_EXPORT_JPDUMP_MAILDIR $CERT_ARGS -g --jpps $GLITE_LB_EXPORT_JPPS $pidfile$keep_jobs> $LOGDIR/jp-importer.log 2>&1 &
JP_PID=$!
trap "kill $JP_PID; exit 0" SIGINT

while [ 1 ]; do
  $PREFIX/sbin/glite-lb-purge $GLITE_LB_EXPORT_PURGE_ARGS -l -m $GLITE_LB_EXPORT_BKSERVER

  for file in $GLITE_LB_EXPORT_DUMPDIR/*; do
    if [ -s $file ]; then
      $PREFIX/sbin/glite-lb-lb_dump_exporter -d $file -s $GLITE_LB_EXPORT_JOBSDIR -m $GLITE_LB_EXPORT_JPDUMP_MAILDIR
      if [ -n "$GLITE_LB_EXPORT_DUMPDIR_KEEP" ]; then
        mv $file $GLITE_LB_EXPORT_DUMPDIR_KEEP
      else
        rm $file
      fi
    else
      rm $file
    fi
  done

  sleep 30
done
