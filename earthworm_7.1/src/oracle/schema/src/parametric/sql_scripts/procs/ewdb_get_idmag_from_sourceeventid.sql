/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_idMag_From_SourceEventID
(OUT_idMag out number,
 IN_sAuthor varchar,
 IN_sSourceEventID varchar,
 IN_sMagType varchar
)
as
/* Return Codes for OUT_idMagnitude:
                  >0  DB idMagnitude
                  -1  Unknown Error
                  -2  IN_sMagType is Invalid Magnitude Type
                  -3  No matching magnitude record found
                  -4  Unknown NO_DATA_FOUND excep
                  -5  Unknown excep in retreiving iMagType
                  -6  Unknown excep in Get_idSource()
                  -7  Unknown excep in selecting idMag
                  -8  Magnitude Table not found in EWDB_Tablelist
                  -9  ExternalEvent Table not found in EWDB_Tablelist
                 -1X  Error from Get_idSource(), 
                       X indicates error returned by Get_idSource()

				  Others:  Undefined
*/
Temp_ID          number;
State            number := 0;
Temp_idMag       number;
Temp_iMagType   number;
Temp_idSource    number := 0;
Temp_tiMagnitude number := 0;
Temp_tiExternalEvent number := 0;
begin

  State := 0;

  select idTable into Temp_iMagType from EWDB_Tablelist
    where sTableName = IN_sMagType;
  State := 4;

  select idTable into Temp_tiMagnitude from EWDB_Tablelist
    where sTableName = 'Magnitude';
  State := 5;

  select idTable into Temp_tiExternalEvent from EWDB_Tablelist
    where sTableName = 'ExternalEvent';
  State := 1;


  Get_idSource(Temp_idSource,IN_sAuthor);
  if not(Temp_idSource > 0) then
   /* error, set return code and quit */
   OUT_idMag := -10 + Temp_idSource;
   return;
  end if;

  State := 2;

  select m.idmag into Temp_idMag
    from magnitude m, externalevent x, bind b1, bind b2
    where x.sSourceEventID = IN_sSourceEventID
      and x.idSource = Temp_idSource
      and x.idSource = m.idSource
      and m.iMagType = Temp_iMagType
      and x.idExternalEvent = b1.idCore
      and b1.tiCore = Temp_tiExternalEvent
      and m.idMag    = b2.idCore
      and b2.tiCore   = Temp_tiMagnitude
      and b1.idEvent  = b2.idEvent;


  State := 3;

  OUT_idMag := Temp_idMag;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      OUT_idMag := -2;   /* Invalid Magnitude Type */
    elsif State = 2 then
      OUT_idMag := -3;   /* Magnitude Record Not Found */
    elsif State = 4 then
      OUT_idMag := -8;   /* Magnitude Table Not found in EWDB_TableList */
    elsif State = 5 then
      OUT_idMag := -9;   /* ExternalEvent Table Not found in EWDB_TableList */
    else
      OUT_idMag := -4;   /* Unknown NO_DATA_FOUND Error */
    
    end if;
   
  WHEN OTHERS THEN
    if State = 0 then
      OUT_idMag := -5;
    elsif State = 1 then
      OUT_idMag := -6;
    elsif State = 2 then
      OUT_idMag := -7;
    else 
      OUT_idMag := -1;
    end if;

    Temp_ID:= SQLCODE;
    insert into test values('Get_idMag_F_SEID',Temp_idSource,Temp_ID);
END;
