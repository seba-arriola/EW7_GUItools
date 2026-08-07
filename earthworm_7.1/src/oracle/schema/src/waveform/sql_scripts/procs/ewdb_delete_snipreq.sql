/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Delete_SnipReq
(OUT_RetCode out number,
 IN_idSnipReq number,
 IN_iLockCheckTime number
)
as

/*  Delete a SnipReq record, because retrieval of its associated data has 
    been attempted the maximum number of times, or because data has been
	successfully retrieved, or as much data as possible has been retrieved. */


/* RETURN CODES:
       0:     Success
      -1:     Unknown Exception see debug Table
      -2:     SnippetRequest with matching idSnipReq did not exist
               or the LockCheckTime did not match.
********************************************/
Temp           number;
State          number;


BEGIN
  State := 1;
  delete from SnippetRetrievalSchedule 
    where idSnipReq = IN_idSnipReq
      AND (iLockTime = IN_iLockCheckTime  OR
           iLockTime = 0
          );


  State :=2;
  delete from SnippetRequest where idSnipReq = IN_idSnipReq;

  State :=3;
  OUT_RetCode := 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
  WHEN OTHERS THEN
  Temp := SQLCODE;
  insert into test values('Delete_SnipReq '|| IN_idSnipReq,State,Temp);
  insert into test values('Delete_SnipReq2 '|| IN_idSnipReq,0,IN_iLockCheckTime);
  OUT_RetCode := -1;
END;



