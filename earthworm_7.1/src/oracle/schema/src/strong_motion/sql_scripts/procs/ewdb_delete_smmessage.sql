/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/*    Revision history:                                     *
 *                                                          *
 *     $Log: ewdb_delete_smmessage.sql,v $
 *     Revision 1.4  2001/07/14 07:59:53  davidk
 *     Removed last comment, which was supposed to say:
 *     Added bForce support to give caller the option of deleting data
 *     associated with an event or not.
 *
 *     Revision 1.3  2001/07/14 07:59:15  davidk
 *
 *     Revision 1.2  2001/04/11 18:10:23  davidk
 *     fixed a typo in two of the select statements.  Replaced 'int' with 'into'.
 *                   *
 ************************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT 
TYPE FUNCTION_PROTOTYPE 

LIBRARY  EWDB_API_LIB 

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE SQL

LOCATION THIS_FILE 

FUNCTION Delete_SMMessage

RETURN_TYPE NUMBER 

RETURN_VALUE 0
RETURN_DESCRIPTION Success

RETURN_VALUE -1
RETURN_DESCRIPTION Unknown error

RETURN_VALUE -2
RETURN_DESCRIPTION IN_idSMMessage was not valid.  No SMMessage record
with a matching idSMMessage was found.

RETURN_VALUE -3
RETURN_DESCRIPTION The deletion request violated a Foreign Key constraint.
Please see the DEBUG table for more debugging information.

PARAMETER 1 
PARAM_NAME IN_idSMMessage
PARAM_TYPE number 
PARAM_DESCRIPTION The DB ID of the SMMessage to delete.

PARAMETER 2 
PARAM_NAME IN_bForce
PARAM_TYPE number 
PARAM_DESCRIPTION Flag indicating whether or not the function 
should force the delete of an SMMessage, even if it is still
bound to existing events.

DESCRIPTION This procedure deletes a strong motion message
from the DB, including:  1)Deleting all of the SMMotions 
associated with the message;  2)Deleting any bindings of the
message to an Event; and 3)Deleting the SMMessage record itself.

*************************************************
************************************************/

CREATE OR REPLACE FUNCTION Delete_SMMessage
(IN_idSMMessage number,
 IN_bForce number
) RETURN number
as

Temp       number;
State      number;
Temp_Count number;
Temp_tiCore number;
begin
  
  State := 0;

  if IN_bForce = 0 then
    Temp_tiCore := GetTI('SMMessage');
    select count(idEvent) into Temp from Bind 
      where idCore=IN_idSMMessage and tiCore=Temp_tiCore;
	if Temp > 0 then
      return(2);  /* We don't want to delete this smmessage
	                 because it is linked to 1 or more existing
					 events and the bForce flag isn't set. */
    end if;
  end if;

  State := 11;


  /* delete all SMMotion measurements for the message */
  select count(idSMMotion) into Temp_Count from SMMotion 
   where idSMMessage = IN_idSMMessage;

  if Temp_Count > 0 then
    delete SMMotion where idSMMessage = IN_idSMMessage;
  end if;

  State := 1;

  /* delete any binds between the message and events */
  select count(idBind) into Temp_Count from Bind 
   where idCore = IN_idSMMessage AND tiCore = GetTI('SMMessage');

  if Temp_Count > 0 then
    delete Bind where idCore = IN_idSMMessage AND tiCore = GetTI('SMMessage');
  end if;

  State := 2;

  /* delete the message itself */
  delete SMMessage where idSMMessage = IN_idSMMessage;

  return(0);

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    insert into test values('Delete_SMMessage_NDF',IN_idSMMessage,State);
    return(-2);
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      insert into test values('Delete_SMMessage_FK',IN_idSMMessage,State);
      return(-3);
    else
      insert into test values('Delete_SMMessage' || IN_idSMMessage, State, Temp);
      return(-1);
    end if;
END;
