/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_TCoda
(OUT_idTCoda out number,
 IN_sExternalTableName varchar,
 IN_xidExternal varchar,
 IN_idChan number,
 IN_tCodaTermObs number,
 IN_tCodaTermXtp number

)
as
/* Return Codes for OUT_idTCoda:
                  >0  DB idTCoda
                  -1  Unknown Error
                 -1X  Error from Check_External_Record_Validity(), 
                       X indicates error returned by C_E_R_V()

				  Others:  Undefined
*/
Temp_ID          number;
State            number;
Temp_idTCoda     number;

begin

  State := 0;

  /**********************************/
  /* Check for external record validity
  /**********************************/

  Check_External_Record_Validity(Temp_ID,IN_xidExternal,
                                 IN_sExternalTableName);
  if not(Temp_ID >= 0) then
   /* error, set return code and quit */
   OUT_idTCoda := -10 + Temp_ID;
   return;
  end if;
    
  State := 1;

  /**********************************/
  /* Get A New TCodaID.              */
  /**********************************/
  select TCodaSeq.NEXTVAL into Temp_idTCoda from sys.dual;

  State :=2;

  Create_Core_idKey(Temp_idTCoda);
  if Temp_idTCoda <= 0 then
    OUT_idTCoda := Temp_idTCoda;
  end if;

  State := 3;

  /**********************************/
  /* Insert new TCoda Record         */
  /**********************************/
  insert into TCoda(idTCoda,tiExternal,xidExternal,idChan,
                    tCodaTermObs,tCodaTermXtp)
    values(Temp_idTCoda,Temp_ID,IN_xidExternal,IN_idChan,
           IN_tCodaTermObs,IN_tCodaTermXtp);

  State := 4;

  /**********************************/
  /* Set the idTCoda return value   */
  /**********************************/

  OUT_idTCoda := Temp_idTCoda;

  State := 5;

EXCEPTION
  WHEN OTHERS THEN
    /*  ??? */
    insert into test values('Create_TCoda1',IN_idChan,Temp_ID);
    insert into test values('Create_TCoda2',IN_tCodaTermObs,IN_tCodaTermObs);

    Temp_ID := SQLCODE;
    insert into test values('Create_TCoda',State,Temp_ID);
	  OUT_idTCoda := -1;
END;
