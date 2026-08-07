/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_event.sql,v $
/*     Revision 1.7  2005/05/16 20:59:24  mark
/*     Added comment deletion
/*
/*     Revision 1.6  2005/01/12 22:05:33  davidk
/*     Updated based on bugs and problems found in hydra_reaper.
/*
/*     Revision 1.5  2004/09/07 19:31:14  davidk
/*     Reaper v2 09/02/2004.
/*                      */
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/

CREATE OR REPLACE PROCEDURE Delete_Event
(OUT_RetCode OUT number,
 IN_idEvent      number,
 IN_bDeleteWaveformDataOnly number
)
as

Temp_tiCore  number;


Cursor Bind_Cursor(Cursor_idEvent number, Cursor_tiCore number) is
  select * from bind where idEvent=Cursor_idEvent and tiCore = Cursor_tiCore;

Cursor SnipReq_Cursor(Cursor_idEvent number) is
  select idSnipReq from SnippetRequest where idEvent=Cursor_idEvent;


  Temp             number := 0;
  Temp_RetCode     number := 0;
  State            number;
  idPOrigin        number;
  idPMag           number;
  Temp_idComment   number;


begin
  State := 1;

  /* delete snippet requests */
  for Curr in SnipReq_Cursor(IN_idEvent) loop
    Temp:=Delete_SnippetRequest(Curr.idSnipReq);
    if(Temp < 0) THEN
      insert into test values('Delete_Event2: DeleteSnipReq', IN_idEvent, Temp);
      OUT_RetCode := -2;
      return;
    elsif(Temp > 0) then
      Temp_RetCode := Temp_RetCode + 1;
    end if;
  end loop;

  /* delete waveforms(snippets) */
  State := 2;
  Temp_tiCore := getti('WaveformDesc');
  for Curr in bind_Cursor(IN_idEvent, Temp_tiCore) loop
    Temp:=Delete_Waveform(Curr.idCore, 0/* Force */);
    if(Temp < 0) THEN
      insert into test values('Delete_Event2: DeleteWaveform', IN_idEvent, Temp);
      OUT_RetCode := -3;
      return;
    elsif(Temp > 0) then
      Temp_RetCode := Temp_RetCode + 1;
    end if;
  end loop;

  State := 3;
  if(IN_bDeleteWaveformDataOnly != 0)  THEN
    OUT_RetCode := 0;
    return;
  end if;

  State := 4;
  Temp := Delete_Alarms_By_Event(IN_idEvent);
  if(Temp < 0) THEN
    insert into test values('Delete_Event2: DeleteAlarms', IN_idEvent, Temp);
    OUT_RetCode := -4;
    return;
  elsif(Temp > 0) then
    Temp_RetCode := Temp_RetCode + 1;
  end if;
    
  State := 5;
  delete Prefer where idEvent = IN_idEvent;

  /* delete bound magnitudes */
  State := 6;
  Temp_tiCore := getti('Magnitude');
  for Curr in Bind_Cursor(IN_idEvent,Temp_tiCore) loop
    Temp:=Delete_Magnitude(Curr.idCore);
    if(Temp < 0) THEN
      insert into test values('Delete_Event2: Del_Mag ' || IN_idEvent, Curr.idCore, Temp);
      OUT_RetCode := -6;
      return;
    elsif(Temp > 0) then
      Temp_RetCode := Temp_RetCode + 1;
    end if;
  end loop;

  /* delete bound mechanisms */
  State := 7;
  /* nothing MechFM to delete */


  /* delete bound origins */
  State := 8;
  Temp_tiCore := getti('Origin');
  for Curr in Bind_Cursor(IN_idEvent,Temp_tiCore) loop
    Temp:=Delete_Origin(Curr.idCore);
    if(Temp < 0) THEN
      insert into test values('Delete_Event2: Del_Origin ' || IN_idEvent, Curr.idCore, Temp);
      OUT_RetCode := -5;
      return;
    elsif(Temp > 0) then
      Temp_RetCode := Temp_RetCode + 1;
    end if;
  end loop;

  /* delete any comments associated with this event */
  State := 9;
  select idComment into Temp_idComment from Event
    where idEvent = IN_idEvent;
  if (Temp_idComment IS NOT NULL AND Temp_idComment > 0) then
    delete Comments where idComment = Temp_idComment;
  end if;

  State := 10;
  select idInternalComment into Temp_idComment from Event
    where idEvent = IN_idEvent;
  if (Temp_idComment IS NOT NULL AND Temp_idComment > 0) then
    delete Comments where idComment = Temp_idComment;
  end if;

  State := 11;
  select idContribMagComment into Temp_idComment from Event
    where idEvent = IN_idEvent;
  if (Temp_idComment IS NOT NULL AND Temp_idComment > 0) then
    delete Comments where idComment = Temp_idComment;
  end if;

  /* delete external events */
  State := 12;
  Temp_tiCore := getti('ExternalEvent');
  for Curr in Bind_Cursor(IN_idEvent,Temp_tiCore) loop
    Delete ExternalEvent where idExternalEvent = Curr.idCore;
  end loop;

  /* Delete Lock */
  State := 14;
  Delete EventLock where idEvent=IN_idEvent;
  
  /* delete Event bonds */
  State := 15;
  Delete Bind where idEvent = IN_idEvent;

  /* delete Event */
  State := 16;
  Delete Event where idEvent = IN_idEvent;

  OUT_RetCode := Temp_RetCode;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) then
      insert into test values('Delete_Event_FK ' || IN_idEvent, Temp,State);
      OUT_RetCode := 1;
      return;
    else
      insert into test values('Delete_Event_Exc ' || IN_idEvent, Temp,State); 
      OUT_RetCode := -1;
    end if;
END;

