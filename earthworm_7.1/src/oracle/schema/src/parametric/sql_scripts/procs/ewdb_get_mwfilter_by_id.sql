/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE IT CVS "EDITED"
 *
 *    $Id: ewdb_get_mwfilter_by_id.sql,v 1.1 2005/03/23 06:21:17 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_get_mwfilter_by_id.sql,v $
 *     Revision 1.1  2005/03/23 06:21:17  davidk
 *     Added SQL functions for Mw
 *
 *
 *********************************************************************************
 *********************************************************************************/

CREATE OR REPLACE PROCEDURE Get_MwFilter_By_ID
(
 OUT_RetCode      OUT number,
 IN_idMwFilter        number,
 OUT_dLowCutHz    OUT number,
 OUT_dLowTaperHz  OUT number,
 OUT_dHighTaperHz OUT number,
 OUT_dHighCutHz   OUT number
)
as

Temp               number;
Temp_ID            number;
State              number;

BEGIN

  State := 1;

  select dLowCutHz, dLowTaperHz, dHighTaperHz, dHighCutHz 
   into OUT_dLowCutHz, OUT_dLowTaperHz, OUT_dHighTaperHz, OUT_dHighCutHz
   from MwFilter
   where idMwFilter = IN_idMwFilter;

  State := 2;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN 
    OUT_RetCode := -2;
	return;
  WHEN OTHERS THEN

    Temp := SQLCODE;
    OUT_RetCode := -1;
    insert into test values('Get_MwFilter_By_ID ' || IN_idMwFilter, State, Temp);
END;

