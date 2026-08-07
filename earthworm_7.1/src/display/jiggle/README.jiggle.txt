README.jiggle.txt

#### NOTE
The jiggle zip file was last updated 2004/01/07.
When running Jiggle, that date should show up as
part of the version in the title bar.
DK 2004/01/07

This file contains instructions to install and run the copy
of Jiggle supplied with the Earthworm v6.2 release.

####################
Installing Jiggle
####################

To install Jiggle, do the following:

1) Install the Java Runtime Environment(1.3.1) if it is not
already installed.  (Run j2re-1_3_1_02-win.exe to install it
from the jiggle directory onto a MS Windows PC).

2) Create a jiggle home directory, this can be anywhere you like.
This directory will be your JIGGLE_HOME directory.

3) Copy all of the jiggle files (except for the source zip)
into your JIGGLE_HOME directory.

4) From your JIGGLE_HOME directory, unzip the jiggle_props.zip file.  
This will create a ".jiggle" subdirectory with the default properties
files.

5) Edit the "jiggle.bat" file in your JIGGLE_HOME directory:
	a) set "JIGGLE_HOME" to your JIGGLE_HOME directory
	b) set "JIGGLE_USER_NAME" to the author name the DB author name
		that you wish to use with Jiggle.
	c) set "JIGGLE_USER_HOMEDIR" to your "JIGGLE_HOME" directory.


6) You need access to a location server.  This is a program that runs on top of hypoinverse
or some other locator, and reads/writes from/to a TCP socket.  Consult 
the Earthworm distributor for a copy of this program, if you cannot get access
to an existing server for your area.

7) Edit the JIGGLE_HOME\.jiggle\properties file
You will need to change the following lines in the default properties file
to customize it to your use.  Normally it helps to modify all properties from
within the Jiggle program, but it is easier to modify these by hand for your
first use.

channelCacheFilename=c\:\\jiggle\\.jiggle\\channelList.cache
	replace c:\jiggle with your JIGGLE_HOME directory.  Note escape characters
dbaseDomain=<DB_DOMAIN_NAME>
	set this to the domainname of the machine running your EW database
dbaseHost=<DB_HOSTNAME>
	set this to the hostname of the machine running your EW database
dbasePort=<DB_PORT>
	set this to the TCP port to which your EW database listens (normally 1521)
dbaseName=<DB_SID>
	set this to the ORACLE SID of your database instance.  Get this from your
	database administrator, or try "EQS" if you are the DBA.
dbaseUser=<DB_USER>
	set this to the Earthworm DB user.  In general this is the same Oracle user that
	your other EW programs use to access the database.
dbasePasswd=<DB_PASSWD>
	set this to the Earthworm DB user's Password.

locationEngineAddress=<LOCATION_SERVER>
	set this to the fully qualified hostname of the machine that runs your
	location server (solserver).

locationEnginePort=6500
	set this to the TCP port to which your location server listens.  6500 is
	the default.

localNetCode=<NETWORK_CODE>
	set this to your seismic network code.  Example: NC for Northern California


example:
channelCacheFilename=c\:\\jiggle\\.jiggle\\channelList.cache
dbaseDomain=cr.usgs.gov
dbaseHost=db_server
dbasePort=1521
dbaseName=EQS
dbaseUser=db_user
dbasePasswd=db_password
locationEngineAddress=locator.cr.usgs.gov
locationEnginePort=6500
localNetCode=US



<DONE>


####################
Running Jiggle
####################

Once Jiggle is properly installed, you run it by executing the jiggle.bat file
in your JIGGLE_HOME directory.


####################
Blame
####################

David Kragness
2003/02/12


