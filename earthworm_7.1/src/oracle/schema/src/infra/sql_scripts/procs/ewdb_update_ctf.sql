/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_CTF
(OUT_Retcode out number,
 IN_idCTF number,
 IN_sFuncDesc varchar,
 IN_Poles varchar,
 IN_Zeroes varchar,
 IN_bUpdatePZ number
)
 as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
      -2:     Unknown Exception during update of Cooked TF record
      -3:     Unknown Exception during deletion of previous poles and zeroes
      -4:     Unknown Exception during insertion of new poles and zeroes
     -1X:     Error in DeletePZForCTF(), X is the Return Code
     -2X:     Error in Add_PolesAndZeroes_For_CTF(), X is the Return Code

********************************************/
Temp           number;
Temp_RetCode   number;
State          number;

BEGIN

  State := 1;
  update CookedTF set sFuncDesc=IN_sFuncDesc where idCTF=IN_idCTF;

  State := 2;

  if IN_bUpdatePZ = 1 then

    State := 3;

    Delete_PZ_For_CTF(Temp_Retcode,IN_idCTF);
    if Temp_RetCode < 0 then
      OUT_RetCode := -10 + Temp_RetCode;
      return;
    end if;

    State := 4;

    Add_PolesAndZeroes_For_CTF(Temp_RetCode,IN_idCTF,IN_Poles,IN_Zeroes);
    if Temp_RetCode < 0 then
      OUT_RetCode := -20 + Temp_RetCode;
      return;
    end if;
  end if;  /* end if update Poles And Zeroes */

  State := 5;


  OUT_RetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    if State = 1 then
      OUT_RetCode := -2;
    elsif State = 3 then
      OUT_RetCode := -3;
    elsif State = 4 then
      OUT_RetCode := -4;
    else 
      OUT_RetCode := -1;
    end if;

    Temp := SQLCODE;
    insert into test values('UpdateCTF_Excep',State,Temp);
END;
