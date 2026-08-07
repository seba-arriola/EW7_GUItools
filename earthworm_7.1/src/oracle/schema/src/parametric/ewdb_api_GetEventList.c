/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetEventList.c,v 1.15 2005/06/15 19:03:12 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_api_GetEventList.c,v $
 *    Revision 1.15  2005/06/15 19:03:12  davidk
 *    DB API Cleanup
 *
 *    Revision 1.14  2005/05/16 19:10:02  mark
 *    Increased static str size
 *
 *    Revision 1.13  2005/05/12 20:32:01  mark
 *    Added comments to event struct
 *
 *    Revision 1.12  2004/08/12 22:39:22  davidk
 *    Removed references to Reviewed-By criteria.  This criteria was never fully
 *    implemented and was futzing up processing of the Dubiocity critieria.
 *
 *    Revision 1.11  2004/08/12 21:28:55  davidk
 *    Fixed two bugs:
 *     1) Oracle value pointer was not pointed at Minstruct.iDubiocity.
 *    2) Dubiocity.useField was not reset to false at the beginning of each funciton.
 *
 *    Revision 1.10  2004/08/12 20:19:02  davidk
 *    Added iDubiocity critieria.
 *    Set pCriteria->Event.iDubiocity = 1
 *    Set pMaxEvent->Event.iDubiocity = <VALUE YOU WANT MATCHED>.
 *    So if you don't want dubious events, set:
 *            pCriteria->Event.iDubiocity = 1
 *            pMaxEvent->Event.iDubiocity = 0.
 *
 *    Revision 1.9  2003/09/16 16:56:55  davidk
 *    General API Cleanup
 *
 *    Revision 1.8  2003/08/21 00:52:10  davidk
 *    Restructured API code that deals with dynamic allocation of memory.
 *
 *    Revision 1.7  2003/03/13 23:54:31  davidk
 *    Fixed bug in InitGetEventList() that was causing the EventType and Dubiocity
 *    be corrupted by the bArchived flag for each returned Event.
 *
 *    Revision 1.6  2001/07/16 22:07:52  davidk
 *    Removed lines that reinitialized the RVBN and RBNN members
 *    of the BindArray.  (Those members don't exist, and it was
 *    causing a pointer overwrite, causing the SQL_STRING to be
 *    truncated.)
 *
 *    Revision 1.5  2001/07/14 07:42:06  davidk
 *    API Cleanup.
 *    Added support for bArchived flag.
 *
 *    Revision 1.3  2001/05/24 19:11:19  lucky
 *    Changed Magnitude to PrefMag and added retrieval of idOrigin.
 *    We need this to handle ML type mags in getlist
 *
 * 
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
  
/* get_event_list.c */

static char SQL_STRING[1024];
static char SQL_STRING_BASE[] =
 "select idEvent, tiEventType, iDubiocity, bArchived, sSource, sHumanReadable, sSourceEventID, "
 "idOrigin, tOrigin, dLat, dLon, dDepth, dPrefMag, idComment, idInternalComment, idContribMagComment "
 "from ALL_EVENTS_INFO";

#define szWHERE " WHERE "
#define szAND   " AND "

#define szBASE 16
#define szSTTM 0
#define szENTM 1
#define szSRCE 2
#define szLATN 3
#define szLATX 4
#define szLONN 5
#define szLONX 6
#define szZMIN 7
#define szZMAX 8
#define Local_szMGMN 9
#define Local_szMGMX 10
#define szEVTP 11
#define szDUBI 12

/* Total number of fields, currently BASE + VLMD + 1 */
#define NUM_FIELDS 29


static char SQL_STRING_OPT[][30] = 
    {
      "tOrigin>=:starttime",
      "tOrigin<=:endtime",
      "sSource=:source",
      "dLat>=:minlat",
      "dLat<=:maxlat",
      "dLon>=:minlon",
      "dLon<=:maxlon",
      "dDepth>=:minz",
      "dDepth<=:maxz",
      "dPrefMag>=:minmag",
      "dPrefMag<=:maxmag",
      "tiEventType=:eventtype",
      "iDubiocity=:dubiocity"
    };

/* DK CLEANUP  iSourceType is not a defined member of the Source table 
   or of any of the resulting views */

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idEvent"},
  {0,1,0,0,0,OA_INT,"2tiEventType"},
  {0,1,0,0,0,OA_INT,"3iDubiocity"},
  {0,1,0,0,0,OA_INT,"4bArchived"},
  {0,1,52,0,0,OA_SZ,"5sSource"},
  {0,1,256,0,0,OA_SZ,"6sHumanReadable"},
  {0,1,30,0,0,OA_SZ,"7sSourceEventID"},
  {0,1,0,0,0,OA_EWDBID,"8idOrigin"},
  {0,1,20,0,0,OA_DOUBLE,"9tOrigin"},
  {0,1,20,0,0,OA_FLOAT,"10dLat"},
  {0,1,20,0,0,OA_FLOAT,"11dLon"},
  {0,1,20,0,0,OA_FLOAT,"12dDepth"},
  {0,1,20,0,0,OA_FLOAT,"13dPrefMag"},
  {0,1,0,0,0,OA_EWDBID,"14idComment"},
  {0,1,0,0,0,OA_EWDBID,"15idInternalComment"},
  {0,1,0,0,0,OA_EWDBID,"16idContribMagComment"},
  {0,0,0,0,0,OA_INT,":starttime"},
  {0,0,0,0,0,OA_INT,":endtime"},
  {0,0,0,0,0,OA_SZ,":source"},
  {0,0,0,0,0,OA_FLOAT,":minlat"},
  {0,0,0,0,0,OA_FLOAT,":maxlat"},
  {0,0,0,0,0,OA_FLOAT,":minlon"},
  {0,0,0,0,0,OA_FLOAT,":maxlon"},
  {0,0,0,0,0,OA_FLOAT,":minz"},
  {0,0,0,0,0,OA_FLOAT,":maxz"},
  {0,0,0,0,0,OA_FLOAT,":minmag"},
  {0,0,0,0,0,OA_FLOAT,":maxmag"},
  {0,0,0,0,0,OA_INT,":eventtype"},
  {0,0,0,0,0,OA_INT,":dubiocity"}
};

/* Insertion Struct for GetEventList statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     Local_tStartTime;
static  int     Local_iEndTime;
static  char    Local_szSource[30];
static  int     Local_iLastState;
static  EWDB_EventListStruct     Local_MinEventStruct;
static char Local_szMinLat[15],Local_szMaxLat[15],Local_szMinLon[15],
            Local_szMaxLon[15],Local_szMinZ[15],Local_szMaxZ[15],
            Local_szMinMag[15],Local_szMaxMag[15];


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetEventListExec(EWDB_EventListStruct * pESCriteria,
                                EWDB_EventListStruct * pESMax, 
                                int IN_tStartTime, int IN_tEndTime,
                                EWDB_Cursor * ppCursor);
static int PostGetEventListExec(EWDB_EventListStruct * pBuffer, 
                                int BufferRecLen);
static char * AddCriteria(char * MainString, int NumAdditions, 
                          char * pCriteria);
static int InitGetEventListStatement(char * szStatement, 
                                     EWDB_OCIStatementStruct *pSS);


/*********************************************************************** 
   Used to get the list of all events in the database, in a given time range,
     that meet a ReviewedBy criteria
	pBuffer is a pointer to a buffer allocated by the caller and used
	 by GetEventList to store the array of events.
  pBuffer[0] should contain the criteria for the List selection.
	len is the length of the event array in event structures, allocated
	 by the caller.
  pCriteria is a pointer to the EventStruct that contains the list formulation
   criteria.
	 IN_tStartTime and IN_tEndTime are time_t's that define the time threshhold for
	  selecting events.
   EventStruct members:
    ReviewedBy is the name of the person who last reviewed the event.
 		 "*" defines all events
		 "**" defines all events reviewed
		 "" (Blank String) defines all events not reviewed
		 Any other string will be matched against the name in the ReviewedBy
		  field of an event.
   See the EventStruct definition in ora_api.h for info on the other
    struct members.

	pNumEvents is the number of event structures returned by GetEventList.

      Return Value: -1 when an error occurs
					-x when the buffer passed by the caller is too
					 small, where x is the neccessary size of len
					 in EventStructs to get all of the events for the given
					 criteria.
					0 on success.

    Other Details:  Caller is responsible for allocating
	space for the event list buffer.
***********************************************************************/
int ewdb_api_GetEventList(EWDB_EventListStruct * pBuffer, int len,
                          unsigned int IN_tStartTime, unsigned int IN_tEndTime, 
                          EWDB_EventListStruct * pCriteria, 
                          EWDB_EventListStruct * pMaxEvent, int * pNumEvents)
{

  int NumEventsReceived;
  EWDB_Cursor  pCursor;

  
  ewdb_base_SetLastOraAPIActionTime();

  if(!pBuffer || !pCriteria || !pMaxEvent || !pNumEvents)
  {
    logit("","ewdb_api_GetEventList(): Invalid params passed in:\n"
             "pBuffer %d, pCriteria %u, pMaxEvent %u, pNumEvents %u\n",
          pBuffer, pCriteria, pMaxEvent, pNumEvents);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
    /* Establishes connection, and performs binding!?! */
  {
     return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetEventListExec(pCriteria,pMaxEvent,IN_tStartTime,IN_tEndTime,&pCursor)
      != EWDB_RETURN_SUCCESS )
  {
    logit("","ewdb_api_GetEventList(): PrepGetEventListExec() failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetEventList(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetEventList(): ewdb_base_SQLCommit",2);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  if((NumEventsReceived=PostGetEventListExec((EWDB_EventListStruct *)pBuffer,len)) 
      == EWDB_RETURN_FAILURE )
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();
  if(NumEventsReceived > len)
  {
    *pNumEvents=len;
    return(0-NumEventsReceived);
  }
  else
  {
    *pNumEvents=NumEventsReceived;
    return(EWDB_RETURN_SUCCESS);
  }
}  /* end ewdb_api_GetEventList() */


static int InitGetEventListStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idEvent*/
    iRecordSize += sizeof(int); /*tiEventType*/
    iRecordSize += sizeof(int); /*iDubiocity*/
    iRecordSize += sizeof(int); /*bArchived*/
    iRecordSize += pSS->FieldArray[5].Ind; /*sSource*/
    iRecordSize += pSS->FieldArray[6].Ind; /*sHumanReadable*/
    iRecordSize += pSS->FieldArray[7].Ind; /*sSourceEventID*/
    iRecordSize += sizeof(EWDBid); /*idOrigin*/
    iRecordSize += pSS->FieldArray[8].Ind; /*tOrigin*/
    iRecordSize += pSS->FieldArray[9].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[10].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[11].Ind; /*dDepth*/
    iRecordSize += pSS->FieldArray[12].Ind; /*dPrefMag*/
    iRecordSize += sizeof(EWDBid); /*idComment*/
    iRecordSize += sizeof(EWDBid); /*idInternalComment*/
    iRecordSize += sizeof(EWDBid); /*idContribMagComment*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(int);
  
    pSS->FieldArray[1].pVal= (void *) (
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
  
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
  
    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
  
    pSS->FieldArray[4].pVal= (void *) (
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /*char sSource[]*/
  
    pSS->FieldArray[5].pVal= (void *) (
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* char sHumanReadable[]*/
  
    pSS->FieldArray[6].pVal= (void *) (
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* char sSourceEventID[]*/
  
    pSS->FieldArray[7].pVal= (void *) (
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (int); /* EWDBid idOrigin*/

    pSS->FieldArray[8].pVal= (void *) (
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind;  /* double tOrigin */
  
    pSS->FieldArray[9].pVal= (void *) (
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind;  /* float dLat */
  
    pSS->FieldArray[10].pVal= (void *) (
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind;  /* float dLon */
  
    pSS->FieldArray[11].pVal= (void *) (
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind;  /* float dDepth */
  
    pSS->FieldArray[12].pVal= (void *) (
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind;  /* float dMag */
  
    pSS->FieldArray[13].pVal= (void *) (
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (EWDBid); /* EWDBid idComment*/

    pSS->FieldArray[14].pVal= (void *) (
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (EWDBid); /* EWDBid idInternalComment*/

    pSS->FieldArray[15].pVal= (void *) (
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (EWDBid); /* EWDBid idContribMagComment*/

    pSS->FieldArray[16].pVal=&(Local_tStartTime);
    pSS->FieldArray[17].pVal=&(Local_iEndTime);
    pSS->FieldArray[18].pVal=Local_szSource;
    pSS->FieldArray[19].pVal=Local_szMinLat;
    pSS->FieldArray[20].pVal=Local_szMaxLat;
    pSS->FieldArray[21].pVal=Local_szMinLon;
    pSS->FieldArray[22].pVal=Local_szMaxLon;
    pSS->FieldArray[23].pVal=Local_szMinZ;
    pSS->FieldArray[24].pVal=Local_szMaxZ;
    pSS->FieldArray[25].pVal=Local_szMinMag;
    pSS->FieldArray[26].pVal=Local_szMaxMag;
    pSS->FieldArray[27].pVal=&(Local_MinEventStruct.Event.iEventType);
    pSS->FieldArray[28].pVal=&(Local_MinEventStruct.Event.iDubiocity);
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetSelectedChannelsStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetEventListStatement() */

       
static char * AddCriteria(char * MainString,int NumAdditions, char * pCriteria)
{
  if(NumAdditions == 0)
    strcat(MainString,szWHERE);
  else
    strcat(MainString,szAND);
  return(strcat(MainString,pCriteria));
}


static int PrepGetEventListExec(EWDB_EventListStruct * pESCriteria,
                         EWDB_EventListStruct * pESMax, 
                         int IN_tStartTime, int IN_tEndTime,
                         EWDB_Cursor * ppCursor)
    /*
    1.  check criteria to see which statement configuration to execute.
    2.  check to see if the desired configuration is the same as the last.
    3.  if yes, then done.  if no, then we need to re prep the statement, go to 4.
    4.  Build the desired statement.
    5.  Parse the statement.
    6.  Autobind the statement.
    7.  Manual bind any remaining variables.
    8.  Done.
    */
{

  int StatementAdditions=0;

  Local_szSource[0]=0;
  Local_iLastState=0;
  Local_szMinLat[0]=Local_szMaxLat[0]=Local_szMinLon[0]=Local_szMaxLon[0]=
  Local_szMinZ[0]=Local_szMaxZ[0]=Local_szMinMag[0]=Local_szMaxMag[0]=0;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }


  /* reset all of the optional fields */
  /* Added by DK 071301 */
  SQLParamsBindArray[szBASE+szSTTM].UseField = FALSE;
  SQLParamsBindArray[szBASE+szENTM].UseField = FALSE;
  SQLParamsBindArray[szBASE+szSRCE].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLATN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLATX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szZMIN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szZMAX].UseField = FALSE;
  SQLParamsBindArray[szBASE+Local_szMGMN].UseField = FALSE;
  SQLParamsBindArray[szBASE+Local_szMGMX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szEVTP].UseField = FALSE;
  SQLParamsBindArray[szBASE+szDUBI].UseField = FALSE;

  memcpy(&Local_MinEventStruct,pESCriteria,sizeof(EWDB_EventListStruct));
  Local_tStartTime=IN_tStartTime;
  Local_iEndTime=IN_tEndTime;

  /* Create the base string */
  sprintf(SQL_STRING,"%s",SQL_STRING_BASE);

  if(Local_tStartTime)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szSTTM]);
    SQLParamsBindArray[szBASE+szSTTM].UseField=1;
    StatementAdditions++;
  }

  if(Local_iEndTime)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szENTM]);
    SQLParamsBindArray[szBASE+szENTM].UseField=1;
    StatementAdditions++;
  }
    
  if(Local_MinEventStruct.dLat)
  {
    sprintf(Local_szMinLat,"%f",Local_MinEventStruct.dLat);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLATN]);
    SQLParamsBindArray[szBASE+szLATN].UseField=1;
    StatementAdditions++;
  }

  if(pESMax->dLat)
  {
    sprintf(Local_szMaxLat,"%f",pESMax->dLat);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLATX]);
    SQLParamsBindArray[szBASE+szLATX].UseField=1;
    StatementAdditions++;
  }

  if(Local_MinEventStruct.dLon)
  {
    sprintf(Local_szMinLon,"%f",Local_MinEventStruct.dLon);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONN]);
    SQLParamsBindArray[szBASE+szLONN].UseField=1;
    StatementAdditions++;
  }

  if(pESMax->dLon)
  {
    sprintf(Local_szMaxLon,"%f",pESMax->dLon);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONX]);
    SQLParamsBindArray[szBASE+szLONX].UseField=1;
    StatementAdditions++;
  }

  if(Local_MinEventStruct.dDepth)
  {
    sprintf(Local_szMinZ,"%f",Local_MinEventStruct.dDepth);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szZMIN]);
    SQLParamsBindArray[szBASE+szZMIN].UseField=1;
    StatementAdditions++;
  }

  if(pESMax->dDepth)
  {
    sprintf(Local_szMaxZ,"%f",pESMax->dDepth);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szZMAX]);
    SQLParamsBindArray[szBASE+szZMAX].UseField=1;
    StatementAdditions++;
  }

  if(Local_MinEventStruct.dPrefMag)
  {
    sprintf(Local_szMinMag,"%f",Local_MinEventStruct.dPrefMag);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[Local_szMGMN]);
    SQLParamsBindArray[szBASE+Local_szMGMN].UseField=1;
    StatementAdditions++;
  }

  if(pESMax->dPrefMag)
  {
    sprintf(Local_szMaxMag,"%f",pESMax->dPrefMag);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[Local_szMGMX]);
    SQLParamsBindArray[szBASE+Local_szMGMX].UseField=1;
    StatementAdditions++;
  }

  if(Local_MinEventStruct.Event.iEventType)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szEVTP]);
    SQLParamsBindArray[szBASE+szEVTP].UseField=1;
    StatementAdditions++;
  }

  if(Local_MinEventStruct.Event.iDubiocity == 1)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szDUBI]);
    SQLParamsBindArray[szBASE+szDUBI].UseField=1;
    /* to use dubiocity criteria, Local_MinEventStruct.iDubiocity must be 1,
       and MaxEventStruct.iDubiocity is used for the flag value. 
     ****************************************************************/
    Local_MinEventStruct.Event.iDubiocity = pESMax->Event.iDubiocity;
    StatementAdditions++;
  }

  /* Set the query to order by Origin Time */
  strcat(SQL_STRING," ORDER BY tOrigin DESC");

  if(InitGetEventListStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetEventListExec() */


static int PostGetEventListExec(EWDB_EventListStruct * pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into EWDB_EventListStruct format, 
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
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  char * pTemp;
  int RowsProcessed;


  while(!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetEventListExec:ewdb_base_SQLFetchRows",1);
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
        pBuffer[UCurr].Event.idEvent=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].Event.iEventType=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[1].pVal));
        
        pBuffer[UCurr].Event.iDubiocity=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[2].pVal));
        
        pBuffer[UCurr].Event.bArchived=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[3].pVal));
        
        pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + (int)(pSS->FieldArray[4].pVal));
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Event.szSource,pTemp);

        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal));
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Event.szSourceName,pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + (int)(pSS->FieldArray[6].pVal));
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Event.szSourceEventID,pTemp);
        
        pBuffer[UCurr].idOrigin=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[7].pVal));

        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dOT=atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal) );
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLat=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[10].Ind*BCurr) + (int)(pSS->FieldArray[10].pVal) );
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLon=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[11].Ind*BCurr) + (int)(pSS->FieldArray[11].pVal) );
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dDepth=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[12].Ind*BCurr) + (int)(pSS->FieldArray[12].pVal) );
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dPrefMag=(float)atof(pTemp);

        pBuffer[UCurr].Event.idPublicComment=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[13].pVal));
        pBuffer[UCurr].Event.idInternalComment=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[14].pVal));
        pBuffer[UCurr].Event.idContribMagComment=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[15].pVal));
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
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
      {
      }
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetEventListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  

  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* end PostGetEventListExec() */
