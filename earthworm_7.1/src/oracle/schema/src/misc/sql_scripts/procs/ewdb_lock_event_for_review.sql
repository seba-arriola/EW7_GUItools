/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Lock_Event_For_Review
(OUT_Retcode out number,
 IN_idEvent number,
 IN_iLockTime number,
 IN_idSource number,
 IN_sSource varchar,
 IN_sMachineName varchar,
 IN_sNote varchar
)
 as



/*
    Lock a Solution for Review.  Prevents multiple users from concurrently
    reviewing the same event.
    DK 07/31/2002
  ********************************************************************/


/* RETURN CODES:
       0:     Success
       1:     Event already locked
      -1:     Unknown Exception while updating SnipReq (see Debug Table)
      -2:     Invalid idEvent
      -3:     Invalid idSource
      -4:     Invalid sSource

********************************************/
Temp                       number;
State                      number;

BEGIN

  State := 0;

  select idEvent into Temp from Event where idEvent = IN_idEvent;

  State := 1;

  if(IN_idSource > 0) then
    select idSource into Temp from Source where idSource = IN_idSource;
  else
    select idSource into Temp from Source where sSource = IN_sSource;
  end if;

  State := 2;
  insert into EventLock(idEvent, idSource, sMachineName, iLockTime, sNote)
    values(IN_idEvent, Temp, IN_sMachineName, IN_iLockTime, IN_sNote);

  State := 3;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
     OUT_RetCode := -2;  /* unknown idEvent */
    elsif State = 1 then
      if IN_idSource > 0 then
       OUT_RetCode := -3;  /* unknown idSource */
      else
       OUT_RetCode := -4;  /* unknown sSource */
      end if;
    else
     OUT_RetCode := -1;
      Temp := SQLCODE;
      insert into test values('Lock_Event_NDF',State,Temp);
    end if;

  WHEN DUP_VAL_ON_INDEX THEN
    if State = 2 then
      OUT_RetCode := 1;
    else
      OUT_RetCode := -1;
      Temp := SQLCODE;
      insert into test values('Lock_Event_DVOI',State,Temp);
    end if;

  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Lock_Event',State,Temp);
END;


