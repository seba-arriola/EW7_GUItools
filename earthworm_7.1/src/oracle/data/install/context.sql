connect SYS/change_on_install as SYSDBA
set echo on
spool logs\context.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\ctx\admin\dr0csys change_on_install DRSYS TEMP;
connect CTXSYS/change_on_install
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\ctx\admin\dr0inst $(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\bin\oractxx9.dll;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\ctx\admin\defaults\dr0defin.sql AMERICAN;
spool off
exit;
