/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE PROCEDURE Split_SiteT
(
 OUT_iRetCode out number,
 OUT_idSiteT  out number,
 IN_idSiteT       number,
 IN_tSplit        number
)
as

/*****************************
     Return Codes (OUT_iRetCode):
      0   :  SUCCESS
     -1   :  UNKNOWN ERROR  : see debug table
     -2   :  idSiteT was not valid
     -3   :  Unexpected NDF error: see debug table
     -5   :  IN_tSplit was not within the tOn-tOff range 
	         of the given SiteT record
    -2X   :  Error in proc Create_SiteT().  X is the error
              returned by that proc.

*****************************/
Temp               number;
State              number;
Temp_RetCode       number;
Temp_SiteT         SiteT%ROWTYPE;

begin

  State := 0;

  select * into Temp_SiteT from SiteT 
   where idSiteT = IN_idSiteT;

  State := 1;

  if(Temp_SiteT.tOn >= IN_tSplit OR Temp_SiteT.tOff <= IN_tSplit) then
    OUT_iRetCode := -5;
    return;
  end if;

  State := 5;

  /* We have a valid idSiteT, so just do a stealth update. */
  update SiteT set tOff = IN_tSplit where idSiteT = IN_idSiteT;

  State := 6;

  /* Create the 2nd SiteT with the same params as the original. */
  Create_SiteT(Temp, Temp_SiteT.idSite, IN_tSplit, Temp_SiteT.tOff, 
               Temp_SiteT.dLat, Temp_SiteT.dLon, Temp_SiteT.dElev, NULL);
  if(Temp <= 0) then
    OUT_iRetCode := -20 + Temp;
    return;
  end if;

  /* Copy the comment from the original to the 2nd. */

  State := 7;

  update SiteT set idComment = Temp_SiteT.idComment where idSiteT = Temp;

  State := 8;

  OUT_iRetCode := 0;
  OUT_idSiteT := Temp;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then 
	  OUT_iRetCode :=-2;
	else
	  insert into test values('Split_SiteT_NDF ' || IN_tSplit ,State, IN_idSiteT);
	  OUT_iRetCode :=-3;
	end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
	  insert into test values('Split_SiteT_ex ' || IN_idSiteT,State,Temp);
	  insert into test values('Split_SiteT_ex2 ' || IN_idSiteT ,IN_tSplit,0);
    OUT_iRetCode := -1;
END;

