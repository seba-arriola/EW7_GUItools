/*                                                          *
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE *
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.             *
 *                                                          *
 *                                                          *
 *    Revision history:                                     *
 *                                                          *
 *     $Log: ewdb_delete_peakamp.sql,v $
 *     Revision 1.3  2004/09/07 19:31:14  davidk
 *     Reaper v2 09/02/2004.
 *                   *
 *                                                          *
 *                                                          *
 ************************************************************/


CREATE OR REPLACE FUNCTION Delete_PeakAmp
(IN_idPeakAmp number
) RETURN number
as

Temp number;

begin
  delete PeakAmp where idPeakAmp = IN_idPeakAmp;

  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test values('Delete_PeakAmp',IN_idPeakAmp,Temp);
      return(-1);
    end if;
END;
