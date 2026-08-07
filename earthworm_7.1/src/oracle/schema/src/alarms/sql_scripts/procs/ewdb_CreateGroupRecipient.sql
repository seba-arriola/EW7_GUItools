/*                                                          */
/*   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_GroupRecipient
(OUT_idGroupRecipient out number,
 IN_idGroup number,
 IN_idRecipientDelivery number,
 IN_bActive number
)
as
/* Return Codes for OUT_idRecipient:
                  >0  DB idRecipient
                  -1  Unknown Error
                  -2  Attempting to insert an existing recipient
*/
Temp_idGroupRecipient   number;

begin

  /*********************************************/
  /* Check to see if this recipient already exists  */
  /*********************************************/
	select idGroupRecipient into Temp_idGroupRecipient from AlarmGroupRecipient
		where idRecipientDelivery = IN_idRecipientDelivery
		AND idGroup = IN_idGroup;

	/* Update the "active" flag only */
	update AlarmGroupRecipient
		set AlarmGroupRecipient.bActive = IN_bActive
		where idGroupRecipient = Temp_idGroupRecipient;

	OUT_idGroupRecipient := Temp_idGroupRecipient;

EXCEPTION
	WHEN NO_DATA_FOUND THEN

		/* Insert the new recipient */
		select GroupRecipientSeq.NEXTVAL into Temp_idGroupRecipient from sys.dual;
		insert into AlarmGroupRecipient (idGroupRecipient, idGroup, idRecipientDelivery, bActive)
			values (Temp_idGroupRecipient, IN_idGroup, IN_idRecipientDelivery, IN_bActive);

		OUT_idGroupRecipient := Temp_idGroupRecipient;

end;
