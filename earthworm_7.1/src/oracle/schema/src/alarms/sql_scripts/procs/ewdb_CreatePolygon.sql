CREATE OR REPLACE PROCEDURE Create_Polygon
(OUT_idPolygon out number,
 IN_sPolygonName varchar
)
as
/* Return Codes for OUT_idPolygon:
                  >0  DB idPolygon
                  -1  Unknown Error

				  Others:  Undefined
*/
Temp_ID          Number;
State            Number;

begin

  /**********************************/
  /* Get A New OPID.              */
  /**********************************/
  select PolygonSeq.NEXTVAL into Temp_ID from sys.dual;

  Create_Core_idKey(Temp_ID);
  if Temp_ID <= 0 then
    OUT_idPolygon := Temp_ID;
	return;
  end if;

  State := 1;

  /**********************************/
  /* Insert new Polygon Record    */
  /**********************************/
  insert into Polygon(idPolygon,sPolygonName)
    values(Temp_ID, IN_sPolygonName);

  /**********************************/
  /* Set the idOP return value      */
  /**********************************/

  OUT_idPolygon := Temp_ID;

EXCEPTION

 WHEN OTHERS THEN
  OUT_idPolygon := -1;
END;

