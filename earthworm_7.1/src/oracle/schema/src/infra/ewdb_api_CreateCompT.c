/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateCompT.c,v 1.4 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateCompT.c,v $
 *     Revision 1.4  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/12/04 19:33:34  davidk
 *     Improved handling of SQL warnings and errors, passing more information
 *     back to the caller.
 *
 *     Revision 1.2  2003/12/03 00:44:55  davidk
 *     Fixed a C++ style comment //
 *
 *     Revision 1.1  2003/12/03 00:23:29  davidk
 *     Initial revision
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] = 
     "  Begin Create_CompT(OUT_RetCode => :OUT_RetCode, "
           "OUT_idCompT => :OUT_idCompT, "
           "IN_idComp => :IN_idComp, "
           "IN_tOn => :IN_tOn, "
           "IN_tOff => :IN_tOff, "
           "IN_dLat => :IN_dLat, "
           "IN_dLon => :IN_dLon, "
           "IN_dElev => :IN_dElev, "
           "IN_dAzm => :IN_dAzm, "
           "IN_dDip => :IN_dDip, "
           "IN_sComment => :IN_sComment, "
           "IN_bForce => :IN_bForce); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idCompT"},
  {0,1,0,0,0,OA_EWDBID,":IN_idComp"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLat"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLon"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dElev"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dAzm"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dDip"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"},
  {0,1,0,0,0,OA_INT,":IN_bForce"}
};

#define  NUM_FIELDS 12

/* static variables */
static char Local_sztOn[20];
static char Local_sztOff[20];
static char Local_szdLat[20];
static char Local_szdLon[20];
static char Local_szdElev[20];
static char Local_szdAzm[20];
static char Local_szdDip[20];

static char Local_szComment[]="";
static EWDB_ChannelStruct Local_ChanStruct;
static int  Local_iRetCode;

static int  Local_bForce = FALSE;

/* Statement Struct for CreateCompT szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateCompTExec(EWDB_ChannelStruct * IN_pChan,
                        EWDB_Cursor * ppCursor);
static int PostCreateCompTExec(EWDBid * pid);
static int InitCreateCompTStatement(char *szStatement, 
                                 EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_CreateCompT
*********************************************************************/
int ewdb_api_CreateCompT(EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_CreateCompT():Null pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_CreateCompT(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCreateCompTExec(pChan, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_CreateCompT():PrepCreateCompTExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateCompT():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateCompT():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostCreateCompTExec(&pChan->idCompT);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateCompT(): "
           "ERROR:  PostCreateCompTExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(rc);
}  /* end ewdb_api_CreateCompT() */


static int InitCreateCompTStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_ChanStruct.idCompT;
  pSS->FieldArray[2].pVal = &Local_ChanStruct.Comp.idComp;
  pSS->FieldArray[3].pVal = Local_sztOn;
  pSS->FieldArray[4].pVal = Local_sztOff;
  pSS->FieldArray[5].pVal = Local_szdLat;
  pSS->FieldArray[6].pVal = Local_szdLon;
  pSS->FieldArray[7].pVal = Local_szdElev;
  pSS->FieldArray[8].pVal = Local_szdAzm;
  pSS->FieldArray[9].pVal = Local_szdDip;
  pSS->FieldArray[10].pVal = Local_szComment;
  pSS->FieldArray[11].pVal = &Local_bForce;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitCreateCompTStatement() */


static int PrepCreateCompTExec(EWDB_ChannelStruct * IN_pChan,
                        EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy user's vars to local vars here */
  memcpy(&Local_ChanStruct, IN_pChan, sizeof(Local_ChanStruct));
  sprintf(Local_sztOn,"%.2f",IN_pChan->tOn);
  sprintf(Local_sztOff,"%.2f",IN_pChan->tOff);
  sprintf(Local_szdLat,"%.2f",IN_pChan->Comp.Lat);
  sprintf(Local_szdLon,"%.2f",IN_pChan->Comp.Lon);
  sprintf(Local_szdElev,"%.2f",IN_pChan->Comp.Elev);
  sprintf(Local_szdAzm,"%.2f",IN_pChan->Comp.Azm);
  sprintf(Local_szdDip,"%.2f",IN_pChan->Comp.Dip);

  if(InitCreateCompTStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepCreateCompTExec(): InitCreateCompTStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepCreateCompTExec() */


static int PostCreateCompTExec(EWDBid * pid)
{
  EWDB_Cursor pCursor;
  
  *pid=Local_ChanStruct.idCompT;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  if(Local_iRetCode != 0)
  {
    if(Local_iRetCode < 0)
      *pid = Local_iRetCode;  /* copy the SQL error over to *pid for return to caller */

    logit("t", "PostCreateCompTExec(): SQL Proc returned error(%d) while trying to create compt "
               " for idComp %d (%s - %s)\n",
           Local_iRetCode, Local_ChanStruct.Comp.idComp, Local_sztOn, Local_sztOff);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostCreateCompTExec() */


