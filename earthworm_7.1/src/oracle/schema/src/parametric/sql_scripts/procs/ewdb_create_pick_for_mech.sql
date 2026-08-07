/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Pick_For_Mech
(OUT_idMechFMPick out number,
 OUT_idPick out number,
 IN_idMechFM number,
 IN_sPhase number,
 IN_tPhase number,
 IN_idChan number,
 IN_sExternalTableName varchar,
 IN_xidExternal varchar,
 IN_cMotion char,
 IN_cOnset char,
 IN_dSigma number
)
as
/* Return Codes for OUT_idMechFMPick:
                  >0  DB idEvent
                  -1  Unknown Error
                  -7  Unknown DUP_VAL_ON_INDEX exception
                  -8  Index problem on table MechFMPick. SERIOUS!!!!
                 -1X  Error from Check_Record_Validity(), 
                       X indicates error returned by C_R_V()
                       for Origin call.
                -1XX  Error from Create_Pick(), 
                       XX indicates error returned by Create_Pick()

				  Others:  Undefined
*/
Temp_ID           number;
Temp_idPick       number;
State             number;
Temp_idMechFMPick number;

begin

begin


  /**********************************/
  /* Check for MechFM record validity */
  /**********************************/

  Check_Record_Validity(Temp_ID,IN_idMechFM,'MechFM');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idMechFMPick := -10 + Temp_ID;
   return;
  end if;
    
  State := 1;

  /**********************************/
  /* Create the Pick                */
  /**********************************/

  Create_Pick(Temp_idPick,IN_idChan,
              IN_sExternalTableName,IN_xidExternal,
              IN_sPhase,IN_tPhase,
              IN_cMotion, IN_cOnset,IN_dSigma);
  if not(Temp_idPick > 0) then
   /* error, set return code and quit */
   OUT_idMechFMPick := -100 + Temp_idPick;
   return;
  else
    OUT_idPick := Temp_idPick;
  end if;
    
  State := 2;

  /**********************************/
  /* Get A New MechFMPick ID.       */
  /**********************************/
  select MechFMPickSeq.NEXTVAL into Temp_idMechFMPick from sys.dual;

  Create_Core_idKey(Temp_idMechFMPick);
  if Temp_idMechFMPick <= 0 then
    OUT_idMechFMPick := Temp_idMechFMPick;
  end if;

  /**********************************/
  /* Insert new MechFMPick Record    */
  /**********************************/
  insert into MechFMPick(idMechFMPick,idMechFM,idPick)
    values(Temp_idMechFMPick,IN_idMechFM,Temp_idPick);

  /**********************************/
  /* Set the idMechFMPick return value      */
  /**********************************/

  OUT_idMechFMPick := Temp_idMechFMPick;

EXCEPTION

  WHEN DUP_VAL_ON_INDEX THEN
    if State = 2 then
      select idMechFMPick into Temp_idMechFMPick
       from MechFMPick
       where idMechFM = IN_idMechFM
         and idPick   = Temp_idPick;

      OUT_idMechFMPick := Temp_idMechFMPick;
    else
      OUT_idMechFMPick := -7;
    end if;

  WHEN OTHERS THEN
    /*  ??? */
	  OUT_idMechFMPick := -1;
END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_idMechFMPick := -8;
END;
