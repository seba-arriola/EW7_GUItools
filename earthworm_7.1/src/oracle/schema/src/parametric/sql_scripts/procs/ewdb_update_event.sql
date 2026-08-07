/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_update_event.sql,v 1.2 2005/05/12 20:46:50 mark Exp $
 *
 *    Revision history:
 *     $Log: ewdb_update_event.sql,v $
 *     Revision 1.2  2005/05/12 20:46:50  mark
 *     Added comments to event struct
 *
 *     Revision 1.1  2001/07/14 07:50:16  davidk
 *     Initial revision
 *
 *
 */

CREATE OR REPLACE PROCEDURE Update_Event
(OUT_RetCode out number,
 IN_idEvent number,
 IN_iEventType number,
 IN_iDubiocity number,
 IN_bArchived number,
 IN_idPubComment number,
 IN_idInternalComment number,
 IN_idContribComment number,
 IN_bSetEventType varchar,
 IN_bSetDubiocity number,
 IN_bSetArchived number,
 IN_bSetComment number
)
as
/* Return Codes for OUT_idEvent:
                   0  Success
                  -1  Unknown Error
                  -2  Invalid IN_idEvent.
                  -3  Error updating EventType
                  -4  Error updating Dubiocity
                  -5  Error updating Archived Flag
				  -6  Error updating Comments
				  Others:  Exist but not described, see code!(please)
*/
Temp                 number;
State                number;

begin
  State := 0;

  State := 1;

  if IN_bSetEventType != 0 then
    update event set tiEventType=IN_iEventType
     where idEvent = IN_idEvent;
  end if;

  State := 2;

  if IN_bSetDubiocity != 0 then
    update event set iDubiocity=IN_iDubiocity 
     where idEvent = IN_idEvent;
  end if;

  State := 3;

  if IN_bSetArchived != 0 then
    update event set bArchived=IN_bArchived
     where idEvent = IN_idEvent;
  end if;

  State := 4;

  if IN_bSetComment != 0 then
    update event
		set idComment=IN_idPubComment,
			idInternalComment=IN_idInternalComment,
			idContribMagComment=IN_idContribComment
		where idEvent = IN_idEvent;
  end if;

  State := 5;

  OUT_RetCode := 0;

EXCEPTION
  /**********************************/
  /* We aren't expecting any excep's*/
  /**********************************/
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;

  WHEN OTHERS THEN
    if State = 1 then
      /* Error updating iEventType */
      OUT_RetCode := -3;
    elsif State = 2 then
      /* Error updating iDubiocity */
      OUT_RetCode := -4;
    elsif State = 3 then
      /* Error updating bArchived */
      OUT_RetCode := -5;
    elsif State = 4 then
      /* Error updating comments */
      OUT_RetCode := -6;
    else
      /* Clueless */
      OUT_RetCode := -1;
    end if;
    Temp := SQLCODE;
    insert into test values('Update_Event_' || IN_idEvent, State, Temp);
END;
