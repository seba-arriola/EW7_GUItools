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


PROCEDURE Create_DeviceType 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

PROTOTYPE_LOCATION 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION Null IN_idModule (NULL or 0)

RETURN_VALUE -3
RETURN_DESCRIPTION Foreign Key violation, idModule does not 
correspond to a matching Module record.

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X 
indicates error returned by that proc.

PARAMETER 1 
PARAM_NAME OUT_RetCode 
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idDeviceType 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_idModule 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_sDeviceTypeName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Optional Function Description ended by a blank line. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Create_DeviceType
(
 OUT_RetCode OUT number,
 OUT_idDeviceType OUT number,
 IN_idModule number,
 IN_sDeviceTypeName varchar2
)
as

Temp_idDeviceType      number;
State                  number;
Temp                   number;
Temp_RetCode           number;
                   
begin

  State := 0;
  
  if IN_idModule IS NULL or IN_idModule <= 0 then
    OUT_RetCode := -2;  /* IN_idModule is invalid */
    return;
  end if;

  State := 1;

  /**********************************/
  /* Get A New idDeviceType     */
  /**********************************/
  select DeviceTypeSeq.NEXTVAL into Temp_idDeviceType from sys.dual;
 
  Create_Core_idKey(Temp_idDeviceType);
  if Temp_RetCode <= 0 then
    OUT_RetCode := -10 + Temp_RetCode;
    return;
  end if;

  State := 2;

  insert into DeviceType(idDeviceType, idModule, sDeviceTypeName)
    values(Temp_idDeviceType, IN_idModule, IN_sDeviceTypeName);

  State := 3;

  OUT_idDeviceType := Temp_idDeviceType;
  OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if SQLCODE = -2291 then
      OUT_RetCode := -3;  /* foreign key problem */
    else
      insert into test values('Create_DeviceType',Temp,State);
      OUT_RetCode := -1;
    end if;
END;


