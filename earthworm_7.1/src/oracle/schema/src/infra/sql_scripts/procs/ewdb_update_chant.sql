/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Update_ChanT
(
 OUT_RetCode out number,
 IN_idChanT      number,
 IN_idCompT      number,
 IN_idDeviceSlot number,
 IN_iPlexor      number,
 IN_sComment     varchar,
 IN_bForce       number
)
as

Temp_idComment     number;
Temp               number;
State              number;
Temp_idChanT       number;

begin
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

  select idChanT into Temp_idChanT
   from ChanT
   where idChanT = IN_idChanT;
  
  State := 3;

  if IN_bForce=1 then

    State := 4;
    update ChanT
     set idCompT=IN_idCompT, idDeviceSlot=IN_idDeviceSlot,
         iPlexor=IN_iPlexor, idComment = Temp_idComment
     where idChanT = IN_idChanT;
  else

    State := 5;

    if IN_idCompT IS NOT NULL then
      update ChanT
       set idCompT = IN_idCompT;
    end if;

    State := 6;

    if IN_idDeviceSlot IS NOT NULL then
      update ChanT
       set idDeviceSlot=IN_idDeviceSlot, iPlexor=IN_iPlexor;
    end if;

    State := 7;

    if Temp_idComment IS NOT NULL then
      update ChanT
       set idComment = Temp_idComment;
    end if;

  end if;  /* bForce != 1 */

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 2 then   /*select idChanT=IN_idChanT*/
      /**********************************/
      /* Invalid idChanT                */
      /**********************************/
      OUT_RetCode := -2;
      return;
    else
      Temp := SQLCODE;
	    insert into test values('Update_ChanT_NDF_ex',State,Temp);
	    insert into test values('Update_ChanT_NDF_ex',IN_idChanT,0);
      OUT_RetCode := -1;
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
	    insert into test values('Update_ChanT_ex',State,Temp);
	    insert into test values('Update_ChanT_ex',IN_idChanT,0);
    OUT_RetCode := -1;
END;

