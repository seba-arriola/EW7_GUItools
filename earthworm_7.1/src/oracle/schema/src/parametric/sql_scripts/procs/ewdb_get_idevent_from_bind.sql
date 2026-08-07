/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_idEvent_From_Bind
(
 OUT_idEvent out number,
 IN_idCore number,
 IN_sCoreTableName varchar
)
as
/* Return Codes for OUT_idEvent:
          >0  DB idEvent
          -1  Unknown Error
          -2  idEvent not found
          -3  More than one idEvent not found
         -1X  Error occured in Check_Record_Validity(), X
                indicates the error returned.
				  Others:  Undefined
*/
Temp_idEvent Number;
Temp_tiCore  Number;

begin

  /**********************************/
  /* Check for core record validity   */
  /**********************************/

  Check_Record_Validity(Temp_tiCore,IN_idCore,IN_sCoreTableName);
  if not(Temp_tiCore > 0) then
   /* error, set return code and quit */
   OUT_idEvent := -10 + Temp_tiCore;
   return;
  end if;

  /**********************************/
  /* Check for Event Record */
  /**********************************/

  select idEvent into Temp_idEvent from Bind
    where tiCore=Temp_tiCore AND idCore=IN_idCore;

  OUT_idEvent := Temp_idEvent;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* Event not Found!!!
    /**********************************/
    OUT_idEvent := -2;
  WHEN TOO_MANY_ROWS THEN
    /**********************************/
    /* More than one Event Found!!!
    /**********************************/
    OUT_idEvent := -3;
  WHEN OTHERS THEN
    OUT_idEvent := -1;
END;


