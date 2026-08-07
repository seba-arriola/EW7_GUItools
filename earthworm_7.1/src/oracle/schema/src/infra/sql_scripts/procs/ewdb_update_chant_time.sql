/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_ChanT_Time
(
 OUT_iRetCode out number,
 IN_idChanT number,
 IN_tOn number,
 IN_tOff number
)
 as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
      -2:     IN_idChanT invalid.  Not found in DB.
      -3:     An existing ChanT record overlaps with new time range
                (matching idChan overlapping tOff/tOn range)
      -4:     Error processing Comment

********************************************/
Temp           number;
Temp_RetCode   number;
State          number;
Temp_idChan    number;
Temp_idChanT   number;


BEGIN

  State := 1;
  select idChan into Temp_idChan
   from ChanT where idChanT = IN_idChanT;

  State := 2;
  select count(idChanT) into Temp
   from ChanT 
   where idChan = Temp_idChan
     AND tOn  < IN_tOff
     AND tOff > IN_tOn;

  State := 21;
  if(Temp > 0) then
    State := 22;
    if(Temp > 1) then
      OUT_iRetCode := -3;
      return;
    else
      State := 23;
      select idChanT into Temp_idChanT
       from ChanT 
       where idChan = Temp_idChan
         AND tOn  < IN_tOff
         AND tOff > IN_tOn;

      State := 24;
      if(Temp_idChanT != IN_idChanT) then
        OUT_iRetCode := -3;
        return;
      end if;
    end if;
  end if;


  State := 3;

  update ChanT set tOn = IN_tOn, tOff=IN_tOff
    where idChanT = IN_idChanT;

  State := 4;


  OUT_iRetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    if State = 1 then
      OUT_iRetCode := -2;
    else 
      Temp := SQLCODE;
      insert into test values('UpdateCTF_Excep',State,Temp);
      OUT_iRetCode := -1;
    end if;

END;
