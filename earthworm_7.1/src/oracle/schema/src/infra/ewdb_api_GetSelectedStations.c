/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetSelectedStations.c,v 1.7 2005/06/27 15:30:01 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_api_GetSelectedStations.c,v $
 *    Revision 1.7  2005/06/27 15:30:01  davidk
 *    Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *    Revision 1.6  2005/06/10 16:27:59  davidk
 *    DB API Cleanup
 *
 *    Revision 1.5  2003/12/03 00:23:29  davidk
 *    Changed starttime and endtime params to doubles from ints.  int
 *    did not have enough precision for test values (post 2038).
 *    Modified the row order (ORDER BY clause).
 *    Added support for NULL location codes, including increasing the length of
 *    SQL_STRING_OPTions.
 *    Added code to handle two additional parameters: tOnCompt and tOffCompT,
 *    representing the on/off times for the Compt as opposed to the ChanT.
 *
 *    Revision 1.4  2003/09/16 17:00:14  davidk
 *    General API Cleanup of internal structures.
 *
 *    Revision 1.3  2003/08/21 00:41:46  davidk
 *    Restructured API code that deals with dynamic allocation of memory.
 *
 *    Revision 1.2  2003/03/28 18:19:22  davidk
 *    Added idCompT parameter to SQL Query.
 *
 * 
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
  
/* ewdb_api_GetSelectedStations.c */

static char SQL_STRING[500];
static char SQL_STRING_BASE[] =
 "select idSite, idChan, idComp, sSta, sComp, sNet, sLoc, dAzm, dDip, "
         " dLat, dLon, dElev, tOn, tOff, idCompT, idChanT, tOnCompT, tOffCompT "
 "from ALL_STATION_INFO_W_SITE";

#define szWHERE " WHERE "
#define szAND   " AND "

#define szBASE 18
#define szSTTM 0
#define szENTM 1
#define szLATN 2
#define szLATX 3
#define szLONN 4
#define szLONX 5
#define szSTA  6
#define szCOMP 7
#define szNET  8
#define szLOC  9
#define szCHAN 10

/* Total number of fields, currently BASE + VLMD + 1 */
#define NUM_FIELDS 29


static char SQL_STRING_OPT[][80] = 
    {
      "tOff > :starttime",
      "tOn < :endtime",
      "dLat>=:minlat",
      "dLat<=:maxlat",
      "dLon>=:minlon",
      "dLon<=:maxlon",
      "sSta = :station_name",
      "sComp = :comp_name",
      "sNet = :net_name",
      "(sLoc =  :loc_name or(sLoc IS NULL and :loc_name IS NULL))",
      "idChan = :IN_idChan"
    };

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, "1idSite"},
  {0,1,0,0,0,OA_EWDBID, "2Local_idChan"},
  {0,1,0,0,0,OA_EWDBID, "3idComp"},
  {0,1,10,0,0,OA_SZ,    "4sSta"},
  {0,1,10,0,0,OA_SZ,    "5sComp"},
  {0,1,10,0,0,OA_SZ,    "6sNet"},
  {0,1,10,0,0,OA_SZ,    "7sLoc"},
  {0,1,20,0,0,OA_FLOAT, "8dAzm"},
  {0,1,20,0,0,OA_FLOAT, "9dDip"},
  {0,1,20,0,0,OA_DOUBLE,"10dLat"},
  {0,1,20,0,0,OA_DOUBLE,"11dLon"},
  {0,1,20,0,0,OA_DOUBLE,"12dElev"},
  {0,1,20,0,0,OA_DOUBLE,"13tOn"},
  {0,1,20,0,0,OA_DOUBLE,"14tOff"},
  {0,1,0,0,0,OA_EWDBID, "15idCompT"},
  {0,1,0,0,0,OA_EWDBID, "16idChanT"},
  {0,1,20,0,0,OA_DOUBLE,"17tOnCompT"},
  {0,1,20,0,0,OA_DOUBLE,"18tOffCompT"},
  {0,1,0,0,0,OA_DOUBLE,   ":starttime"},
  {0,1,0,0,0,OA_DOUBLE,   ":endtime"},
  {0,1,0,0,0,OA_DOUBLE,":MinLat"},
  {0,1,0,0,0,OA_DOUBLE,":MaxLat"},
  {0,1,0,0,0,OA_DOUBLE,":MinLon"},
  {0,1,0,0,0,OA_DOUBLE,":MaxLon"},
  {0,0,0,0,0,OA_SZ,":station_name"},
  {0,0,0,0,0,OA_SZ,":comp_name"},
  {0,0,0,0,0,OA_SZ,":net_name"},
  {0,0,0,0,0,OA_SZ,":loc_name"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idChan"}
};

/* Insertion Struct for GetSelectedStations statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  EWDBid  Local_idChan;
static char Local_szMinLat[15],Local_szMaxLat[15],Local_szMinLon[15], Local_szMaxLon[15],
            Local_szSta[15],Local_szComp[15],Local_szNet[15],Local_szLoc[15],
            Local_sztStartTime[20],Local_sztEndTime[20];


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetSelectedStationsExec(EWDB_ChannelStruct * pCSCriteria,
                                EWDB_ChannelStruct * pCSMax, 
                                int iCriteria,
                                EWDB_Cursor * ppCursor);
static int PostGetSelectedStationsExec(EWDB_ChannelStruct * pBuffer, 
                                int BufferRecLen);
static char * AddCriteria(char * MainString, int NumAdditions, 
                          char * pCriteria);
static int InitGetSelectedStationsStatement(char * Local_szStatement, 
                                     EWDB_OCIStatementStruct *pSS);


/*********************************************************************** 
   Used to get the list of all Stations in the database, in a given time range,
     that meet a ReviewedBy criteria
	pBuffer is a pointer to a buffer allocated by the caller and used
	 by GetSelectedStations to store the array of Stations.
  pBuffer[0] should contain the criteria for the List selection.
	len is the length of the event array in event structures, allocated
	 by the caller.
  pCriteria is a pointer to the Stationstruct that contains the list formulation
   criteria.
	 StartTime and EndTime are time_t's that define the time threshhold for
	  selecting Stations.
   Stationstruct members:
    ReviewedBy is the name of the person who last reviewed the event.
 		 "*" defines all Stations
		 "**" defines all Stations reviewed
		 "" (Blank String) defines all Stations not reviewed
		 Any other string will be matched against the name in the ReviewedBy
		  field of an event.
   See the Stationstruct definition in ora_api.h for info on the other
    struct members.

	pNumStations is the number of event structures returned by GetSelectedStations.

      Return Value: -1 when an error occurs
					-x when the buffer passed by the caller is too
					 small, where x is the neccessary size of len
					 in Stationstructs to get all of the Stations for the given
					 criteria.
					0 on success.

    Other Details:  Caller is responsible for allocating
	space for the event list buffer.
***********************************************************************/
int ewdb_api_GetSelectedStations(EWDB_ChannelStruct * pBuffer, int iBufferLen,
                                 EWDB_ChannelStruct * pCSCriteria, 
                                 EWDB_ChannelStruct * pCSMaxCriteria, 
                                 int iCriteria,
                                 int * pNumStationsFound, 
                                 int * pNumStationsRetrieved)
{

  EWDB_Cursor  pCursor;

  
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
    /* Establishes connection, and performs binding!?! */
  {
     return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetSelectedStationsExec(pCSCriteria,pCSMaxCriteria,iCriteria,&pCursor)
      != EWDB_RETURN_SUCCESS )
  {
    logit("","ewdb_api_GetSelectedStations(): PrepGetSelectedStationsExec() failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetSelectedStations(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetSelectedStations(): ewdb_base_SQLCommit",2);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  if((*pNumStationsFound=PostGetSelectedStationsExec((EWDB_ChannelStruct *)pBuffer,iBufferLen)) 
      == EWDB_RETURN_FAILURE )
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();
  if(*pNumStationsFound > iBufferLen)
  {
    *pNumStationsRetrieved=iBufferLen;
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    *pNumStationsRetrieved=*pNumStationsFound;
    return(EWDB_RETURN_SUCCESS);
  }
}  /* end ewdb_api_GetSelectedStations() */


static int InitGetSelectedStationsStatement(char * Local_szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idSite*/
    iRecordSize += sizeof(EWDBid); /*Local_idChan*/
    iRecordSize += sizeof(EWDBid); /*idComp*/
    iRecordSize += pSS->FieldArray[3].Ind; /*sSta*/
    iRecordSize += pSS->FieldArray[4].Ind; /*sComp*/
    iRecordSize += pSS->FieldArray[5].Ind; /*sNet*/
    iRecordSize += pSS->FieldArray[6].Ind; /*sLoc*/
    iRecordSize += pSS->FieldArray[7].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[8].Ind; /*dDip*/
    iRecordSize += pSS->FieldArray[9].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[10].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[11].Ind; /*dElev*/
    iRecordSize += pSS->FieldArray[12].Ind; /*tOn*/
    iRecordSize += pSS->FieldArray[13].Ind; /*tOff*/
    iRecordSize += sizeof(EWDBid); /*idCompT*/
    iRecordSize += sizeof(EWDBid); /*Local_idChanT*/
    iRecordSize += pSS->FieldArray[16].Ind; /*tOnCompT*/
    iRecordSize += pSS->FieldArray[17].Ind; /*tOffCompT*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);  /* idSite */
  
    pSS->FieldArray[1].pVal= (void *) (
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);  /* Local_idChan */
  
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);  /* idComp */
  
    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* sSta */
  
    pSS->FieldArray[4].pVal= (void *) (
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* sComp */
  
    pSS->FieldArray[5].pVal= (void *) (
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* sNet */
  
    pSS->FieldArray[6].pVal= (void *) (
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* sLoc */
  
    pSS->FieldArray[7].pVal= (void *) (
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* dAzm */

    pSS->FieldArray[8].pVal= (void *) (
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind;  /* dDip */
  
    pSS->FieldArray[9].pVal= (void *) (
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind;  /* float dLat */
  
    pSS->FieldArray[10].pVal= (void *) (
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind;  /* float dLon */
  
    pSS->FieldArray[11].pVal= (void *) (
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind;  /* float dElev */
  
    pSS->FieldArray[12].pVal= (void *) (
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind;  /* double tOn */
  
    pSS->FieldArray[13].pVal= (void *) (
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[13].Ind;  /* double tOff */
  
    pSS->FieldArray[14].pVal= (void *) (
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);  /* idCompT */
  
    pSS->FieldArray[15].pVal= (void *) (
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);  /* Local_idChanT */
  
    pSS->FieldArray[16].pVal= (void *) (
      (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[16].Ind;  /* double tOnCompT */
  
    pSS->FieldArray[17].pVal= (void *) (
      (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[17].Ind;  /* double tOffCompT */
  
  
    pSS->FieldArray[18].pVal=Local_sztStartTime;
    pSS->FieldArray[19].pVal=Local_sztEndTime;
    pSS->FieldArray[20].pVal=Local_szMinLat;
    pSS->FieldArray[21].pVal=Local_szMaxLat;
    pSS->FieldArray[22].pVal=Local_szMinLon;
    pSS->FieldArray[23].pVal=Local_szMaxLon;
    pSS->FieldArray[24].pVal=Local_szSta;
    pSS->FieldArray[25].pVal=Local_szComp;
    pSS->FieldArray[26].pVal=Local_szNet;
    pSS->FieldArray[27].pVal=Local_szLoc;
    pSS->FieldArray[28].pVal=&Local_idChan;

  }

  if(!pLocalBuffer)
    logit("","InitGetSelectedStationsStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(Local_szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetSelectedStationsStatement() */

        
static char * AddCriteria(char * MainString,int NumAdditions, char * pCriteria)
{
  if(NumAdditions == 0)
    strcat(MainString,szWHERE);
  else
    strcat(MainString,szAND);
  return(strcat(MainString,pCriteria));
}


static int PrepGetSelectedStationsExec(EWDB_ChannelStruct * pCSCriteria,
                                       EWDB_ChannelStruct * pCSMax, 
                                       int iCriteria,
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

  Local_szMinLat[0]=Local_szMaxLat[0]=Local_szMinLon[0]=Local_szMaxLon[0] = 0;

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
  SQLParamsBindArray[szBASE+szLATN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLATX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szSTA].UseField = FALSE;
  SQLParamsBindArray[szBASE+szCOMP].UseField = FALSE;
  SQLParamsBindArray[szBASE+szNET].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLOC].UseField = FALSE;
  SQLParamsBindArray[szBASE+szCHAN].UseField = FALSE;



  /* Create the base string */
  sprintf(SQL_STRING,"%s",SQL_STRING_BASE);

  if(iCriteria & EWDB_CRITERIA_USE_TIME)
  {
    sprintf(Local_sztStartTime, "%.0f", pCSCriteria->tOn);
    sprintf(Local_sztEndTime, "%.0f", pCSCriteria->tOff);

    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szSTTM]);
    SQLParamsBindArray[szBASE+szSTTM].UseField=1;
    StatementAdditions++;
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szENTM]);
    SQLParamsBindArray[szBASE+szENTM].UseField=1;
    StatementAdditions++;
  }

  if(iCriteria & EWDB_CRITERIA_USE_LAT)
  {
    sprintf(Local_szMinLat, "%.5f", pCSCriteria->Comp.Lat);
    sprintf(Local_szMaxLat, "%.5f", pCSMax->Comp.Lat);
    sprintf(Local_szMinLon, "%.5f", pCSCriteria->Comp.Lon);
    sprintf(Local_szMaxLon, "%.5f", pCSMax->Comp.Lon);

    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLATN]);
    SQLParamsBindArray[szBASE+szLATN].UseField=1;
    StatementAdditions++;
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLATX]);
    SQLParamsBindArray[szBASE+szLATX].UseField=1;
    StatementAdditions++;
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONN]);
    SQLParamsBindArray[szBASE+szLONN].UseField=1;
    StatementAdditions++;
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONX]);
    SQLParamsBindArray[szBASE+szLONX].UseField=1;
    StatementAdditions++;
  }

  if(iCriteria & EWDB_CRITERIA_USE_SCNL)
  {
    strcpy(Local_szSta,  pCSCriteria->Comp.Sta);
    strcpy(Local_szComp, pCSCriteria->Comp.Comp);
    strcpy(Local_szNet,  pCSCriteria->Comp.Net);
    strcpy(Local_szLoc,  pCSCriteria->Comp.Loc);

    if(strcmp(pCSCriteria->Comp.Sta, "*"))
    {
      AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szSTA]);
      SQLParamsBindArray[szBASE+szSTA].UseField=1;
      StatementAdditions++;
    }

    if(strcmp(pCSCriteria->Comp.Comp, "*"))
    {
      AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szCOMP]);
      SQLParamsBindArray[szBASE+szCOMP].UseField=1;
      StatementAdditions++;
    }
    
    if(strcmp(pCSCriteria->Comp.Net, "*"))
    {
      AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szNET]);
      SQLParamsBindArray[szBASE+szNET].UseField=1;
      StatementAdditions++;
    }
    
    if(strcmp(pCSCriteria->Comp.Loc, "*"))
    {
      AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLOC]);
      SQLParamsBindArray[szBASE+szLOC].UseField=1;
      StatementAdditions++;
    }

  }

  if(iCriteria & EWDB_CRITERIA_USE_IDCHAN)
  {
    Local_idChan = pCSCriteria->Comp.idChan;

    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szCHAN]);
    SQLParamsBindArray[szBASE+szCHAN].UseField=1;
    StatementAdditions++;
  }



  /* Set the query to order by Origin Time */
  strcat(SQL_STRING," ORDER BY sSta, sNet, sComp, sLoc");

  if(InitGetSelectedStationsStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetSelectedStationsExec() */


static int PostGetSelectedStationsExec(EWDB_ChannelStruct * pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into EWDB_StationListStruct format, 
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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSelectedStationsExec:ewdb_base_SQLFetchRows",1);
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

        pBuffer[UCurr].pChanProps = NULL;

        pBuffer[UCurr].idSite=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].Comp.idChan=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[1].pVal));
        
        pBuffer[UCurr].Comp.idComp=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[2].pVal));
        
        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal));
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Comp.Sta,pTemp);

        pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + (int)(pSS->FieldArray[4].pVal));
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Comp.Comp,pTemp);

        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal));
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Comp.Net,pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + (int)(pSS->FieldArray[6].pVal));
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Comp.Loc,pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[7].Ind*BCurr) + (int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Comp.Azm=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Comp.Dip=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal) );
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Comp.Lat=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[10].Ind*BCurr) + (int)(pSS->FieldArray[10].pVal) );
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Comp.Lon=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[11].Ind*BCurr) + (int)(pSS->FieldArray[11].pVal) );
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Comp.Elev=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[12].Ind*BCurr) + (int)(pSS->FieldArray[12].pVal) );
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOn=atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[13].Ind*BCurr) + (int)(pSS->FieldArray[13].pVal) );
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOff=atof(pTemp);

        pBuffer[UCurr].idCompT=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[14].pVal));
        
        pBuffer[UCurr].idChanT=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[15].pVal));

        pTemp=(char *) ((pSS->FieldArray[16].Ind*BCurr) + (int)(pSS->FieldArray[16].pVal) );
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOnCompT=atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[17].Ind*BCurr) + (int)(pSS->FieldArray[17].pVal) );
        pTemp[pSS->FieldArray[17].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOffCompT=atof(pTemp);

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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSelectedStationsExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  

  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* end PostGetSelectedStationsExec() */
