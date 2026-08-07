/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsRules.c,v 1.12 2005/06/10 16:07:48 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsRules.c,v $
 *     Revision 1.12  2005/06/10 16:07:48  davidk
 *     DB Cleanup.  Fixed comments.
 *
 *     Revision 1.11  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.10  2005/03/07 18:54:50  mark
 *     Added bSecondary to rules
 *
 *     Revision 1.9  2005/02/03 20:39:15  mark
 *     Added groups; fixed parsing bugs
 *
 *     Revision 1.8  2005/01/19 21:34:23  mark
 *     Added iPhases and bUseMag to alarms rule struct
 *
 *     Revision 1.7  2004/12/08 22:26:34  mark
 *     Added idPolygon field
 *
 *     Revision 1.6  2003/10/22 15:50:17  davidk
 *     Enlarged the BUFFERSIZE of the internal buffer to incorporate the large record size > 8k.
 *
 *     Revision 1.5  2003/10/09 17:56:58  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.4  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.3  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.2  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.1  2001/05/15 02:16:15  davidk
 *     Initial revision
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_oci_base_internal.h>


/* 
  We work in two ways: (1) get all rules, and 
  (2) get a specific rule
*/

static char SQL_STRING[512];

static char SQL_STRING_BASE[] =
        "select IN_idRule, bAuto, sFmtInsert, sFmtDelete, sDescription, idCritProgram, "
        "       idRecipientDelivery, dMag, idPolygon, bUseMag, iPhases, idGroup, bSecondary, "
        "       idFormat "
        " from ALL_AlarmsRule_INFO_W_Format";


static char SQL_STRING_WHERE[] =
        " where idRule = :IN_idRule";

static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,   OA_INT,     "1idRule"},
  {0,1,0,0,0,   OA_INT,     "2bAuto"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,0,0, OA_SZ,      "3sFmtInsert"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,0,0, OA_SZ,      "4sFmtDelete"},
  {0,1,256,0,0, OA_SZ,      "5sDescription"},
  {0,1,0,0,0,   OA_INT,     "6idCritProgram"},
  {0,1,0,0,0,   OA_INT,     "7idRecipientDelivery"},
  {0,1,20,0,0,  OA_DOUBLE,  "8dMag"},
  {0,1,0,0,0,   OA_INT,     "9idPolygon"},
  {0,1,0,0,0,   OA_INT,     "10bUseMag"},
  {0,1,0,0,0,   OA_INT,     "11iPhases"},
  {0,1,0,0,0,   OA_INT,     "12idGroup"},
  {0,1,0,0,0,   OA_INT,     "13bSecondary"},
  {0,1,0,0,0,   OA_INT,     "14idFormat"},
  {0,1,0,0,0,   OA_INT,     ":IN_idRule"},
};


/* define the total number of fields to be bound */
#define NUM_FIELDS   15


/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 245760
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static  int     Local_idRule;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int InitGetAlarmsRulesStatement(char *, EWDB_OCIStatementStruct *);
static int PrepGetAlarmsRulesExec(int, EWDB_Cursor *);
static int PostGetAlarmsRulesExec(EWDB_AlarmsRuleStruct *, int);
/*******************************/

int ewdb_api_GetAlarmsRules(int IN_idRule, EWDB_AlarmsRuleStruct *pRule,   
                            int *pNumFound, int *pNumRetrieved, int BufferLen)
{
  EWDB_Cursor pCursor;

  if ((pRule == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "%s:ERROR: Invalid arguments passed in.\n",
           "ewdb_api_GetAlarmsRules()");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "%s:ERROR: Call to ewdb_base_Reconnect failed.\n",
           "ewdb_api_GetAlarmsRules()");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetAlarmsRulesExec (IN_idRule, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", " ewdb_api_GetAlarmsRules(): PrepGetAlarmsRulesExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor," ewdb_api_GetAlarmsRules(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAlarmsRules(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  if ((*pNumFound = PostGetAlarmsRulesExec (pRule, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", " ewdb_api_GetAlarmsRules(): PostGetAlarmsRulesExec failed.\n");
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
}  /* end ewdb_api_GetAlarmsRules() */


/******************* InitGetAlarmsRulesStatement *******************/
static int InitGetAlarmsRulesStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);         /*IN_idRule*/
    iRecordSize += sizeof(int);            /*bAuto*/
    iRecordSize += pSS->FieldArray[2].Ind; /*sFmtInsert*/
    iRecordSize += pSS->FieldArray[3].Ind; /*sFmtDelete*/
    iRecordSize += pSS->FieldArray[4].Ind; /*sDescription*/
    iRecordSize += sizeof(EWDBid);         /*idCritProgram*/
    iRecordSize += sizeof(EWDBid);         /*idRecipientDelivery*/
    iRecordSize += pSS->FieldArray[7].Ind; /*dMag*/
    iRecordSize += sizeof(EWDBid);         /*idPolygon*/
    iRecordSize += sizeof(int);            /*bUseMag*/
    iRecordSize += sizeof(int);            /*iPhases*/
    iRecordSize += sizeof(EWDBid);         /*idGroup*/
    iRecordSize += sizeof(int);            /*bSecondary*/
    iRecordSize += sizeof(EWDBid);         /*idFormat*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = sizeof(int); /* IN_idRule */
    
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* bAuto */
    
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[2].Ind; /* sFmtInsert */
    
    pSS->FieldArray[3].pVal= (void *) ((int)(pSS->FieldArray[2].pVal) + 
      (LastSize * iRecordsPerBuffer)); 
    LastSize = pSS->FieldArray[3].Ind; /* sFmtDelete */
    
    pSS->FieldArray[4].pVal= (void *) ((int)(pSS->FieldArray[3].pVal) + 
      (LastSize * iRecordsPerBuffer)); 
    LastSize = pSS->FieldArray[4].Ind; /* sDescription */
    
    pSS->FieldArray[5].pVal= (void *) ((int)(pSS->FieldArray[4].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* idCritProgram */
    
    pSS->FieldArray[6].pVal= (void *) ((int)(pSS->FieldArray[5].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* idRecipientDelivery */
    
    pSS->FieldArray[7].pVal= (void *) ((int)(pSS->FieldArray[6].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[7].Ind; /* dMag */
    
    pSS->FieldArray[8].pVal= (void *) ((int)(pSS->FieldArray[7].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (EWDBid); /* idPolygon */
        
    pSS->FieldArray[9].pVal= (void *) ((int)(pSS->FieldArray[8].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* bUseMag */
    
    pSS->FieldArray[10].pVal= (void *) ((int)(pSS->FieldArray[9].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* iPhases */
    
    pSS->FieldArray[11].pVal= (void *) ((int)(pSS->FieldArray[10].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (EWDBid); /* idGroup */

    pSS->FieldArray[12].pVal= (void *) ((int)(pSS->FieldArray[11].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* bSecondary */

    pSS->FieldArray[13].pVal= (void *) ((int)(pSS->FieldArray[12].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (EWDBid); /* idFormat */
    
    pSS->FieldArray[14].pVal= &Local_idRule;     /* incoming IN_idRule */
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetAlarmsRulesStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetAlarmsRulesStatement() */



/******************* PrepGetAlarmsRulesExec *******************/
static int PrepGetAlarmsRulesExec(int IN_idRule, EWDB_Cursor *ppCursor)
{
  int rc;

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetAlarmsRulesExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (IN_idRule < 0)
  {
    SQLParamsBindArray[14].UseField = FALSE;
    sprintf(SQL_STRING, "%s%s", SQL_STRING_BASE, SQL_STRING_WHERE);
  }
  else
  {
    Local_idRule = IN_idRule;
    SQLParamsBindArray[14].UseField = TRUE;
    sprintf(SQL_STRING, "%s", SQL_STRING_BASE);
  }

  rc = InitGetAlarmsRulesStatement (SQL_STRING, &SSStatement);

  *ppCursor = SSStatement.pCda;

  return(rc);
}  /* end PrepGetAlarmsRulesExec() */


/******************* PostGetAlarmsRulesExec *******************/
static int PostGetAlarmsRulesExec(EWDB_AlarmsRuleStruct *pBuffer, int BufferRecLen)
{

  int           done = 0;
  int           RowsRetrieved = 0;
  int           RowsDone = 0;
  EWDB_Cursor       pCursor = SSStatement.pCda;
  char           *pTemp;
  int           BCurr,UCurr;
  int           RowsProcessed;
  EWDB_OCIStatementStruct  *pSS=&SSStatement;

  while (!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if (ewdb_base_SQLFetchRows (pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
        done=1;
      else
      {
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetAlarmsRulesExec:ewdb_base_SQLFetchRows", 1);
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

      /* field 1: IN_idRule */
      pBuffer[UCurr].idRule = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[0].pVal));

      /* field 2: bAuto */
      pBuffer[UCurr].Auto = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[1].pVal));

      /* field 3: sFmtInsert */
            pTemp = (char *) ((pSS->FieldArray[2].Ind * BCurr) +
                                        (int)(pSS->FieldArray[2].pVal));
            pTemp[pSS->FieldArray[2].pRetLens[BCurr]] = 0;
            strcpy (pBuffer[UCurr].Format.sFmtInsert, pTemp);

      /* field 4: sFmtDelete */
            pTemp = (char *) ((pSS->FieldArray[3].Ind * BCurr) +
                                        (int)(pSS->FieldArray[3].pVal));
            pTemp[pSS->FieldArray[3].pRetLens[BCurr]] = 0;
            strcpy (pBuffer[UCurr].Format.sFmtDelete, pTemp);

      /* field 5: sDescription */
            pTemp = (char *) ((pSS->FieldArray[4].Ind * BCurr) +
                                        (int)(pSS->FieldArray[4].pVal));
            pTemp[pSS->FieldArray[4].pRetLens[BCurr]] = 0;
            strcpy (pBuffer[UCurr].Format.sDescription, pTemp);

      /* field 6: idCritProgram */
      pBuffer[UCurr].CritProg.idCritProgram = 
                *(int *)((sizeof (EWDBid) * BCurr) + 
                      (int)(pSS->FieldArray[5].pVal));

      /* field 7: idRecipientDelivery */
      pBuffer[UCurr].idRecipientDelivery = *(int *)((sizeof (EWDBid) * BCurr) + 
                      (int)(pSS->FieldArray[6].pVal));

      /* field 8: dMag */
            pTemp = (char *) ((pSS->FieldArray[7].Ind * BCurr) +
                                        (int)(pSS->FieldArray[7].pVal));
            pTemp[pSS->FieldArray[7].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dMag = atof (pTemp);

      /* field 9: idPolygon */
      pBuffer[UCurr].idPolygon = *(int *)((sizeof (EWDBid) * BCurr) + 
                      (int)(pSS->FieldArray[8].pVal));

      /* field 10: bUseMag */
      pBuffer[UCurr].bUseMag = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[9].pVal));

      /* field 11: iPhases */
      pBuffer[UCurr].iPhases = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[10].pVal));

      /* field 12: idGroup */
      pBuffer[UCurr].idGroup = *(int *)((sizeof (EWDBid) * BCurr) + 
                      (int)(pSS->FieldArray[11].pVal));

      /* field 13: bSecondary */
      pBuffer[UCurr].bSecondary = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[12].pVal));

      /* field 13: idFormat */
      pBuffer[UCurr].Format.idFormat = *(int *)((sizeof (EWDBid) * BCurr) + 
                      (int)(pSS->FieldArray[13].pVal));


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
                       "PostGetAlarmsRulesExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetAlarmsRulesExec() */


