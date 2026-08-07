# Create an Earthworm environment on Solaris!
# This file should be sourced by any shell wanting 
# to run or build EARTHWORM system.

# Set environment variables describing your Earthworm directory/version
setenv EW_HOME          /home/earthworm
setenv EW_VERSION       working-will
setenv SYS_NAME         `hostname`

# Set environment variables used by earthworm modules at run-time
# Path names must end with the slash "/"
setenv EW_INSTALLATION  INST_UNKNOWN
setenv EW_PARAMS        ${EW_HOME}/run-will/params/
setenv EW_LOG           ${EW_HOME}/run-will/log/

#
# Database (Oracle) related environment
#
setenv SCHEMA_DIR       ${EW_HOME}/${EW_VERSION}/src/oracle/schema
setenv APPS_DIR         ${EW_HOME}/${EW_VERSION}/src/oracle/apps
setenv WEB_DOC_DIR      ${EW_HOME}/${EW_VERSION}/WEB_DOC/DBMS/API-DOC

#
# Web server related environment
#
setenv WEB_DIR          ${EW_HOME}/web

# Tack the earthworm bin directory in front of the current path
# Also add Oracle paths to the current path.
set path=( ${EW_HOME}/${EW_VERSION}/bin /opt/oracle/bin /var/opt/oracle $path )

# Set up library path for dynamically loaded libraries:
setenv OPENWINHOME   /usr/openwin
setenv ORACLE_HOME   /opt/oracle
setenv COMPILER_DIR  /opt/SUNWspro
setenv LD_LIBRARY_PATH "${OPENWINHOME}/lib:${ORACLE_HOME}/lib:${COMPILER_DIR}/lib:/usr/lib"

# Set environment variables for compiling earthworm modules
setenv GLOBALFLAGS "-D_INTEL -D_SOLARIS -I${EW_HOME}/${EW_VERSION}/include -I${SCHEMA_DIR}/src/include -I${SCHEMA_DIR}/src/include/internal -I${APPS_DIR}/src/include -I${ORACLE_HOME}/rdbms/demo"

# Create an alias for making executables
alias emake   'make -f makefile.sol'
alias make_ew 'make_ew_solaris'

# Number of available file descriptors
limit descriptors 256
