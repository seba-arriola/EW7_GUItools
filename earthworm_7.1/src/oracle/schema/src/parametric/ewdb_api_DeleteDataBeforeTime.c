/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteDataBeforeTime.c,v 1.5 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteDataBeforeTime.c,v $
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2004/09/09 19:23:51  davidk
 *     Fixed call to ewdb_api_DeleteSMMessagesBeforeTime(), which doesn't follow
 *     the new deletion call convention. (follows the old one).
 *
 *     Revision 1.3  2004/09/09 17:22:27  davidk
 *     Re-Updated for reaper v2.
 *     Now includes a pSQLRetCode, for more precise result description.
 *
 *     Revision 1.2  2004/09/09 05:49:45  davidk
 *     Updated for reaper v2.
 *     Now includes a iMaxRecsToDelete
 *     (so the caller can delete piecemeal for faster execution).
 *     Automatically forces.
 *
 *     Revision 1.1  2001/07/14 07:40:13  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


int ewdb_api_DeleteDataBeforeTime(int IN_tTime, int IN_iDatatypes, 
                                  int IN_iMaxRecsToDelete, int * pSQLRetCode)
{

  int iRetCode=0;
  int iSQLRetCode=0;
  int iTemp;

  if(IN_iDatatypes ==  EWDB_UNASSOCIATED_DATA_NONE)
  {
    return(EWDB_RETURN_WARNING);
    /* skip unassociated data, warn the caller that their was
       nothing to delete  */
  }
  else if(IN_iDatatypes & EWDB_UNASSOCIATED_DATA_ALL)
  {
    iRetCode = ewdb_api_DeleteAllDataBeforeTime(IN_tTime, IN_iMaxRecsToDelete, &iSQLRetCode);
  }
  else
  {
    if(IN_iDatatypes &  EWDB_UNASSOCIATED_DATA_PICKS)
    {
      iRetCode |= ewdb_api_DeletePicksBeforeTime(IN_tTime, IN_iMaxRecsToDelete, &iTemp);
      iSQLRetCode |= iTemp;
    }

    if(IN_iDatatypes &  EWDB_UNASSOCIATED_DATA_WAVEFORMS)
    {
      iRetCode |= ewdb_api_DeleteWaveformsBeforeTime(IN_tTime, IN_iMaxRecsToDelete, &iTemp);
      iSQLRetCode |= iTemp;
    }

    if(IN_iDatatypes &  EWDB_UNASSOCIATED_DATA_PEAKAMPS)
    {
      iRetCode |= ewdb_api_DeletePeakAmpsBeforeTime(IN_tTime, IN_iMaxRecsToDelete, &iTemp);
      iSQLRetCode |= iTemp;
    }

    if(IN_iDatatypes &  EWDB_UNASSOCIATED_DATA_SMMESSAGES)
    {
      iRetCode |= ewdb_api_DeleteSMMessagesBeforeTime(IN_tTime, TRUE/*bForce*/);
      iSQLRetCode |= iTemp;
    }
  }

  /* We bitwise OR the return codes together, and hope that
     error = 0xffffffff, success = 0x00000000, and warning = 0x00000001,
     that way you receive the worst of all the return codes you received.
     Call it a pipe dream....
  ***********************************************************************/
  *pSQLRetCode = iSQLRetCode;
  return(iRetCode);


}  /* end ewdb_api_DeleteDataBeforeTime() */


