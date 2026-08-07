/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT 
TYPE FUNCTION_PROTOTYPE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE-INTERNAL

LANGUAGE SQL

LOCATION THIS_FILE


PROCEDURE Bind_Device_To_Slot 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown Error

RETURN_VALUE -2
RETURN_DESCRIPTION NULL IN_idDeviceToBind.  IN_idDeviceToBind must contain
a valid idDevice value.

RETURN_VALUE -3
RETURN_DESCRIPTION NULL IN_idSlotToBind.  IN_idSlotToBind must contain
a valid idDeviceSlot value.

RETURN_VALUE -4
RETURN_DESCRIPTION Invalid foreign key.  Either IN_idSlotToBind, 
IN_idDeviceToBind, or IN_idContextDevice did not contain valid values.
IN_idContextDevice must correspond to a Device.idDevice
unless it is NULL or 0.

RETURN_VALUE -5
RETURN_DESCRIPTION Overlapping DeviceBind records exist, a IN_bOverwrite
was not set to 1.

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X indicates error
returned by that proc. 

RETURN_VALUE -2X
RETURN_DESCRIPTION Error in Fix_Device_Bind(), where X indicates error returned
by that proc.  Fix_Device_Bind() is only executed when there is an existing 
DeviceBind record with matching keys and an overlapping time period (tOn - tOff).

PARAMETER 1 
PARAM_NAME OUT_RetCode 
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idDeviceBind 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_idDeviceToBind 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_idSlotToBind 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_idContextDevice 
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

PARAMETER 8
PARAM_NAME IN_bOverwrite 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Optional Function Description ended by a blank line. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Bind_Device_To_Slot
(
 OUT_RetCode OUT number,
 OUT_idDeviceBind OUT number,
 IN_idDeviceToBind number,
 IN_idSlotToBind number,
 IN_idContextDevice number,
 IN_tOff number,
 IN_tOn number,
 IN_bOverwrite number
)
as


Temp_idDeviceSlot      number;
Temp_idContextDevice   number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_idDeviceBind      number;
Temp_idDupDeviceBind   number;
                   
begin

  State := 0;
  
  if IN_idDeviceToBind IS NULL or IN_idDeviceToBind = 0 then
    OUT_RetCode := -2;  /* IN_idDeviceToBind is NULL */
    return;
  end if;

  if IN_idSlotToBind IS NULL or IN_idSlotToBind = 0 then
    OUT_RetCode := -3;  /* IN_idSlotToBind is NULL */
    return;
  end if;

  if IN_idContextDevice = 0 then
    Temp_idContextDevice := NULL;
  else
    Temp_idContextDevice := IN_idContextDevice;
  end if;

  State := 1;

  /**********************************/
  /* Get A New idDeviceBind     */
  /**********************************/
  select DeviceBindSeq.NEXTVAL into Temp_idDeviceBind from sys.dual;
 
  Create_Core_idKey(Temp_idDeviceBind);
  if Temp_RetCode <= 0 then
    OUT_RetCode := -10 + Temp_RetCode;
    return;
  end if;

  State := 2;

  select idDeviceBind into Temp_idDupDeviceBind
   from DeviceBind 
   where idDeviceSlot = IN_idSlotToBind
     and (idContextDev = Temp_idContextDevice
          or (    idContextDev IS NULL
              and Temp_idContextDevice IS NULL)
         )
     and tOff > IN_tOn
     and tOn  < IN_tOff;
     
  State := 3;

  /* if we get here, that means there is an overlapping record 
     with idDeviceBind = Temp_idDupDeviceBind 
  *************************************************************/
  if IN_bOverwrite = 1 then
    Fix_Device_Bind(Temp_RetCode, Temp_idDupDeviceBind, IN_tOff, IN_tOn);
    if Temp_RetCode <= 0 then
      OUT_RetCode := -20 + Temp_RetCode;
      return;
    else
      /* we searched for an overlapping DeviceBind record but did not find
         one, so we are clear to insert 
      *************************************************************/
      insert into DeviceBind(idDeviceBind, idContextDev, idDeviceSlot, idDevice, 
                             tOff, tOn)
        values(Temp_idDeviceBind, Temp_idContextDevice, IN_idSlotToBind,
               IN_idDeviceToBind, IN_tOff, IN_tOn);
      OUT_idDeviceBind := Temp_idDeviceBind;
      OUT_RetCode := 0;
    end if;
  else
    OUT_RetCode := -5;
    return;
  end if;
    
  State := 4;
    
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 2 then
      /* we searched for an overlapping DeviceBind record but did not find
         one, so we are clear to insert 
      *************************************************************/
      insert into DeviceBind(idDeviceBind, idContextDev, idDeviceSlot, idDevice, 
                             tOff, tOn)
        values(Temp_idDeviceBind, Temp_idContextDevice, IN_idSlotToBind,
               IN_idDeviceToBind, IN_tOff, IN_tOn);
      OUT_idDeviceBind := Temp_idDeviceBind;
      OUT_RetCode := 0;
    else
      insert into test values('Create_DeviceBind_NDF',Temp,State);
      OUT_RetCode := -1;
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    if SQLCODE = -2291 then
      OUT_RetCode := -3;  /* foreign key problem */
    else
      insert into test values('Create_DeviceBind',Temp,State);
      OUT_RetCode := -1;
    end if;
END;


