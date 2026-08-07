/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE PROCEDURE Create_ChanT
(
 OUT_RetCode out number,
 OUT_idChanT out number,
 IN_idChan       number,
 IN_tOn          number,
 IN_tOff         number,
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

New_idChanT        number;
Temp_tOff          number;
Temp_RetCode       number;
New_idChanT2       number;
Temp_tOn           number;
Temp_idComment     number;
Temp               number;
State              number;

BEGIN

begin
  State := 1;
  if IN_bForce=1 then
    State := 2;
    /**********************************/
    /* Deal with Comment String      */
    /**********************************/
    if IN_sComment IS NULL then
      Temp_idComment := NULL;
    else
      Create_Comment(Temp_idComment, IN_sComment);
    end if;

    State := 3;

    /**********************************/
    /* Get New idChanT                */
    /**********************************/
    select ChanTSeq.nextval into new_idChanT from sys.dual;
    
    Create_Core_idKey(new_idChanT);
    if new_idChanT <= 0 then
      OUT_RetCode := -10 + new_idChanT;
      return;
    end if;

    State := 103;

    /**********************************/
    /* Insert new ChanT record, with  */
    /* faux times.                    */
    /**********************************/
    insert into ChanT(idChanT,idChan,idDeviceSlot,iPlexor,tOff,tOn,idCompT)
      values (New_idChanT,IN_idChan,IN_idDeviceSlot,IN_iPlexor,-1,-1,IN_idCompT);

    State := 4;

    /**********************************/
    /* Loop through all the records   */
    /* that overlap with the newly    */
    /* inserted one.                  */
    /**********************************/
    for Curr in ChanT_Cursor(IN_idChan,IN_tOn,IN_tOff) loop
      State := 5;
      /* if this record starts before the new one, then
         update it so that it ends at the start of the
         new one. 
      *********************************/
      if Curr.tOn < IN_tON then
        Temp_tOff := Curr.tOff;
        update ChanT set tOff = IN_tOn where idChanT=Curr.idChanT;

        State := 6;

        /* if this record also ends after the new one, then
           we have to segment it, because the new record
           effectively splits it, so that old_part1 < new <
           old_part2
        *********************************/
        if(Temp_tOff > IN_tOff) then
          select ChanTSeq.nextval into New_idChanT2 from sys.dual;
          Create_Core_idKey(New_idChanT2);
          if New_idChanT2 <= 0 then
            OUT_RetCode := -20 + New_idChanT2;
            return;
          end if;

          insert into ChanT(idChanT,idChan,tOff,tOn,idCompT,idDeviceSlot,iPlexor,idComment)
            values(New_idChanT2,Curr.idChan,Temp_tOff,IN_tOff,Curr.idCompT,
                   Curr.idDeviceSlot,Curr.iPlexor,Curr.idComment);
        end if; /* temp_toff > in_toff (if we split the existing record) */
      else /* curr.tOn < IN_tON  (the existing record starts at or after the new one */
        if curr.tOff > IN_tOff then /* we are clipping the front of this record */
          /* we know that tOnOld  >= tOnNew 
                      and tOnOld  < tOffNew
                      and tOffOld > tOffNew
             so we know that we are clipping off the front of the old record 
          ******************************************/
          State := 7;

          update ChanT set tOn = IN_tOff where idChanT=curr.idChanT;
        else /* we are overwriting the entire record */
          /* this is dangerous, overwriting an existing chanT record!
             we need to be sure that we document the danger of using
             the force flag in the procedure doc
          ***************************************/
          State := 8;
          if(Delete_ChanT(curr.idChanT)!=0) THEN
            insert into test values('Create_ChanT(): Delete_ChanT', curr.idChanT, IN_idCompT);
            OUT_RetCode := -8;
            return;
          END IF;
          /* delete ChanT where idChanT=curr.idChanT; */
        end if;
      end if; /* else curr.tOn < IN_tON  (the existing record starts at or after the new one */
    end loop;  /* for each ChanT record that we overlap with */
    State := 9;
    update ChanT set tOn=IN_tOn,tOff=IN_tOff /*,params=IN_params*/
      where idChanT=new_idChanT;
    OUT_idChanT :=new_idChanT;
  else  /* if(force) */
    State := 10;
    select min(idChanT) into new_idChanT from ChanT 
      where idChan=IN_idChan and tOff > IN_tOn and tOn <IN_tOff;
    select tOn,tOff into Temp_tOn,Temp_tOff
      from ChanT where idChanT=new_idChanT;
    OUT_idChanT := new_idChanT;
    if Temp_tOn > IN_tON or Temp_tOff < IN_tOff then
      OUT_RetCode := 1;
      return;
    end if;
  end if;
  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN /*select min(idChanT)*/
    if State = 10 then
      /**********************************/
      /* Deal with Comment String      */
      /**********************************/
      if IN_sComment IS NULL then
        Temp_idComment := NULL;
      else
        Create_Comment(Temp_idComment, IN_sComment);
      end if;

      select ChanTSeq.nextval into new_idChanT from sys.dual;
    
      Create_Core_idKey(new_idChanT);
      if new_idChanT <= 0 then
        OUT_RetCode := -30 + new_idChanT;
        return;
      end if;

      insert into ChanT(idChanT,idChan,tOff,tOn,idCompT,idDeviceSlot,iPlexor)
        values(new_idChanT, IN_idChan, IN_tOff, IN_tOn, 
               IN_idCompT, IN_idDeviceSlot,IN_iPlexor);
      OUT_RetCode := 0;
      OUT_idChanT := New_idChanT;
    else
      Temp := SQLCODE;
	    insert into test values('Create_ChanT_NDF_ex',State,Temp);
      OUT_RetCode := -2;
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
	  insert into test values('Create_ChanT_ex',State,Temp);
    OUT_RetCode := -1;
END;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
	  insert into test values('Create_ChanT_ex_ex',State,Temp);
    OUT_RetCode := -3;
END;

