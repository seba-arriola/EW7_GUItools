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


PROCEDURE Get_Next_Plexor 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION No matching ModuleTemplate record found.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idNextDevSlot 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME OUT_iNextPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_idDeviceSlot 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_iPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Retrieves the idNextDeviceSlot and iNextPlexor 
from the ModuleTemplate table using the given IN_idDeviceSlot
and IN_iPlexor

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Get_Next_Plexor
(
 OUT_RetCode OUT number,
 OUT_idNextDevSlot OUT number,
 OUT_iNextPlexor OUT number,
 IN_idDeviceSlot number,
 IN_iPlexor number
)
as

Temp               number;
State              number;
begin

  State := 0;

  select idNextDevSlot, iNextPlexor 
   into OUT_idNextDevSlot, OUT_iNextPlexor 
   from ModuleTemplate
   where idDeviceSlot = IN_idDeviceSlot 
     and iPlexor      = IN_iPlexor;


  State := 1;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
    /* No ModuleTemplate record found for the idDeviceSlot/iPlexor */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Next_Plexor',Temp,State);
    insert into test values('Get_Next_Plexor2',IN_idDeviceSlot,IN_iPlexor);
    OUT_RetCode := -1;
END;
