/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetAllSMMessages.c,v 1.6 2004/02/12 22:51:49 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetAllSMMessages.c,v $
 *     Revision 1.6  2004/02/12 22:51:49  davidk
 *     Fixed a bug in PostXXX().  The idEvent field may be null for some records, and Oracle
 *     does not appear to be writing a 0 value to the memory area for the null field,
 *     so that record assumes the idEvent of the previous record located at that point
 *     in the internal buffer.  The result is data corruption, such that you get
 *     messages that are unassociated with an Event, but they appear to be associated
 *     with an event.
 *
 *     Revision 1.5  2003/09/16 17:01:59  davidk
 *     General API Cleanup of internal data structure.
 *
 *     Revision 1.4  2003/08/21 01:00:07  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.3  2003/06/09 23:05:29  davidk
 *     Removed useless debugging statements.
 *
 *     Revision 1.2  2003/01/30 23:13:30  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/05/15 02:16:42  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/04/16 21:15:55  davidk
 *     fixed two bugs:
 *     1)bEventCriteriaAvailable was not getting set right for retrieving data
 *     for only a single event.
 *
 *     2)MinLat, MaxLat, MinLon, and MaxLon were not labeled properly as input
 *     variables in the sql string.  (Missing ':').
 *
 *     Revision 1.1  2001/04/06 19:01:00  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 ********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <trace_buf.h>
#include <ewdb_sm_internal.h>

/* selection criteria constants */
#define SELECT_CRITERIA_TMOTION    5
#define SELECT_CRITERIA_COMPT      6
#define SELECT_CRITERIA_SSTA       7
#define SELECT_CRITERIA_SCOMP      8
#define SELECT_CRITERIA_SNET       9
#define SELECT_CRITERIA_SLOC      10
#define SELECT_CRITERIA_SLOC_NULL 11
#define SELECT_CRITERIA_IDEVENT   12
#define SELECT_CRITERIA_DLAT      13
#define SELECT_CRITERIA_DLON      14


static char SQL_STRINGS[][240] =
{
  /* 0*/ "select idSMMessage, tMotion, tLoad, tAlternate, iAltCode, tPGA, tPGV, tPGD, "
         "  idChan from ALL_SM_MESSAGES",

  /* 1*/ "select idSMMessage from ALL_SM_MESSAGES",

  /* 2*/ "select distinct idSMMessage, tMotion, tLoad, tAlternate, iAltCode, tPGA, tPGV, tPGD, "
         "  idChan, sSta, sComp, sNet, sLoc, dLat, dLon, dElev, dAzm, dDip, idEvent "
         "from ALL_SMMESSAGE_INFO_W_OPT_EVENT",

  /* 3*/ "select distinct idSMMessage, tMotion, tLoad, tAlternate, iAltCode, tPGA, tPGV, tPGD, "
         "  idChan, sSta, sComp, sNet, sLoc, dLat, dLon, dElev, dAzm, dDip, idEvent"
         " from ALL_SMMESSAGE_INFO_W_EVENT ",

  /* 4*/ "select distinct idSMMessage, tMotion, tLoad, tAlternate, iAltCode, tPGA, tPGV, tPGD, "
         "  idChan, sSta, sComp, sNet, sLoc, dLat, dLon, dElev, dAzm, dDip, idEvent"
         " from ALL_SMMESSAGE_INFO_WO_EVENT ",

  /* 5*/ " ((tMotion > :IN_tIntOn AND tMotion < :IN_tIntOff) OR "
         "  (tAlternate > :IN_tIntOn AND tAlternate < :IN_tIntOff))",
  /* 6*/ " tOff >= :IN_tIntOn AND tOn <= :IN_tIntOff",
  /* 7*/ " sSta  = :IN_sSta",
  /* 8*/ " sComp = :IN_sComp",
  /* 9*/ " sNet  = :IN_sNet",
  /*10*/ " sLoc  = :IN_sLoc",
  /*11*/ " sLoc  IS NULL",
  /*12*/ " idEvent = :IN_idEvent",
  /*13*/ " dLat >= :IN_dMinLat AND dLat <= :IN_dMaxLat",
  /*14*/ " dLon >= :IN_dMinLon AND dLon <= :IN_dMaxLon"
};

/* arbitrary length string */
static char SQL_STRING[1600];

#define WHERE_STRING " WHERE "
#define AND_STRING "   AND "
         
#define ORDER_BY_STRING " ORDER BY idSMMessage "

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,0, 0,0,0,OA_EWDBID,"1idSMMessage"},
  {0,0,20,0,0,OA_DOUBLE,"2tMotion"},
  {0,0,20,0,0,OA_DOUBLE,"3tLoad"},
  {0,0,20,0,0,OA_DOUBLE,"4tAlternate"},
  {0,0, 0,0,0,OA_INT,    "5iAltCode"},
  {0,0,20,0,0,OA_DOUBLE,"6tPGA"},
  {0,0,20,0,0,OA_DOUBLE,"7tPGV"},
  {0,0,20,0,0,OA_DOUBLE,"8tPGD"},
  {0,0, 0,0,0,OA_EWDBID,"9idChan"},
  {0,0,10,0,0,OA_SZ,    "10sSta"},
  {0,0,10,0,0,OA_SZ,    "11sComp"},
  {0,0,10,0,0,OA_SZ,    "12sNet"},
  {0,0,10,0,0,OA_SZ,    "13sLoc"},
  {0,0,20,0,0,OA_DOUBLE,"14dLat"},
  {0,0,20,0,0,OA_DOUBLE,"15dLon"},
  {0,0,20,0,0,OA_DOUBLE,"16dElev"},
  {0,0,20,0,0,OA_DOUBLE,"17dAzm"},
  {0,0,20,0,0,OA_DOUBLE,"18dDip"},
  {0,0, 0,0,0,OA_EWDBID,"19idEvent"},
  {0,0,0,0,0,OA_INT,":IN_tIntOn"},
  {0,0,0,0,0,OA_INT,":IN_tIntOff"},
  {0,0,0,0,0,OA_SZ, ":IN_sSta"},
  {0,0,0,0,0,OA_SZ, ":IN_sComp"},
  {0,0,0,0,0,OA_SZ, ":IN_sNet"},
  {0,0,0,0,0,OA_SZ, ":IN_sLoc"},
  {0,0, 0,0,0,OA_EWDBID,":IN_idEvent"},
  {0,0,0,0,0,OA_DOUBLE, ":IN_dMinLat"},
  {0,0,0,0,0,OA_DOUBLE, ":IN_dMaxLat"},
  {0,0,0,0,0,OA_DOUBLE, ":IN_dMinLon"},
  {0,0,0,0,0,OA_DOUBLE, ":IN_dMaxLon"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 30

/* Insertion Struct for statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage pLocalBuffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static int    tIntOff, tIntOn;
static char   szSta[10],szComp[10],
              szNet[10],szLoc[10];
static char   szDMinLat[20], szDMaxLat[20],
              szDMinLon[20], szDMaxLon[20];
static EWDBid idEvent;

static int    iQueryType;
static int    bIncludeCompTInfo, bEventCriteriaAvailable;
static int    bWhereStringAdded;

int PrepGetAllSMMessagesExec(char * IN_szSta, char * IN_szComp,
                             char * IN_szNet, char * IN_szLoc,
                             EWDBid IN_idEvent, int IN_iQueryType,
                             EWDB_CriteriaStruct * pCriteria, 
                             EWDB_Cursor * ppCursor);
int PostGetAllSMMessagesExec(void * pBuffer, int BufferRecLen,                              
                             int * pNumRecordsFound, 
                             int * pNumRecordsRetrieved);
int InitGetAllSMMessagesStatement(char * szStatement, 
                                  EWDB_OCIStatementStruct *pSS);

static char * AddClauseToString(char * szString, char * szClause);

/****************************************************************/
/****************************************************************/
int ewdb_internal_GetAllSMMessages(char * IN_szSta, char * IN_szComp,
                                   char * IN_szNet, char * IN_szLoc,
                                   EWDBid IN_idEvent, int IN_iQueryType,
                                   EWDB_CriteriaStruct * IN_pCriteria,
                                   void * pBuffer, int iBufferRecLen,
                                   int * pNumRecordsFound, 
                                   int * pNumRecordsRetrieved)
{

  EWDB_Cursor pCursor;

  ewdb_base_SetLastOraAPIActionTime ();

  if(!(IN_szSta && IN_szComp && IN_szNet && IN_szLoc && IN_pCriteria))
  {
    logit("","ewdb_internal_GetAllSMMessages(): ERROR! One or more NULL params"
             " passed to function.\n");
  }

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_internal_GetAllSMMessages(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetAllSMMessagesExec(IN_szSta, IN_szComp, IN_szNet, IN_szLoc, 
                               IN_idEvent, IN_iQueryType, IN_pCriteria, 
                               &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_GetAllSMMessages():PrepGetAllSMMessages() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_GetAllSMMessages:ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_GetAllSMMessages:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostGetAllSMMessagesExec(pBuffer, iBufferRecLen, 
                               pNumRecordsFound, pNumRecordsRetrieved) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostGetAllSMMessagesExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if(*pNumRecordsFound > *pNumRecordsRetrieved)
    return(EWDB_RETURN_WARNING);
  else
    return(EWDB_RETURN_SUCCESS);

}  /* end ewdb_internal_GetAllSMMessages() */



int InitGetAllSMMessagesStatement(char * szStatement, 
                                  EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);         /*idSMMessage*/
    iRecordSize += pSS->FieldArray[1].Ind; /*tMotion*/
    iRecordSize += pSS->FieldArray[2].Ind; /*tLoad*/
    iRecordSize += pSS->FieldArray[3].Ind; /*tAlternate*/
    iRecordSize += sizeof(int);            /*iAltCode*/
    iRecordSize += pSS->FieldArray[5].Ind; /*tPGA*/
    iRecordSize += pSS->FieldArray[6].Ind; /*tPGV*/
    iRecordSize += pSS->FieldArray[7].Ind; /*tPGD*/
    iRecordSize += sizeof(EWDBid);         /*idChan*/
    iRecordSize += pSS->FieldArray[9].Ind; /*sSta*/
    iRecordSize += pSS->FieldArray[10].Ind; /*sComp*/
    iRecordSize += pSS->FieldArray[11].Ind; /*sNet*/
    iRecordSize += pSS->FieldArray[12].Ind; /*sLoc*/
    iRecordSize += pSS->FieldArray[13].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[14].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[15].Ind; /*dElev*/
    iRecordSize += pSS->FieldArray[16].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[17].Ind; /*dDip*/
    iRecordSize += sizeof(EWDBid);          /*idEvent */
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }


  pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
  LastSize=sizeof(EWDBid);         /* idSMMessage */
  pSS->FieldArray[1].pVal= (void *) (
    (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[1].Ind; /* tMotion */
  pSS->FieldArray[2].pVal= (void *) (
    (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[2].Ind; /* tLoad */
  pSS->FieldArray[3].pVal= (void *) (
    (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[3].Ind; /* tAlternate */
  pSS->FieldArray[4].pVal= (void *) (
    (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);            /* iAltCode */
  pSS->FieldArray[5].pVal= (void *) (
    (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[5].Ind; /* tPGA */
  pSS->FieldArray[6].pVal= (void *) (
    (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[6].Ind; /* tPGV */
  pSS->FieldArray[7].pVal= (void *) (
    (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[7].Ind; /* tPGD */
  pSS->FieldArray[8].pVal= (void *) (
    (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid);         /* idChan */
  pSS->FieldArray[9].pVal= (void *) (
    (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[9].Ind; /* sSta */
  pSS->FieldArray[10].pVal= (void *) (
    (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[10].Ind; /* sComp */
  pSS->FieldArray[11].pVal= (void *) (
    (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[11].Ind; /* sNet */
  pSS->FieldArray[12].pVal= (void *) (
    (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[12].Ind; /* sLoc */
  pSS->FieldArray[13].pVal= (void *) (
    (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[13].Ind; /* dLat */
  pSS->FieldArray[14].pVal= (void *) (
    (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[14].Ind; /* dLon */
  pSS->FieldArray[15].pVal= (void *) (
    (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[15].Ind; /* dElev */
  pSS->FieldArray[16].pVal= (void *) (
    (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[16].Ind; /* dAzm */
  pSS->FieldArray[17].pVal= (void *) (
    (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[17].Ind; /* dDip */
  pSS->FieldArray[18].pVal= (void *) (
    (int)(pSS->FieldArray[17].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid);         /* idEvent */

  pSS->FieldArray[19].pVal= &tIntOn;
  pSS->FieldArray[20].pVal= &tIntOff;
  pSS->FieldArray[21].pVal= szSta;
  pSS->FieldArray[22].pVal= szComp;
  pSS->FieldArray[23].pVal= szNet;
  pSS->FieldArray[24].pVal= szLoc;
  pSS->FieldArray[25].pVal= &idEvent;
  pSS->FieldArray[26].pVal= szDMinLat;
  pSS->FieldArray[27].pVal= szDMaxLat;
  pSS->FieldArray[28].pVal= szDMinLon;
  pSS->FieldArray[29].pVal= szDMaxLon;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetAllSMMessagesStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* end InitGetAllSMMessagesStatement() */


char * AddClauseToString(char * szString, char * szClause)
{
  /* bWhereStringAdded is a global that indicates that
     a "WHERE" has already been added to the SQL string
     and that "AND"s should be added from now on.
  ****************************************************/
  if(!bWhereStringAdded)
  {
    strcat(szString, WHERE_STRING);
    bWhereStringAdded = TRUE;
  }
  else
  {
    strcat(szString, AND_STRING);
  }
  strcat(szString, szClause);
  
  return(szString);
}



int PrepGetAllSMMessagesExec(char * IN_szSta, char * IN_szComp,
                             char * IN_szNet, char * IN_szLoc,
                             EWDBid IN_idEvent, int IN_iQueryType,
                             EWDB_CriteriaStruct * pCriteria, 
                             EWDB_Cursor * ppCursor)
{

  int i;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  /* reinitialize the UseField field of the SQLParamsBindArray */
  for(i=0; i < NUM_FIELDS; i++)
    SQLParamsBindArray[i].UseField = FALSE;

  bWhereStringAdded = FALSE;

  /* copy the input params over to the local copies */
  strcpy(szSta,  IN_szSta);
  strcpy(szComp, IN_szComp);
  strcpy(szNet, IN_szNet);
  strcpy(szLoc, IN_szLoc);

  iQueryType = IN_iQueryType;

  idEvent = IN_idEvent;
  sprintf(szDMinLat, "%.6f", pCriteria->MinLat);
  sprintf(szDMaxLat, "%.6f", pCriteria->MaxLat);
  sprintf(szDMinLon, "%.6f", pCriteria->MinLon);
  sprintf(szDMaxLon, "%.6f", pCriteria->MaxLon);
  tIntOff = pCriteria->MaxTime;
  tIntOn  = pCriteria->MinTime;


  /* Build the SQL String */

  /* First specify the base query */
  switch(IN_iQueryType)
  {
  case EWDB_SM_SELECT_SMMESSAGES_W_INFO:
    {
      strcpy(SQL_STRING, 
             SQL_STRINGS[EWDB_SM_SELECT_SMMESSAGES_W_INFO]);
      for(i=0; i <= 8; i++)
        SQLParamsBindArray[i].UseField = TRUE;

      bIncludeCompTInfo = FALSE;
      bEventCriteriaAvailable = FALSE;

      break;
    }
    
  case EWDB_SM_SELECT_SMMESSAGES_W_ID_ONLY:
    {
      strcpy(SQL_STRING, 
             SQL_STRINGS[EWDB_SM_SELECT_SMMESSAGES_W_ID_ONLY]);
      for(i=0; i <= 0; i++)
        SQLParamsBindArray[i].UseField=TRUE;
      bIncludeCompTInfo = FALSE;
      bEventCriteriaAvailable = FALSE;

      break;
    }
    
  case EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO:
    {
      strcpy(SQL_STRING, 
             SQL_STRINGS[EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO]);
      for(i=0; i <= 18; i++)
        SQLParamsBindArray[i].UseField=TRUE;
      bIncludeCompTInfo = TRUE;
      bEventCriteriaAvailable = FALSE;

      break;
    }
    
  case EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_BY_EVENT:
    {
      strcpy(SQL_STRING, 
             SQL_STRINGS[EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_BY_EVENT]);
      for(i=0; i <= 18; i++)
        SQLParamsBindArray[i].UseField=TRUE;
      bIncludeCompTInfo = TRUE;
      bEventCriteriaAvailable = TRUE;

      break;
    }
  case EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_WITHOUT_EVENT:
    {
      strcpy(SQL_STRING, 
             SQL_STRINGS[EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_WITHOUT_EVENT]);
      for(i=0; i <= 18; i++)
        SQLParamsBindArray[i].UseField=TRUE;
      bIncludeCompTInfo = TRUE;
      bEventCriteriaAvailable = FALSE;

      break;
    }
  default:
    {
      logit("","PrepGetAllSMMessagesExec() ERROR! Unsupported iQueryType (%d) "
               "passed to call.\n",
            IN_iQueryType);
      return(EWDB_RETURN_FAILURE);
    }
  }
   
  /* Then add criteria to the base query */
  if(pCriteria->Criteria & EWDB_CRITERIA_USE_TIME)
  {
    AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_TMOTION]);
    if(bIncludeCompTInfo)
      AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_COMPT]);
    for(i=19; i <= 20; i++)
      SQLParamsBindArray[i].UseField=TRUE;
  }

  if(pCriteria->Criteria & EWDB_CRITERIA_USE_SCNL)
  {
    if(szSta[0] != '*')
    {
      AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_SSTA]);
      SQLParamsBindArray[21].UseField=TRUE;
    }
    
    if(szComp[0] != '*')
    {
      AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_SCOMP]);
      SQLParamsBindArray[22].UseField=TRUE;
    }
    
    if(szNet[0] != '*')
    {
      AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_SNET]);
      SQLParamsBindArray[23].UseField=TRUE;
    }

    if(szLoc[0] == 0x00)
    {
      AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_SLOC_NULL]);
    }
    else if(szLoc[0] != '*')
    {
      AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_SLOC]);
      SQLParamsBindArray[24].UseField=TRUE;
    }
  }  /* end if use SCNL criteria */

  if(pCriteria->Criteria & EWDB_CRITERIA_USE_IDEVENT)
  {
    if(!bEventCriteriaAvailable)
    {
      logit("","PrepGetAllSMMessagesExec() ERROR! "
               "Event Criteria specified utilizing a query that does "
               "not allow any. ");
      return(EWDB_RETURN_FAILURE);
    }

    AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_IDEVENT]);
    SQLParamsBindArray[25].UseField=TRUE;
  }
   
  if(pCriteria->Criteria & EWDB_CRITERIA_USE_LAT)
  {
    AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_DLAT]);
    for(i=26; i <= 27; i++)
      SQLParamsBindArray[i].UseField=TRUE;
  }

  if(pCriteria->Criteria & EWDB_CRITERIA_USE_LON)
  {
    AddClauseToString(SQL_STRING, SQL_STRINGS[SELECT_CRITERIA_DLON]);
    for(i=28; i <= 29; i++)
      SQLParamsBindArray[i].UseField=TRUE;
  }


  strcat(SQL_STRING, ORDER_BY_STRING);

  InitGetAllSMMessagesStatement(SQL_STRING,
                                      &SSStatement);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}

int PostGetAllSMMessagesExec(void * pBuffer, int BufferRecLen,                              
                             int * pNumRecordsFound, 
                             int * pNumRecordsRetrieved)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into SnipDescStruct format, 
        and copy it into the UserBuffer.
    3.  Repeat step 2. until we have reached capacity in the 
        UserBuffer or have copied all of the records from the 
        bunch.  Then we are done for the bunch.
    4.  If there are more records available, and we haven't reached
        capacity, go back to step 1.  Otherwise go to step 5.
    5.  Figure out why we stopped processing records.  If we reached
        capacity, then find out how many records were actually available
        and return that amount.  If we processed all records, return the
        amount that we processed.
    6.  Done.
    */

  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  char * pTemp;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RowsProcessed;

  EWDBid * pidSMMessageBuffer = pBuffer;
  EWDB_SMChanAllStruct * pSMMS = pBuffer;


  while(!done)
  {
    /* DK 021204  Trying to fix an issue where a NULL field doesn't appear to
                  being set, and so the old contents remain, and cause corruption
                  of the idEvent field
                  // clear the internal buffer between fetch calls
      ****************************************************************************/
    memset(pSS->FieldArray[0].pVal, 0, BUFFERSIZE);  
    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done = TRUE;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetAllSMMessagesExec:ewdb_base_SQLFetchRows",1);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
      }
    }
    RowsRetrieved = ewdb_base_GetCursorRowsProcessedCount(pCursor);
    
    for(; RowsDone < RowsRetrieved; RowsDone++)
    {
      if(RowsDone >= BufferRecLen)
      {
        done=1;
        break;
      }

      /* CopyRowFromBuffertoUserBuffer */
      {
        BCurr=RowsDone % iRecordsPerBuffer;
        UCurr=RowsDone;

        /*
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].author,pTemp);

        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].ot=atof(pTemp);
        */

        if(iQueryType == EWDB_SM_SELECT_SMMESSAGES_W_ID_ONLY)
        {
          pidSMMessageBuffer[UCurr] = *(EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));
          continue;
        }

        pSMMS[UCurr].idSMMessage=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        pTemp=(char *) ((pSS->FieldArray[1].Ind*BCurr) + (int)(pSS->FieldArray[1].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        pSMMS[UCurr].SMChan.t = atof(pTemp);  /* tMotion */

        pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pSMMS[UCurr].SMChan.tload = atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pSMMS[UCurr].SMChan.talt = atof(pTemp);

        pSMMS[UCurr].SMChan.altcode=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[4].pVal));

        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pSMMS[UCurr].SMChan.tpga = atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + (int)(pSS->FieldArray[6].pVal) );
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pSMMS[UCurr].SMChan.tpgv = atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[7].Ind*BCurr) + (int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pSMMS[UCurr].SMChan.tpgd = atof(pTemp);

        pSMMS[UCurr].Station.idChan=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[8].pVal));      

        if(iQueryType == EWDB_SM_SELECT_SMMESSAGES_W_INFO)
        {
          continue;
        }

        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal) );
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strncpy(pSMMS[UCurr].Station.Sta, pTemp, TRACE_STA_LEN);
        pSMMS[UCurr].Station.Sta[TRACE_STA_LEN] = 0x00;
        strcpy(pSMMS[UCurr].SMChan.sta, pSMMS[UCurr].Station.Sta);

        pTemp=(char *) ((pSS->FieldArray[10].Ind*BCurr) + (int)(pSS->FieldArray[10].pVal) );
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        strncpy(pSMMS[UCurr].Station.Comp, pTemp, TRACE_CHAN_LEN);
        pSMMS[UCurr].Station.Comp[TRACE_CHAN_LEN] = 0x00;
        strcpy(pSMMS[UCurr].SMChan.comp, pSMMS[UCurr].Station.Comp);

        pTemp=(char *) ((pSS->FieldArray[11].Ind*BCurr) + (int)(pSS->FieldArray[11].pVal) );
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        strncpy(pSMMS[UCurr].Station.Net, pTemp, TRACE_NET_LEN);
        pSMMS[UCurr].Station.Net[TRACE_NET_LEN] = 0x00;
        strcpy(pSMMS[UCurr].SMChan.net, pSMMS[UCurr].Station.Net);

        pTemp=(char *) ((pSS->FieldArray[12].Ind*BCurr) + (int)(pSS->FieldArray[12].pVal) );
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        strncpy(pSMMS[UCurr].Station.Loc, pTemp, TRACE_LOC_LEN);
        pSMMS[UCurr].Station.Loc[TRACE_LOC_LEN] = 0x00;
        strcpy(pSMMS[UCurr].SMChan.loc, pSMMS[UCurr].Station.Loc);

        pTemp=(char *) ((pSS->FieldArray[13].Ind*BCurr) + (int)(pSS->FieldArray[13].pVal) );
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pSMMS[UCurr].Station.Lat = (float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[14].Ind*BCurr) + (int)(pSS->FieldArray[14].pVal) );
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        pSMMS[UCurr].Station.Lon = (float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[15].Ind*BCurr) + (int)(pSS->FieldArray[15].pVal) );
        pTemp[pSS->FieldArray[15].pRetLens[BCurr]]=0;
        pSMMS[UCurr].Station.Elev = (float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[16].Ind*BCurr) + (int)(pSS->FieldArray[16].pVal) );
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        pSMMS[UCurr].Station.Azm = (float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[17].Ind*BCurr) + (int)(pSS->FieldArray[17].pVal) );
        pTemp[pSS->FieldArray[17].pRetLens[BCurr]]=0;
        pSMMS[UCurr].Station.Dip = (float)atof(pTemp);

        pSMMS[UCurr].idEvent=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[18].pVal));      
      }
    } /* End for RowsDone < RowsRetrieved */
  }  /* End while !done */
  
  if (RowsRetrieved > BufferRecLen)
  {
    /* keep going till we get all of the rows,
       but ignore the contents, since we've
       filled up the user's buffer.
    */
    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer));
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetGetAllSMMessagesExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  *pNumRecordsRetrieved = RowsDone;
  *pNumRecordsFound = RowsProcessed;

  ewdb_base_ReleaseCursor(pCursor);

  return(EWDB_RETURN_SUCCESS);
}  /* end PostXXX() */

