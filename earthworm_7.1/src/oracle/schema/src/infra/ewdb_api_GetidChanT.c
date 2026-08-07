/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetidChanT.c,v 1.7 2005/06/27 15:30:01 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetidChanT.c,v $
 *     Revision 1.7  2005/06/27 15:30:01  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.6  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2004/03/31 18:57:41  davidk
 *     Cleaned up format.  Added Local_idChan validation and debugging
 *     statements.
 *
 *     Revision 1.4  2003/11/11 17:48:41  davidk
 *     Switched the SQL conversion type of the tTime param from int
 *     to double.  I think this was causing bad vaules.
 *
 *     Revision 1.3  2001/07/23 17:09:11  davidk
 *     Added exception handler in SQL query, so that a return
 *     code is set, that can be checked by PostXXX().
 *
 *     Revision 1.2  2001/05/15 02:16:30  davidk
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

static char SQL_STRING[] =
  "Begin select idChanT into :OUT_idChant from ALL_ChanT_INFO "
  " where idChan = :IN_idChan "
  "   and tON  <= :IN_tTime "
  "   and tOff >= :IN_tTime; "
  "EXCEPTION "
  "  WHEN OTHERS THEN "
  "    :OUT_idChant := -1; "
  "End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idChanT"},
  {0,1,0,0,0,OA_INT,":IN_idChan"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tTime"}
};

#define  NUM_FIELDS 3

static EWDBid Local_idChanT, Local_idChan;
static char Local_szTime[20];

/* Statement Struct for GetidChanT szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetidChanTExec(EWDBid IN_idChan, double IN_tTime, EWDB_Cursor * ppCursor);
static int PostGetidChanTExec(EWDBid * pidChanT);
static int InitGetidChanTStatement(char *szStatement, 
                            EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetidChanT(EWDBid IN_idChan, double tTime, EWDBid * pidChanT)
{

  EWDB_Cursor pCursor;

  if (pidChanT== NULL)
  {
    logit ("", "ewdb_api_GetidChanT():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "EWDB_GetidChanT(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetidChanTExec (IN_idChan,tTime, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetidChanT(): PrepGetidChanTExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetidChanT(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetidChanT(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostGetidChanTExec (pidChanT) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetidChanT(): PostGetidChanTExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetidChanT() */


static int InitGetidChanTStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_idChanT;
  pSS->FieldArray[1].pVal = &Local_idChan;
  pSS->FieldArray[2].pVal = Local_szTime;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* End InitGetidChanTStatement() */


static int PrepGetidChanTExec(EWDBid IN_idChan, double IN_tTime, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  Local_idChan=IN_idChan;

  sprintf(Local_szTime,"%.4f",IN_tTime);

  if (InitGetidChanTStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitGetidChanTStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetidChanTExec() */


static int PostGetidChanTExec(EWDBid * pidChanT)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  *pidChanT = Local_idChanT;

  if(Local_idChanT <= 0)
  {
    logit("","%s():ERROR! %d returned by SQL Query for Local_idChan=%d, tTime=(%s)\n",
          "PostGetidChanTExec",Local_idChanT, Local_idChan, Local_szTime);
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostGetidChanTExec() */



