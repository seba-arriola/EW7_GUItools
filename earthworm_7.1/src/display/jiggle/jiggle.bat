
REM # Set JIGGLE_HOME
set JIGGLE_HOME=c:\jiggle

REM #Set CLASSPATH if desired
REM # Must include steim library  FissuresTest5.jar
REM # and oracle library  classes12.zip
set classpath=%JIGGLE_HOME%\jiggle_ew_v62.zip;%JIGGLE_HOME%\classes12.zip;%JIGGLE_HOME%\FissuresSeedTest5.jar

REM # The OS user name or the JIGGLE_USER_NAME must be a valid, existing DB author(source)
set JIGGLE_USER_NAME=davek
set JIGGLE_USER_HOMEDIR=%JIGGLE_HOME%

REM # Run Jiggle
java -Xss128k -XX:SurvivorRatio=8 -Xms256m -Xmx256m -DJIGGLE_HOME=%JIGGLE_HOME% -DJIGGLE_USER_NAME=%JIGGLE_USER_NAME% -DJIGGLE_USER_HOMEDIR=%JIGGLE_USER_HOMEDIR% org.trinet.jiggle.Jiggle
