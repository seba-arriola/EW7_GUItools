/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Set_Prefer
(OUT_idPrefer out number,
 IN_idEvent in number,
 IN_PreferType in number,
 IN_idPrefered in number
)
as

/* Supported Prefered Types:
           1  Origin
				   2  Mechanism
				   3  Magnitude
*/

/* Return Codes for OUT_idPrefer:
          >0  DB idPrefer
          -1  Unknown Error
				  -2  Invalid eventID
				  -3  Invalid Prefer Type
				  -4  Invalid ID for Prefered Type
				  -5  Check_Record_Validity() Not Implemented for Listed Table
				  -6  Exception in Check_Record_Validity()
          -7  Error during update of Prefer
				  Others:  Undefined
*/

Temp_ID Number;
Temp_idPrefer Number;
Temp_TableID Number;
Temp_Result Number;
State   Number := 0;
Temp_Pref_Tablename varchar(32);
begin

begin


  if IN_PreferType = 1 then
    Temp_Pref_Tablename := 'Origin';
  elsif IN_PreferType = 2 then
    Temp_Pref_Tablename := 'MechFM';
  elsif IN_PreferType = 3 then
    Temp_Pref_Tablename := 'Magnitude';
  else
    OUT_idPrefer := -3;
    return;
  end if;

  /**********************************/
  /* Check for idEvent validity     */
  /**********************************/

  select idEvent into Temp_ID from event 
    where idEvent = IN_idEvent;
    
  State := 1;
  /***************************************/
  /* Check for core record validity      */
  /* Check_Record_Validity() has the     */
  /* following return codes: -1,-3,-4,-5 */
  /***************************************/

  Check_Record_Validity(Temp_TableID,IN_idPrefered,Temp_Pref_Tablename);
  if not(Temp_TableID > 0) then
   /* error, set return code and quit */
   OUT_idPrefer := Temp_TableID;
   return;
  end if;

  State := 2;
 
  /**********************************/
  /* Check for existing idPrefer    */
  /**********************************/

  select idPrefer into Temp_idPrefer from Prefer 
    where idEvent = IN_idEvent;
    
  State := 3;

  /**********************************/
  /* We've got a preferID, now we   */
  /* need to update the appropriate */
  /* column in the record from which*/
  /* it came.                       */
  /**********************************/
  if IN_PreferType = 1 then
    update Prefer set idPrefOrigin = IN_idPrefered 
     where idPrefer = Temp_idPrefer;
  elsif IN_PreferType = 2 then
    update Prefer set idPrefMech = IN_idPrefered
     where idPrefer = Temp_idPrefer;
  elsif IN_PreferType = 3 then
    update Prefer set idPrefMag = IN_idPrefered
     where idPrefer = Temp_idPrefer;
  else
    OUT_idPrefer := -3;
    return;
  end if;

  OUT_idPrefer := Temp_idPrefer;


EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
	    /*  idEvent not found */
	    OUT_idPrefer := -2;
	  elsif State = 1 then
	    /*  Error during Check_Record_Validity() */
	    OUT_idPrefer := -6;
	  elsif State = 2 then
	    /*  A Prefer record does not yet exist.  Create one */

      /**********************************/
      /* Get A New PreferID.            */
      /**********************************/
      select PreferSeq.NEXTVAL into Temp_idPrefer from sys.dual;

      Create_Core_idKey(Temp_idPrefer);
      if Temp_idPrefer <= 0 then
        OUT_idPrefer := Temp_idPrefer;
      end if;

      /**********************************/
      /* Create new prefer record       */
      /**********************************/
      if IN_PreferType = 1 then
        insert into Prefer(idPrefer,idEvent,idPrefOrigin,idPrefMag,idPrefMech)
          values(Temp_idPrefer,IN_idEvent,IN_idPrefered,NULL,NULL);
      elsif IN_PreferType = 2 then
        insert into Prefer(idPrefer,idEvent,idPrefOrigin,idPrefMag,idPrefMech)
          values(Temp_idPrefer,IN_idEvent,NULL,NULL,IN_idPrefered);
      elsif IN_PreferType = 3 then
        insert into Prefer(idPrefer,idEvent,idPrefOrigin,idPrefMag,idPrefMech)
          values(Temp_idPrefer,IN_idEvent,NULL,IN_idPrefered,NULL);
      else
        OUT_idPrefer := -3;
        return;
      end if;

      OUT_idPrefer := Temp_idPrefer;


	  elsif State = 3 then
	    /*  Error during update!  Huh!??!*/
      OUT_idPrefer := -7;
    else /* what, huh, excuse me.... */
      OUT_idPrefer := -1;
	  end if;

  WHEN OTHERS THEN
    if State = 1 then
	    /*  Error during Check_Record_Validity() */
	    OUT_idPrefer := -6;
    else /* what, huh, excuse me.... */
      OUT_idPrefer := -1;
	  end if;
END;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    Set_Prefer(Temp_idPrefer,IN_idEvent,IN_PreferType,IN_idPrefered);
    OUT_idPrefer := Temp_idPrefer;
END;
