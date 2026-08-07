/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE CreatePhoneDelivery
(OUT_idDelivery out number,
 OUT_idRecipientDelivery out number,
 IN_idRecipient number,
 IN_sPhoneNumber varchar,
 IN_bisAudit number,
 IN_idDelivery number
)
as
/* Return Codes for OUT_idDelivery:
                  >0    DB idDelivery
                  -1    Unknown Error

   Return Codes for OUT_idRecipientDelivery:
                  -101  idDelivery found for this address, but no
						idRecipientDelivery found. 

                  -200  Audit flag is set, not returning idRecipientDelivery


   if bisAudit is set to 1, we are dealing with the Audit tables -- 
     do not insert into RecipientDelivery.

*/
State						number;
Temp_idDelivery				number;
Temp_idRecipientDelivery	number;
in_number						varchar(256);

begin

	in_number := RTRIM (IN_sPhoneNumber);

  /**************************************************/
  /* Check to see if this address already exists    */
  /**************************************************/
	if IN_bisAudit = 0 then

		State := 0;
		select idDelivery into Temp_idDelivery from PhoneDelivery
		/**	where sPhoneNumber = in_number; **/
		    where idDelivery = IN_idDelivery;

		update PhoneDelivery
                    set PhoneDelivery.sPhoneNumber = in_number
				where idDelivery = Temp_idDelivery;
				
		OUT_idDelivery := Temp_idDelivery;
	
		State := 1;
	
		/* idDelivery was found: check RecipientDelivery */
		select idRecipientDelivery into Temp_idRecipientDelivery from RecipientDelivery
			where idRecipient = IN_idRecipient 
				AND sTableName = 'phone' 
				AND idDelivery = Temp_idDelivery;
	
		OUT_idRecipientDelivery := Temp_idRecipientDelivery;

	else

        select AuditPhoneDeliverySeq.NEXTVAL into Temp_idDelivery from sys.dual;
        insert into AuditPhoneDelivery
            (idDelivery, sPhoneNumber)
            values (Temp_idDelivery, IN_sPhoneNumber);

        OUT_idDelivery := Temp_idDelivery;
        OUT_idRecipientDelivery := -200;

	end if;

EXCEPTION
	WHEN NO_DATA_FOUND THEN


		/* State = 0: idDelivery not found -- insert a new one */
		if State = 0 then
			select PhoneDeliverySeq.NEXTVAL into Temp_idDelivery from sys.dual;
			insert into PhoneDelivery 
				(idDelivery, sPhoneNumber)
				values (Temp_idDelivery, IN_sPhoneNumber);
	
			OUT_idDelivery := Temp_idDelivery;
	
			/* insert record into RecipientDelivery table */
			select RecipientDeliverySeq.NEXTVAL 
						into Temp_idRecipientDelivery from sys.dual;
			insert into RecipientDelivery 
				(idRecipientDelivery, idRecipient, sTableName, idDelivery)
					values (Temp_idRecipientDelivery, IN_idRecipient, 
													'phone', Temp_idDelivery);

			OUT_idRecipientDelivery := Temp_idRecipientDelivery;

		/* State = 1: can't find idRecipientDelivery where Delivery exists */
		elsif State = 1 then
			OUT_idRecipientDelivery := -101;
		else
			OUT_idDelivery := -1;
			OUT_idRecipientDelivery := -1;
		end if;
end;
