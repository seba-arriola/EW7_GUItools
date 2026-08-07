/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetMagnitude.c,v 1.5 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetMagnitude.c,v $
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2001/07/12 21:07:17  davidk
 *     Cleaned up the function as part of the API cleanup.  Added improved
 *     error logging, handling, and reporting.
 *
 *     Revision 1.3  2001/05/24 00:53:05  davidk
 *     Added an OUT_idOrigin variable to the SQL query.  It had been left out by mistake before.  This call
 *     now works on Solaris.
 *
 *     Revision 1.2  2001/05/15 02:16:21  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_Magnitude(OUT_Retcode => :OUT_Retcode, IN_idMag => :IN_idMag, "
        " OUT_sSource => :OUT_sSource, OUT_dMagAvg => :OUT_dMagAvg, "
        " OUT_iNumMags => :OUT_iNumMags, OUT_dMagErr => :OUT_dMagErr, "
        " OUT_iMagType => :OUT_iMagType, OUT_idOrigin => :OUT_idOrigin); End;";



static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMag"},
  {0,1,0,0,0,OA_SZ,":OUT_sSource"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dMagAvg"},
  {0,1,0,0,0,OA_INT,":OUT_iNumMags"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dMagErr"},
  {0,1,0,0,0,OA_INT,":OUT_iMagType"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idOrigin"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 8

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

static EWDB_MagStruct Local_Mag;

static char Local_szdMagAvg[20],Local_szdMagErr[20];

static int Local_iRetCode;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetMagnitudeExec(EWDBid IN_idMag, EWDB_Cursor * ppCursor);
static int PostGetMagnitudeExec(EWDB_MagStruct * pMagnitude);
static int InitGetMagnitudeStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetMagnitude(EWDBid IN_idMag, EWDB_MagStruct * pMag)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idMag <= 0 || !pMag )
  {
    logit("","ewdb_api_GetMagnitude(): Invalid params passed in:\n"
             "IN_idMag %d, pMag %u, \n",
          IN_idMag, pMag);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetMagnitudeExec(IN_idMag,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetMagnitude(): PrepGetMagnitudeExec() failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetMagnitude(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetMagnitude(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if( PostGetMagnitudeExec(pMag) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetMagnitude(): PostGetMagnitudeExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetMagnitude() */


static int InitGetMagnitudeStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &(Local_Mag.idMag);
	pSS->FieldArray[2].pVal = Local_Mag.szSource;
	pSS->FieldArray[3].pVal = Local_szdMagAvg;
	pSS->FieldArray[4].pVal = &(Local_Mag.iNumMags);
	pSS->FieldArray[5].pVal = Local_szdMagErr;
	pSS->FieldArray[6].pVal = &(Local_Mag.iMagType);
	pSS->FieldArray[7].pVal = &(Local_Mag.idOrigin);

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepGetMagnitudeExec(EWDBid IN_idMag, EWDB_Cursor * ppCursor)
{

  /* initialize output buffers */
  memset(Local_szdMagAvg, 0, sizeof(Local_szdMagAvg));
  memset(Local_szdMagErr, 0, sizeof(Local_szdMagErr));
  memset(&Local_Mag, 0, sizeof(Local_Mag));

  /* copy user's data over to local buffers */
  Local_Mag.idMag=IN_idMag;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  if(InitGetMagnitudeStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetMagnitudeExec(EWDB_MagStruct * pMagnitude)
{

  if(Local_iRetCode >= 0)
  {
    Local_Mag.dMagAvg=(float)atof(Local_szdMagAvg);
    Local_Mag.dMagErr=(float)atof(Local_szdMagErr);

    /* Copy the results to the users struct */
    memcpy(pMagnitude,&Local_Mag,sizeof(EWDB_MagStruct));

    ewdb_base_ReleaseCursor (SSStatement.pCda);

    if(Local_iRetCode > 0)
    {
      logit("","PostGetMagnitudeExec(): SQL Proc Get_Magnitude() returned warning(%d).\n",Local_iRetCode);  
    }
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
      logit("","PostGetMagnitudeExec(): SQL Proc Get_Magnitude() returned error(%d).\n",Local_iRetCode);  
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

