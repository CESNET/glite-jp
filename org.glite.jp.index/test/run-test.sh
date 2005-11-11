#! /bin/bash

#
# test script for the index server
#
# requires running mysql
#
# configuration:
#   GLITE_JPIS_TEST_PIDFILE.........pidfile (default `pwd`/glite-jp-indexd.pid)
#   GLITE_JPIS_TEST_LOGFILE.........logfile (default `pwd`/glite-jp-indexd.log)
#   GLITE_JPIS_TEST_PORT............index server port
#   GLITE_JPIS_TEST_DB..............connection string 
#                                   (default jpis/@localhost:jpis1test,
#                                    autocreating the database when empty)
#   GLITE_JPIS_TEST_ROOT_USER.......root user (default empty)
#   GLITE_JPIS_TEST_ROOT_PASSWORD...root password (default empty)
#

# get the configuration
GLITE_LOCATION=${GLITE_LOCATION:-"/opt/glite"}
[ -f /etc/glite.conf ] && . /etc/glite.conf
[ -f $HOME/.glite.conf ] && . $HOME/.glite.conf
[ -f $GLITE_LOCATION/etc/jpis.conf ] && . $GLITE_LOCATION/etc/jpis.conf

# handle the configuration
ARGS="-u ${GLITE_JPIS_ROOT_USER:-root}"
[ -z "$GLITE_JPIS_ROOT_PASSWORD" ] || ARGS="-p ${GLITE_JPIS_ROOT_PASSWORD} $ARGS"
GLITE_JPIS_TEST_PORT=${GLITE_JPIS_TEST_PORT:-"10000"}
GLITE_JPIS_TEST_PIDFILE=${GLITE_JPIS_TEST_PIDFILE:-"`pwd`/glite-jp-indexd.pid"}
GLITE_JPIS_TEST_LOGFILE=${GLITE_JPIS_TEST_LOGFILE:-"`pwd`/glite-jp-indexd.log"}

if [ 0 ]; then
# create database when needed
if [ -z "$GLITE_JPIS_TEST_DB" ]; then
	GLITE_JPIS_TEST_DB="jpis/@localhost:jpis1test"
	mysqladmin -f $ARGS drop jpis1test > /dev/null 2>&1
	mysqladmin -f $ARGS create jpis1test && \
	mysql $ARGS -e 'GRANT ALL on jpis1test.* to jpis@localhost' && \
	mysql -u jpis jpis1test < ../config/glite-jp-index-dbsetup.sql || exit 1
	db_created="1"
fi

# run index server
GLITE_JPIS_PIDFILE=${GLITE_JPIS_TEST_PIDFILE} \
GLITE_JPIS_LOGFILE=${GLITE_JPIS_TEST_LOGFILE} \
GLITE_JPIS_DEBUG="0" \
GLITE_JPIS_DB=$GLITE_JPIS_TEST_DB \
GLITE_JPIS_PORT=$GLITE_JPIS_TEST_PORT \
	./glite-jp-indexd
if [ x"$?" != x"0" ]; then
	echo FAILED
	[ -z "$db_created" ] || mysqladmin -f $ARGS drop jpis1test
	exit 1
fi
if [ ! -s "./glite-jp-indexd.pid" ]; then
	echo "Can't startup index server."
	[ -z "$db_created" ] || mysqladmin -f $ARGS drop jpis1test
	exit 1
fi
fi

# wait for index server
ret=1
while [ x"$ret" != x"0" ]; do
	LC_ALL=C netstat -tap 2>/dev/null | grep "\<$GLITE_JPIS_TEST_PORT\>" > /dev/null
	ret=$?
	LC_ALL=C sleep 0.1
done
# run the example
numok=`GLITE_JPIS_DB=$GLITE_JPIS_TEST_DB \
GLITE_JPIS_PORT=$GLITE_JPIS_TEST_PORT \
	./glite-jpis-test | grep "OK" | wc -l`

# kill the index server
kill `cat ${GLITE_JPIS_TEST_PIDFILE}`

# drop databaze when created
[ -z "$db_created" ] || mysqladmin -f $ARGS drop jpis1test >/dev/null

if [ "$numok" -eq "2" ]; then
	echo OK
fi
