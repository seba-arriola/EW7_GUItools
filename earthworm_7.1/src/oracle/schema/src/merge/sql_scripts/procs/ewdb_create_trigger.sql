/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Trigger
(OUT_idTrigger out number,
 IN_idCoincidence number,
 IN_idChan number,
 IN_tTrigger number
)
as
/* Return Codes for OUT_idTrigger:
                  >0  DB idTrigger 
                  -1  Unknown Error
                  -7  Unknown DUP_VAL_ON_INDEX exception
                 -1X  Error from Check_Record_Validity(), 
                       X indicates error returned by C_R_V()
                       for CoincidenceEvent call.
                -1XX  Error while inserting into the ChannelTrigger table
                -2XX  Error while inserting into the CoincTriggerLink table

				  Others:  Undefined
*/
Temp_ID          number;
Temp_idTrigger   number;

begin


  /**********************************/
  /* Check for Origin record validity */
  /**********************************/

  Check_Record_Validity (Temp_ID, IN_idCoincidence, 'CoincidenceEvent');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idTrigger := -10 + Temp_ID;
   return;
  end if;
    
  /**********************************/
  /* Create the Trigger             */
  /**********************************/

  /**********************************/
  /* Get A New Trigger ID.          */
  /**********************************/
  select TriggerSeq.NEXTVAL into Temp_idTrigger from sys.dual;

  Create_Core_idKey (Temp_idTrigger);
  if Temp_idTrigger <= 0 then
    OUT_idTrigger := -100 + Temp_idTrigger;
    return;
  end if;

  /**********************************/
  /* Insert new Trigger Record         */
  /**********************************/
  insert into ChannelTrigger (idTrigger, idCoincidence, tTrigger, idChan)
    values (Temp_idTrigger, IN_idCoincidence, IN_tTrigger, IN_idChan);

	OUT_idTrigger := Temp_idTrigger;


  /**********************************/
  /* Set the idTrigger return value      */
  /**********************************/

  OUT_idTrigger := Temp_idTrigger;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
      OUT_idTrigger := -7;
  WHEN OTHERS THEN
	  OUT_idTrigger := -1;

END;

