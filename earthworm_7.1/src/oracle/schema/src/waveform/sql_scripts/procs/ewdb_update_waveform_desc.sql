/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Update_Waveform_Desc
(OUT_RetCode out number,
 IN_idWaveform number,
 IN_tStart number,
 IN_tEnd number,
 IN_iDataFormat number,
 IN_iByteLen number
)
as


/* Return Codes for OUT_idWaveform:
           >0  DB idEvent
           -1  Unknown Error
				  Others:  Undefined
    *Note* IN_iDataFormat is currently ignored!!!
*********************************************************************/

Temp_idWaveform number;
Temp            number;
State           number;
begin
  State := 1;

  update WaveformDesc
    set tStart=IN_tStart, tEnd=IN_tEnd, iByteLen = IN_iByteLen
    where idWaveform = IN_idWaveform;

  State := 2;

  OUT_RetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Update_Waveform_Desc',State,Temp);
    OUT_RetCode := -1;
END;




