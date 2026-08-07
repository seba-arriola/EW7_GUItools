/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_create_event.sql,v 1.3 2005/05/12 20:44:27 mark Exp $
 *
 *    Revision history:
 *     $Log: ewdb_create_event.sql,v $
 *     Revision 1.3  2005/05/12 20:44:27  mark
 *     Added comments to event struct
 *
 *     Revision 1.2  2001/07/14 07:45:11  davidk
 *     Added support for bArchived flag.
 *     Improved RCS header comment to get more info.
 *
 *
 */

CREATE OR REPLACE PROCEDURE Create_Event
(OUT_idEvent out number,
 IN_tiEventType number,
 IN_iDubiocity number,
 IN_sComment varchar,
 IN_sSource varchar,
 IN_sSourceEventID varchar
)
as
/* Return Codes for OUT_idEvent:
                  >0  DB idEvent
                  -1  Unknown Error
                  -2  Error in Get_idSource();
                  -3  Unknown NO_DATA_FOUND error
                  -7  Unknown error in execption handler
                -1XX  Error in Get_idExtEv_From_Source_EvID(),
                       XX indicates the error returned by
                       that procedure
                -2XX  Error in Create_Core_idKey(), where
                       XX indicates error returned by that proc.
                -3XX  Error in Create_Core_idKey(), where
                       XX indicates error returned by that proc.
                -4XX  Error in Create_Bind(), where
                       XX indicates error returned by that proc.
				  Others:  Exist but not described, see code!(please)
*/
Temp_idEvent         Number;
Temp_idComment       Number;
Temp_idSource        Number;
Temp_tiExternalEvent Number;
Temp_idBind          number;
Temp_idExternalEvent number;
State                number;
begin
begin
  State := 0;
  /**********************************/
  /* Is this for an external
  /* event that may already be in
  /* the database?
  /**********************************/
  if NOT(IN_sSource is NULL) then
    State := 1;
    /* Get the idSource of the Source */
    Get_idSource(Temp_idSource,IN_sSource);
    if Temp_idSource <= 0 then
      OUT_idEvent := -2; /* Error in Get_idSource() */
      return;
    end if;
    State := 2;
    /* Get the Get_idExtEv_From_Source_EvID for the external event */
    Get_idExtEv_From_Source_EvID(Temp_idExternalEvent,
                                 Temp_idSource,IN_sSourceEventID);
    if Temp_idExternalEvent <= 0 then
      OUT_idEvent := -100 + Temp_idExternalEvent;
                             /* Error in Create_External_event() */
      insert into test values('Create_Event_2',Temp_idExternalEvent,State);
      return;
    end if;
    State := 3;
    /* Get the table identifier for 'ExternalEvent' */
    select idTable into Temp_tiExternalEvent from EWDB_Tablelist
      where sTableName = 'ExternalEvent';
    State := 4;
    /* Get the idEvent for our idExternalEvent via Bind */
    select idEvent into Temp_idEvent from Bind
      where idCore = Temp_idExternalEvent
        and tiCore = Temp_tiExternalEvent;
    State := 5;
  else
    State := 11;
    /**********************************/
    /* Deal with Comment Strings      */
    /**********************************/
	if IN_sComment IS NULL then
	  Temp_idComment := 0;
	else
	  Create_Comment(Temp_idComment, IN_sComment);
	end if;
    State := 12;
    /**********************************/
    /* Get A New EventID.             */
    /**********************************/
    select EventSeq.NEXTVAL into Temp_idEvent from sys.dual;
    Create_Core_idKey(Temp_idEvent);
    if Temp_idEvent <= 0 then
      OUT_idEvent := -200 + Temp_idEvent;
    end if;
    State := 13;
    /**********************************/
    /* Insert new Event Record        */
    /**********************************/
    insert into event(idEvent,tiEventType,iDubiocity,bArchived,idComment,
						idInternalComment,idContribMagComment)
      values(Temp_idEvent,IN_tiEventType,IN_iDubiocity,0/*bArchived*/,Temp_idComment,0,0);
    State := 14;
  end if;
  /**********************************/
  /* Set the idEvent return value   */
  /**********************************/
  OUT_idEvent := Temp_idEvent;
EXCEPTION
  /**********************************/
  /* We aren't expecting any excep's*/
  /**********************************/
  WHEN NO_DATA_FOUND THEN
    if State = 4 then
      /* The External Event has not yet been bound to an Event */
      State := 21;
      /**********************************/
      /* Deal with Comment Strings      */
      /**********************************/
      if IN_sComment IS NULL then
        Temp_idComment := 0;
      else
        Create_Comment(Temp_idComment, IN_sComment);
      end if;
      State := 22;
      /**********************************/
      /* Get A New EventID.             */
      /**********************************/
      select EventSeq.NEXTVAL into Temp_idEvent from sys.dual;
      Create_Core_idKey(Temp_idEvent);
      if Temp_idEvent <= 0 then
        OUT_idEvent := -300 +Temp_idEvent;
        return;
      end if;
      State := 23;
      /**********************************/
      /* Insert new Event Record        */
      /**********************************/
      insert into event(idEvent,tiEventType,iDubiocity,bArchived,idComment)
        values(Temp_idEvent,IN_tiEventType,IN_iDubiocity,0/*bArchived*/,Temp_idComment);
      State := 24;
      /**********************************/
      /* Bind the ExternalEvent to our new Event
      /**********************************/
      Create_Bind(Temp_idBind,Temp_idEvent,'ExternalEvent',
                  Temp_idExternalEvent);
      if Temp_idBind <= 0 then
        OUT_idEvent := -400 + Temp_idBind;
        insert into test values('Create_event_bind',Temp_idEvent,Temp_idExternalEvent);
        return;
      end if;
      OUT_idEvent := Temp_idEvent;
    else
      /* State != 4 */
      OUT_idEvent := -3;  /* Unknown NO_DATA_FOUND error */
    end if;
  WHEN OTHERS THEN
    insert into test values('Create_Event',Temp_idExternalEvent,State);
    OUT_idEvent := -1;
END;
EXCEPTION
  WHEN OTHERS THEN
    insert into test values('Create_Event',Temp_idExternalEvent,State);
    OUT_idEvent := -7;
END;
