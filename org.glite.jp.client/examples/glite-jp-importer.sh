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
# bookkeeping server
if [ -z "$GLITE_LB_EXPORT_BKSERVER" ]; then
  echo "Please specify the Book Keeping server."
  exit 1
fi
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

CERT_ARGS="-c $X509_USER_CERT -k $X509_USER_KEY"
GLITE_LB_EXPORT_JPDUMP_MAILDIR=${GLITE_LB_EXPORT_JPDUMP_MAILDIR:-/tmp/jpdump}
GLITE_LB_EXPORT_DUMPDIR_OLD=${GLITE_LB_EXPORT_DUMPDIR_OLD:-$GLITE_LB_EXPORT_DUMPDIR.old}
GLITE_LB_EXPORT_EXPORTDIR=${GLITE_LB_EXPORT_EXPORTDIR:-/tmp/lbexport}
GLITE_LB_EXPORT_SANDBOX_MAILDIR=${GLITE_LB_EXPORT_SANDBOX_MAILDIR:-/tmp/jpsandbox}
PREFIX=${PREFIX:-`dirname $0`/..}
LOGDIR=$GLITE_LOCATION_VAR
GLITE_LB_EXPORT_PURGE_ARGS=${GLITE_LB_EXPORT_PURGE_ARGS:--a 1h -c 1h -n 1h -o 1d}


[ -d $GLITE_LB_EXPORT_JPDUMP_MAILDIR ] || mkdir -p $GLITE_LB_EXPORT_JPDUMP_MAILDIR
[ -d $GLITE_LB_EXPORT_DUMPDIR ] || mkdir -p $GLITE_LB_EXPORT_DUMPDIR
[ -d $GLITE_LB_EXPORT_DUMPDIR_OLD ] || mkdir -p $GLITE_LB_EXPORT_DUMPDIR_OLD
[ -d $GLITE_LB_EXPORT_EXPORTDIR ] || mkdir -p $GLITE_LB_EXPORT_EXPORTDIR
[ -d $GLITE_LB_EXPORT_SANDBOX_MAILDIR ] || mkdir -p $GLITE_LB_EXPORT_SANDBOX_MAILDIR
[ -d $LOGDIR ] || mkdir -p $LOGDIR

echo "Using cert args $CERT_ARGS"

$PREFIX/bin/glite-jp-importer --reg-mdir $GLITE_LB_EXPORT_JPREG_MAILDIR --dump-mdir $GLITE_LB_EXPORT_JPDUMP_MAILDIR $CERT_ARGS --sandbox-mdir $GLITE_LB_EXPORT_SANDBOX_MAILDIR -g --jpps $GLITE_LB_EXPORT_JPPS $pidfile$keep_jobs> $LOGDIR/jp-importer.log 2>&1 &
JP_PID=$!
trap "kill $JP_PID; exit 0" SIGINT

while [ 1 ]; do
  $PREFIX/sbin/glite-lb-purge $GLITE_LB_EXPORT_PURGE_ARGS -l -m $GLITE_LB_EXPORT_BKSERVER

  for file in $GLITE_LB_EXPORT_DUMPDIR/*; do
    if [ -s $file ]; then
      $PREFIX/sbin/glite-lb-lb_dump_exporter -d $file -s $GLITE_LB_EXPORT_EXPORTDIR -m $GLITE_LB_EXPORT_JPDUMP_MAILDIR
      mv $file $GLITE_LB_EXPORT_DUMPDIR_OLD
    else
      rm $file
    fi
  done

  sleep 30
done
