/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetOrigin.c,v 1.9 2005/06/27 15:31:36 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetOrigin.c,v $
 *     Revision 1.9  2005/06/27 15:31:36  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.8  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.7  2005/05/19 23:23:01  davidk
 *     Added code to initialize the local string buffers in each PrepXXX()  call.
 *     Added call to ReleaseCursor() when there is an error in the sql proc.
 *
 *     Revision 1.6  2003/10/20 18:08:13  mark
 *     Added error reporting; other minor fixes
 *
 *     Revision 1.5  2003/06/10 14:38:55  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2003/01/16 18:40:26  lucky
 *     Added starting location fields (dLatStart, dLonStart, dDepthStart)
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     Cleaned up the function as part of the API cleanup.  Added improved
 *     error logging, handling, and reporting.
 *
 *     Revision 1.2  2001/05/15 02:16:21  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.6  2001/02/21 08:39:22  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.5  2000/09/07 21:12:25  lucky
 *     Added an additional parameter to retrieve both the Human Readable
 *      Author string and also the sSource from the Database. We use the
 *      sSource to create a new sSource for reviewed events.
 *
 *     Revision 1.4  2000/06/21 22:51:35  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.3  2000/05/12 23:58:14  davidk
 *     Added prototypes for each function implemented in the file.
 *
 *     Revision 1.2  1999/11/09 18:21:08  lucky
 *     *** empty log message ***
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_Origin(OUT_Retcode => :OUT_Retcode, IN_idOrigin => :IN_idOrigin, "
        " OUT_xidExternal => :OUT_xidExternal, "
        " OUT_sSource => :OUT_sSource, OUT_sRealSource => :OUT_sRealSource, OUT_tOrigin => :OUT_tOrigin, "
        " OUT_dLat => :OUT_dLat, OUT_dLon => :OUT_dLon, OUT_dDepth => :OUT_dDepth, "
        " OUT_iGap => :OUT_iGap, OUT_dDmin => :OUT_dDmin, OUT_dRMS => :OUT_dRMS, "
        " OUT_iAssocRd => :OUT_iAssocRd, OUT_iAssocPh => :OUT_iAssocPh, "
        " OUT_iUsedRd => :OUT_iUsedRd, OUT_iUsedPh => :OUT_iUsedPh, "
        " OUT_iE0Azm => :OUT_iE0Azm, OUT_iE0Dip => :OUT_iE0Dip, OUT_dE0 => :OUT_dE0, "
        " OUT_iE1Azm => :OUT_iE1Azm, OUT_iE1Dip => :OUT_iE1Dip, "
        " OUT_dE1 => :OUT_dE1, OUT_iE2Azm => :OUT_iE2Azm, OUT_iE2Dip => :OUT_iE2Dip, "
        " OUT_dE2 => :OUT_dE2, OUT_dErLat => :OUT_dErLat, OUT_dErLon => :OUT_dErLon, "
        " OUT_dErZ => :OUT_dErZ, OUT_tMCI => :OUT_tMCI, "
        " OUT_iFixedDepth => :OUT_iFixedDepth, OUT_Comment => :OUT_Comment); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_INT,":IN_idOrigin"},
  {0,1,0,0,0,OA_SZ,":OUT_xidExternal"},
  {0,1,0,0,0,OA_SZ,":OUT_sSource"},
  {0,1,0,0,0,OA_SZ,":OUT_sRealSource"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tOrigin"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dLat"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dLon"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dDepth"},
  {0,1,0,0,0,OA_INT,":OUT_iGap"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dDmin"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dRMS"},
  {0,1,0,0,0,OA_INT,":OUT_iAssocRd"},
  {0,1,0,0,0,OA_INT,":OUT_iAssocPh"},
  {0,1,0,0,0,OA_INT,":OUT_iUsedRd"},
  {0,1,0,0,0,OA_INT,":OUT_iUsedPh"},
  {0,1,0,0,0,OA_INT,":OUT_iE0Azm"},
  {0,1,0,0,0,OA_INT,":OUT_iE0Dip"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dE0"},
  {0,1,0,0,0,OA_INT,":OUT_iE1Azm"},
  {0,1,0,0,0,OA_INT,":OUT_iE1Dip"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dE1"},
  {0,1,0,0,0,OA_INT,":OUT_iE2Azm"},
  {0,1,0,0,0,OA_INT,":OUT_iE2Dip"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dE2"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dErLat"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dErLon"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dErZ"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tMCI"},
  {0,1,0,0,0,OA_INT,":OUT_iFixedDepth"},
  {0,1,0,0,0,OA_SZ,":OUT_Comment"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 31

/* Insertion Struct for GetOrigin statement */
static EWDB_OCIStatementStruct SSStatement;

static EWDB_OriginStruct Local_Origin;

static char Local_sztOrigin[20],Local_szdLat[20],Local_szdLon[20],
            Local_szdDepth[20],Local_szdDmin[20],Local_szdRms[20],Local_szdE0[20],
            Local_szdE1[20],Local_szdE2[20],Local_szdErLat[20],
            Local_szdErLon[20],Local_szdErZ[20],Local_szdMCI[20];

static int Local_iRetCode = 0;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PostGetOriginExec(EWDB_OriginStruct * pOrigin);
int PrepGetOriginExec(EWDBid IN_idOrigin, EWDB_Cursor * ppCursor);
int InitGetOriginStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetOrigin(EWDBid IN_idOrigin, EWDB_OriginStruct * pOrigin)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idOrigin <= 0 || !pOrigin)
  {
    logit("","ewdb_api_GetPicksByTime(): Invalid params passed in:\n"
             "IN_idOrigin %d, pOrigin %u\n",
          IN_idOrigin, pOrigin);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetOriginExec(IN_idOrigin,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetOrigin(): PrepGetOriginExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetOrigin(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetOrigin(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if( PostGetOriginExec(pOrigin) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetOrigin(): PostGetOriginExec() failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetOrigin() */


static int InitGetOriginStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  static int bInitialized = 0;

  if(!bInitialized)
  {
	  pSS->FieldArray[0].pVal = &Local_iRetCode;
	  pSS->FieldArray[1].pVal = &(Local_Origin.idOrigin);
	  pSS->FieldArray[2].pVal = Local_Origin.xidExternal;
	  pSS->FieldArray[3].pVal = Local_Origin.sSource;
	  pSS->FieldArray[4].pVal = Local_Origin.sRealSource;
	  pSS->FieldArray[5].pVal = Local_sztOrigin;
	  pSS->FieldArray[6].pVal = Local_szdLat;
	  pSS->FieldArray[7].pVal = Local_szdLon;
	  pSS->FieldArray[8].pVal = Local_szdDepth;
	  pSS->FieldArray[9].pVal = &(Local_Origin.iGap);
	  pSS->FieldArray[10].pVal = Local_szdDmin;
	  pSS->FieldArray[11].pVal = Local_szdRms;
	  pSS->FieldArray[12].pVal = &(Local_Origin.iAssocRd);
	  pSS->FieldArray[13].pVal = &(Local_Origin.iAssocPh);
	  pSS->FieldArray[14].pVal = &(Local_Origin.iUsedRd);
	  pSS->FieldArray[15].pVal = &(Local_Origin.iUsedPh);
	  pSS->FieldArray[16].pVal = &(Local_Origin.iE0Azm);
	  pSS->FieldArray[17].pVal = &(Local_Origin.iE0Dip);
	  pSS->FieldArray[18].pVal = Local_szdE0;
	  pSS->FieldArray[19].pVal = &(Local_Origin.iE1Azm);
	  pSS->FieldArray[20].pVal = &(Local_Origin.iE1Dip);
	  pSS->FieldArray[21].pVal = Local_szdE1;
	  pSS->FieldArray[22].pVal = &(Local_Origin.iE2Azm);
	  pSS->FieldArray[23].pVal = &(Local_Origin.iE2Dip);
	  pSS->FieldArray[24].pVal = Local_szdE2;
	  pSS->FieldArray[25].pVal = Local_szdErLat;
	  pSS->FieldArray[26].pVal = Local_szdErLon;
	  pSS->FieldArray[27].pVal = Local_szdErZ;
	  pSS->FieldArray[28].pVal = Local_szdMCI;
	  pSS->FieldArray[29].pVal = &(Local_Origin.iFixedDepth);
	  pSS->FieldArray[30].pVal = Local_Origin.Comment;
  }

  return(ewdb_base_RequestCursor(szStatement, pSS, 0/*don't force rebind*/));
}  /* end InitGetOriginStatement() */


static int PrepGetOriginExec(EWDBid IN_idOrigin, EWDB_Cursor * ppCursor)
{

  memset(&Local_Origin, 0, sizeof(Local_Origin));
  memset(Local_sztOrigin, 0, sizeof(Local_sztOrigin));
  memset(Local_szdLat, 0, sizeof(Local_szdLat));
  memset(Local_szdLon, 0, sizeof(Local_szdLon));
  memset(Local_szdDepth, 0, sizeof(Local_szdDepth));
  memset(Local_szdDmin, 0, sizeof(Local_szdDmin));
  memset(Local_szdE0, 0, sizeof(Local_szdE0));
  memset(Local_szdE1, 0, sizeof(Local_szdE1));
  memset(Local_szdE2, 0, sizeof(Local_szdE2));
  memset(Local_szdErLat, 0, sizeof(Local_szdErLat));
  memset(Local_szdErLon, 0, sizeof(Local_szdErLon));
  memset(Local_szdErZ, 0, sizeof(Local_szdErZ));
  memset(Local_szdMCI, 0, sizeof(Local_szdMCI));

  Local_Origin.idOrigin=IN_idOrigin;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  if(InitGetOriginStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetOriginExec(EWDB_OriginStruct * pOrigin)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetCode >= 0)
  {
    Local_Origin.tOrigin=atof(Local_sztOrigin);
    Local_Origin.dLat=(float)atof(Local_szdLat);
    Local_Origin.dLon=(float)atof(Local_szdLon);
    Local_Origin.dDepth=(float)atof(Local_szdDepth);
    Local_Origin.dDmin=(float)atof(Local_szdDmin);
    Local_Origin.dRms=(float)atof(Local_szdRms);
    Local_Origin.dE0=(float)atof(Local_szdE0);
    Local_Origin.dE1=(float)atof(Local_szdE1);
    Local_Origin.dE2=(float)atof(Local_szdE2);
    Local_Origin.dErrLat=(float)atof(Local_szdErLat);
    Local_Origin.dErrLon=(float)atof(Local_szdErLon);
    Local_Origin.dErZ=(float)atof(Local_szdErZ);
    Local_Origin.dMCI=atof(Local_szdMCI);

    /* Copy the results to the users struct */
    memcpy(pOrigin,&Local_Origin,sizeof(EWDB_OriginStruct));

    if(Local_iRetCode > 0)
    {
      logit("","PostGetOriginExec(): SQL Proc Get_Origin(%d) returned warning (%d)\n",
            Local_Origin.idOrigin, Local_iRetCode);  
    }
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {

		switch(Local_iRetCode)

		{
		case -2:
      logit("","PostGetOriginExec(): SQL Proc Get_Origin(%d) reports Origin not found in DB: %d.\n",
            Local_Origin.idOrigin, Local_iRetCode);  
			break;

		default:
      logit("","PostGetOriginExec(): SQL Proc Get_Origin(%d) returned ERROR (%d)\n",
            Local_Origin.idOrigin, Local_iRetCode);  
			break;
	  }
    return(EWDB_RETURN_WARNING);
  }
}  /* end PostXXX() */




