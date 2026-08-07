/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_CTF
( IN_idCTF number
)
 RETURN number
as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
********************************************/
Temp           number;
State          number;

BEGIN
  State := 1;
  Delete_PZ_For_CTF(Temp, IN_idCTF);
  State := 2;

  delete CookedTF where idCTF = IN_idCTF;
  State := 3;

  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;

    if(Temp = -2292  AND State = 2) THEN
      return(1);
    END IF;

    insert into test values('Delete_CTF_Ex ' || IN_idCTF,Temp, State);
    return(-1);
END;

