/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_CodaAmp
(OUT_idCodaAmp out number,
 IN_idTCoda number,
 IN_sExternalTableName varchar,
 IN_xidExternal varchar,
 IN_idChan number,
 IN_tOn number,
 IN_tOff number,
 IN_iAvgAmp number
)
as

/* Return Codes for OUT_idCodaAmp:
                  >0  DB idEvent
                  -1  Unknown Error
                 -1X  Error from Check_Record_Validity(), 
                       X indicates error returned by C_R_V()
                 -2X  Error from Check_External_Record_Validity(), 
                       X indicates error returned by C_E_R_V()

				  Others:  Undefined
*/
Temp_ID          number;
State            number;
Temp_idCodaAmp   number;

begin


  /**********************************/
  /* Check for TCoda record validity */
  /**********************************/

  Check_Record_Validity(Temp_ID,IN_idTCoda,'TCoda');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idCodaAmp := -10 + Temp_ID;
   return;
  end if;
    
  State := 1;


  /**********************************/
  /* Check for external record validity
  /**********************************/

  Check_External_Record_Validity(Temp_ID,IN_xidExternal,
                                 IN_sExternalTableName);
  if not(Temp_ID >= 0) then
   /* error, set return code and quit */
   OUT_idCodaAmp := -20 + Temp_ID;
   return;
  end if;
    
  State := 2;


  /**********************************/
  /* Get A New CodaAmpID.           */
  /**********************************/
  select CodaAmpSeq.NEXTVAL into Temp_idCodaAmp from sys.dual;

  Create_Core_idKey(Temp_idCodaAmp);
  if Temp_idCodaAmp <= 0 then
    OUT_idCodaAmp := Temp_idCodaAmp;
  end if;

  /**********************************/
  /* Insert new CodaAmp Record         */
  /**********************************/
  insert into CodaAmp(idCodaAmp,idTCoda,tiExternal,xidExternal,idChan,
                      tOn,tOff,iAvgAmp)
    values(Temp_idCodaAmp,IN_idTCoda,Temp_ID,IN_xidExternal,IN_idChan,
            IN_tOn,IN_tOff,IN_iAvgAmp);

  /**********************************/
  /* Set the idCodaAmp return value   */
  /**********************************/

  OUT_idCodaAmp := Temp_idCodaAmp;

EXCEPTION
  WHEN OTHERS THEN
    /*  ??? */
	  OUT_idCodaAmp := -1;
END;
