/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE UpdateRulePolygons
(IN_idOldPolygon number,
 IN_idNewPolygon number
)
as
/* Return Codes for OUT_idRule:
                  >0  DB idRule
                  -1  Unknown Error
*/
Temp			number;

begin

  /**************************************************/
  /* Check to see if this rule already exists       */
  /**************************************************/

	update AlarmsRule
		set AlarmsRule.idPolygon = IN_idNewPolygon
		where idPolygon = IN_idOldPolygon;
			
EXCEPTION
  WHEN OTHERS THEN
  Temp := SQLCODE;
  insert into test values('UpdateRulePolygons ' || IN_idOldPolygon, IN_idNewPolygon, Temp);
end;
