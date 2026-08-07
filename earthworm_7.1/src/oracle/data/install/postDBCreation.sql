connect SYS/change_on_install as SYSDBA
set echo on
spool logs\postDBCreation.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\utlrp.sql;
shutdown ;
connect SYS/change_on_install as SYSDBA
set echo on
create spfile='$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\database\spfile$(EW_ORACLE_SID).ora' FROM pfile='$(SCRIPT_RUN_DIRECTORY)\init$(EW_ORACLE_SID).ora';
startup ;
exit;

