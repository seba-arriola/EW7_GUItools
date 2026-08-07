  
  
/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/* Check for an existing Snippet Request or Snippet that matches
   the parameters of the Snippet Request passed in.  We don't want
   any obvious duplicates.
*/

CREATE OR REPLACE FUNCTION Check_For_Matching_Snippet
(IN_idChan number,
 IN_tStart number,
 IN_tEnd number,
 IN_idEvent number
)
 RETURN number
as
/* Return Codes:
         -1   Unknown Error
          0   No matching idSnipReq found
         >0   Matching idSnipReq

          Others:  Undefined
*/
Temp                  number;
Temp_ExtensionSeconds number := 5;

Temp_tStart      number;
Temp_tEnd        number;
Temp_idWaveform  number;
Temp_idSnipReq   number;
State            number;

begin


  State := 0;
  select max(idWaveform) into Temp_idWaveform from waveformdesc 
   where tStart < (IN_tStart + Temp_ExtensionSeconds)
     and tEnd   > (IN_tEnd - Temp_ExtensionSeconds)
     and idChan = IN_idChan;
  /* this could potentially leave us with a hole, where we are double filling
     and thus we give up, when there is an existing snippet that already 
     fulfills all our needs (tStart < IN_tStart and tEnd > IN_tEnd)
   ****************************************************************/

  State := 1;

  if Temp_idWaveform is NULL or Temp_idWaveform <= 0 then 
    return(0);
  end if;

  /* else, we have a waveform we can work with */
  select tStart, tEnd into Temp_tStart, Temp_tEnd 
    from waveformdesc where idwaveform = Temp_idWaveform;

  if Temp_tStart <= IN_tStart and Temp_tEnd >= IN_tEnd then
    /* this record works, ship it */
    return(Temp_idWaveform);

  elsif Temp_tStart > IN_tStart and Temp_tEnd < IN_tEnd then
    /* we'd have to double fill this record to make it work.  no go! */
    return(0);

  elsif Temp_tStart > IN_tStart then
    Create_SnipReq(Temp, Temp_idSnipReq, IN_idChan, IN_tStart, Temp_tStart, IN_idEvent, 0,0,0);
    if Temp >= 0 then
      update SnippetRetrievalSchedule set idWaveform = Temp_idWaveform where idSnipReq = Temp_idSnipReq;
      return(Temp_idWaveform);
    else
      return(-1);
    end if;
  else /* Temp_tEnd < IN_tEnd */
    Create_SnipReq(Temp, Temp_idSnipReq, IN_idChan, Temp_tStart, IN_tEnd, IN_idEvent, 0,0,0);
    if Temp >= 0 then  
      update SnippetRetrievalSchedule set idWaveform = Temp_idWaveform where idSnipReq = Temp_idSnipReq;
      return(Temp_idWaveform);
    else
      return(-1);
    end if;
  end if;

  return(-2);

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      return(0);
    end if;

    Temp := SQLCODE;
    insert into test values('Check_For_Matching_Snippet_NDF',Temp,IN_idChan);
    return(-1);
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Check_For_Matching_Snippet',Temp,IN_idChan);
    return(-1);
END;


