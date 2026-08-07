/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_idExtEv_From_Source_EvID
(
 OUT_idExternalEvent out number,
 IN_idSource number,
 IN_sSourceEventID varchar
)
as
/* Return Codes for OUT_idExternalEvent:
          >0  DB idExternalEvent
          -1  Unknown Error
          -5  Unexpected exception in Exception handler
         -1X  Error occurred in Create_ExternalEvent().
               X indicates the error returned by 
               Create_ExternalEvent()
				  Others:  Undefined
*/
Temp_idExternalEvent Number;

begin

begin

  /**********************************/
  /* Check for ExternalEvent Record */
  /**********************************/

  select idExternalEvent into Temp_idExternalEvent from ExternalEvent
    where sSourceEventID=IN_sSourceEventID AND idSource=IN_idSource;

  OUT_idExternalEvent := Temp_idExternalEvent;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* ExternalEvent not Found!!!
    /**********************************/
    Create_ExternalEvent(Temp_idExternalEvent,IN_idSource,
                          IN_sSourceEventID);
    if Temp_idExternalEvent <= 0 then
      OUT_idExternalEvent := -10 + Temp_idExternalEvent;
      return;
    end if;

    OUT_idExternalEvent := Temp_idExternalEvent;
  WHEN OTHERS THEN
    OUT_idExternalEvent := -1;
END;

EXCEPTION
  WHEN OTHERS THEN
    OUT_idExternalEvent := -5;
END;

