# Set Up for MySQL Tests
The mysql tests require that a new database and a pair of tables be added to the main centrallix mysql server. This assumes that a mysql database is being used and that the default sign in for root had not been changed (that is, no password). 
Make sure the mysql driver is enabled in .../centrallix/tests/centrallix.conf-test.in and /usr/local/etc/centrallix.conf


## Database Schema
These tests require the creation of a database named "driver_test", which must contain the tables described below. 
Note that both the latin1 and the UTF-8 tables both contain the same schema. They only differ by encoding.
+------------+--------------+------+-----+---------+----------------+
| Field      | Type         | Null | Key | Default | Extra          |
+------------+--------------+------+-----+---------+----------------+
| id         | int(11)      | NO   | PRI | NULL    | auto_increment |
| string     | varchar(50)  | YES  |     | NULL    |                |
| longString | varchar(250) | YES  |     | NULL    |                |
| date       | date         | YES  |     | NULL    |                |
| number     | int(11)      | YES  |     | NULL    |                |
| raw        | blob         | YES  |     | NULL    |                |
| text       | mediumtext   | YES  |     | NULL    |                |
+------------+--------------+------+-----+---------+----------------+
### **latin1:**
CREATE TABLE latin1 (id INT PRIMARY KEY AUTO_INCREMENT, string VARCHAR(50), longString VARCHAR(250), date DATE, number INT, raw BLOB, text TEXT);

**NOTE:** The above command assumes that the default encoding for tables is latin1. Otherwise, set it manually.

### **utf8:**
CREATE TABLE utf8 (id INT PRIMARY KEY AUTO_INCREMENT, string VARCHAR(50), longString VARCHAR(250), date DATE, number INT, raw BLOB, text TEXT);
alter table utf8 convert to character set utf8mb4;

**NOTE:** make sure to set the above table to utf8mb4; utf8 is an alias for utf8mb3, which is depreciated. 

## Useful queries:
The following two queries can be helpful to confirm the table character sets are what they should be. 

SELECT T.table_name, CCSA.character_set_name FROM information_schema.`TABLES` T,
       information_schema.`COLLATION_CHARACTER_SET_APPLICABILITY` CCSA
WHERE CCSA.collation_name = T.table_collation
  AND T.table_schema = "driver_test";

SELECT table_name, column_name, character_set_name FROM information_schema.`COLUMNS` 
WHERE table_schema = "driver_test";

## To connect to the database: 
The following shell commands should be helpful with conecting with the database to run commands without going through centrallix. This assumes the server is running locally. 

 - **Running:** .../bin/mysqld_safe &
 - **Stopping:** .../bin/mysqladmin -u root -p shutdown
 - **Connect:** mysql -u root -p 
 - **Connect (utf-8 support):**  mysql -u root --default-character-set=utf8mb4