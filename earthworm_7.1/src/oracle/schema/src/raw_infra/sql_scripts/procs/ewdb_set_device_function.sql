/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/*   1) if idFunctionBind is non NULL(0) then SDF updates the 
        existing FunctionBind record, and ignores the idDevice parameter.
        SDF will check to make sure that this does not override any other 
        records that serve the same time window.  If the time window for
        the new record overwrites any other records, and the IN_bOverwrite
        flag is not set to 1, then the proc will return an error.
     2) if idFunctionBind is NULL, then a new FunctionBind record will
        be created that will overwrite all previous records for the
        same sFunctionType for the same idDevice and 
        overlapping tOn - tOff timeframe.
*/

/* DK_CLEANUP */

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT 
TYPE FUNCTION_PROTOTYPE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE-INTERNAL

LANGUAGE SQL

LOCATION THIS_FILE


PROCEDURE Set_Device_Function 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION NULL IN_idDeviceToBind.  IN_idDeviceToBind
must contain a valid idDevice value.

RETURN_VALUE -3
RETURN_DESCRIPTION NULL IN_idFunctionToBind. IN_idFunctionToBind
must contain a valid idFunction value.

RETURN_VALUE -4
RETURN_DESCRIPTION Invalid sFunctionType/idFunction

RETURN_VALUE -5
RETURN_DESCRIPTION Overlapping FunctionBind records exist, and 
IN_bOverwrite was not set to 1.

RETURN_VALUE -6
RETURN_DESCRIPTION Invalid foreign key.  IN_idDeviceToBind, did
not contain a valid value.

RETURN_VALUE -7
RETURN_DESCRIPTION Invalid idFunctionBind key.  INOUT_idFunctionBind was
non-null but did not contain a valid value. INOUT_idFunctionBind must
correspond to an existing FunctionBind.idFunctionBind if it is not NULL.

RETURN_VALUE -8
RETURN_DESCRIPTION Invalid IN_sFunctionType.  GetTI() was not able to
lookup a table identifier for IN_sFunctionType.

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X indicates 
error returned by that proc.

RETURN_VALUE -2X
RETURN_DESCRIPTION Error in Fix_Function_Bind(), where X indicates
error returned by that proc.  Fix_Function_Bind() is only executed
when there is an existing FunctionBind record with matching keys and
an overlapping time period (tOn - tOff). 

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME INOUT_idFunctionBind 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_idDeviceToBind 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_idFunctionToBind 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 5
PARAM_NAME IN_sFunctionType 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 6
PARAM_NAME IN_tOff 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 7
PARAM_NAME IN_tOn 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 8
PARAM_NAME IN_bOverwrite 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 9
PARAM_NAME IN_bOverridable 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION Optional Function Description ended by a blank line. 

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Set_Device_Function
(
 OUT_RetCode OUT number,
 INOUT_idFunctionBind IN OUT number,
 IN_idDeviceToBind number,
 IN_idFunctionToBind number,
 IN_sFunctionType number,
 IN_tOff number,
 IN_tOn number,
 IN_bOverwrite number,
 IN_bOverridable number
)
as

Temp_idDeviceSlot      number;
Temp_idFunctionBind    number;
State                  number;
Temp                   number;
Temp_RetCode           number;
Temp_idDupFunctionBind number;
Temp_tiFunction        number;
Temp_idFunction        number;   
Temp_bUpdate           number;                
BEGIN
begin

  State := 0;
  
  if IN_idDeviceToBind IS NULL or IN_idDeviceToBind = 0 then
    OUT_RetCode := -2;  /* IN_idDeviceToBind is NULL */
    return;
  end if;

  if IN_idFunctionToBind IS NULL or IN_idFunctionToBind = 0 then
    OUT_RetCode := -3;  /* IN_idFunctionToBind is NULL */
    return;
  end if;

  
  State := 102;

  Check_Record_Validity(Temp_tiFunction, IN_idFunctionToBind, IN_sFunctionType);
  if Temp_tiFunction <= 0 then
    if Temp_tiFunction = -4 then
      OUT_RetCode := -4;
    else
      OUT_RetCode := -8;
    end if;
  end if;

  if INOUT_idFunctionBind = 0  OR INOUT_idFunctionBind IS NULL then
    State := 101;
    /**********************************/
    /* Get A New idFunctionBind     */
    /**********************************/
    select FunctionBindSeq.NEXTVAL into Temp_idFunctionBind from sys.dual;
 
    Create_Core_idKey(Temp_idFunctionBind);
    if Temp_RetCode <= 0 then
      OUT_RetCode := -10 + Temp_RetCode;
      return;
    end if;
    Temp_bUpdate := 1;
  else
    State := 1;
    /* check to make sure that INOUT_idFunctionBind is valid */
    select idFunctionBind into Temp_idFunctionBind 
     from FunctionBind 
     where idFunctionBind = INOUT_idFunctionBind;
     Temp_idFunctionBind := INOUT_idFunctionBind;
    Temp_bUpdate := 0;
  end if;

  State := 2;

  select idFunctionBind into Temp_idDupFunctionBind
   from FunctionBind 
   where idDevice   = IN_idDeviceToBind
     and tiFunction = Temp_tiFunction
     and tOff > IN_tOn
     and tOn  < IN_tOff
     and idFunctionBind != Temp_idFunctionBind;
     /* This is to check for overlapping function binds, but 
        if this is an update then we don't care about the 
        previous version of the record we are updating.  */
     
  State := 3;

  /* if we get here, that means there is an overlapping record 
     with idFunctionBind = Temp_idDupFunctionBind 
  *************************************************************/
  if IN_bOverwrite = 1 then
    Fix_Function_Bind(Temp_RetCode, Temp_idDupFunctionBind, IN_tOff, IN_tOn);
    if Temp_RetCode <= 0 then
      OUT_RetCode := -20 + Temp_RetCode;
      return;
    else
      /* we fixed functionbind so that there are no longer any 
         overlapping records, so now we are clear to insert.
      *************************************************************/
      insert into FunctionBind(idFunctionBind, idDevice, tiFunction, idFunction,
                               tOff, tOn, bOverridable)
        values(Temp_idFunctionBind, IN_idDeviceToBind, Temp_tiFunction, IN_idFunctionToBind,
               IN_tOff, IN_tOn, IN_bOverridable);
      INOUT_idFunctionBind := Temp_idFunctionBind;
      OUT_RetCode := 0;
      return;
    end if;
  else
    OUT_RetCode := -5;
    return;
  end if;
    
  State := 4;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      /* INOUT_idFunctionBind was not NULL, but we didn't find a corresponding
         record in the FunctionBind table. */
      OUT_RetCode := -7;
      return;
    elsif State = 2 then
      /* we searched for an overlapping FunctionBind record but did not find
         one, so we are clear to insert or update
      *************************************************************/
      if Temp_bUpdate = 1 then
        update FunctionBind 
         set idFunction=IN_idFunctionToBind, tOff=IN_tOff, tOn=IN_tOn, 
             bOverridable=IN_bOverridable
         where idFunctionBind = Temp_idFunctionBind;
      else
        insert into FunctionBind(idFunctionBind, idDevice, tiFunction, idFunction,
                                 tOff, tOn, bOverridable)
         values(Temp_idFunctionBind, IN_idDeviceToBind, Temp_tiFunction, IN_idFunctionToBind,
                IN_tOff, IN_tOn, IN_bOverridable);
      end if;
      INOUT_idFunctionBind := Temp_idFunctionBind;
      OUT_RetCode := 0;
    else
      insert into test values('Create_FunctionBind_NDF',Temp,State);
      insert into test values('Create_FunctionBind_NDF2',IN_idDeviceToBind, IN_idFunctionToBind);
      OUT_RetCode := -1;
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    if SQLCODE = -2291 then
      insert into test values('Create_FunctionBind_FK',Temp,State);
      insert into test values('Create_FunctionBind_FK2',IN_idDeviceToBind, IN_idFunctionToBind);
      OUT_RetCode := -6;  /* idDevice foreign key problem */
    else
      insert into test values('Create_FunctionBind',Temp,State);
      insert into test values('Create_FunctionBind2',IN_idDeviceToBind, IN_idFunctionToBind);
      OUT_RetCode := -1;
    end if;
END;
EXCEPTION    
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_FunctionBind_EX',Temp,State);
    insert into test values('Create_FunctionBind_EX2',IN_idDeviceToBind, IN_idFunctionToBind);
    OUT_RetCode := -1;
END;


