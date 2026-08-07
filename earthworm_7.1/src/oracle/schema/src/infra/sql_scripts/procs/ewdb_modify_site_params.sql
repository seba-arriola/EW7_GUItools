/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Modify_Site_Params
 (
 OUT_iRetCode out number,
 IN_idSiteT number,
 IN_dLat number,
 IN_dLon number,
 IN_dElev number,
 IN_sComment varchar
)
 as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
      -2:     IN_idSiteT invalid.  Not found in DB.
      -3:     Unexpected NDF exception, see Debug Table

********************************************/
Temp           number;
State          number;
Temp_idComment number;


BEGIN

  State := 1;
  update sitet
    set dLat = IN_dLat,
     dLon = IN_dLon,
  dElev = IN_dElev
 where idSiteT = IN_idSiteT;

  if(IN_sComment IS NOT NULL) then
    if(IN_sComment != '') then
      /**********************************/
      /* Deal with Comment String      */
      /**********************************/
      State := 5;
      Create_Comment(Temp_idComment, IN_sComment);

      State := 6;
      update SiteT set idComment = Temp_idComment
       where idSiteT = IN_idSiteT;
    end if;
  end if;

  State := 7;
  OUT_iRetCode := 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      OUT_iRetCode := -2;
    else
      Temp := SQLCODE;
      insert into test values('Modify_SiteParams_NDF ' || IN_idSiteT, State, Temp);
      OUT_iRetCode := -3;  /* unknown NDF exception */
    end if;
  WHEN OTHERS THEN
      Temp := SQLCODE;
      insert into test values('Modify_SiteParams_Ex ' || IN_idSiteT, State, Temp);
      OUT_iRetCode := -1;

END;
