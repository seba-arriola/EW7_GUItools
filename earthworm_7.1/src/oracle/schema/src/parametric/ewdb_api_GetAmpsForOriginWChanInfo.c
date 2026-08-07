/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAmpsForOriginWChanInfo.c,v 1.5 2005/06/17 21:00:03 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAmpsForOriginWChanInfo.c,v $
 *     Revision 1.5  2005/06/17 21:00:03  davidk
 *     Fixed problems in SQL code caused by errant global replacements during API cleanup.
 *
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2004/11/06 04:27:05  davidk
 *     Updated phasename field(s) in the SQL statement structs, to use the new
 *     EWDB_PHASENAME_SIZE length, as it is used in EWDB_ArrivalStructs,
 *     which is where the results are headed.
 *
 *     Revision 1.2  2004/09/08 19:00:47  davidk
 *     Fixed two minor bugs (argument order in SQL string differed from params list),
 *     one item in PostXXX() had wrong argument 221 instead of 22.
 *     Missing AND when adding MagType constraint.
 *     Tested, and appears to work(with small caveats).
 *
 *     Revision 1.1  2004/05/22 18:41:27  davidk
 *     Monster query to retrieve amp info for origin (via pick->amp).
 *
 *     Revision 1.4  2004/04/23 20:06:07  mark
 *     Changed DB calls to match reality
 *
 *     Revision 1.3  2003/09/16 16:56:55  davidk
 *     General API Cleanup
 *
 *     Revision 1.2  2003/08/21 00:57:41  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     *** empty log message ***
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[768];

static char SQL_STRING_BASE[] =
         /* To get all messages from a certain time */
        "select idPick, xidPick, idChan, sPhase, tPhase, "
        " cMotion, cOnset, dSigma, "
        " idOriginPick, IN_idOrigin, sCalcPhase, tCalcPhase, "
        " dWeight, dDist, dArrivalAzm, dTakeoff, tResPick, "
        " idPeakAmp, xidExternal, dPeakAmp1, "
        " tPeriod1, tAmp1, dPeakAmp2, tPeriod2, tAmp2, "
        " iMagType, tStartInterval, tEndInterval, "
        " tOn, tOff, idChanT, idComp, sSta, sComp, "
        " sNet, sLoc, dAzm, dDip, dLat, dLon, dElev, idCompT, "
        " tOnCompT, tOffCompT "
        "from ALL_AMPS_FOR_ORIGIN_BY_PICK "
        "where idOrigin= :IN_idOrigin ";
        
static char SQL_STRING_MAGTYPE[] =  "iMagType = :IN_iMagType";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, "1idPick"},
  {0,1,16,0,0,OA_SZ,    "2xidPick"},
  {0,1,0,0,0,OA_EWDBID, "3idChan"},
  {0,1,EWDB_PHASENAME_SIZE,
          0,0,OA_SZ,    "4sPhase"},
  {0,1,20,0,0,OA_DOUBLE,"5tPhase"},
  {0,1,2,0,0,OA_SZ,     "6cMotion"},
  {0,1,2,0,0,OA_SZ,     "7cOnset"},
  {0,1,20,0,0,OA_DOUBLE,"8dSigma"},
  {0,1,0,0,0,OA_INT,    "9idOriginPick"},
  {0,1,0,0,0,OA_INT,    "10idOrigin"},
  {0,1,6,0,0,OA_SZ,     "11sCalcPhase"},
  {0,1,20,0,0,OA_DOUBLE,"12tCalcPhase"},
  {0,1,20,0,0,OA_FLOAT, "13dWeight"},
  {0,1,20,0,0,OA_FLOAT, "14dDist"},
  {0,1,20,0,0,OA_FLOAT, "15dArrivalAzm"},
  {0,1,20,0,0,OA_FLOAT, "16dTakeoff"},
  {0,1,20,0,0,OA_DOUBLE,"17tResPick"},
  {0,1,0,0,0,OA_EWDBID, "18idPeakAmp"},
  {0,1,16,0,0,OA_SZ,    "19xidExternal"},
  {0,1,20,0,0,OA_FLOAT, "20dPeakAmp1"},
  {0,1,20,0,0,OA_FLOAT, "21tPeriod1"},
  {0,1,20,0,0,OA_DOUBLE,"22tAmp1"},
  {0,1,20,0,0,OA_FLOAT, "23dPeakAmp2"},
  {0,1,20,0,0,OA_FLOAT, "24tPeriod2"},
  {0,1,20,0,0,OA_DOUBLE,"25tAmp2"},
  {0,1,0,0,0, OA_SHORT, "26iMagType"},
  {0,1,20,0,0,OA_DOUBLE,"27tStartInterval"},
  {0,1,20,0,0,OA_DOUBLE,"28tEndInterval"},
  {0,1,20,0,0,OA_DOUBLE,"29tOn"},
  {0,1,20,0,0,OA_DOUBLE,"30tOff"},
  {0,1,0,0,0, OA_EWDBID,"31idChanT"},
  {0,1,0,0,0, OA_EWDBID,"32idComp"},
  {0,1,10,0,0,OA_SZ,    "33sSta"},
  {0,1,10,0,0,OA_SZ,    "34sComp"},
  {0,1,10,0,0,OA_SZ,    "35sNet"},
  {0,1,10,0,0,OA_SZ,    "36sLoc"},
  {0,1,20,0,0,OA_FLOAT, "37dAzm"},
  {0,1,20,0,0,OA_FLOAT, "38dDip"},
  {0,1,20,0,0,OA_DOUBLE,"39dLat"},
  {0,1,20,0,0,OA_DOUBLE,"40dLon"},
  {0,1,20,0,0,OA_DOUBLE,"41dElev"},
  {0,1,0,0,0, OA_EWDBID,"42idCompT"},
  {0,1,20,0,0,OA_DOUBLE,"43tOnCompT"},
  {0,1,20,0,0,OA_DOUBLE,"44tOffCompT"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idOrigin"},
  {0,1,0,0,0,OA_INT,    ":IN_iMagType"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 46

/* Insertion Struct for GetAmpsForOriginWChanInfo statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 32768
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;


static  int     Local_iMagType;
static  EWDBid  Local_idOrigin;

/****************************************************************/
/****************************************************************/
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetAmpsForOriginWChanInfoExec(EWDBid IN_idORigin, 
                                             MAGNITUDE_TYPE IN_iMagType, 
                                             EWDB_Cursor * ppCursor);
static int PostGetAmpsForOriginWChanInfoExec(EWDB_PeakAmpStruct * pAmpBuffer, 
                                             EWDB_ArrivalStruct * pArrBuffer,
                                             EWDB_ChannelStruct * pChanBuffer,
                                             int BufferRecLen);
static int InitGetAmpsForOriginWChanInfoStatement(char * szStatement, 
                                EWDB_OCIStatementStruct *pSS);


/* 
   Retrieves a list of peak amplitudes (along with associated arrival and
   channel information, for a given origin.  Amps are associated via
   origin -> pick -> amplitude.


    Return Value:   
     EWDB_RETURN_SUCCESS   Success.
     EWDB_RETURN_FAILURE   Error
     EWDB_RETURN_WARNING   WARNING: The buffer passed by the caller is too
                           small, where *pNumItemsFound is the neccessary 
                           size of iBufferLen to get all of the picks for the 
                           given origin.
                           Results that fit in the buffer are passed back.

    Other Details:  Caller is responsible for allocating
                    space for the amps buffer.
*/
int ewdb_api_GetAmpsForOriginWChanInfo(EWDBid IN_idOrigin, MAGNITUDE_TYPE iMagType,
                                       EWDB_PeakAmpStruct * pAmpBuffer,
                                       EWDB_ArrivalStruct * pArrBuffer,
                                       EWDB_ChannelStruct * pChanBuffer,
                                       int * pNumItemsFound, int * pNumItemsRetrieved,
                                       int iBufferLen)
{

  EWDB_Cursor  pCursor;
 
  if(IN_idOrigin <= 0)
  {
    logit("","ewdb_api_GetAmpsForOriginWChanInfo():  bad IN_idOrigin(%d). \n", IN_idOrigin);
    return(EWDB_RETURN_FAILURE);
  }

  if(!(pAmpBuffer && pArrBuffer && pChanBuffer && 
       pNumItemsFound && pNumItemsRetrieved))
  {
    logit("","ewdb_api_GetAmpsForOriginWChanInfo():  NULL input pointer: %u %u %u %u\n",
          pAmpBuffer, pArrBuffer, pChanBuffer, pNumItemsFound, pNumItemsRetrieved);
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetAmpsForOriginWChanInfoExec(IN_idOrigin, iMagType, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetAmpsForOriginWChanInfo(): PrepGetAmpsForOriginWChanInfoExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"GetAmpsForOriginWChanInfo:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAmpsForOriginWChanInfo(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((*pNumItemsFound = PostGetAmpsForOriginWChanInfoExec(pAmpBuffer,pArrBuffer,pChanBuffer,iBufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetAmpsForOriginWChanInfo(): PostGetAmpsForOriginWChanInfoExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumItemsFound <= iBufferLen)
  {
    *pNumItemsRetrieved = *pNumItemsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumItemsRetrieved = iBufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetAmpsForOriginWChanInfo() */


static int InitGetAmpsForOriginWChanInfoStatement(char * szStatement, 
                                                  EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idPick*/
    iRecordSize += pSS->FieldArray[1].Ind; /*xidPick*/
    iRecordSize += sizeof(EWDBid); /*idChan*/
    iRecordSize += pSS->FieldArray[3].Ind; /*sPhase*/
    iRecordSize += pSS->FieldArray[4].Ind; /*tPhase*/
    iRecordSize += pSS->FieldArray[5].Ind; /*cMotion*/
    iRecordSize += pSS->FieldArray[6].Ind; /*cOnset*/
    iRecordSize += pSS->FieldArray[7].Ind; /*dSigma*/
    iRecordSize += sizeof(EWDBid); /*idOriginPick*/
    iRecordSize += sizeof(EWDBid); /*IN_idOrigin*/
    iRecordSize += pSS->FieldArray[10].Ind; /*sCalcPhase*/
    iRecordSize += pSS->FieldArray[11].Ind; /*tCalcPhase*/
    iRecordSize += pSS->FieldArray[12].Ind; /*dWeight*/
    iRecordSize += pSS->FieldArray[13].Ind; /*dDist*/
    iRecordSize += pSS->FieldArray[14].Ind; /*dArrivalAzm*/
    iRecordSize += pSS->FieldArray[15].Ind; /*dTakeOff*/
    iRecordSize += pSS->FieldArray[16].Ind; /*tResPick*/

    iRecordSize += sizeof(EWDBid); /*idPeakAmp*/
    iRecordSize += pSS->FieldArray[18].Ind; /*xidExternal*/
    iRecordSize += pSS->FieldArray[19].Ind; /*dAmp1*/
    iRecordSize += pSS->FieldArray[20].Ind; /*tPeriod1*/
    iRecordSize += pSS->FieldArray[21].Ind; /*tAmp1*/
    iRecordSize += pSS->FieldArray[22].Ind; /*dAmp2*/
    iRecordSize += pSS->FieldArray[23].Ind; /*tPeriod2*/
    iRecordSize += pSS->FieldArray[24].Ind; /*tAmp2*/
    iRecordSize += sizeof(int); /*iMagType*/
    iRecordSize += pSS->FieldArray[26].Ind; /*tStartInterval*/
    iRecordSize += pSS->FieldArray[27].Ind; /*tEndInterval*/
    iRecordSize += pSS->FieldArray[28].Ind; /*tOnCompT*/
    iRecordSize += pSS->FieldArray[29].Ind; /*tOffCompT*/
    iRecordSize += sizeof(EWDBid); /*idChanT*/

    iRecordSize += sizeof(EWDBid); /*idComp*/
    iRecordSize += pSS->FieldArray[32].Ind; /*sSta*/
    iRecordSize += pSS->FieldArray[33].Ind; /*sComp*/
    iRecordSize += pSS->FieldArray[34].Ind; /*sNet*/
    iRecordSize += pSS->FieldArray[35].Ind; /*sLoc*/
    iRecordSize += pSS->FieldArray[36].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[37].Ind; /*dDip*/
    iRecordSize += pSS->FieldArray[38].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[39].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[40].Ind; /*dElev*/
    iRecordSize += sizeof(EWDBid); /*idCompT*/
    iRecordSize += pSS->FieldArray[42].Ind; /*tOnCompT*/
    iRecordSize += pSS->FieldArray[43].Ind; /*tOffCompT*/

 
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    
    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* xidPick */
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idChan */
    pSS->FieldArray[3].pVal=(void *)(
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* sPhase */
    pSS->FieldArray[4].pVal=(void *)(
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* tPhase */
    pSS->FieldArray[5].pVal=(void *)(
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* cMotion */
    pSS->FieldArray[6].pVal=(void *)(
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* cOnset */
    pSS->FieldArray[7].pVal=(void *)(
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* dSigma */
    pSS->FieldArray[8].pVal=(void *)(
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idOriginPick */
    pSS->FieldArray[9].pVal=(void *)(
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* IN_idOrigin */
    pSS->FieldArray[10].pVal=(void *)(
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /* sCalcPhase */
    pSS->FieldArray[11].pVal=(void *)(
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind; /* tCalcPhase */
    pSS->FieldArray[12].pVal=(void *)(
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind; /* dWeight */
    pSS->FieldArray[13].pVal=(void *)(
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[13].Ind; /* dDist */
    pSS->FieldArray[14].pVal=(void *)(
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[14].Ind; /* dArrivalAzm */
    pSS->FieldArray[15].pVal=(void *)(
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[15].Ind; /* dTakeOff */
    pSS->FieldArray[16].pVal=(void *)(
      (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[16].Ind; /* tResPick */
    
    pSS->FieldArray[17].pVal=(void *)(
      (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idPeakAmp */
    pSS->FieldArray[18].pVal=(void *)(
      (int)(pSS->FieldArray[17].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[18].Ind; /* xidExternal */
    pSS->FieldArray[19].pVal=(void *)(
      (int)(pSS->FieldArray[18].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[19].Ind; /* dAmp1 */
    pSS->FieldArray[20].pVal=(void *)(
      (int)(pSS->FieldArray[19].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[20].Ind; /* tPeriod1 */
    pSS->FieldArray[21].pVal=(void *)(
      (int)(pSS->FieldArray[20].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[21].Ind; /* tAmp1 */
    pSS->FieldArray[22].pVal=(void *)(
      (int)(pSS->FieldArray[21].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[22].Ind; /* tAmp2 */
    pSS->FieldArray[23].pVal=(void *)(
      (int)(pSS->FieldArray[22].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[23].Ind; /* tPeriod2 */
    pSS->FieldArray[24].pVal=(void *)(
      (int)(pSS->FieldArray[23].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[24].Ind; /* tAmp2 */
    pSS->FieldArray[25].pVal=(void *)(
      (int)(pSS->FieldArray[24].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* iMagType */
    pSS->FieldArray[26].pVal=(void *)(
      (int)(pSS->FieldArray[25].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[26].Ind; /* tStartInterval */
    pSS->FieldArray[27].pVal=(void *)(
      (int)(pSS->FieldArray[26].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[27].Ind; /* tEndInterval */
    pSS->FieldArray[28].pVal=(void *)(
      (int)(pSS->FieldArray[27].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[28].Ind; /* tOnCompT */
    pSS->FieldArray[29].pVal=(void *)(
      (int)(pSS->FieldArray[28].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[29].Ind; /* tOffCompT */
    pSS->FieldArray[30].pVal=(void *)(
      (int)(pSS->FieldArray[29].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idChanT */
    
    
    pSS->FieldArray[31].pVal=(void *)(
      (int)(pSS->FieldArray[30].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idComp */
    pSS->FieldArray[32].pVal=(void *)(
      (int)(pSS->FieldArray[31].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[32].Ind; /* sSta */
    pSS->FieldArray[33].pVal=(void *)(
      (int)(pSS->FieldArray[32].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[33].Ind; /* sComp */
    pSS->FieldArray[34].pVal=(void *)(
      (int)(pSS->FieldArray[33].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[34].Ind; /* sNet */
    pSS->FieldArray[35].pVal=(void *)(
      (int)(pSS->FieldArray[34].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[35].Ind; /* sLoc */
    pSS->FieldArray[36].pVal=(void *)(
      (int)(pSS->FieldArray[35].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[36].Ind; /* dAzm */
    pSS->FieldArray[37].pVal=(void *)(
      (int)(pSS->FieldArray[36].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[37].Ind; /* dDip */
    pSS->FieldArray[38].pVal=(void *)(
      (int)(pSS->FieldArray[37].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[38].Ind; /* dLat */
    pSS->FieldArray[39].pVal=(void *)(
      (int)(pSS->FieldArray[38].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[39].Ind; /* dLon */
    pSS->FieldArray[40].pVal=(void *)(
      (int)(pSS->FieldArray[39].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[40].Ind; /* dElev */
    pSS->FieldArray[41].pVal=(void *)(
      (int)(pSS->FieldArray[40].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idCompT */
    pSS->FieldArray[42].pVal=(void *)(
      (int)(pSS->FieldArray[41].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[42].Ind; /* tOnCompT */
    pSS->FieldArray[43].pVal=(void *)(
      (int)(pSS->FieldArray[42].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[43].Ind; /* tOffCompT */
    
    pSS->FieldArray[44].pVal=&Local_idOrigin;
    pSS->FieldArray[45].pVal=&Local_iMagType;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetAmpsForOriginWChanInfo_w_StaInfoStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetAmpsForOriginWChanInfo_w_StaInfoStatement() */


static int PrepGetAmpsForOriginWChanInfoExec(EWDBid IN_idOrigin, MAGNITUDE_TYPE IN_iMagType, 
                          EWDB_Cursor * ppCursor)
{

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  Local_idOrigin  = IN_idOrigin;
  Local_iMagType  = IN_iMagType;

  if(Local_iMagType > 0)
  {
    sprintf(SQL_STRING,"%s AND %s",SQL_STRING_BASE, SQL_STRING_MAGTYPE);
    SSStatement.FieldArray[45].UseField = TRUE;
  }
  else
  {
    strcpy(SQL_STRING,SQL_STRING_BASE);
    SSStatement.FieldArray[45].UseField = FALSE;
  }

  if(InitGetAmpsForOriginWChanInfoStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetAmpsForOriginWChanInfoExec() */


static int PostGetAmpsForOriginWChanInfoExec(EWDB_PeakAmpStruct * pAmpBuffer, 
                                             EWDB_ArrivalStruct * pArrBuffer,
                                             EWDB_ChannelStruct * pChanBuffer,
                                             int BufferRecLen)
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


  while(!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetAmpsForOriginWChanInfoExec:ewdb_base_SQLFetchRows",1);
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

        memset(&pAmpBuffer[UCurr], 0, sizeof(EWDB_PeakAmpStruct));
        memset(&pArrBuffer[UCurr], 0, sizeof(EWDB_ArrivalStruct));
        memset(&pChanBuffer[UCurr], 0, sizeof(EWDB_ChannelStruct));


        /* idPick */
        pArrBuffer[UCurr].idPick=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));
        pAmpBuffer[UCurr].idPick = pArrBuffer[UCurr].idPick;

        /* xidExternal - Pick */
        pTemp=(char *) 
           ((pSS->FieldArray[1].Ind*BCurr) +(int)(pSS->FieldArray[1].pVal));
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        strncpy(pArrBuffer[UCurr].szExternalPickID, pTemp, sizeof(pArrBuffer[UCurr].szExternalPickID)-1);

        /* idChan */
        pAmpBuffer[UCurr].idChan=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[2].pVal));
        pChanBuffer[UCurr].Comp.idChan = pArrBuffer[UCurr].idChan = pAmpBuffer[UCurr].idChan;

        /* sPhase */
        pTemp=(char *) 
           ((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal));
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        strncpy(pArrBuffer[UCurr].szObsPhase, pTemp, sizeof(pArrBuffer[UCurr].szObsPhase)-1);

        /* tPhase */
        pTemp=(char *) 
           ((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal));
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].tObsPhase=atof(pTemp);

        /* cMotion */
        pTemp=(char *) 
           ((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal));
        pArrBuffer[UCurr].cMotion=*pTemp;

        /* cOnset */
        pTemp=(char *) 
           ((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal));
        pArrBuffer[UCurr].cOnset=*pTemp;

        /* dSigma */
        pTemp=(char *) 
           ((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal));
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].dSigma=atof(pTemp);

        /* idOriginPick */
        pArrBuffer[UCurr].idOriginPick=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[8].pVal));

        /* IN_idOrigin */
        pArrBuffer[UCurr].idOrigin=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[9].pVal));

        /* sCalcPhase */
        pTemp=(char *) 
           ((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal));
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        strncpy(pArrBuffer[UCurr].szCalcPhase, pTemp, sizeof(pArrBuffer[UCurr].szCalcPhase)-1);

        /* tCalcPhase */
        pTemp=(char *) 
           ((pSS->FieldArray[11].Ind*BCurr) +(int)(pSS->FieldArray[11].pVal));
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].tCalcPhase=atof(pTemp);

        /* dWeight - Pick */
        pTemp=(char *) 
           ((pSS->FieldArray[12].Ind*BCurr) +(int)(pSS->FieldArray[12].pVal));
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].dWeight=(float)atof(pTemp);

        /* dDist */
        pTemp=(char *) 
           ((pSS->FieldArray[13].Ind*BCurr) +(int)(pSS->FieldArray[13].pVal));
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].dDist=(float)atof(pTemp);

        /* dAzm */
        pTemp=(char *) 
           ((pSS->FieldArray[14].Ind*BCurr) +(int)(pSS->FieldArray[14].pVal));
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].dAzm=(float)atof(pTemp);

        /* dTakeoff */
        pTemp=(char *) 
           ((pSS->FieldArray[15].Ind*BCurr) +(int)(pSS->FieldArray[15].pVal));
        pTemp[pSS->FieldArray[15].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].dTakeoff=(float)atof(pTemp);

        /* tResPick */
        pTemp=(char *) 
           ((pSS->FieldArray[16].Ind*BCurr) +(int)(pSS->FieldArray[16].pVal));
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        pArrBuffer[UCurr].tResPick=atof(pTemp);

        /* idPeakAmp */
        pAmpBuffer[UCurr].idPeakAmp=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[17].pVal));

        /* xidExternal */
        pTemp=(char *) 
           ((pSS->FieldArray[18].Ind*BCurr) +(int)(pSS->FieldArray[18].pVal));
        pTemp[pSS->FieldArray[18].pRetLens[BCurr]]=0;
        strncpy(pAmpBuffer[UCurr].szExternalAmpID, pTemp, sizeof(pAmpBuffer[UCurr].szExternalAmpID)-1);

        /* dPeakAmp1 */
        pTemp=(char *) 
           ((pSS->FieldArray[19].Ind*BCurr) +(int)(pSS->FieldArray[19].pVal));
        pTemp[pSS->FieldArray[19].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmp1=(float)atof(pTemp);

        /* dPeriod1 */
        pTemp=(char *) 
           ((pSS->FieldArray[20].Ind*BCurr) +(int)(pSS->FieldArray[20].pVal));
        pTemp[pSS->FieldArray[20].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmpPeriod1=(float)atof(pTemp);

        /* tAmp1 */
        pTemp=(char *) 
           ((pSS->FieldArray[21].Ind*BCurr) +(int)(pSS->FieldArray[21].pVal));
        pTemp[pSS->FieldArray[21].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tAmp1=atof(pTemp);

        /* dPeakAmp2 */
        pTemp=(char *) 
           ((pSS->FieldArray[22].Ind*BCurr) +(int)(pSS->FieldArray[22].pVal));
        pTemp[pSS->FieldArray[22].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmp2=(float)atof(pTemp);

        /* dPeriod2 */
        pTemp=(char *) 
           ((pSS->FieldArray[23].Ind*BCurr) +(int)(pSS->FieldArray[23].pVal));
        pTemp[pSS->FieldArray[23].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmpPeriod2=(float)atof(pTemp);

        /* tAmp2 */
        pTemp=(char *) 
           ((pSS->FieldArray[24].Ind*BCurr) +(int)(pSS->FieldArray[24].pVal));
        pTemp[pSS->FieldArray[24].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tAmp2=atof(pTemp);

        /* iMagType */
        pAmpBuffer[UCurr].iAmpType=*(short *)
           ((sizeof(short)*BCurr) +(int)(pSS->FieldArray[25].pVal));

        /* tStartInterval */
        pTemp=(char *) 
           ((pSS->FieldArray[26].Ind*BCurr) +(int)(pSS->FieldArray[26].pVal));
        pTemp[pSS->FieldArray[26].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tStartInterval=atof(pTemp);

        /* tEndInterval */
        pTemp=(char *) 
           ((pSS->FieldArray[27].Ind*BCurr) +(int)(pSS->FieldArray[27].pVal));
        pTemp[pSS->FieldArray[27].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tEndInterval=atof(pTemp);

        /* tOn */
        pTemp=(char *) 
           ((pSS->FieldArray[28].Ind*BCurr) +(int)(pSS->FieldArray[28].pVal));
        pTemp[pSS->FieldArray[28].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].tOn=atof(pTemp);

        /* tOff */
        pTemp=(char *) 
           ((pSS->FieldArray[29].Ind*BCurr) +(int)(pSS->FieldArray[29].pVal));
        pTemp[pSS->FieldArray[29].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].tOff=atof(pTemp);

        /* idChanT */
        pChanBuffer[UCurr].idChanT=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[30].pVal));

        /* idComp */
        pChanBuffer[UCurr].Comp.idComp=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[31].pVal));

        /* sSta */
        pTemp=(char *) 
           ((pSS->FieldArray[32].Ind*BCurr) +(int)(pSS->FieldArray[32].pVal));
        pTemp[pSS->FieldArray[32].pRetLens[BCurr]]=0;
        strncpy(pChanBuffer[UCurr].Comp.Sta, pTemp, sizeof(pChanBuffer[UCurr].Comp.Sta)-1);

        /* sComp */
        pTemp=(char *) 
           ((pSS->FieldArray[33].Ind*BCurr) +(int)(pSS->FieldArray[33].pVal));
        pTemp[pSS->FieldArray[33].pRetLens[BCurr]]=0;
        strncpy(pChanBuffer[UCurr].Comp.Comp, pTemp, sizeof(pChanBuffer[UCurr].Comp.Comp)-1);

        /* sNet */
        pTemp=(char *) 
           ((pSS->FieldArray[34].Ind*BCurr) +(int)(pSS->FieldArray[34].pVal));
        pTemp[pSS->FieldArray[34].pRetLens[BCurr]]=0;
        strncpy(pChanBuffer[UCurr].Comp.Net, pTemp, sizeof(pChanBuffer[UCurr].Comp.Net)-1);

        /* sLoc */
        pTemp=(char *) 
           ((pSS->FieldArray[35].Ind*BCurr) +(int)(pSS->FieldArray[35].pVal));
        pTemp[pSS->FieldArray[35].pRetLens[BCurr]]=0;
        strncpy(pChanBuffer[UCurr].Comp.Loc, pTemp, sizeof(pChanBuffer[UCurr].Comp.Loc)-1);

        /* dAzm */
        pTemp=(char *) 
           ((pSS->FieldArray[36].Ind*BCurr) +(int)(pSS->FieldArray[36].pVal));
        pTemp[pSS->FieldArray[36].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].Comp.Azm=(float)atof(pTemp);

        /* dDip */
        pTemp=(char *) 
           ((pSS->FieldArray[37].Ind*BCurr) +(int)(pSS->FieldArray[37].pVal));
        pTemp[pSS->FieldArray[37].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].Comp.Dip=(float)atof(pTemp);

        /* dLat */
        pTemp=(char *) 
           ((pSS->FieldArray[38].Ind*BCurr) +(int)(pSS->FieldArray[38].pVal));
        pTemp[pSS->FieldArray[38].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].Comp.Lat=(float)atof(pTemp);

        /* dLon */
        pTemp=(char *) 
           ((pSS->FieldArray[39].Ind*BCurr) +(int)(pSS->FieldArray[39].pVal));
        pTemp[pSS->FieldArray[39].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].Comp.Lon=(float)atof(pTemp);

        /* dElev */
        pTemp=(char *) 
           ((pSS->FieldArray[40].Ind*BCurr) +(int)(pSS->FieldArray[40].pVal));
        pTemp[pSS->FieldArray[40].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].Comp.Elev=(float)atof(pTemp);

        /* idCompT */
        pChanBuffer[UCurr].idCompT=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[41].pVal));

        /* tOnCompT */
        pTemp=(char *) 
           ((pSS->FieldArray[42].Ind*BCurr) +(int)(pSS->FieldArray[42].pVal));
        pTemp[pSS->FieldArray[42].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].tOnCompT=atof(pTemp);

        /* tOffCompT */
        pTemp=(char *) 
           ((pSS->FieldArray[43].Ind*BCurr) +(int)(pSS->FieldArray[43].pVal));
        pTemp[pSS->FieldArray[43].pRetLens[BCurr]]=0;
        pChanBuffer[UCurr].tOffCompT=atof(pTemp);
      }
    } /* End for RowsDone < RowsRetrieved */
  }  /* End while !done */
  
  if(RowsRetrieved > BufferRecLen)
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetGetAmpsForOriginWChanInfoExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetAmpsForOriginWChanInfoExec() */


