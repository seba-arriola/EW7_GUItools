/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/* 
 Create Strong Motion views
*****************************/


/* This is the base view that retrieves all info about SMMessages */
CREATE OR REPLACE VIEW ALL_SMMESSAGE_INFO
 as
 select smmsg.tMotion, smmsg.tLoad, smmsg.tAlternate, smmsg.iAltCode, 
        smmsg.idSMMessage, smmsg.tPGA, smmsg.tPGV, smmsg.tPGD, smmsg.idChan, 
        co.sSta, co.sComp, co.sNet ,co.sLoc ,co.dLat ,co.dLon ,
        co.dElev, co.dAzm, co.dDip, co.tOn, co.tOff
 from SMMessage smmsg, chant cht, compt co
 WHERE smmsg.idChan = cht.idChan
   AND cht.idCompt = co.idCompt;


/* This view retrieves the IDs of all SMMESSAGES that are bound
   to an event. If a message is bound to more than one event, 
   it will appear multiple times in the results of this view. */
CREATE OR REPLACE VIEW ALL_SMMESSAGE_EVENT_LINKS
 as
 select b.idCore idSMMessage, b.idEvent
 from bind b
 where b.tiCore = GetTI('SMMessage');


/* This view retrieves info for all SMMessages that are bound to 
   atleast one event.  A message will appear in the results for 
   each Event that it is bound to. */
CREATE OR REPLACE VIEW ALL_SMMESSAGE_INFO_W_EVENT
 as
 select asmi.*, asmel.idEvent
 from ALL_SMMESSAGE_INFO asmi, ALL_SMMESSAGE_EVENT_LINKS asmel
 WHERE asmi.idSMMessage = asmel.idSMMessage;


/* This view retrieves info for all SMMessages.  If the SMMessage 
   is bound to an Event, then the idEvent for that Event will show 
   up in the result list, otherwise the idEvent column will be NULL.  
   An SMMessage maybe bound to more than one event, in which case
   it will appear in the results for each Event that it is bound to. */
CREATE OR REPLACE VIEW ALL_SMMESSAGE_INFO_W_OPT_EVENT
 as
 select asmi.*, asmel.idEvent
 from ALL_SMMESSAGE_INFO asmi, ALL_SMMESSAGE_EVENT_LINKS asmel
 WHERE asmi.idSMMessage = asmel.idSMMessage(+);


/* This view retrieves the idSMMessage of ONLY SMMessages that
   are NOT bound to an Event. */
CREATE OR REPLACE VIEW ALL_SMMESSAGES_WO_EVENT
as
 select idSMMessage from ALL_SMMESSAGE_INFO
  MINUS
 select idSMMessage from ALL_SMMESSAGE_EVENT_LINKS;


/* This view retrieves info for all SMMessages that are NOT bound 
   to any events.  */
CREATE OR REPLACE VIEW ALL_SMMESSAGE_INFO_WO_EVENT
 as
 select asmi.*, 0 idEvent
 from ALL_SMMESSAGE_INFO asmi, ALL_SMMESSAGES_WO_EVENT asmwoe
 WHERE asmi.idSMMessage = asmwoe.idSMMessage;


/* This view retrieves info for all SMMotions */
CREATE OR REPLACE VIEW ALL_SMMOTION_INFO
 as
 select * from SMMotion;

/* This view retrieves all SMMessages */
CREATE OR REPLACE VIEW ALL_SM_MESSAGES
 as
 select * from SMMessage;


/* 
 Load Strong Motion procedures
********************************/

@ewdb_create_smmessage
/

@ewdb_create_smmotion
/

@ewdb_delete_smmessage
/

@ewdb_delete_smmessages_before_time
/


