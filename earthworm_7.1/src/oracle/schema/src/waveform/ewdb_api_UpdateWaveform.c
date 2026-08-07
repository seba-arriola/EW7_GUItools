/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateWaveform.c,v 1.1 2002/05/16 17:06:28 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateWaveform.c,v $
 *     Revision 1.1  2002/05/16 17:06:28  davidk
 *     Initial revision
 *
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

/*  ewdb_api_UpdateWaveform() and it's underlying calls are based 
    upon the ewdb_api_CreateWaveform() family of calls.
 *******************************************************************/

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <internal/ewdb_internal_functions.h>
/********************************************************************
********************************************************************/
int ewdb_api_UpdateWaveform(EWDB_WaveformStruct * pWaveform)
{

  int RetCode;


  ewdb_base_SetLastOraAPIActionTime();

  if((RetCode=ewdb_internal_UpdateWaveformDesc(pWaveform)) 
     == EWDB_RETURN_FAILURE)
  {
		logit ("", "Call to ewdb_internal_UpdateWaveformDesc() failed!\n");
    goto Abort;
  }
  
  if((RetCode=ewdb_internal_UpdateSnippet(pWaveform->idWaveform,
                                          pWaveform->binSnippet,
                                          pWaveform->iByteLen)) 
     == EWDB_RETURN_FAILURE)
  {
		logit ("", "Call to ewdb_internal_UpdateSnippet() failed!\n");
    goto Abort;
  }


  /* Commit the transaction (all the previous inserts!)
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_UpdateWaveform:ewdb_base_SQLCommit",3);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);

Abort:
  /* rollback the transaction (all the previous inserts!)
  ****************************************************/
  if (ewdb_base_SQLRollback(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_UpdateWaveform:EWDB_SQLRollback",3);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  return(RetCode);

}  /* end ewdb_api_UpdateWaveform() */

