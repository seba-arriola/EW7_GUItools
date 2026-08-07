  
  
/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/* Check for an existing Snippet Request or Snippet that matches
   the parameters of the Snippet Request passed in.  We don't want
   any obvious duplicates.
*/

CREATE OR REPLACE FUNCTION Check_For_Matching_SnipReq
(IN_idChan number,
 IN_tStart number,
 IN_tEnd number,
 IN_idEvent number
)
 RETURN number
as
/* Return Codes for OUT_RetCode:
                 -1   Unknown Error
          0   No matching idSnipReq found
         >0   Matching idSnipReq

          Others:  Undefined
*/
Temp_idSnipReq        number;
Temp                  number;
Temp_ExtensionSeconds number := 5;
Temp_iLockKey         number := 9999;



Cursor SnipReq_cursor(Cursor_idEvent number, Cursor_idChan number, Cursor_tStart number, Cursor_tEnd number) is
  select idSnipReq, tStart, tEnd, idWaveform from all_snippet_requests
   where idEvent = Cursor_idEvent
     and idChan  = Cursor_idChan
   and (   tStart <= Cursor_tStart
        or tEnd   >= Cursor_tEnd);

  Temp_RetCode     number;
  State            number;
  wf_tStart        number;
  wf_tEnd          number; 

begin
  State := 1;

  for Curr in SnipReq_cursor(IN_idEvent, IN_idChan, IN_tStart + Temp_ExtensionSeconds, IN_tEnd - Temp_ExtensionSeconds) loop
    if Curr.idWaveform is NULL or Curr.idWaveform <= 0 then
      if Curr.tStart <= (IN_tStart + Temp_ExtensionSeconds) and 
         Curr.tEnd   >= (IN_tEnd - Temp_ExtensionSeconds) then
        /* we've found something we can use */
        if Curr.tStart <= IN_tStart and Curr.tEnd >= IN_tEnd then
          /* it's a match, we're done. */
          State:= 2;
        else
          /* lock the snipreq */
          Lock_Reserved_Snip_Req(Temp, Curr.idSnipReq, 0, Temp_iLockKey);
          if Temp < 0 then
            /* uh, we couldn't get the lock.  Must already be locked. 
               durr... uh... what do we do now??? 
               DK Cleanup
               let's pretend that the existing request matches the new one,
               and hope (don't hit me...) that it is good enough */
            State:= 3;
          else
            /* we got lock */
            if Curr.tStart > IN_tStart then
              /* update the request.  Make our own backdoor, since we want
             to be efficient and we are only changing the start time. */
             update snippetrequest set tStart = IN_tStart where idSnipReq = Curr.idSnipReq;

            end if;
            if Curr.tEnd < IN_tEnd then
              /* update the request.  Make our own backdoor, since we want
                 to be efficient and we are only changing the start time. */
              update snippetrequest set tEnd = IN_tEnd where idSnipReq = Curr.idSnipReq;
            end if;

            /* release the lock */
            update snippetretrievalschedule
             set iLockTime = 0
             where idSnipReq = Curr.idSnipReq;
          end if;  /* Lock SnipReq failed */
        end if;  /* else (it's a full match) */

        return(Curr.idSnipReq);

      end if;  /* we practically match the current existing snipreq */
    else  /* current snipreq has an existing waveform */
      /* get the descriptor for the existing snippet */
      State := 15;
      select tStart, tEnd into wf_tStart, wf_tEnd
       from waveformdesc 
       where idwaveform = Curr.idWaveform;

      if Curr.tStart <= (IN_tStart + Temp_ExtensionSeconds) or wf_tStart <= IN_tStart then
        if Curr.tEnd >= (IN_tEnd - Temp_ExtensionSeconds)   or wf_tEnd   >= IN_tEnd then
          /* this record will do, check to see if we need to make adjustments. */
          if (Curr.tStart <= IN_tStart or wf_tStart <= IN_tStart) AND
             (Curr.tEnd >= IN_tEnd or wf_tEnd >= IN_tEnd) then
            /* record is fine. do nothing */
            State:= 4;
          else
            /* we need to make adjustments */
            /* lock the snipreq */
            Lock_Reserved_Snip_Req(Temp, Curr.idSnipReq, 0, Temp_iLockKey);
            if Temp < 0 then
              /* uh, we couldn't get the lock.  Must already be locked. 
                 durr... uh... what do we do now??? 
                 DK Cleanup
                 let's pretend that the existing request matches the new one,
                 and hope (don't hit me...) that it is good enough */
              State:= 5;
            else
              /* we got lock */
              if Curr.tStart > IN_tStart and wf_tStart > IN_tStart then
                /* update the request.  Make our own backdoor, since we want
                   to be efficient and we are only changing the start time. */
                update snippetrequest set tStart = IN_tStart where idSnipReq = Curr.idSnipReq;
              end if;
              if Curr.tEnd < IN_tEnd and wf_tEnd < IN_tEnd then
                /* update the request.  Make our own backdoor, since we want
                   to be efficient and we are only changing the start time. */
                update snippetrequest set tEnd = IN_tEnd where idSnipReq = Curr.idSnipReq;
              end if;

              /* release the lock */
              update snippetretrievalschedule
               set iLockTime = 0
               where idSnipReq = Curr.idSnipReq;
            end if;  /* couldn't get lock */
          end if;  /* (else) need to make adjustments */

          return(Curr.idSnipReq);
        end if;  /* (end is ok) we practically match the current existing snipreq */
      end if;  /* start is ok */
    end if;  /* snipreq has waveform */
  end loop;  

  /* no match found */
  return(0);
  

EXCEPTION
  /* Warning, we could have foreign key problems here, 
     with the insertion of a record with unchecked 
   idEvent and idChan foreign keys.  Davidk 2000/11/29 
   DK CLEANUP */
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      /* no matching snipreqs found */
      return(0);
    else 
      insert into test values('Check_For_Matching_SnipReq_NDF',State,IN_idChan);
      return(-1);
    end if;
      
  WHEN OTHERS THEN
    /*  ??? */
    Temp := SQLCODE;
    insert into test values('Check_For_Matching_SnipReq',Temp,IN_idChan);
    return(-1);
END;


