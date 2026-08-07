connect SYS/change_on_install as SYSDBA

set echo on
spool logs\ewdb_create_tablespaces.log

/******************************************/
/* STEP 1:  Create tablespaces and files  */
/******************************************/

CREATE TABLESPACE EWDB_Corespace 
  DATAFILE '$(EW_DATA_CORE)\core01.ora' 
    SIZE 500M AUTOEXTEND ON NEXT 100M MAXSIZE 1000M;

CREATE TABLESPACE EWDB_Infraspace 
  DATAFILE '$(EW_DATA_INFRA)\infra01.ora' 
    SIZE 10M AUTOEXTEND ON NEXT 10M MAXSIZE 1000M;

CREATE TABLESPACE EWDB_Snippetspace
   DATAFILE '$(EW_DATA_WAVEFORM_1)\snippet01.ora'
     SIZE 4000M AUTOEXTEND OFF;

alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_1)\snippet02.ora' SIZE 4000M;

alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_2)\snippet03.ora' SIZE 4000M;
alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_2)\snippet04.ora' SIZE 4000M;

/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_2)\snippet05.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_2)\snippet06.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_2)\snippet07.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_2)\snippet08.ora' SIZE 4000M; */

/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_3)\snippet09.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_3)\snippet10.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_3)\snippet11.ora' SIZE 4000M; */

/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_4)\snippet12.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_4)\snippet13.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_4)\snippet14.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_4)\snippet15.ora' SIZE 4000M; */

/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_5)\snippet16.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_5)\snippet17.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_5)\snippet18.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_5)\snippet19.ora' SIZE 4000M; */

/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_6)\snippet20.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_6)\snippet21.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_6)\snippet22.ora' SIZE 4000M; */
/* alter tablespace EWDB_Snippetspace add datafile '$(EW_DATA_WAVEFORM_6)\snippet23.ora' SIZE 4000M; */


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


spool off
exit;
