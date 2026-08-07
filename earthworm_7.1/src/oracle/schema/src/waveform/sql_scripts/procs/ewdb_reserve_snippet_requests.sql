/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */



CREATE OR REPLACE PROCEDURE Reserve_Snippet_Requests
(OUT_Retcode out number,
 IN_idSnipReq number,
 IN_tThreshold number,
 IN_iRequestGroup number,
 IN_iReserveKey number
)
 as


/*
    Reserve a set of snippet requests, so that they can be locked and 
    processed by a data_retrieval process like ora_trace_fetch.
    This procedure takes several criteria params as input,
    selects a set of records from the existing snippet_requests,
    based on the given criteria, then marks the requests as reserved.
    DK 04/15/2002
  ********************************************************************/


/* RETURN CODES:
       0:     Success
      -1:     Unknown Exception while updating SnipReq (see Debug Table)
      -2:     SnippetRequest not found for given idSnipReq
      -3:     SnippetRequest not found for given tThreshold/iRequestGroup
      -4:     SnippetRequest not found for given tThreshold
      -5:     Unknown NO_DATA_FOUND exception

********************************************/
Temp                       number;
State                      number;

BEGIN

  State := 0;

  if IN_idSnipReq > 0 then
    State := 1;
    update SnippetRetrievalSchedule
      set iLockTime = IN_iReserveKey
      where idSnipReq = IN_idSnipReq
        and iLockTime = 0;
  end if;

  State := 2;

  if IN_iRequestGroup >= 0 then
    State := 3;
    update SnippetRetrievalSchedule
      set iLockTime = IN_iReserveKey
      where tNextAttempt <= IN_tThreshold
        and iLockTime = 0
        and iRequestGroup = IN_iRequestGroup;
  else
    State := 4;
    update SnippetRetrievalSchedule
      set iLockTime = IN_iReserveKey
      where tNextAttempt <= IN_tThreshold
        and iLockTime = 0;
  end if;

  State := 5;

  OUT_RetCode := 0;
  return;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
  	  OUT_RetCode := -2;
    elsif State = 3 then
  	  OUT_RetCode := -3;
    elsif State = 4 then
  	  OUT_RetCode := -4;
    else
  	  OUT_RetCode := -5;
    end if;
  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Reserve_Snippet_Requests',State,Temp);
END;
