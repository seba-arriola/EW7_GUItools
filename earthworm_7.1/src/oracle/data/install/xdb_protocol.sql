connect SYS/change_on_install as SYSDBA
set echo on
spool logs\xdb_protocol.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catqm.sql change_on_install XDB TEMP;
connect SYS/change_on_install as SYSDBA
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catxdbj.sql;
spool off
exit;
