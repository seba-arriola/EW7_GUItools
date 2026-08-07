/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE IT CVS "EDITED"
 *
 *    $Id: ewdb_create_mwchan.sql,v 1.1 2005/03/23 06:21:17 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_create_mwchan.sql,v $
 *     Revision 1.1  2005/03/23 06:21:17  davidk
 *     Added SQL functions for Mw
 *
 *
 *********************************************************************************
 *********************************************************************************/

CREATE OR REPLACE PROCEDURE Create_MwChan
(
 OUT_RetCode      OUT number,
 OUT_idMwChan     OUT number,
 IN_idMw              number,
 IN_idPick            number,
 IN_idChan            number,
 IN_iOffset           number,
 IN_dMisfit           number,
 IN_idSynthTS         number,
 IN_idRealTS          number
)
as

Temp               number;
Temp_ID            number;
State              number;

BEGIN
  State := 1;

  /**********************************/
  /* Get A New MwChanID           */
  /**********************************/
  select MwChanSeq.NEXTVAL into Temp_ID from sys.dual;

  State := 2;

  Create_Core_idKey(Temp_ID);
  if Temp_ID <= 0 then
    OUT_idMwChan := 0;
    OUT_RetCode := Temp_ID;
    return;
  end if;


  State := 3;

  /**********************************/
  /* Insert new MwChan Record    */
  /**********************************/
  insert into MwChan(idMwChan, idMw, idPick, idChan, iOffset, dMisfit, idSynthTS, idRealTS)
         values     (Temp_ID, IN_idMw, IN_idPick, IN_idChan, IN_iOffset, IN_dMisfit, IN_idSynthTS, IN_idRealTS);

  /**********************************/
  /* Set the  return value          */
  /**********************************/
  State := 4;

  OUT_idMwChan := Temp_ID;
  OUT_RetCode    := 0;

EXCEPTION
  WHEN OTHERS  THEN 
    Temp := SQLCODE;

    Check_Record_Validity(Temp_ID,IN_idMw,'Mw');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -2;
      return;
    end if;
    
    Check_Record_Validity(Temp_ID,IN_idSynthTS,'MwChanTS');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -3;
      return;
    end if;

    Check_Record_Validity(Temp_ID,IN_idRealTS,'MwChanTS');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -4;
      return;
    end if;

    Check_Record_Validity(Temp_ID,IN_idPick,'Pick');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -5;
      return;
    end if;

    Check_Record_Validity(Temp_ID,IN_idChan,'Chan');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -5;
      return;
    end if;

    insert into test values('Create_MwChan ' || IN_idMW, IN_idChan,Temp);
    insert into test values('Create_MwChan2 ' || IN_idSynthTS, IN_idRealTS,IN_idPick);
    OUT_RetCode := -1;
END;

