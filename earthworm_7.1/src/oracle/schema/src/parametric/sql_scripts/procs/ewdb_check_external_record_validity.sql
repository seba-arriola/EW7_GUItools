/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE or REPLACE PROCEDURE Check_External_Record_Validity
(
 OUT_TableID out number,
 IN_Ext_RecordID  in varchar,
 IN_sTableName in varchar
)
as
/* Return Codes for OUT_TableID:
          >0  DB Record ID
          -1  Unknown Error
                                  -4  Invalid external table id
                                  -5  Check Not Implemented for Listed Table
           0  IN_sTableName is NULL, or the IN_Ext_RecordID
               is Null, or Invalid external table name
                                  Others:  Undefined
*/

State              Number := 0;
Temp_idTable       Number;
Temp_Ext_RecordID  Varchar(32);
err_num Number;
err_msg Varchar(150);

begin


  /**********************************/
  /* Check for NULLs, cause they're */
  /* special                        */
  /**********************************/
  if (IN_sTableName IS NULL) OR (IN_Ext_RecordID IS NULL) then
    OUT_TableID := 0;
    return;
  end if;

  /**********************************/
  /* Check for Tablename validity   */
  /**********************************/

  select idTable into Temp_idTable from EWDB_Tablelist
    where sTableName = IN_sTableName;

  OUT_TableID := Temp_idTable;

  State := 1;

  /**********************************/
  /* Check for external record validity   */
  /**********************************/

  /* We have to figure out which table we are looking in, and
     then query it.  We are choosing to do it the brutally painfull
           way for the programmer, yet the fast and efficient way for the
           application.  Poor programmer ...
   **********************************/
   /*
   **********************************/


  OUT_TableID := -5;

EXCEPTION
  /**********************************/
  /* Set the appropriate error code
  /*  for OUT_TableID
  /**********************************/
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
          /*  Tablename not found in Tablelist */
          OUT_TableID := 0;
        elsif State = 1 then
          /*  Record not found in table!*/
          OUT_TableID := -4;
        else
          OUT_TableID := -1;
        end if;

  WHEN OTHERS THEN
    err_num := SQLCODE;
    if err_num = -4091 then
      /*************************************************8
         we are being called from a trigger, and are not allowed to
         look at the table on which the trigger is running.  Return
         the Temp_idTable, and accept that the external record might
         not be there.  This is the same as if the external record
         was once there but has been deleted.  We will have to put the
         brunt of the external record validity checking on the programmer
         who wrote the trigger.  (we're in trouble!)
      ****************************************************/
      OUT_TableID := Temp_idTable;
    else
      err_msg := SUBSTR(SQLERRM, 1, 150);
      INSERT INTO test VALUES (err_msg,Temp_idTable,err_num);
            OUT_TableID := -1;
    end if;
END;
