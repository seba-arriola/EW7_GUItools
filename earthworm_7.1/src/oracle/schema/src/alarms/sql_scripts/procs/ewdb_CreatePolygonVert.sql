/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_PolygonVert
(OUT_idVertex out number,
 IN_idPolygon number,
 IN_dLat number,
 IN_dLon number,
 IN_iOrder number
)
as
/* Return Codes for OUT_idVertex:
                  >0  DB idVertex
                  -1  Unknown Error

				  Others:  Undefined
*/
Temp_ID          number;
State            number;

begin

  /**********************************/
  /* Get A New OPID.              */
  /**********************************/
  select PolygonVertSeq.NEXTVAL into Temp_ID from sys.dual;

  Create_Core_idKey(Temp_ID);
  if Temp_ID <= 0 then
    OUT_idVertex := Temp_ID;
	return;
  end if;

  State := 1;

  /**********************************/
  /* Insert new Polygon Record    */
  /**********************************/
  insert into Polygon_Vert(idVertex,idPolygon,dLat,dLon,iOrder)
    values(Temp_ID, IN_idPolygon, IN_dLat, IN_dLon, IN_iOrder);

  /**********************************/
  /* Set the idOP return value      */
  /**********************************/

  OUT_idVertex := Temp_ID;

EXCEPTION

 WHEN OTHERS THEN
  OUT_idVertex := -1;
END;

