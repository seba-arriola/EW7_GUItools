/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_WaveformDesc
(OUT_RetCode         out number,
 IN_idWaveform           number,
 OUT_idChan          out number,
 OUT_tStart          out number,
 OUT_tEnd            out number,
 OUT_iDataFormat     out number,
 OUT_iByteLen        out number
)
as
/* Return Codes for OUT_RetCode:
           0  Success
          -1  Unknown Error
          -2  idWaveform not found
				  Others:  Undefined
*/
State          Number;
Temp           Number;
begin


  /**********************************/
  /* Check for Magnitude Record     */
  /**********************************/

  State := 1;

  select idChan, tStart, tEnd, iDataFormat, iByteLen into
    OUT_idChan, OUT_tStart, OUT_tEnd, OUT_iDataFormat, OUT_iByteLen
   from ALL_WAVEFORM_DESCRIPTORS 
    where idWaveform = IN_idWaveform;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
      /**********************************
       * idWaveform not Found!!!
       **********************************/
    OUT_RetCode := -2;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_WaveformDesc',IN_idWaveform,Temp);
    OUT_RetCode := -1;
END;


