/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Check_idEvent_Validity
(IN_OUT_idEvent in out number
)
as
/* Return Codes for IN_OUT_idEvent:
          >0  DB idEvent
          -1  Unknown Error
          -2  idEvent not found
				  Others:  Undefined
*/
Temp_idEvent Number;
begin
  /**********************************/
  /* Check for Event Record         */
  /**********************************/
  select idEvent into Temp_idEvent from Event
    where idEvent = IN_OUT_idEvent;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* Event not Found!!!
    /**********************************/
    IN_OUT_idEvent := -2;
  WHEN OTHERS THEN
    IN_OUT_idEvent := -1;
END;
