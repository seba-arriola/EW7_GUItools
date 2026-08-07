/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE CreateOrUpdateAlarmsFormat
(OUT_idFormat out number,
 IN_sDescription varchar,
 IN_sFmtInsert varchar,
 IN_sFmtDelete varchar,
 IN_idFormat number
)
as
/* Return Codes for OUT_idFormat:
                  >0  DB idFormat
                  -1  Unknown Error
                  -2  Attempting to insert an existing format
*/
Temp_idFormat 	   number;

begin

  /***********************************************/
  /* Check to see if this format already exists  */
  /***********************************************/
	select idFormat into Temp_idFormat from AlarmsFormat
		where idFormat = IN_idFormat;

	/*  Update the info */

	update AlarmsFormat
        set AlarmsFormat.sFmtInsert = IN_sFmtInsert,
			AlarmsFormat.sFmtDelete = IN_sFmtDelete,
			AlarmsFormat.sDescription = IN_sDescription
		where idFormat = Temp_idFormat;

	OUT_idFormat := Temp_idFormat;

EXCEPTION
	WHEN NO_DATA_FOUND THEN

		/* Insert the new format */
		select AlarmsFormatSeq.NEXTVAL into Temp_idFormat from sys.dual;
		insert into AlarmsFormat (idFormat, sDescription, sFmtInsert, sFmtDelete)
			values (Temp_idFormat, IN_sDescription, IN_sFmtInsert, IN_sFmtDelete);

		OUT_idFormat := Temp_idFormat;

end;
