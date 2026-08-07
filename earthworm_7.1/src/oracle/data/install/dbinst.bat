mkdir logs
mkdir $(EW_ORACLE_BASE)\admin\$(EW_ORACLE_SID)\bdump
mkdir $(EW_ORACLE_BASE)\admin\$(EW_ORACLE_SID)\cdump
mkdir $(EW_ORACLE_BASE)\admin\$(EW_ORACLE_SID)\create
mkdir $(EW_ORACLE_BASE)\admin\$(EW_ORACLE_SID)\pfile
mkdir $(EW_ORACLE_BASE)\admin\$(EW_ORACLE_SID)\udump
mkdir $(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\database
mkdir $(EW_ORACLE_BASE)\oradata\$(EW_ORACLE_SID)
set ORACLE_SID=$(EW_ORACLE_SID)
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\oradim.exe -new  -sid $(EW_ORACLE_SID) -startmode m 
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\oradim.exe -edit  -sid $(EW_ORACLE_SID) -startmode a 
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\orapwd.exe file=$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\database\PWD$(EW_ORACLE_SID).ora password=change_on_install
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @CreateDB.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @CreateDBFiles.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @CreateDBCatalog.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @JServer.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @ordinst.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @interMedia.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @context.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @xdb_protocol.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @postDBCreation.sql
REM uncomment next line for pre 9i, or non-automatic undo mode
REM $(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @ewdb_create_rollbacks.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @ewdb_create_base_tablespaces.sql
$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\sqlplus /nolog @ewdb_set_passwords.sql


