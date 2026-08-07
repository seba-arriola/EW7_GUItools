  
  
/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/* Check for an existing Snippet Request or Snippet that matches
   the parameters of the Snippet Request passed in.  We don't want
   any obvious duplicates.
*/

CREATE OR REPLACE FUNCTION Find_Dup_SnipReq_or_Snippet
(OUT_idSnipReq out number,
 OUT_idSnippet out number,
 IN_idChan number,
 IN_tStart number,
 IN_tEnd number,
 IN_idEvent number
)
 RETURN number
as
/* Return Codes:
          0   No match found
         -1   Unknown Error
         -2   Error checking for existing SnipReq
         -3   Error checking for existing Snippet
          1   Warning:  Matching Snipreq already exists
          2   Warning:  Matching Snippet already exists  

          Others:  Undefined
*/
Temp            number;
TempID          number;
begin


  OUT_idSnipReq := 0;
  OUT_idSnippet := 0;
  /**********************************/
  /* Check for existing matching    */
  /* SnipReqs                       */
  /**********************************/
  TempID := Check_For_Matching_SnipReq(IN_idChan, IN_tStart, IN_tEnd, IN_idEvent);

  if TempID = -1 then
    /* error while looking for snipreq.  
     We're hosed, technically speaking.  Return an error. */
    TempID := -2;
    return(TempID);
  elsif(TempID > 0) then
    /* ah we found an existing matching snipreq */
    OUT_idSnipReq := TempID;
    return(1);
  end if;

  /* we didn't find anything */

  /**********************************/
  /* Check for existing matching    */
  /* Snippets                       */
  /**********************************/
  TempID := Check_For_Matching_Snippet(IN_idChan, IN_tStart, IN_tEnd, IN_idEvent);

  if TempID = -1 then
    /* error while looking for snippet.  
     We're hosed, technically speaking.  Return an error. */
    TempID := -3;
    return(TempID);
  elsif(TempID > 0) then
    /* ah we found an existing matching snippet */
    OUT_idSnippet := TempID;
    return(2);
  end if;
 
  /* we didn't find anything */
  return(0); 

EXCEPTION
  /* Warning, we could have foreign key problems here, 
     with the insertion of a record with unchecked 
   idEvent and idChan foreign keys.  Davidk 2000/11/29 
   DK CLEANUP */
  WHEN OTHERS THEN
    /*  ??? */
    Temp := SQLCODE;
    insert into test values('Find_Dup_SnipReq_or_Snippet',Temp,IN_idChan);
    return(-1);
END;


