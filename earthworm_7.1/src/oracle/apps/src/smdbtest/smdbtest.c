/*  This file is under RCS
*/




/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: smdbtest.c,v 1.7 2001/07/27 19:18:00 davidk Exp $
 *
 *    Revision history:
 *     $Log: smdbtest.c,v $
 *     Revision 1.7  2001/07/27 19:18:00  davidk
 *     *** empty log message ***
 *
 *     Revision 1.6  2001/05/15 02:15:48  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.5  2001/04/06 22:32:26  davidk
 *     converted from strong_motion I to strong_motionII.
 *
 *
 **************************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_APPLICATIONS

SUB_LIBRARY smdbtest2

LOCATION THIS_FILE

DESCRIPTION smdbtest2 is a simple stand-alone application that
performs a minimal test of the strong motion functionality of
an EW DBMS system.  It utilizes the RW_STRONGMOTION_II 
and EWDB_API_LIB/STRONG_MOTION_API libraries.
<br><br>
<b> Application Usage:  smdbtest2 [DBUser] [DBPasswd] [DBSID]" </b>
<br> Where <b>DBUser</b> is the Oracle user to use for the test,
<b>DBPasswd</b> is DBUser's password, and <b>DBSID</b> is the Oracle
Service ID(SID) of the database you wish to use.
<br><br>
Initial version unclaimed!
<br><br>
Converted from TYPE_STRONGMOTION to TYPE_STRONGMOTIONII
on 2001/04/04 by DavidK.
<br><br>

*************************************************
************************************************/






#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <earthworm_simple_funcs.h>
#include <rw_strongmotionII.h>

#define BUFLEN 5000


int main(int argc, char ** argv)
{
   char * szDBUser   = argv[1];
   char * szDBPasswd = argv[2];
   char * szDBSID    = argv[3];

   SM_INFO sm;
   char    buf[BUFLEN];
   int     nRSA = 3;
   double  freq = 0., RSA = 1.;
   int     i,j,rc;
   int     iNumMessages = 3;
   EWDB_CriteriaStruct csCriteria;
   int     messages_retrieved,messages_found;
   EWDB_SMChanAllStruct smchans[5];

   char szUsageMsg[] = "Usage:  smdbtest2 <DBUser> <DBPasswd> <DBSID>\n";

   time_t tNow;


   if(argc != 4)
   {
     printf(szUsageMsg);
     return(-1);
   }

   logit_init( "smdbtest2", 0, 512, 1 );
   memset(&sm,0,sizeof(sm));

   printf( "sizeof(SM_INFO) = %d\n", sizeof(SM_INFO) );


   for( i=0; i<iNumMessages; i++ )
   {
     sprintf(sm.sta,"TEST%d",i%10);
     sprintf(sm.comp, "AD%c",'A'+i);
     strcpy( sm.net, "NC" );
     strcpy( sm.loc, "gd" );

     time(&tNow);
     sm.t = 
     sm.pga  = 0.25;
     sm.pgv  = 0.01;
     sm.pgd = 0.01;

     sm.t = tNow + .0025;
     sm.tpga = tNow +.0125;
     sm.tpgv = tNow +.2125;
     sm.tpgd = tNow +.1525;

     sm.nrsa = nRSA;
   
     freq = 0.0;
     RSA = 0.0;

     for( j=0; j<nRSA; j++ )
     {
       sm.pdrsa[j] = freq += 0.02;
       sm.rsa[j] = RSA += 0.35;
     }
   }

   logit("","---------------\nsm before wr_strongmotion:\n");
   log_strongmotionII( &sm );

   rc = wr_strongmotionII( &sm, buf, BUFLEN );

   logit("","---------------\nbuf after wr_strongmotion:\n");
   if( rc != 0 ) 
     logit("et","ERROR in wr_strongmotionII()!\n" );
   logit("","%s", buf );

   rc = ewdb_api_Init(szDBUser, szDBPasswd, szDBSID);
   if (rc!= EWDB_RETURN_SUCCESS)
   {
    printf("ewdb_api_Init() failed with retcode %d!\n",rc);
   }

   rc = ewdb_api_PutSMMessage(&sm, 0);

   if( rc != 0 ) printf( "ERROR:%d in PutSMMessage()!\n",rc);
   printf("---------------\nsm after PutSMMessage():\n");
   log_strongmotionII( &sm );

   memset( &sm, 0, sizeof(SM_INFO) );
   printf("---------------\nsm after memset:\n");
   log_strongmotionII( &sm );

   rc = rd_strongmotionII( buf, strlen(buf), &sm );

   printf("---------------\nsm after rd_strongmotion:\n");
   if( rc != 0 ) printf( "ERROR in rd_strongmotion!\n" );
   
   log_strongmotionII( &sm );

   memset(&csCriteria,0,sizeof(csCriteria));
   csCriteria.Criteria = EWDB_CRITERIA_USE_TIME;
   csCriteria.MaxTime =(int)999999999.99;
   csCriteria.MinTime =EWDB_MIN_TIME;

   memset( &sm, 0, sizeof(SM_INFO) );
   printf("---------------\nsm after GetSMMessages():\n");
   logit("t","Starting ewdb_api_GetSMDataWithChannelInfo()\n");
   rc = ewdb_api_GetSMDataWithChannelInfo(&csCriteria,
                                      "", "", "", "",
                                      0, EWDB_SM_SEARCH_FOR_ALL_SMMESSAGES,
                                      smchans, 
                                      sizeof(smchans)/sizeof(EWDB_SMChanAllStruct),
                                      &messages_retrieved, &messages_found);
   logit("t","Ending ewdb_api_GetSMDataWithChannelInfo()\n");
   if(rc < 0) 
     logit("et","Error(%d) in ewdb_api_GetSMDataWithChannelInfo()!\n",rc);
   else
   {
     if(!messages_retrieved)
       logit("et","%s:Error %d messages found and %d messages retrieved.\n",
             "ewdb_api_GetSMDataWithChannelInfo()",
             messages_found, messages_retrieved);
     else
     {
       log_strongmotionII( &(smchans->SMChan) );
     }
   }

   logit("t","Starting ewdb_api_GetSMData()\n");
   rc = ewdb_api_GetSMData(&csCriteria,
                                      "", "", "", "",
                                      0, EWDB_SM_SEARCH_FOR_ALL_UNASSOCIATED_MESSAGES,
                                      &(smchans->SMChan), 1,
                                      &messages_retrieved, &messages_found);
   logit("t","Ending ewdb_api_GetSMData()\n");
   if(rc < 0) 
     logit("et","Error(%d) in ewdb_api_GetSMData()!\n",rc);
   else
   {
     if(!messages_retrieved)
       logit("et","%s:Error %d messages found and %d messages retrieved.\n",
             "ewdb_api_GetSMData()",
             messages_found, messages_retrieved);
     else
     {
       log_strongmotionII( &(smchans->SMChan) );
     }
   }
   
   return( 0 );
}  /* end main() */

