/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */



CREATE OR REPLACE PROCEDURE Get_Event_Lock_Info
(OUT_Retcode out number,
 IN_idEvent number,
 OUT_iLockTime    OUT number,
 OUT_sMachineName OUT varchar,
 OUT_idSource     OUT number,
 OUT_sSource      OUT varchar,
 OUT_sNote        OUT varchar
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

********************************************/
Temp                       number;
State                      number;

BEGIN

  State := 0;

  select el.iLockTime, el.idSource, el.sMachineName, el.sNote, s.sSource
    into OUT_iLockTime, OUT_idSource, OUT_sMachineName, OUT_sNote, OUT_sSource
   from EventLock el, Source s
   where el.idSource = s.idSource
      and el.idEvent  = IN_idEvent;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
 	  OUT_RetCode := -2;  /* unknown idEvent */
      
  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Get_Event_Lock_Info',State,Temp);
END;
