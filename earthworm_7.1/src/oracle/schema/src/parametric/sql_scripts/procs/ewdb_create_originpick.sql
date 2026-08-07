/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_OriginPick
(OUT_idOP out number,
 OUT_idPick out number,
 IN_idOrigin number,
 IN_sSource varchar,
 IN_sSourcePickID varchar,
 IN_sPhase varchar,
 IN_tPhase number,
 IN_dWeight number,
 IN_dDist number,
 IN_dAzm number,
 IN_dTakeoff number,
 IN_tResPick number,
 IN_idPick number
)
as

Temp_ID          number;
Temp_idPick      number :=0;
Temp_idOP        number :=0;

begin

  /************************************/
  /* Check for Origin record validity */
  /************************************/
  Check_Record_Validity(Temp_ID,IN_idOrigin,'Origin');
  if not(Temp_ID > 0) then
       /* error, set return code and quit */
       OUT_idOP := -10 + Temp_ID;
       OUT_idPick := -1;
       return;
  end if;

    /* 
	 * We work in two modes:  either we are called with a "good" known idPick,
	 * or we have to figure it out from the pick's source and sourceId
	 */
    if IN_idPick < 0 then
    	Temp_ID := Get_idPick(Temp_idPick, IN_sSource, IN_sSourcePickID);

	    if Temp_ID != 0 then
			OUT_idOP := -2;
			OUT_idPick := -1;
			return;
		end if;
	else
		Temp_idPick := IN_idPick;
	end if;


  /**********************************/
  /* Get A New OPID.                */
  /**********************************/
  select OriginPickSeq.NEXTVAL into Temp_idOP from sys.dual;

  Create_Core_idKey(Temp_idOP);
  if Temp_idOP <= 0 then
    OUT_idOP := Temp_idOP;
    OUT_idPick := -1;
	return;
  end if;


  /**********************************/
  /* Insert new OriginPick Record    */
  /**********************************/

  insert into OriginPick(idOriginPick, idOrigin, idPick,
                        sPhase, tPhase, dWeight, dDist,
                        dAzm, dTakeoff, tResPick)
    values(Temp_idOP, IN_idOrigin, Temp_idPick,
           IN_sPhase, IN_tPhase, IN_dWeight, IN_dDist,
           IN_dAzm, IN_dTakeOff, IN_tResPick);

    OUT_idOP := Temp_idOP;
   	OUT_idPick := Temp_idPick;


EXCEPTION
  WHEN OTHERS THEN
    /*  ??? */
    Temp_ID := SQLCODE;
    insert into test values('Create_OriginPick_ex', Temp_idPick, Temp_ID);
    OUT_idPick := -1;
    OUT_idOP := -1;
END;
