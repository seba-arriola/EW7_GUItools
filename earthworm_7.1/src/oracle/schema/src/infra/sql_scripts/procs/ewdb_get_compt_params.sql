/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_CompT_Params
(
 OUT_iRetCode  OUT number,
 IN_idCompT        number,
 IN_idComp         number,
 IN_tOn            number,
 IN_tOff           number,
 OUT_idCompT OUT   number,
 OUT_idComp  OUT   number,
 OUT_dLat    OUT   number,
 OUT_dLon    OUT   number,
 OUT_dElev   OUT   number,
 OUT_dAzm    OUT   number,
 OUT_dDip    OUT   number,
 OUT_tOn     OUT   number,
 OUT_tOff    OUT   number,
 OUT_sSta    OUT   varchar,
 OUT_sComp   OUT   varchar,
 OUT_sNet    OUT   varchar,
 OUT_sLoc    OUT   varchar,
 OUT_sComment OUT  varchar
)
as

/*  errors
                 -1  Unknown Exception
                 -2   Both IN_idCompT and IN_idComp were invalid
                 -3   CompT record not found with matchin IN_idComp/IN_tOn-IN_tOff
                 -4   IN_idCompT not 0, Invalid
*/


Temp_idCompT       number;
Temp               number;
State              number;
Temp_bWarning      number := 0;
Temp_idComment     number := 0;

begin

  State := 1;

  if(IN_idCompT > 0) then
    Temp_idCompT := IN_idCompT;
  elsif(IN_idComp > 0) then
    State := 2;
    select count(idCompT) into Temp from CompT
     where tOn <= IN_tOff AND tOff > IN_tOn;
    if(Temp = 1) then
      select idCompT into Temp_idCompT  from CompT
       where tOn <= IN_tOff AND tOff > IN_tOn;
    elsif(Temp > 1) then
      Temp_bWarning := 1;
      select min(idCompT) into Temp_idCompT  from CompT
       where tOn <= IN_tOff AND tOff > IN_tOn;
    else
      OUT_idCompT := -3;  /* No CompT record found with overlapping tOn/tOff interval */
      return;
    end if;
  else
    OUT_iRetCode := -2;
    return;
  end if;

  State := 3;

  select idCompT, idComp, dLat, dLon, dElev, dAzm, dDip, 
         tOn, tOff, 
		 sSta, sComp, sNet, sLoc, idComment
    into OUT_idCompT, OUT_idComp, OUT_dLat, OUT_dLon, OUT_dElev, OUT_dAzm, OUT_dDip,
         OUT_tOn, OUT_tOff,
         OUT_sSta, OUT_sComp, OUT_sNet, OUT_sLoc, Temp_idComment
    from CompT 
    where idCompT = Temp_idCompT;

  State := 4;

  if(Temp_idComment IS NOT NULL AND Temp_idComment != 0) then
    select sComment into OUT_sComment from comments 
      where idComment = Temp_idComment;
  end if;

  OUT_iRetCode := Temp_bWarning;
  return;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 3 then
      insert into test values('Get_CompT_Params - 3',Temp_idCompT,State);
      OUT_idCompT := -4;  /* IN_idCompT must not've been valid*/
    else
      OUT_idCompT := -5;  /* Unknown NDF error */
      insert into test values('Get_CompT_Params - NDF',Temp_idCompT,State);
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_CompT_Params_Ex',Temp,State);
    OUT_idCompT := -1;  /* Unknown error */
end;

