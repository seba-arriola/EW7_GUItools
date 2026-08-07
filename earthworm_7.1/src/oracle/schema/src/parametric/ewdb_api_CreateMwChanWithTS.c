/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateMwChanWithTS.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateMwChanWithTS.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2005/03/24 18:14:43  davidk
 *     Fixed bugs during initial testing of Mw API
 *
 *     Revision 1.1  2005/03/22 23:38:56  davidk
 *     Added API functions for Mw
 *
 *
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_internal_functions.h>

int ewdb_api_CreateMwChanWithTS(EWDB_MwChanStruct * pMwChan)
{
  int rc;

  /* first validate the channels are consistent. */
  if(!((pMwChan->idChan == pMwChan->SynthTS.idChan) && (pMwChan->idChan == pMwChan->RealTS.idChan)))
  {
      logit("et","ewdb_api_CreateMwChanWithTS(%d %d): ERROR: idChans not consistent in MwChan struct.\n"
                 "MwChan(%d), SynthTS(%d), RealTS(%d).  Aborting.\n",
            pMwChan->idMw, pMwChan->idChan, pMwChan->idChan, pMwChan->SynthTS.idChan,pMwChan->RealTS.idChan);
      return(EWDB_RETURN_FAILURE);
  }

  /* then check the synthetic time series.  Create it in the DB if it doesn't already have a valid ID */
  if(pMwChan->SynthTS.idMwCTS <= 0)
  {
    rc = ewdb_internal_CreateMwChanTS(&pMwChan->SynthTS);
    if(rc != EWDB_RETURN_SUCCESS)
    {
      logit("et"," ewdb_api_CreateMwChanWithTS(%d %d): ERROR(%d) in ewdb_internal_CreateMwChanTS(Synth). Aborting.\n",
            pMwChan->idMw, pMwChan->idChan, rc);
      return(EWDB_RETURN_FAILURE);
    }
  }

  /* then check the real time series.  Create it in the DB if it doesn't already have a valid ID */
  if(pMwChan->RealTS.idMwCTS <= 0)
  {
    rc = ewdb_internal_CreateMwChanTS(&pMwChan->RealTS);
    if(rc != EWDB_RETURN_SUCCESS)
    {
      logit("et"," ewdb_api_CreateMwChanWithTS(%d %d): ERROR(%d) in ewdb_internal_CreateMwChanTS(Real). Aborting.\n",
            pMwChan->idMw, pMwChan->idChan, rc);
      return(EWDB_RETURN_FAILURE);
    }
  }

  /* then once both time series exist in the DB, Create the MwChan record. */
  rc = ewdb_internal_CreateMwChan(pMwChan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    logit("et"," ewdb_api_CreateMwChanWithTS(%d %d): ERROR(%d) in ewdb_internal_CreateMwChan(). Aborting.\n",
      pMwChan->idMw, pMwChan->idChan, rc);
    return(EWDB_RETURN_FAILURE);
  }

  /* done */
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateMwChanWithTS() */

