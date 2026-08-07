/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_SiteT_Params
(
 OUT_iRetCode OUT number,
 IN_idSiteT        number,
 IN_idSite         number,
 IN_tOn            number,
 IN_tOff           number,
 OUT_idSiteT OUT   number,
 OUT_idSite  OUT   number,
 OUT_dLat    OUT   number,
 OUT_dLon    OUT   number,
 OUT_dElev   OUT   number,
 OUT_tOn     OUT   number,
 OUT_tOff    OUT   number,
 OUT_sSta    OUT   varchar,
 OUT_sNet    OUT   varchar,
 OUT_sComment OUT  varchar
)
as

/*  errors
                 -1  Unknown Exception
                 -2   Both IN_idSiteT and IN_idSite were invalid
                 -3   SiteT record not found with matchin IN_idSite/IN_tOn-IN_tOff
                 -4   IN_idSiteT not 0, Invalid
*/


Temp_idSiteT       number;
Temp               number;
State              number;
Temp_bWarning      number := 0;
Temp_idComment     number := 0;

begin

  State := 1;

  if(IN_idSiteT > 0) then
    Temp_idSiteT := IN_idSiteT;
  elsif(IN_idSite > 0) then
    State := 2;
    select count(idSiteT) into Temp from SiteT
     where tOn <= IN_tOff AND tOff > IN_tOn;
    if(Temp = 1) then
      select idSiteT into Temp_idSiteT from SiteT
       where tOn <= IN_tOff AND tOff > IN_tOn;
    elsif(Temp > 1) then
      Temp_bWarning := 1;
      select min(idSiteT) into Temp_idSiteT from SiteT
       where tOn <= IN_tOff AND tOff > IN_tOn;
    else
      OUT_iRetCode := -3;  /* No SiteT record found with overlapping tOn/tOff interval */
      return;
    end if;
  else
    OUT_iRetCode := -2;
    return;
  end if;

  State := 3;

  select idSiteT, idSite, dLat, dLon, dElev, tOn, tOff, sSta, sNet, idComment
    into OUT_idSiteT, OUT_idSite, OUT_dLat, OUT_dLon, OUT_dElev, OUT_tOn, OUT_tOff,
         OUT_sSta, OUT_sNet, Temp_idComment
    from ALL_SITET_INFO
    where idSiteT = Temp_idSiteT;

  if(Temp_idComment IS NOT NULL AND Temp_idComment != 0) then
    select sComment into OUT_sComment from comments 
      where idComment = Temp_idComment;
  end if;
  
  OUT_iRetCode := 0;
  return;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 3 then
      insert into test values('Get_SiteT_Params - 3',Temp_idSiteT,State);
      OUT_iRetCode := -4;  /* IN_idSiteT must not've been valid*/
    else
      OUT_iRetCode := -5;  /* Unknown NDF error */
      insert into test values('Get_SiteT_Params - NDF',Temp_idSiteT,State);
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_SiteT_Params_Ex',Temp,State);
    OUT_iRetCode := -1;  /* Unknown error */
end;