#! /bin/bash

#
# all-in-one example script for purging LB and importing the dumps to JP
#
# uses helper purging script glite-lb-export.sh from glite.lb.client
#

GLITE_LOCATION=${GLITE_LOCATION:-/opt/glite}
GLITE_LOCATION_VAR=${GLITE_LOCATION_VAR:-${GLITE_LOCATION}/var}

[ -f /etc/glite.conf ] && . /etc/glite.conf
[ -f $GLITE_LOCATION/etc/glite-wms.conf ] && . $GLITE_LOCATION/etc/glite-wms.conf

[ -f $GLITE_LOCATION/etc/jp.conf ] && . $GLITE_LOCATION/etc/jp.conf
[ -f $GLITE_LOCATION_VAR/etc/jp.conf ] && . $GLITE_LOCATION_VAR/etc/jp.conf

[ -f $HOME/.glite.conf ] && . $HOME/.glite.conf

# get default values for purge and export
PREFIX=${GLITE_LOCATION:-`dirname $0`/..}
GLITE_LB_EXPORT_ENABLED="false" GLITE_LB_PURGE_ENABLED="false" . $PREFIX/bin/glite-lb-export.sh

# job provenance server
if [ -z "$GLITE_LB_EXPORT_JPPS" ]; then
  echo "Please specify the Job Provanance Primary Storage server."
  exit 1
fi
# certificates
if [ -z "$X509_USER_CERT" -o -z "$X509_USER_KEY" ]; then
  echo "Please set X509_USER_CERT and X509_USER_KEY."
  exit 1
fi
# LB maildir for job registration
if [ -z "$GLITE_LB_EXPORT_JPREG_MAILDIR" ]; then
  GLITE_LB_EXPORT_JPREG_MAILDIR=$GLITE_LOCATION_VAR/jpreg
  echo "GLITE_LB_EXPORT_JPREG_MAILDIR not specified (-J arguent of the bkserver), used $GLITE_LB_EXPORT_JPREG_MAILDIR"
fi
if [ -n "$GLITE_LB_EXPORT_SANDBOX_MAILDIR" ]; then
	sandbox_maildir="--sandbox-mdir $GLITE_LB_EXPORT_SANDBOX_MAILDIR "
fi
# pidfile
[ -n "$GLITE_JP_IMPORTER_PIDFILE" ] && pidfile="-i $GLITE_JP_IMPORTER_PIDFILE "

CERT_ARGS="-c $X509_USER_CERT -k $X509_USER_KEY"
LOGDIR=$GLITE_LOCATION_VAR
GLITE_LB_EXPORT_PURGE_ARGS=${GLITE_LB_EXPORT_PURGE_ARGS:---cleared 2d --aborted 2w --cancelled 2w --other 2m}

if [ -n "$GLITE_LB_EXPORT_JOBSDIR_KEEP" ]; then
  keep_jobs="--store ${GLITE_LB_EXPORT_JOBSDIR_KEEP} "
  [ -d $GLITE_LB_EXPORT_JOBSDIR_KEEP ] || mkdir -p $GLITE_LB_EXPORT_JOBSDIR_KEEP
fi

[ -d $LOGDIR ] || mkdir -p $LOGDIR

echo "Using cert args $CERT_ARGS"

$PREFIX/bin/glite-jp-importer --reg-mdir $GLITE_LB_EXPORT_JPREG_MAILDIR --dump-mdir $GLITE_LB_EXPORT_JPDUMP_MAILDIR $CERT_ARGS ${sandbox_maildir}-g --jpps $GLITE_LB_EXPORT_JPPS $pidfile$keep_jobs$GLITE_JP_IMPORTER_ARGS > $LOGDIR/jp-importer.log 2>&1 &

JP_PID=$!
trap "kill $JP_PID; exit 0" SIGINT

while [ 1 ]; do
  GLITE_LB_EXPORT_ENABLED="true" GLITE_LB_PURGE_ENABLED="true" $PREFIX/bin/glite-lb-export.sh

  sleep 30
done
