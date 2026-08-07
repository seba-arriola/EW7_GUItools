/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_Actual_Waveform
(IN_idWaveform number,
 IN_StorageType number,
 IN_StorageInfo number) return number

as 

Temp     number;

begin
  delete Waveform where idWaveform=IN_idWaveform;
  /* note this will have to be altered as new StorageTypes
     are added.  */
  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test values('Delete_Actual_Waveform',IN_idWaveform,Temp);
      return(-1);
    end if;
END;

