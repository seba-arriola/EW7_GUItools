/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Release_Reserved_Snip_Reqs
(OUT_Retcode out number,
 IN_iReserveKey number
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

  update SnippetRetrievalSchedule
    set iLockTime = 0
    where iLockTime = IN_iReserveKey;

  State := 1;

  OUT_RetCode := 0;
  return;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
  	  OUT_RetCode := -2;
  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Release_Reserved_Snip_Reqs',State,Temp);
END;
