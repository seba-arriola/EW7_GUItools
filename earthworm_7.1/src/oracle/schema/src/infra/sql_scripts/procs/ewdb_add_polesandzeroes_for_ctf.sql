/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Add_PolesAndZeroes_For_CTF
(OUT_RetCode OUT number,
 IN_idCTF number,
 IN_Poles varchar,
 IN_Zeroes varchar
 )
 as
/* RETURN CODES:
      -1:     Unknown Error see Debug Table
      -2:     Problem in CreatePoleOrZero() for a Pole Record
      -3:     Problem in CreatePoleOrZero() for a Zero Record
      -4:     Problem during parsing for a Pole Record
      -5:     Problem during parsing for a Zero Record


********************************************/
Temp_idPZ         number;
Temp              number;
State             number;
Temp_dReal        number;
Temp_dImaginary   number;
Temp_String       varchar(1000);
TempNumber        number;
TempNumberString  varchar(20);
BEGIN

  State := 3;
  /* parse the poles */

  Temp := 1;
  Temp_String := LTRIM(IN_Poles);

  
  TempNumberString := substr(Temp_String,1,instr(Temp_String || ' ',' '));
  TempNumber := ewdb_VCtoNum(TempNumberString);

  while TempNumber IS NOT NULL LOOP
    Temp := Temp+1;
    Temp_dReal := TempNumber;
    Temp_String := substr(Temp_String,instr(Temp_String || '  ',' ')+1);
    /* there may be a problem here at the end of the string, if instr() returns
       a position at the '<SPACE>' that was concatenated on to Temp_String.
       We may need to add a '<SPACE>' to the end during each pass. DK */
    TempNumberString := substr(Temp_String,1,instr(Temp_String || ' ',' '));
    TempNumber := ewdb_VCtoNum(TempNumberString);
    if TempNumber IS NOT NULL then
      Temp_dImaginary := TempNumber;
    else
      insert into test values('Add_PZ_For_CTF_PoleParse',-4,Temp);
      OUT_RetCode := -4;
      return;
    end if;

    Create_PoleOrZero(Temp_idPZ,IN_idCTF,1/*Pole*/,Temp_dReal,Temp_dImaginary);
    if Temp_idPZ <= 0 then
      OUT_RetCode := -2;
      insert into test values('Add_PZ_For_CTF_Poles',Temp_idPZ,Temp);
      return;
    end if;
    Temp_String := substr(Temp_String,instr(Temp_String || '  ',' ')+1);
    TempNumberString := substr(Temp_String,1,instr(Temp_String || ' ',' '));
    TempNumber := ewdb_VCtoNum(TempNumberString);
      
  END LOOP;

  State := 4;
  /* parse the zeroes */
  Temp := 1;
  Temp_String := LTRIM(IN_Zeroes);

  TempNumberString := substr(Temp_String,1,instr(Temp_String || ' ',' '));
  TempNumber := ewdb_VCtoNum(TempNumberString);

  while TempNumber IS NOT NULL LOOP
    Temp := Temp+1;
    Temp_dReal := TempNumber;
    Temp_String := substr(Temp_String,instr(Temp_String || '  ',' ')+1);
    /* there may be a problem here at the end of the string, if instr() returns
       a position at the '<SPACE>' that was concatenated on to Temp_String.
       We may need to add a '<SPACE>' to the end during each pass. DK */
    TempNumberString := substr(Temp_String,1,instr(Temp_String || ' ',' '));
    TempNumber := ewdb_VCtoNum(TempNumberString);
    if TempNumber IS NOT NULL then
      Temp_dImaginary := TempNumber;
    else
      insert into test values('Add_PZ_For_CTF_ZeroesParse',-5,Temp);
      OUT_RetCode := -5;
      return;
    end if;

    Create_PoleOrZero(Temp_idPZ,IN_idCTF,0/*Zero*/,Temp_dReal,Temp_dImaginary);
    if Temp_idPZ <= 0 then
      OUT_RetCode := -3;
      insert into test values('Add_PZ_For_CTF_Poles',Temp_idPZ,Temp);
      return;
    end if;
    Temp_String := substr(Temp_String,instr(Temp_String || '  ',' ')+1);
    TempNumberString := substr(Temp_String,1,instr(Temp_String || ' ',' '));
    TempNumber := ewdb_VCtoNum(TempNumberString);
  END LOOP;

  State := 5;
  /* done */

  OUT_RetCode := IN_idCTF;
EXCEPTION
  WHEN OTHERS THEN
    insert into test values('Add_PZ_For_CTF_Excep',State,Temp);
    OUT_RetCode := -1;
END;
