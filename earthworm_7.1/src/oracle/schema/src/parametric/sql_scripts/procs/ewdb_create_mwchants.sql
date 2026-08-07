/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE IT CVS "EDITED"
 *
 *    $Id: ewdb_create_mwchants.sql,v 1.2 2005/03/24 18:19:05 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_create_mwchants.sql,v $
 *     Revision 1.2  2005/03/24 18:19:05  davidk
 *     Added 2nd time series to MwChanTS to overcome 2000 byte string limitation
 *     in OCI 7.3.
 *
 *     Revision 1.1  2005/03/23 06:21:17  davidk
 *     Added SQL functions for Mw
 *
 *
 *********************************************************************************
 *********************************************************************************/

CREATE OR REPLACE PROCEDURE Create_MwChanTS
(
 OUT_RetCode      OUT number,
 OUT_idMwCTS      OUT number,
 IN_dTSStart          number,
 IN_dTSFreq           number,
 IN_iTSLen            number,
 IN_iType             number,
 IN_idMwFilter        number,
 IN_idChan            number,
 IN_sTS1              varchar2,
 IN_sTS2              varchar2
)
as

Temp               number;
Temp_ID            number;
State              number;

BEGIN
  State := 1;

  /**********************************/
  /* Get A New MwChanTSID           */
  /**********************************/
  select MwChanTSSeq.NEXTVAL into Temp_ID from sys.dual;

  State := 2;

  Create_Core_idKey(Temp_ID);
  if Temp_ID <= 0 then
    OUT_idMwCTS := 0;
    OUT_RetCode := Temp_ID;
    return;
  end if;


  State := 3;

  /**********************************/
  /* Insert new MwChanTS Record    */
  /**********************************/
  insert into MwChanTS(idMwCTS, dTSStart, dTSFreq, iTSLen, iType, idMwFilter, idChan, sTS1, sTS2)
         values     (Temp_ID, IN_dTSStart, IN_dTSFreq, IN_iTSLen, IN_iType, IN_idMwFilter, IN_idChan, IN_sTS1, IN_sTS2);

  /**********************************/
  /* Set the  return value          */
  /**********************************/
  State := 4;

  OUT_idMwCTS    := Temp_ID;
  OUT_RetCode    := 0;

EXCEPTION
  WHEN OTHERS  THEN 
    Temp := SQLCODE;

    Check_Record_Validity(Temp_ID,IN_idChan,'Chan');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -2;
      return;
    end if;

    Check_Record_Validity(Temp_ID,IN_idMwFilter,'MwFilter');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -3;
      return;
    end if;
    
    insert into test values('Create_MwChanTS ' || IN_dTSStart, IN_idChan,Temp);
    OUT_RetCode := -1;
END;

