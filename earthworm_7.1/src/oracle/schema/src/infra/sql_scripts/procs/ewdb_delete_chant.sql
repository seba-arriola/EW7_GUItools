/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_ChanT
( IN_idChanT number
)
 RETURN number
as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
********************************************/
Temp           number;
Temp_idChanCTF number;

BEGIN
  select count(idChanCTF) into Temp from ChanCTF where idChanT = IN_idChanT;
  if(Temp = 1) THEN
    select idChanCTF into Temp_idChanCTF from ChanCTF  where idChanT = IN_idChanT;
    if(Delete_ChanCTF(Temp_idChanCTF) != 0) THEN
      return(-2);
    END IF;
  END IF;
  delete chant where idchant = IN_idChanT;
  return(0);
END;

