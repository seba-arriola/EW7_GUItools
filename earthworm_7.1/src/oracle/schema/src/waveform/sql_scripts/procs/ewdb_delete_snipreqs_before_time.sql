/*                                                          *
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE *
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.             *
 *                                                          *
 *                                                          *
 *    Revision history:                                     *
 *                                                          *
 *     $Log: ewdb_delete_snipreqs_before_time.sql,v $
 *     Revision 1.1  2004/09/07 19:31:15  davidk
 *     Reaper v2 09/02/2004.
 *         *
 *     Revision 1.1  2001/07/14 07:48:58  davidk            *
 *     Initial revision                                     *
 *                                                          *
 *                                                          *
 ************************************************************/

CREATE OR REPLACE FUNCTION Delete_SnipReqs_Before_Time
(IN_tTime            number,
 IN_iMaxRecsToDelete number
) RETURN NUMBER

  /*  Delete_SnippetRequests_Before_Time Return codes:
           0 :   Success
          -1 :   Unknown error, see debug table for info
           1 :   Warning, some of the attempted deletions were not
                  possible due to ForeignKey issues
           2 :   More than iMaxRecsToDelete found, only
                  iMaxRecsToDelete deleted.
           3 :   See 1,2 above.
   ******************************************************************/
as
 
  Cursor SnippetRequest_cursor(Cursor_tTime number) is
    select idSnipReq, tEnd from SnippetRequest
      where tEnd < Cursor_tTime
      order by tEnd asc;

  Temp             number := 0;
  Temp_RetCode     number := 0;
  State            number;
  Temp_Count       number := 0;

begin
  State := 1;

  for Curr in SnippetRequest_cursor(IN_tTime) loop
    Temp_RetCode := Delete_SnippetRequest(Curr.idSnipReq);
    if Temp < 0 then
      return(Temp);
    elsif Temp > 0 then
      Temp_RetCode := 1;
    else
      Temp_Count := Temp_Count + 1;
    end if;

    if (Temp_Count = IN_iMaxRecsToDelete) THEN
     return(Temp_RetCode+2);
    end if;
  end loop;

  return(Temp_RetCode);

EXCEPTION
  WHEN OTHERS THEN
    insert into test values('Delete_SnippetRequests_Before_Time',IN_tTime,State);
	  return(-1);
END;

