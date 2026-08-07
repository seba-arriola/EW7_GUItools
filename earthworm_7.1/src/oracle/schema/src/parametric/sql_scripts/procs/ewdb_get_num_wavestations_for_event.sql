/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*
 *    $Id: ewdb_get_num_wavestations_for_event.sql,v 1.1 
 *
 *    Revision history:
 *     $Log: ewdb_get_num_wavestations_for_event.sql,v $
 *     Revision 1.1  2001/09/27 23:55:01  davidk
 *     Initial revision
 *
 *
 *
 */

CREATE OR REPLACE PROCEDURE GET_NUM_WAVESTATIONS_FOR_EVENT 
(OUT_iNumStations OUT NUMBER,
 IN_idEvent NUMBER
)
as

/* Return Codes for OUT_iRetCode:
          >0  Success
          -1  Unknown Error
				  Others:  Undefined
*/

begin

  select count(ssta) into OUT_iNumStations 
  from
    (
     select asi.ssta
     from waveformdesc wd, bind b, all_station_info asi 
     where b.idevent = IN_idEvent
       and b.ticore=27
       and b.idcore = wd.idwaveform
       and wd.idchan=asi.idchan
     group by asi.ssta, asi.snet
    );

  return;
EXCEPTION
  WHEN OTHERS THEN
    OUT_iNumStations := -1;
end;


