/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Update_Merge_Preference
(OUT_idPrefEvent out number,
 IN_idPh number,
 IN_idPrefEvent number
)
as
Temp_idPrefEvent     Number := IN_idPrefEvent;

begin

    /**********************************/
    /* Deal With Event Issues         */
    /**********************************/
     Check_idEvent_Validity (Temp_idPrefEvent);
     if Temp_idPrefEvent < 1 then
       OUT_idPrefEvent := -100;
       return;
     end if;

    /***************************************/
    /* Update the Phenomenon               */
    /***************************************/
     update Phenomena
		set idPrefEvent=Temp_idPrefEvent
		where idPh=IN_idPh;


    /**********************************************/
    /* Set OUT_idPrefEvent to the idPrefEvent
    /* just set as preferred
    /**********************************************/
    OUT_idPrefEvent := Temp_idPrefEvent;

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/

end;
