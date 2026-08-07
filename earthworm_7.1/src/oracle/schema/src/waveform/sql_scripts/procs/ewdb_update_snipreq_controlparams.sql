/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_SnipReqControlParams
(OUT_Retcode out number,
 IN_idSnipReq number,
 IN_iLockCheckTime number,
 IN_iRequestGroup number,
 IN_iNumAttempts number
)
 as


/*  update a SnipReq record so that it has a new start or end time.  Optionally
  tell it the idWaveform of a snippet that stores partial data for this request,
  so that new data can be combined with it.  Set iModifyParams to true
  if you wish to have the update procedure modify the associated 
  SnippetRetrievalSchedule record. */



/* RETURN CODES:
       0:     Success
      -1:     Unknown Exception while updating SnipReq (see Debug Table)
      -2:     SnippetRequest not found for given idSnipReq.
      -3:     Unknown NDF Exception while retrieving SnippetRetrievalSchedule record
      -6:     iLockTime does not match.  Record not properly locked, or locked
	          by someone else.

********************************************/
Temp                       number;
Temp_RetCode               number;
State                      number;

Temp_iNumAttempts          number;
Temp_tNextAttempt          number;
Temp_iLockTime             number;

BEGIN

  State := 0;

  select iLockTime into Temp_iLockTime
    from SnippetRetrievalSchedule
	where idSnipReq = IN_idSnipReq;

  if IN_iLockCheckTime != Temp_iLockTime then 
    /* Lock does not match.  By default you can't modify
	   something that you haven't locked and you can't
	   override an existing lock.
	 *****************************************************/
    OUT_RetCode := -6;
	return;
  end if;

  State := 1;

  if IN_iRequestGroup >= 0 then
    update SnippetRetrievalSchedule
      set iRequestGroup = IN_iRequestGroup
      where idSnipReq = IN_idSnipReq;
  end if;

  State := 2;

  if IN_iNumAttempts >= 0 then
    update SnippetRetrievalSchedule
      set iNumAttempts = IN_iNumAttempts
      where idSnipReq = IN_idSnipReq;
  end if;

  State := 3;

  /* done */
  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 THEN
	  OUT_RetCode := -2;
	else
	  OUT_RetCode := -3;
      Temp := SQLCODE;
      insert into test values('Update_SnipReqControlParams_NDF',State,Temp);
	end if;

  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Update_SnipReqControlParams',State,Temp);
END;
