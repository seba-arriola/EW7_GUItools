/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Set_Site_Params
(
 OUT_idSiteT OUT number,
 IN_sSta varchar,
 IN_sNet varchar,
 IN_tOn number,
 IN_tOff number,
 IN_dLat number,
 IN_dLon number,
 IN_dElev number,
 IN_sComment varchar
)
as


/* errors
    -1X Get_idSite
    -1XX Create_SiteT
    -2XX Create_SiteT
    -3XX Create_SiteT
     -2 Trying to update pre-record data.  Too complex! 
     -3 Trying to update  partial record data.  Too complex! 
     -4 Unknown NO_DATA_FOUND exception
     -1 Unknown Error

*/
Temp_idSiteT       number;
Temp_tOn           number;
Temp_tOff          number;
Temp_idSite        number;
Temp_idComment     number;

Temp               number;
State              number;

begin

  State := 1;

  /**********************************/
  /* Get the ID for the Site        */
  /**********************************/
  Get_idSite(Temp, Temp_idSite,IN_sSta,IN_sNet,'');
  if Temp < 0 then
    /* Error */
    OUT_idSiteT := -10 + Temp_idSite;
    return;
  end if;

  State := 2;

  /**********************************/
  /* Look for a relevant existing record.
  /**********************************/
  select idSiteT,tOn,tOff into Temp_idSiteT,Temp_tOn,Temp_tOff 
    from SiteT 
    where idSite = Temp_idSite 
      and tOff = 
          (select min(tOff) from SiteT 
            where idSite = Temp_idSite 
              and tOff   > IN_tOn 
          );

  State := 3;

  if Temp_tOn >= IN_tOff then
    /* Our record is completely before the closest relevant record */
    /* So, just create a new record. */

    State := 4;

    Create_SiteT(Temp_idSiteT,Temp_idSite,IN_tOn,IN_tOff,
                 IN_dLat,IN_dLon,IN_dElev,IN_sComment);
    if Temp_idSiteT <= 0 then
      /* Error */
      OUT_idSiteT := -100 + Temp_idSiteT;
      return;
    end if;


  elsif Temp_tOn > IN_tOn then
    OUT_idSiteT := -2; /* Trying to update pre-record data.  Too complex! */
    return;

  elsif Temp_tOff > IN_tOff then
    OUT_idSiteT := -3;  /* Trying to update  partial record data.  Too complex! */
    return;

  elsif Temp_tOn = IN_tOn then

    State := 0;

    /**********************************/
    /* Deal with Comment Strings      */
    /**********************************/
    if IN_sComment IS NULL then
      Temp_idComment := 0;
    else
      Create_Comment(Temp_idComment, IN_sComment);
    end if;

    State := 5;

    /* We have to update the existing SiteT record to 
       reflect the new params 
    ***********************/
    if Temp_idComment = 0 then
      update SiteT 
        set tOff=IN_tOff, dLat=IN_dLat, dLon=IN_dLon, dElev=IN_dElev
        where idSiteT = Temp_idSiteT;
    else  /* new comment */
      update SiteT 
        set tOff=IN_tOff, dLat=IN_dLat, dLon=IN_dLon, dElev=IN_dElev,
          idComment = Temp_idComment
        where idSiteT = Temp_idSiteT;
    end if;
  else

    State := 6;

    /* There is an existing SiteT record with which we overlap.
       The record starts before our new record starts, and ends 
       before our new record ends.  We will truncate the existing 
       record at our start time, so that it no longer overlaps 
       with us.  We assume that our new information is more 
       correct and up to date than the info in the DB.
    **********************************************/
    update SiteT 
      set tOff=IN_tON
      where idSiteT = Temp_idSiteT;

    State := 7;

    Create_SiteT(Temp_idSiteT,Temp_idSite,IN_tOn,IN_tOff,
                 IN_dLat,IN_dLon,IN_dElev,IN_sComment);
    if Temp_idSiteT <= 0 then
      /* Error */
      OUT_idSiteT := -200 + Temp_idSiteT;
      return;
    end if;

    State := 8;
  end if;  /* testing the closest relevant SiteT record */

  
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 2 then
      /* No relevant SiteT records found, 
         just create a new record 
      **********************************/
      Create_SiteT(Temp_idSiteT,Temp_idSite,IN_tOn,IN_tOff,
                   IN_dLat,IN_dLon,IN_dElev,IN_sComment);
      if Temp_idSiteT <= 0 then
        /* Error */
        OUT_idSiteT := -300 + Temp_idSiteT;
        return;
      end if;

      OUT_idSiteT := Temp_idSiteT;

    else
      Temp := SQLCODE;
      insert into test values('Set_Site_Params_ex',Temp,State);
      OUT_idSiteT := -4;   /* Unknown NDF exception */
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Set_Site_Params_ex',Temp,State);
    OUT_idSiteT := -1;   /* Unknown error */

END;

