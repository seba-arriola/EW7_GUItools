/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_waveform.sql,v $
/*     Revision 1.3  2004/09/08 17:28:04  davidk
/*     Reaper v2 09/02/2004.
/*                   */
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/

CREATE OR REPLACE FUNCTION Delete_Waveform
(IN_idWaveform number,
 IN_bForce number
) return number
as
Temp               number:=0;
Temp_tiCore        number;
Temp_iStorageType  number;
Temp_sStorageInfo  varchar(100);
State              number; 

begin
  State := 1;  
  Temp_tiCore := GetTI('WaveformDesc');

  State := 2;  
  delete Bind where idCore = IN_idWaveform and tiCore = Temp_tiCore;

  State := 3;  
  if(IN_bForce != 0) THEN
    delete SnippetRetrievalSchedule where idWaveform = IN_idWaveform;
  end if;
    
  State := 4;  
  select iStorageType,sStorageInfo 
   into Temp_iStorageType, Temp_sStorageInfo
   from WaveformDesc where idWaveform=IN_idWaveform;

  State := 5;  
  Temp := Delete_Actual_Waveform(IN_idWaveform, 
                                  Temp_iStorageType, Temp_sStorageInfo);

  State := 6;  
  delete WaveformDesc where idWaveform=IN_idWaveform;

  return(0);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) THEN  /* Foreign Key */
      insert into test values('Delete_Waveform_FK ' || IN_idWaveform,State, Temp);
      return(1);
    else
      insert into test values('Delete_Waveform ' ||IN_idWaveform, State, Temp);
      return(-1);
    end if;
  END;
