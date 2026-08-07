connect SYS/change_on_install as SYSDBA
set echo on
spool logs\JServer.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\javavm\install\initjvm.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\xdk\admin\initxml.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\xdk\admin\xmlja.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catjava.sql;
spool off
exit;
