/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */



CREATE OR REPLACE PROCEDURE Release_Locked_Event
(OUT_Retcode out number,
 IN_idEvent number,
 IN_iLockTime number
)
 as



/*
    Unlock a Solution that has been locked for Review.  
    Prevents multiple users from concurrently reviewing the same Event.
    DK 07/31/2002
  ********************************************************************/


/* RETURN CODES:
       0:     Success:   Event unlocked
      -1:     Unknown Exception while unlocking Event (see Debug Table)
      -2:     Event not locked.
      -3:     Incorrect lock time.

********************************************/
Temp                       number;
State                      number;

BEGIN

  State := 0;

  select idEvent into Temp from EventLock where idEvent = IN_idEvent;

  State := 1;

  select idEvent into Temp from EventLock 
    where idEvent = IN_idEvent and iLockTime=IN_iLockTime;

  State := 2;

  delete EventLock 
    where idEvent = IN_idEvent and iLockTime = IN_iLockTime;

  State := 3;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
  	  OUT_RetCode := -2;  /* unknown idEvent */
    elsif State = 1 then
  	  OUT_RetCode := -3;  /* bad iLockTime */
    else
  	  OUT_RetCode := -1;
      Temp := SQLCODE;
      insert into test values('Release_Locked_Event_NDF',State,Temp);
    end if;
      
  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Release_Locked_Event',State,Temp);
END;
