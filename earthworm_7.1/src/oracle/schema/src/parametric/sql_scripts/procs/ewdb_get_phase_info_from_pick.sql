/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE  Get_Phase_Info_From_Pick
(OUT_iRetCode out number,
 OUT_tPhase out number,
 OUT_sPhase out varchar,
 IN_idPick number
)
as
/* Return Codes for OUT_iRetCode:
          >0  Success
          -1  Unknown Error
          -2  Pick Record Not Found
				  Others:  Undefined
*/

begin

  /**********************************/
  /* Check for Pick Record     */
  /**********************************/

  select tPhase,sPhase into OUT_tPhase,OUT_sPhase from Pick
    where idPick = IN_idPick;

  OUT_iRetCode := 1;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* Pick not Found!!!
    /**********************************/
    OUT_iRetCode := -2;
  WHEN OTHERS THEN
    OUT_iRetCode := -1;
END;


