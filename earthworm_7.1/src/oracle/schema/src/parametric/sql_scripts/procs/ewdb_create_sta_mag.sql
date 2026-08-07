/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Sta_Mag
(OUT_idMagLink out number,
 IN_idMagnitude number,
 IN_idDatum number,
 IN_dMag number,
 IN_dWeight number
)
as
/* Return Codes for OUT_idMagLink:
                  >0  DB idEvent
                  -1  Unknown Error
                  -2  MagType Mismatch between IN_sMagType and
                        Magnitude.tiMagType
                  -6  MagLink Record already exists between Magnitude.IN_idMagnitude 
                       and <Datum>.IN_idDatum.
                  -7  Unknown DUP_VAL_ON_INDEX exception
                  -8  Index problem on table OriginPick. SERIOUS!!!!
                 -1X  Error from Check_Record_Validity(), 
                       X indicates error returned by C_R_V()
                 -2X  Error from Get_Magnitude_Type(), 
                       X indicates error returned by G_M_T()  (ha!ha!)
                 -3X  Error from Get_Mag_Type_Info(), 
                       X indicates error returned by G_M_T_I()
                 -4X  Error from Check_Record_Validity(), 
                       X indicates error returned by C_R_V()

				  Others:  Undefined
*/
Temp_ID             number;
State               number;
Temp_idMagLink      number;
Temp_tiMagType      number;
Temp_sMagTableName  varchar2(30);
Temp_iMagType       number;
Temp_iMagTypeDatum  number;
Temp                number;

begin

  /**********************************/
  /* Check for Magnitude record validity 
  /**********************************/

  Check_Record_Validity(Temp_ID,IN_idMagnitude,'Magnitude');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idMagLink := -10 + Temp_ID;
   return;
  end if;
    
  State := 1;

  Get_Magnitude_Type(Temp_iMagType,IN_idMagnitude);
  if not(Temp_iMagType > 0) then
   /* error, set return code and quit */
   OUT_idMagLink := -20 + Temp_iMagType;
   return;
  end if;

  State := 2;

  Get_Mag_Type_Info(Temp_ID, Temp_iMagType, Temp_tiMagType, Temp_sMagTableName);
  if not(Temp_ID = 0) then
   /* error, set return code and quit */
   OUT_idMagLink := -30 + Temp_ID;
   return;
  end if;

  State := 3;

  /**********************************
   * Check for Datum record validity 
   **********************************/
  Check_Record_Validity(Temp_ID,IN_idDatum,Temp_sMagTableName);
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idMagLink := -40 + Temp_ID;
   return;
  end if;
    
   
  State := 3;

  if NOT(Temp_sMagTableName = 'CodaDur') then
    select iMagType into Temp_iMagTypeDatum
     from PeakAmp where idPeakAmp = IN_idDatum;

    State := 4;

    if Not(Temp_iMagType = Temp_iMagTypeDatum) then
      /* not the correct type of supporting measurement for 
         the given magnitude (ie. bodyamp instead of coda dur */
      OUT_idMagLink := -2;
      return;
    end if;
  end if;

  State := 5;
  /**********************************/
  /* Get A New MagLinkID.           */
  /**********************************/
  select MagLinkSeq.NEXTVAL into Temp_idMagLink from sys.dual;

  Create_Core_idKey(Temp_idMagLink);
  if Temp_idMagLink <= 0 then
    OUT_idMagLink := Temp_idMagLink;
  end if;

  State := 6;

  /**********************************/
  /* Insert new MagLink Record      */
  /**********************************/
  insert into MagLink(idMagLink,idMag,idDatum,dMag,dWeight)
    values(Temp_idMagLink,IN_idMagnitude,IN_idDatum,IN_dMag,IN_dWeight);

  /**********************************/
  /* Set the idMagLink return value   */
  /**********************************/

  OUT_idMagLink := Temp_idMagLink;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    if State = 6 then
      OUT_idMagLink := -6;
    else
      OUT_idMagLink := -7;
    end if;

  WHEN NO_DATA_FOUND THEN
    OUT_idMagLink := -8;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Sta_Mag ' || IN_dMag,Temp,Temp_idMagLink);
    insert into test values('Create_Sta_Mag2 ' || IN_dWeight,State, Temp_idMagLink);
    insert into test values('Create_Sta_Mag3 ' || IN_idDatum, IN_idMagnitude, Temp_idMagLink);
	  OUT_idMagLink := -1;
END;

