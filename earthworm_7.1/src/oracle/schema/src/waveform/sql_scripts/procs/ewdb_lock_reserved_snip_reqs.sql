/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Lock_Reserved_Snip_Reqs
(OUT_Retcode out number,
 IN_iReserveKey number,
 IN_iLockTime number
)
 as


/*
    Lock a set of reserved snippet requests.  This procedure locks
    all "snippet request" records that are reserved with the given
    Reserve Key.
    DK 04/15/2002
  ********************************************************************/


/* RETURN CODES:
       0:     Success
      -1:     Unknown Exception while updating SnipReq (see Debug Table)
      -2:     SnippetRequest not found for given iReserveKey

********************************************/
Temp                       number;
State                      number;

BEGIN

  State := 0;

  select count(idSnipReq)into Temp
    from SnippetRetrievalSchedule
    where iLockTime = IN_iReserveKey;

  State := 1;

  update SnippetRetrievalSchedule
    set iLockTime = IN_iLockTime
    where iLockTime = IN_iReserveKey;
  State := 2;


  OUT_RetCode := 0;
  return;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
    Temp := SQLCODE;
    insert into test values('Lock_Reserved_Snip_Reqs-NDF',IN_iReserveKey,IN_iLockTime);
  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Lock_Reserved_Snip_Reqs',State,Temp);
END;

