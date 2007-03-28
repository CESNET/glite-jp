#! /bin/bash

#
# test script for the index server
#
# requires running mysql
#

usage() {
cat <<EOF

 ./run-test.sh

 non-default configuration is possible via following env variables:
   GLITE_LOCATION..................path to glite SW
   GLOBUS_LOCATION.................path to globus SW
   GLITE_HOST_CERT.................path to host certificate
   GLITE_HOST_KEY..................path to host key
   GLITE_JPIS_TEST_PIDFILE.........pidfile (default \`pwd\`/glite-jp-indexd.pid)
   GLITE_JPIS_TEST_LOGFILE.........logfile (default \`pwd\`/glite-jp-indexd.log)
   GLITE_JPIS_TEST_PORT............index server port
   GLITE_JPIS_TEST_DB..............connection string 
                                   (default jpis/@localhost:jpis1test,
                                    autocreating the database when empty)
   GLITE_JPIS_TEST_ROOT_USER.......root user for mysqladmin (default empty)
   GLITE_JPIS_TEST_ROOT_PASSWORD...root password mysqladmin (default empty)

EOF
}

init() {
	# get the configuration
	GLITE_LOCATION=${GLITE_LOCATION:-"/opt/glite"}
	[ -f /etc/glite.conf ] && . /etc/glite.conf
	[ -f $HOME/.glite.conf ] && . $HOME/.glite.conf
	[ -f $GLITE_LOCATION/etc/jpis.conf ] && . $GLITE_LOCATION/etc/jpis.conf

	GLOBUS_LOCATION=${GLOBUS_LOCATION:-"/opt/globus"}

	if [ -n "$GLITE_HOST_CERT" -a -n "$GLITE_HOST_KEY" ] ;then
		X509_USER_CERT="$GLITE_HOST_CERT"
		X509_USER_KEY="$GLITE_HOST_KEY"
	fi

	if [ -z "$X509_USER_CERT" -o -z "$X509_USER_KEY" ] ; then
		if [ -e "$GLOBUS_LOCATION/bin/grid-proxy-info" ] ; then
			timeleft=`$GLOBUS_LOCATION/bin/grid-proxy-info 2>&1| \
				grep timeleft| sed 's/^.* //'`
			if [ "$timeleft" = "0:00:00" -o -z "$timeleft" ]; then 
				echo "No valid proxy cert found nor "\
				"GLITE_HOST_KEY/GLITE_HOST_KEY specified!"\
				" Aborting."
				exit 1
			fi
		else
			echo "Can't check proxy cert (grid-proxy-info not found). If you do not have valid proxy certificate, set GLITE_HOST_KEY/GLITE_HOST_KEY - otherwise tests will fail!"
		fi
	fi

	# handle the configuration
	ARGS="-u ${GLITE_JPIS_TEST_ROOT_USER:-root}"
	[ -z "$GLITE_JPIS_TEST_ROOT_PASSWORD" ] || ARGS="--password=${GLITE_JPIS_TEST_ROOT_PASSWORD} $ARGS"
	GLITE_JPIS_TEST_PORT=${GLITE_JPIS_TEST_PORT:-"10000"}
	GLITE_JPIS_TEST_PIDFILE=${GLITE_JPIS_TEST_PIDFILE:-"/tmp/glite-jp-indexd.pid"}
	GLITE_JPIS_TEST_LOGFILE=${GLITE_JPIS_TEST_LOGFILE:-"/tmp/glite-jp-indexd.log"}
	GLITE_JPIS_TEST_CONFIG=${GLITE_JPIS_TEST_CONFIG:-"$GLITE_LOCATION/etc/glite-jpis-config.xml"}

	if [ -z "$GLITE_JPIS_TEST_DB" ]; then
		GLITE_JPIS_TEST_DB="jpis/@localhost:jpis1test"
		need_new_db=1;
	fi
	DB_USER=`echo $GLITE_JPIS_TEST_DB| sed 's!/.*$!!'`
	DB_HOST=`echo $GLITE_JPIS_TEST_DB| sed 's!^.*@!!' | sed 's!:.*!!'`
	DB_NAME=`echo $GLITE_JPIS_TEST_DB| sed 's!^.*:!!'`

	GLITE_JPIS_DEBUG=0
}

create_db() {
	# create database when needed
	if [ "x$need_new_db" = "x1" ]; then
		mysqladmin -f $ARGS drop $DB_NAME > /dev/null 2>&1
		mysqladmin -f $ARGS create $DB_NAME && \
		mysql $ARGS -e "GRANT ALL on $DB_NAME.* to jpis@localhost" && \
		mysql -u $DB_USER $DB_NAME < $GLITE_LOCATION/etc/glite-jp-index-dbsetup.sql || exit 1
		db_created="1"
	fi
}

import_db() {
	# import database
	cat $1 | sed "s/jpis1test/$DB_NAME/" | mysql -u $DB_USER -h $DB_HOST
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
	# check
	if [ -f "${GLITE_JPIS_TEST_PIDFILE}" ]; then
		echo "Index server already running!"
		echo "  pid $(cat ${GLITE_JPIS_TEST_PIDFILE})"
		echo "  pidfile ${GLITE_JPIS_TEST_PIDFILE}"
		exit 1
	fi

	# run index server
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} \
	$GLITE_LOCATION/bin/glite-jp-indexd -m $GLITE_JPIS_TEST_DB -p $GLITE_JPIS_TEST_PORT \
			-i ${GLITE_JPIS_TEST_PIDFILE} -o ${GLITE_JPIS_TEST_LOGFILE} \
			-x ${GLITE_JPIS_TEST_CONFIG} $1\
			2>/tmp/result

	if [ x"$?" != x"0" ]; then
		echo FAILED
		drop_db;
		exit 1
	fi
	if [ ! -s "${GLITE_JPIS_TEST_PIDFILE}" ]; then
		sleep 1
	fi
	if [ ! -s "${GLITE_JPIS_TEST_PIDFILE}" ]; then
		echo "Can't startup index server."
		drop_db;
		exit 1
	fi

	# wait for index server
	ret=1
	i=0
	while [ x"$ret" != x"0" -a $i -lt 20 ]; do
		LC_ALL=C netstat -tap 2>/dev/null | grep "\<$GLITE_JPIS_TEST_PORT\>" > /dev/null
		ret=$?
		i=$(($i+1))
		LC_ALL=C sleep 0.1
	done
}

kill_is() {
	# kill the index server
	kill `cat ${GLITE_JPIS_TEST_PIDFILE}`;
	sleep 1;
	kill -9 `cat ${GLITE_JPIS_TEST_PIDFILE}` 2>/dev/null
	rm -f ${GLITE_JPIS_TEST_PIDFILE}
}

run_test_query() {
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} \
	$GLITE_LOCATION/examples/glite-jpis-client -f strippedxml -q $1 \
		 -i http://localhost:$GLITE_JPIS_TEST_PORT 2>&1 | sed -e 's,<?xml version="1.0" encoding="UTF-8"?>,,' -e 's, SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/",,' > /tmp/result
	DIFF=`diff -b -B --ignore-matching-lines="query: using JPIS" $2 /tmp/result`
	if [ -z "$DIFF" -a "$?" -eq "0" ] ; then
		echo "OK."
		rm /tmp/result
	else
		echo "FAILED!"
		echo
		echo "Expected result (using query $1):"
		echo ------------------------------------------------------------------------------
		cat $2
		echo
		echo ---------------------------------------------------------------------------------------------------
		echo
		echo "Obtained result (in /tmp/result):"
		echo ---------------------------------
		cat /tmp/result
		echo
		echo ---------------------------------------------------------------------------------------------------
		drop_db;
		kill_is;
		exit 1
	fi
}

run_test_feed() {
	# run the example
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT}\
                $GLITE_LOCATION/examples/glite-jpis-test -p $GLITE_JPIS_TEST_PORT \
                -m $GLITE_JPIS_TEST_DB -x $GLITE_JPIS_TEST_CONFIG &>/tmp/result
	numok="$(cat /tmp/result | grep -c OK)"
	if [ "$numok" -eq "2" ]; then
		echo OK.
	else
		echo FAILED!
		echo ---------------------------------------------------------------------------------------------------
		echo
		echo "Obtained result (in /tmp/result):"
		echo ---------------------------------
		cat /tmp/result
		echo
		echo ---------------------------------------------------------------------------------------------------
		drop_db;
		kill_is;
		exit 1
	fi
}


##########################################################################
#

if [ "$1" ]; then usage; exit 1; fi
init;

echo

echo -n "Simple query test.... "
create_db;
run_is "-n";
import_db $GLITE_LOCATION/examples/query-tests/dump1.sql;
run_test_query $GLITE_LOCATION/examples/query-tests/simple_query.in $GLITE_LOCATION/examples/query-tests/simple_query.out;
drop_db;
kill_is;

echo -n "Complex query test... "
create_db;
run_is "-n";
import_db $GLITE_LOCATION/examples/query-tests/dump1.sql;
run_test_query $GLITE_LOCATION/examples/query-tests/complex_query.in $GLITE_LOCATION/examples/query-tests/complex_query.out;
drop_db;
kill_is;

echo -n "Feed & query test.... "
create_db;
run_is;
run_test_feed;
drop_db;
kill_is;

echo -n "Authz test........... "
create_db;
run_is;
import_db $GLITE_LOCATION/examples/query-tests/dump1.sql;
run_test_query $GLITE_LOCATION/examples/query-tests/simple_query.in $GLITE_LOCATION/examples/query-tests/authz.out;
drop_db;
kill_is;

echo -n "Query jobId test........... "
create_db;
run_is "-n";
import_db $GLITE_LOCATION/examples/query-tests/dump1.sql;
run_test_query $GLITE_LOCATION/examples/query-tests/jobid_query.in $GLITE_LOCATION/examples/query-tests/jobid_query.out;
drop_db;
kill_is;

echo -n "Origin test........... "
create_db;
run_is "-n";
import_db $GLITE_LOCATION/examples/query-tests/dump1.sql;
run_test_query $GLITE_LOCATION/examples/query-tests/origin_query.in $GLITE_LOCATION/examples/query-tests/origin_query.out;
drop_db;
kill_is;
