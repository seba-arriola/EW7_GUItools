/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_BindToEvent.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_BindToEvent.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2002/06/18 16:10:35  lucky
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_Bind (OUT_idBind => :OUT_idBind,"
  "IN_idEvent => :IN_idEvent,"
  "IN_sCoreTableName => :IN_sCoreTableName,"
  "IN_idCore => :IN_idCore); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idBind"},
  {0,1,0,0,0,OA_INT,":IN_idEvent"},
  {0,1,0,0,0,OA_SZ, ":IN_sCoreTableName"},
  {0,1,0,0,0,OA_INT,":IN_idCore"},
};

#define	NUM_FIELDS	4

static EWDB_OCIStatementStruct SSStatement;

static	EWDBid	Local_idBind;
static	EWDBid	Local_idEvent;
static	EWDBid	Local_idCore;
static	char	  Local_szCoreTableName[256];


/*******************************
   FUNCTION PROTOTYPES
*******************************/
static int PrepBindToEventExec(EWDBid IN_idEvent, int IN_tiCoreTable, 
                               EWDBid IN_idCore,EWDB_Cursor *ppCursor);
static int PostBindToEventExec(EWDBid * pidBind);
static int InitBindToEventStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);
/******************************/


int ewdb_api_BindToEvent(EWDBid IN_idEvent, int IN_tiCoreTable, EWDBid IN_idCore, 
                         EWDBid *pidBind) 
{

	EWDB_Cursor pCursor;
	int rc;

	if ((IN_idEvent < 0) || (IN_idCore < 0) || (IN_tiCoreTable < 0) || !pidBind) 
  {
  	logit("", "ewdb_api_BindToEvent(): Invalid parameters passed in(%d,%d,%d,%u!\n",
          IN_idEvent, IN_idCore, IN_tiCoreTable, pidBind);
  	return EWDB_RETURN_FAILURE;
  }

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_BindToEvent(): Could not reconnect to the database!\n");
  	return (EWDB_RETURN_FAILURE);
  }


	if (PrepBindToEventExec (IN_idEvent, IN_tiCoreTable, IN_idCore, &pCursor) != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_BindToEvent():PrepBindToEventExec() failed.\n");
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (ewdb_base_SQLExecute (pCursor))
  {
  	ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_BindToEvent(): ewdb_base_SQLExecute", 1);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
  {
  	ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_BindToEvent(): ewdb_base_SQLCommit",2);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	rc = PostBindToEventExec(pidBind);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_FAILURE)
    {
    	logit("", "ewdb_api_BindToEvent(): PostBindToEventExec failed!\n");
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    else
      return(EWDB_RETURN_FAILURE);
  }
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_BindToEvent() */


static int InitBindToEventStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	SSStatement.FieldArray[0].pVal = &(Local_idBind);
	SSStatement.FieldArray[1].pVal = &(Local_idEvent);
	SSStatement.FieldArray[2].pVal = Local_szCoreTableName;
	SSStatement.FieldArray[3].pVal = &(Local_idCore);

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitCreateCodaDurStatement() */


static int PrepBindToEventExec(EWDBid IN_idEvent, int IN_tiCoreTable, 
                               EWDBid IN_idCore,EWDB_Cursor *ppCursor)
{

	Local_idEvent = IN_idEvent;
	Local_idCore  = IN_idCore;
  
	if (IN_tiCoreTable == CORE_TABLE_ORIGIN)
  	strcpy (Local_szCoreTableName, "Origin");
	else if (IN_tiCoreTable == CORE_TABLE_MAGNITUDE)
  	strcpy (Local_szCoreTableName, "Magnitude");
	else if (IN_tiCoreTable == CORE_TABLE_MECHFM)
  	strcpy (Local_szCoreTableName, "MechFM");
	else if (IN_tiCoreTable == CORE_TABLE_WAVEFORMDESC)
  	strcpy (Local_szCoreTableName, "WaveformDesc");
	else if (IN_tiCoreTable == CORE_TABLE_COINCIDENCE)
  	strcpy (Local_szCoreTableName, "CoincidenceEvent");
	else
  {
  	logit ("", "PrepBindToEventExec(): Invalid IN_tiCoreTable designation <%d>\n", IN_tiCoreTable);
  	return (EWDB_RETURN_FAILURE);
  }

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  if(InitBindToEventStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
	return EWDB_RETURN_SUCCESS;
}  /* end PrepBindToEventExec() */


int PostBindToEventExec(EWDBid * pidBind)
{
	EWDB_Cursor pCursor;
  
	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);

  *pidBind = Local_idBind;
  if(Local_idBind <= 0)
  {
    logit("", "PostBindToEventExec(): SQL Proc Create_Bind() failed with error %d\n",
          Local_idBind);
    return(EWDB_RETURN_WARNING);
  }

	return (EWDB_RETURN_SUCCESS);
}  /* end PostBindToEventExec() */
