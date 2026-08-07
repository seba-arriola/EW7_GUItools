/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_alarms_by_event.sql,v $
/*     Revision 1.2  2004/09/09 19:25:38  davidk
/*     Original version was bogus.  Fixed it.
/*
/*     Revision 1.1  2004/09/07 19:31:14  davidk
/*     Reaper v2 09/02/2004.
/*                 */
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/



CREATE OR REPLACE Function Delete_Alarms_By_Event
(IN_idEvent      number
)
RETURN NUMBER

as

Cursor Alarms_Delivery_Cursor(Cursor_idEvent number) is
  select idDelivery from AlarmsAudit
    where idEvent=Cursor_idEvent;

Temp            number;
State           number;

begin
  State := 1;

  select count(idDelivery) into Temp from AlarmsAudit where idEvent = IN_idEvent;
  if(Temp = 0) then
    return(0);
  end if;

  State := 2;
  for Curr in Alarms_Delivery_Cursor(IN_idEvent) loop

    State := 3;
    delete AuditEmailDelivery where idDelivery = Curr.idDelivery;

    State := 4;
    delete AuditPagerDelivery where idDelivery = Curr.idDelivery;

    State := 5;
    delete AuditPhoneDelivery where idDelivery = Curr.idDelivery;

    State := 6;
    delete AuditQddsDelivery where idDelivery = Curr.idDelivery;

    State := 7;
    delete AuditCustomDelivery where idDelivery = Curr.idDelivery;

  end loop;

  State := 8;
  delete AlarmsAudit where idEvent = IN_idEvent;

  State := 9;
  delete CubeVersionNumber where idEvent = IN_idEvent;

  return(0);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Delete_Alarms_BEE ' || IN_idEvent, Temp, State);
END;