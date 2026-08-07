connect SYS/change_on_install as SYSDBA
set echo on
spool logs\ordinst.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\ord\admin\ordinst.sql;
spool off
exit;
