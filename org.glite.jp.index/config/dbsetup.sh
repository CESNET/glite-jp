#! /bin/sh

#
# Shell example of preparing the database for JP Index Server
#

# database
mysqladmin -u root -p create jpis1

# user
mysql -u root -p -e 'GRANT ALL on jpis1.* to jpis@localhost'

# tables
mysql -u jpis jpis1 < `dirname $0`/glite-jp-index-dbsetup.sql
