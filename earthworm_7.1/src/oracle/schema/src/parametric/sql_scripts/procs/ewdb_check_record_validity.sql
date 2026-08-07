/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE PROCEDURE Check_Record_Validity
(OUT_TableID out number,
 IN_RecordID  in number,
 IN_sTableName in varchar
)
as
/* Return Codes for OUT_TableID:
          >0  DB Record ID from TableList
          -1  Unknown Error
				  -3  Invalid core table name
				  -4  Invalid core table id
				  -5  Check Not Implemented for Listed Table
           0  IN_sTableName is NULL, or the IN_RecordID
               is Null!  This may not be an error!
				  Others:  Undefined
*/

State          Number := 0;
Temp_idTable   Number;
Temp_RecordID  Number;
begin


  /**********************************/
  /* Check for NULLs, cause they're */
  /* special                        */
  /**********************************/
  if (IN_sTableName IS NULL) OR (IN_RecordID IS NULL) OR (IN_RecordID = 0) then
    OUT_TableID := 0;
    return;
  end if;

  /**********************************/
  /* Check for Tablename validity   */
  /**********************************/

  select idTable into Temp_idTable from EWDB_Tablelist
    where sTableName = IN_sTableName;

  /**********************************/
  /* Success so far.  Set the return 
  /* Code as a valid TableID, and if
  /* the record ID fails later on, we will
  /* reset it to an error at that time.
  /**********************************/
  OUT_TableID := Temp_idTable;

  State := 1;

  /**********************************/
  /* Check for core record validity   */
  /**********************************/

  /* We have to figure out which table we are looking in, and
     then query it.  We are choosing to do it the brutally painfull
	 way for the programmer, yet the fast and efficient way for the
	 application.  Poor programmer ...  
   **********************************/
   /* 1  CodaAmp
   2  Pick
   3  TCoda
   4  Origin
   5  Magnitude
   6  MechFM
   7  PeakAmp
   8  CodaDur
   9  Chan
   10 Source
   11 Waveform
   12 Ray
   13 Event
   14 Prefer
   15 Bind
   16 Comments
   17 OriginPick
   18 MechFMPick
   19 MagLink
   */

  if Temp_idTable = 1 then
    select idCodaAmp into Temp_RecordID from CodaAmp
      where idCodaAmp = IN_RecordID;
  elsif Temp_idTable = 2 then
    select idPick into Temp_RecordID from Pick
      where idPick = IN_RecordID;
  elsif Temp_idTable = 3 then
    select idTCoda into Temp_RecordID from TCoda
      where idTCoda = IN_RecordID;
  elsif Temp_idTable = 4 then
    select idOrigin into Temp_RecordID from Origin
      where idOrigin = IN_RecordID;
  elsif Temp_idTable = 5 then
    select idMag into Temp_RecordID from Magnitude
      where idMag = IN_RecordID;
  elsif Temp_idTable = 6 then
    select idMechFM into Temp_RecordID from MechFM
      where idMechFM = IN_RecordID;
  elsif Temp_idTable = 7 then
    select idPeakAmp into Temp_RecordID from PeakAmp
      where idPeakAmp = IN_RecordID;
  elsif Temp_idTable = 8 then
    select idCodaDur into Temp_RecordID from CodaDur
      where idCodaDur = IN_RecordID;
  elsif Temp_idTable = 9 then
    select idChan into Temp_RecordID from Chan
      where idChan = IN_RecordID;
  elsif Temp_idTable = 10 then
    select idSource into Temp_RecordID from Source
      where idSource = IN_RecordID;
  elsif Temp_idTable = 11 then
    select idWaveform into Temp_RecordID from Waveform
      where idWaveform = IN_RecordID;
  elsif Temp_idTable = 12 then
    select idRay into Temp_RecordID from Ray
      where idRay = IN_RecordID;
  elsif Temp_idTable = 13 then
    select idEvent into Temp_RecordID from Event
      where idEvent = IN_RecordID;
  elsif Temp_idTable = 14 then
    select idPrefer into Temp_RecordID from Prefer
      where idPrefer = IN_RecordID;
  elsif Temp_idTable = 15 then
    select idBind into Temp_RecordID from Bind
      where idBind = IN_RecordID;
  elsif Temp_idTable = 16 then
    select idComment into Temp_RecordID from Comments
      where idComment = IN_RecordID;
  elsif Temp_idTable = 17 then
    select idOriginPick into Temp_RecordID from OriginPick
      where idOriginPick = IN_RecordID;
  elsif Temp_idTable = 18 then
    select idMechFMPick into Temp_RecordID from MechFMPick
      where idMechFMPick = IN_RecordID;
  elsif Temp_idTable = 19 then
    select idMagLink into Temp_RecordID from MagLink
      where idMagLink = IN_RecordID;
  elsif Temp_idTable = 25 then
    select idExternalEvent into Temp_RecordID from ExternalEvent
      where idExternalEvent = IN_RecordID;
  elsif Temp_idTable = 26 then
    select idWaveform into Temp_RecordID from Waveform
      where idWaveform = IN_RecordID;
  elsif Temp_idTable = 27 then
    select idWaveform into Temp_RecordID from WaveformDesc
      where idWaveform = IN_RecordID;
  elsif Temp_idTable = 41 then
    select idSMMessage into Temp_RecordID from SMMessage
      where idSMMessage = IN_RecordID;
  elsif Temp_idTable = 50 then
    select idCoincidence into Temp_RecordID from CoincidenceEvent
      where idCoincidence = IN_RecordID;
  elsif Temp_idTable = 51 then
    select idTrigger into Temp_RecordID from ChannelTrigger
      where idTrigger = IN_RecordID;
  elsif Temp_idTable = 52 then
    select idMw into Temp_RecordID from Mw
      where idMw = IN_RecordID;
  elsif Temp_idTable = 53 then
    select idMwChan into Temp_RecordID from MwChan
      where idMwChan = IN_RecordID;
  elsif Temp_idTable = 54 then
    select idMwCTS into Temp_RecordID from MwChanTS
      where idMwCTS = IN_RecordID;
  elsif Temp_idTable = 55 then
    select idMwFilter into Temp_RecordID from MwFilter
      where idMwFilter = IN_RecordID;
  else
    OUT_TableID := -5;
  end if;

EXCEPTION
  /**********************************/
  /* Set the appropriate error code 
  /*  for OUT_TableID
  /**********************************/
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
	  /*  Tablename not found in Tablelist */
	  OUT_TableID := -3;
	elsif State = 1 then
	  /*  Record not found in table!*/
	  OUT_TableID := -4;
	else
	  OUT_TableID := -1;
	end if;

  WHEN OTHERS THEN
	OUT_TableID := -1;
END;
