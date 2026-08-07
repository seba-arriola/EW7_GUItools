/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_TS_Convert.c,v 1.4 2005/06/15 19:03:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_TS_Convert.c,v $
 *     Revision 1.4  2005/06/15 19:03:13  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2005/03/24 18:14:43  davidk
 *     Fixed bugs during initial testing of Mw API
 *
 *     Revision 1.2  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.1  2005/03/22 23:38:56  davidk
 *     Added API functions for Mw
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_internal_functions.h>

#define TS_SQLtoC_DELIMITERS " \n"

static char szTemp[EWDB_MAX_SQL_STRING_LEN+1];

int ewdb_internal_TS_CtoSQL(double * pTSList, int iNumDP, char * szSQLTSBuffer1, char * szSQLTSBuffer2 )
/* pTSList:       an Array of doubles representing the time series in C.
   iNumDP:        the number of data points in the time series.
   szSQLTSBuffer1:a character array atleast 4001 bytes long, where 
                  the function will write the null-terminated space delimited
                  string representing the time series in SQL.
   szSQLTSBuffer2:a character array atleast 2001 bytes long, where 
                  the function will write any remaining portion of the time series that would not fit in szSQLTSBuffer1.

   return codes:
     EWDB_RETURN_FAILURE:   Invalid input param.  (bad pointer)
     EWDB_RETURN_WARNING:   Array contained too many Datapoints.  It was truncated after 
                            (EWDB_MAX_SQL_STRING_LEN / EWDB_SQL_DOUBLE_LEN) = 285, or
                            array contained ZERO datapoints.
     EWDB_RETURN_SUCCESS:   Success
 ****************************************************************************/
{

  int i;
  int iTempDP;
  int rc = EWDB_RETURN_SUCCESS;
  char szTemp[EWDB_SQL_DOUBLE_LEN+1];


  if(!szSQLTSBuffer1 || !szSQLTSBuffer2 || !pTSList || (iNumDP < 0))
  {
    logit("et","ERROR! ewdb_internal_TS_SQLtoC(): invalid pointer or value passed in.(%u,%u,%u,%u)\n",
          szSQLTSBuffer1, szSQLTSBuffer2, pTSList, iNumDP);
    return(EWDB_RETURN_FAILURE);
  }

  szSQLTSBuffer2[0] = 0x00;

  if(iNumDP > (EWDB_MAX_SQL_STRING_LEN / EWDB_SQL_DOUBLE_LEN))
  {
    rc = EWDB_RETURN_WARNING;
    iTempDP = (int)(EWDB_MAX_SQL_STRING_LEN / EWDB_SQL_DOUBLE_LEN);
  }
  else 
  {
    if (iNumDP == 0)
      rc = EWDB_RETURN_WARNING;
    iTempDP = iNumDP;
  }

  szSQLTSBuffer1[0] = 0x00;

  for(i=0; i < iTempDP; i++)
  {
    sprintf(szTemp, "%e ", pTSList[i]);
    strcat(szSQLTSBuffer1, szTemp);  
  }

  if(strlen(szSQLTSBuffer1) > 1999)
  {
    strcpy(szSQLTSBuffer2, &szSQLTSBuffer1[1999]);
    szSQLTSBuffer1[1999] = 0x00;
  }
     

  return(rc);
}  /* end ewdb_internal_TS_CtoSQL() */


int ewdb_internal_TS_SQLtoC(char * szSQLTSBuffer1, char * szSQLTSBuffer2, double * pTSList, int* piNumDP)
/* 
   szSQLTSBuffer1: a null terminated character array representing the time series
                  in SQL.atleast 2001 bytes long, where 
                  the function will write the null-terminated space delimited
                  string representing the time series in SQL.
   szSQLTSBuffer2: a null terminated character array representing the time series
                  in SQL.atleast 2001 bytes long, where 
                  the function will write the remainder of the time series 
                  (that didn't fit in szSQLTSBuffer1) to this char array.
   pTSList:       an Array of atleast 285 doubles where 
                  the function will write the list representing the time 
                  series in C.
   piNumDP:       pointer to the number of data points in the time series, which
                  the function will fill in, indicating the number of doubles 
                  it wrote to pTSList

   return codes:
     EWDB_RETURN_FAILURE:   Invalid input param.  (bad pointer, or bad string)
     EWDB_RETURN_WARNING:   Parsing went OK, but no Datapoints retrieved from string.
     EWDB_RETURN_SUCCESS:   Success
 ****************************************************************************/
{

  int i;
  int rc = EWDB_RETURN_SUCCESS;

  char * szToken;

  if(!szSQLTSBuffer1 || !szSQLTSBuffer2 || !pTSList || !piNumDP)
  {
    logit("et","ERROR! ewdb_internal_TS_SQLtoC(): invalid pointer passed in.(%u,,%u%u,%u)\n",
          szSQLTSBuffer1, szSQLTSBuffer2, pTSList, piNumDP);
    return(EWDB_RETURN_FAILURE);
  }

  if( (strlen(szSQLTSBuffer1) + strlen(szSQLTSBuffer2)) > EWDB_MAX_SQL_STRING_LEN)
  {
    logit("et","ERROR! ewdb_internal_TS_SQLtoC(): questionable string passed in."
               "  Length = %d, but max should be %d.  RETURNING ERROR!\n",
          strlen(szSQLTSBuffer1) + strlen(szSQLTSBuffer2), EWDB_MAX_SQL_STRING_LEN);
    return(EWDB_RETURN_FAILURE);
  }

  sprintf(szTemp, "%s%s", szSQLTSBuffer1,szSQLTSBuffer2);
  szToken = strtok(szTemp, TS_SQLtoC_DELIMITERS);
  for(i=0; szToken; i++)
  {
    pTSList[i] = atof(szToken);
    szToken = strtok(NULL, TS_SQLtoC_DELIMITERS);
  }

  *piNumDP = i;

  if(i == 0)
    return(EWDB_RETURN_WARNING);
  else
    return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_TS_SQLtoC() */

