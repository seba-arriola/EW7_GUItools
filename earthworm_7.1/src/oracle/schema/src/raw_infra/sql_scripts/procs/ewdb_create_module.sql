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


PROCEDURE Create_Module 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION N_idSlotType is NULL.  IN_idSlotType must be a 
valid idSlotType.

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Get_SlotType_Info(). Where X 
indicates error returned by that proc.

RETURN_VALUE -2X
RETURN_DESCRIPTION Error in Create_Core_idKey(). Where X 
indicates error returned by that proc.

RETURN_VALUE -3X
RETURN_DESCRIPTION Error in Create_ModuleEntry(). Where X 
indicates error returned by that proc.

PARAMETER 1 
PARAM_NAME OUT_RetCode 
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idModule 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_idSlotType 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_sModuleName 
PARAM_TYPE varchar2 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION This function not only creates a module record, but creates
IN_idSlotType.iNumOutputs ModuleEntry records with idSubDevSlot and
iSubPlexor set to NULL.

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Create_Module
(
 OUT_RetCode OUT number,
 OUT_idModule OUT number,
 IN_idSlotType number,
 IN_sModuleName varchar2
)
as

Temp_idModule          number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_iNumInputs        number;
Temp_iNumOutputs       number;
Temp_Name              varchar2(40);
Temp_Counter           number;
Temp_idModuleEntry  number;
                   
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
  /* Get A New idModule         */
  /**********************************/
  select ModuleSeq.NEXTVAL into Temp_idModule from sys.dual;
 
  Create_Core_idKey(Temp_idModule);
  if Temp_RetCode <= 0 then
    OUT_RetCode := -20 + Temp_RetCode;
    return;
  end if;

  State := 2;

  insert into Module(idModule, idSlotType, sModuleName)
    values(Temp_idModule,IN_idSlotType,IN_sModuleName);

  State := 3;

  OUT_idModule := Temp_idModule;

  for Temp_Counter in 1..Temp_iNumOutputs loop
    Create_ModuleEntry(Temp_RetCode, Temp_idModuleEntry, 
                           Temp_idModule, Temp_Counter, NULL, NULL);
    if Temp_RetCode <= 0 then
      OUT_RetCode := -30 + Temp_RetCode;
      insert into test values('Create_Module_MT',Temp_idModule, Temp_Counter);
      return;
    end if;
  end loop;

  OUT_RetCode := 0;
    

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Module',Temp,State);
    insert into test values('Create_Module2',IN_idSlotType,0);
    OUT_RetCode := -1;
END;


