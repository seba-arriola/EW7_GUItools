/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_CompT
(
 OUT_RetCode out number,
 OUT_idCompT out number,
 IN_idComp       number,
 IN_tOn          number,
 IN_tOff         number,
 IN_dLat         number,
 IN_dLon         number,
 IN_dElev        number,
 IN_dAzm         number,
 IN_dDip         number,
 IN_sComment     varchar,
 IN_bForce       number
)
as

/* Cursors */
Cursor CompT_Cursor(
                    Cursor_idComp number,
                    Cursor_tOn number,
                    Cursor_tOff number
                   ) is
  select * from CompT 
    where idComp=Cursor_idComp 
      and tOff > Cursor_tOn 
      and tOn  < Cursor_tOff
    order by tOn;

Cursor ChanT_Cursor(Cursor_idCompT number) is
  select * from ChanT where idCompT=Cursor_idCompT;



New_idCompT        number;
Temp_tOff          number;
Temp_RetCode       number;
New_idCompT2       number;
Temp_tOn           number;
Temp_sSta          varchar(10) := NULL;
Temp_sComp         varchar(10);
Temp_sNet          varchar(10);
Temp_sLoc          varchar(10);
Temp_idComment     number;
Temp               number;
State              number;

BEGIN
begin
  if IN_bForce=1 then

    State := 1;
    /**********************************/
    /* Deal with Comment String      */
    /**********************************/
    if IN_sComment IS NULL then
      Temp_idComment := NULL;
    else
      Create_Comment(Temp_idComment, IN_sComment);
    end if;

    State := 2;
    /**********************************/
    /* Create new CompT Record        */
    /**********************************/
    select CompTSeq.nextval into new_idCompT from sys.dual;
    
    Create_Core_idKey(new_idCompT);
    if new_idCompT <= 0 then
      OUT_idCompT := new_idCompT;
      return;
    end if;

    insert into CompT(idCompT,idComp,tOff,tOn,
                      dLat,dLon,dElev,dAzm,dDip,idComment)
      values(new_idCompT,IN_idComp,-1,-1,IN_dLat,IN_dLon,IN_dElev,
              IN_dAzm,IN_dDip,Temp_idComment);

    State := 3;
    /**********************************/
    /* Find all CompT records with    */
    /* which we overlap               */
    /**********************************/
    for Curr in CompT_Cursor(IN_idComp,IN_tOn,IN_tOff) loop
      if CompT_Cursor%ROWCOUNT = 1 then
	      Temp_sSta  := Curr.sSta;
	      Temp_sComp := Curr.sComp;
	      Temp_sNet  := Curr.sNet;
	      Temp_sLoc  := Curr.sLoc;
      end if;

      /* don't overwrite the new one we just created */
      if Curr.idCompT != new_idCompT then

      State := 4;
	    if Curr.tOn < IN_tON then
        Temp_tOff := Curr.tOff;
        update CompT set tOff = IN_tOn where idCompT=Curr.idCompT;

        /**********************************/
        /* For each overlapping CompT 		*/ 
        /* record, adjust it's start and	*/
        /* end times and then make				*/
        /* neccessary adjustments to any	*/
        /* associated ChanT records.			*/
		/**********************************/
        State := 5;
        for CurrChanT in ChanT_Cursor(Curr.idCompT) loop

          /* Call the ChanT update function */
          Update_ChanT_For_Clipped_CompT(Temp_RetCode,CurrChanT.tOn,CurrChanT.toff,
                                         IN_tOn,IN_tOff,CurrChanT.idChanT,
                                         new_idCompT,CurrChanT.idDeviceSlot,CurrChanT.iPlexor);
        end loop; /* for CurrChanT */
        if(Temp_tOff > IN_tOff) then
          State := 6;
          select CompTSeq.nextval into New_idCompT2 from sys.dual;
    
          Create_Core_idKey(New_idCompT2);
          if New_idCompT2 <= 0 then
            OUT_idCompT := New_idCompT2;
            return;
          end if;

          insert into CompT(idCompT,idComp,tOff,tOn,sSta,sComp,sNet,sLoc,
                            dLat,dLon,dElev,dAzm,dDip,idComment)
            values(New_idCompT2,Curr.idComp,Temp_tOff,IN_tOff,Curr.sSta,Curr.sComp,
                   Curr.sNet,Curr.sLoc,Curr.dLat,Curr.dLon,Curr.dElev,
                   Curr.dAzm,Curr.dDip,Curr.idComment);

          State := 7;
          for CurrChanT in ChanT_Cursor(Curr.idCompT) loop

            /* Call the ChanT update function */
            Update_ChanT_For_Clipped_CompT(Temp_RetCode,CurrChanT.tOn,CurrChanT.toff,
                                           IN_tOff,Temp_tOff,CurrChanT.idChanT,
                                           new_idCompT,CurrChanT.idDeviceSlot,CurrChanT.iPlexor);
          end loop; /* CurrChanT*/
        end if; /* temp_toff > in_toff (if we split the existing record) */
      else /* curr.tOn < IN_tON  (the existing record starts at or after the new one */
        State := 8;
        if curr.tOff > IN_tOff then /* we are clipping the front of this record */
          State := 9;
          update CompT set tOn = IN_tOff where idCompT=curr.idCompT;
          State := 10;
          for CurrChanT in ChanT_Cursor(Curr.idCompT) loop

            /* Call the ChanT update function */
            Update_ChanT_For_Clipped_CompT(Temp_RetCode,CurrChanT.tOn,CurrChanT.toff,
                                           IN_tOn,IN_tOff,CurrChanT.idChanT,
                                           new_idCompT,CurrChanT.idDeviceSlot,CurrChanT.iPlexor);
          end loop; /* CurrChanT*/
        else /* we are overwriting the entire record */
          State := 11;
          update ChanT set idCompT=new_idCompT where idCompT=curr.idCompT;
          State := 12;
          delete CompT where idCompT=curr.idCompT;
        end if;
      end if; /* else curr.tOn < IN_tON  
                 (the existing record starts at or after the new one */
	  end if;  /* not the new row we're inserting */
    end loop;  /* for each CompT record that we overlap with */
    State := 13;
    /* if we went through the above loop atleast once,
       then Temp_sSta should have a valid value, otherwise
       it will be NULL, and we need to query for the SCNL
    ******************************************************/
    if Temp_sSta IS NULL then
      select s.sSta,s.sNet,c.sComp,c.sLoc 
        into Temp_sSta,Temp_sNet,Temp_sComp,Temp_sLoc
        from site s, comp c
        where c.idComp=IN_idComp
          and c.idSite=s.idSite;
    end if;  /* Temp_sSta is NULL */
    update CompT set tOn=IN_tOn,tOff=IN_tOff,
         sSta=Temp_sSta,sComp=Temp_sComp,sNet=Temp_sNet,sLoc=Temp_sLoc
      where idCompT=new_idCompT;

    OUT_idCompT :=new_idCompT;
  else  /* if(force) */
    State := 138;
    select count(idCompT) into new_idCompT from CompT 
      where idComp=IN_idComp and tOff > IN_tOn and tOn <IN_tOff;
	if(new_idCompT > 0) then
      State := 139;
      select min(idCompT) into new_idCompT from CompT 
        where idComp=IN_idComp and tOff > IN_tOn and tOn <IN_tOff;
	else
      State := 14;
	  /* get an NDF exception thrown, so that we run the NDF codeblock below.
	     THIS IS AN AWFUL HACK...  huh huh huh! */
      select idCompT into new_idCompT from CompT 
        where idComp=IN_idComp and tOff > IN_tOn and tOn <IN_tOff;
    end if;
    State := 15;
    select tOn,tOff into Temp_tOn,Temp_tOff
      from CompT where idCompT=new_idCompT;
    OUT_idCompT := new_idCompT;
    State := 16;
    if Temp_tOn > IN_tON or Temp_tOff < IN_tOff then
      OUT_RetCode := 1;
      return;
    end if;
  end if;
  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN /*select min(idCompT)*/
    if State = 14 then

      State := 25;
      /**********************************/
      /* Deal with Comment String      */
      /**********************************/
      if IN_sComment IS NULL then
        Temp_idComment := NULL;
      else
        Create_Comment(Temp_idComment, IN_sComment);
      end if;

      State := 26;
      select s.sSta,s.sNet,c.sComp,c.sLoc 
	      into Temp_sSta,Temp_sNet,Temp_sComp,Temp_sLoc
	      from Site s, Comp c
	      where c.idComp=IN_idComp
	        and c.idSite=s.idSite;

      State := 27;
      select CompTSeq.nextval into new_idCompT from sys.dual;
      Create_Core_idKey(new_idCompT);
      if new_idCompT <= 0 then
        OUT_idCompT := new_idCompT;
        return;
      end if;

      insert into CompT(idCompT,idComp,tOff,tOn,sSta,SComp,
	                      sNet,sLoc,dLat,dLon,dElev,dAzm,dDip)
        values(new_idCompT, IN_idComp, IN_tOff, IN_tOn, Temp_sSta, Temp_sComp,
	             Temp_sNet, Temp_sLoc, IN_dLat, IN_dLon, IN_dElev, IN_dAzm,IN_dDip);
      OUT_RetCode := 0;
      OUT_idCompT := New_idCompT;
    else
      Temp := SQLCODE;
	    insert into test values('Create_CompT_NDF_ex',State,Temp);
      OUT_RetCode := -2;
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
	  insert into test values('Create_CompT_ex',State,Temp);
          insert into test values('Create_CompT_ex ' || Temp,IN_tOn,IN_tOff);
    OUT_RetCode := -1;
END;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
	  insert into test values('Create_CompT_ex_ex',State,Temp);
    OUT_RetCode := -3;
END;

