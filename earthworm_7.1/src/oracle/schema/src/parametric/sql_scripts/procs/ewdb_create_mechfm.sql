/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_MechFM
(OUT_idMechFM out number,
 IN_sExternalTableName in varchar,
 IN_xidExternal in varchar,
 IN_idOrigin in number,
 IN_bBindToEvent number,
 IN_bSetToPreferred number,
 IN_idEvent in number
)
as
/* Return Codes for OUT_idMechFM:
                  >0  DB idEvent
                  -1  Unknown Error
                  -8  IN_bBindToEvent set, and IN_idEvent invalid!
                 -1X  Error from Check_Record_Validity(), 
                       X indicates error returned by C_R_V()
                 -2X  Error from Check_External_Record_Validity(), 
                       X indicates error returned by C_E_R_V()
                 -3X  Error in Create_Bind(), X describes the error returned
                       by Create_Bind()
                 -3X  Error in Set_Prefer(), X describes the error returned
                       by Set_Prefer()
				  Others:  Undefined
*/
Temp_ID     number;
State       number;
Temp_idMechFM number;
Temp_idBind      Number;
Temp_idPrefer    Number;
Temp_idEvent     Number := IN_idEvent;
begin

  /**********************************/
  /* Check for origin record validity   */
  /**********************************/

  Check_Record_Validity(Temp_ID,IN_idOrigin,'Origin');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idMechFM := -10 + Temp_ID;
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
   OUT_idMechFM := -20 + Temp_ID;
   return;
  end if;
    
  State := 2;


  if IN_bBindToEvent = 1 then /* if we are binding to an event */
    /**********************************/
    /* Deal With Event Issues         */
    /**********************************/
    Check_idEvent_Validity(Temp_idEvent);
    if Temp_idEvent < 1 then
      OUT_idMechFM := -8;
      return;
    end if;
  end if;  /* if IN_bBindToEvent = 1 */

  /**********************************/
  /* Get A New MechFMID.              */
  /**********************************/
  select MechFMSeq.NEXTVAL into Temp_idMechFM from sys.dual;

  Create_Core_idKey(Temp_idMechFM);
  if Temp_idMechFM <= 0 then
    OUT_idMechFM := Temp_idMechFM;
  end if;

  /**********************************/
  /* Insert new MechFM Record       */
  /**********************************/
  insert into MechFM(idMechFM,tiExternal,xidExternal,idOrigin)
    values(Temp_idMechFM,Temp_ID,IN_xidExternal,IN_idOrigin);


  if IN_bBindToEvent = 1 then
    /**********************************/
    /* Bind our Mechanism to the Event   */
    /**********************************/
    Create_Bind(Temp_idBind,Temp_idEvent,'MechFM',Temp_idMechFM);
    if Temp_idBind < 1 then
       OUT_idMechFM := -30 + Temp_idBind;
       return;
    end if;
    if IN_bSetToPreferred = 1 then
      /**********************************/
      /* Make our Mech the preferred for 
      /* our event
      /**********************************/
      Set_Prefer(Temp_idPrefer,Temp_idEvent,2/*MechFM*/,Temp_idMechFM);
      if Temp_idPrefer < 1 then
        OUT_idMechFM := -40 + Temp_idPrefer;
        return;
      end if;
    end if;
  end if;

  /**********************************/
  /* Set the idMechFM return value   */
  /**********************************/

  OUT_idMechFM := Temp_idMechFM;

EXCEPTION
  WHEN OTHERS THEN
    /*  ??? */
	  OUT_idMechFM := -1;
END;
