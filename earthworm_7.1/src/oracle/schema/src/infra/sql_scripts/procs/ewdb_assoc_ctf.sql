/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Assoc_CTF
(OUT_idChanCTF OUT number,
 IN_idCTF number,
 IN_idChanT number,
 IN_dGain number,
 IN_dSampRate number
 )
 as

/* RETURN CODES:
      -1:     Unknown Error see Debug Table
      -2:     Invalid idChanT
      -3:     Invalid idCTF
      -4:     Unknown NO_DATA_FOUND exception
      -5:     Unknown DUP_VAL_ON_INDEX exception


********************************************/
Temp_idChanCTF number;
Temp           number;
State          number;
Temp_idCTF     number;

BEGIN
  
  State := 1;
  select idChanT into Temp from ChanT where idChanT=IN_idChanT;

  State :=2;
  if(IN_idCTF <= 0) then 
    Temp_idCTF := NULL;
  else
    select idCTF into Temp_idCTF from CookedTF where idCTF=IN_idCTF;
  end if;

  State :=3;
  select ChanCTFSeq.NextVal into Temp_idChanCTF from sys.dual;

  Create_Core_idKey(Temp_idChanCTF);
  if Temp_idChanCTF <= 0 then
    OUT_idChanCTF := Temp_idChanCTF;
  end if;

  insert into ChanCTF(idChanCTF,idChanT,idCTF,dGain,dSampRate)
    values(Temp_idChanCTF,IN_idChanT,Temp_idCTF,IN_dGain,IN_dSampRate);

  OUT_idChanCTF := Temp_idChanCTF;
EXCEPTION
 WHEN NO_DATA_FOUND THEN
   if State = 1 then
     OUT_idChanCTF := -2;
   elsif State = 2 then
     OUT_idChanCTF := -3;
   else
     OUT_idChanCTF := -4;
     Temp := SQLCODE;
     insert into test values('AssocCTF_NDF_Excep',State,Temp);
   end if;
 WHEN DUP_VAL_ON_INDEX THEN
   if State = 3 then
     update ChanCTF set idCTF=Temp_idCTf, dGain=IN_dGain, dSampRate=IN_dSampRate
      where idChanT=IN_idChanT;
     select idChanCTF into OUT_idChanCTF from ChanCTF where idChant=IN_idChanT;
   else
     Temp := SQLCODE;
     insert into test values('AssocCTF_DVOI_Excep',State,Temp);
     OUT_idChanCTF := -5;
   end if;
 WHEN OTHERS THEN
   Temp := SQLCODE;
   insert into test values('AssocCTF_Unknown_Excep',State,Temp);
   OUT_idChanCTF := -1;
END;
