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


PROCEDURE Fix_Device_Bind 
RETURN_PARAMETER 1 

SOURCE_LOCATION THIS_FILE 

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION Error, an idChan and idCompT already exist for
the given SCNL and time interval

RETURN_VALUE -1X
RETURN_DESCRIPTION Error in Create_Core_idKey(), where X 
indicates error returned by that proc.

PARAMETER 1 
PARAM_NAME OUT_RetCode
PARAM_TYPE number 
PARAM_DESCRIPTION This parameter holds the return value for the procedure.

PARAMETER 2 
PARAM_NAME IN_idDupDeviceBind 
PARAM_TYPE number  
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 3
PARAM_NAME IN_tOff 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

PARAMETER 4
PARAM_NAME IN_tOn 
PARAM_TYPE number 
PARAM_DESCRIPTION Optional Parameter Description for Param2. 

DESCRIPTION this function should modify neccessary reords so that 
DeviceBind.idDupDeviceBind is not valid over the time range of 
tOn - tOff.  We expect that this means that we must split the 
record in half, with the first half occurring before tOn and
the second half occurring after tOff

************************************************* 
************************************************/ 
CREATE OR REPLACE PROCEDURE Fix_Device_Bind
(
 OUT_RetCode OUT number,
 IN_idDupDeviceBind number,
 IN_tOff number,
 IN_tOn number
)
as

State                  number;
Temp                   number;
Temp_RetCode           number;

Temp_idContextDevice   number;
Temp_idDeviceSlot      number;
Temp_idDevice          number;
Temp_tOn               number;
Temp_tOff              number;

Temp_idDeviceBind      number;

Temp_bRecordUpdated    number;

                   
begin

  State := 0;

  select idContextDev, idDeviceSlot, idDevice, 
         tOn, tOff 
   into  Temp_idContextDevice, Temp_idDeviceSlot, Temp_idDevice, 
         Temp_tOn, Temp_tOff
   from  DeviceBind
   where idDeviceBind = IN_idDupDeviceBind;

  State := 1;

  if Temp_tOn < IN_tOn then
    update DeviceBind set tOff = IN_tOn 
     where idDeviceBind = IN_idDupDeviceBind;
    Temp_bRecordUpdated := 1;
  end if;

  State := 2;

  if Temp_tOff > IN_tOff then
    if Temp_bRecordUpdated = 1 then
      State := 3;

      /**********************************/
      /* Get A New idDeviceBind     */
      /**********************************/
      select DeviceBindSeq.NEXTVAL into Temp_idDeviceBind from sys.dual;
     
      Create_Core_idKey(Temp_idDeviceBind);
      if Temp_RetCode <= 0 then
        OUT_RetCode := -10 + Temp_RetCode;
        return;
      end if;

      State := 4;

      insert into DeviceBind(idDeviceBind, idContextDev, idDeviceSlot, 
                             idDevice, tOff, tOn)
        values(Temp_idDeviceBind, Temp_idContextDevice, Temp_idDeviceSlot, 
               Temp_idDevice, Temp_tOff, IN_tOff);
    else
      State := 5;

      update DeviceBind set tOn = IN_tOff 
       where idDeviceBind = IN_idDupDeviceBind;
      Temp_bRecordUpdated := 1;
    end if;
  end if;

  State := 6;

  if Temp_bRecordUpdated = 0 then
    State := 7;

    delete DeviceBind where idDeviceBind = IN_idDupDeviceBind;
  end if;

  State := 8;

  OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Fix_Device_Bind',Temp,State);
    insert into test values('Fix_Device_Bind2',IN_tOff, IN_tOn);
    insert into test values('Fix_Device_Bind3',IN_idDupDeviceBind, 0);
    OUT_RetCode := -1;
END;


