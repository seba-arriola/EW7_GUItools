/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_ExternalEvent
(OUT_idExternalEvent out number,
 IN_idSource number,
 IN_sSourceEventID varchar
)
as
/* Return Codes for OUT_idExternalEvent:
                  >0  DB idExternalEvent
                  -1  Unknown Error
                  -4  Index problems on ExternalEvent.  SERIOUS!!!!
				  Others:  Undefined
*/

Temp_idExternalEvent Number;
Temp_idBind Number;
Temp                 number;
State                number;

begin

begin

  State := 1;
  /**********************************/
  /* Get A New ExternalEventID.             */
  /**********************************/
  select ExternalEventSeq.NEXTVAL into Temp_idExternalEvent from sys.dual;

  Create_Core_idKey(Temp_idExternalEvent);
  if Temp_idExternalEvent <= 0 then
    OUT_idExternalEvent := Temp_idExternalEvent;
  end if;

  State := 2;
  /**********************************/
  /* Insert new ExternalEvent Record        */
  /**********************************/
  insert into ExternalEvent(idExternalEvent,idSource,sSourceEventID)
    values(Temp_idExternalEvent,IN_idSource,IN_sSourceEventID);

  State := 3;

  /**********************************/
  /* Set the idExternalEvent return value   */
  /**********************************/

  State := 4;

  OUT_idExternalEvent := Temp_idExternalEvent;

EXCEPTION
  /**********************************/
  /* We aren't expecting any excep's*/
  /**********************************/
  WHEN DUP_VAL_ON_INDEX THEN
    /* somebody must have just created the EXEV before us */
    select idExternalEvent into Temp_idExternalEvent
     from ExternalEvent
     where idSource       = IN_idSource
       and sSourceEventID = IN_sSourceEventID;

    OUT_idExternalEvent := Temp_idExternalEvent;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('create_extevent1' || IN_sSourceEventID, Temp, State);
    insert into test values('create_extevent2' || IN_sSourceEventID,
                            IN_idSource, Temp_idExternalEvent);
    OUT_idExternalEvent := -1;
END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /******************************************/
    /* This is screwed up!  We try to insert the record, and it
       tells us one already exists, and then we try to select it,
       and its gone.  Somebody is either screwing with our minds
       or indexing on the ExternalEvent table is in chaos   */
    /******************************************/
    OUT_idExternalEvent := -4;
END;
