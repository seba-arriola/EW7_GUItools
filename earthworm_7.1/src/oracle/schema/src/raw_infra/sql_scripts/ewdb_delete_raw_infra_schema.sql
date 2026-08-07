/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


/* 
  Delete the tables 
************************/
Drop table DeviceSlot cascade constraints;
Drop table ModuleTemplate cascade constraints;
Drop table ModuleEntry cascade constraints;
Drop table DeviceType cascade constraints;
Drop table Device cascade constraints;
Drop table ActualDevice cascade constraints;
Drop table GenericDevice cascade constraints;
Drop table DeviceBind cascade constraints;
Drop table SlotType cascade constraints;
Drop table Module cascade constraints;
Drop table FunctionBind cascade constraints;
Drop table RealDevice cascade constraints;
Drop table DeviceLocation cascade constraints;
Drop table Location cascade constraints;
Drop table Manufacturer cascade constraints;
Drop table PZFilter cascade constraints;
Drop table PolesZeroes cascade constraints;
Drop table UnitType cascade constraints;

Drop Sequence DeviceSlotSeq;
Drop Sequence ModuleEntrySeq     ;
Drop Sequence ModuleSeq          ;
Drop Sequence DeviceBindSeq      ;
Drop Sequence SlotTypeSeq        ;
Drop Sequence DeviceTypeSeq      ;
Drop Sequence ModuleTemplateSeq  ;
Drop Sequence DeviceSeq          ;

Drop Sequence FunctionBindSeq    ;
Drop Sequence FunctionSeq        ;
Drop Sequence PoleZeroSeq        ;


Drop Sequence DeviceLocationSeq  ;
Drop Sequence LocationSeq        ;



/* Drop the infrastructure views */
DROP VIEW ALL_FUNCTIONBIND_INFO;

/* Drop the infrastructure procs */
DROP PROCEDURE Bind_Device_To_Slot;

DROP PROCEDURE Create_DeviceType;

DROP PROCEDURE Create_ModuleEntry;

DROP PROCEDURE Create_Module;

DROP PROCEDURE Create_ModuleTemplate;

DROP PROCEDURE Create_DeviceSlot;

DROP PROCEDURE Create_or_Update_Device;

DROP PROCEDURE Create_SlotType;

DROP PROCEDURE Fix_Device_Bind;

DROP PROCEDURE Fix_Function_Bind;

DROP PROCEDURE Get_Device_Info;

DROP PROCEDURE Get_DeviceSlot_Info;

DROP PROCEDURE Get_DeviceType_Info;

DROP PROCEDURE Get_Module_Info;

DROP PROCEDURE Get_ModuleEntry;

DROP PROCEDURE Get_Next_Plexor;

DROP PROCEDURE Get_Plexor_For_ChanT;

DROP PROCEDURE Get_SlotType_Info;

DROP PROCEDURE Set_Device_Function;

DROP PROCEDURE Set_Plexor_For_Channel;


