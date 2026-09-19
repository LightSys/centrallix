#!/bin/bash

####  Centrallix Test Database Setup
####
####  Copyright (C) 2024 by LightSys Technology Services, Inc.
####
####  FREE SOFTWARE, NO WARRANTY: This program is Free Software, licensed
####  under version 2 of the GNU General Public License, or any later
####  version of the GNU GPL published by the Free Software Foundation, at
####  your option.  This program is provided AS-IS and with ABSOLUTELY NO
####  WARRANTY.  See the file 'COPYING' provided with this software for
####  more information about the rights granted to you under this license.
####
####
####  This script creates and intializes a database and two tables for use by the automated
####  tests for either the Sybase or MySQL driver. 
####  This script relies on the following eviroment variables: 
####	$DB_USER - the username for login. Must be a super user.
####	$DB_PASS - the password for login
####	$DB_TEST_USER - the user that will be granted permissions to the tables for testing
####	$DB_NAME - name of the database server (sybase only)
####
#### to use it, simply run: 
####	./test_db_setup.sh <DATABASE-TYPE>


# check which database to use
if [ $# != 1 ]
then
	echo "ERROR: Please enter exactly one database type to run this script against"
	exit
fi

if [ ${1,,} = "sybase" ]; then
	# create the new database and add the tables
	echo "running against Sybase"
	printf "CREATE DATABASE Test_DB\ngo\n" | isql -U$DB_USER -P$DB_PASS -S$DB_NAME
	printf "CREATE TABLE typeTests (dates datetime, monies money, ints int, strings varchar(30), bits bit, floats float NULL, PRIMARY KEY(dates, monies, ints, strings))\nGO\n"| isql -U$DB_USER -P$DB_PASS -S$DB_NAME -DTest_DB
	printf "sp_adduser $DB_TEST_USER\nGRANT all ON typeTests TO $DB_TEST_USER\nGO\n" | isql -U$DB_USER -P$DB_PASS -S$DB_NAME -DTest_DB
	printf "CREATE TABLE stringTests (a varchar(32), b varchar(32), c varchar(32), d varchar(32), PRIMARY KEY(a, b, c))\nGO\n" | isql -U$DB_USER -P$DB_PASS -S$DB_NAME -DTest_DB
	printf "GRANT all ON stringTests TO $DB_TEST_USER\nGO\n" | isql -U$DB_USER -P$DB_PASS -S$DB_NAME -DTest_DB
elif [ ${1,,} = "mysql" ]; then
	# create the new database and add the tables
	echo "running against mysql"
	mysql -u$DB_USER -p$DB_PASS -e 'CREATE DATABASE Test_DB'
	mysql -u$DB_USER -p$DB_PASS -DTest_DB -e 'CREATE TABLE typeTests (dates datetime, monies decimal(14,4), ints int, strings varchar(30), bits bit, floats float NULL, blobs blob NULL, PRIMARY KEY(dates, monies, ints, strings))'
	mysql -u$DB_USER -p$DB_PASS -DTest_DB -e 'GRANT ALL ON typeTests TO $DB_TEST_USER'
	mysql -u$DB_USER -p$DB_PASS -DTest_DB -e 'CREATE TABLE stringTests (a varchar(32), b varchar(32), c varchar(32), d varchar(32), PRIMARY KEY(a, b, c))'
	mysql -u$DB_USER -p$DB_PASS -DTest_DB -e 'GRANT ALL ON stringTests TO $DB_TEST_USER'
else
	echo "ERROR: cannot support database \"$1\". Please specify either sybase or mysql"
	exit
fi
