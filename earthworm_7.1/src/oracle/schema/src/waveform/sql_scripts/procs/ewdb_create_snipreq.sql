/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/* Create a new snipreq with trace from channel idChan, of time period tStart - tEnd,
   and tell the snippet retriever to associate any snippet retrieved for this
   request with event idEvent. */

CREATE OR REPLACE PROCEDURE Create_SnipReq
(OUT_RetCode out number,
 OUT_idSnipReq out number,
 IN_idChan number,
 IN_tStart number,
 IN_tEnd number,
 IN_idEvent number,
 IN_iNumAttempts number,
 IN_tInitialRequest number,
 IN_iRequestGroup number
)
as
/* Return Codes for OUT_RetCode:
                  0   Success
                 -1   Unknown Error

				  Others:  Undefined
*/
Temp_idSnipReq  number;

begin

  /**********************************/
  /* Get A New idSnipReq            */
  /**********************************/
  select SnippetRequestSeq.NEXTVAL into Temp_idSnipReq from sys.dual;

  Create_Core_idKey(Temp_idSnipReq);
  if Temp_idSnipReq <= 0 then
    OUT_RetCode := Temp_idSnipReq;
  end if;

  /************************************/
  /* Insert new SnippetRequest Record */
  /************************************/
  insert into SnippetRequest(idSnipReq, tStart, tEnd, idChan, idEvent)
    values(Temp_idSnipReq, IN_tStart, IN_tEnd, IN_idChan, IN_idEvent);

  /************************************/
  /* Create the associated SnippetRetrievalSchedule Record */
  /************************************/
  insert into SnippetRetrievalSchedule(idSnipReq, tNextAttempt, tInitialRequest,
              iNumAttempts, iNumAlreadyAtmptd, 
			  idWaveform, iRetCode, iRequestGroup, iLockTime, sNote)
    values(Temp_idSnipReq, 0/*tNextAttempt*/, IN_tInitialRequest,
	       IN_iNumAttempts, 0/*iNumAlreadyAtmptd*/, 
		   NULL/*idWaveform*/, 0/*iRetCode*/, IN_iRequestGroup,
		   0/*iLockTime*/, ''/*sNote*/);

  /**********************************/
  /* Set the idSnipReq        value */
  /**********************************/
  OUT_idSnipReq := Temp_idSnipReq;

  /**********************************/
  /* Set the RetCode return value   */
  /**********************************/
  OUT_RetCode := 0;

  return;

EXCEPTION
  /* Warning, we could have foreign key problems here, 
     with the insertion of a record with unchecked 
	 idEvent and idChan foreign keys.  Davidk 2000/11/29 
	 DK CLEANUP */
  WHEN OTHERS THEN
    /*  ??? */
	  OUT_RetCode := -1;
END;
