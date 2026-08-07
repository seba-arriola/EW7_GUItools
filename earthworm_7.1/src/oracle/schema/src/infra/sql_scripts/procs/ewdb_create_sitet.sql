/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_SiteT
(
 OUT_idSiteT OUT number,
 IN_idSite number,
 IN_tOn number,
 IN_tOff number,
 IN_dLat number,
 IN_dLon number,
 IN_dElev number,
 IN_sComment varchar
)
as

  /***********************************
    Return Values (OUT_idSiteT):
      >0    Success
      -1    Unknown Error, see Debug Table
      -2    Duplicate Record already exists
      -3    Overlapping SiteT record already exists
               Matching idSite, overlapping tOn/tOff
     -1X    Problem creating Unique idSiteT
  ***********************************/


Temp_idSiteT       number;
Temp               number;
Temp_idComment     number;
State              number;
begin

  select count(idSiteT) into Temp
   from SiteT
  where idSite = IN_idSite
    AND tOn  < IN_tOff
    AND tOff > IN_tOn;

  if Temp > 0 then
    OUT_idSiteT := -3;
    return;
  end if;

  /**********************************/
  /* Deal with Comment String      */
  /**********************************/
  if IN_sComment IS NULL then
    Temp_idComment := NULL;
  else
    Create_Comment(Temp_idComment, IN_sComment);
  end if;

  /**********************************/
  /* Get A New SiteID.              */
  /**********************************/
  select SiteTSeq.NEXTVAL into Temp_idSiteT from sys.dual;
 
  Create_Core_idKey(Temp_idSiteT);
  if Temp_idSiteT <= 0 then
    OUT_idSiteT := -10 + Temp_idSiteT;
  end if;


  insert into SiteT(idSiteT,idSite,tOff,tOn,dLat,dLon,dElev,idComment)
    values(Temp_idSiteT,IN_idSite,IN_tOff,IN_tON,IN_dLat,IN_dLon,IN_dElev,Temp_idComment);

  OUT_idSiteT := Temp_idSiteT;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    OUT_idSiteT := -2;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_SiteT',Temp,0);
    insert into test values('Create_SiteT1_' || IN_tOff, IN_idSite,IN_tOn);
    OUT_idSiteT := -1;
END;
