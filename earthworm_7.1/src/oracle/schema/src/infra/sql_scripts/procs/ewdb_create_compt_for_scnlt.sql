/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_CompT_For_SCNLT
(OUT_idChan out number,
 IN_sSta varchar,
 IN_sComp varchar,
 IN_sNet varchar,
 IN_sLoc varchar,
 IN_tStart number,
 IN_tEnd number
)
as

Temp_idChan       number;
Temp_idChanT      number;
Temp_idCompT      number;
Temp              number;



/* Return Codes for OUT_idChan:

   > 0   Success, the idChan of the newly created channel.
    -1   Unknown Error.  See debug table for details.
    -2   Atleast one idchan already exists with a matching 
         SCNL and an overlapping T.  This function is only 
         for use when NO idchans exist for the given SCNL 
         and Time interval!!  Try using XXX() instead?
   -1X   Error returned by Create_Chan(), where X indicates the
         error returned by that function.
 -1XXX   Error returned by Set_Comp_Params(), where XXX indicates 
         the error returned by that function.
 -2XXX   Error returned by Set_Chan_Params(), where XXX indicates 
         the error returned by that function.

******************************************************************/

begin

  /**********************************/
  /* Verify there are no existing   */
  /* idChan's for the time interval */
  /**********************************/
  /* DK 02/20/2001  Added a NULL check for sLoc */
  select count(idChan) into Temp
   from ALL_STATION_INFO 
   where sSta =   IN_sSta
     and sComp =  IN_sComp
     and sNet =   IN_sNet
     and (sLoc =  IN_sLoc or (sLoc IS NULL and IN_sLoc is NULL))
     and tOn <=   IN_tEnd
     and tOff >=  IN_tStart;

  if Temp > 0 then
    /* Atleast one idchan already exists.  This function
       is only for use when NO idchans exist for the given
       SCNL and Time interval!!  Return Error */
    OUT_idChan := -2;
    return;
  end if;

  /**********************************/
  /* Create A New Chan              */
  /**********************************/
  Create_Chan(Temp_idChan,'Created for '
              || IN_sSta || ', ' || IN_sComp || ', '
              || IN_sNet || ', ' || IN_sLoc 
              || 'by Create_CompT_For_SCNLT()');
  if Temp_idChan <= 0 then
    OUT_idChan := -10 + Temp_idChan;
    return;
  end if;

  Set_Comp_Params(Temp_idCompT,IN_sSta,IN_sComp,IN_sNet,IN_sLoc,
                  IN_tStart, IN_tEnd, 0/*dLat*/, 0/*dLon*/, 0/*dElev*/,
                  0/*dAzm*/, 0/*dDip*/, NULL);
  if Temp_idCompT <= 0 then
    OUT_idChan := -1000 + Temp_idCompT;
    return;
  end if;

  Set_Chan_Params(Temp_idChanT,Temp_idChan,Temp_idCompT,
                  IN_tStart,IN_tEnd,NULL,NULL,NULL);
  if Temp_idChanT <= 0 then
    OUT_idChan := -2000 + Temp_idChanT;
    return;
  end if;


  OUT_idChan := Temp_idChan;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_CompT_For_SCNLT()',0,Temp);
    OUT_idChan := -1;
END;

