/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Create_Comp_From_SCNL
(
 OUT_idComp out number,
 OUT_idSite out number,
 IN_sSta varchar,
 IN_sComp varchar,
 IN_sNet varchar,
 IN_sLoc varchar,
 IN_sComment varchar
)
as

/* RETURN CODES:
      -1:     Unknown Exception during Get_idSite
      -2:     Unknown Exception during Get_idComp
      -3:     Unknown Exception
     -1X:     Error in Get_idSite(), X indicates
              the error returned.
     -2X:     Error in Get_idComp(), X indicates
              the error returned.


********************************************/
Temp_idSite    number;
Temp_idComp    number;
State          number;
Temp           number;

begin

  State := 1;
  /**********************************/
  /* Get the ID for the Site        */
  /**********************************/
  Get_idSite(Temp, Temp_idSite,IN_sSta,IN_sNet,'');
  if Temp < 0 then
    /* Error */
    OUT_idComp := -10 + Temp_idSite;
    return;
  end if;

  State := 2;
  /**********************************/
  /* Get the ID for the Comp        */
  /**********************************/
  Get_idComp(Temp_idComp,Temp_idSite,IN_sComp,IN_sLoc,'');
  if Temp_idComp <= 0 then
    /* Error */
    OUT_idComp := -20 + Temp_idComp;
    return;
  end if;

  State :=3;

  OUT_idComp := Temp_idComp;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Comp_From_SCNL',Temp,State);
    OUT_idComp := 0-State;
END;
