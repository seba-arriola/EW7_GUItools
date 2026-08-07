
connect SYS/change_on_install as SYSDBA
set echo on
spool logs\set_passwords.log

REM **** Setting Password for sys ****************
ALTER USER SYS IDENTIFIED BY $(SYS_PSWD)

REM **** Setting Password for system ****************
ALTER USER SYSTEM IDENTIFIED BY $(SYSTEM_PSWD)


spool off
exit;
