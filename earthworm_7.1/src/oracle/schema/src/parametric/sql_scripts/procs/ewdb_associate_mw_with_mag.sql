/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE IT CVS "EDITED"
 *
 *    $Id: ewdb_associate_mw_with_mag.sql,v 1.1 2005/03/23 06:21:17 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_associate_mw_with_mag.sql,v $
 *     Revision 1.1  2005/03/23 06:21:17  davidk
 *     Added SQL functions for Mw
 *
 *
 *********************************************************************************
 *********************************************************************************/

CREATE OR REPLACE PROCEDURE Associate_Mw_With_Mag
(
 OUT_RetCode out number,
 IN_idMW         number,
 IN_idMag        number
)
as

Temp               number;
Temp_ID            number;
State              number;

BEGIN

  State := 1;
  update Mw set idMag = IN_idMag 
   where idMw = IN_idMw;


  OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS  THEN 
    Temp := SQLCODE;

    Check_Record_Validity(Temp_ID,IN_idMw,'Mw');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -2;
      return;
    end if;
    
    Check_Record_Validity(Temp_ID,IN_idMag,'Magnitude');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -3;
      return;
    end if;

    insert into test values('Assoc_Mw_Mag ' || IN_idMW, IN_idMag,Temp);
    OUT_RetCode := -1;
END;

