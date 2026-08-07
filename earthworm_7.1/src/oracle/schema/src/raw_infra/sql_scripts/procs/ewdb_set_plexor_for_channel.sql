/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

/*     1) if idChanT is VALID then that ChanT record will be updated
          with the new idDeviceSlot and iPlexor.
       2) if idChanT is NOT valid (NOT NULL and != 0) then, 
          a) if there are existing ChanT records with matching 
             idChan and overlapping timerange (tOn - tOff), 
             then those records will be updated with the new
             idDeviceSlot and iPlexor.
          b) else (no matching records), a new ChanT record will be 
             created, with the new idDeviceSlot and iPlexor
*****************************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT 
TYPE FUNCTION_PROTOTYPE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE-INTERNAL

LANGUAGE SQL

LOCATION THIS_FILE


PROCEDURE Set_Plexor_For_Channel 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION NULL IN_idDeviceSlot.  IN_idDeviceSlot must 
contain a valid idDeviceSlot value

RETURN_VALUE -3
RETURN_DESCRIPTION NULL IN_iPlexor.  IN_iPlexor must contain
a valid iPlexor value.

RETURN_VALUE -4
RETURN_DESCRIPTION Unknown Duplicate Index Value error.

RETURN_VALUE -5
RETURN_DESCRIPTION Internal error while processing Create_ChanT.

RETURN_VALUE -6
RETURN_DESCRIPTION Unknown Foreign Key error.

RETURN_VALUE -7
RETURN_DESCRIPTION idDeviceSlot/iPlexor is not valid FK to 
ModuleTemplate table.
      
RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X 
indicates error returned by that proc.

RETURN_VALUE -2X
RETURN_DESCRIPTION Error in Update_ChanT(), where X indicates
error returned by that proc.

RETURN_VALUE -1XX
RETURN_DESCRIPTION Error in Create_ChanT(), where XX indicates
error returned by that proc.

RETURN_VALUE -2XX
RETURN_DESCRIPTION Error in Update_ChanTs_For_TimeRange(), where
XX indicates error returned by that proc.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME INOUT_idChanT 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_idChan 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_idDeviceSlot 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_iPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 6
PARAM_NAME IN_tOff 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 7
PARAM_NAME IN_tOn 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Optional Function Description ended by a blank line. 

*************************************************
************************************************/
CREATE OR REPLACE PROCEDURE Set_Plexor_For_Channel
(
 OUT_RetCode OUT number,
 INOUT_idChanT IN OUT number,
 IN_idChan number,
 IN_idDeviceSlot number,
 IN_iPlexor number,
 IN_tOff number,
 IN_tOn number
)
as

Temp_idDeviceSlot      number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_idModuleTemplate  number;
Temp_iChanT_Count      number;
Temp_idChanT           number;
                   
begin

  State := 0;
  
  if IN_idDeviceSlot IS NULL or IN_idDeviceSlot = 0 then
    OUT_RetCode := -2;  /* IN_idDeviceSlot is NULL */
    return;
  end if;

  if IN_iPlexor IS NULL or IN_iPlexor = 0 then
    OUT_RetCode := -3;  /* IN_iPlexor is NULL */
    return;
  end if;

  State := 1;

  select idModuleTemplate into Temp_idModuleTemplate
   from ModuleTemplate
   where idDeviceSlot = IN_idDeviceSlot
     and iPlexor = IN_iPlexor;

  State := 2;

  if INOUT_idChanT IS NULL or INOUT_idChanT = 0 then
    State := 3;

    select count(idChanT) into Temp_iChanT_Count
     from ChanT
     where idChan = IN_idChan
       and tOff   > IN_tOn
       and tOn    < IN_tOff;

    if Temp_iChanT_Count > 0 then
      State := 4;

      /* update all of the ChanT records that fit the time range */
      Update_ChanTs_For_TimeRange(Temp_RetCode, IN_tOff, IN_tOn, IN_idChan, 
                                  NULL /*IN_idCompT*/, IN_idDeviceSlot, IN_iPlexor, 
                                  NULL /* sComment*/, 0 /*bForce*/);
      if Temp_RetCode < 0 then
        OUT_RetCode := -200 + Temp_RetCode;
        return;
      else
        State := 5;

        select min(idChanT) into INOUT_idChanT 
         from ChanT
         where idChan = IN_idChan
           and tOff   > IN_tOn
           and tOn    < IN_tOff;
      end if;
    else  /*no records exist in that time window */
      State := 6;

      /* create a new ChanT record to fit the time window */
      Create_ChanT(Temp_RetCode, Temp_idChanT, IN_idChan, IN_tOn, IN_tOff,
                   NULL /*idCompT*/, IN_idDeviceSlot, IN_iPlexor, 
                   NULL /*sComment*/, 0/* bForce shouldn't matter here*/);
      if Temp_RetCode != 0 then
        if Temp_RetCode < 0 then
          OUT_RetCode := -100 + Temp_RetCode;
          return;
        else
          /* Create_ChanT return 1, warning that there were overlapping
             records, but we didn't find overlapping records, so issue
             an error log, and return an internal error 
          *************************************************************/
          insert into test values('Set_Plexor_For_Channel_CCT',IN_idChan,State);
          insert into test values('Set_Plexor_For_Channel_CCT2',IN_tOn,IN_tOff);
          OUT_RetCode := -5;
          return;
        end if;
      end if;  /* Create_ChanT() returned an error or warning */

      State := 7;

      INOUT_idChanT := Temp_idChanT;
    end if;
  else
    State := 8;

    /* We have a valid(we think) idChanT, so just do a simple update. */
    Update_ChanT(Temp_RetCode, INOUT_idChanT, NULL, IN_idDeviceSlot, IN_iPlexor, NULL, 0);
    if Temp_RetCode < 0 then
      OUT_RetCode := -20 + Temp_RetCode;
      return;
    end if;
  end if;

  State := 9;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      OUT_RetCode := -7;
    else
      Temp := SQLCODE;
      insert into test values('Set_Plexor_For_Channel_NDF',Temp,State);
      insert into test values('Set_Plexor_For_Channel_NDF',IN_tOn, IN_tOff);
      insert into test values('Set_Plexor_For_Channel_NDF',IN_idDeviceSlot, IN_iPlexor);
      insert into test values('Set_Plexor_For_Channel_NDF',IN_idChan, 0);
      OUT_RetCode := -1;
    end if;

  WHEN DUP_VAL_ON_INDEX THEN
    OUT_RetCode := -4;  
    Temp := SQLCODE;
    insert into test values('Set_Plexor_For_Channel_DVOI',Temp,State);
    insert into test values('Set_Plexor_For_Channel_DVOI2',IN_tOn, IN_tOff);
    insert into test values('Set_Plexor_For_Channel_DVOI3',IN_idDeviceSlot, IN_iPlexor);
    insert into test values('Set_Plexor_For_Channel_DVOI4',IN_idChan, 0);

  WHEN OTHERS THEN
    Temp := SQLCODE;
    if SQLCODE = -2291 then
      insert into test values('Set_Plexor_For_Channel_FK',Temp,State);
      insert into test values('Set_Plexor_For_Channel_FK',IN_tOn, IN_tOff);
      insert into test values('Set_Plexor_For_Channel_FK',IN_idDeviceSlot, IN_iPlexor);
      insert into test values('Set_Plexor_For_Channel_FK',IN_idChan, 0);
      OUT_RetCode := -3;  /* foreign key problem */
    else
      insert into test values('Set_Plexor_For_Channel',Temp,State);
      insert into test values('Set_Plexor_For_Channel',IN_tOn, IN_tOff);
      insert into test values('Set_Plexor_For_Channel',IN_idDeviceSlot, IN_iPlexor);
      insert into test values('Set_Plexor_For_Channel',IN_idChan, 0);
      OUT_RetCode := -1;
    end if;
END;


