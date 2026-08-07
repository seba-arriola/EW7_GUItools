/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Set_Comp_Params
(
 OUT_idCompT OUT number,
 IN_sSta varchar,
 IN_sComp varchar,
 IN_sNet varchar,
 IN_sLoc varchar,
 IN_tOn number,
 IN_tOff number,
 IN_dLat number,
 IN_dLon number,
 IN_dElev number,
 IN_dAzm number,
 IN_dDip number,
 IN_sComment varchar
)
as

/* errors
    -1X Get_idSite
    -2X Get_idComp
   -1XX Create_CompT
     -1 Unknown Error

*/
Temp_idCompT       number;
Temp_New_idCompT   number;
Temp_tOn           number;
Temp_tOff          number;
Temp_idSite        number;
Temp_idComp        number;
Temp_idComment     number;
Temp_RetCode       number;
Temp               number;
State              number;


Cursor ChanT_cursor(Cursor_idCompT number) is
  select * from ChanT where idCompT=Cursor_idCompT;

begin

  State := 1;

  /**********************************/
  /* Get the ID for the Site        */
  /**********************************/
  Get_idSite(Temp, Temp_idSite,IN_sSta,IN_sNet,'');
  /* Do not continue unless Get_idSite returned 0 or 1 */
  if Temp < 0 then
    /* Error */
    OUT_idCompT := -10 + Temp;
    return;
  elsif Temp = 1 then
    /* a new Site was created, we need to create a new SiteT */
    Set_Site_Params(Temp, IN_sSta, IN_sNet, IN_tOn, IN_tOff, IN_dLat, IN_dLon, IN_dElev, IN_sComment);
    /* ignore the return code */
  elsif Temp > 0 then
    /* unexepected return code */
    OUT_idCompT:= Temp;
    Temp := SQLCODE;
    insert into test values('Set_Comp_Params:Get_idSiteT',Temp,OUT_idCompT);
    OUT_idCompT:= -10;
    return;
  end if;

  State := 2;

  /**********************************/
  /* Get the ID for the Comp        */
  /**********************************/
  Get_idComp(Temp_idComp,Temp_idSite,IN_sComp,IN_sLoc,'');
  if Temp_idComp <= 0 then
    /* Error */
    OUT_idCompT := -20 + Temp_idComp;
    return;
  end if;

  State := 3;

  Create_CompT(Temp_RetCode,Temp_idCompT,Temp_idComp,IN_tOn,IN_tOff,
               IN_dLat,IN_dLon,IN_dElev,IN_dAzm,IN_dDip,IN_sComment,1/*FORCE*/);
    if Temp_RetCode < 0 then
      /* Error */
      OUT_idCompT := -100 + Temp_RetCode;
      return;
    end if;

  State := 7;

/*   We assume that our new information is more 
       correct and up to date than the info in the DB.
    **********************************************/
 
  OUT_idCompT := Temp_idCompT;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Set_Comp_Params_ex',Temp,State);
    OUT_idCompT := -1;   /* Unknown error */

END;

