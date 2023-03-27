# Set Up for Sybase Tests
The sybase tests require that a new database with five tables be added to the main centrallix sybase server. This assumes that a sybase database is being used. Make sure the following are all completed: 
The Test_DB.sybdb file will have to be edited to ensure that valid sign in credentials have been included. 
Make sure the sybase driver is enabled in .../centrallix/tests/centrallix.conf-test.in and /usr/local/etc/centrallix.conf
Set the LANG enviremnt var to be set to something valid for sybase. This is true both for accessing via isql, as well as via test_obj
Example: export LANG=us_english.utf8 

<br>

## Database Schema
These tests require the creation of the following tables, described below:

**NOTE:** The database user used to connect to the database will need to be granted access to all of the tables described below

### **basic:** <BR>
This table uses various text fields.

**Create:** <BR>
`CREATE TABLE basic (id INT PRIMARY KEY, string VARCHAR(30) NULL, longString VARCHAR(100) NULL, number INT NULL, text TEXT NULL)` <br>
`GRANT all ON basic TO devel`

**Data:** <BR>
All of the data for the table is inserted, read, and deleted by the centrallix tests. No data needs to be initialized. 

**Schema:**
Column_name|Type|Length 
|-----------|-------|------ 
id|int|4
string|varchar|30 
longString|varchar|100 
number|int|4 
text|text|16 

<br>

### **invalid:** <br>
This table is the same as basic, but contains data that is meant to only be read rather than having inserts and deletes performed like the basic table. 

**Create:** <br>
`CREATE TABLE invalid (id INT PRIMARY KEY, string VARCHAR(30) NULL, longString VARCHAR(100) NULL, number INT NULL, text TEXT NULL)` <br>
`GRANT all ON invalid TO devel`

**Data:** <br>
The best way to get the invalid tests loaded in is to temporarily edit the verifiy to allow anything, use the test_obj commands to upload the invalid text, and then switch it back. Note that the '�' character represents a '\xFF' character. In this document it has been converted to the '0xef 0xbf 0xbd' character; it is currently valid UTF-8, and will need replaced with an invalid character. 

- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="1 byte ascii", longString="A longer�example of ascii", number=1, text="even more ascii text"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="1 byte ascii", longString="A longer example of ascii", number=1, text="even more�ascii text"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="2 byte UTF-8: Своего", longString="ΆΈΉΊΌΎΏΐΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨ�ΪΫάέήίΰαβγδεζηθικ", number=2, text="ͰͱͲͳʹ͵Ͷͷͺͻͼͽ;΄"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="2 byte UTF-8: Своего", longString="ΆΈΉΊΌΎΏΐΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΪΫάέήίΰαβγδεζηθικ", number=2, text="ͰͱͲͳʹ�͵Ͷͷͺͻͼͽ;΄"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="3 byte UTF-8: 実に神は", longString="え惜しまず与えるほどにこの�界を愛してくださいました。それは", number=3, text="実に神はひとり子"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="3 byte UTF-8: 実に神は", longString="え惜しまず与えるほどにこの界を愛してくださいました。それは", number=3, text="実に神は�ひとり子"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="4 byte UTF-8: 𓀀𓀁𓀂", longString="𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀙𓀛𓀜𓀝�𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦", number=4, text="𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊𓀋𓀌"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="4 byte UTF-8: 𓀀𓀁𓀂", longString="𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀙𓀛𓀜𓀝𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦", number=4, text="𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇�𓀈𓀉𓀊𓀋𓀌"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="test of utf-8 planes", longString="|�￦|𐊌🫖|𠀀蜨|𰀀𲎯|�|", number=5, text="|�￦|𐊌🫖|𠀀蜨|𰀀𲎯|"`
- `query insert into object /tests/Test_DB.sybdb/invalid/rows/* select string="test of utf-8 planes", longString="|�￦|𐊌🫖|𠀀蜨|𰀀𲎯|", number=5, text="|�￦|𐊌🫖|𠀀蜨|𰀀𲎯|�|"`

**Schema**
Column_name|Type|Length 
-----------|-------|------
 id|int|4 
 string|varchar|30 
 longString|varchar|100 
 number|int|4 
 text|text|16 

**NOTE:** Inserting invalid data intially is tricky. Commands lines tend to not like invalid UTF-8 characters like '\xFF'. The best way I have found to insert invalid characters is by disabling the verifyUTF8 function (by temporariliy editing it in util.c to always retun success), inserting the invalid commands through the test_obj command, and then reverting the changes to verifyUTF8. This should only need done once. It is worth noting that becase the reads from the table should fail, the exact contents do NOT need to match the data used here, though reprsentative examples which test the same types of situations should be supplied. Hex editing a .to file is probably the simplest way to generate an invalid character.

<br>

### **long** 
This table works like basic; the data is written to, read, and cleared out by the tests. The table only needs created. 

**Create:** <br>
`CREATE TABLE long (id INT PRIMARY KEY, text TEXT)` <br>
`GRANT all ON long TO devel`

**Data:** <br>
The data is read from centrallix-os/tests/long_utf8.json. This enables strings longer than the test_obj command parser allows to be inserted into the database. 


**Schema** <br>
Column_name|Type|Length 
-----------|----|------
id|int|4
text|text|16

<br>

## **noText**
This table is used to test non-text datatypes. The primary purpose of these tests is to ensure centrallix does not perform verification on data which will not be interpreted as a string. This would likely be useful for other tests, however. 

**Create:** <br>
`CREATE TABLE noText (id INT PRIMARY KEY, int INT, float FLOAT, bit BIT, img IMAGE)` <br>
`GRANT all ON noText TO devel`


**Data:** <br>
The test_obj system is not able to insert into IMAGE columns, so the data must be supplied manually. 
- `INSERT INTO noText VALUES (1, 0xff, 123.123, 1, 0x736F6D6520FF2074657874)`
- `INSERT INTO noText VALUES (2, 0xffff, 123.123, 0, 0xce86ce88ce89ce8ace8cce8ece8fce90ce91ce92ff)`
- `INSERT INTO noText VALUES (3, 0x6869ff, 123.123, -1, 0xe5ae9fe381abe7a59ee381afe381b2e381a8ff)`
- `INSERT INTO noText VALUES (4, 0xe5ae9fff, 123.123, 0xff, 0xf0938190f0938191f0938192f09fa697ff)`

**Schema** <br>
|Column_name|Type  |Length
|-----------|------|------
|id|int|4
|int|int|4
|float|float|8
|bit|bit|1
|img|image|16


<br>

## **link** 
This table is used to check cases where foreign key constraints are needed for tests. 


**Create:**<br>
`CREATE TABLE link (id INT PRIMARY KEY, basic_ID INT REFERENCES basic(id), invalid_ID INT REFERENCES invalid(id))` <br>
`GRANT all ON link TO devel`


**Data**<br>
Data insertion is handled by the test_obj script.


**Schema**<br>
|Column_name|Type  |Length
|-----------|------|------
|id|int|4
|basic_ID|int|4
|invalid_ID|int|4

<br>

## **block**
This table is just used to test when devel does not have access to a table. Like link, block need not contain any data.

**Create:** <br>
`CREATE TABLE block (id INT PRIMARY KEY, num INT)`<br>
Make sure devel does not have access to read from the table. 

## **If the Tests Break**
The tests will occasionally break for various reasons that are not caused by the code. If you encounter errrors that do not seem to be the result of changes you made, you can try the following:
- Check if the database's transaction log is full. If it is, deletes and inserts will fail. Look into running a dump transaction command.
- Check if you have granted permissions to the relevant tables for the account runing the tests to the database. 
- If the sybase driver is not enabled, run the configure script with `--enable-sybase`, and make sure the centrallix.conf file has received the correct setting. 
	- it is likely that it might fail to find the path. If so, also include `--with-sybase=<path to ocs-16_0/lib>`
- If both the tests and the isql command will not work, try changing the LANG envirement variable to a value that sybase excepts
- If the commands work in test_obj but not with make test, make sure both centrallix.conf-test and centrallix.conf-test.in are enabling sybase. 
	- you may have to set charsetmap_file = "##SYSCONFDIR##/centrallix/charsetmap.cfg";, or whatever path works for the local repo.

## To connect to the database: 
The following shell commands should be helpful with conecting with the database to run commands without going through centrallix. This assumes the sybase server and/or client is set up properly.  

 - **Connect:** `isql -SSYBTEST -Usa`