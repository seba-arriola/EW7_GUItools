/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */


/* DK_CLEANUP */

/************************************************  
************ SPECIAL FORMATTED COMMENT **********  
EW API FORMATTED COMMENT 
TYPE FUNCTION_PROTOTYPE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE-INTERNAL

LANGUAGE SQL

LOCATION THIS_FILE


PROCEDURE Get_SlotType_Info 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION No matching SlotType record found

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_iNumInputs 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME OUT_iNumOutputs 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME OUT_sSlotTypeName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_idSlotType 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION This function retrieves the contents of the 
SlotType table for a given idSlotType.

*************************************************  
************************************************/   
CREATE OR REPLACE PROCEDURE Get_SlotType_Info
(
 OUT_RetCode OUT number,
 OUT_iNumInputs OUT number,
 OUT_iNumOutputs OUT number,
 OUT_sSlotTypeName OUT varchar2,
 IN_idSlotType number
)
as

Temp               number;
State              number;
begin

  State := 0;

  select iNumInputs, iNumOutputs, sSlotTypeName
   into OUT_iNumInputs, OUT_iNumOutputs, OUT_sSlotTypeName
   from SlotType
   where idSlotType = IN_idSlotType;

  State := 1;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
    /* No SlotType record found for the idSlotType */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Slot_Type_Info',Temp,IN_idSlotType);
    OUT_RetCode := -1;
END;
