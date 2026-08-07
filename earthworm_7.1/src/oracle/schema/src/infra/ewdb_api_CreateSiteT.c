/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateSiteT.c,v 1.4 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateSiteT.c,v $
 *     Revision 1.4  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/12/03 01:12:49  davidk
 *     Fixed bug in variable declarations.  Variables that were supposed to
 *     be static(file scope) were not.  Made 'em so they were.
 *
 *     Revision 1.2  2003/12/03 00:44:55  davidk
 *     Fixed a C++ style comment //
 *
 *     Revision 1.1  2003/12/03 00:19:45  davidk
 *     Initial revision
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] = 
     "  Begin Create_SiteT(OUT_idSiteT => :OUT_idSiteT, "
           "               IN_idSite => :IN_idSite, "
           "               IN_tOn => :IN_tOn, "
           "               IN_tOff => :IN_tOff, "
           "               IN_dLat => :IN_dLat, "
           "               IN_dLon => :IN_dLon, "
           "               IN_dElev => :IN_dElev, "
           "               IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idSiteT"},
  {0,1,0,0,0,OA_INT,":IN_idSite"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLat"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLon"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dElev"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 8

/* static variables */
static char Local_sztOn[20];
static char Local_sztOff[20];
static char Local_szdLat[20];
static char Local_szdLon[20];
static char Local_szdElev[20];

static char szComment[] = "";
static EWDB_ChannelStruct Local_ChanStruct;

/* Statement Struct for CreateSiteT szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateSiteTExec(EWDB_ChannelStruct * IN_pChan,
                           EWDB_Cursor * ppCursor);
static int PostCreateSiteTExec(EWDBid * pid);
static int InitCreateSiteTStatement(char *szStatement, 
                                 EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_CreateSiteT
*********************************************************************/
int ewdb_api_CreateSiteT(EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_CreateSiteT():Null pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_CreateSiteT(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCreateSiteTExec(pChan, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_CreateSiteT():PrepCreateSiteTExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateSiteT():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateSiteT():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostCreateSiteTExec(&pChan->idSiteT);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateSiteT(): "
           "ERROR:  PostCreateSiteTExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateSiteT() */


static int InitCreateSiteTStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_ChanStruct.idSiteT;
  pSS->FieldArray[1].pVal = &Local_ChanStruct.idSite;
  pSS->FieldArray[2].pVal = Local_sztOn;
  pSS->FieldArray[3].pVal = Local_sztOff;
  pSS->FieldArray[4].pVal = Local_szdLat;
  pSS->FieldArray[5].pVal = Local_szdLon;
  pSS->FieldArray[6].pVal = Local_szdElev;
  pSS->FieldArray[7].pVal = szComment;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitCreateSiteTStatement() */


int PrepCreateSiteTExec(EWDB_ChannelStruct * IN_pChan,
                        EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy user's vars to local vars here */
  memcpy(&Local_ChanStruct, IN_pChan, sizeof(EWDB_ChannelStruct));

  sprintf(Local_sztOn,"%.2f",Local_ChanStruct.tOn);
  sprintf(Local_sztOff,"%.2f",Local_ChanStruct.tOff);
  sprintf(Local_szdLat,"%.4f",Local_ChanStruct.Comp.Lat);
  sprintf(Local_szdLon,"%.4f",Local_ChanStruct.Comp.Lon);
  sprintf(Local_szdElev,"%.2f",Local_ChanStruct.Comp.Elev);


  if(InitCreateSiteTStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepCreateSiteTExec(): InitCreateSiteTStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepCreateSiteTExec() */


int PostCreateSiteTExec(EWDBid * pid)
{
  EWDB_Cursor pCursor;
  
  *pid=Local_ChanStruct.idSiteT;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_ChanStruct.idSiteT <= 0)
  {
    logit("", "ewdb_api_CreateSiteT(): "
           "ERROR:  PostCreateSiteTExec() reports SQL Proc %s returned error(%d)!\n",
          "Create_SiteT()", Local_ChanStruct.idSiteT);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostCreateSiteTExec() */


