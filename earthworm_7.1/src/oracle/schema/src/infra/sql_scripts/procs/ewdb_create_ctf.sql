/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Create_CTF
(OUT_idCTF OUT number,
 OUT_idChanCTF OUT number,
 IN_sFuncDesc varchar,
 IN_Poles varchar,
 IN_Zeroes varchar,
 IN_idChanT number,
 IN_dGain number,
 IN_dSampRate number
 )
 as
/* RETURN CODES:
      -1:     Unknown Error see Debug Table
      -2:     Invalid idChanT
      -3:     Unknown NO_DATA_FOUND exception
     -1X:     Error in AssocCTF(), X is the Return Code
    -1XX:     Error in CreateCoreCTF(), XX is the Return Code


********************************************/
Temp_idCTF     number;
Temp_idChanCTF number := 0;
Temp           number;
State          number;

BEGIN

  State := 1;

  if IN_idChanT != 0 then
    select idChanT into Temp from ChanT where idChanT=IN_idChanT;
  end if;

  State := 2;
  select CTFSeq.NextVal into Temp_idCTF from sys.dual;

  Create_Core_idKey(Temp_idCTF);
  if Temp_idCTF <= 0 then
    OUT_idCTF  := Temp_idCTF;
  end if;

  State := 3;
  insert into CookedTF(idCTF,sFuncDesc)
    values (Temp_idCTF,IN_sFuncDesc);

  State := 4;
  Add_PolesAndZeroes_For_CTF(Temp,Temp_idCTF,IN_Poles,IN_Zeroes);
  if Temp <= 0 then
    OUT_idCTF := -100 + Temp;
    return;
  end if;

  State := 5;

  if IN_idChanT != 0 then
    Assoc_CTF(Temp_idChanCTF,Temp_idCTF,IN_idChanT,IN_dGain,IN_dSampRate);
    if Temp_idChanCTF <= 0 then
      OUT_idCTF := -10 + Temp_idChanCTF;
      return;
    end if;
  end if;

  State := 6;

  OUT_idCTF     := Temp_idCTF;
  OUT_idChanCTF := Temp_idChanCTF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      OUT_idCTF := -2;
    else
      Temp := SQLCODE;
      insert into test values('CreateCTF_NDF_Excep',State,Temp);
      OUT_idCTF := -3;
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('CreateCTF_Unknown_Excep',State,Temp);
    OUT_idCTF := -1;
END;
