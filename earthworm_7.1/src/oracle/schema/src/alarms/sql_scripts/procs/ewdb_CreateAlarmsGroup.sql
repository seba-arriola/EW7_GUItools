/*                                                          */
/*   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE CreateAlarmsGroup
(OUT_idGroup out number,
 IN_sName varchar,
 IN_bActive number
)
as
/* Return Codes for OUT_idRecipient:
                  >0  DB idRecipient
                  -1  Unknown Error
                  -2  Attempting to insert an existing recipient
*/
Temp_idGroup   number;
in_desc       	   varchar(256);

begin

	in_desc := RTRIM (IN_sName);

  /*********************************************/
  /* Check to see if this recipient already exists  */
  /*********************************************/
	select idGroup into Temp_idGroup from AlarmGroup
		where sGroupName = in_desc;

	/* Update the "active" flag only */
	update AlarmGroup
		set AlarmGroup.bActive = IN_bActive
		where idGroup = Temp_idGroup;

	OUT_idGroup := Temp_idGroup;

EXCEPTION
	WHEN NO_DATA_FOUND THEN

		/* Insert the new recipient */
		select GroupSeq.NEXTVAL into Temp_idGroup from sys.dual;
		insert into AlarmGroup (idGroup, sGroupName, bActive)
			values (Temp_idGroup, IN_sName, IN_bActive);

		OUT_idGroup := Temp_idGroup;

end;
