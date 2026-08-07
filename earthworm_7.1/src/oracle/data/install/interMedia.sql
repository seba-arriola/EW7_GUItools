connect SYS/change_on_install as SYSDBA
set echo on
spool logs\interMedia.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\ord\im\admin\iminst.sql;
spool off
exit;
