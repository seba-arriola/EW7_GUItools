/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Arrival
(OUT_idOP out number,
 OUT_idPick out number,
 IN_idOrigin number,
 IN_idPick number,
 IN_sCalcPhase varchar,
 IN_tCalcPhase number,
 IN_dWeight number,
 IN_dDist number,
 IN_dAzm number,
 IN_dTakeoff number,
 /* Pick Properties */
 IN_sObsPhase varchar,
 IN_tObsPhase number,
 IN_sSource varchar,
 IN_xidExternal varchar,
 IN_idChan number,
 IN_cMotion char,
 IN_cOnset char,
 IN_tResPick number,
 IN_dSigma number
)
as
/* Return Codes for OUT_idOP:
                  >0  DB idEvent
                  -1  Unknown Error
                  -7  Unknown DUP_VAL_ON_INDEX exception
                  -8  Index problem on table OriginPick. SERIOUS!!!!
                 -1X  Error from Check_Record_Validity(), 
                       X indicates error returned by C_R_V()
                       for Origin call.
                -1XX  Error from Create_Pick(), 
                       XX indicates error returned by Create_Pick()

				  Others:  Undefined
*/
Temp_ID          number;
Temp_idPick      number;
State            number;
Temp_idOP        number;

begin

begin


  /**********************************/
  /* Check for Origin record validity */
  /**********************************/

  Check_Record_Validity(Temp_ID,IN_idOrigin,'Origin');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idOP := -10 + Temp_ID;
   return;
  end if;
    
  State := 1;

  /*****************************************/
  /* Create the Pick, only if it is new    */
  /*****************************************/

	if not (IN_idPick > 0) then
		Create_Pick(Temp_idPick, IN_idChan, 
                IN_sSource, IN_xidExternal,
                IN_sObsPhase, IN_tObsPhase,
								IN_cMotion, IN_cOnset, IN_dSigma);

		if not(Temp_idPick > 0) then
			/* error, set return code and quit */
			OUT_idOP := -100 + Temp_idPick;
			return;
		else
			OUT_idPick := Temp_idPick;
		end if;
	else

		/* We got something that looks like idPick -- check it */
		Check_Record_Validity (Temp_ID, IN_idPick, 'Pick');

		if not(Temp_ID > 0) then
			OUT_idOP := -20 + Temp_ID;
			return;
		end if;

		OUT_idPick := IN_idPick;
	end if;

  State := 2;

  /**********************************/
  /* Get A New OPID.              */
  /**********************************/
  select OriginPickSeq.NEXTVAL into Temp_idOP from sys.dual;

  Create_Core_idKey(Temp_idOP);
  if Temp_idOP <= 0 then
    OUT_idOP := Temp_idOP;
  end if;

  State := 3;

  /**********************************/
  /* Insert new OriginPick Record    */
  /**********************************/
  insert into OriginPick(idOriginPick,idOrigin,idPick,
                         sPhase,tPhase,dWeight,dDist,dAzm,dTakeoff,tResPick)
    values(Temp_idOP, IN_idOrigin, OUT_idPick,
           IN_sCalcPhase, IN_tCalcPhase, IN_dWeight, IN_dDist,
           IN_dAzm, IN_dTakeOff, IN_tResPick);

  /**********************************/
  /* Set the idOP return value      */
  /**********************************/

  OUT_idOP := Temp_idOP;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    if State = 3 then
      select idOriginPick into Temp_idOP from OriginPick 
       where idOrigin = IN_idOrigin
         and idPick   = OUT_idPick;

      OUT_idOP := Temp_idOP;
    else
      OUT_idOP := -7;
    end if;


  WHEN OTHERS THEN
    /*  ??? */
	  OUT_idOP := -1;
END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_idOP := -8;
END;
