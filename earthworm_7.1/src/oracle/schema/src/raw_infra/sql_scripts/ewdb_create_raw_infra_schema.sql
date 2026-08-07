/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


/* 
   Create the infrastructure constraints 
******************************************/

/*************************************************/
/* THE FOLLOWING SCRIPTS CROSS REFERENCE RAW INFRA TABLES */

ALTER TABLE DeviceSlot 
 ADD( 
 CONSTRAINT DeviceSlot_idSlotType FOREIGN KEY (idSlotType) 
   REFERENCES SlotType(idSlotType)
);

ALTER TABLE ModuleTemplate 
 ADD( 
 CONSTRAINT ModuleTemplate_idDeviceSlot FOREIGN KEY (idDeviceSlot) 
   REFERENCES DeviceSlot(idDeviceSlot),
 CONSTRAINT ModuleTemplate_idNextDevSlot FOREIGN KEY (idNextDevSlot) 
   REFERENCES DeviceSlot(idDeviceSlot),
 CONSTRAINT ModuleTemplate_idNextPlexor FOREIGN KEY (idNextDevSlot,iNextPlexor) 
   REFERENCES ModuleTemplate(idDeviceSlot,iPlexor)
);


ALTER TABLE ModuleEntry 
 ADD( 
 CONSTRAINT ModuleEntry_idSubDevSlot FOREIGN KEY (idSubDevSlot) 
   REFERENCES DeviceSlot(idDeviceSlot)
);

ALTER TABLE DeviceType 
 ADD( 
 CONSTRAINT DeviceType_idModule FOREIGN KEY (idModule) 
   REFERENCES Module(idModule)
);

ALTER TABLE Device 
 ADD( 
 CONSTRAINT Device_idDeviceModel FOREIGN KEY (idDeviceModel) 
   REFERENCES GenericDevice(idDevice),
 CONSTRAINT Device_idModule FOREIGN KEY (idModule) 
   REFERENCES Module(idModule),
 CONSTRAINT Device_idDeviceType FOREIGN KEY (idDeviceType) 
   REFERENCES DeviceType(idDeviceType)
);


ALTER TABLE ActualDevice 
 ADD( 
 CONSTRAINT ActualDevice_idDevice FOREIGN KEY (idDevice) 
   REFERENCES Device(idDevice)
);


ALTER TABLE GenericDevice 
 ADD( 
 CONSTRAINT GenericDevice_idDevice FOREIGN KEY (idDevice) 
   REFERENCES Device(idDevice)
);


ALTER TABLE DeviceBind 
 ADD( 
 CONSTRAINT DeviceBind_idDeviceSlot FOREIGN KEY (idDeviceSlot) 
   REFERENCES DeviceSlot(idDeviceSlot),
 CONSTRAINT DeviceBind_idDevice FOREIGN KEY (idDevice) 
   REFERENCES Device(idDevice),
 CONSTRAINT DeviceBind_idContextDev FOREIGN KEY (idContextDev) 
   REFERENCES Device(idDevice)
);


ALTER TABLE Module 
 ADD( 
 CONSTRAINT Module_idSlotType FOREIGN KEY (idSlotType) 
   REFERENCES SlotType(idSlotType)
);


/* Physical Inventory Section */
ALTER TABLE RealDevice 
 ADD( 
 CONSTRAINT RealDevice_idActualDevice FOREIGN KEY (idActualDevice) 
   REFERENCES ActualDevice(idDevice)
);


/* Atomic Function Section */
ALTER TABLE FunctionBind 
 ADD( 
 CONSTRAINT FuncBind_idDevice FOREIGN KEY (idDevice) 
   REFERENCES Device(idDevice)
);


ALTER TABLE PolesZeroes 
 ADD( 
 CONSTRAINT PolesZeroes_idFunction FOREIGN KEY (idFunction) 
   REFERENCES PZFilter(idFunction)
);


/* 
   Alter the ChanT table to contain new columns, in case someone
   is running a pre-infrastructure database that did not have 
   those columns in ChanT */
ALTER TABLE ChanT
 ADD( idDeviceslot NUMBER(13,0) NULL, iPlexor NUMBER(3,0) NULL
);



/* 
   Create the infrastructure views 
***************************************/

CREATE OR REPLACE VIEW ALL_FUNCTIONBIND_INFO AS
select fb.*, ewdbt.sTableName sFunctionName
 from FunctionBind fb, EWDB_TableList ewdbt
 where fb.tiFunction = ewdbt.idTable
;


/* 
   Load the infrastructure procedures 
***************************************/

@ewdb_fix_device_bind
/

/* Bind_Device_To_Slot is dependent upon Fix_Device_Bind */
@ewdb_bind_device_to_slot
/

@ewdb_create_moduletemplate
/

/* Create_DeviceSlot is dependent upon Create_ModuleTemplate */
@ewdb_create_deviceslot
/

@ewdb_create_devicetype
/

@ewdb_create_moduleentry
/

/* Create_Module is dependent upon Create_ModuleEntry */
@ewdb_create_module
/

@ewdb_create_or_update_device
/

@ewdb_create_slottype
/

@ewdb_fix_function_bind
/

@ewdb_get_device_info
/

@ewdb_get_deviceslot_info
/

@ewdb_get_devicetype_info
/

@ewdb_get_module_info
/

@ewdb_get_moduleentry
/

@ewdb_get_next_plexor
/

@ewdb_get_plexor_for_chant
/

@ewdb_get_slottype_info
/

@ewdb_set_device_function
/

@ewdb_set_plexor_for_channel
/

