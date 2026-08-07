/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_CompT_Time
(
 OUT_iRetCode out number,
 IN_idCompT number,
 IN_tOn number,
 IN_tOff number
)
 as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
      -2:     IN_idCompT invalid.  Not found in DB.
      -3:     An existing CompT record overlaps with new time range
                (matching idComp overlapping tOff/tOn range)
      -4:     Error processing Comment

********************************************/
Temp           number;
Temp_RetCode   number;
State          number;
Temp_idComp    number;
Temp_idCompT   number;


BEGIN

  State := 1;
  select idComp into Temp_idComp
   from CompT where idCompT = IN_idCompT;

  State := 2;
  select count(idCompT) into Temp
   from CompT 
   where idComp = Temp_idComp
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
      select idCompT into Temp_idCompT
       from CompT 
       where idComp = Temp_idComp
         AND tOn  < IN_tOff
         AND tOff > IN_tOn;

      State := 24;
      if(Temp_idCompT != IN_idCompT) then
        OUT_iRetCode := -3;
        return;
      end if;
    end if;
  end if;


  State := 3;

  update CompT set tOn = IN_tOn, tOff=IN_tOff
    where idCompT = IN_idCompT;

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
