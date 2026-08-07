connect SYS/change_on_install as SYSDBA
set echo on
spool logs\CreateDBCatalog.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catalog.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catexp7.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catblock.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catproc.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\catoctk.sql;
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\rdbms\admin\owminst.plb;
connect SYSTEM/manager
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\sqlplus\admin\pupbld.sql;
connect SYSTEM/manager
set echo on
spool off
spool logs\sqlPlusHelp.log
@$(EW_ORACLE_BASE)\$(EW_ORACLE_HOME)\sqlplus\admin\help\hlpbld.sql helpus.sql;
spool off
exit;
