#! /bin/sh

#
# Shell example of preparing the database for JP Index Server
#

# database
mysqladmin -u root -p create jpis

# user
mysql -u root -p -e 'GRANT ALL on jpis.* to jpis@localhost'

# tables
mysql -u jpis jpis < `dirname $0`/glite-jp-index-dbsetup.sql
