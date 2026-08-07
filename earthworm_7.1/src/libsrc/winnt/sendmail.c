
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sendmail.c,v 1.7 2007/02/27 04:55:11 stefan Exp $
 *
 *    Revision history:
 *     $Log: sendmail.c,v $
 *     Revision 1.7  2007/02/27 04:55:11  stefan
 *     null username check per Alex
 *
 *     Revision 1.6  2006/05/19 23:47:51  dietz
 *     Added argument to SendMail() so user can specify the "From" field of
 *     outgoing email. Defaults to %USERNAME%@%COMPUTERNAME% if a null pointer
 *     or empty string is passed in the argument. Previously defaulted to
 *     "root@mailserver".
 *
 *     Revision 1.5  2003/06/11 19:13:14  kohler
 *     Increased BUFFSIZE from 200 to 250 in define statment.
 *
 *     Revision 1.4  2002/09/30 19:34:52  alex
 *     changed 'printf' to 'logit'
 *     Alex
 *
 *     Revision 1.3  2001/10/02 18:15:18  dietz
 *     Changed to use COMPUTERNAME in the email sender field
 *     instead of Earthworm.
 *
 *     Revision 1.2  2000/05/23 18:05:31  dietz
 *     Changed blat sender to root@<MailHost>
 *
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

     /*****************************************************************
      *                            sendmail                           *
      *                                                               *
      *         Function to send email.  Windows NT version.          *
      *                                                               *
      *  This function requires a mail server computer.               *
      *                                                               *
      *  person:     List of email recipients                         *
      *  nmail:      The number of recipients                         *
      *  mailProg:   Mail program to use - for solaris compatibility  *
      *  subject:    Subject of the message                           *
      *  msg  :      The body of the email message                    *
      *  msgPrefix : Prefix to the body of the message                *
      *  msgSuffix : Suffix to the body of the message                *
      *  mailServer: Computer that sends the mail message.            *
      *                                                               *
      *  Returns -1 if an error occurred, 0 otherwise                 *
      *                                                               *
      *  mailProg, subject, msgPrefix, and msgSuffix added by         *
      *    Lucky Vidmar  Tue Jan 19 16:17:01 MST 1999                 *
      *                                                               *
      *****************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <earthworm.h>

#define BUFFSIZE 250

static char Sender[BUFFSIZE];
static int  SenderInit = 0;

int SendMail( char person[][60], int nmail, char *mailProg, char *subject, 
              char *msg, char *msgPrefix, char *msgSuffix, char *mailServer,
              char *from )
{
   FILE                *fp;
   DWORD               pathSize;
   char                tmp[BUFFSIZE];
   char                pathBuffer[BUFFSIZE];
   char                tempFilename[MAX_PATH];
   char                commandLine[BUFFSIZE*4];
   STARTUPINFO         startUpInfo;
   BOOL                success;
   DWORD               priorityClass;
   PROCESS_INFORMATION procInfo;
   DWORD               exitCode;
   DWORD               rc;
   int                 i;

/* Check arguments
   ***************/
   if ( nmail < 1 )  return -1;

/* Do once: load Sender with thisuser@thiscomputer
   ***********************************************/
   if ( !SenderInit ) 
   { 
      char nobody[10];
      char *user     = getenv( "USERNAME" );
      char *computer = getenv( "COMPUTERNAME" );
      int   len;     
      if (user == NULL) {
          strcpy( nobody, "nobody\0" );
          user = nobody;
      }
      len = strlen(user)+strlen(computer)+1;
      if( len < BUFFSIZE )   sprintf( Sender, "%s@%s", user, computer );
      else if( strlen(computer) < BUFFSIZE ) strcpy( Sender, computer );
      else                                   strcpy( Sender, "nobody" );
      SenderInit = 1;
   }

/* Get the path of the temporary file directory
   ********************************************/
   pathSize = GetTempPath( BUFFSIZE, pathBuffer );
   if ( pathSize < BUFFSIZE )
      pathBuffer[pathSize] = 0;
   else
   {
      logit("","Error getting path of temporary file directory." );
      return -1;
   }

/* Create a unique file in the temporary file directory
   ****************************************************/
   if ( GetTempFileName( pathBuffer, "mai", 0, tempFilename ) == 0 )
   {
      logit("","Error getting name of temporary file.\n" );
      return -1;
   }

/* Write the mail message to the temporary file
   ********************************************/
   fp = fopen( tempFilename, "w" );
   if ( fp == NULL )
   {
      logit("","Error opening the temporary file.\n" );
      return -1;
   }
 
   if (msgPrefix != NULL)
       fputs( msgPrefix, fp );

   fputs( msg, fp );

   if (msgSuffix != NULL)
       fputs( msgSuffix, fp );

   fclose( fp );

/* Build a command line for the Blat program, which sends
   the temporary file to the mail server computer.
   ******************************************************/
   strcpy( commandLine, "blat " );
   strcat( commandLine, tempFilename );

   if( subject != NULL )
   {
       sprintf( tmp, " -s \"%s\"", subject );
       strcat( commandLine, tmp );
   }

   strcat( commandLine, " -f " );
   if( from && strlen(from)!=0 ) strcat( commandLine, from );
   else                          strcat( commandLine, Sender );

   strcat( commandLine, " -server " );
   strcat( commandLine, mailServer );

   strcat( commandLine, " -t " );
   strcat( commandLine, &person[0][0] );
   for ( i = 1; i < nmail; i++ )
   {
      strcat( commandLine, "," );
      strcat( commandLine, &person[i][0] );
   }

   /* logit("","sendmail command line:\n.%s.\n",commandLine); */

/* Invoked the Blat program
   ************************/
   GetStartupInfo( &startUpInfo );

   priorityClass = GetPriorityClass( GetCurrentProcess() );

   success = CreateProcess( 0,
                            commandLine,      /* Command line to invoke child */
                            0, 0,                   /* No security attributes */
                            FALSE,                    /* No inherited handles */
                            0 |                /* Child does not get a window */
                            priorityClass,      /* Same as priority of parent */
                            0,              /* Not passing environmental vars */
                            0,                  /* Current dir same as parent */
                            &startUpInfo,     /* Attributes of process window */
                            &procInfo );       /* Attributes of child process */
   if ( !success )
   {
      logit("","Error starting Blat: %d\n", GetLastError() );
      return -1;
   }

/* Wait 5 minutes for Blat to complete.
   Check the process exit code for abnormal termination.
   ****************************************************/
   rc = WaitForSingleObject( procInfo.hProcess, 300000 );

   if ( rc == WAIT_FAILED )
   {
      logit("","Blat WaitForSingleObject() failed with error: %d\n", GetLastError() );
      return -1;
   }
   if ( rc == WAIT_TIMEOUT )
   {
      logit("","Error. Blat not completed within 5 minutes.\n" );
      return -1;
   }
   success = GetExitCodeProcess( procInfo.hProcess, &exitCode );
   if ( !success )
   {
      logit("","Error getting the Blat exit code: %d\n", GetLastError() );
      return -1;
   }
   if ( exitCode != 0 )
   {
      logit("","Blat failed.\n" );
      return -1;
   }

/* Delete the mail file
   ********************/
   success = DeleteFile( tempFilename );
   if ( !success )
   {
      logit("","Error deleting temporary file: %s\n", tempFilename );
      return -1;
   }

/* Everything went smoothly
   ************************/
   return 0;
}

