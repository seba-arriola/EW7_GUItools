Readme for startstop_service
----------------------------

Service functionality added by:
   Mark Morrison
   USGS - Golden, CO
   markmm@ecomail.org

Setup of startstop_service
--------------------------

startstop_service is identical to the old startstop_nt, except that it runs as a Windows service.  This means that the parameters are all the same - read from startstop_nt.d - but that startstop isn't just executed from a command window or via the scheduler.  Note that this version is taken from startstop_nt, so I haven't compiled or tested any of this under Solaris or other systems.  I believe it's generally felt, however, that these modifications don't apply to Solaris.

To install startstop_service as a service, do the following:

 - Open a command window.  Switch to the directory containing the startstop executable (if it's not already in the Windows path).

 - Type:  startstop_service -install
       This will install the service.  You should get a message saying that installation was successful.

 - Open the Services Control Panel.  (It's under Control Panel->Administrative Tools->Services.)  You should see something listed as "Earthworm start-stop".  Verify that this service is set to run in Automatic mode (meaning it automatically starts after a reboot, without waiting for user intervention.)

 - By default, startstop is set to run as the "Local Service" account.  We need to change this to run as the Administrator, or else any Earthworm rings won't be accessible to other users, and typing "status" won't work.  To change the user the service runs under, Select the Properties of the startstop service (double-clicking or selecting Properties from the toolbar).  Select the Log On tab, select This account, and select Browse.  For Win2000 machines, select the Administrator account from the list, then click OK.  Type in the password twice, and click OK again.  For WinXP machines, select Advanced, then click Find Now.  The listbox at the bottom should show all the users locally set up on that computer.  Select the Administrator, then click OK twice.  Now type in the Administrator password twice, then click OK again.

NOTE:  This whole rigamarole means that whenever the Administrator password is changed on a machine running startstop, the password must be changed again from the Services control panel.

 - There's no way I've been able to find that allows environment variables to be loaded via ew_nt.cmd (or a similar file) before startstop runs.  So, any environment variables required for Earthworm functionality have to be loaded systemwide before starting the startstop service.  To set environment variables, select the System control panel, select the Advanced tab, and select Environment Variables.  Under the "System variables" window (NOT the "user variables for XXX" window), make sure that any environment variables you'll need are defined here.  A typical subset may include:
      - EW_HOME
      - EW_INSTALLATION
      - EW_LOG
      - EW_PARAMS
      - SYS_NAME
      - TZ
In addition, make sure \earthworm\bin directory (or whichever directory contains your earthworm binaries) is included somewhere in the "Path" variable.  Once these are set, click OK, and reboot the machine.

 - If you didn't have to change any environment variables and reboot, you can now start the startstop service by clicking Start from the Services Control Panel (the triangle "Play" button).  If you reboot, the service will start automatically.

Notes on running startstop_service
----------------------------------

Apart from running as a Windows service, startstop_service behaves just as startstop_nt.  One interesting note is that if you select startstop_service in the Services control panel and click Stop or Restart, then startstop will attempt to shut down all applications attached to its ring via an Earthworm TERMINATE message.

If you ever need to remove startstop_service from your machine, first stop the service (as above), then open a command window and type:  startstop_service -uninstall
        You should also perform this step if you ever need to move the startstop binary to another directory on your hard drive; otherwise, Windows will not know where to find the executable.
