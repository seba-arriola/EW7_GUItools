/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Core_idKey
(IN_OUT_idKey in out number
)
as
/* Return Codes for IN_OUT_idKey:
                  >0  DB idKey
                  -1  Unknown Error
                  -2  No local inst ID found
                  -3  More than one local inst ID found

				  Others:  Undefined
*/
Temp_ID          number;

begin
select EWDBNodeID into Temp_ID from EWDBNode where iIsMyNodeID = 1;

IN_OUT_idKey := (Temp_ID * 1000000000) + IN_OUT_idKey;


EXCEPTION
  WHEN NO_DATA_FOUND THEN
    IN_OUT_idKey := -9999;

  WHEN TOO_MANY_ROWS THEN
    IN_OUT_idKey := -9998;

  WHEN OTHERS THEN
    IN_OUT_idKey := -1;

END;
