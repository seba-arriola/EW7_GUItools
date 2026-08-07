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


PROCEDURE Create_Or_Update_Device 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION INOUT_idDevice is non-NULL, but invalid.  
The caller must set INOUT_idDevice to NULL(0) before calling
this proc to create a new device.

RETURN_VALUE -3
RETURN_DESCRIPTION Violated a Foreign Key constraint on idDeviceModel,
idModule, or idDeviceType during insert of new Device record.

RETURN_VALUE -4
RETURN_DESCRIPTION Violated a Foreign Key constraint on idDeviceModel,
idModule, or idDeviceType during update of Device record.

RETURN_VALUE -5
RETURN_DESCRIPTION Internal idDevice error.
                  
RETURN_VALUE -7
RETURN_DESCRIPTION IN_idDeviceType is non-NULL, but invalid.

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X indicates
error returned by that proc.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME INOUT_idDevice 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_idDeviceModel 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_sDeviceName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_idDeviceType 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 6
PARAM_NAME IN_bIsGeneric 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 7
PARAM_NAME IN_bIsRealizable 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION This function either attempts to create a new Device record
or update an existing Device record.  

NOTE When INOUT_idDevice is 0 or NULL:
  The proc will attempt to create a new Device record using the params 
  passed.  If IN_bIsGeneric = 1 then it will also attempt to create
  a GenericDevice with matching idDevice and bIsRealizable set to
  IN_bIsRealizable; otherwise (IN_bisGeneric != 1) it will attempt to
  create an ActualDevice with matching idDevice.
  When INOUT_idDevice != 0 and NOT NULL:
  The proc will attempt to update the existing Device record with 
  idDevice = INOUT_idDevice, using the params passed.  If IN_bIsGeneric = 1
  then the proc will attempt to update the GenericDevice record with 
  matching idDevice.  If a matching GenericDevice record does not exist,
  (meaning the Device was originally created as an ActualDevice not a 
  GenericDevice) then GenericDevice will not be updated and the proc will
  still return success. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Create_Or_Update_Device
(
 OUT_RetCode OUT number,
 INOUT_idDevice OUT number,
 IN_idDeviceModel number,
 IN_sDeviceName varchar2,
 IN_idDeviceType number,
 IN_bIsGeneric number,
 IN_bIsRealizable number
)
as

Temp_idDevice          number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_bCreateNewDevice  number;  
Temp_idModule          number;                   
begin

  State := 0;

  if INOUT_idDevice IS NULL or INOUT_idDevice = 0 then
    Temp_bCreateNewDevice := 1;
  else
    Temp_bCreateNewDevice := 0;
  end if;

  if IN_idDeviceType IS NULL or IN_idDeviceType = 0 then
    Temp_idModule := NULL;
  else
    select idModule into Temp_idModule 
      from DeviceType
      where idDeviceType = IN_idDeviceType;
  end if;

  State := 1;

  if Temp_bCreateNewDevice = 1 then

    State := 2;

    /**********************************/
    /* Get A New idDevice             */
    /**********************************/
    select DeviceSeq.NEXTVAL into Temp_idDevice from sys.dual;
 
    Create_Core_idKey(Temp_idDevice);
    if Temp_RetCode <= 0 then
      OUT_RetCode := -10 + Temp_RetCode;
      return;
    end if;

    State := 3;

    insert into Device(idDevice, idDeviceModel, sDeviceName, 
                       idDeviceType, idModule)
      values(Temp_idDevice, IN_idDeviceModel, IN_sDeviceName, 
             IN_idDeviceType, Temp_idModule);

    State := 4;

    if IN_bIsGeneric = 1 then
      State := 5;
      insert into GenericDevice(idDevice, bIsRealizable)
        values (Temp_idDevice, IN_bIsRealizable);
    else
      State := 6;
      insert into ActualDevice(idDevice)
        values (Temp_idDevice);
    end if;

    INOUT_idDevice := Temp_idDevice;
  else

    State := 7;
    /* we are doing an update */

    select idDevice into Temp_idDevice 
     from Device
     where idDevice = INOUT_idDevice;

    State := 8;
    update Device set idDeviceModel = IN_idDeviceModel,
                      sDeviceName   = IN_sDeviceName,
                      idDeviceType  = IN_idDeviceType,
                      idModule      = Temp_idModule
     where idDevice = INOUT_idDevice;

    State := 9;
    if IN_bIsGeneric = 1 then
      update GenericDevice set bIsRealizable = IN_bIsRealizable;
    end if;
  end if;

  State := 10;

  OUT_RetCode := 0;

EXCEPTION
  
  WHEN NO_DATA_FOUND THEN
    if State = 7 then
      OUT_RetCode := -2;

    elsif State = 0 then
      OUT_RetCode := -7;

    else
      Temp := SQLCODE;
      insert into test values('Create_ModuleTemplate_NDF',Temp,State);
      OUT_RetCode := -1;
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    if SQLCODE = -2291 then  /* foreign key constraint */
      if State = 3 then
        OUT_RetCode := -3;
        /* this is a new device record issue */

      elsif State >= 5 and State <= 6 then
        OUT_RetCode := -5;
        /* internal error, Device inserted properly, but now generic/actual
           device can't find idDevice FK in Device table */

      elsif State = 8 then
        OUT_RetCode := -4;
        /* this is a device record update issue */

      else
        insert into test values('Create_ModuleTemplate_FK',Temp,State);
        OUT_RetCode := -1;
      end if;
    else
      insert into test values('Create_ModuleTemplate',Temp,State);
      OUT_RetCode := -1;
    end if;
END;


