mkdir c:\oradata\ewdb
mkdir e:\oradata\ewdb
mkdir f:\oradata\ewdb
mkdir g:\oradata\ewdb
mkdir h:\oradata\ewdb
mkdir i:\oradata\ewdb
mkdir j:\oradata\ewdb

set ORACLE_SID=ewdb
C:\orant\bin\oradim80 -new -sid ewdb -intpwd earthworm1 -startmode auto -pfile C:\orant\database\initewdb.ora
C:\orant\bin\oradim80 -startup -sid ewdb -starttype srvc,inst -usrpwd earthworm1 -pfile C:\orant\database\initewdb.ora
C:\orant\bin\svrmgr30 @ewdbrun.sql
