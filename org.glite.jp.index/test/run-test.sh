#! /bin/bash

#
# test script for the index server
#
# requires running mysql
#
# configuration:
#   GLITE_LOCATION..................path to glite SW
#   GLOBUS_LOCATION.................path to globus SW
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

GLOBUS_LOCATION=${GLOBUS_LOCATION:-"/opt/globus"}

if [ -e "$GLOBUS_LOCATION/bin/grid-proxy-info" ] ;then
	timeleft=`$GLOBUS_LOCATION/bin/grid-proxy-info 2>&1| grep timeleft| sed 's/^.* //'`
	if [ "$timeleft" = "0:00:00" -o -z "$timeleft" ]; then 
		echo "No valid proxy cert found! Aborting."
		exit 1
	fi
fi

# handle the configuration
ARGS="-u ${GLITE_JPIS_ROOT_USER:-root}"
[ -z "$GLITE_JPIS_ROOT_PASSWORD" ] || ARGS="-p ${GLITE_JPIS_ROOT_PASSWORD} $ARGS"
GLITE_JPIS_TEST_PORT=${GLITE_JPIS_TEST_PORT:-"10000"}
GLITE_JPIS_TEST_PIDFILE=${GLITE_JPIS_TEST_PIDFILE:-"/tmp/glite-jp-indexd.pid"}
GLITE_JPIS_TEST_LOGFILE=${GLITE_JPIS_TEST_LOGFILE:-"/tmp/glite-jp-indexd.log"}

if [ -z "$GLITE_JPIS_TEST_DB" ]; then
	GLITE_JPIS_TEST_DB="jpis/@localhost:jpis1test"
	need_new_db=1;
fi
DB_USER=`echo $GLITE_JPIS_TEST_DB| sed 's!/.*$!!'`
DB_HOST=`echo $GLITE_JPIS_TEST_DB| sed 's!^.*@!!' | sed 's!:.*!!'`
DB_NAME=`echo $GLITE_JPIS_TEST_DB| sed 's!^.*:!!'`


create_db() {
	# create database when needed
	if [ "x$need_new_db" = "x1" ]; then
		mysqladmin -f $ARGS drop $DB_NAME > /dev/null 2>&1
		mysqladmin -f $ARGS create $DB_NAME && \
		mysql $ARGS -e 'GRANT ALL on $DB_NAME.* to jpis@localhost' && \
		mysql -u $DB_USER $DB_NAME < $GLITE_LOCATION/stage/etc/glite-jp-index-dbsetup.sql || exit 1
		db_created="1"
	fi
}

import_db() {
	# import database
	mysql -u $DB_USER -h $DB_HOST <$1
	if [ x"$?" != x"0" ]; then
		echo "FAILED to import database."
		kill_is;
		drop_db;
		exit 1
	fi
}

drop_db() {
	# drop databaze when created
	[ -z "$db_created" ] || mysqladmin -f $ARGS drop $DB_NAME >/dev/null

}

run_is() {
	# run index server
	$GLITE_LOCATION/stage/bin/glite-jp-indexd -m $GLITE_JPIS_TEST_DB -p $GLITE_JPIS_TEST_PORT \
			-i ${GLITE_JPIS_TEST_PIDFILE} -o ${GLITE_JPIS_TEST_LOGFILE} $1\
			2>/dev/null


	if [ x"$?" != x"0" ]; then
		echo FAILED
		drop_db;
		exit 1
	fi
	if [ ! -s "${GLITE_JPIS_TEST_PIDFILE}" ]; then
		echo "Can't startup index server."
		drop_db;
		exit 1
	fi

	# wait for index server
	ret=1
	while [ x"$ret" != x"0" ]; do
		LC_ALL=C netstat -tap 2>/dev/null | grep "\<$GLITE_JPIS_TEST_PORT\>" > /dev/null
		ret=$?
		LC_ALL=C sleep 0.1
	done
}

kill_is() {
	# kill the index server
	kill `cat ${GLITE_JPIS_TEST_PIDFILE}`

}

run_test_query() {
	$GLITE_LOCATION/stage/bin/glite-jpis-client -q $1 \
		 -i http://localhost:$GLITE_JPIS_TEST_PORT &>/tmp/result
	DIFF=`diff --ignore-matching-lines="query: using JPIS" $2 /tmp/result`
	if [ -z "$DIFF" ] ; then
		echo "OK."
		rm /tmp/result
	else
		echo "FAILED!"
		echo
		echo "Expected result:"
		cat $2
		echo "Obtained result:"
		cat /tmp/result
		rm /tmp/result
		drop_db;
		kill_is;
		exit 1
	fi
}

run_test3() {
	# run the example
	numok=`GLITE_JPIS_DB=$GLITE_JPIS_TEST_DB \
	GLITE_JPIS_PORT=$GLITE_JPIS_TEST_PORT \
		$GLITE_LOCATION/stage/bin/glite-jpis-test 2>/dev/null| grep "OK" | wc -l`
	if [ "$numok" -eq "2" ]; then
		echo OK.
	fi
}

echo

echo -n "Simple query test.... "
create_db;
run_is "-n";
import_db $GLITE_LOCATION/stage/examples/dump1.sql;
run_test_query $GLITE_LOCATION/stage/examples/simple_query.in $GLITE_LOCATION/stage/examples/simple_query.out;
drop_db;
kill_is;

echo -n "Complex query test... "
create_db;
run_is "-n";
import_db $GLITE_LOCATION/stage/examples/dump1.sql;
run_test_query $GLITE_LOCATION/stage/examples/complex_query.in $GLITE_LOCATION/stage/examples/complex_query.out;
drop_db;
kill_is;

echo -n "Feed & query test.... "
create_db;
run_is;
run_test3;
drop_db;
kill_is;


