/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Assoc_Comp_w_Chan
(
 OUT_RetCode out number,
 OUT_idCompT out number,
 OUT_idChanT out number,
 IN_idComp       number,
 IN_idChan       number,
 IN_tStart       number,
 IN_tEnd         number
)
as

Temp_idChanT      number;
Temp_idCompT      number;
Temp              number;
Temp_sSta         varchar(10);
Temp_sComp        varchar(10);
Temp_sNet         varchar(10);
Temp_sLoc         varchar(10);
State             number;

begin

  State :=1;
  /**********************************/
  /* Verify that the Chan and Comp  */
  /* records exist.                 */
  /**********************************/
  Check_Record_Validity(Temp,IN_idComp,'Comp');
  if Temp <= 0 then
    OUT_RetCode := -2;
    return;
  end if;

  Check_Record_Validity(Temp,IN_idChan,'Chan');
  if Temp <= 0 then
    OUT_RetCode := -3;
    return;
  end if;

  State :=2;
  select s.sSta,s.sNet,c.sComp,c.sLoc 
    into Temp_sSta,Temp_sNet,Temp_sComp,Temp_sLoc
    from  site s, comp c
    where c.idComp=IN_idComp
      and s.idSite=c.idSite;

  State :=3;
  Set_Comp_Params(Temp_idCompT,Temp_sSta,Temp_sComp,Temp_sNet,Temp_sLoc,
                  IN_tStart, IN_tEnd, 0/*dLat*/, 0/*dLon*/, 0/*dElev*/,
                  0/*dAzm*/, 0/*dDip*/, NULL);
  if Temp_idCompT <= 0 then
    OUT_RetCode := -1000 + Temp_idCompT;
    return;
  end if;

  State :=4;
  Set_Chan_Params(Temp_idChanT,IN_idChan,Temp_idCompT,
                  IN_tStart,IN_tEnd,NULL,NULL,NULL);
  if Temp_idChanT <= 0 then
    OUT_RetCode := -2000 + Temp_idChanT;
    return;
  end if;


  OUT_idChanT := Temp_idChanT;
  OUT_idCompT := Temp_idCompT;
  OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Assoc_Comp_w_Chan',State,Temp);
    OUT_RetCode := -1;
END;

