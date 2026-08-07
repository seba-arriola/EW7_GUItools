/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_ChanCTF
( IN_idChanCTF number
)
 RETURN number
as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
********************************************/
Temp           number;
Temp_idCTF     number;
State          number;


BEGIN

  State := 1;
  select idCTF into Temp_idCTF from ChanCTF where idChanCTF = IN_idChanCTF;

  State := 2;

  delete ChanCTF where idChanCTF = IN_idChanCTF;

  State := 3;

  if(Delete_CTF(Temp_idCTF) < 0) THEN
    return(-2);
  END IF;

  State := 4;

  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Delete_ChanCTF_Ex ' || IN_idChanCTF,Temp,State);
    insert into test values('Delete_ChanCTF_Ex ' || IN_idChanCTF,Temp_idCTF, Temp);
    return(-1);
END;

