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


PROCEDURE Create_ModuleEntry 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION NULL IN_idModule.  IN_idModule must contain
a valid idModule value.

RETURN_VALUE -3
RETURN_DESCRIPTION Invalid foreign key.  Either IN_idModule, 
IN_idSubDevSlot, or the combo of IN_idSubDevSlot and IN_iSubPlexor 
did not contain valid values.

RETURN_VALUE -4
RETURN_DESCRIPTION ModuleEntry record already exists with duplicate
IN_idModule, IN_iPlexor.

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X 
indicates error returned by that proc.

PARAMETER 1 
PARAM_NAME OUT_RetCode 
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME OUT_idModuleEntry 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_idModule 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_iPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_idSubDevSlot 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 6
PARAM_NAME IN_iSubPlexor 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Optional Function Description ended by a blank line. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Create_ModuleEntry
(
 OUT_RetCode OUT number,
 OUT_idModuleEntry OUT number,
 IN_idModule number,
 IN_iPlexor number,
 IN_idSubDevSlot number,
 IN_iSubPlexor number
)
as

Temp_idModule      number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_idModuleEntry  number;
                   
begin

  State := 0;
  
  if IN_idModule IS NULL or IN_idModule = 0 then
    OUT_RetCode := -2;  /* IN_idModule is NULL */
    return;
  end if;

  State := 1;

  /**********************************/
  /* Get A New idModuleEntry     */
  /**********************************/
  select ModuleEntrySeq.NEXTVAL into Temp_idModuleEntry from sys.dual;
 
  Create_Core_idKey(Temp_idModuleEntry);
  if Temp_RetCode <= 0 then
    OUT_RetCode := -10 + Temp_RetCode;
    return;
  end if;

  State := 2;

  insert into ModuleEntry(idModuleEntry, idModule, iPlexor, 
                            idSubDevSlot, iSubPlexor)
    values(Temp_idModuleEntry,IN_idModule, IN_iPlexor, 
           IN_idSubDevSlot, IN_iSubPlexor);

  State := 3;

  OUT_idModuleEntry := Temp_idModuleEntry;
  OUT_RetCode := 0;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    OUT_RetCode := -4;  /* duplicate idModule/iPlexor */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    if SQLCODE = -2291 then
      OUT_RetCode := -3;  /* foreign key problem */
    else
      insert into test values('Create_ModuleEntry',Temp,State);
      OUT_RetCode := -1;
    end if;
END;


