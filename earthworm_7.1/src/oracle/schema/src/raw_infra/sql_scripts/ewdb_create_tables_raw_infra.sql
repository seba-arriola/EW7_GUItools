/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */
/*
  Create Raw Infrastructure Tables */

CREATE TABLE DeviceSlot(idDeviceSlot NUMBER(13,0) NULL,
 idSlotType NUMBER(13,0) NOT NULL, sSlotName VARCHAR2(40) NULL,
 CONSTRAINT idDeviceSlot PRIMARY KEY (idDeviceSlot)
)  TABLESPACE EWDB_INFRASPACE;



CREATE TABLE ModuleTemplate(idModuleTemplate NUMBER(13,0) NULL,
 idDeviceSlot NUMBER(13,0) NOT NULL, iPlexor NUMBER(3,0) NOT NULL, 
 idNextDevSlot NUMBER(13,0) NULL, iNextPlexor NUMBER(3,0) NULL, 
 CONSTRAINT idModuleTemplate PRIMARY KEY (idModuleTemplate),
 CONSTRAINT ModuleTemplate_UK
   UNIQUE  (idDeviceSlot,iPlexor)
)  TABLESPACE EWDB_INFRASPACE;



CREATE TABLE ModuleEntry(idModuleEntry NUMBER(13,0) NULL,
 idModule NUMBER(13,0) NULL, iPlexor NUMBER(3,0) NULL, 
 idSubDevSlot NUMBER(13,0) NULL, iSubPlexor NUMBER(3,0) NULL, 
 CONSTRAINT idModuleEntry PRIMARY KEY (idModuleEntry),
 CONSTRAINT ModuleEntry_UK
   UNIQUE  (idModule,iPlexor)
)  TABLESPACE EWDB_INFRASPACE;

/* 
   We cannot use the id*DeviceSlot/i*Plexor combo as a foreign
   key on the PlexorSlot table, because we have allowed for the
   existence of virtual PlexorSlots that are not listed in the
   PlexorSlot table, but are referenced in ChanT.  We have to 
   settle for having idNextDeviceSlot and idSubDeviceSlot as
   referential integrity checks for the DeviceSlot records.
************************************************/


CREATE TABLE DeviceType(idDeviceType NUMBER(13,0) NULL,
 sDeviceTypeName VARCHAR2(40) NULL, idModule NUMBER(13,0) NULL,
 CONSTRAINT idDeviceType PRIMARY KEY (idDeviceType)
)  TABLESPACE EWDB_INFRASPACE;


CREATE TABLE Device(idDevice NUMBER(13,0) NULL,
  idDeviceModel NUMBER(13,0) NULL, idModule NUMBER(13,0) NULL,
  sDeviceName VARCHAR2(40) NULL, idDeviceType NUMBER(13,0) NULL, 
 CONSTRAINT idDevice PRIMARY KEY (idDevice)
)  TABLESPACE EWDB_INFRASPACE;


CREATE TABLE ActualDevice(idDevice NUMBER(13,0) NULL,
 CONSTRAINT idActualDevice PRIMARY KEY (idDevice)
)  TABLESPACE EWDB_INFRASPACE;

CREATE TABLE GenericDevice(idDevice NUMBER(13,0) NULL,
 bIsRealizable CHAR(1) NULL,
 CONSTRAINT idGenericDevice PRIMARY KEY (idDevice)
)  TABLESPACE EWDB_INFRASPACE;

CREATE TABLE DeviceBind(idDeviceBind NUMBER(13,0) NOT NULL,
  idContextDev NUMBER(13,0) NOT NULL, idDeviceSlot NUMBER(13,0) NOT NULL,
  tOff NUMBER(14,4) NULL, tOn NUMBER(14,4) NULL, 
  idDevice NUMBER(13,0) NOT NULL,
 CONSTRAINT idDeviceBind PRIMARY KEY (idDeviceBind),
 CONSTRAINT DeviceBind_UK UNIQUE (idContextDev, idDeviceSlot, toff)
 )  TABLESPACE EWDB_INFRASPACE;


CREATE TABLE SlotType(idSlotType NUMBER(13,0) NOT NULL,
  iNumInputs NUMBER(3,0) NOT NULL, iNumOutputs NUMBER(3,0) NOT NULL,
  sSlotTypeName VARCHAR2(40) NULL,
 CONSTRAINT idSlotType PRIMARY KEY (idSlotType)
)   TABLESPACE EWDB_INFRASPACE;

CREATE TABLE Module(idModule NUMBER(13,0) NOT NULL,
  idSlotType NUMBER(13,0) NOT NULL, sModuleName VARCHAR2(40) NULL,
 CONSTRAINT idModule PRIMARY KEY (idModule)
)   TABLESPACE EWDB_INFRASPACE;


CREATE TABLE FunctionBind(idFunctionBind NUMBER(13,0) NOT NULL,
  idDevice NUMBER(13,0) NOT NULL,
  tiFunction NUMBER(13,0) NULL, idFunction NUMBER(13,0) NULL,
  tOff NUMBER(14,4) NULL, tOn NUMBER(14,4) NULL, 
  bOverridable CHAR(1) NULL,
 CONSTRAINT idFunctionBind PRIMARY KEY (idFunctionBind),
 CONSTRAINT FuncBind_UK 
   UNIQUE (idDevice, tiFunction, idFunction, tOff)
)   TABLESPACE EWDB_INFRASPACE;


CREATE TABLE RealDevice(idActualDevice NUMBER(13,0) NOT NULL,
  sSerialNum VARCHAR2(20) NOT NULL, idManufacturer NUMBER(13,0) NOT NULL, 
 CONSTRAINT RealDevice_pk PRIMARY KEY (idActualDevice),
 CONSTRAINT RealDevice_Serial_Manufact
   UNIQUE(sSerialNum, idManufacturer)
)  TABLESPACE EWDB_INFRASPACE;




CREATE TABLE DeviceLocation(idDeviceLocation NUMBER(13,0) NOT NULL,
  idActualDevice NUMBER(13,0) NOT NULL, idLocation NUMBER(13,0) NOT NULL,
  tOff NUMBER(14,4) NULL, tOn NUMBER(14,4) NULL, 
 CONSTRAINT idDeviceLocation PRIMARY KEY (idDeviceLocation),
 CONSTRAINT DeviceLocation_unique
   UNIQUE(idActualDevice, idLocation, tOff)
)  TABLESPACE EWDB_INFRASPACE;


CREATE TABLE Location(idLocation NUMBER(13,0) NOT NULL,
  dLat NUMBER(6,3) NULL, dLon NUMBER(6,3) NULL, 
  dElev NUMBER(6,1) NULL,
  sLocationName VARCHAR2(80) NULL,
  sLocationDescription VARCHAR2(255) NULL,
 CONSTRAINT idLocation PRIMARY KEY (idLocation),
 CONSTRAINT Location_sLocationName
   UNIQUE(sLocationName)
)  TABLESPACE EWDB_INFRASPACE;


CREATE TABLE Manufacturer(idManufacturer NUMBER(13,0) NOT NULL,
  sManufacturerName  VARCHAR2(80) NULL,
  sServiceRepName  VARCHAR2(50) NULL,
  sServiceRepPhone VARCHAR2(20) NULL,
  CONSTRAINT idManufacturer PRIMARY KEY (idManufacturer),
  CONSTRAINT Manufacturer_UK UNIQUE (sManufacturerName)
 )  TABLESPACE EWDB_INFRASPACE;


CREATE TABLE PZFilter (idFunction NUMBER(13,0) NULL, 
  sFilterName varchar(50) NULL,
 CONSTRAINT idFunction PRIMARY KEY (idFunction)
)  TABLESPACE EWDB_INFRASPACE;


CREATE TABLE PolesZeroes (idPolesZeroes NUMBER(13,0) NULL,
  idFunction NUMBER(13,0) NULL, cPZType CHAR(1) NULL, 
  dReal NUMBER(14,7) NULL, dImaginary NUMBER(14,7) NULL,  
 CONSTRAINT idPolesZeroes PRIMARY KEY (idPolesZeroes)  
)  TABLESPACE EWDB_INFRASPACE;

/* Don't know if this is still needed */
CREATE TABLE UnitType(iUnitType NUMBER(4,0) NULL,
 sUnitTypeName VARCHAR2(20) NULL, sUnitTypeDescription VARCHAR2(80) NULL,
 CONSTRAINT iUnitType PRIMARY KEY (iUnitType)
)  TABLESPACE EWDB_INFRASPACE;


/* Create Sequences */
/* Raw Infrastructure schema */
Create Sequence DeviceSlotSeq      START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence ModuleEntrySeq     START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence ModuleSeq          START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence DeviceBindSeq      START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence SlotTypeSeq        START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence DeviceTypeSeq      START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence ModuleTemplateSeq  START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence DeviceSeq          START WITH 1 MAXVALUE 999999999 CYCLE;

Create Sequence FunctionBindSeq    START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence FunctionSeq        START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;
Create Sequence PoleZeroSeq        START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;


Create Sequence DeviceLocationSeq  START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence LocationSeq        START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;





