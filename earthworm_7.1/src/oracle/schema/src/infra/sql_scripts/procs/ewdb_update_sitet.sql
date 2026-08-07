/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_SiteT
(
 OUT_iRetCode out number,
 IN_idSiteT number,
 IN_tOn number,
 IN_tOff number,
 IN_sComment varchar
)
 as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
      -2:     IN_idSiteT invalid.  Not found in DB.
      -3:     An existing SiteT record overlaps with new time range
                (matching idSite overlapping tOff/tOn range)
      -4:     Error processing Comment

********************************************/
Temp           number;
Temp_RetCode   number;
State          number;
Temp_idSite    number;
Temp_idComment number;
Temp_idSiteT   number;


BEGIN

  State := 1;
  select idSite into Temp_idSite
   from SiteT where idSiteT = IN_idSiteT;

  State := 2;
  select count(idSiteT) into Temp
   from SiteT 
   where idSite = Temp_idSite
     AND tOn  < IN_tOff
     AND tOff > IN_tOn;

  State := 21;
  if(Temp > 0) then
    State := 22;
    if(Temp > 1) then
      OUT_iRetCode := -3;
      return;
    else
      State := 23;
      select idSiteT into Temp_idSiteT
       from SiteT 
       where idSite = Temp_idSite
         AND tOn  < IN_tOff
         AND tOff > IN_tOn;

      State := 24;
      if(Temp_idSiteT != IN_idSiteT) then
        OUT_iRetCode := -3;
        return;
      end if;
    end if;
  end if;


  State := 3;

  update SiteT set tOn = IN_tOn, tOff=IN_tOff
    where idSiteT = IN_idSiteT;

  State := 4;

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

  OUT_iRetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    if State = 1 then
      OUT_iRetCode := -2;
    elsif State = 5 OR State = 6 then
      OUT_iRetCode := -4;
    else 
      Temp := SQLCODE;
      insert into test values('UpdateCTF_Excep',State,Temp);
      OUT_iRetCode := -1;
    end if;

END;
