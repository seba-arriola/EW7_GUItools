/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateWaveform.c,v 1.2 2001/07/23 17:23:42 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateWaveform.c,v $
 *     Revision 1.2  2001/07/23 17:23:42  davidk
 *     removed unneccessary #includes, and added neccessary ones.
 *
 *     Revision 1.1  2001/05/15 02:16:43  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <internal/ewdb_internal_functions.h>
/********************************************************************
********************************************************************/
int ewdb_api_CreateWaveform(EWDB_WaveformStruct * pWaveform, EWDBid idEvent)
{

  int RetCode;


  ewdb_base_SetLastOraAPIActionTime();

  if((RetCode=ewdb_internal_CreateWaveformDesc(pWaveform,TRUE,idEvent)) 
     == EWDB_RETURN_FAILURE)
  {
		logit ("", "Call to ewdb_internal_CreateWaveformDesc() failed!\n");
    goto Abort;
  }
  
  if((RetCode=ewdb_internal_CreateSnippet(pWaveform->idWaveform,
                                          pWaveform->binSnippet,
                                          pWaveform->iByteLen)) 
     == EWDB_RETURN_FAILURE)
  {
		logit ("", "Call to ewdb_internal_CreateSnippet() failed!\n");
    goto Abort;
  }

  /* CHANGED: DavidK 05/08/2000 
     Code was changed so that only one waveform insertion
     API call would be exposed(ewdb_api_CreateWaveform).  The 
     other two snippet-insertion related calls
     (ewdb_internal_CreateWaveformDesc, ewdb_internal_CreateSnippet) would
     be internal to the API.  This allows us to execute
     both the internal calls as part of a single transaction,
     by putting a single commit in ewdb_api_CreateWaveform.  This
     way if the ewdb_internal_CreateSnippet call fails we don't end
     up with a waveform descriptor in the DB that describes
     a non-existent waveform.  
  ***********************************************************/

  /* Commit the transaction (all the previous inserts!)
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateWaveform:ewdb_base_SQLCommit",3);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);

Abort:
  /* rollback the transaction (all the previous inserts!)
  ****************************************************/
  if (ewdb_base_SQLRollback(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateWaveform:EWDB_SQLRollback",3);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  return(RetCode);

}  /* end ewdb_api_CreateWaveform() */

