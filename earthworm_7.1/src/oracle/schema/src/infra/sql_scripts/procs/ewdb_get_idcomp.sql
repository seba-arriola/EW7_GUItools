/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_idComp
(
 OUT_idComp OUT number,
 IN_idSite number,
 IN_sComp varchar,
 IN_sLoc varchar,
 IN_sComment varchar
)
as

Temp               number;

begin
  /**********************************/
  /* Get the ID for the Comp        */
  /**********************************/
  if IN_sLoc IS NULL then
    select idComp into OUT_idComp from Comp 
      where idSite = IN_idSite
        and sComp  = IN_sComp
        and sLoc IS NULL;
  else
    select idComp into OUT_idComp from Comp 
      where idSite = IN_idSite
        and sComp  = IN_sComp
        and sLoc =  IN_sLoc;
  end if;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    Create_Comp(OUT_idComp,IN_idSite,IN_sComp,IN_sLoc,IN_sComment);

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_idComp',Temp,0);
    OUT_idComp := -1;
END;
