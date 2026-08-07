/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE or REPLACE PROCEDURE Get_idEvent_From_ExternalID
(
 OUT_idEvent out number,
 IN_sSource  number,
 IN_sSourceEventID varchar
)
as
/* Return Codes for OUT_idEvent:
          >0  DB Record ID from Event
          -1  Unknown Error
          -2  More than one Event record was found matching 
               IN_sSourceEventID from IN_idSource.  This 
               indicates that there is a bug in the internal
               core processing system.
          -3  CreateEvent() failed!
				  Others:  Undefined
*/

Temp_idEvent   Number;
Temp_idSource  Number;
Temp_idExternalEvent Number;

begin
    /* Call Create_Event, it should handle all of the details for us. */
    Create_Event(Temp_idEvent,2/*Quake*/,0,'',
                 IN_sSource,IN_sSourceEventID);

    if Temp_idEvent < 0 then
     OUT_idEvent := -3;
     return;
    end if;

  OUT_idEvent := Temp_idEvent;

EXCEPTION 
  WHEN OTHERS THEN
    OUT_idEvent := -1;
END;



