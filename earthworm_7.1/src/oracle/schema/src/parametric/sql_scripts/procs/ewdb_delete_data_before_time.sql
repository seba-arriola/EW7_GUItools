/*                                                          *
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE *
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.             *
 *                                                          *
 *                                                          *
 *    Revision history:                                     *
 *                                                          *
 *     $Log: ewdb_delete_data_before_time.sql,v $
 *     Revision 1.1  2004/09/07 19:31:14  davidk
 *     Reaper v2 09/02/2004.
 *           *
 *     Revision 1.1  2001/07/14 07:48:58  davidk            *
 *     Initial revision                                     *
 *                                                          *
 *                                                          *
 ************************************************************/

CREATE OR REPLACE FUNCTION Delete_Data_Before_Time
(IN_tTime            number,
 IN_iMaxRecsToDelete number
) RETURN NUMBER

  /*  Delete_Data_Before_Time Return codes:
           0 :   Success
          -1 :   Unknown error, see debug table for info
           1 :   Warning, some of the attempted deletions were not
                  possible due to ForeignKey issues
           2 :   More than iMaxRecsToDelete found, only
                  iMaxRecsToDelete deleted.
           3 :   See 1,2 above.
   ******************************************************************/
as
 
  Temp             number := 0;
  Temp_RetCode     number := 0;
  State            number;
  Temp_Count       number := 0;

begin
  State := 1;
  Temp := Delete_Waveforms_Before_Time(IN_tTime, IN_iMaxRecsToDelete/10);
  if(Temp < 0) then
    insert into test values('DDBT: DWBT FAIL' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    return(-1);
  elsif(Temp = 1 OR Temp = 3) then
    insert into test values('DDBT: DWBT WARN' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    Temp_RetCode := Temp_RetCode+1;
  end if;

  State := 2;
  Temp := Delete_SnipReqs_Before_Time(IN_tTime, IN_iMaxRecsToDelete);
  if(Temp < 0) then
    insert into test values('DDBT: DSRBT FAIL' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    return(-1);
  elsif(Temp = 1 OR Temp = 3) then
    insert into test values('DDBT: DSRBT WARN' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    Temp_RetCode := Temp_RetCode+1;
  end if;

  State := 3;
  Temp := Delete_PeakAmps_Before_Time(IN_tTime, IN_iMaxRecsToDelete);
  if(Temp < 0) then
    insert into test values('DPABT: DSRBT FAIL' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    return(-1);
  elsif(Temp = 1 OR Temp = 3) then
    insert into test values('DPABT: DSRBT WARN' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    Temp_RetCode := Temp_RetCode+1;
  end if;

  State := 4;
  Temp := Delete_Picks_Before_Time(IN_tTime, IN_iMaxRecsToDelete);
  if(Temp < 0) then
    insert into test values('DDBT: DPBT FAIL' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    return(-1);
  elsif(Temp = 1 OR Temp = 3) then
    insert into test values('DDBT: DPBT WARN' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    Temp_RetCode := Temp_RetCode+1;
  end if;

  State := 5;
  Temp := Delete_TCodas_Before_Time(IN_tTime, IN_iMaxRecsToDelete);
  if(Temp < 0) then
    insert into test values('DTCBT: DSRBT FAIL' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    return(-1);
  elsif(Temp = 1 OR Temp = 3) then
    insert into test values('DTCBT: DSRBT WARN' || IN_tTime, IN_iMaxRecsToDelete, Temp);
    Temp_RetCode := Temp_RetCode+1;
  end if;

  State := 6;
  return(Temp_RetCode);

EXCEPTION
  WHEN OTHERS THEN
    insert into test values('Delete_Data_Before_Time',IN_tTime,State);
	  return(-1);
END;

