/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Create_PoleOrZero
(OUT_idPZ OUT number,
 IN_idCTF number,
 IN_cPZType char,
 IN_dReal number,
 IN_dImaginary number
 )
 as
/* RETURN CODES:
      -1:     Unknown Error see Debug Table
      -2:     Invalid idCTF
      -4:     Unknown NO_DATA_FOUND exception
      -5:     Unknown DUP_VAL_ON_INDEX exception


********************************************/
Temp_idPZ      number;
Temp           number;
State          number;
BEGIN
  
  State :=1;
  select idCTF into Temp from CookedTF where idCTF=IN_idCTF;

  State :=2;
  select PZSeq.NextVal into Temp_idPZ from sys.dual;

  Create_Core_idKey(Temp_idPZ);
  if Temp_idPZ <= 0 then
    OUT_idPZ := Temp_idPZ;
  end if;

  insert into PolesAndZeroes(idPZ,idCTF,cPZType,dReal,dImaginary)
    values(Temp_idPZ,IN_idCTF,IN_cPZType,IN_dReal,IN_dImaginary);

  OUT_idPZ := Temp_idPZ;

EXCEPTION
 WHEN NO_DATA_FOUND THEN
   if State = 1 then
     OUT_idPZ := -2;
   else
     OUT_idPZ := -4;
     Temp := SQLCODE;
     insert into test values('CreatePoleOrZero_NDF_Excep',State,Temp);
   end if;
 WHEN DUP_VAL_ON_INDEX THEN
   Temp := SQLCODE;
   insert into test values('CreatePoleOrZero_DVOI_Excep',State,Temp);
   OUT_idPZ := -5;
 WHEN OTHERS THEN
   Temp := SQLCODE;
   insert into test values('CreatePoleOrZero_Unknown_Excep',State,Temp);
   OUT_idPZ := -1;
 END;
