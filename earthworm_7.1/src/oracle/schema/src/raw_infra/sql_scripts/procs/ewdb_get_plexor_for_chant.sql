/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT 
TYPE FUNCTION_PROTOTYPE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE-INTERNAL

LANGUAGE SQL

LOCATION THIS_FILE


PROCEDURE Get_Plexor_For_ChanT 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION No matching ChanT record found

RETURN_VALUE -3
RETURN_DESCRIPTION ChanT record has NULL DeviceSlot

RETURN_VALUE -1X
RETURN_DESCRIPTION GetidChanT_From_idChan_and_T() failed, X is the
return code from it.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idDeviceSlot 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME OUT_iPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_idChanT 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_idChan 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 6
PARAM_NAME IN_tTime 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION This funcntion looks in the ChanT table for the 
idDeviceSlot and iPlexor fields for a given idChanT. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Get_Plexor_For_ChanT
(
 OUT_RetCode OUT number,
 OUT_idDeviceSlot OUT number,
 OUT_iPlexor OUT number,
 IN_idChanT number,
 IN_idChan number,
 IN_tTime number
)
as

Temp               number;
State              number;
Temp_idChanT       number;
Temp_RetCode       number;
begin

  State := 0;

  if IN_idChanT IS NULL OR IN_idChanT = 0 THEN
    State := 1;
    Get_idChanT(Temp_RetCode, Temp_idChanT, IN_idChan, IN_tTime);
    if Temp_RetCode < 0 then
      OUT_RetCode := Temp_RetCode - 10;
      return;
    end if;
  else
    Temp_idChanT := IN_idChanT;
  end if;

  State := 2;

  select idDeviceslot, iPlexor into OUT_idDeviceSlot, OUT_iPlexor 
	  from ChanT
    where idChanT = IN_idChanT;

  State := 3;

  if OUT_idDeviceSlot IS NULL then
    OUT_RetCode := -3;
  else
    OUT_RetCode := 0;
  end if;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
    /* No ChanT record found for the idChanT */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Plexor_For_ChanT',Temp,State);
    OUT_RetCode := -1;
END;
