/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_Mag_Type_Info
(OUT_RetCode         out number,
 IN_iMagType             number,
 OUT_tiMagType       out number,
 OUT_sDatumTableName out varchar
)
as
/* Return Codes for OUT_RetCode:
          >0  DB iMagType
          -1  Unknown Error
          -2  tiMagType was NULL for IN_iMagType
          -3  MagType record not found for IN_iMagType
          -4  EWDB_TableList record not found 
				  Others:  Undefined
*/
Temp_tiMagType Number;
State          Number;

begin

  /**********************************/
  /* Check for Magnitude Record     */
  /**********************************/

  State := 1;

  select tiMagType into Temp_tiMagType from MagType
    where iMagType = IN_iMagType;

  if Temp_tiMagType IS NULL then
    OUT_RetCode := -2;
    return;
  end if;

  OUT_tiMagType := Temp_tiMagType;

  State := 2;

  select sTableName into OUT_sDatumTableName 
   from EWDB_TABLELIST
   where idTable = Temp_tiMagType;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      /**********************************
       * iMagType not Found!!!
       **********************************/
      OUT_RetCode := -3;
    elsif State = 1 then
      /**********************************
       * tiMagType not Found!!!
       **********************************/
      OUT_RetCode := -4;
    else
      OUT_RetCode := -1;
    end if;

  WHEN OTHERS THEN
    OUT_RetCode := -1;
END;


