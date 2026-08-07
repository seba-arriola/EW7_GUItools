/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE IT CVS "EDITED"
 *
 *    $Id: ewdb_create_mwfilter.sql,v 1.1 2005/03/23 06:21:17 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_create_mwfilter.sql,v $
 *     Revision 1.1  2005/03/23 06:21:17  davidk
 *     Added SQL functions for Mw
 *
 *
 *********************************************************************************
 *********************************************************************************/

CREATE OR REPLACE PROCEDURE Create_MwFilter
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

  /**********************************/
  /* Get A New MwFilterID           */
  /**********************************/
  select MwFilterSeq.NEXTVAL into Temp_ID from sys.dual;

  State := 2;

  Create_Core_idKey(Temp_ID);
  if Temp_ID <= 0 then
    OUT_idMwFilter := 0;
    OUT_RetCode := Temp_ID;
    return;
  end if;

  State := 3;

  /**********************************/
  /* Insert new MwFilter Record    */
  /**********************************/
  insert into MwFilter(idMwFilter, dLowCutHz, dLowTaperHz, dHighTaperHz, dHighCutHz)
         values       (Temp_ID, IN_dLowCutHz, IN_dLowTaperHz, IN_dHighTaperHz, IN_dHighCutHz);

  /**********************************/
  /* Set the  return value          */
  /**********************************/
  State := 4;

  OUT_idMwFilter := Temp_ID;
  OUT_RetCode    := 0;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN 
    select  count(idMwFilter) into Temp 
     from MwFilter
     where dLowCutHz   = IN_dLowCutHz
       AND dLowTaperHz = IN_dLowTaperHz
       AND dHighTaperHz= IN_dHighTaperHz
       AND dHighCutHz  = IN_dHighCutHz;

    if(Temp = 1) THEN
      select idMwFilter into OUT_idMwFilter
       from MwFilter
       where dLowCutHz   = IN_dLowCutHz
         AND dLowTaperHz = IN_dLowTaperHz
         AND dHighTaperHz= IN_dHighTaperHz
         AND dHighCutHz  = IN_dHighCutHz;
      OUT_RetCode := 0;
      return;
    else
      /* Some Bizarre error where we can't insert the filter and can't
         retrieve it.  Maybe somebody changed the fields/unique indexes
         from what we're expecting
       **************************************************************/
      OUT_RetCode := -1;
      insert into test values('Create_MwFilter_DV_EX', State, Temp);
    END IF;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    OUT_RetCode := -1;
    insert into test values('Create_MwFilter_EX', State, Temp);
END;

