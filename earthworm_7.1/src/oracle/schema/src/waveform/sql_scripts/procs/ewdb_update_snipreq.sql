/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_SnipReq
(OUT_Retcode out number,
 IN_idSnipReq number,
 IN_tNewStart number,
 IN_tNewEnd number,
 IN_idExistingWaveform number,
 IN_tNextAttempt number,
 IN_bModifyAttemptParams number,
 IN_bModifyResultParams number,
 IN_iAttemptRetCode number,
 IN_iLockCheckTime number,
 IN_bUnlock number,
 IN_sNote  varchar
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
Temp_iLockTime             number;
Temp_tNextAttempt          number;


BEGIN

  State := 0;

  select iLockTime into Temp_iLockTime
    from SnippetRetrievalSchedule
 where idSnipReq = IN_idSnipReq;

  if IN_iLockCheckTime != Temp_iLockTime OR Temp_iLockTime = 0 then
  /* Lock does not match.  By default you can't modify
    something that you haven't locked and you can't
    override an existing lock.
  *****************************************************/
    OUT_RetCode := -6;
    insert into test values('Update_SnipReq-Lk ' || IN_idSnipReq, IN_iLockCheckTime, Temp_iLockTime);
   return;
  end if;

  State := 1;

  update SnippetRetrievalSchedule
    set tNextAttempt = IN_tNextAttempt
    where idSnipReq = IN_idSnipReq;

  State := 2;

  if IN_bModifyResultParams = 1 then
    State := 3;

    update SnippetRequest set tStart=IN_tNewStart, tEnd=IN_tNewEnd
     where idSnipReq=IN_idSnipReq;

    State := 4;

    if IN_idExistingWaveform IS NOT NULL AND IN_idExistingWaveform != 0 then
      update SnippetRetrievalSchedule set idWaveform=IN_idExistingWaveform
       where idSnipReq=IN_idSnipReq;
    end if;

    State := 5;
  end if;

  if IN_bModifyAttemptParams = 1 then

    State := 6;

    update SnippetRetrievalSchedule
      set iNumAlreadyAtmptd=iNumAlreadyAtmptd + 1,
   iRetCode = IN_iAttemptRetCode,
   sNote = IN_sNote
      where idSnipReq=IN_idSnipReq;

    State := 7;

    OUT_RetCode := 0;
  else
    State := 8;
    OUT_RetCode := 0;
  end if;  /* modify attempt params */

  State := 9;

 if IN_bUnlock != 0 then
  update snippetretrievalschedule
   set iLockTime = 0
     where idSnipReq = IN_idSnipReq;
 end if;

 State := 10;
  /* done */


EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 THEN
   OUT_RetCode := -2;
 else
   OUT_RetCode := -3;
      Temp := SQLCODE;
      insert into test values('Update_SnipReq_NDF',State,Temp);
 end if;

  WHEN OTHERS THEN
    OUT_RetCode := -1;
    Temp := SQLCODE;
    insert into test values('Update_SnipReq',State,Temp);
END;

