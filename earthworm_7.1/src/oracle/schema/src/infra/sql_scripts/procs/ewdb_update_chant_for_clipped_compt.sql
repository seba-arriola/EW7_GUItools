/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */



CREATE OR REPLACE PROCEDURE Update_ChanT_For_Clipped_CompT
(
 OUT_RetCode OUT         number,
 tOn_OldChanT            number,
 tOff_OldChanT           number,
 tOn_NewCompT            number,
 tOff_NewCompT           number,
 idOldChanT              number,
 idNewCompT              number,
 idDeviceSlot_OldChanT   number,
 iPlexor_OldChanT        number
)
 
as

/***************************
     Return Values for OUT_RetCode:
              0 Success
             -1 Unknown Exception
***************************/

New_idChanT        number;
Temp_tChanT        number;
Temp               number;
Temp_idChan        number;
State              number;


begin

  State := 0;
  select idChan into Temp_idChan from ChanT where idChanT = idOldChanT;

  State := 1;

  if tOn_OldChanT < tOn_NewCompT then
    if tOff_OldChanT > tOn_NewCompT then
      /* this code is symmetric to that below */
      State := 2;
      select ChanTSeq.nextval into New_idChanT from sys.dual;
      Temp_tChanT := tOff_OldChanT;
      State := 3;
      update ChanT set tOff=tOn_NewCompT 
        where idChanT=idOldChanT;
      State := 4;
      insert into ChanT(idChanT,tOff,tOn,idCompT,idDeviceSlot,iPlexor)
        values(New_idChanT,Temp_tChanT,tOn_NewCompT,
               idNewCompT,idDeviceSlot_OldChanT,iPlexor_OldChanT);
      State := 5;
    /* else do nothing */
    end if;
      State := 6;
  else  /* tOn_NewCompT >= tOn_OldChanT */
    if tOff_OldChanT <= tOff_NewCompT then
      State := 7;
      update ChanT set idCompT=idNewCompT where idChanT=idOldChanT;
    else
      if tOn_OldChanT < tOff_NewCompT then
        /* this code is symmetric to that above */
        State := 8;
        select ChanTSeq.nextval into New_idChanT from sys.dual;
        Temp_tChanT := tOn_OldChanT;
        State := 9;
        update ChanT set tOn=tOff_NewCompT where idChant=idOldChanT;
        State := 10;
        insert into ChanT(idChanT,tOff,tOn,idCompT,idDeviceSlot,iPlexor)
          values(New_idChanT,tOff_NewCompT,Temp_tChanT,
                 idNewCompT,idDeviceSlot_OldChanT,iPlexor_OldChanT);
      /* else do nothing */
      end if;
    end if;
  end if;

  State := 11;

  OUT_RetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Update_ChanT_For_Clipped_CompT ' || idOldChanT,Temp,State);
    insert into test values('Update_ChanT_For_Clipped_CompT1 ' || idOldChanT,tOn_OldChanT,tOff_OldChanT);
    insert into test values('Update_ChanT_For_Clipped_CompT2 ' || idOldChanT,tOn_NewCompT,tOff_NewCompT);
    insert into test values('Update_ChanT_For_Clipped_CompT3 ' || idOldChanT,idNewCompT,idDeviceSlot_OldChanT);
    OUT_RetCode := -1;
END;
