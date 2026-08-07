/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE IT CVS "EDITED"
 *
 *    $Id: ewdb_get_mwfilter_by_props.sql,v 1.1 2005/03/23 06:21:17 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_get_mwfilter_by_props.sql,v $
 *     Revision 1.1  2005/03/23 06:21:17  davidk
 *     Added SQL functions for Mw
 *
 *
 *********************************************************************************
 *********************************************************************************/

CREATE OR REPLACE PROCEDURE Get_MwFilter_By_Props
(
 OUT_RetCode      OUT number,
 OUT_idMwFilter   OUT number,
 IN_dLowCutHz         number,
 IN_dLowTaperHz       number,
 IN_dHighTaperHz      number,
 IN_dHighCutHz        number
)
as

Temp               number;
Temp_ID            number;
State              number;

BEGIN
  State := 1;

  select idMwFilter into OUT_idMwFilter
   from MwFilter
   where dLowCutHz   = IN_dLowCutHz
     AND dLowTaperHz = IN_dLowTaperHz
     AND dHighTaperHz= IN_dHighTaperHz
     AND dHighCutHz  = IN_dHighCutHz;

  State := 2;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN 
    Create_MwFilter(Temp, Temp_ID, IN_dLowCutHz, IN_dLowTaperHz, IN_dHighTaperHz, IN_dHighCutHz);
    if(Temp = 0) THEN
      OUT_RetCode := 0;
      OUT_idMwFilter := Temp_ID;
    elsif(Temp = 1) THEN
      OUT_RetCode := 0;
      OUT_idMwFilter := Temp_ID;
    else
      OUT_RetCode := -2;
      OUT_idMwFilter := 0;
      insert into test values('Get_MwFilter_By_Props', Temp, Temp_ID);
    end if;

    return;
  WHEN OTHERS THEN

    Temp := SQLCODE;
    OUT_RetCode := -1;
    insert into test values('Get_MwFilter_By_Props_EX', State, Temp);
END;

