/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/* Create a new snipreq with trace from channel idChan, of time period tStart - tEnd,
   and tell the snippet retriever to associate any snippet retrieved for this
   request with event idEvent. */

CREATE OR REPLACE PROCEDURE Find_Or_Create_SnipReq
(OUT_RetCode out number,
 OUT_idSnipReq out number,
 IN_idChan number,
 IN_tStart number,
 IN_tEnd number,
 IN_idEvent number,
 IN_iNumAttempts number,
 IN_tInitialRequest number,
 IN_iRequestGroup number,
 IN_bForce number
)
as
/* Return Codes for OUT_RetCode:
          0   Success
         -1   Unknown Error
         -2   Error checking for existing SnipReq
         -3   Error checking for existing Snippet
         -4   Unexpected return code from find_dup_xxx()
          1   Warning:  Matching Snipreq already exists
          2   Warning:  Matching Snippet already exists  
              OUT_idSnipReq is the ID of the matching SNIPPET.
            The Snippet wass bound to this function by IN_idEvent

          Others:  Undefined
*/
Temp_idSnipReq  number;
Temp_idSnippet  number;
Temp            number;

begin


  if IN_bForce = 0 then
    /**********************************/
    /* Check for existing matching    */
    /* Snippet/SnipReq                */
    /**********************************/
    Temp := Find_Dup_SnipReq_or_Snippet(Temp_idSnipReq, Temp_idSnippet, IN_idChan, IN_tStart, IN_tEnd, IN_idEvent);

    if Temp = -2  then
      /* error while looking for snipreq.  
         We're hosed, technically speaking.  Return an error. */
      OUT_RetCode := -2;
      return;
    elsif Temp = -3  then
      /* error while looking for snippet.  
         We're hosed, technically speaking.  Return an error. */
      OUT_RetCode := -3;
      return;
    elsif Temp = -1 then
      OUT_RetCode := -1;
      return;
    elsif Temp = 1  then
      /* ah we found an existing matching snipreq */
      OUT_idSnipReq := Temp_idSnipReq;
      OUT_RetCode := 1;
      return;
    elsif Temp = 2  then
      /* ah we found an existing matching snippet */
      OUT_idSnipReq := Temp_idSnipReq;
      Create_Bind(Temp,IN_idEvent,'WaveformDesc',Temp_idSnippet);  
      /* we don't care about the retcode, there's nothing we can do about it. */
      OUT_RetCode := 2;
      return;
    elsif Temp != 0 then
      /* ah, an unexpected return code!!! */
      OUT_RetCode := -4;
      return;
    end if;
    /* The only way we get through this if block and drop down into
       the Create_SnipReq() call, is if find_dup_xxx() returns 0,
       meaning (no matching snipreqs/snippets found) */  

  end if;  /* bForce */


  Create_SnipReq(OUT_RetCode, OUT_idSnipReq, IN_idChan, IN_tStart, IN_tEnd, 
                 IN_idEvent, IN_iNumAttempts, IN_tInitialRequest, IN_iRequestGroup );

  return;

  

EXCEPTION
  /* Warning, we could have foreign key problems here, 
     with the insertion of a record with unchecked 
   idEvent and idChan foreign keys.  Davidk 2000/11/29 
   DK CLEANUP */
  WHEN OTHERS THEN
    /*  ??? */
    Temp := SQLCODE;
    OUT_RetCode := -1;
    insert into test values('Find_Or_Create_SnipReq ' || IN_idEvent || ' ' || IN_idChan, Temp_idSnipReq, Temp);
END;
