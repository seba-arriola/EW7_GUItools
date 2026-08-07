/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Set_CompT_Params
(
 OUT_iRetCode OUT number,
 IN_idCompT number,
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
Temp_idComment     number;
Temp               number;
State              number;


begin

  State := 1;

  update CompT set dLat=IN_dLat, dLon=IN_dLon, dElev=IN_dElev, dAzm=IN_dAzm, dDip=IN_dDip
   where idCompT = IN_idCompT;

  OUT_iRetCode := 0;

  State := 2;

  if(IN_sComment IS NOT NULL) then
      /**********************************/
      /* Deal with Comment String      */
      /**********************************/
      State := 5;
      Create_Comment(Temp_idComment, IN_sComment);

      State := 6;
      update CompT set idComment = Temp_idComment
       where idCompT = IN_idCompT;
  end if;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if(State = 1) then
      OUT_iRetCode := -2;  /* invalid idCompT */
	  return;
	else
	  OUT_iRetCode := -3;  /* comment error */
	end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
	if(State = 1) then
      insert into test values('Set_CompT_Params_ex',Temp,IN_idCompT);
      OUT_iRetCode := -1;   /* Unknown error */
	else
      insert into test values('Set_CompT_Params_comm_ex',Temp,IN_idCompT);
      OUT_iRetCode := -4;   /* Unknown comment error */
	end if;


END;

