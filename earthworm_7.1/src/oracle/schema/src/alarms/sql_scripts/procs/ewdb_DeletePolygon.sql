/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeletePolygon
(OUT_RetCode OUT number,
 IN_idPolygon number
)
as

Temp  number;

begin

	delete Polygon where idPolygon=IN_idPolygon; 
	delete Polygon_Vert where idPolygon=IN_idPolygon;

	OUT_RetCode := 0;


EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeletePolygon',IN_idPolygon,Temp);
		OUT_RetCode := 0 - Temp;
END;

