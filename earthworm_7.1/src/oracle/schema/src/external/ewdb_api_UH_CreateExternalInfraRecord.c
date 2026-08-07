
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UH_CreateExternalInfraRecord.c,v 1.3 2005/06/06 16:12:37 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UH_CreateExternalInfraRecord.c,v $
 *     Revision 1.3  2005/06/06 16:12:37  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2002/03/22 20:48:38  lucky
 *     Pre v6.1 checkin
 *
 *     Revision 1.1  2001/09/26 21:47:39  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin Create_UhInfo(OUT_idUHInfo => :OUT_idUHInfo,"
  " IN_idChanT => :IN_idChanT, IN_dNaturalFrequency => :IN_dNaturalFrequency, "
  " IN_dDamping=>:IN_dDamping, "
  " IN_dFullscale=>:IN_dFullscale, "
  " IN_dSensitivity=>:IN_dSensitivity, "
  " IN_dAzm=>:IN_dAzm, "
  " IN_dDip=>:IN_dDip, "
  " IN_iGain => :IN_iGain, "
  " IN_iSensorType=>:IN_iSensorType); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idUHInfo"},
  {0,1,0,0,0,OA_INT,":IN_idChanT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dNaturalFrequency"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dDamping"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dFullscale"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dSensitivity"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dAzm"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dDip"},
  {0,1,0,0,0,OA_INT,":IN_iGain"},
  {0,1,0,0,0,OA_INT,":IN_iSensorType"},
};

#define  NUM_FIELDS 10

static EWDB_External_UH_InfraInfo UHInfo;

static char Local_szdNatFreq[20],Local_szdDamping[20], Local_szdAzm[20], 
            Local_szdDip[20], Local_szdFS[20], Local_szdSensitivity[20];


/* Statement Struct for CreateUhInfrast statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateUhInfrastExec(EWDB_External_UH_InfraInfo * IN_pUHInfo,
                            EWDB_Cursor * ppCursor) ;
static int PostCreateUhInfrastExec(EWDB_External_UH_InfraInfo *pUHInfo);
static int InitCreateUhInfrastStatement(char *statement, 
                                 EWDB_OCIStatementStruct *pSS);

int ewdb_api_UH_CreateExternalInfraRecord (EWDB_External_UH_InfraInfo *pUHInfo)
{

  EWDB_Cursor pCursor;

  if (pUHInfo== NULL)
  {
    logit ("", "ewdb_api_UH_CreateExternalInfraRecord(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_UH_CreateExternalInfraRecord(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateUhInfrastExec (pUHInfo, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_UH_CreateExternalInfraRecord(): PrepCreateUhInfrastExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,
				"ewdb_api_UH_CreateExternalInfraRecord(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,
				"ewdb_api_UH_CreateExternalInfraRecord():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateUhInfrastExec (pUHInfo) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_UH_CreateExternalInfraRecord(): PostCreateUhInfrastExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UH_CreateExternalInfraRecord() */


static int InitCreateUhInfrastStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &(UHInfo.idUHInfo);
  pSS->FieldArray[1].pVal = &(UHInfo.idChanT);
  pSS->FieldArray[2].pVal = Local_szdNatFreq;
  pSS->FieldArray[3].pVal = Local_szdDamping;
  pSS->FieldArray[4].pVal = Local_szdFS;
  pSS->FieldArray[5].pVal = Local_szdSensitivity;
  pSS->FieldArray[6].pVal = Local_szdAzm;
  pSS->FieldArray[7].pVal = Local_szdDip;
  pSS->FieldArray[8].pVal = &(UHInfo.iGain);
  pSS->FieldArray[9].pVal = &(UHInfo.iSensorType);

  return(ewdb_base_RequestCursor (statement, pSS, 0));
}  /* End InitCreateUhInfrastStatement() */


static int PrepCreateUhInfrastExec(EWDB_External_UH_InfraInfo *IN_pUHInfo,
                            EWDB_Cursor * ppCursor) 
{
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  memcpy (&UHInfo, IN_pUHInfo, sizeof(EWDB_External_UH_InfraInfo));

	sprintf (Local_szdNatFreq, "%0.2f", UHInfo.dNaturalFrequency);
	sprintf (Local_szdDamping, "%0.2f", UHInfo.dDamping);
	sprintf (Local_szdFS, "%0.2f", UHInfo.dFullscale);
	sprintf (Local_szdSensitivity, "%0.6f", UHInfo.dSensitivity);
	sprintf (Local_szdAzm, "%0.2f", UHInfo.dAzm);
	sprintf (Local_szdDip, "%0.2f", UHInfo.dDip);

  if (InitCreateUhInfrastStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateUhInfrastExec(): InitCreateUhInfrastStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateUhInfrastExec() */


static int PostCreateUhInfrastExec (EWDB_External_UH_InfraInfo *pUHInfo)
{

	EWDB_Cursor pCursor;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if (pUHInfo == NULL)
	{
		logit ("", "PostCreateUhInfrastExec(): Invalid arguments passed in.\n");
		        return EWDB_RETURN_FAILURE;
  }

	pUHInfo->idUHInfo = UHInfo.idUHInfo;

	return(EWDB_RETURN_SUCCESS);

}  /* End PostCreateUhInfrastExec() */


