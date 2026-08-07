/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE  Get_Coda_Term_Times
(OUT_iRetCode out number,
 OUT_tCodaTermObs out number,
 OUT_tCodaTermXtp out number,
 IN_idTCoda number
)
as
/* Return Codes for OUT_iRetCode:
          >0  Success
          -1  Unknown Error
          -2  TCoda Record Not Found
				  Others:  Undefined
*/

begin

  /**********************************/
  /* Check for TCoda Record         */
  /**********************************/

  select tCodaTermObs,tCodaTermXtp 
    into OUT_tCodaTermObs,OUT_tCodaTermXtp from TCoda
    where idTCoda = IN_idTCoda;

  OUT_iRetCode := 1;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* TCoda not Found!!!
    /**********************************/
    OUT_iRetCode := -2;
  WHEN OTHERS THEN
    OUT_iRetCode := -1;
END;


