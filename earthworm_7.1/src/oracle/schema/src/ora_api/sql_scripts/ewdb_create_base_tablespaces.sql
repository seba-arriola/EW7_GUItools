
/******************************************/
/* STEP 1:  Create tablespaces and files  */
/******************************************/

CREATE TABLESPACE EWDB_Corespace 
  DATAFILE 'g:\oradata\ewdb\core01.ora' 
    SIZE 500M AUTOEXTEND ON NEXT 100M MAXSIZE 1000M;

CREATE TABLESPACE EWDB_Infraspace 
  DATAFILE 'e:\oradata\ewdb\infra01.ora' 
    SIZE 10M AUTOEXTEND ON NEXT 10M MAXSIZE 1000M;

CREATE TABLESPACE EWDB_Snippetspace
   DATAFILE 'e:\oradata\ewdb\snippet01.ora'
     SIZE 4000M AUTOEXTEND OFF;

alter tablespace EWDB_Snippetspace add datafile 'e:\oradata\ewdb\snippet02.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'e:\oradata\ewdb\snippet03.ora' SIZE 4000M;

alter tablespace EWDB_Snippetspace add datafile 'f:\oradata\ewdb\snippet04.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'f:\oradata\ewdb\snippet05.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'f:\oradata\ewdb\snippet06.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'f:\oradata\ewdb\snippet07.ora' SIZE 4000M;

alter tablespace EWDB_Snippetspace add datafile 'g:\oradata\ewdb\snippet08.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'g:\oradata\ewdb\snippet09.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'g:\oradata\ewdb\snippet10.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'g:\oradata\ewdb\snippet11.ora' SIZE 4000M;

alter tablespace EWDB_Snippetspace add datafile 'h:\oradata\ewdb\snippet12.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'h:\oradata\ewdb\snippet13.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'h:\oradata\ewdb\snippet14.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'h:\oradata\ewdb\snippet15.ora' SIZE 4000M;

alter tablespace EWDB_Snippetspace add datafile 'i:\oradata\ewdb\snippet16.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'i:\oradata\ewdb\snippet17.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'i:\oradata\ewdb\snippet18.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'i:\oradata\ewdb\snippet19.ora' SIZE 4000M;

alter tablespace EWDB_Snippetspace add datafile 'j:\oradata\ewdb\snippet20.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'j:\oradata\ewdb\snippet21.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'j:\oradata\ewdb\snippet22.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile 'j:\oradata\ewdb\snippet23.ora' SIZE 4000M;


/******************************************/
/* STEP 2:  Create User for EWDB Schema   */
/******************************************/

CREATE USER ewdb_main IDENTIFIED BY main 
  DEFAULT TABLESPACE EWDB_CoreSpace 
  PROFILE DEFAULT 
  QUOTA UNLIMITED ON EWDB_CoreSpace 
  QUOTA UNLIMITED ON EWDB_Infraspace 
  QUOTA UNLIMITED ON EWDB_Snippetspace ;


GRANT "CONNECT" TO ewdb_main;
ALTER USER ewdb_main DEFAULT ROLE ALL;

GRANT CREATE PROCEDURE TO EWDB_MAIN;
GRANT CREATE TRIGGER TO EWDB_MAIN;


