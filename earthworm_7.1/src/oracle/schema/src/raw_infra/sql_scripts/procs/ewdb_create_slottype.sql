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


PROCEDURE Create_SlotType 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION Invalid iNumInputs (must be > 0)

RETURN_VALUE -3
RETURN_DESCRIPTION Invalid iNumOutputs (must be > 0)

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X 
indicates error returned by that proc.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idSlotType 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_iNumInputs 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_iNumOutputs 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_sSlotTypeName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Optional Function Description ended by a blank line. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Create_SlotType
(
 OUT_RetCode OUT number,
 OUT_idSlotType OUT number,
 IN_iNumInputs number,
 IN_iNumOutputs number,
 IN_sSlotTypeName varchar2
)
as

Temp_idDeviceSlot      number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_idSlotType  number;
                   
begin

  State := 0;
  
  if IN_iNumInputs IS NULL or IN_iNumInputs <= 0 then
    OUT_RetCode := -2;  /* IN_iNumInputs is invalid */
    return;
  end if;

  if IN_iNumOutputs IS NULL or IN_iNumOutputs <= 0 then
    OUT_RetCode := -3;  /* IN_iNumOutputs is invalid */
    return;
  end if;

  State := 1;

  /**********************************/
  /* Get A New idSlotType     */
  /**********************************/
  select SlotTypeSeq.NEXTVAL into Temp_idSlotType from sys.dual;
 
  Create_Core_idKey(Temp_idSlotType);
  if Temp_RetCode <= 0 then
    OUT_RetCode := -10 + Temp_RetCode;
    return;
  end if;

  State := 2;

  insert into SlotType(idSlotType, iNumInputs, iNumOutputs, sSlotTypeName)
    values(Temp_idSlotType,IN_iNumInputs, IN_iNumOutputs, IN_sSlotTypeName);

  State := 3;

  OUT_idSlotType := Temp_idSlotType;
  OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_SlotType',Temp,State);
    OUT_RetCode := -1;
END;


