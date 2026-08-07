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

PROCEDURE Get_Module_Info 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION No matching Module record found

RETURN_VALUE -1X
RETURN_DESCRIPTION Get_SlotType_Info() failed returning -X.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idSlotType 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME OUT_sModuleName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME OUT_sSlotTypeName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME OUT_iNumInputs 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 6
PARAM_NAME OUT_iNumOutputs 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 7
PARAM_NAME IN_idModule 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Retrieves the contents of the Module table and 
associated SlotType table for the given IN_idModule.

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Get_Module_Info
(
 OUT_RetCode OUT number,
 OUT_idSlotType OUT number,
 OUT_sModuleName OUT varchar2,
 OUT_sSlotTypeName OUT varchar2,
 OUT_iNumInputs OUT number,
 OUT_iNumOutputs OUT number,
 IN_idModule number
)
as

Temp               number;
State              number;
Temp_idSlotType    number;
Temp_RetCode       number;

begin


  State := 0;

  select sModuleName, idSlotType
   into  OUT_sModuleName, Temp_idSlotType
   from Module
   where idModule = IN_idModule;

  OUT_idSlotType := Temp_idSlotType;

  State := 1;

  Get_SlotType_Info(Temp_RetCode, OUT_iNumInputs, OUT_iNumOutputs, OUT_sSlotTypeName, Temp_idSlotType);
 
  if Temp_RetCode < 0 then
    State := 2;
    OUT_RetCode := Temp_RetCode - 10;
  else
    OUT_RetCode := 0;
  end if;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
      /* No Module record found for the idModule */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Module_Info',Temp,State);
    insert into test values('Get_Module_Info2',IN_idModule,Temp_idSlotType);
    OUT_RetCode := -1;
END;
