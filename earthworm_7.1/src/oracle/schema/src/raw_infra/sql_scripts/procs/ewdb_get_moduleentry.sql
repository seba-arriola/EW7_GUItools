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


PROCEDURE Get_ModuleEntry 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION No matching ModuleEntry record found.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idSubDevSlot 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME OUT_iSubPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_idModule 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_iPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Retrieves the contents of the ModuleEntry table 
for a given idModule and iPlexor.

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Get_ModuleEntry
(
 OUT_RetCode OUT number,
 OUT_idSubDevSlot OUT number,
 OUT_iSubPlexor OUT number,
 IN_idModule number,
 IN_iPlexor number
)
as

Temp               number;
State              number;

begin


  State := 0;

  select idSubDevSlot, iSubPlexor
   into  OUT_idSubDevSlot, OUT_iSubPlexor
   from ModuleEntry
   where idModule = IN_idModule
     and iPlexor  = IN_iPlexor;

  State := 1;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
      /* No ModuleEntry record found for the idModule/iPlexor */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Module_Info',Temp,State);
    insert into test values('Get_Module_Info2',IN_idModule,IN_iPlexor);
    OUT_RetCode := -1;
END;
