
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetTransformFunction.c,v 1.5 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetTransformFunction.c,v $
 *     Revision 1.5  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2003/09/16 17:00:14  davidk
 *     General API Cleanup of internal structures.
 *
 *     Revision 1.3  2003/08/21 00:45:16  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.1  2001/07/23 17:10:39  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/05/15 02:16:31  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <internal/ewdb_internal_functions.h>


static char SQL_STRING[] =
         /* To get all channels from a certain message */
         "select cPZType,dReal,dImaginary "
         "from ALL_PZ "
         "where idCTF = :IN_idCookedTF ";



static EWDB_OCI_SFS SQLParamsBindArray[] = 
{

  {0,1,0,0,0,OA_CHAR,"1cPZType"},
  {0,1,20,0,0,OA_DOUBLE,"2dReal"},
  {0,1,20,0,0,OA_DOUBLE,"3dImaginary"},
  {0,1,0,0,0,OA_INT,":IN_idCookedTF"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 4

/* Insertion Struct for GetTransFunc statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static EWDBid   Local_idCookedTF;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetTransFuncExec(EWDBid IN_idCookedTF, EWDB_Cursor * ppCursor);
static int PostGetTransFuncExec(EWDB_TransformFunctionStruct * pCookedTF);
static int InitGetTransFuncStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);

/* Retrieves a Transform function from the DB, using the CookedTF 
   record identifier(Local_idCookedTF).
*********************************************************************/
int ewdb_internal_GetTransformFunction(EWDBid Local_idCookedTF, 
                                       EWDB_TransformFunctionStruct * pCookedTF)
{
  EWDB_Cursor pCursor;

  if(pCookedTF== NULL)
  {
    logit("", "ewdb_internal_GetTransformFunction():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_internal_GetTransformFunction(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetTransFuncExec(Local_idCookedTF, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_internal_GetTransformFunction():PrepGetTransFuncExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,
                          "ewdb_internal_GetTransformFunction(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                          "ewdb_internal_GetTransformFunction(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(PostGetTransFuncExec(pCookedTF) == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_internal_GetTransformFunction(): PostCreateTransFuncExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_GetTransformFunction() */


static int InitGetTransFuncStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize +=  2;                    /*cPZType*/
    iRecordSize += pSS->FieldArray[1].Ind;/*dReal*/
    iRecordSize += pSS->FieldArray[2].Ind;/*dImaginary*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize= 2 * sizeof(char);
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* sta char[10] */
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* comp char[10] */
    pSS->FieldArray[3].pVal=&Local_idCookedTF;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetTransFuncStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetTransFuncStatement() */


static int PrepGetTransFuncExec(EWDBid IN_idCookedTF, EWDB_Cursor * ppCursor)
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

  Local_idCookedTF = IN_idCookedTF;

  if(InitGetTransFuncStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetTransFuncExec() */


static int PostGetTransFuncExec(EWDB_TransformFunctionStruct * pCookedTF)
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
  int BufferRecLen = 2*EWDB_MAX_POLES_OR_ZEROES;
  char cTemp;
  double dReal,dImag;

  pCookedTF->iNumPoles=0;
  pCookedTF->iNumZeroes=0;


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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetTransFuncExec:ewdb_base_SQLFetchRows",1);
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

        cTemp=*((char*)((int)(pSS->FieldArray[0].pVal) + BCurr));

        pTemp=(char *) 
           ((pSS->FieldArray[1].Ind*BCurr) +(int)(pSS->FieldArray[1].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        dReal=atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[2].Ind*BCurr) +(int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        dImag=atof(pTemp);

        if(cTemp == EWDB_PZTYPE_POLE)
        {
          if(pCookedTF->iNumPoles < EWDB_MAX_POLES_OR_ZEROES)
          {
            pCookedTF->Poles[pCookedTF->iNumPoles].dReal=dReal;
            pCookedTF->Poles[pCookedTF->iNumPoles].dImag=dImag;
            pCookedTF->iNumPoles++;
          }
          else
          {
            logit("","PostGetTransFuncExec(): ERROR!! Too many poles "
                  "for Local_idCookedTF %d, %.4f,%.4f ignored!\n",
                  pCookedTF->idCookedTF,dReal,dImag);
          }
        }
        else if(cTemp == EWDB_PZTYPE_ZERO)
        {
          if(pCookedTF->iNumZeroes < EWDB_MAX_POLES_OR_ZEROES)
          {
            pCookedTF->Zeroes[pCookedTF->iNumZeroes].dReal=dReal;
            pCookedTF->Zeroes[pCookedTF->iNumZeroes].dImag=dImag;
            pCookedTF->iNumZeroes++;
          }
          else
          {
            logit("","PostGetTransFuncExec(): ERROR!! Too many zeroes for "
                  "Local_idCookedTF %d, %.4f,%.4f ignored!\n",
                  pCookedTF->idCookedTF,dReal,dImag);
          }
        }
        else 
        {
          logit("","PostGetTransFuncExec(): ERROR!! Invalid cPZType:%d "
                "for Local_idCookedTF %d(%.7f,%.7f)\n",
                cTemp,Local_idCookedTF,dReal,dImag);
        }


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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetGetTransFuncExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetTransFuncExec() */
