/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE  Get_Magnitude_Type
(OUT_iMagType out number,
 IN_idMag number
)
as
/* Return Codes for OUT_iMagType:
          >0  DB iMagType
          -1  Unknown Error
          -2  Magnitude Record Not Found
          -3  iMagType was null!
				  Others:  Undefined
*/
Temp_iMagType Number;
Temp_RetCode Number;
Temp_sTableName varchar(50);

begin

  /**********************************/
  /* Check for Magnitude Record     */
  /**********************************/

  select iMagType into Temp_iMagType from Magnitude
    where idMag = IN_idMag;

  if Temp_iMagType IS NULL then
    OUT_iMagType := -3;
    return;
  end if;

    
  OUT_iMagType := Temp_iMagType;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* Magnitude not Found!!!
    /**********************************/
    OUT_iMagType := -2;
  WHEN OTHERS THEN
    OUT_iMagType := -1;
END;



