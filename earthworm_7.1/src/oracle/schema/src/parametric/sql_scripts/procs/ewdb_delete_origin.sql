/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_Origin

(IN_idOrigin      number
)
RETURN NUMBER

as

Cursor Mag_Cursor(Cursor_idOrigin number) is
  select idMag from Magnitude
    where idOrigin=Cursor_idOrigin;

Cursor OP_Cursor(Cursor_idOrigin number) is
  select idPick from OriginPick
    where idOrigin=Cursor_idOrigin;

Temp            number;
State           number;
Temp_RetCode    number := 0;

begin
  State := 1;

  /* handle dependent magnitudes */
  select count(idMag) into Temp from Magnitude where idOrigin = IN_idOrigin;
  if(Temp > 0) then
    State := 2;
    for Curr in Mag_Cursor(IN_idOrigin) loop
      State := 3;
        Temp := Delete_Magnitude(Curr.idMag);
        if(Temp < 0) then
          return(-2);
        elsif(Temp > 0) then
          Temp_RetCode := Temp_RetCode + 1;
        end if;

    end loop;  /* for each mag */
  end if;


  State := 4;

  /* handle originpicks and associated picks */
  Delete OriginPick where idOrigin = IN_idOrigin;

  State := 8;
  Delete MechFM where idOrigin = IN_idOrigin;

  State := 9;
  Delete Origin where idOrigin = IN_idOrigin;

  return(Temp_RetCode);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) THEN    /* Foreign Key constraint */
      insert into test values('Delete_Origin_FK ' || IN_idOrigin, Temp, State);
      return(1);
    else
      insert into test values('Delete_Origin_Exc ' || IN_idOrigin, Temp, State);
      return(-1);
    end if;
END;