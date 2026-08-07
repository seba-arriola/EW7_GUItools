/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateOrigin.c,v 1.10 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateOrigin.c,v $
 *     Revision 1.10  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.9  2005/05/05 18:54:21  davidk
 *     Fixed a bug in function where OriginStruct.sSource was being used instead
 *     of OriginStruct.sRealSource for the author string for the origin.
 *
 *     Revision 1.8  2003/05/22 19:49:24  lucky
 *     Removed dLatStart, dLonStart, dDepthStart
 *
 *     Revision 1.7  2003/01/30 23:13:30  lucky
 *     *** empty log message ***
 *
 *     Revision 1.6  2003/01/16 18:36:32  lucky
 *     Added starting location fields (dLatStart, dLonStart, dDepthStart)
 *
 *     Revision 1.5  2002/08/14 14:56:53  davidk
 *     Updated to handle Origin Version Numbers
 *
 *     Revision 1.4  2002/07/16 19:22:43  davidk
 *     Modified the sprintf statements that convert double/float input
 *     values to strings before they are passed to oracle.
 *     Modified the sprintf statements to add formatting to the
 *     float/double values, in order to truncate them to "2 to 4"
 *     decimal places, instead of the default (no truncation).
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     cleaned up the function as part of the API cleanup.  Added improved
 *     error loggin, handling, and reporting.
 *
 *     Revision 1.2  2001/05/15 02:16:19  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.8  2001/02/21 08:39:22  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.7  2000/06/21 22:51:35  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.6  2000/05/12 22:43:18  davidk
 *     Changed code to match the style of other API functions.  An API
 *     function calls a PrepXXX() and a PostXXX() function that copy
 *     variables in between the caller's struct and a local struct.
 *     The API function itself has no knowledge of the local struct.
 *     The Init_XXX() function only receives two parameters: a szStatement
 *     to be parsed, and a EWDB_OCIStatementStruct to hold binding
 *     information.  All binding is done to local variables, not user
 *     variable, so that a rebind does not need to occur for each new
 *     call.  EWDB_OCIStatementStruct.UseField assignments are set as
 *     constant in the struct declaration unless the SQL string is
 *     variable, meaning that the number of sql variables can change
 *     (example: when selecting an event list, sometimes you might use
 *     depth criteria and sometimes not).
 *
 *     Revision 1.5  2000/05/12 22:13:17  davidk
 *     Added prototypes for the functions implemented in this file.
 *     Fixed an error in the call to PostCreateOriginExec from EWDB_Create_Origin,
 *     where an OriginStruct was being passed instead of a pointer to an
 *     OriginStruct.
 *
 *     Revision 1.4  1999/11/09 18:21:08  lucky
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_Origin(OUT_idOrigin => :OUT_idOrigin,"
  "IN_tOrigin => :IN_tOrigin,"
  "IN_dLat => :IN_dLat,"
  "IN_dErLat => :IN_dErLat,"
  "IN_dLon => :IN_dLon,"
  "IN_dErLon => :IN_dErLon,"
  "IN_dDepth => :IN_dDepth,"
  "IN_dErZ => :IN_dErZ,"
  "IN_bBindToEvent => :IN_bBindToEvent,"
  "IN_idEvent => :IN_idEvent,"
  "IN_bSetPrefer => :IN_bSetPrefer,"
  "IN_sExternal_Table_Name => :IN_sExternal_Table_Name,"
  "IN_xidExternal => :IN_xidExternal,"
  "IN_sSource => :IN_sSource,"
  "IN_iGap => :IN_iGap,"
  "IN_dDmin => :IN_dDmin,"
  "IN_dRms => :IN_dRms,"
  "IN_iAssocRd => :IN_iAssocRd,"
  "IN_iAssocPh => :IN_iAssocPh,"
  "IN_iUsedRd => :IN_iUsedRd,"
  "IN_iUsedPh => :IN_iUsedPh,"
  "IN_iE0Azm => :IN_iE0Azm,"
  "IN_iE0Dip => :IN_iE0Dip,"
  "IN_iE1Azm => :IN_iE1Azm,"
  "IN_iE1Dip => :IN_iE1Dip,"
  "IN_iE2Azm => :IN_iE2Azm,"
  "IN_iE2Dip => :IN_iE2Dip,"
  "IN_dE0 => :IN_dE0,"
  "IN_dE1 => :IN_dE1,"
  "IN_dE2 => :IN_dE2,"
  "IN_sComment => :IN_sComment,"
  "IN_tMCI => :IN_tMCI,"
  "IN_iFixedDepth => :IN_iFixedDepth,"
  "IN_iVersionNum => :IN_iVersionNum,"
  "IN_sSourceEventID => :IN_sSourceEventID"
  "); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{

  {0,1,0,0,0,OA_INT,":OUT_idOrigin"},
  {0,1,0,0,0,OA_FLOAT,":IN_tOrigin"},
  {0,1,0,0,0,OA_FLOAT,":IN_dLat"},
  {0,1,0,0,0,OA_FLOAT,":IN_dErLat"},
  {0,1,0,0,0,OA_FLOAT,":IN_dLon"},
  {0,1,0,0,0,OA_FLOAT,":IN_dErLon"},
  {0,1,0,0,0,OA_FLOAT,":IN_dDepth"},
  {0,1,0,0,0,OA_FLOAT,":IN_dErZ"},
  {0,1,0,0,0,OA_INT,":IN_bBindToEvent"},
  {0,1,0,0,0,OA_INT,":IN_idEvent"},
  {0,1,0,0,0,OA_INT,":IN_bSetPrefer"},
  {0,1,0,0,0,OA_SZ,":IN_sExternal_Table_Name"},
  {0,1,0,0,0,OA_SZ,":IN_xidExternal"},
  {0,1,0,0,0,OA_SZ,":IN_sSource"},
  {0,1,0,0,0,OA_INT,":IN_iGap"},
  {0,1,0,0,0,OA_FLOAT,":IN_dDmin"},
  {0,1,0,0,0,OA_FLOAT,":IN_dRms"},
  {0,1,0,0,0,OA_INT,":IN_iAssocRd"},
  {0,1,0,0,0,OA_INT,":IN_iAssocPh"},
  {0,1,0,0,0,OA_INT,":IN_iUsedRd"},
  {0,1,0,0,0,OA_INT,":IN_iUsedPh"},
  {0,1,0,0,0,OA_INT,":IN_iE0Azm"},
  {0,1,0,0,0,OA_INT,":IN_iE0Dip"},
  {0,1,0,0,0,OA_INT,":IN_iE1Azm"},
  {0,1,0,0,0,OA_INT,":IN_iE1Dip"},
  {0,1,0,0,0,OA_INT,":IN_iE2Azm"},
  {0,1,0,0,0,OA_INT,":IN_iE2Dip"},
  {0,1,0,0,0,OA_FLOAT,":IN_dE0"},
  {0,1,0,0,0,OA_FLOAT,":IN_dE1"},
  {0,1,0,0,0,OA_FLOAT,":IN_dE2"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"},
  {0,1,0,0,0,OA_FLOAT,":IN_tMCI"},
  {0,1,0,0,0,OA_INT,":IN_iFixedDepth"},
  {0,1,0,0,0,OA_INT,":IN_iVersionNum"},
  {0,1,0,0,0,OA_SZ,":IN_sSourceEventID"}
};

#define  NUM_FIELDS  35

static char  Local_szdLat[20], Local_szdLatErr[20], Local_szdLon[20], Local_szdLonErr[20],
             Local_szdDepth[20], Local_szdDepthErr[20], Local_szdDMin[20], Local_szdRMS[20],
             Local_szdE0[20], Local_szdE1[20], Local_szdE2[20], Local_sztOrigin[20], Local_szdMCI[20];



/* Insertion Struct for CreateOrigin szStatement */
static EWDB_OCIStatementStruct SSStatement;

static  EWDB_OriginStruct  Local_OriginStruct;


/*******************************
   FUNCTION PROTOTYPES
*******************************/
static int PrepCreateOriginExec(EWDB_OriginStruct *pOrigin, EWDB_Cursor *ppCursor);
static int PostCreateOriginExec(EWDB_OriginStruct *pOrigin);
static int InitCreateOriginStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);
/******************************/


int ewdb_api_CreateOrigin(EWDB_OriginStruct *pOrigin)
{

  EWDB_Cursor pCursor;
  int rc;

  if (pOrigin == NULL)
  {
    logit ("", "ewdb_api_CreateOrigin(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit ("", "ewdb_api_CreateOrigin(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateOriginExec (pOrigin, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateOrigin(): PrepCreateOriginExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateOrigin(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreateOrigin(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostCreateOriginExec(pOrigin);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateOrigin(): PostCreateOriginExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateOrigin() */


static int InitCreateOriginStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &(Local_OriginStruct.idOrigin);
  pSS->FieldArray[1].pVal = Local_sztOrigin;
  pSS->FieldArray[2].pVal = Local_szdLat;
  pSS->FieldArray[3].pVal = Local_szdLatErr;
  pSS->FieldArray[4].pVal = Local_szdLon;
  pSS->FieldArray[5].pVal = Local_szdLonErr;
  pSS->FieldArray[6].pVal = Local_szdDepth;
  pSS->FieldArray[7].pVal = Local_szdDepthErr;
  pSS->FieldArray[8].pVal = &(Local_OriginStruct.BindToEvent);
  pSS->FieldArray[9].pVal = &(Local_OriginStruct.idEvent);
  pSS->FieldArray[10].pVal = &(Local_OriginStruct.SetPreferred);
  pSS->FieldArray[11].pVal = Local_OriginStruct.ExternalTableName;
  pSS->FieldArray[12].pVal = Local_OriginStruct.xidExternal;
  pSS->FieldArray[13].pVal = Local_OriginStruct.sRealSource;
  pSS->FieldArray[14].pVal = &(Local_OriginStruct.iGap);
  pSS->FieldArray[15].pVal = Local_szdDMin;
  pSS->FieldArray[16].pVal = Local_szdRMS;
  pSS->FieldArray[17].pVal = &(Local_OriginStruct.iAssocRd);
  pSS->FieldArray[18].pVal = &(Local_OriginStruct.iAssocPh);
  pSS->FieldArray[19].pVal = &(Local_OriginStruct.iUsedRd);
  pSS->FieldArray[20].pVal = &(Local_OriginStruct.iUsedPh);
  pSS->FieldArray[21].pVal = &(Local_OriginStruct.iE0Azm);
  pSS->FieldArray[22].pVal = &(Local_OriginStruct.iE0Dip);
  pSS->FieldArray[23].pVal = &(Local_OriginStruct.iE1Azm);
  pSS->FieldArray[24].pVal = &(Local_OriginStruct.iE1Dip);
  pSS->FieldArray[25].pVal = &(Local_OriginStruct.iE2Azm);
  pSS->FieldArray[26].pVal = &(Local_OriginStruct.iE2Dip);
  pSS->FieldArray[27].pVal = Local_szdE0;
  pSS->FieldArray[28].pVal = Local_szdE1;
  pSS->FieldArray[29].pVal = Local_szdE2;
  pSS->FieldArray[30].pVal = Local_OriginStruct.Comment;
  pSS->FieldArray[31].pVal = Local_szdMCI;
  pSS->FieldArray[32].pVal = &(Local_OriginStruct.iFixedDepth);
  pSS->FieldArray[33].pVal = &(Local_OriginStruct.iVersionNum);
  pSS->FieldArray[34].pVal = Local_OriginStruct.szSourceEventID;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitCreateOriginStatement() */


static int PrepCreateOriginExec (EWDB_OriginStruct *pOrigin, EWDB_Cursor *ppCursor)
{
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy the incomming struct into the local struct */
  memcpy (&Local_OriginStruct, pOrigin, sizeof (EWDB_OriginStruct));

  sprintf (Local_sztOrigin, "%0.2f", pOrigin->tOrigin);
  sprintf (Local_szdMCI, "%0.2f", pOrigin->dMCI);
  sprintf (Local_szdLon, "%3f", pOrigin->dLon);
  sprintf (Local_szdLat, "%3f", pOrigin->dLat);
  sprintf (Local_szdLonErr, "%3f", pOrigin->dErrLon);
  sprintf (Local_szdLatErr, "%3f", pOrigin->dErrLat);
  sprintf (Local_szdDepth, "%3f", pOrigin->dDepth);
  sprintf (Local_szdDepthErr, "%3f", pOrigin->dErZ);
  sprintf (Local_szdDMin, "%2f", pOrigin->dDmin);
  sprintf (Local_szdRMS, "%4f", pOrigin->dRms);
  sprintf (Local_szdE0, "%2f", pOrigin->dE0);
  sprintf (Local_szdE1, "%2f", pOrigin->dE1);
  sprintf (Local_szdE2, "%2f", pOrigin->dE2);

  if (InitCreateOriginStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateOriginExec(): InitCreateOriginStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
} /* End PrepCreateOriginExec() */


static int PostCreateOriginExec (EWDB_OriginStruct *pOrigin)
{
  EWDB_Cursor pCursor;
  
  pOrigin->idOrigin=Local_OriginStruct.idOrigin;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(pOrigin->idOrigin <= 0)
  {
    if(pOrigin->idOrigin == -1)
      logit("","PostCreateOriginExec():  SQL Proc Create_Origin() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else if(pOrigin->idOrigin == -14)
      logit("","PostCreateOriginExec():  SQL Proc Create_Origin() returned "
                "\"Bad idEvent\".  If bBindToEvent is set to a non-zero value "
                "then idEvent must be a valid existing idEvent(%d) from the DB.\n",
            pOrigin->idEvent);
    else if(pOrigin->idOrigin <= -200)
      logit("","PostCreateOriginExec():  SQL Proc Create_Origin() returned "
                "\"Unable to Make Origin Preferred\".  For some reason "
                "the function was unable to make the idOrigin, the "
                "preferred Origin for the idEvent(%d).\n",
            pOrigin->idOrigin, pOrigin->idEvent);
    else
      logit("","PostCreateOriginExec():  SQL Proc Create_Origin() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pOrigin->idOrigin);
    return(EWDB_RETURN_WARNING);
  }
  
  return (EWDB_RETURN_SUCCESS);
} /* End PostCreateOriginExec() */
