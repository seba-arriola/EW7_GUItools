
==========================================================================
    INSTALLATION CHECKLIST FOR INSTALLING AND CONFIGURING EARTHWORM V6 
                       ON A SINGLE NT MACHINE

    Caveat Installer: These instructions presuppose a computer-
      savvy installer, as well as one who is intimate with Earthworm 
      setup. This is not a step-by-step guide for the meek and easily 
      intimidated. It is meant to be a reminder of major steps 
      that must be taken to get Earthworm v6 to run all on NT.

      It is also presumed that the installer has access to the rest of 
      the Earthworm documentation, specifically that which deals in detail 
      with installation and configuration of Quick Review and Alarms systems.
  
                                             Lucky Vidmar, November 2001
==========================================================================

[ ] Download the utilities (from gldrocky.cr.usgs.gov /home/earthworm/EW6_utils/nt into a separate directory. It should contain at 
    least the following:
        zip.exe
        unzip.exe
        vnc-xxx.zip
        cygwin.zip
        gmt.zip
        gs.zip
        netcdf.zip
        imagemagick.zip
        instmsi.exe
        apache_x.xxx.msi
        httpd.conf
        listener.ora
        tnsnames.ora

[ ] Install VNC and start the service
      unzip vnc-xx.zip
      Run Setup.exe from the vnc-xxx\winvnc directory
      Install and Start the service from the 
                Start->Programs->Vnc->Administrative Tools menu

[ ] Set up Autostart for user earthworm -- all remaining steps 
    must be completed while logged in as user earthworm.

[ ] Setup system paths 

  From the Control Panel, click on System
  Click the Environment tab
  In the System Variables window
    highlight the Path variable
    in the Value field, add the following:
      c:\local\bin;c:\gmt\bin;c:\netcdf\bin;c:\gs\gs7.00\bin;
      c:\Program Files\Apache Group\Apache\bin;c:\cygwin\bin
    Click Set

    Add the variable TZ and set its value to GMT
    Add the variable EW_LOG and set its value to c:\earthworm\web\log

[ ] Download the Earthworm distribution, along with the latest patch

[ ] Install Earthworm v6, remember to apply the latest patch. Create a \earthworm\web. Move all from \web_sample to there.

[ ] Install Oracle from CD

[ ] Install and Configure the Earthworm DBMS Instance
  In \earthworm\v6.0\ora_instance_install should be three files:
        initewdb.ora
        ewdbrun.sql
        ewdb_create_instance.bat

  [ ] Copy initewdb.ora to c:\orant\Database
  [ ] If you are installing a basic, standard EW setup no further 
      editing is required.
          Otherwise, edit ewdbrun.sql and ewdb_create_instance.bat 
          to reflect the correct locations of database files.
  [ ] Run ewdb_create_instance.bat 

[ ] Configure Oracle Networking
  Example files tnsnames.ora and listener.ora are provided with the utilities.
  For most installation it will only be necessary to change the IP 
  address of the machine in both files, as well as the service name which 
  will be assigned to the Earthworm Oracle instance.  If you require 
  something more fancy, you are on your own.

Change the service name IN Tnsname.ora: e.g. "EQS.ANCHEOC" to whatever you wish. "EQS.<EwInsId>" is suggested.

   [ ] Copy the two files into \orant\net80\admin and modify them 
       to reflect local reality.
   [ ] Restart the OracleTNSListener80 Service from the Control Panel

[ ] Load the Earthworm schema
   [ ] Edit the file 
       \earthworm\v6.0\src\oracle\schema\sql_scripts\ewdb_create_database.sql 
       to reflect local reality:
          At the minimum, change the oracle service ID and the location of 
          the output log file.
          See the comments in the file itself for more instructions.
   [ ] In a Command Window 
      [ ] cd \earthworm\v6.0\src\oracle\schema\sql_scripts
      [ ] \orant\bin\sqlplus system/earthworm@<your service.name>
      [ ] SQL> @ewdb_create_database
              This will take a looooong time ~ one hour!! You may want to take care 
              of the remaining configuration steps below while you wait.
   [ ] When schema is loaded, fill some of the basic stuff:
     [ ] station list
     [ ] poles and zeros:
		in v6.0\src\oracle\data\responses\P-Z.all
		These are in autodrm format, and can be loaded via 
			nsn_pz2ora <file> <user><password><service>
		For pz files in SAC format, use
			sac_pz2ora <-"->

     [ ] installation Source mapping -- in file instid_2_networkname.sql.
          if this file is changed, you will have to reload it to the database:
                 In a Command Window 
                    cd to \earthworm\v6.0\src\oracle\schema\sql_scripts 
                    \orant\bin\sqlplus system/earthworm@service.name
                    SQL> @instid_2_networkname

[ ] Install GMT
  cd c:\
  unzip gmt.zip
  mkdir gmttmp

[ ] Install the Netcdf library (required by gmt)
  cd c:\
  unzip netcdf.zip

[ ] Install Ghostscript
  cd c:\
  unzip gs.zip

[ ] Install Imagemagick
  cd c:\
  unzip imagemagick.zip

[ ] Install Cygwin
  cd c:\
  unzip cygwin.zip
  mkdir tmp

[ ] Install the Apache web server
  [ ] cd c:\
  [ ] Run instmsi.exe
  [ ] Right click on the apache_xxx.msi file and select Install
        Accept defaults
        Restart
  [ ] Configure Apache
    [ ] Copy the example httpd.conf file into 
           C:\Program Files\Apache Group\Apache\conf
    [ ] Edit the file to reflect local changes
  [ ](Re) Start the Apache Service
        Start->Programs->Apache->Control Apache Server->Start 



[ ] Set up the Earthworm web directory
  [ ] Copy web_sample from the earthworm distribution to \earthworm\web
  [ ] Customize the parameter files in \earthworm\web\params
        db_connect_opts.d -- Oracle connection parameters for user ewdb_main 
To be found in the schema creation script ewdb_create_database.sql
        web_host_opts -- name of the host running the web server (this machine)
          etc...
  [ ] Set up web\html directory
    [ ] If the web directory is to be password protected
          Customize \web\html\Htaccess file
             Change AuthUserFile to \earthworm\web\html\review.pwd
          Create the password file
             htpasswd -cm \earthworm\web\html\review.pwd <user>
              you will be prompted for the password    
    [ ] If no password protection is desired, remove the Htaccess file.
    [ ] Point your browser to the machine name.

Customize web\html\index.html, e.g., links to other pages, etc. Under NT, all 
links to cgi-bin programs should be of the form:
   http:/earthworm/cgi-bin/getlist


