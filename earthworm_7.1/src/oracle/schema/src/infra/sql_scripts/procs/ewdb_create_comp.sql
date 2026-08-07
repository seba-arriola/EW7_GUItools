/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Comp
(
 OUT_idComp OUT number,
 IN_idSite number,
 IN_sComp varchar,
 IN_sLoc varchar,
 IN_sComment varchar
)
as
/* Return Codes for OUT_idComp:
                  >0  DB idComp
                  -1  Unknown Error
                  -2  Comp Record with matching idSite/sComp/sLoc already exists
                  -3  Unknown DUP_VAL_ON_INDEX exception
                 -1X  Error in Create_Core_idKey(), where
                       X indicates error returned by that proc.

				  Others:  Undefined
*/


Temp_idComp        number;
Temp_idComment     number;
State              number;
Temp               number;

begin

  State := 0;
  
  /**********************************/
  /* Deal with Comment Strings      */
  /**********************************/
  if IN_sComment IS NULL then
    Temp_idComment := NULL;
  else
    Create_Comment(Temp_idComment, IN_sComment);
  end if;

  State := 1;

  /**********************************/
  /* Get A New CompID               */
  /**********************************/
  select CompSeq.NEXTVAL into Temp_idComp from sys.dual;
 
  Create_Core_idKey(Temp_idComp);
  if Temp_idComp <= 0 then
    OUT_idComp := -10 + Temp_idComp;
  end if;

  State := 2;

  insert into Comp(idComp,idSite,sComp,sLoc,idComment)
    values(Temp_idComp,IN_idSite,IN_sComp,IN_sLoc,Temp_idComment);

  State := 3;

  OUT_idComp := Temp_idComp;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    if State = 2 then
      /* Comp already exists */
      OUT_idComp := -2 ;
    else
      OUT_idComp := -3;   /* unknown DVOI Exception */
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Comp',Temp,State);
    OUT_idComp := -1;
END;


