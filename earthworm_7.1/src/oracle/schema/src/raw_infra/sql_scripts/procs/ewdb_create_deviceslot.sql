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


PROCEDURE Create_DeviceSlot
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0  
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown Error 

RETURN_VALUE -2
RETURN_DESCRIPTION IN_idSlotType is NULL.  IN_idSlotType must be a valid idSlotType. 

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Get_SlotType_Info(), where X indicates error returned by that procedure.

RETURN_VALUE -2X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X indicates error returned by that procedure.

RETURN_VALUE -3X
RETURN_DESCRIPTION Error in Create_ModuleTemplate(), where X indicates error returned by that procedure.

PARAMETER 1 
PARAM_NAME OUT_RetCode 
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return code for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idDeviceSlot
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3 
PARAM_NAME IN_idSlotType
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4 
PARAM_NAME IN_sSlotName
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Optional Function Description ended by a blank line. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Create_DeviceSlot
(
 OUT_RetCode OUT number,
 OUT_idDeviceSlot OUT number,
 IN_idSlotType number,
 IN_sSlotName varchar2
)
as
Temp_idDeviceSlot      number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_iNumInputs        number;
Temp_iNumOutputs       number;
Temp_Name              varchar2(40);
Temp_Counter           number;
Temp_idModuleTemplate  number;
                   
begin

  State := 0;
  
  if IN_idSlotType IS NULL or IN_idSlotType = 0 then
    OUT_RetCode := -2;  /* IN_idSlotType is NULL */
    return;
  end if;

  State := 1;

  /**********************************/
  /* Get the SlotType record info   */
  /**********************************/
  Get_SlotType_Info(Temp_RetCode, Temp_iNumInputs, Temp_iNumOutputs, 
                     Temp_Name, IN_idSlotType);
  if Temp_RetCode <= 0 then
    OUT_RetCode := -10 + Temp_RetCode;
    return;
  end if;

  State := 2;

  /**********************************/
  /* Get A New idDeviceSlot         */
  /**********************************/
  select DeviceSlotSeq.NEXTVAL into Temp_idDeviceSlot from sys.dual;
 
  Create_Core_idKey(Temp_idDeviceSlot);
  if Temp_RetCode <= 0 then
    OUT_RetCode := -20 + Temp_RetCode;
    return;
  end if;

  State := 2;

  insert into DeviceSlot(idDeviceSlot, idSlotType, sSlotName)
    values(Temp_idDeviceSlot,IN_idSlotType,IN_sSlotName);

  State := 3;

  OUT_idDeviceSlot := Temp_idDeviceSlot;

  for Temp_Counter in 1..Temp_iNumOutputs loop

    Create_ModuleTemplate(Temp_RetCode, Temp_idModuleTemplate, 
                           Temp_idDeviceSlot, Temp_Counter, NULL, NULL);
    if Temp_RetCode <= 0 then
      OUT_RetCode := -30 + Temp_RetCode;
      insert into test values('Create_DeviceSlot_MT',Temp_idDeviceSlot, Temp_Counter);
      return;
    end if;
  end loop;

  OUT_RetCode := 0;
    

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_DeviceSlot',Temp,State);
    insert into test values('Create_DeviceSlot2',IN_idSlotType,0);
    OUT_RetCode := -1;
END;


