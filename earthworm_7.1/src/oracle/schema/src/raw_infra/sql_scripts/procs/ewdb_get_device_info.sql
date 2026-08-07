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


PROCEDURE Get_Device_Info 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION No matching Device record found

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idModule 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME OUT_sDeviceName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME OUT_idDeviceModel 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME OUT_idDeviceType 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 6
PARAM_NAME OUT_bIsRealizable 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 7
PARAM_NAME IN_idDevice 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 8
PARAM_NAME IN_bWantRealizable 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Retrieves the contents of the Device table for 
the given IN_idDevice. If IN_bWantRealizable = 1, then the 
procedure will attempt to check the GenericDevice.bIsRealizable
value, copying -1 into OUT_bIsRealizable if there is no 
corresponding GenericDevice record.

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Get_Device_Info
(
 OUT_RetCode OUT number,
 OUT_idModule OUT number,
 OUT_sDeviceName OUT varchar2,
 OUT_idDeviceModel OUT number,
 OUT_idDeviceType OUT number,
 OUT_bIsRealizable OUT number,
 IN_idDevice number,
 IN_bWantRealizable number
)
as

Temp               number;
State              number;
begin

  State := 0;

  select idModule, sDeviceName, idDeviceModel, idDeviceType 
   into OUT_idModule, OUT_sDeviceName, OUT_idDeviceModel, OUT_idDeviceType
   from Device
   where idDevice = IN_idDevice;


  State := 1;

  if IN_bWantRealizable = 1 then

    State := 2;
    select bIsRealizable into OUT_bIsRealizable
	   from GenericDevice
     where idDevice = IN_idDevice;
  end if;

  State := 3;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      OUT_RetCode := -2;
      /* No Device record found for the idDevice */
    else
      OUT_RetCode := 0;
      OUT_bIsRealizable := -1;
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Device_Info',Temp,State);
    OUT_RetCode := -1;
END;
