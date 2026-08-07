/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsAudit.c,v 1.11 2005/06/10 16:07:48 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsAudit.c,v $
 *     Revision 1.11  2005/06/10 16:07:48  davidk
 *     DB Cleanup.  Fixed comments.
 *
 *     Revision 1.10  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.9  2005/02/03 20:37:37  mark
 *     Added groups and recipientdeliveries
 *
 *     Revision 1.8  2004/03/17 21:05:27  lucky
 *     *** empty log message ***
 *
 *     Revision 1.7  2003/10/09 17:56:51  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.6  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.5  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.4  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.3  2001/07/20 17:33:23  lucky
 *     Added InvocationString
 *
 *     Revision 1.2  2001/06/26 17:24:13  lucky
 *     State of the code after Utah specs have been met.
 *
 *     Revision 1.1  2001/05/15 02:16:15  davidk
 *     Initial revision
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>


static char SQL_STRING[] =
         "select idAudit, IN_idEvent, bAuto, idRecipient, "
         "       idFormat, sTableName, idDelivery, tAlarmDeclared, tAlarmExecuted, "
         "       sUsrDescription, sFmtInsert, sFmtDelete, sFmtDescription, "
         "       sInvocationString, idCore, iAlarmType, idGroup, idRecipientDelivery "
         " from ALL_AlarmsAR_INFO_W_Format "
         " where idEvent = :IN_idEvent ";

static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,   OA_EWDBID,  "1idAudit"},
  {0,1,0,0,0,   OA_EWDBID,  "2idEvent"},
  {0,1,0,0,0,   OA_INT,     "3bAuto"},
  {0,1,0,0,0,   OA_EWDBID,  "4idRecipient"},
  {0,1,0,0,0,   OA_EWDBID,  "5idFormat"},
  {0,1,256,0,0, OA_SZ,      "6sTableName"},
  {0,1,0,0,0,   OA_EWDBID,  "7idDelivery"},
  {0,1,20,0,0,  OA_DOUBLE,  "8tAlarmDeclared"},
  {0,1,20,0,0,  OA_DOUBLE,  "9tAlarmExecuted"},
  {0,1,256,0,0, OA_SZ,      "10sUsrDescription"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,
           0,0, OA_SZ,      "11sFmtInsert"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,
           0,0, OA_SZ,      "12sFmtDelete"},
  {0,1,256,0,0, OA_SZ,      "13sFmtDescription"},
  {0,1,256,0,0, OA_SZ,      "14sInvocationString"},
  {0,1,0,0,0,   OA_EWDBID,  "15idCore"},
  {0,1,0,0,0,   OA_EWDBID,  "16iAlarmType"},
  {0,1,0,0,0,   OA_EWDBID,  "17idGroup"},
  {0,1,0,0,0,   OA_EWDBID,  "18idRecipientDelivery"},
  {0,1,0,0,0,   OA_EWDBID,  ":IN_idEvent"}
};


/* define the total number of fields to be bound */
#define NUM_FIELDS   19

/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 245760
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;
static int    bInitialized=FALSE;

static EWDBid Local_idEvent;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetAlarmsAuditExec (int, EWDB_Cursor *);
static int InitGetAlarmsAuditStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetAlarmsAuditExec (EWDB_AlarmAuditStruct *, int);
/*******************************/


int ewdb_api_GetAlarmsAudit(EWDBid IN_idEvent, EWDB_AlarmAuditStruct *pAudit,   
                            int *pNumFound, int *pNumRetrieved, int BufferLen)
{

  EWDB_Cursor pCursor;

  if ((pAudit == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "ewdb_api_GetAlarmsAudit(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsAudit(): Call to ewdb_base_Reconnect failed.\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetAlarmsAuditExec (IN_idEvent, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsAudit(): PrepGetAlarmsAuditExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_GetAlarmsAudit:ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAlarmsAudit(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  if ((*pNumFound = PostGetAlarmsAuditExec (pAudit, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetAlarmsAudit(): PostGetAlarmsAuditExec failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (*pNumFound <= BufferLen)
  {
    *pNumRetrieved = *pNumFound;
    return (EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumRetrieved = BufferLen;
    return (EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetAlarmsAudit() */


/******************* InitGetAlarmsAuditStatement *******************/
static int InitGetAlarmsAuditStatement (char *szStatement,   
                                        EWDB_OCIStatementStruct *pSS)
{
  int    LastSize, i; 

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idAudit*/
    iRecordSize += sizeof(EWDBid); /*IN_idEvent*/
    iRecordSize += sizeof(int);    /*bAuto*/
    iRecordSize += sizeof(EWDBid); /*idRecipient*/
    iRecordSize += sizeof(EWDBid); /*idFormat*/
    iRecordSize += pSS->FieldArray[5].Ind; /*sTableName*/
    iRecordSize += sizeof(EWDBid); /*idDelivery*/
    iRecordSize += pSS->FieldArray[7].Ind; /*tAlarmDeclared*/
    iRecordSize += pSS->FieldArray[8].Ind; /*tAlarmExecuted*/
    iRecordSize += pSS->FieldArray[9].Ind; /*sUsrDescription*/
    iRecordSize += pSS->FieldArray[10].Ind;/*sFmtInsert*/
    iRecordSize += pSS->FieldArray[11].Ind;/*sFmtDelete*/
    iRecordSize += pSS->FieldArray[12].Ind;/*sFmtDescription*/
    iRecordSize += pSS->FieldArray[13].Ind;/*sInvocationString*/
    iRecordSize += sizeof(EWDBid); /*idCore*/
    iRecordSize += sizeof(EWDBid); /*iAlarmType*/
    iRecordSize += sizeof(EWDBid); /*idGroup*/
    iRecordSize += sizeof(EWDBid); /*idRecipientDelivery*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
    
    pSS->FieldArray[0].pVal = &(pLocalBuffer[0]);
    LastSize = sizeof(int);  /* idAudit */

    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* IN_idEvent */

    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* bAuto */

    pSS->FieldArray[3].pVal= (void *) ((int)(pSS->FieldArray[2].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* idRecipient*/

    pSS->FieldArray[4].pVal= (void *) ((int)(pSS->FieldArray[3].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* idFormat */

    pSS->FieldArray[5].pVal= (void *) ((int)(pSS->FieldArray[4].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[5].Ind;  /* sTableName */

    pSS->FieldArray[6].pVal= (void *) ((int)(pSS->FieldArray[5].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* idDelivery */

    pSS->FieldArray[7].pVal= (void *) ((int)(pSS->FieldArray[6].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[7].Ind;  /* tAlarmDeclared */

    pSS->FieldArray[8].pVal= (void *) ((int)(pSS->FieldArray[7].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[8].Ind;   /* tAlarmExecuted */

    pSS->FieldArray[9].pVal= (void *) ((int)(pSS->FieldArray[8].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[9].Ind;   /* sDescription - Recipient */

    pSS->FieldArray[10].pVal= (void *) ((int)(pSS->FieldArray[9].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[10].Ind;   /* sFmtInsert */

    pSS->FieldArray[11].pVal= (void *) ((int)(pSS->FieldArray[10].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[11].Ind;  /* sFmtDelete */

    pSS->FieldArray[12].pVal= (void *) ((int)(pSS->FieldArray[11].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[12].Ind;  /* sDescription - Format */

    pSS->FieldArray[13].pVal= (void *) ((int)(pSS->FieldArray[12].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[13].Ind;  /* sInvocationString */
    
    pSS->FieldArray[14].pVal= (void *) ((int)(pSS->FieldArray[13].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* idCore */
    
    pSS->FieldArray[15].pVal= (void *) ((int)(pSS->FieldArray[14].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* iAlarmType */

    pSS->FieldArray[16].pVal= (void *) ((int)(pSS->FieldArray[15].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* iAlarmType */

    pSS->FieldArray[17].pVal= (void *) ((int)(pSS->FieldArray[16].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* iAlarmType */

    pSS->FieldArray[18].pVal= &Local_idEvent;  /* incoming IN_idEvent */

  } /* end if(!pLocalBuffer) */
  

  if(!pLocalBuffer)
  {
    logit("","InitGetAlarmsAuditStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));


}  /* End InitGetAlarmsAuditStatement() */


/******************* PrepGetAlarmsAuditExec *******************/
static int PrepGetAlarmsAuditExec (int IN_idEvent, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  Local_idEvent = IN_idEvent;

  if(InitGetAlarmsAuditStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);
}  /* end PrepGetAlarmsAuditExec() */


/******************* PostGetAlarmsAuditExec *******************/
static int PostGetAlarmsAuditExec (EWDB_AlarmAuditStruct *pBuffer, int BufferRecLen)
{

  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  char * pTemp;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RowsProcessed;
  char * sTableName;

  while (!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);
    
    if (ewdb_base_SQLFetchRows (pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
        done=1;
      else
      {
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetAlarmsAuditExec:ewdb_base_SQLFetchRows", 1);
        return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
      }
    }

    RowsRetrieved=ewdb_base_GetCursorRowsProcessedCount(pCursor);

    for(; RowsDone < RowsRetrieved; RowsDone++)
    {
      if(RowsDone >= BufferRecLen)
      {
        done=1;
        break;
      }


      /* Copy from DB rows into the output buffer */
      BCurr = RowsDone % iRecordsPerBuffer;
      UCurr = RowsDone;

      /* field 1: idAudit */
      pBuffer[UCurr].idAudit = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[0].pVal));

      /* field 2: IN_idEvent */
      pBuffer[UCurr].idEvent = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[1].pVal));

      /* field 3: bAuto */
      pBuffer[UCurr].bAuto = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[2].pVal));

      /* field 4: idRecipient */
      pBuffer[UCurr].Recipient.idRecipient = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[3].pVal));

      /* field 5: idFormat */
      pBuffer[UCurr].Format.idFormat = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[4].pVal));

      /* field 6: sTableName */
      pTemp = (char *) ((pSS->FieldArray[5].Ind * BCurr) +
                      (int)(pSS->FieldArray[5].pVal));
      pTemp[pSS->FieldArray[5].pRetLens[BCurr]] = 0;
      sTableName = pTemp;

      if (strcmp (sTableName, "email") == 0)
        pBuffer[UCurr].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_EMAIL;
      else if (strcmp (sTableName, "pager") == 0)
        pBuffer[UCurr].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PAGER;
      else if (strcmp (sTableName, "phone") == 0)
        pBuffer[UCurr].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PHONE;
      else if (strcmp (sTableName, "qdds") == 0)
        pBuffer[UCurr].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_QDDS;
      else if (strcmp (sTableName, "custom") == 0)
        pBuffer[UCurr].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_CUSTOM;
      else
      {
        logit ("", "Unknown delivery table: %s.\n", sTableName);
        return 0;
      }

      /* field 7: idDelivery */
      pBuffer[UCurr].idDelivery = *(int *)((sizeof (int) * BCurr) + 
                        (int)(pSS->FieldArray[6].pVal));

      /* field 8: tAlarmDeclared */
            pTemp = (char *) ((pSS->FieldArray[7].Ind * BCurr) +
                                        (int)(pSS->FieldArray[7].pVal));
            pTemp[pSS->FieldArray[7].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].tAlarmDeclared = atof (pTemp);

      /* field 9: tAlarmExecuted */
            pTemp = (char *) ((pSS->FieldArray[8].Ind * BCurr) +
                                        (int)(pSS->FieldArray[8].pVal));
            pTemp[pSS->FieldArray[8].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].tAlarmExecuted = atof (pTemp);

      /* field 10: sDescription -- recipient */
      pTemp = (char *) ((pSS->FieldArray[9].Ind * BCurr) +
                      (int)(pSS->FieldArray[9].pVal));
      pTemp[pSS->FieldArray[9].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].Recipient.sDescription, pTemp);

      /* field 11: sFmtInsert */
      pTemp = (char *) ((pSS->FieldArray[10].Ind * BCurr) +
                      (int)(pSS->FieldArray[10].pVal));
      pTemp[pSS->FieldArray[10].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].Format.sFmtInsert, pTemp);

      /* field 12: sFmtDelete */
      pTemp = (char *) ((pSS->FieldArray[11].Ind * BCurr) +
                      (int)(pSS->FieldArray[11].pVal));
      pTemp[pSS->FieldArray[11].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].Format.sFmtDelete, pTemp);

      /* field 13: sDescription -- format */
      pTemp = (char *) ((pSS->FieldArray[12].Ind * BCurr) +
                      (int)(pSS->FieldArray[12].pVal));
      pTemp[pSS->FieldArray[12].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].Format.sDescription, pTemp);

      /* field 14: sInvocationString */
      pTemp = (char *) ((pSS->FieldArray[13].Ind * BCurr) +
                      (int)(pSS->FieldArray[13].pVal));
      pTemp[pSS->FieldArray[13].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sInvocationString, pTemp);

      /* field 15: idCore */
      pBuffer[UCurr].idCore = *(int *)((sizeof (int) * BCurr) + 
                        (int)(pSS->FieldArray[14].pVal));

      /* field 16: iAlarmType */
      pBuffer[UCurr].AlarmType = *(int *)((sizeof (int) * BCurr) + 
                        (int)(pSS->FieldArray[15].pVal));

      /* field 17: idGroup */
      pBuffer[UCurr].idGroup = *(int *)((sizeof (EWDBid) * BCurr) + 
                        (int)(pSS->FieldArray[16].pVal));

      /* field 18: idRecipientDelivery */
      pBuffer[UCurr].idRecipientDelivery = *(int *)((sizeof (EWDBid) * BCurr) + 
                        (int)(pSS->FieldArray[17].pVal));

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
      ewdb_base_ErrorReport(hEWDBC, pCursor,
                       "PostGetAlarmsAuditExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetAlarmsAuditExec() */

