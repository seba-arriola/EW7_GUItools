/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE IT CVS "EDITED"
 *
 *    $Id: ewdb_create_mw.sql,v 1.4 2005/06/14 16:55:10 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_create_mw.sql,v $
 *     Revision 1.4  2005/06/14 16:55:10  davidk
 *     Added a lot of debug listings for unexpected Exceptions in Create_Mw()
 *
 *     Revision 1.3  2005/03/31 18:53:35  davidk
 *     Changed the EWDB_MwStruct:
 *       Added dM0 (doh!)
 *       Changed dPercentCVLD to dPercentCLVD
 *
 *     Revision 1.2  2005/03/24 18:18:41  davidk
 *     Fixed bug were IN_idMag was being used in the insert statement, when Temp_idMag
 *     should be used to handle cases where 0 is converted to NULL.
 *
 *     Revision 1.1  2005/03/23 06:21:17  davidk
 *     Added SQL functions for Mw
 *
 *
 *********************************************************************************
 *********************************************************************************/

CREATE OR REPLACE PROCEDURE Create_Mw
(
 OUT_RetCode      OUT number,
 OUT_idMw         OUT number,
 IN_dMxx              number,
 IN_dMxy              number,
 IN_dMxz              number,
 IN_dMyy              number,
 IN_dMyz              number,
 IN_dMzz              number,
 IN_dM0               number,
 IN_iScalarExp        number,
 IN_dPFPStrike        number,
 IN_dPFPDip           number,
 IN_dPFPRake          number,
 IN_dAFPStrike        number,
 IN_dAFPDip           number,
 IN_dAFPRake          number,
 IN_dDepth            number,
 IN_idOrigin          number,
 IN_idMag             number,
 IN_iNumStations      number,
 IN_dMisfit           number,
 IN_dPercentDC        number,
 IN_dPercentCLVD      number,
 IN_idMwFilter        number
)
as

Temp               number;
Temp_ID            number;
Temp_idMag         number;
State              number;

BEGIN
  State := 1;

  /**********************************/
  /* Get A New MwID           */
  /**********************************/
  select MwSeq.NEXTVAL into Temp_ID from sys.dual;

  State := 2;

  Create_Core_idKey(Temp_ID);
  if Temp_ID <= 0 then
    OUT_idMw := 0;
    OUT_RetCode := Temp_ID;
    return;
  end if;

  if(IN_idMag = 0) THEN 
    Temp_idMag := NULL;
  else 
    Temp_idMag := IN_idMag;
  end if;

  State := 3;

  /**********************************/
  /* Insert new Mw Record    */
  /**********************************/
  insert into Mw(idMw, dMxx, dMxy, dMxz, dMyy, dMyz, dMzz, dM0, iScalarExp,
                 dPFPStrike, dPFPDip, dPFPRake, dAFPStrike, dAFPDip, dAFPRake, 
                 dDepth, idOrigin, idMag, iNumStations, dMisfit, dPercentDC, dPercentCLVD, 
                 idMwFilter)
         values  (Temp_ID, IN_dMxx, IN_dMxy, IN_dMxz, IN_dMyy, IN_dMyz, IN_dMzz, IN_dM0, IN_iScalarExp,
                  IN_dPFPStrike, IN_dPFPDip, IN_dPFPRake, IN_dAFPStrike, IN_dAFPDip, IN_dAFPRake, 
                  IN_dDepth, IN_idOrigin, Temp_idMag, IN_iNumStations, IN_dMisfit, IN_dPercentDC, 
                  IN_dPercentCLVD, IN_idMwFilter);

  /**********************************/
  /* Set the  return value          */
  /**********************************/
  State := 4;

  OUT_idMw       := Temp_ID;
  OUT_RetCode    := 0;

EXCEPTION
  WHEN OTHERS  THEN 
    Temp := SQLCODE;

    Check_Record_Validity(Temp_ID,IN_idOrigin,'Origin');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -2;
      return;
    end if;

    if(Temp_idMag != 0) THEN
      Check_Record_Validity(Temp_ID,IN_idMag,'Magnitude');
      if not(Temp_ID > 0) then
        /* error, set return code and quit */
        OUT_RetCode := -3;
        return;
      end if;
    end if;
    
    Check_Record_Validity(Temp_ID,IN_idMwFilter,'MwFilter');
    if not(Temp_ID > 0) then
      /* error, set return code and quit */
      OUT_RetCode := -4;
      return;
    end if;

    if(IN_dM0 = 0) THEN
      /* error, set return code and quit */
      OUT_RetCode := -5;
      return;
    end if;
    
    insert into test values('Create_Mw ' || IN_idOrigin, IN_idMag,Temp);

    insert into test values('Create_Mw1 ' || IN_dMxx, IN_idOrigin,Temp);
    insert into test values('Create_Mw2 ' || IN_dMxy, IN_idOrigin,Temp);
    insert into test values('Create_Mw3 ' || IN_dMxz, IN_idOrigin,Temp);
    insert into test values('Create_Mw4 ' || IN_dMyy, IN_idOrigin,Temp);
    insert into test values('Create_Mw5 ' || IN_dMyz, IN_idOrigin,Temp);
    insert into test values('Create_Mw6 ' || IN_dMzz, IN_idOrigin,Temp);
    insert into test values('Create_Mw7 ' || IN_dM0, IN_idOrigin,Temp);
    insert into test values('Create_Mw8 ' || IN_iScalarExp, IN_idOrigin,Temp);
    insert into test values('Create_Mw9 ' || IN_dPFPStrike, IN_idOrigin,Temp);
    insert into test values('Create_Mw10 ' || IN_dPFPDip, IN_idOrigin,Temp);
    insert into test values('Create_Mw11 ' || IN_dPFPRake, IN_idOrigin,Temp);
    insert into test values('Create_Mw12 ' || IN_dAFPStrike, IN_idOrigin,Temp);
    insert into test values('Create_Mw13 ' || IN_dAFPDip, IN_idOrigin,Temp);
    insert into test values('Create_Mw14 ' || IN_dAFPRake, IN_idOrigin,Temp);
    insert into test values('Create_Mw15 ' || IN_dDepth, IN_idOrigin,Temp);
    insert into test values('Create_Mw16 ' || IN_dMisfit, IN_idOrigin,Temp);
    insert into test values('Create_Mw17 ' || IN_dPercentDC, IN_idOrigin,Temp);
    insert into test values('Create_Mw18 ' || IN_dPercentCLVD, IN_idOrigin,Temp);
    insert into test values('Create_Mw19 ' || IN_iNumStations, IN_idOrigin,Temp);
    insert into test values('Create_Mw20 ' || IN_dMxx, IN_idOrigin,Temp);
    insert into test values('Create_Mw21 ' || IN_dMxx, IN_idOrigin,Temp);
    insert into test values('Create_Mw22 ' || IN_dMxx, IN_idOrigin,Temp);
    OUT_RetCode := -1;
END;

