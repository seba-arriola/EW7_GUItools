/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Waveform_Desc
(OUT_idWaveform out number,
 IN_idChan number,
 IN_tStart number,
 IN_tEnd number,
 IN_iDataFormat number,
 IN_iByteLen number,
 IN_bBindToEvent number,
 IN_idEvent number
)
as


/* Return Codes for OUT_idWaveform:
           >0  DB idEvent
           -1  Unknown Error
           -2  Invalid Input Param (IN_tStart, IN_tEnd, or IN_iByteLen is NULL)
           -1  Invalid Input Param (IN_tStart <= IN_tEnd)
           -1  Invalid Input Param (IN_iByteLen <= 0)
           -7  Create_core_idkey() failed for idWaveform
           -8  IN_bBindToEvent set, and IN_idEvent invalid!
          -1X  Error in Create_Bind(), X describes the error returned
                by Create_Bind()
				  Others:  Undefined
*********************************************************************/

Temp_idWaveform number;
Temp            number;
State           number;
Temp_idEvent    number := IN_idEvent;
Temp_idBind     number;
begin
  State := 0;
  if IN_tStart is null or  IN_tEnd is null or IN_iByteLen is null then 
    OUT_idWaveform := -2;
    return;
  end if;

  State := 10;

  if IN_tStart >= IN_tEnd  then 
    OUT_idWaveform := -3;
    return;
  end if;

  State := 11;

  if IN_iByteLen <= 0  then 
    OUT_idWaveform := -4;
    return;
  end if;

  State := 1;
  if IN_bBindToEvent = 1 then /* if we are binding to an event */
    /**********************************/
    /* Deal With Event Issues         */
    /**********************************/
    Check_idEvent_Validity(Temp_idEvent);
    if Temp_idEvent < 1 then
      OUT_idWaveform := -8;
      return;
    end if;
  end if;  /* if IN_bBindToEvent = 1 */

  State := 2;

  select WaveformSeq.NextVal into Temp_idWaveform from sys.dual;
  Create_Core_idKey(Temp_idWaveform);
  if Temp_idWaveform <= 0 then
    OUT_idWaveform := -7;
  end if;
  State := 3;
  insert into WaveformDesc(idWaveform,idChan,
                           tStart,tEnd,iDataFormat,iByteLen)
    values(Temp_idWaveform,IN_idChan,
           IN_tStart,IN_tEnd,IN_iDataFormat,IN_iByteLen);
  State := 4;

  if IN_bBindToEvent = 1 then
    /**********************************/
    /* Bind our WaveformDesc to the Event   */
    /**********************************/
    Create_Bind(Temp_idBind,Temp_idEvent,'WaveformDesc',Temp_idWaveform);
    if Temp_idBind < 1 then
       OUT_idWaveform := -10 + Temp_idBind;
       return;
    end if;
  end if;

  State := 5;

  OUT_idWaveform := Temp_idWaveform;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Waveform_Desc',State,Temp);
    OUT_idWaveform := -1;
END;





