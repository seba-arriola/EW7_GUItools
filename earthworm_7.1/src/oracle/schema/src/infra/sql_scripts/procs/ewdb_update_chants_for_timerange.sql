/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_ChanTs_For_TimeRange
(
 OUT_RetCode out number,
 IN_tOff         number,
 IN_tOn          number,
 IN_idChan       number,
 IN_idCompT      number,
 IN_idDeviceSlot number,
 IN_iPlexor      number,
 IN_sComment     varchar,
 IN_bForce       number
)
as

/* Cursors */
Cursor ChanT_Cursor(
                    Cursor_idChan number,
                    Cursor_tOn number,
                    Cursor_tOff number
                   ) is
  select * from ChanT 
    where idChan=Cursor_idChan 
      and tOff > Cursor_tOn 
      and tOn  < Cursor_tOff
    order by tOn;

Temp               number;
State              number;
Temp_RetCode       number;

begin

  State := 0;

  for Curr in ChanT_Cursor(IN_idChan,IN_tOn,IN_tOff) loop
    State := 1;

    /* We have a valid idChanT, so just do a simple update. */
    Update_ChanT(Temp_RetCode, Curr.idChanT, NULL, IN_idDeviceSlot, IN_iPlexor, NULL, 0);
    if Temp_RetCode < 0 then
      OUT_RetCode := -10 + Temp_RetCode;
	    insert into test values('Update_ChanTs_For_TimeRange_uct',IN_tOn,IN_tOff);
      insert into test values('Update_ChanTs_For_TimeRange_uct2',IN_idChan,IN_bForce);
      return;
    end if;

  end loop;

  State := 2;

  OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
	  insert into test values('Update_ChanTs_For_TimeRange_ex',State,Temp);
	  insert into test values('Update_ChanTs_For_TimeRange_ex2',IN_tOn,IN_tOff);
	  insert into test values('Update_ChanTs_For_TimeRange_ex3',IN_idChan,IN_bForce);
    OUT_RetCode := -1;
END;

