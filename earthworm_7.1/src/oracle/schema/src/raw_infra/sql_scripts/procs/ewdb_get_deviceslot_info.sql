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


PROCEDURE Get_DeviceSlot_Info 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION No matching DeviceSlot record found

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idSlotType 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME OUT_sSlotName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_idDeviceSlot 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Retrieves the contents of the DeviceSlot table for
a given idDeviceSlot.

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Get_DeviceSlot_Info
(
 OUT_RetCode OUT number,
 OUT_idSlotType OUT number,
 OUT_sSlotName OUT varchar2,
 IN_idDeviceSlot number
)
as

Temp               number;
State              number;
begin

  State := 0;

  select idSlotType, sSlotName
   into OUT_idSlotType, OUT_sSlotName
   from DeviceSlot
   where idDeviceSlot = IN_idDeviceSlot;

  State := 1;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
    /* No DeviceSlot record found for the idDeviceSlot */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Device_Slot_Info',Temp,IN_idDeviceSlot);
    OUT_RetCode := -1;
END;
