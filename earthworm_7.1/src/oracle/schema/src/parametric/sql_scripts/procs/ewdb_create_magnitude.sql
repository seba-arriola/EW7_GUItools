/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_create_magnitude.sql,v 1.6 2002/03/22 20:52:55 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_create_magnitude.sql,v $
 *     Revision 1.6  2002/03/22 20:52:55  davidk
 *     pre v6.1 checkin
 *
 *
 **************************************************************************/


CREATE OR REPLACE PROCEDURE Create_Magnitude
(OUT_idMag out number,
 IN_sExternalTableName in varchar,
 IN_xidExternal in varchar,
 IN_sSource in varchar,
 IN_iMagType in number,
 IN_dMagAvg in number,
 IN_dMagErr in number,
 IN_iNumMags in number,
 IN_bBindToEvent number,
 IN_bSetPreferred number,
 IN_idEvent in number,
 IN_idOrigin in number
)
as

/* Return Codes for OUT_idMag:
                  >0  DB idEvent
                  -1  Unknown Error
                  -2  Invalid magnitude type in IN_iMagType.
                  -7  Invalid IN_idOrigin parameter
                  -8  IN_bBindToEvent set, and IN_idEvent invalid!
                  -9  Invalid IN_sSource parameter
                 -1X  Error from Check_External_Record_Validity(),
                       X indicates error returned by C_E_R_V()
                 -2X  Error in Create_Bind(), X describes the error returned
                       by Create_Bind()
                 -3X  Error in Set_Prefer(), X describes the error returned
                       by Set_Prefer()


				  Others:  Undefined
*/
Temp_ID          number;
State            number;
Temp             number;
Temp_idMag       number;
Temp_idBind      Number;
Temp_idEvent     Number := IN_idEvent;
Temp_idSource    number;
Temp_idPrefer    number;
Temp_idOrigin    number;
Temp_tiOrigin    number;
begin


  State := 0;

  if IN_idOrigin = 0 then
    Temp_idOrigin := NULL;
  else 
    Temp_idOrigin := IN_idOrigin;
  end if;

  State := 1;

  /**********************************/
  /* Check for external record validity
  /**********************************/
  Check_External_Record_Validity(Temp_ID,IN_xidExternal,
                                 IN_sExternalTableName);
  if not(Temp_ID >= 0) then
   /* error, set return code and quit */
   OUT_idMag := -10 + Temp_ID;
   return;
  end if;

  State := 2;

  if IN_bBindToEvent = 1 then /* if we are binding to an event */
    /**********************************/
    /* Deal With Event Issues         */
    /**********************************/
    Check_idEvent_Validity(Temp_idEvent);
    if Temp_idEvent < 1 then
      OUT_idMag := -8;
      return;
    end if;
  end if;  /* if IN_bBindToEvent = 1 */

  State := 21;

  if Temp_idOrigin IS NOT NULL then /* if we are binding to an Origin */
    /**********************************/
    /* Deal With Origin Issues        */
    /**********************************/
    Check_Record_Validity(Temp_tiOrigin, Temp_idOrigin, 'Origin');
    if Temp_tiOrigin < 0 then
      OUT_idMag := -7;
      return;
    end if;
  end if;  /* if Temp_idOrigin NOT NULL */

  State := 3;

  /**********************************/
  /* Get the idSource               */
  /**********************************/
  Get_idSource(Temp_idSource,IN_sSource);
  if Temp_idSource < 1 then
    OUT_idMag := -9;
    return;
  end if;

  State := 4;
  /**********************************/
  /* Get A New MagID.              */
  /**********************************/
  select MagnitudeSeq.NEXTVAL into Temp_idMag from sys.dual;

  Create_Core_idKey(Temp_idMag);
  if Temp_idMag <= 0 then
    OUT_idMag := Temp_idMag;
  end if;

  State := 5;
  /**********************************/
  /* Insert new Magnitude Record    */
  /**********************************/
  insert into Magnitude(idMag,tiExternal,xidExternal,dMagAvg,
                        dMagErr,iNumMags,iMagType,idSource,idOrigin)
    values(Temp_idMag,Temp_ID,IN_xidExternal,IN_dMagAvg,
           IN_dMagErr,IN_iNumMags,IN_iMagType,Temp_idSource,Temp_idOrigin);

  State := 6;

  if IN_bBindToEvent = 1 then
    /**********************************/
    /* Bind our Magnitude to the Event   */
    /**********************************/
    Create_Bind(Temp_idBind,Temp_idEvent,'Magnitude',Temp_idMag);
    if Temp_idBind < 1 then
       OUT_idMag := -20 + Temp_idBind;
       return;
    end if;

  State := 7;

    if IN_bSetPreferred = 1 then
      /**********************************/
      /* Make our magnitude the preferred one
      /* for this event.
      /**********************************/
      Set_Prefer(Temp_idPrefer,Temp_idEvent,3/*Magnitude*/,Temp_idMag);
      if Temp_idPrefer < 1 then
         OUT_idMag := -30 + Temp_idPrefer;
         return;
      end if;
    end if;
  end if;  /* IN_bBindToEvent */

  State := 8;

  /**********************************/
  /* Set the idMag return value   */
  /**********************************/

  OUT_idMag := Temp_idMag;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      OUT_idMag := -2;
    else
      OUT_idMag := -1;
    end if;
    insert into test values('Create_Magnitude_NDF',State,Out_idMag);
  WHEN OTHERS THEN
    /*  ??? */
    Temp := SQLCODE;
    insert into test values('Create_Magnitude',State,Temp);
	  OUT_idMag := -1;
END;

