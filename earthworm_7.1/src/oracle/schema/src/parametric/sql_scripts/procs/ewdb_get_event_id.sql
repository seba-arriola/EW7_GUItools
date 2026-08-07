/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_Event_ID
(OUT_idEvent out number,
 IN_sSource varchar,
 IN_sSourceEventID varchar
)
as
/* Return Codes for OUT_idEvent:
                  >0  Event ID 
                  -1  Error or event not found
*/

Temp_idEvent         Number;
Temp_idSource        Number;
Temp_tiExternalEvent Number;
Temp_idBind          number;
Temp_idExternalEvent number;
State                number;

begin

  /**********************************/
  /* Is this for an external
  /* event that may already be in
  /* the database?
  /**********************************/

	State := 0;

    /* Get the idSource of the Source */
    Get_idSource(Temp_idSource,IN_sSource);

    if Temp_idSource <= 0 then
      OUT_idEvent := -100 + Temp_idSource; /* Error in Get_idSource() */
      return;
    end if;

	State := 1;

    /* Get the external id for the external event */
    Get_idExtEv_From_Source_EvID(Temp_idExternalEvent,
                                 Temp_idSource,IN_sSourceEventID);

    if Temp_idExternalEvent <= 0 then
      OUT_idEvent := -1000 + Temp_idExternalEvent;
      return;
    end if;

	State := 2;

    /* Get the table identifier for 'ExternalEvent' */
    select idTable into Temp_tiExternalEvent from EWDB_Tablelist
      where sTableName = 'ExternalEvent';

	State := 3;

    /* Get the idEvent for our idExternalEvent via Bind */
    select idEvent into Temp_idEvent from Bind
      where idCore = Temp_idExternalEvent
        and tiCore = Temp_tiExternalEvent;

	OUT_idEvent := Temp_idEvent;

EXCEPTION
	WHEN NO_DATA_FOUND THEN

	if State = 2 then
		/* problem with ExternalEvent table identifier */
		OUT_idEvent := -10000;
		return;
	else
		OUT_idEvent := -1;
		return;
	end if;

end;
