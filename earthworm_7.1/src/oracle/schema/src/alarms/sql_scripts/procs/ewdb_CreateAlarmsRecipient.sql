/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE CreateAlarmsRecipient
(OUT_idRecipient out number,
 IN_dPriority number,
 IN_sDescription varchar,
 IN_bActive number
)
as
/* Return Codes for OUT_idRecipient:
                  >0  DB idRecipient
                  -1  Unknown Error
*/
Temp_idRecipient   number;
in_desc       	   varchar(256);

begin

	in_desc := RTRIM (IN_sDescription);

  /*********************************************/
  /* Check to see if this recipient already exists  */
  /*********************************************/
	select idRecipient into Temp_idRecipient from AlarmsRecipient
		where sDescription = in_desc;

	/* Update the priority and active-ness only */
	update AlarmsRecipient
		set AlarmsRecipient.dPriority = IN_dPriority,
		    AlarmsRecipient.bActive = IN_bActive
		where idRecipient = Temp_idRecipient;

	OUT_idRecipient := Temp_idRecipient;

EXCEPTION
	WHEN NO_DATA_FOUND THEN

		/* Insert the new recipient */
		select AlarmsRecipientSeq.NEXTVAL into Temp_idRecipient from sys.dual;
		insert into AlarmsRecipient (idRecipient, dPriority, sDescription, bActive)
			values (Temp_idRecipient, IN_dPriority, IN_sDescription, IN_bActive);

		OUT_idRecipient := Temp_idRecipient;

end;
