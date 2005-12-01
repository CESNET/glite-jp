#! /bin/sh

#
# Shell example of preparing the database for JP Index Server
#

# database
mysqladmin -u root -p create jpps

# user
mysql -u root -p -e 'GRANT ALL on jpps.* to jpps@localhost'

# tables
mysql -u jpps jpps < `dirname $0`/glite-jp-primary-dbsetup.sql
