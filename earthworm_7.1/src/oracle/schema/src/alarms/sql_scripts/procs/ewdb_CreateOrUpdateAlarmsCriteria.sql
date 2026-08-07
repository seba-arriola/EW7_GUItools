/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE CreateOrUpdateAlarmsCriteria
(OUT_idCritProgram out number,
 IN_sProgName varchar,
 IN_sProgDir varchar,
 IN_sProgDescription varchar,
 IN_idCritProgram number
)
as
/* Return Codes for OUT_idCritProgram:
                  >0  DB idCritProgram
                  -1  Unknown Error
*/
Temp_idCrit			number;

begin

  /**************************************************/
  /* Check to see if this criterion already exists  */
  /**************************************************/
	select idCritProgram into Temp_idCrit from CriteriaProgram
		where idCritProgram = IN_idCritProgram;

	/* 
		Update the criteria
	 */
	OUT_idCritProgram := Temp_idCrit;

	update CriteriaProgram
		set CriteriaProgram.sProgName = IN_sProgName,
			CriteriaProgram.sProgDir = IN_sProgDir,
			CriteriaProgram.sProgDescription = IN_sProgDescription
		where idCritProgram = Temp_idCrit;
			

EXCEPTION
	WHEN NO_DATA_FOUND THEN

		/* Insert the new program */
		select AlarmsCritSeq.NEXTVAL into Temp_idCrit from sys.dual;
		insert into CriteriaProgram 
			(idCritProgram, sProgName, sProgDir, sProgDescription)
			values (Temp_idCrit, IN_sProgName, IN_sProgDir, IN_sProgDescription);

		OUT_idCritProgram := Temp_idCrit;


end;
