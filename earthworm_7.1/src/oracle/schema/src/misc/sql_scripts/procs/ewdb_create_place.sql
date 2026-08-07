
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE   
     CHECKED IT OUT USING THE COMMAND CHECKOUT.  

    $Id: ewdb_create_place.sql,v 1.2 2003/06/10 14:39:04 lucky Exp $

    Revision history:
     $Log: ewdb_create_place.sql,v $
     Revision 1.2  2003/06/10 14:39:04  lucky
     *** empty log message ***

     Revision 1.1  2002/08/26 16:44:31  davidk
     Initial revision

     Revision 1.6  2002/03/22 20:52:55  davidk
     pre v6.1 checkin

*/


CREATE OR REPLACE PROCEDURE Create_Place
(OUT_idPlace out number,
 IN_sState in varchar,
 IN_sPlaceName in varchar,
 IN_dLat in number,
 IN_dLon in number,
 IN_dElev in number,
 IN_iPopulation in number,
 IN_iPlaceMajorType in number,
 IN_iPlaceMinorType number,
 IN_sCounty varchar,
 IN_sCountry varchar
)
as

/* Return Codes for OUT_idPlace:
                  >0  Success
                  -1  Unknown Error
                  -2  Problem creating core ID Key
                   1  Record with matchin place name already exists

				  Others:  Undefined
*/
Temp_idPlace     number;
State            number;
Temp             number;
begin
begin

  State := 0;

  /**********************************/
  /* Get A New MagID.              */
  /**********************************/
  select PlaceSeq.NEXTVAL into Temp_idPlace from sys.dual;

  Create_Core_idKey(Temp_idPlace);
  if Temp_idPlace <= 0 then
    OUT_idPlace := -2;
  end if;

  State := 1;

  insert into Place(idPlace, sState, sName, dLat, dLon, dElev, iPopulation,
                    iPlaceMajorType, iPlaceMinorType, sCounty, sCountry)
    values(Temp_idPlace, IN_sState, IN_sPlaceName, IN_dLat, IN_dLon,
			IN_dElev, IN_iPopulation, IN_iPlaceMajorType, IN_iPlaceMinorType, 
			IN_sCounty, IN_sCountry);

  State := 2;

  OUT_idPlace := Temp_idPlace;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    if State = 1 then
      /* must be a duplicate name/lat/lon */
      select idPlace into OUT_idPlace from Place
        where sName = IN_sPlaceName
          and dLat  = IN_dLat
          and dLon  = IN_dLon;
    else
      insert into test values('Create_Place_DVOI '|| IN_sPlaceName,State,Temp_idPlace);
      OUT_idPlace := -1;
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Place',State,Temp);
	  OUT_idPlace := -1;
END;
EXCEPTION
  WHEN OTHERS THEN 
    Temp := SQLCODE;
    insert into test values('Create_Place_x ' || IN_sPlaceName,State,Temp);
	  OUT_idPlace := -1;
END;
